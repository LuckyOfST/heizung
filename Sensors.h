#ifndef Sensors_h
#define Sensors_h

#include <DallasTemperature.h>
#include <OneWire.h>
#include <Time.h>

#include "Defines.h"

//////////////////////////////////////////////////////////////////////////////

// defines the number 1-wire busses
#define BUS_COUNT 24

// defines the number of sensors connected to 1-wire busses
#define SENSOR_COUNT 25  

//////////////////////////////////////////////////////////////////////////////

struct Sensor{
  void setValid( bool valid ){ _temp = valid ? 0.f : 9999.f; }
  bool valid() const{ return _temp != 9999.f; }
  
  void requestValue();
  bool isAvailable();
  bool update( bool waitForValue = false );
  void sendT();

  char* _name;
  byte _bus;
  union{
    DeviceAddress _addr;
    struct{
      byte _zero;
      unsigned long _lastRequest;
    } _dhtData;
  };
  unsigned long _errorCount;
  float _temp;
  time_t _lastChange;
};

extern void setupSensors();
extern Sensor& getSensor( unsigned short id );
extern const Sensor* findSensor( const char* name );
extern unsigned short detectSensors( Stream& out );
extern void requestTemp();
extern void writeSensorsErrors( Stream& out );

#endif // Sensors_h

