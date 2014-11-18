#include <Streaming.h>
#include <Time.h>

#include "Actor.h"
#include "Sensors.h"
#include "SingleSourceTwoLevelController.h"
#include "Tools.h"

SingleSourceTwoLevelController::SingleSourceTwoLevelController( uint8_t id, int8_t sensor, int8_t actor )
:Controller( id )
,_sensor( sensor )
,_actor( actor )
,_heat( 0.f )
,_sensorMode( Idle )
{
}

bool SingleSourceTwoLevelController::working() const{
  if ( _sensor == -1 ){
    return false;
  }
  return getSensor( _sensor ).valid();
}

unsigned long SingleSourceTwoLevelController::doJob(){
  if ( _sensor == -1 ){
    return CONTROLLER_UPDATE_INTERVAL;
  }
#ifndef DEBUG_IGNORE_SENSORS
  Sensor& sensor = getSensor( _sensor );
  if ( !sensor.valid() ){
    //Serial << _name << F(": skipped due missing sensor '") << g_sensors[ _sensor ]._name << F("'.") << endl;
    return CONTROLLER_UPDATE_INTERVAL;
  }
  if ( _sensorMode == Idle ){
    sensor.requestValue();
    _sensorMode = WaitingForValue;
  }
  if ( _sensorMode == WaitingForValue ){
    if ( !sensor.isAvailable() ){
      return 100;
    }
    _sensorMode = Idle;
  }
  if ( !sensor.update() ){
    Serial << getName() << F(": error in temperature update!") << endl;
    return CONTROLLER_UPDATE_INTERVAL;
  }

  float t = sensor._temp;
#else
  float t = 21.f;
#endif
  
  // execute special functions (like weekly water flow in heating system)
  unsigned long dt = specialFunctions( t );
  if ( dt > 0 ){
    return dt;
  }

  if ( t < getT() ){
    heat( 1.f );
  } else if ( t > getT() ){
    heat( getMinimumLevel() );
  }
  return CONTROLLER_UPDATE_INTERVAL - 100;
}

void SingleSourceTwoLevelController::heat( float level ){
  if ( level != _heat ){
    _heat = level;
    DEBUG2{ Serial << lz(day()) << '.' << lz(month()) << '.' << year() << ',' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second()) << F(" : ") << getName() << ( ( level > 0.f ) ? F(": start") : F(": stop") ) << F(" heating. (level ") << _FLOAT( level, 2 ) << F(")") << endl; }
    BEGINMSG "H " << getName() << ' ' << _FLOAT( level, 2 ) ENDMSG
    if ( _actor != -1 ){
      g_actors[ _actor ]->setLevel( level );
    }
  }
}

void SingleSourceTwoLevelController::printStatus( Stream& out ){
  if ( _sensor == -1 || _actor == -1 ){
    out << F( "NOT WORKING" );
    return;
  }
  if ( _heat > 0 && g_actors[ _actor ]->isOpen() ){
    out << F( "HEATING " );
    if ( _heat < 1 ){
      out << '(' << int( _heat * 100 ) << F( "%)" );
    }
  }
}
