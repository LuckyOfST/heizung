#ifndef SingleSourceTwoLevelController_h
#define SingleSourceTwoLevelController_h

#include "Controller.h"

class SingleSourceTwoLevelController
:public Controller
{
public:
  typedef enum{ Idle, WaitingForValue } SensorMode;
  
  SingleSourceTwoLevelController( uint8_t id, unsigned short sensor, unsigned short actor );

  virtual bool working() const;

  virtual unsigned long doJob();

protected:
  virtual void heat( float level );
  
private:
  unsigned short _sensor;
  unsigned short _actor;
  float _heat;
  SensorMode _sensorMode;
};

#endif // SingleSourceTwoLevelController_h

