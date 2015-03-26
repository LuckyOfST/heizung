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
  new SingleSourceTwoLevelController( 0, 24, 22 ),
  new SingleSourceTwoLevelController( 1, 18, 9 ),
  new SingleSourceTwoLevelController( 2, -1, -1 ), // N/A
  new SingleSourceTwoLevelController( 3, 19, 23 ),
  new SingleSourceTwoLevelController( 4, 20, 15 ),
  new SingleSourceTwoLevelController( 5, 21, 13 ),
  new SingleSourceTwoLevelController( 6, 16, 12 ),
  new SingleSourceTwoLevelController( 7, 17, 11 ),
  new SingleSourceTwoLevelController( 8, 23, 10 ),
  new SingleSourceTwoLevelController( 9, 22, 14 ),
  new SingleSourceTwoLevelController( 10, 9, 18 ),
  new SingleSourceTwoLevelController( 11, 11, 21 ),
  new SingleSourceTwoLevelController( 12, 12, 20 ),
  new SingleSourceTwoLevelController( 13, 13, 17 ), // EG_Wohnen / 2 sensors (13+14)!
  new SingleSourceTwoLevelController( 14, 15, 16 ),
  new SingleSourceTwoLevelController( 15, 10, 19 ), // EG_Flur_Diele / 2 sensors (10 + 8)
  new SingleSourceTwoLevelController( 16, 4, 1 ), // UG_Flur_Diele / 2 sensors (4 + 3)
  new SingleSourceTwoLevelController( 17, 5, 5 ), 
  new SingleSourceTwoLevelController( 18, 6, 4 ),
  new SingleSourceTwoLevelController( 19, 1, 3 ),
  new SingleSourceTwoLevelController( 20, 2, 2 ),
  new SingleSourceTwoLevelController( 21, 7, 0 ),
  new SingleSourceTwoLevelController( 22, 0, 8 ),
  new SingleSourceTwoLevelController( 23, 3, 6 ),
  new SingleSourceTwoLevelController( 24, -1, -1 ),
  new SingleSourceTwoLevelController( 25, -1, -1 ),
  0 // define 0 as last entry!
};

const char g_cnC0[] PROGMEM = "DG";
const char g_cnC1[] PROGMEM = "OG_Bad";
const char g_cnC2[] PROGMEM = "OG_Bad_Handtuch";
const char g_cnC3[] PROGMEM = "OG_WC";
const char g_cnC4[] PROGMEM = "OG_Arbeit";
const char g_cnC5[] PROGMEM = "OG_Eltern";
const char g_cnC6[] PROGMEM = "OG_Kind2";
const char g_cnC7[] PROGMEM = "OG_Kind3";
const char g_cnC8[] PROGMEM = "OG_Flur";
const char g_cnC9[] PROGMEM = "OG_Diele";
const char g_cnC10[] PROGMEM = "EG_Garderobe";
const char g_cnC11[] PROGMEM = "EG_Bad";
const char g_cnC12[] PROGMEM = "EG_Zimmer";
const char g_cnC13[] PROGMEM = "EG_Wohnen";
const char g_cnC14[] PROGMEM = "EG_Kueche";
const char g_cnC15[] PROGMEM = "EG_Flur_Diele";
const char g_cnC16[] PROGMEM = "UG_Flur";
const char g_cnC17[] PROGMEM = "UG_Vorrat";
const char g_cnC18[] PROGMEM = "UG_Bad";
const char g_cnC19[] PROGMEM = "UG_Gast";
const char g_cnC20[] PROGMEM = "UG_Hobby";
const char g_cnC21[] PROGMEM = "UG_Keller";
const char g_cnC22[] PROGMEM = "UG_Waschen";
const char g_cnC23[] PROGMEM = "UG_Diele";
const char g_cnC24[] PROGMEM = "UG_Technik_Lueftung";
const char g_cnC25[] PROGMEM = "UG_Waschen_Lueftung";

PGM_P const g_controllerNames[ CONTROLLER_COUNT ] PROGMEM = {
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
  g_cnC23,
  g_cnC24,
  g_cnC25
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
  return EEPROM.read( EEPROM_TEMP_BASE + _id * EEPROM_CONTROLLER_SETTING_SIZE );
}
  
void Controller::setProfileID( uint8_t id ){
  EEPROM.write( EEPROM_TEMP_BASE + _id * EEPROM_CONTROLLER_SETTING_SIZE, id );
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
  EEPROM.write( EEPROM_TEMP_BASE + _id * EEPROM_CONTROLLER_SETTING_SIZE + 1, targetT >> 8 );
  EEPROM.write( EEPROM_TEMP_BASE + _id * EEPROM_CONTROLLER_SETTING_SIZE + 2, targetT & 0xff );
}
  
float Controller::getTargetT() const{
  int targetT = EEPROM.read( EEPROM_TEMP_BASE + _id * EEPROM_CONTROLLER_SETTING_SIZE + 1 ) * 256 + EEPROM.read( EEPROM_TEMP_BASE + _id * EEPROM_CONTROLLER_SETTING_SIZE + 2 );
  return targetT / 10.f;
}

void Controller::setMinimumLevel( float minLevel ){
  EEPROM.write( EEPROM_TEMP_BASE + _id * EEPROM_CONTROLLER_SETTING_SIZE + 3, (uint8_t)( minLevel * 255 ) );
}

float Controller::getMinimumLevel() const{
  return EEPROM.read( EEPROM_TEMP_BASE + _id * EEPROM_CONTROLLER_SETTING_SIZE + 3 ) / 255.f;
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

float Controller::getMeasuredT() const{
  return 0.f;
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

void Controller::printStatus( Stream& out ) const{
}

void Controller::sendStatus() const{
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
