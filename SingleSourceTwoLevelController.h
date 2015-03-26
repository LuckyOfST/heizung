#ifndef SingleSourceTwoLevelController_h
#define SingleSourceTwoLevelController_h

#include "Controller.h"

class SingleSourceTwoLevelController
:public Controller
{
public:
  SingleSourceTwoLevelController( uint8_t id, int8_t sensor, int8_t actor );

  virtual bool working() const;

  virtual unsigned long doJob();

  float getMeasuredT() const;

  virtual void printStatus( Stream& out ) const;

  virtual void sendStatus() const;

protected:
  virtual void heat( float level );
  
private:
  int8_t _sensor;
  int8_t _actor;
  float _currentT;
  float _heat;
};

#endif // SingleSourceTwoLevelController_h

