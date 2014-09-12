#ifndef Actor_h
#define Actor_h

#include <arduino.h>

#include "Defines.h"
#include "Job.h"

class Actor;

extern PROGMEM const char* g_actorNames[];
extern Actor* g_actors[ ACTOR_COUNT + 1 ];

class Actor
:public Job
{
public:
  typedef enum{ Standard, ForceOff, ForceOn } Mode;

  static Actor* findActor( const char* name );
  static float getCurrentUsedAmperage();
  static float getCurrentRequiredAmperage();
  static bool hasActorStateChanged( bool reset = true );
  
  Actor();

  bool isOpen() const{ return _state; }
  
  virtual void setup( int i, int amount );
  
  void setLevel( float level );
  
  virtual unsigned long doJob();
  
  void setMode( Mode mode );
  
  unsigned long tryWrite( int level, unsigned long delay = 0 );

  virtual void update();
  
  float _level;
  bool _open;
  bool _state;
  float _currentAmperage;
  unsigned long _switchOnTime;
  bool _waitingForPower;
private:
  Mode _mode;
};

#endif // Actor.h

