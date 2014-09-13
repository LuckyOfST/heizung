#include <EEPROM.h>
#include <Time.h>

#include "Controller.h"
#include "SingleSourceTwoLevelController.h"
#include "TemperatureProfiles.h"

// elements = new XXXController( ... )
// Each controller uses one or more temperature sensors and one actor to reach the target temperature of the controller.
// Implemented controllers:
//   SingleSourceTwoLevelController( name, sensor, actor ): This Controller is a very simple two-point-controller without
//   histersis function. It simply turns the actor on if the temperature is below the target temperature and turns the actor
//   off otherwise.
//     "sensor" is the index in g_sensors of the sensor used as current temperature source
//     "actor" is the index in g_actors of the actor to be controlled by this controller.
Controller* g_controller[ CONTROLLER_COUNT + 1 ] = {
  new SingleSourceTwoLevelController( 0, 0, 0 ),
  new SingleSourceTwoLevelController( 1, 1, 1 ),
  new SingleSourceTwoLevelController( 2, 2, 2 ),
  new SingleSourceTwoLevelController( 3, 3, 3 ),
  new SingleSourceTwoLevelController( 4, 4, 4 ),
  new SingleSourceTwoLevelController( 5, 5, 5 ),
  new SingleSourceTwoLevelController( 6, 6, 6 ),
  new SingleSourceTwoLevelController( 7, 7, 7 ),
  new SingleSourceTwoLevelController( 8, 8, 8 ),
  new SingleSourceTwoLevelController( 9, 9, 9 ),
  new SingleSourceTwoLevelController( 10, 10, 10 ),
  new SingleSourceTwoLevelController( 11, 11, 11 ),
  new SingleSourceTwoLevelController( 12, 12, 12 ),
  new SingleSourceTwoLevelController( 13, 13, 13 ),
  new SingleSourceTwoLevelController( 14, 14, 14 ),
  new SingleSourceTwoLevelController( 15, 15, 15 ),
  new SingleSourceTwoLevelController( 16, 16, 16 ),
  new SingleSourceTwoLevelController( 17, 17, 17 ),
  new SingleSourceTwoLevelController( 18, 18, 18 ),
  new SingleSourceTwoLevelController( 19, 19, 19 ),
  new SingleSourceTwoLevelController( 20, 20, 20 ),
  new SingleSourceTwoLevelController( 21, 21, 21 ),
  new SingleSourceTwoLevelController( 22, 22, 22 ),
  new SingleSourceTwoLevelController( 23, 23, 23 ),
  0 // define 0 as last entry!
};

char g_cnC0[] PROGMEM = "C0"; // DG
char g_cnC1[] PROGMEM = "C1"; // OG Bad
char g_cnC2[] PROGMEM = "C2"; // OG Badheizkörper
char g_cnC3[] PROGMEM = "C3"; // OG WC
char g_cnC4[] PROGMEM = "C4"; // OG Arbeiten
char g_cnC5[] PROGMEM = "C5"; // OG Eltern
char g_cnC6[] PROGMEM = "C6"; // OG Kind 2
char g_cnC7[] PROGMEM = "C7"; // OG Kind 3
char g_cnC8[] PROGMEM = "C8"; // OG Flur / Diele
char g_cnC9[] PROGMEM = "C9"; // UG Lüftung Technik
char g_cnC10[] PROGMEM = "C10"; // EG Garderobe
char g_cnC11[] PROGMEM = "C11"; // EG Bad
char g_cnC12[] PROGMEM = "C12"; // EG Spielen
char g_cnC13[] PROGMEM = "C13"; // EG Wohnen
char g_cnC14[] PROGMEM = "C14"; // EG Küche
char g_cnC15[] PROGMEM = "C15"; // EG Flur / Diele
char g_cnC16[] PROGMEM = "C16"; // UG Lüftung Bad
char g_cnC17[] PROGMEM = "C17"; // UG Vorrat
char g_cnC18[] PROGMEM = "C18"; // UG Bad
char g_cnC19[] PROGMEM = "C19"; // UG Gäste
char g_cnC20[] PROGMEM = "C20"; // UG Hobby
char g_cnC21[] PROGMEM = "C21"; // UG Keller
char g_cnC22[] PROGMEM = "C22"; // UG Waschen
char g_cnC23[] PROGMEM = "C23"; // UG Flur / Diele

