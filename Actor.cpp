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
  new Actor( 24, true ),
  0 // define 0 as last entry!
};

const char g_an0[] PROGMEM = "a0";  // UG Keller
const char g_an1[] PROGMEM = "a1";  // UG Flur
const char g_an2[] PROGMEM = "a2";  // UG Hobby
const char g_an3[] PROGMEM = "a3";  // UG Gast
const char g_an4[] PROGMEM = "a4";  // UG Bad
const char g_an5[] PROGMEM = "a5";  // UG Vorrat
const char g_an6[] PROGMEM = "a6";  // UG Diele
const char g_an7[] PROGMEM = "a7";  // n/a

const char g_an8[] PROGMEM = "b0";  // UG Waschen
const char g_an9[] PROGMEM = "b1";  // OG Bad
const char g_an10[] PROGMEM = "b2"; // OG Flur
const char g_an11[] PROGMEM = "b3"; // OG Kind 3
const char g_an12[] PROGMEM = "b4"; // OG Kind 2
const char g_an13[] PROGMEM = "b5"; // OG Eltern (OG Kind 1)
const char g_an14[] PROGMEM = "b6"; // OG Diele
const char g_an15[] PROGMEM = "b7"; // OG Arbeiten

const char g_an16[] PROGMEM = "c0"; // EG Kueche
const char g_an17[] PROGMEM = "c1"; // EG Wohnen
const char g_an18[] PROGMEM = "c2"; // EG Garderobe
const char g_an19[] PROGMEM = "c3"; // EG Flur + Diele
const char g_an20[] PROGMEM = "c4"; // EG Zimmer
const char g_an21[] PROGMEM = "c5"; // EG Bad
const char g_an22[] PROGMEM = "c6"; // DG
const char g_an23[] PROGMEM = "c7"; // OG WC

// Switches...
const char g_an24[] PROGMEM = "s0"; // UG Technik Lueftung

PGM_P const g_actorNames[ ACTOR_COUNT ] PROGMEM = {
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
  g_an23,
  g_an24
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
      if ( g_actors[ i ]->_invertOutputPin ^ !g_actors[ i ]->isOpen() ){
#else
      if ( g_actors[ i ]->_invertOutputPin ^ g_actors[ i ]->isOpen() ){
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
Actor::Actor( uint8_t id, bool invertOutputPin )
#else
  #error ACTORS_COMMTYPE_XXX must be defined!
#endif // ACTORS_COMMTYPE_DIRECT
:_aid( id )
,_level( 0 )
,_open( false )
,_state( false )
, _currentAmperage( 0.f )
, _waitingForPower( false )
#ifdef ACTORS_COMMTYPE_DIRECT
,_pin( pin )
#else
, _invertOutputPin( invertOutputPin )
#endif // ACTORS_COMMTYPE_DIRECT
, _mode( Standard )
{
}

const char* Actor::getName() const{
  strcpy_P( (char*)g_buffer, (char*)pgm_read_word(&(g_actorNames[ _aid ])) );
  return (char*)g_buffer;
}
  
String Actor::title() const {
  String s = F( "act-" );
  s += getName();
  return s;
}

void Actor::setup( int i, int amount ){
  if ( isSwitch() ){
    return;
  }
  Job::setup( i, amount );
#ifdef ACTORS_COMMTYPE_DIRECT
  pinMode( _pin, OUTPUT );
#endif // ACTORS_COMMTYPE_DIRECT
}
  
void Actor::setLevel( float level ){
  if ( isSwitch() ){
    applyActorState( ( _mode & 1 != 0) ? (level > 0.f) : (_mode == ForceOn) );
    return;
  }
  if ( level < 0.f ){
    level = 0.f;
  }
  if ( level > 1.f ){
    level = 1.f;
  }
  _level = level;
  _delayMillis = (unsigned long)(ACTOR_CYCLE_INTERVAL * (_open ? _level : (1.f - _level)) * 1000.f);
}

unsigned long Actor::doJob(){
  if ( isSwitch() ){
    return ACTOR_CYCLE_INTERVAL;
  }
  if ( _waitingForPower ){
    unsigned long delay = tryWrite( true );
    if ( _waitingForPower ){
      return delay;
    }
    _open = !_open;
  }
  if ( _mode & 1 != 0 ){ // Standard mode or forced standard mode
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
  if ( isSwitch() ){
    setLevel( _level );
  }
}

void Actor::forceStandardMode( bool forced ){
  if ( forced ){
    _mode = (Mode)( _mode | 1 );
  } else if ( _mode != Standard ){
    _mode = (Mode)( _mode & ~1 );
  }
}

unsigned long Actor::tryWrite( bool on, unsigned long delay ){
  if ( isSwitch() ){
    applyActorState( on );
    return delay;
  }
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
  if ( isSwitch() || _currentAmperage == 0.f ){
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
  bool stateChanged = on ^ _state;
#ifdef ACTORS_COMMTYPE_DIRECT
#ifdef INVERT_RELAIS_LEVEL
  digitalWrite( _pin, on ? LOW : HIGH );
#else
  digitalWrite( _pin, on ? HIGH : LOW );
#endif // INVERT_RELAIS_LEVEL
#elif defined(ACTORS_COMMTYPE_SERIAL_V1) || defined( ACTORS_COMMTYPE_CRYSTAL64IO )
  g_actorStateChanged |= stateChanged;
#else
  #error ACTORS_COMMTYPE_XXX must be defined!
#endif // ACTORS_COMMTYPE_DIRECT
  _state = on;
  if ( stateChanged ){
    sendStatus();
  }
}
  
void Actor::sendStatus() const{
  BEGINMSG(1) "A " << getName() << ' ' << (_state ? 1 : 0) ENDMSG
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

