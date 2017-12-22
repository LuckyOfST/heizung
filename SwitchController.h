#ifndef SwitchController_h
#define SwitchController_h

#include "Controller.h"

class SwitchController
  :public Controller
{
public:
  SwitchController( uint8_t id, int8_t actor );

  bool isSwitch() const override;
  unsigned long doJob() override;

  bool working() const override;

  void printStatus( Stream& out ) const override;

  void sendStatus() const override;

  void printActors( Stream& out ) const override;

protected:
  float getLevel( bool& activeFlag ) const;
  
  // level: 0=off; 1=on (100%)
  void heat( float level ) override;

private:
  int8_t _actor;
  float _heat;

};

#endif // SwitchController_h

