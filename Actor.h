#ifndef Actor_h
#define Actor_h

#include <arduino.h>

#include "Defines.h"
#include "Job.h"

//////////////////////////////////////////////////////////////////////////////

// defines the number of actors (relais) connected
#define ACTOR_COUNT 25

// defines the number of thermostat actors connected for amperage calculation.
// The thermastat actors must be the first HEATING_ACTOR_COUNT actors. All other actors
// are simple switches with level 0/1 only.
#define HEATING_ACTOR_COUNT 24

// define the cycle time of each actor in seconds (=PWM frequence)
#define ACTOR_CYCLE_INTERVAL 60ul*20
//#define ACTOR_CYCLE_INTERVAL 5

// set this define if the relais are "on" using a low input level
#define INVERT_RELAIS_LEVEL

#define ACTOR_SWITCH_ON_SURGE_AMPERAGE 0.2f
#define ACTOR_SWITCH_ON_SURGE_DURATION 60000
#define ACTOR_WORKING_AMPERAGE 0.075f
#define ACTORS_MAX_AMPERAGE 5.f

////// The ACTORS_COMMTYPE defines the interface type to the actors.
// "DIRECT": Each actor is assigned to a digital I/O pin of the board
// "SERIAL_V1": The actors are connected to a shield with shift registers
// "CRYSTAL64IO": The actors are connected to a shiled like the Crystal 64 IO or Centipede based on MCP23017 chips using the I2C interface

//#define ACTORS_COMMTYPE_DIRECT
//#define ACTORS_COMMTYPE_SERIAL_V1
#define ACTORS_COMMTYPE_CRYSTAL64IO

#ifdef ACTORS_COMMTYPE_SERIAL_V1
  // SER: yellow wire
  // RCLK: black wire
  // SRCLK: red wire
  #define ACTORS_SER_PIN 5
  #define ACTORS_RCLK_PIN 6
  #define ACTORS_SRCLK_PIN 7
#endif // ACTORS_COMMTYPE_SERIAL_V1

//////////////////////////////////////////////////////////////////////////////

class Actor;

extern PROGMEM const char* const g_actorNames[];
extern Actor* g_actors[ ACTOR_COUNT + 1 ];

class Actor
:public Job
{
public:
  typedef enum{ Standard=1, ForceOff=2, ForceOn=4 } Mode;

  static Actor* findActor( const char* name );
  static float getCurrentUsedAmperage();
  static float getCurrentRequiredAmperage();
  static void applyActorsStates();
#if defined( ACTORS_COMMTYPE_CRYSTAL64IO )
  static void setupI2C();
#endif

#if defined( ACTORS_COMMTYPE_DIRECT )
  Actor( uint8_t aid, unsigned char pin );
#elif defined(ACTORS_COMMTYPE_SERIAL_V1) || defined( ACTORS_COMMTYPE_CRYSTAL64IO )
  Actor( uint8_t aid, bool invertOutputPin = false );
#else
  #error ACTORS_COMMTYPE_XXX must be defined!
#endif // ACTORS_COMMTYPE_DIRECT

  uint8_t getID() const{ return _aid; }

  const char* getName() const;

  String title() const override;

  void setup( int i, int amount ) override;

  bool isOpen() const{ return _state; }
  
  void setLevel( float level );
  
  unsigned long doJob() override;
  
  void setMode( Mode mode );

  Mode getMode() const{ return _mode; }
  
  unsigned long tryWrite( bool on, unsigned long delay = 0 );

  void update() override;
  
  void sendStatus() const;

  bool isSwitch() const{ return _aid >= HEATING_ACTOR_COUNT; }
  
  // If the standard mod is forced the mode set using setMode(...) is ignored and the standard mode is used.
  // This is usefull while executing special (mostly security) functions in the controllers.
  void forceStandardMode( bool forced );
  
  uint8_t _aid;
  uint8_t _open : 1, _state : 1, _waitingForPower : 1, _invertOutputPin : 1;
  float _level;
  float _currentAmperage;
  unsigned long _switchOnTime;
#if defined( ACTORS_COMMTYPE_DIRECT )
  unsigned char _pin;
#endif
  
protected:
  virtual void applyActorState( bool on );
  
private:
  Mode _mode;
};

extern void setupActors();
extern void testActors();

#endif // Actor.h