PGM_P g_controllerNames[ CONTROLLER_COUNT ] PROGMEM = {
  g_cnC0,
  g_cnC1,
  g_cnC2,
  g_cnC3,
  g_cnC4,
  g_cnC5,
  g_cnC6,
  g_cnC7,
  g_cnC8,
  g_cnC9,
  g_cnC10,
  g_cnC11,
  g_cnC12,
  g_cnC13,
  g_cnC14,
  g_cnC15,
  g_cnC16,
  g_cnC17,
  g_cnC18,
  g_cnC19,
  g_cnC20,
  g_cnC21,
  g_cnC22,
  g_cnC23
};

Controller::Controller( uint8_t id )
:_id( id )
,_forcedLevel( -1 )
{
}
  
const char* Controller::getName() const{
  strcpy_P( (char*)g_buffer, (char*)pgm_read_word(&(g_controllerNames[ _id ])) );
  return (char*)g_buffer;
}
  
void Controller::setup( int i, int amount ){
  Job::setup( i, amount );
  _delayMillis = (unsigned long)( (float)CONTROLLER_UPDATE_INTERVAL / amount * i );
}

uint8_t Controller::getProfileID() const{
  return EEPROM.read( EEPROM_TEMP_BASE + _id * 3 );
}
  
void Controller::setProfileID( uint8_t id ){
  EEPROM.write( EEPROM_TEMP_BASE + _id * 3, id );
}

void Controller::setTargetT( float t ){
  DEBUG{ Serial << F("setTargetT to ") << t << endl; }
  if ( t == 0.f ){
    // intentionally no action: this case is used to work in profiled mode!
  } else if ( t < 5.f ){
    t = 5.f; // minimum temperature (anti-frost)
  } else if ( t > 25 ){
    t = 25.f; // maximum temperature (protect wooden floors)
  }
  int targetT = t * 10;
  DEBUG{ Serial << F("setTargetT to (int) ") << targetT << endl; }
  EEPROM.write( EEPROM_TEMP_BASE + _id * 3 + 1, targetT >> 8 );
  EEPROM.write( EEPROM_TEMP_BASE + _id * 3 + 2, targetT & 0xff );
}
  
float Controller::getTargetT() const{
  int targetT = EEPROM.read( EEPROM_TEMP_BASE + _id * 3 + 1 ) * 256 + EEPROM.read( EEPROM_TEMP_BASE + _id * 3 + 2 );
  return targetT / 10.f;
}

float Controller::getT() const{
  float targetT = getTargetT();
  if ( targetT == 0 ){
    TemperatureProfiles::setCurrentProfile( getProfileID() );
    time_t t = now();
    byte day = weekday( t ) - 2;
    if ( day == -1 ){
      day = 6;
    }
    return TemperatureProfiles::temp( day, hour( t ), minute( t ) );
  }
  return targetT;
}
  
void Controller::writeSettings( Stream& s ){
  s << getTargetT();
}

void Controller::readSettings( Stream& s ){
  setTargetT( s.parseFloat() );
}
  
bool Controller::working() const{
  return false;
}
  
void Controller::setForcedLevel( float level ){
  _forcedLevel = level;
  _delayMillis = 0;
}

unsigned long Controller::specialFunctions( float t ){
  // forced level
  if ( _forcedLevel != -1 ){
    DEBUG{ Serial << "force level -------------------------------------------------------" << endl; }
    heat( _forcedLevel );
    _lastMillis = millis();
    return 1000ul * 10;
  }
  
  // frost protection
  if ( t < 5.f ){
    heat( 1.f );
    return 1000ul * 60 * 5;
  }
  
  return 0;
}

Controller* Controller::find( const char* name ){
  for( int i = 0; i < CONTROLLER_COUNT; ++i ){
    if ( !strcmp_P( name, (PGM_P)pgm_read_word(&g_controllerNames[ i ] )) ){
      return g_controller[ i ];
    }
  }
  return 0;
}

void setupController(){
  Serial << F( "Initializing controller." ) << endl;
  Job::setupJobs( (Job**)g_controller );
}


