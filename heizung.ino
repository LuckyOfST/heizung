#include <DallasTemperature.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <SD.h>
#include <SPI.h>
#include <Streaming.h>
#include <Time.h>
#include <OneWire.h>

#include "Actor.h"
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
#include "Webserver.h"

// TODO / Fehler
// - Unterstützung DHT Hygrometer / Temperatursensor
// - Steuerung der Temperatur über die Woche (Profile)
//   - minimale Temperatur beschränken
//   - Vorausschauende Auswertung von Profilen, um Temperatur zu Zielzeitpunkt zu erreichen.
// - Hysterese des Zweipunktreglers
// - Regler mit mehreren Sensoren
// - PID-Regler
// - Minimale Bodentemperatur / Fusswarmer Boden: Minimales Level im Actor einbauen. Diesen im EEPROM ablegen.
// - SRAM sparen:
//   - strcmp -> strcmp_P
//   - Datenstruktur Sensors überarbeiten (-> PROGMEM)

void help( Stream& out ){
  out << F("Supported Commands:") << endl;
  out << F("'detect': Detect all missing and new devices (sensors).") << endl;
  out << F("'errors': Show the error count for all sensors.") << endl;
  out << F("'reseterrors': Resets the error count for all sensors.") << endl;
  out << F("'status': Shows the current temperatures of all sensors.") << endl;
  out << F("'set CID T': Sets the target temperature of controller CID to T°C.") << endl;
  out << F("'resetControllerTemperatures': Resets the target temperature of all controllers to 21°C.") << endl;
  out << F("'resetControllerProfiles': Resets all controller profiles to 0 (no profile).") << endl;
  out << F("'setProfile CID P': Sets the profile for controller CID to profile nr. P (0 for no profile).") << endl; 
  out << F("'get CID': Shows the current target temperature of controller CID.") << endl;
  out << F("'getProfiles': Shows the profiles settings.") << endl;
  out << F("'setProfiles XXX': Sets the profiles (see documentation for format details).") << endl;
  out << F("'getTemp PID DOW H M': Gets the temperature set in profile PID for Day-Of-Week (0-6) DOW, Time H:M.") << endl;
  out << F("'forceActor AID X': forces the actor AID (or 'all') to bo on (X=1) or off (X=0) or to work in standard mode (X=-1).") << endl;
  out << F("'forceController CID X': forces the controller CID to set the actors' level to X [0..1]. Set X to -1 to work in standard operating mode.") << endl;
  out << F("'setTime DD MM YY H M S': Sets the time to day (DD), month (MM), year (YY); hour (H) minute (M) second (S).") << endl;
  out << F("'getTime': Shows the current time.") << endl;
  out << F("'requestTemp': Updates all sensors and sends their values via UDP (port 12888).") << endl;
}

void setup(){
  Serial.begin( 19200 );
  Serial << F( "--- SETUP STARTING ---" ) << endl;
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
  help( Serial );
}

const char* readText( Stream& s ){
  static char text[ 256 ];
  byte length = s.readBytesUntil( ' ', text, sizeof( text ) / sizeof( char ) );
  text[ length ] = 0;
  return text;
}

void read( Stream& s, char* buffer, unsigned short bufferSize ){
  unsigned short i = 0;
  while ( s.available() && i < bufferSize - 1 ){
    char c = s.read();
    if ( c == 10 || c == 13 || c == 32 ){
      if ( i == 0 ){
        continue;
      }
      break;
    }
    buffer[ i ] = c;
    ++i;
  }
  buffer[ i ] = 0;
  while ( s.available() ){
    char c = s.peek();
    if ( c != 10 || c != 13 || c != 32 ){
      break;
    }
  }
}

void (*delayedCmd)() =0;

