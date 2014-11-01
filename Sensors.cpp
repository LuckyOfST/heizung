#include "Network.h"
#include "Sensors.h"

// elements = number of digital IO pin the 1-wire-bus is connected to
OneWire g_busses[ BUS_COUNT ] = {
  //46, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45
  49, 47, 45, 43, 41, 39, 37, 35, 33, 31, 29, 27, 25, 23, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40
};

// proxy classes to read temperatures from the busses. One entry for each bus.
DallasTemperature g_temperatures[ BUS_COUNT ] = {
  &g_busses[ 0 ],
  &g_busses[ 1 ],
  &g_busses[ 2 ],
  &g_busses[ 3 ],
  &g_busses[ 4 ],
  &g_busses[ 5 ],
  &g_busses[ 6 ],
  &g_busses[ 7 ],
  &g_busses[ 8 ],
  &g_busses[ 9 ],
  &g_busses[ 10 ],
  &g_busses[ 11 ],
  &g_busses[ 12 ],
  &g_busses[ 13 ],
  &g_busses[ 14 ],
  &g_busses[ 15 ],
  &g_busses[ 16 ],
  &g_busses[ 17 ],
  &g_busses[ 18 ],
  &g_busses[ 19 ],
  &g_busses[ 20 ],
  &g_busses[ 21 ],
  &g_busses[ 22 ],
  &g_busses[ 23 ]
};

// elements = name, bus, address of sensor
// "name" is a unique human readable string to identify the temperature sensor
// "bus" is the index of the bus in g_busses the sensor is connected to
// "address" is the DS18B20 internal address (use "detect" command to get this)
Sensor g_sensors[ SENSOR_COUNT ] = {
  { "S_UG_Waschen",       15, { 0x28, 0xE2, 0x12, 0xE2, 0x04, 0x00, 0x00, 0x76 } }, // 0
  //{ "S_UG_Waschen",        0 }, // 0: DHT temperature/humidity sensor (no address)
  { "S_UG_Gast",           8, { 0x28, 0xFB, 0x2E, 0xE1, 0x04, 0x00, 0x00, 0x96 } }, // 1
  { "S_UG_Hobby",          5, { 0x28, 0x7E, 0x14, 0xE1, 0x04, 0x00, 0x00, 0xAE } }, // 2
  { "S_UG_Treppe",        23, { 0x28, 0xEE, 0x32, 0xE1, 0x04, 0x00, 0x00, 0x7B } }, // 3
  { "S_UG_Flur",           0, { 0x28, 0x8F, 0x28, 0xE1, 0x04, 0x00, 0x00, 0x4E } }, // 4
  { "S_UG_Vorrat",        23, { 0x28, 0x06, 0x58, 0xCE, 0x04, 0x00, 0x00, 0xB5 } }, // 5
  { "S_UG_Bad",           12, { 0x28, 0x57, 0xDE, 0xE0, 0x04, 0x00, 0x00, 0x9E } }, // 6
  { "S_UG_Keller",         1, { 0x28, 0x07, 0x25, 0xE1, 0x04, 0x00, 0x00, 0xE9 } }, // 7
  { "S_EG_Treppe",         2, { 0x28, 0xFA, 0x77, 0xE0, 0x04, 0x00, 0x00, 0x48 } }, // 8
  { "S_EG_Garderobe",     10, { 0x28, 0x46, 0x71, 0xE0, 0x04, 0x00, 0x00, 0xAE } }, // 9
  { "S_EG_Flur",          14, { 0x28, 0xDF, 0x43, 0xE1, 0x04, 0x00, 0x00, 0x01 } }, // 10
  { "S_EG_Bad",           21, { 0x28, 0xF0, 0xAD, 0xE2, 0x04, 0x00, 0x00, 0x63 } }, // 11
  { "S_EG_Zimmer",         7, { 0x28, 0xEC, 0x86, 0xE2, 0x04, 0x00, 0x00, 0xCD } }, // 12
  { "S_EG_Wohnen_Flur",    3, { 0x28, 0xA2, 0x6B, 0xE3, 0x04, 0x00, 0x00, 0x12 } }, // 13
  { "S_EG_Wohnen_Kueche",  6, { 0x28, 0xA4, 0x6B, 0xE3, 0x04, 0x00, 0x00, 0xA0 } }, // 14
  { "S_EG_Kueche",        17, { 0x28, 0x39, 0x23, 0xE3, 0x04, 0x00, 0x00, 0x8C } }, // 15
  { "S_OG_Kind2",         19, { 0x28, 0xC8, 0x6D, 0xE2, 0x04, 0x00, 0x00, 0x0D } }, // 16
  { "S_OG_Kind3",         16, { 0x28, 0xA0, 0x32, 0xE1, 0x04, 0x00, 0x00, 0x1D } }, // 17
  { "S_OG_Bad",            4, { 0x28, 0x8E, 0x53, 0xE1, 0x04, 0x00, 0x00, 0x64 } }, // 18
  { "S_OG_WC",            20, { 0x28, 0x45, 0x36, 0xE1, 0x04, 0x00, 0x00, 0xC0 } }, // 19
  { "S_OG_Arbeit",        11, { 0x28, 0x57, 0x45, 0xE1, 0x04, 0x00, 0x00, 0xD6 } }, // 20
  { "S_OG_Eltern",        13, { 0x28, 0x4C, 0x00, 0x9D, 0x04, 0x00, 0x00, 0xF4 } }, // 21
  { "S_OG_Treppe",        18, { 0x28, 0x51, 0x3B, 0xE1, 0x04, 0x00, 0x00, 0xAB } }, // 22
  { "S_OG_Flur",          22, { 0x28, 0x70, 0x6D, 0xE0, 0x04, 0x00, 0x00, 0xAC } }, // 23
  { "S_DG",                9, { 0x28, 0x98, 0x76, 0xE0, 0x04, 0x00, 0x00, 0x28 } }, // 24
};

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
  BEGINMSG "T " << _name << ' ' << _FLOAT( _temp, 1 ) ENDMSG
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
      out << F( "  Bus " ) << bus << F( " is not responding." ) << endl;
      continue;
    }
    DallasTemperature& temp = g_temperatures[ bus ];
    temp.begin();
    uint8_t deviceCount = temp.getDeviceCount();
    out << F( "  " ) << deviceCount << F( " devices on bus " ) << bus << endl;
    // iterate over all sensors of each bus
    for( uint8_t i = 0; i < deviceCount; ++i ){
      DeviceAddress addr;
      if ( !temp.getAddress( addr, i ) ){
        continue;
      }
      unsigned short sid = findSensor( addr );
      if ( sid == SENSOR_COUNT ){
        out << F("  New device detected: ") << endl;
        out << F("{ \"new_device\", { ");
        for( uint8_t j = 0; j < 8; ++j ){
          out << ( j > 0 ? F(", ") : F("") ) << F("0x") << ( addr[ j ] < 16 ? F("0") : F("") ) << _HEX( addr[ j ] );
        }
        out << F(" }, ") << bus << F(" }, // ") << SENSOR_COUNT << endl;
      } else {
        out << F("    Sensor '") << g_sensors[ sid ]._name << F("' detected.") << endl;
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
      out << F("  ERROR: Device '") << sensor._name << F("' could not be found!") << endl;
      //sensor._valid = false;
      sensor.setValid( false );
    } else {
      sensor.update( true );
      out << F("device '") << sensor._name << F("' found. (") << _FLOAT( sensor._temp, 1 ) << F(" C)") << endl;
    }
  }
  return BUS_COUNT;
}

