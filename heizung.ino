#include "avr/wdt.h"
#include "SwitchController.h"
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <SD.h>
#include <SPI.h>
#include <Streaming.h>
#include <Time.h>
#include <OneWire.h>
#include <Wire.h>

#include "Actor.h"
#include "Commands.h"
#include "Controller.h"
#include "Defines.h"
#include "Job.h"
#include "Network.h"
#include "NTPClient.h"
#include "Sensors.h"
#include "SingleSourceTwoLevelController.h"
#include "TemperatureProfiles.h"
#include "TemperatureUploader.h"
#include "Tools.h"
#include "Watchdog.h"
#include "Webserver.h"

#ifdef ACTORS_COMMTYPE_CRYSTAL64IO
#include <IOshield.h>
#endif // ACTORS_COMMTYPE_CRYSTAL64IO

// TODO / Fehler
// - Minimale Bodentemperatur / Fusswarmer Boden: Minimales Level im Actor einbauen. Diesen im EEPROM ablegen.
// - SRAM sparen:
//   - strcmp -> strcmp_P
//   - Datenstruktur Sensors überarbeiten (-> PROGMEM)

void (*delayedCmd)() =0;

void setup(){
  // disable any watchdog as soon as possible...
  wdt_disable();

  Serial.begin( 19200 );
  Serial << F( "--- SETUP STARTING ---" ) << endl;
#ifdef SUPPORT_eBus
  Serial1.begin( 2400 );
#endif
#ifdef SUPPORT_Network
  setupNetwork();
#endif
  setupSensors();
  setupActors();
  setupController();
  Serial << F( "Initializing status LED." ) << endl;
  pinMode( 13, OUTPUT );
#ifdef SUPPORT_TemperatureUploader
  Serial << F( "Initializing temperature uploader." ) << endl;
  ( new TemperaturesUploader() )->setup( 0, 1 );
#endif // SUPPORT_TemperatureUploader
  Serial << F( "--- SETUP FINISHED ---" ) << endl;
  cmdHelp( Serial, Serial );
}

void interpret( Stream& in, Stream& out ){
  char cmd[ 256 ];
  read( in, cmd, sizeof( cmd ) );
  int idx = 0;
  StringStream s;
  commandText( s, true );
  while ( commandText( s ) ){
    if ( !strcmp( cmd, s.c_str() ) ){
      commands[ idx ]( in, out );
      return;
    }
    commandText( s );
    commandText( s );
    s.clear();
    ++idx;
  }
  out << F("Unknown command '") << cmd << F("'.") << endl;
}

void loop(){
  digitalWrite( 13, HIGH );

  startWatchdog();

  DEBUG{ Serial << F("FreeRam: ") << FreeRam() << endl; }

  // listen to serial port for commands
  if ( Serial.available() ){
    DEBUG{ Serial << F("interpret START") << endl; }
    interpret( Serial, Serial );
    DEBUG{ Serial << F("interpret END") << endl; }
  }

  if ( delayedCmd ){
    delayedCmd();
    delayedCmd = 0;
  }
  
  // process all jobs (sensors/controllers/actors)
  unsigned long minDelay = Job::executeJobs( 50 );
  
  Actor::applyActorsStates();

#ifdef SUPPORT_Webserver
  DEBUG{ Serial << F( "execWebserver START" ) << endl; }
  execWebserver();
  DEBUG{ Serial << F( "execWebserver END" ) << endl; }
#endif

  stopWatchdog();

  digitalWrite( 13, LOW );
#ifndef SUPPORT_eBus
  DEBUG{ Serial << F("minDelay=") << minDelay << endl; }
  delay( minDelay );
#endif
}

#if defined(SUPPORT_Network) && defined(SUPPORT_eBus)
char hex( unsigned char c ){
  if ( c < 10 ){
    return '0' + c;
  }
  return 'A' + c - 10;
}

void serialEvent1(){
  Udp.beginPacket( 0xffffffff, 12888 );
  Udp << "eBus: ";
  while ( Serial1.available() ){
    unsigned char c = (unsigned char)Serial1.read();
    Udp << hex(c>>4) << hex(c&0xf) << ' ';
  }
  Udp.endPacket();
}
#endif
