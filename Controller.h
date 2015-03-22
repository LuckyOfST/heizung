#ifndef Controller_h
#define Controller_h

#include <arduino.h>

#include "Defines.h"
#include "Job.h"

//////////////////////////////////////////////////////////////////////////////

// define the number of temperature controllers (usually the number of rooms)
#define CONTROLLER_COUNT 26

// defines the time interval the controller updates its logic in milliseconds
#define CONTROLLER_UPDATE_INTERVAL 5000ul

// set this define to be able to force controllers to a specific level without working sensors.
//#define DEBUG_IGNORE_SENSORS

//////////////////////////////////////////////////////////////////////////////

class Controller;

extern PROGMEM const char* const g_controllerNames[];
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

  void setMinimumLevel( float minLevel );

  float getMinimumLevel() const;

  float getT() const;
  
  virtual void writeSettings( Stream& s );
  
  virtual void readSettings( Stream& s );
  
  virtual bool working() const;
  
  void setForcedLevel( float level );
  float getForcedLevel() const{ return _forcedLevel; }
  
  virtual void printStatus( Stream& out );

protected:
  // level: 0=off; 1=on (100%)
  virtual void heat( float level ) =0;

  unsigned long specialFunctions( float t );
  
  uint8_t _id;
  float _forcedLevel;
};

extern void setupController();

#endif // Controller_h

