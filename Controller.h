#ifndef Controller_h
#define Controller_h

#include <arduino.h>

#include "Defines.h"
#include "Job.h"

class Controller;

extern PROGMEM const char* g_controllerNames[];
extern Controller* g_controller[ CONTROLLER_COUNT + 1 ];

class Controller
:public Job
{
public:
  static Controller* find( const char* name );

  Controller( uint8_t id );
  
  uint8_t getID() const{ return _id; }
  
  const char* getName() const;
  
  virtual void setup( int i, int amount );

  uint8_t getProfileID() const;
  
  void setProfileID( uint8_t id );

  bool isProfileActive() const{ return getTargetT() == 0; }
  
  void setTargetT( float t );
  
  float getTargetT() const;

  float getT() const;
  
  virtual void writeSettings( Stream& s );
  
  virtual void readSettings( Stream& s );
  
  virtual bool working() const;
  
  void setForcedLevel( float level );
  float getForcedLevel() const{ return _forcedLevel; }
  
protected:
  // level: 0=off; 1=on (100%)
  virtual void heat( float level ) =0;

  unsigned long specialFunctions( float t );
  
  uint8_t _id;
  float _forcedLevel;
};

#endif // Controller_h

