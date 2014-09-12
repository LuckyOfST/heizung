#include <EEPROM.h>
#include <Time.h>

#include "Controller.h"
#include "TemperatureProfiles.h"

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