void interpret( Stream& stream, Stream& out ){
  //const char* cmd = readText();
  char cmd[ 256 ];
  read( stream, cmd, sizeof( cmd ) );
  if ( !strcmp( cmd, "help" ) ){
    help( out );
    
  } else if ( !strcmp( cmd, "detect" ) || !strcmp( cmd, "d" ) ){
    out << F("Detection started...") << endl;
    detectSensors( out );
    out << F("Detection finished.") << endl;
    
  } else if ( !strcmp( cmd, "errors" ) ){
    writeSensorsErrors( out );
    
  } else if ( !strcmp( cmd, "reseterrors" ) ){
    for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
      getSensor( i )._errorCount = 0;
    }
    out << F("Error counters resetted.") << endl;
    
  } else if ( !strcmp( cmd, "status" ) ){
    writeTime( out );
    out << F("Begin status") << endl;
    out << F("  SENSORS:") << endl;
    for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
      Sensor& s = getSensor( i );
      if ( !s.valid() ){
        out << F("    '") << s._name << F("' invalid.") << endl;
        continue;
      }
      s.update( true );
      out << F("    '") << s._name << F("': ") << _FLOAT( s._temp, 1 ) << F(" C (") << s._errorCount << F(" errors)") << endl;
    }
    out << F("  CONTROLLERS:") << endl;
    for( unsigned short i = 0; i < CONTROLLER_COUNT; ++i ){
      out << F("    '") << g_controller[ i ]->getName() << F("' ");
      if ( !g_controller[ i ]->working() ){
        out << F("[OUT OF FUNCTION] ");
      }
      if ( g_controller[ i ]->getForcedLevel() == -1 ){
        out << F("set to ");
        if ( g_controller[ i ]->isProfileActive() ){
         out << F("profile ") << g_controller[ i ]->getProfileID() << F(", currently ");
        }
        out << _FLOAT( g_controller[ i ]->getT(), 1 ) << F(" C.") << endl;
      } else {
        out << F("forced to level ") << g_controller[ i ]->getForcedLevel() << '.' << endl;
      }
      
      /*if ( g_controller[ i ]->working() ){
        out << F("set to ");
        if ( g_controller[ i ]->isProfileActive() ){
         out << F("profile ") << g_controller[ i ]->getProfileID() << F(", currently ");
        }
        out << _FLOAT( g_controller[ i ]->getT(), 1 ) << F(" C.") << endl;
      } else {
        out << F("out of function. (set to ") << _FLOAT( g_controller[ i ]->getTargetT(), 1 ) << F(" C.) with profile ") << g_controller[ i ]->getProfileID() << endl;
      }*/
    }
    out << F("  TEMPERATURE PROFILES:") << endl;
    out << F("    ");
    //g_tempProfile.writeSettings( out );
    TemperatureProfiles::writeSettings( out );
    out << F("  CURRENT AMPERAGE: ") << Actor::getCurrentUsedAmperage() << endl;
    out << F("  FREE RAM: ") << FreeRam() << F(" bytes") << endl;
    out << F("End status") << endl;
    
  } else if ( !strcmp( cmd, "set" ) ){
    const char* name = readText( stream );
    Controller* c = Controller::find( name );
    if ( c ){
      float oldT = c->getTargetT();
      c->readSettings( stream );
      out << F("Changed controller '") << name << F("' from ") << _FLOAT( oldT, 1 ) << F(" C to ") << _FLOAT( c->getTargetT(), 1 ) << F(" C target temperature. (0 means profile mode)") << endl;
    } else {
      out << F("Unable to find controller '") << name << F("'.") << endl;
    }
    
  } else if ( !strcmp( cmd, "resetControllerTemperatures" ) ){
    for( int i = 0; i < CONTROLLER_COUNT; ++i ){
      g_controller[ i ]->setTargetT( 21. );
    }
    out << F("All controller target temperatures resetted to 21C.") << endl;
    
  } else if ( !strcmp( cmd, "resetControllerProfiles" ) ){
    for( int i = 0; i < CONTROLLER_COUNT; ++i ){
      g_controller[ i ]->setProfileID( 0 );
    }
    out << F("All controller temperature profiles resetted to profile 0.") << endl;
    
  } else if ( !strcmp( cmd, "setProfile" ) ){
    const char* name = readText( stream );
    Controller* c = Controller::find( name );
    if ( c ){
      int i = stream.parseInt();
      c->setProfileID( i );
      out << F("Profile for controller '") << name << F("' set to profile ") << i << endl;
    }
    
  } else if ( !strcmp( cmd, "get" ) ){
    const char* name = readText( stream );
    Controller* c = Controller::find( name );
    if ( c ){
      out << F("Target temperature for controller '") << name << F("' is set to ") << _FLOAT( c->getTargetT(), 1 ) << F(" C.") << endl;
    } else {
      out << F("Unable to find controller '") << name << F("'.") << endl;
    }
    
  } else if ( !strcmp( cmd, "getProfiles" ) ){
    TemperatureProfiles::writeSettings( out );
    
  } else if ( !strcmp( cmd, "setProfiles" ) ){
    TemperatureProfiles::readSettings( stream );
    
  } else if ( !strcmp( cmd, "getTemp" ) ){
    int id = stream.parseInt();
    int dow = stream.parseInt();
    int h = stream.parseInt();
    int m = stream.parseInt();
    TemperatureProfiles::setCurrentProfile( id );
    out << _FLOAT( TemperatureProfiles::temp( dow, h, m ), 1 ) << endl;
    
  //} else if ( !strcmp( cmd, "files" ) ){
  //  out << F("Files on SD card") << endl;
    
  } else if ( !strcmp( cmd, "forceActor" ) ){
    const char* name = readText( stream );
    out << F( "Actor " ) << name;
    int m;
    if ( !strcmp( name, "all" ) ){
      m = stream.parseInt() + 1;
      for( int i = 0; i < ACTOR_COUNT; ++i ){
        g_actors[ i ]->setMode( (Actor::Mode)m );
      }
    } else {
      Actor* a = Actor::findActor( name );
      if ( !a ){
        out << F( " not found." ) << endl;
        return;
      }
      m = stream.parseInt() + 1;
      a->setMode( (Actor::Mode)m );
    }
    switch ( m ){
    case 0:
      out << F( " set to standard operating mode." ) << endl;
      break;
    case 1:
      out << F( " forced to be OFF." ) << endl;
      break;
    case 2:
      out << F(" forced to be ON." ) << endl;
      break;
    }
    
  } else if ( !strcmp( cmd, "forceController" ) ){
    const char* name = readText( stream );
    out << F( "Controller " ) << name;
    Controller* c = Controller::find( name );
    if ( !c ){
      out << F( " not found." ) << endl;
      return;
    }
    float level = stream.parseFloat();
    c->setForcedLevel( level );
    if ( level == -1 ){
      out << F( " set to standard operating mode." ) << endl;
    } else {
      out << F(" forced to level " ) << level << '.' << endl;
    }
    
  } else if ( !strcmp( cmd, "setTime" ) ){
    int d = stream.parseInt();
    int mo = stream.parseInt();
    int y = stream.parseInt();
    int h = stream.parseInt();
    int m = stream.parseInt();
    int s = stream.parseInt();
    setTime( h, m, s, d, mo, y);
    out << F("Time set to ")  << lz(day()) << '.' << lz(month()) << '.' << year() << ' ' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second()) << endl;
    
  } else if ( !strcmp( cmd, "getTime" ) ){
    writeTime( out );

  } else if ( !strcmp( cmd, "requestTemp" ) ){
    out << F("Request received...") << endl;
    delayedCmd = &requestTemp;
    
  } else {
    out << F("Unknwon command '") << cmd << F("'.") << endl;
  }
}

void loop(){
  digitalWrite( 13, HIGH );

  DEBUG{ Serial << F("FreeRam: ") << FreeRam() << endl; }

#ifdef SUPPORT_Webserver
  DEBUG{ Serial << F("execWebserver START") << endl; }
  execWebserver();
  DEBUG{ Serial << F("execWebserver END") << endl; }
#endif

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

  digitalWrite( 13, LOW );
  DEBUG{ Serial << F("minDelay=") << minDelay << endl; }
  delay( minDelay );
}