void setupSensors(){
  Serial << F("Initializing sensors.") << endl;
  for( int i = 0; i < BUS_COUNT; ++i ){
    g_temperatures[ i ].begin();
    g_temperatures[ i ].setWaitForConversion( false );
    if ( g_temperatures[ i ].isParasitePowerMode() ){
      Serial << F("  WARNING: Bus ") << i << F(" is in parasite power mode.") << endl;
    }
  }
  detectSensors( Serial );
  for( int i = 0; i < SENSOR_COUNT; ++i ){
    Sensor& sensor = g_sensors[ i ];
    if ( !sensor.valid() ){
      continue;
    }
    DallasTemperature& temp = g_temperatures[ sensor._bus ];
    temp.setResolution( sensor._addr, 12 );
    if ( temp.getResolution( sensor._addr ) != 12 ){
      Serial << F("  WARNING: Sensor '") << sensor._name << F("' could not be set to 12 bit resolution.") << endl;
    }
  }
}

void requestTemp(){
  bool ok = false;
  for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
    Sensor& s = g_sensors[ i ];
    if ( !s.valid() ){
      continue;
    }
    s.update( true );
    ok = true;
  }
  if ( !ok ){
    BEGINMSG F("No working sensor found.") ENDMSG
  }
}

void writeSensorsErrors( Stream& out ){
  bool errorsFound = false;
  for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
    const Sensor& s = g_sensors[ i ];
    if ( s._errorCount > 0 ){
      errorsFound = true;
      out << s._errorCount << F(" errors detected for sensor '") << s._name << F("'.") << endl;
    }
  }
  if ( !errorsFound ){
    out << F("No errors detected.") << endl;
  }
}

Sensor& getSensor( unsigned short id ){
  return g_sensors[ id ];
}

