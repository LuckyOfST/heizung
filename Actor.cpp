#include <EEPROM.h>
#include <arduino.h>
#include <Streaming.h>
#include <Wire.h>

#include "Actor.h"
#include "Tools.h"

#ifdef ACTORS_COMMTYPE_CRYSTAL64IO
#include <IOshield.h>
#endif // ACTORS_COMMTYPE_CRYSTAL64IO

Actor* g_actors[ ACTOR_COUNT + 1 ] = {
  new Actor( 0 ),
  new Actor( 1 ),
  new Actor( 2 ),
  new Actor( 3 ),
  new Actor( 4 ),
  new Actor( 5 ),
  new Actor( 6 ),
  new Actor( 7 ),
  new Actor( 8 ),
  new Actor( 9 ),
  new Actor( 10 ),
  new Actor( 11 ),
  new Actor( 12 ),
  new Actor( 13 ),
  new Actor( 14 ),
  new Actor( 15 ),
  new Actor( 16 ),
  new Actor( 17 ),
  new Actor( 18 ),
  new Actor( 19 ),
  new Actor( 20 ),
  new Actor( 21 ),
  new Actor( 22 ),
  new Actor( 23 ),
  0 // define 0 as last entry!
};

char g_an0[] PROGMEM = "a0";  // EG Kueche
char g_an1[] PROGMEM = "a1";  // EG Wohnen
char g_an2[] PROGMEM = "a2";  // EG Garderobe
char g_an3[] PROGMEM = "a3";  // EG Flur
char g_an4[] PROGMEM = "a4";  // EG Zimmer
char g_an5[] PROGMEM = "a5";  // EG Bad
char g_an6[] PROGMEM = "a6";  // DG
char g_an7[] PROGMEM = "a7";  // OG WC

char g_an8[] PROGMEM = "b0";
char g_an9[] PROGMEM = "b1";  // OG Bad
char g_an10[] PROGMEM = "b2"; // OG Flur
char g_an11[] PROGMEM = "b3"; // OG Kind 3
char g_an12[] PROGMEM = "b4"; // OG Kind 2
char g_an13[] PROGMEM = "b5"; // OG Eltern (OG Kind 1)
char g_an14[] PROGMEM = "b6"; // OG Diele
char g_an15[] PROGMEM = "b7"; // OG Arbeiten

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

#if defined(ACTORS_COMMTYPE_SERIAL_V1) || defined( ACTORS_COMMTYPE_CRYSTAL64IO )
static bool g_actorStateChanged = true;
#endif // ACTORS_COMMTYPE_SERIAL_V1 || ACTORS_COMMTYPE_CRYSTAL64IO

#ifdef ACTORS_COMMTYPE_CRYSTAL64IO
IOshield IO;
#define CRYSTAL64IO_BASE_PORT 0
#endif // ACTORS_COMMTYPE_CRYSTAL64IO

float Actor::getCurrentUsedAmperage(){
  return g_totalAmperage;
}

float Actor::getCurrentRequiredAmperage(){
  return g_requiredAmperage;
}

