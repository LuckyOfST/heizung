#ifndef SingleSourceTwoLevelController_h
#define SingleSourceTwoLevelController_h

#include "Controller.h"

class SingleSourceTwoLevelController
:public Controller
{
public:
  typedef enum{ Idle, WaitingForValue } SensorMode;
  
  SingleSourceTwoLevelController( uint8_t id, int8_t sensor, int8_t actor );

  virtual bool working() const;

  virtual unsigned long doJob();

  virtual void printStatus( Stream& out );

protected:
  virtual void heat( float level );
  
private:
  int8_t _sensor;
  int8_t _actor;
  float _heat;
  SensorMode _sensorMode;
};

#endif // SingleSourceTwoLevelController_h

