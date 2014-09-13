#include <arduino.h>
#include <Streaming.h>

#include "Actor.h"

Actor* g_actors[ ACTOR_COUNT + 1 ] = {
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  0 // define 0 as last entry!
};

char g_an0[] PROGMEM = "a0";
char g_an1[] PROGMEM = "a1";
char g_an2[] PROGMEM = "a2";
char g_an3[] PROGMEM = "a3";
char g_an4[] PROGMEM = "a4";
char g_an5[] PROGMEM = "a5";
char g_an6[] PROGMEM = "a6";
char g_an7[] PROGMEM = "a7";
char g_an8[] PROGMEM = "b0";
char g_an9[] PROGMEM = "b1";
char g_an10[] PROGMEM = "b2";
char g_an11[] PROGMEM = "b3";
char g_an12[] PROGMEM = "b4";
char g_an13[] PROGMEM = "b5";
char g_an14[] PROGMEM = "b6";
char g_an15[] PROGMEM = "b7";
char g_an16[] PROGMEM = "c0";
char g_an17[] PROGMEM = "c1";
char g_an18[] PROGMEM = "c2";
char g_an19[] PROGMEM = "c3";
char g_an20[] PROGMEM = "c4";
char g_an21[] PROGMEM = "c5";
char g_an22[] PROGMEM = "c6";
char g_an23[] PROGMEM = "c7";

PGM_P g_actorNames[ ACTOR_COUNT ] PROGMEM = {
  g_an0,
  g_an1,
  g_an2,
  g_an3,
  g_an4,
  g_an5,
  g_an6,
  g_an7,
  g_an8,
  g_an9,
  g_an10,
  g_an11,
  g_an12,
  g_an13,
  g_an14,
  g_an15,
  g_an16,
  g_an17,
  g_an18,
  g_an19,
  g_an20,
  g_an21,
  g_an22,
  g_an23
};

static float g_totalAmperage = 0.f; // current sum of all actors
static float g_requiredAmperage = 0.f; 

#ifdef ACTORS_COMMTYPE_SERIAL_V1
static bool g_actorStateChanged = true;
#endif // ACTORS_COMMTYPE_V1

float Actor::getCurrentUsedAmperage(){
  return g_totalAmperage;
}

float Actor::getCurrentRequiredAmperage(){
  return g_requiredAmperage;
}

void Actor::applyActorsStates(){
#ifdef ACTORS_COMMTYPE_V1
  if ( g_actorStateChanged ){
    g_actorStateChanged = false;
    DEBUG{ Serial << F("Actors state: "); }
    digitalWrite( ACTORS_RCLK_PIN, LOW );
    for( int i = ACTOR_COUNT - 1; i >=  0; i-- ){
      digitalWrite( ACTORS_SRCLK_PIN, LOW );
      DEBUG{ Serial << ( g_actors[ i ]->isOpen() ? F("1") : F("0") ); }
#ifdef INVERT_RELAIS_LEVEL
      digitalWrite( ACTORS_SER_PIN, g_actors[ i ]->isOpen() ? LOW : HIGH );
#else
      digitalWrite( ACTORS_SER_PIN, g_actors[ i ]->isOpen() ? HIGH : LOW );
#endif
      digitalWrite( ACTORS_SRCLK_PIN, HIGH );
    }
    DEBUG{ Serial << endl; }
    digitalWrite( ACTORS_RCLK_PIN, HIGH );
  }
#endif // ACTORS_COMMTYPE_V1
}

#ifdef ACTORS_COMMTYPE_DIRECT
Actor::Actor( unsigned char pin )
#elif defined(ACTORS_COMMTYPE_SERIAL_V1)
Actor::Actor()
#else
  #error ACTORS_COMMTYPE_XXX must be defined!
#endif // ACTORS_COMMTYPE_DIRECT
:_level( 0 )
,_open( false )
,_state( false )
,_mode( Standard )
,_currentAmperage( 0.f )
,_waitingForPower( false )
#ifdef ACTORS_COMMTYPE_DIRECT
,_pin( pin )
#endif // ACTORS_COMMTYPE_SERIAL_V1
{
}

void Actor::setup( int i, int amount ){
  Job::setup( i, amount );
#ifdef ACTORS_COMMTYPE_DIRECT
  pinMode( _pin, OUTPUT );
#endif // ACTORS_COMMTYPE_DIRECT
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
    unsigned long delay = tryWrite( true );
    if ( _waitingForPower ){
      return delay;
    }
    _open = !_open;
  }
  if ( _mode == Standard ){
    _open = !_open;
    unsigned long delay = (unsigned long)( ACTOR_CYCLE_INTERVAL * ( _open ? _level : ( 1.f - _level ) ) * 1000.f );
    if ( delay ){
      delay = tryWrite( _open, delay );
    }
    return delay;
  } else if ( _mode == ForceOn ){
    return tryWrite( true, 1000 );
  }
  return tryWrite( false, 1000 );
}
  
void Actor::setMode( Mode mode ){
  _mode = mode;
  _delayMillis = 0;
}
  
unsigned long Actor::tryWrite( bool on, unsigned long delay ){
  g_totalAmperage -= _currentAmperage;
  g_requiredAmperage -= _currentAmperage;
  if ( !on ){
    // we always can go to LOW state...
    applyActorState( false );
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
    applyActorState( true );
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

void Actor::applyActorState( bool on ){
#ifdef ACTORS_COMMTYPE_DIRECT
#ifdef INVERT_RELAIS_LEVEL
  digitalWrite( _pin, on ? LOW : HIGH );
#else
  digitalWrite( _pin, on ? HIGH : LOW );
#endif // INVERT_RELAIS_LEVEL
#elif defined(ACTORS_COMMTYPE_SERIAL_V1)
  g_actorStateChanged |= on ^ _state;
#else
  #error ACTORS_COMMTYPE_XXX must be defined!
#endif // ACTORS_COMMTYPE_DIRECT
  _state = on;
}
  
Actor* Actor::findActor( const char* name ){
  for( int i = 0; i < ACTOR_COUNT; ++i ){
    if ( !strcmp_P( name, (PGM_P)pgm_read_word(&g_actorNames[ i ] )) ){
      return g_actors[ i ];
    }
  }
  return 0;
}

void setupActors(){
  Serial << F( "Initializing actors." ) << endl;
#ifdef ACTORS_COMMTYPE_DIRECT
  // setup is integrated in the setup of the jobs...
#elif defined(ACTORS_COMMTYPE_SERIAL_V1)
  pinMode( ACTORS_SER_PIN, OUTPUT );
  pinMode( ACTORS_RCLK_PIN, OUTPUT );
  pinMode( ACTORS_SRCLK_PIN, OUTPUT );
#else
  #error ACTORS_COMMTYPE_XXX must be defined!
#endif // ACTORS_COMMTYPE_DIRECT
  Job::setupJobs( (Job**)g_actors );
}

