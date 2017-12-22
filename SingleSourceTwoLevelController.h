#ifndef SingleSourceTwoLevelController_h
#define SingleSourceTwoLevelController_h

#include "Controller.h"

class SingleSourceTwoLevelController
:public Controller
{
public:
  SingleSourceTwoLevelController( uint8_t id, int8_t sensor, int8_t actor );

  bool isSwitch() const override;

  bool working() const override;

  unsigned long doJob() override;

  float getMeasuredT() const override;

  void printStatus( Stream& out ) const override;

  void sendStatus() const override;

  void printActors( Stream& out ) const override;

protected:
  void heat( float level ) override;
  
private:
  int8_t _sensor;
  int8_t _actor;
  float _currentT;
  float _heat;
};

#endif // SingleSourceTwoLevelController_h