void Actor::applyActorsStates(){
#if defined(ACTORS_COMMTYPE_SERIAL_V1)
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
#elif defined( ACTORS_COMMTYPE_CRYSTAL64IO )
  if ( g_actorStateChanged ){
    g_actorStateChanged = false;
    DEBUG{ Serial << F("Actors state: "); }
    uint8_t port = CRYSTAL64IO_BASE_PORT;
#if 0
    for( uint8_t i = 0; i < ACTOR_COUNT; ++i ){
#ifdef INVERT_RELAIS_LEVEL
      IO.digitalWrite( i, g_actors[ i ]->isOpen() ? 0 : 1 );
#else
      IO.digitalWrite( i, g_actors[ i ]->isOpen() ? 1 : 0 );
#endif
    }
#else
    uint16_t state = 0;
    for( uint8_t i = 0; i < ACTOR_COUNT; ++i ){
      DEBUG{ Serial << ( g_actors[ i ]->isOpen() ? F("1") : F("0") ); }
      if ( i > 0 && !( i & 0xf ) ){
        IO.portWrite( port, state );
        state = 0;
        ++port;
      }
      state >>= 1;
#ifdef INVERT_RELAIS_LEVEL
      if ( !g_actors[ i ]->isOpen() ){
#else
      if ( g_actors[ i ]->isOpen() ){
#endif
        state |= 0x8000;
      }
    }
    if ( ACTOR_COUNT & 0xf ){
      // write to the lower bits
      state >>= 16 - ( ACTOR_COUNT & 0xf );
      // preserve the value of the pins not used by the actors
      uint16_t value = IO.portRead( port );
      value &= 0xffff << ( ACTOR_COUNT & 0xf );
      state |= value;
      // write the port
      IO.portWrite( port, state );
    }
    DEBUG{ Serial << endl; }
#endif
  }
#endif // ACTORS_COMMTYPE_SERIAL_V1 || ACTORS_COMMTYPE_CRYSTAL64IO
}

#ifdef ACTORS_COMMTYPE_DIRECT
Actor::Actor( uint8_t id, unsigned char pin )
#elif defined(ACTORS_COMMTYPE_SERIAL_V1) || defined( ACTORS_COMMTYPE_CRYSTAL64IO )
Actor::Actor( uint8_t id )
#else
  #error ACTORS_COMMTYPE_XXX must be defined!
#endif // ACTORS_COMMTYPE_DIRECT
:_id( id )
,_level( 0 )
,_open( false )
,_state( false )
,_mode( Standard )
,_currentAmperage( 0.f )
,_waitingForPower( false )
#ifdef ACTORS_COMMTYPE_DIRECT
,_pin( pin )
#endif // ACTORS_COMMTYPE_DIRECT
{
}

const char* Actor::getName() const{
  strcpy_P( (char*)g_buffer, (char*)pgm_read_word(&(g_actorNames[ _id ])) );
  return (char*)g_buffer;
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
  if ( on ^ _state ){
    BEGINMSG "A " << getName() << ' ' << ( on ? 1 : 0 ) ENDMSG
  }
#ifdef ACTORS_COMMTYPE_DIRECT
#ifdef INVERT_RELAIS_LEVEL
  digitalWrite( _pin, on ? LOW : HIGH );
#else
  digitalWrite( _pin, on ? HIGH : LOW );
#endif // INVERT_RELAIS_LEVEL
#elif defined(ACTORS_COMMTYPE_SERIAL_V1) || defined( ACTORS_COMMTYPE_CRYSTAL64IO )
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

void testActors(){
  // all off
  for( uint8_t i = 0; i < ACTOR_COUNT; ++i ){
    g_actors[ i ]->setMode( Actor::ForceOff );
    g_actors[ i ]->doJob();
  }
  g_actorStateChanged = true;
  Actor::applyActorsStates();

  // single actors test...
  for( uint8_t i = 0; i < ACTOR_COUNT; ++i ){
    if ( i > 0 ){
      g_actors[ i - 1 ]->setMode( Actor::ForceOff );
      g_actors[ i - 1 ]->doJob();
    }
    g_actors[ i ]->setMode( Actor::ForceOn );
    g_actors[ i ]->doJob();
    Actor::applyActorsStates();
    delay( 250 );
  }
  
  // blink all actors (alternating neighbors)
  for( uint8_t j = 0; j < 8; ++j ){
    for( uint8_t i = 0; i < ACTOR_COUNT; ++i ){
      g_actors[ i ]->setMode( ( ( i + j ) & 1 ) ? Actor::ForceOn : Actor::ForceOff );
      g_actors[ i ]->doJob();
    }
    Actor::applyActorsStates();
    delay( 250 );
  }

  // blink all actors (all on/off)
  for( uint8_t j = 0; j < 8; ++j ){
    for( uint8_t i = 0; i < ACTOR_COUNT; ++i ){
      g_actors[ i ]->setMode( ( j & 1 ) ? Actor::ForceOn : Actor::ForceOff );
      g_actors[ i ]->doJob();
    }
    Actor::applyActorsStates();
    delay( 250 );
  }

  // set all actors to standard mode
  for( uint8_t i = 0; i < ACTOR_COUNT; ++i ){
    g_actors[ i ]->setMode( Actor::Standard );
    g_actors[ i ]->doJob();
  }
  Actor::applyActorsStates();
}

#if defined( ACTORS_COMMTYPE_CRYSTAL64IO )
void Actor::setupI2C(){
  Serial << F( "Wire.begin" ) << endl;
  Wire.begin();
  Serial << F( "IO.initilize" ) << endl;
  IO.initialize();
  Serial << F( "set pinmodes" ) << endl;
  for( uint8_t i = 0; i < ACTOR_COUNT; ++i ){
    IO.pinMode( i, OUTPUT );
  }
  TWBR = 12;
  // set all actors to standard mode
  for( uint8_t i = 0; i < ACTOR_COUNT; ++i ){
    g_actors[ i ]->setMode( Actor::Standard );
    g_actors[ i ]->doJob();
  }
  g_actorStateChanged = true;
  Actor::applyActorsStates();
}
#endif

void setupActors(){
  Serial << F( "Initializing actors." ) << endl;
#ifdef ACTORS_COMMTYPE_DIRECT
  // setup is integrated in the setup of the jobs...
#elif defined(ACTORS_COMMTYPE_SERIAL_V1)
  pinMode( ACTORS_SER_PIN, OUTPUT );
  pinMode( ACTORS_RCLK_PIN, OUTPUT );
  pinMode( ACTORS_SRCLK_PIN, OUTPUT );
#elif defined( ACTORS_COMMTYPE_CRYSTAL64IO )
  // nothing to do...
  Actor::setupI2C();
#else
  #error ACTORS_COMMTYPE_XXX must be defined!
#endif // ACTORS_COMMTYPE_DIRECT
  Job::setupJobs( (Job**)g_actors );
}

