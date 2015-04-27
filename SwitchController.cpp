#include <Streaming.h>
#include <Time.h>

#include "Actor.h"
#include "SwitchController.h"
#include "TemperatureProfiles.h"
#include "Tools.h"

SwitchController::SwitchController( uint8_t id, int8_t actor )
  :Controller( id )
  , _actor( actor )
  , _heat( 0.f )
{
}

bool SwitchController::isSwitch() const{
  return _actor >= HEATING_ACTOR_COUNT;
}

bool SwitchController::working() const{
  return true;
}

unsigned long SwitchController::doJob(){
  // execute special functions (like weekly water flow in heating system)
  unsigned long dt = specialFunctions( 21. );
  if ( dt > 0 ){
    return dt;
  }

  bool minLevelActive = false;
  float level = getLevel( minLevelActive );
  heat( minLevelActive ? getMinimumLevel() : level );
  return 2000ul;
}

float SwitchController::getLevel( bool& activeFlag ) const{
  TemperatureProfiles::setCurrentProfile( getProfileID() );
  time_t t = now();
  byte day = weekday( t ) - 2;
  if ( day == -1 ){
    day = 6;
  }
  if ( isSwitch() ){
    activeFlag = false;
    return TemperatureProfiles::highresActiveFlag( day, hour( t ), minute( t ) ) ? 1.f : 0.f;
  }
  return TemperatureProfiles::value( day, hour( t ), minute( t ), activeFlag );
}

void SwitchController::heat( float level ){
  if ( level != _heat ){
    _heat = level;
    DEBUG2{ Serial << lz( day() ) << '.' << lz( month() ) << '.' << year() << ',' << lz( hour() ) << ':' << lz( minute() ) << ':' << lz( second() ) << F( " : " ) << getName() << ((level > 0.f) ? F( ": start" ) : F( ": stop" )) << F( " heating. (level " ) << _FLOAT( level, 2 ) << F( ")" ) << endl; }
    sendStatus();
    if ( _actor != -1 ){
      g_actors[ _actor ]->setLevel( level );
    }
  }
}

void SwitchController::printStatus( Stream& out ) const{
  sendStatus();
  if ( _actor == -1 ){
    out << F( "NOT WORKING" );
    return;
  }
  if ( _heat > 0 && g_actors[ _actor ]->isOpen() ){
    if ( isSwitch() ){
      out << F( "ON" );
    } else {
      out << F( "HEATING " );
      if ( _heat < 1 ){
        out << '(' << int( _heat * 100 ) << F( "%)" );
      }
    }
  }
}

void SwitchController::sendStatus() const{
  // S name profileID heatingLevel minimumLevel forcedLevel
  BEGINMSG "S " << getName() << ' ' << getProfileID() << ' ' << _FLOAT( _heat, 2 ) << ' ' << _FLOAT( getMinimumLevel(), 2 ) << ' ' << _FLOAT( getForcedLevel(), 2 ) ENDMSG;
  if ( _actor != -1 ){
    g_actors[ _actor ]->sendStatus();
  }
}
