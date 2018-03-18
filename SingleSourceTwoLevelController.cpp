#include <Streaming.h>
#include <Time.h>

#include "Actor.h"
#include "MQTTClient.h"
#include "Sensors.h"
#include "SingleSourceTwoLevelController.h"
#include "Tools.h"

SingleSourceTwoLevelController::SingleSourceTwoLevelController( uint8_t id, int8_t sensor, int8_t actor )
:Controller( id )
,_sensor( sensor )
,_actor( actor )
,_currentT( 0.f )
,_heat( 0.f )
{
}

bool SingleSourceTwoLevelController::isSwitch() const{
  return _actor >= HEATING_ACTOR_COUNT;
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
  sensor.requestValue();
  if ( !sensor.isAvailable() ){
    return 1000;
  }
  if ( !sensor.update() ){
    Serial << getName() << F(": error in temperature update!") << endl;
    return CONTROLLER_UPDATE_INTERVAL;
  }
#ifdef SUPPORT_MQTT
  if ( _currentT != sensor._temp ){
    MQTT::publishTemp( getName(), sensor._temp );
  }
#endif
  _currentT = sensor._temp;
#else
  float t = 21.f;
#endif
  
  // execute special functions (like weekly water flow in heating system)
  if ( _actor != -1 ){
    // special functions do have special purposes, mostly for system security reasons. Therefore they have a higher priority than the forced modes set by the user...
    g_actors[ _actor ]->forceStandardMode( true );
  }
  unsigned long dt = specialFunctions( _currentT );
  if ( dt > 0 ){
    return dt;
  }
  if ( _actor != -1 ){
    g_actors[ _actor ]->forceStandardMode( false );
  }

  bool minLevelActive = true;
  float targetT = getT( minLevelActive );
  if ( _currentT < targetT ){
    heat( 1.f );
  } else if ( _currentT > targetT ){
    heat( minLevelActive ? getMinimumLevel() : 0.f );
  }
  return CONTROLLER_UPDATE_INTERVAL - 100;
}

float SingleSourceTwoLevelController::getMeasuredT() const{
  return _currentT;
}

void SingleSourceTwoLevelController::heat( float level ){
  if ( level != _heat ){
    _heat = level;
    DEBUG2{ Serial << lz(day()) << '.' << lz(month()) << '.' << year() << ',' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second()) << F(" : ") << getName() << ( ( level > 0.f ) ? F(": start") : F(": stop") ) << F(" heating. (level ") << _FLOAT( level, 2 ) << F(")") << endl; }
    sendStatus();
    if ( _actor != -1 ){
      g_actors[ _actor ]->setLevel( level );
    }
  }
}

void SingleSourceTwoLevelController::printStatus( Stream& out ) const{
  sendStatus();
  if ( _sensor == -1 || _actor == -1 ){
    out << F( "NOT WORKING" );
    return;
  }
  if ( _heat > 0 && g_actors[ _actor ]->isOpen() ){
    out << ( isSwitch() ? F( "ON" ) : F( "HEATING " ) );
    if ( _heat < 1 ){
      out << '(' << int( _heat * 100 ) << F( "%)" );
    }
  }
}

void SingleSourceTwoLevelController::sendStatus() const{
  // C name profileID currentT targetT heatingLevel minimumLevel forcedLevel
  BEGINMSG(1) "C " << getName() << ' ' << getProfileID() << ' ' << _FLOAT( getMeasuredT(), 1 ) << ' ' << _FLOAT( getT(), 1 ) << ' ' << _FLOAT( _heat, 2 ) << ' ' << _FLOAT( getMinimumLevel(), 2 ) << ' ' << _FLOAT( getForcedLevel(), 2 ) ENDMSG;
  if ( _actor != -1 ){
    g_actors[ _actor ]->sendStatus();
  }
}

void SingleSourceTwoLevelController::printActors( Stream& out ) const{
  if ( _actor != -1 ){
    out << F( "(-->" ) << g_actors[ _actor ]->getName() << ')';
  }
}
