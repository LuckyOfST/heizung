#ifndef Actor_h
#define Actor_h

#include <arduino.h>

#include "Defines.h"
#include "Job.h"

//////////////////////////////////////////////////////////////////////////////

// defines the number of actors (relais) connected
#define ACTOR_COUNT 24

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
  typedef enum{ Standard, ForceOff, ForceOn } Mode;

  static Actor* findActor( const char* name );
  static float getCurrentUsedAmperage();
  static float getCurrentRequiredAmperage();
  static void applyActorsStates();
#if defined( ACTORS_COMMTYPE_CRYSTAL64IO )
  static void setupI2C();
#endif

#if defined( ACTORS_COMMTYPE_DIRECT )
  Actor( uint8_t id, unsigned char pin );
#elif defined(ACTORS_COMMTYPE_SERIAL_V1) || defined( ACTORS_COMMTYPE_CRYSTAL64IO )
  Actor( uint8_t id );
#else
  #error ACTORS_COMMTYPE_XXX must be defined!
#endif // ACTORS_COMMTYPE_DIRECT

  uint8_t getID() const{ return _id; }

  const char* getName() const;

  virtual void setup( int i, int amount );

  bool isOpen() const{ return _state; }
  
  void setLevel( float level );
  
  virtual unsigned long doJob();
  
  void setMode( Mode mode );
  
  unsigned long tryWrite( bool on, unsigned long delay = 0 );

  virtual void update();
  
  float _level;
  bool _open;
  bool _state;
  float _currentAmperage;
  unsigned long _switchOnTime;
  bool _waitingForPower;
  uint8_t _id;
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

