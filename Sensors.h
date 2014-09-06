#ifndef Sensors_h
#define Sensors_h

#include <DallasTemperature.h>
#include <OneWire.h>

#include "Defines.h"

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
};

extern unsigned short findSensor( const DeviceAddress& addr );
extern const struct Sensor* findSensor( const char* name );
extern bool isSensorValid( DallasTemperature& temp, Sensor& sensor );
extern unsigned short detectSensors( Stream& out );

extern Sensor g_sensors[ SENSOR_COUNT ];
extern OneWire g_busses[ BUS_COUNT ];
extern DallasTemperature g_temperatures[ BUS_COUNT ];

#endif // Sensors_h

