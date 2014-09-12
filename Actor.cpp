#include <arduino.h>

#include "Actor.h"

static float g_totalAmperage = 0.f; // current sum of all actors
static float g_requiredAmperage = 0.f; 
static bool g_actorStateChanged = true;

float Actor::getCurrentUsedAmperage(){
  return g_totalAmperage;
}

float Actor::getCurrentRequiredAmperage(){
  return g_requiredAmperage;
}

bool Actor::hasActorStateChanged( bool reset ){
  bool result = g_actorStateChanged;
  g_actorStateChanged &= !reset;
  return result;
}

Actor::Actor()
:_level( 0 )
,_open( false )
,_state( false )
,_mode( Standard )
,_currentAmperage( 0.f )
,_waitingForPower( false )
{
}

void Actor::setup( int i, int amount ){
  Job::setup( i, amount );
}
  
void Actor::setLevel( float level ){
  if ( level < 0.f ){
    level = 0.f;
  }
  if ( level > 1.f ){
    level = 1.f;
  }
  _level = level;
  _delayMillis = (unsigned long)( ACTOR_CYCLE_INTERVAL * ( _open ? _level : ( 1.f - _level ) ) * 1000.f );
}

unsigned long Actor::doJob(){
  if ( _waitingForPower ){
    unsigned long delay = tryWrite( HIGH );
    if ( _waitingForPower ){
      return delay;
    }
    _open = !_open;
  }
  if ( _mode == Standard ){
    _open = !_open;
    unsigned long delay = (unsigned long)( ACTOR_CYCLE_INTERVAL * ( _open ? _level : ( 1.f - _level ) ) * 1000.f );
    if ( delay ){
      delay = tryWrite( _open ? HIGH : LOW, delay );
    }
    return delay;
  } else if ( _mode == ForceOn ){
    return tryWrite( HIGH, 1000 );
  }
  return tryWrite( LOW, 1000 );
}
  
void Actor::setMode( Mode mode ){
  _mode = mode;
  _delayMillis = 0;
}
  
unsigned long Actor::tryWrite( int level, unsigned long delay ){
  g_totalAmperage -= _currentAmperage;
  g_requiredAmperage -= _currentAmperage;
  if ( level == LOW ){
    // we always can go to LOW state...
    g_actorStateChanged |= _state;
    _state = false;
    _currentAmperage = 0.f;
    _waitingForPower = false;
    return delay;
  }
  if ( _currentAmperage == 0.f ){
    if ( g_totalAmperage + ACTOR_SWITCH_ON_SURGE_AMPERAGE > ACTORS_MAX_AMPERAGE ){
      //Serial << F("Actor '") << _name << F("' waits for enough power.") << endl;
      if ( !_waitingForPower ){
        g_requiredAmperage += ACTOR_SWITCH_ON_SURGE_AMPERAGE;
      }
      _waitingForPower = true;
      return 1000;
    }
    if ( _waitingForPower ){
      g_requiredAmperage -= ACTOR_SWITCH_ON_SURGE_AMPERAGE;
    }
    _waitingForPower = false;
    _currentAmperage = ACTOR_SWITCH_ON_SURGE_AMPERAGE;
    _switchOnTime = millis();
    //digitalWrite( _ioPin, HIGH );
    g_actorStateChanged |= !_state;
    _state = true;
    g_actorStateChanged = true;
  }
  g_totalAmperage += _currentAmperage;
  g_requiredAmperage += _currentAmperage;
  return delay;
} 

void Actor::update(){
  if ( _currentAmperage == 0.f ){
    return;
  }
  g_totalAmperage -= _currentAmperage;
  g_requiredAmperage -= _currentAmperage;
  unsigned long elapsed = millis() - _switchOnTime;
  _currentAmperage = elapsed < ACTOR_SWITCH_ON_SURGE_DURATION ? ACTOR_SWITCH_ON_SURGE_AMPERAGE : ACTOR_WORKING_AMPERAGE;
  g_totalAmperage += _currentAmperage;
  g_requiredAmperage += _currentAmperage;
}
  
Actor* Actor::findActor( const char* name ){
  for( int i = 0; i < ACTOR_COUNT; ++i ){
    if ( !strcmp_P( name, (PGM_P)pgm_read_word(&g_actorNames[ i ] )) ){
      return g_actors[ i ];
    }
  }
  return 0;
}

