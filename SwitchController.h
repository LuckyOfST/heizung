#ifndef SwitchController_h
#define SwitchController_h

#include "Controller.h"

class SwitchController
  :public Controller
{
public:
  SwitchController( uint8_t id, int8_t actor );

  virtual void setup( int i, int amount );
  virtual bool isSwitch() const;
  virtual unsigned long doJob();

  // T measured by sensor(s) used to compare against the target T
  virtual float getMeasuredT() const;

  virtual void writeSettings( Stream& s );

  virtual void readSettings( Stream& s );

  virtual bool working() const;

  virtual void printStatus( Stream& out ) const;

  virtual void sendStatus() const;

protected:
  float getLevel( bool& activeFlag ) const;
  
  // level: 0=off; 1=on (100%)
  virtual void heat( float level );

private:
  int8_t _actor;
  float _heat;

};

#endif // SwitchController_h
