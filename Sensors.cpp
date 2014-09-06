#include "Network.h"
#include "Sensors.h"

void Sensor::requestValue(){
  g_temperatures[ _bus ].requestTemperaturesByAddress( _addr );
}

bool Sensor::isAvailable(){
  return g_temperatures[ _bus ].isConversionAvailable( _addr );
}

bool Sensor::update( bool waitForValue ){
  if ( !valid() ){
    return false;
  }
  if ( waitForValue ){
    g_temperatures[ _bus ].setWaitForConversion( true );
    requestValue();
    g_temperatures[ _bus ].setWaitForConversion( false );
  }
  float t = g_temperatures[ _bus ].getTempC( _addr );
  if ( t == 85.f || t == -127.f ){
    ++_errorCount;
    return false;
  }
  _temp = t;
  sendT();
  return true;
}

void Sensor::sendT(){
  Udp.beginPacket( 0xffffffff, 12888 );
  Udp << "T " << _name << ' ' << _FLOAT( _temp, 1 );
  Udp.endPacket();
}

unsigned short findSensor( const DeviceAddress& addr ){
  for( int i = 0; i < SENSOR_COUNT; ++i ){
    if ( !memcmp( addr, g_sensors[ i ]._addr, 8 ) ){
      return i;
    }
  }
  return SENSOR_COUNT;
}

const struct Sensor* findSensor( const char* name ){
  for( int i = 0; i < SENSOR_COUNT; ++i ){
    if ( !strcmp( name, g_sensors[ i ]._name ) ){
      return &g_sensors[ i ];
    }
  }
  return 0;
}

bool isSensorValid( DallasTemperature& temp, Sensor& sensor ){
/*  uint8_t deviceCount = temp.getDeviceCount();
  // iterate over all sensors of each bus
  for( uint8_t i = 0; i < deviceCount; ++i ){
    if ( temp.getAddress( sensor._addr, i ) ){
      return true;
    }
  }*/
  for( int bus = 0; bus < BUS_COUNT; ++bus ){
    DallasTemperature& temp = g_temperatures[ bus ];
    uint8_t deviceCount = temp.getDeviceCount();
    for( uint8_t i = 0; i < deviceCount; ++i ){
      DeviceAddress addr;
      if ( !temp.getAddress( addr, i ) ){
        continue;
      }
      if ( !memcmp( addr, sensor._addr, 8 ) ){
        return true;
      }
    }
  }
  return false;
}

unsigned short detectSensors( Stream& out ){
  //// SEARCH FOR NEW DEVICES
  // iterate over all busses
  for( int bus = 0; bus < BUS_COUNT; ++bus ){
    if ( !g_busses[ bus ].reset() ){
      out << F( "    Bus " ) << bus << F( " is not responding." ) << endl;
    }
    DallasTemperature& temp = g_temperatures[ bus ];
    temp.begin();
    uint8_t deviceCount = temp.getDeviceCount();
    out << F( "    " ) << deviceCount << F( " devices on bus " ) << bus << endl;
    // iterate over all sensors of each bus
    for( uint8_t i = 0; i < deviceCount; ++i ){
      DeviceAddress addr;
      if ( !temp.getAddress( addr, i ) ){
        continue;
      }
      unsigned short sid = findSensor( addr );
      if ( sid == SENSOR_COUNT ){
        out << F("    New device detected: ") << endl;
        out << F("{ \"new_device\", { ");
        for( uint8_t j = 0; j < 8; ++j ){
          out << ( j > 0 ? F(", ") : F("") ) << F("0x") << ( addr[ j ] < 16 ? F("0") : F("") ) << _HEX( addr[ j ] );
        }
        out << F(" }, ") << bus << F(" }, // ") << SENSOR_COUNT << endl;
      } else {
        out << F("      Sensor '") << g_sensors[ sid ]._name << F("' detected.") << endl;
      }
    }
  }
  //// LOOK FOR LOST DEVICES
  for( int i = 0; i < SENSOR_COUNT; ++i ){
    Sensor& sensor = g_sensors[ i ];
    sensor.setValid( true );
    DallasTemperature& temp = g_temperatures[ sensor._bus ];
    //Serial << sensor._name << "," << isSensorValid(temp,sensor) << "," << temp.validAddress(sensor._addr) << "," << temp.isConnected(sensor._addr) << "," << temp.getTempC(sensor._addr) << endl;
    if ( !isSensorValid( temp, sensor ) || !temp.validAddress( sensor._addr ) || !temp.isConnected( sensor._addr ) || temp.getTempC( sensor._addr ) < -126.f ){
      out << F("    ERROR: Device '") << sensor._name << F("' could not be found!") << endl;
      //sensor._valid = false;
      sensor.setValid( false );
    } else {
      sensor.update( true );
      out << F("device '") << sensor._name << F("' found. (") << _FLOAT( sensor._temp, 1 ) << F(" C)") << endl;
    }
  }
  return BUS_COUNT;
}

