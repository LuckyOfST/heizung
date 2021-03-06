#ifndef Controller_h
#define Controller_h

#include <arduino.h>

#include "Defines.h"
#include "Job.h"

//////////////////////////////////////////////////////////////////////////////

// define the number of temperature controllers (usually the number of rooms)
#define CONTROLLER_COUNT 25

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
  
  uint8_t getID() const{ return _cid; }
  
  const char* getName() const;

  String title() const override;

  void setup( int i, int amount ) override;

  uint8_t getProfileID() const;
  
  void setProfileID( uint8_t pid );

  bool isProfileActive() const{ return getTargetT() == 0; }
  
  virtual bool isSwitch() const = 0;

  // target T set by the user or 0 if profile should be used.
  void setTargetT( float t );
  
  float getTargetT() const;

  void setMinimumLevel( float minLevel );

  float getMinimumLevel() const;

  // target T (by user or profile)
  // in profile mode the active flag of the used profile entry is returned, too.
  // in user mode the activeFlag is not touched!
  float getT( bool& activeFlag ) const;

  // target T (by user or profile)
  float getT() const;

  // T measured by sensor(s) used to compare against the target T
  virtual float getMeasuredT() const;

  virtual void writeSettings( Stream& s );
  
  virtual void readSettings( Stream& s );
  
  virtual bool working() const;
  
  void setForcedLevel( float level );
  float getForcedLevel() const{ return _forcedLevel; }
  
  virtual void printStatus( Stream& out ) const;

  virtual void sendStatus() const;

  virtual void printActors( Stream& out ) const;

protected:
  // level: 0=off; 1=on (100%)
  virtual void heat( float level ) =0;

  unsigned long specialFunctions( float t );
  
  void sendSettings();

  uint8_t _cid;
  float _forcedLevel;
};

extern void setupController();

#endif // Controller_h

