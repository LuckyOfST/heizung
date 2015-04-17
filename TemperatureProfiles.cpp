#include "Defines.h"
#include "TemperatureProfiles.h"
#include "Tools.h"

// EEPROM profile format:
//
// uint8 nbProfiles
// 
// address table of profiles (nbProfiles addresses):
// uint16 profileAddr: initial addr of profileData section (=1+2*nbProfiles); incremented by size of profile data for each profile
//
// profile data section: (for each profile)
// uint8 nbValues: number of values (data packets) in current profile
// nbValues times / for each data packet:
// uint8 daysOfWeek: bit encoded days the value is valid: bit 0: monday .. bit 6: sunday; Bit 7: Min level heating activation
// uint8 startTime: start time for the given value relative to midnight of the current day (using 24 hour system, 15 min resolution): hour * 10 + min / 15
// uint8 value: temperature 10th degrees [0-25.5 degrees C] or value in 1/255 steps [0-255]

// example: 1 3 2 255 50 210 127 200 160
// 1 - 1 profile
// 3 - 2 bytes for each entry + 1 byte header => first profile at addr 3
// 2 - 2 packets in profile
// 255 - everyday; min level heating on
// 50 - start at 5:00
// 210 - 21°
// 127 - everyday; min level heating off
// 200 - start at 20:00
// 160 - 16°

namespace TemperatureProfiles{

  byte getNbProfiles(){
    g_eeprom.setCurrentBaseAddr( EEPROM_PROFILES_BASE );
    return g_eeprom.read();
  }
  
  void setCurrentProfile( byte id ){
    g_eeprom.setCurrentBaseAddr( EEPROM_PROFILES_BASE );
    g_eeprom.setPosition( 1 + (int)id * 2 );
    g_eeprom.setPosition( g_eeprom.readInt() );
    g_nbValues = g_eeprom.read();
  }
  
  byte getDays( int idx ){
    return g_eeprom.at( idx * 3 + 0 );
  }
    
  byte getStartTime( int idx ){
    return g_eeprom.at( idx * 3 + 1 );
  }
    
  float getValue( int idx ){
    return g_eeprom.at( idx * 3 + 2 ) / 255.f;
  }
    
  float value( byte day, byte hour, byte min, bool& activeFlag ){
    float value = -1.f;
    do{
      byte dow = 1 << day;
      byte now  = hour * 10 + min / 15;
      byte next = 0;
      for( int i = 0; i < g_nbValues; ++i ){
        byte startTime = getStartTime( i );
        if ( getDays( i ) & dow && startTime <= now && startTime >= next ){
          next = startTime;
          value = getValue( i );
          activeFlag = ( startTime & 0x80 ) != 0;
        }
      }
      hour = 23;
      min = 59;
      ++day;
    } while ( value == -1.f && day < 7 );
    return value;
  }
  
  float value( byte day, byte hour, byte min ){
    bool active;
    return value( day, hour, min, active );
  }

  float temp( byte day, byte hour, byte min, bool& active ){
    float t = value( day, hour, min, active );
    return t == -1.f ? -1 : t * 25.5f;
  }

  float temp( byte day, byte hour, byte min ){
    bool active;
    return temp( day, hour, min, active );
  }

  void writeSettings( Stream& s ){
#ifdef EEPROM_FORCE_PROFILE_RESET
    return;
#endif
    DEBUG{ Serial << F( "Read settings from EEPROM" ) << endl; }
    int nbProfiles = getNbProfiles();
    s << nbProfiles;
    for( int id = 0; id < nbProfiles; ++id ){
      setCurrentProfile( id );
      s << ' ' << g_nbValues;
      for( int i = 0; i < g_nbValues; ++i ){
        s << ' ' << g_eeprom.at( i * 3 + 0 ) << ' ' << g_eeprom.at( i * 3 + 1 ) << ' ' << g_eeprom.at( i * 3 + 2 );
      }
    }
    s << endl;
  }
  
  void readSettings( Stream& s ){
    DEBUG{ Serial << F( "Write settings to EEPROM" ) << endl; }
    g_eeprom.setCurrentBaseAddr( EEPROM_PROFILES_BASE );
    int nbProfiles = s.parseInt();
    DEBUG{ Serial << F( "nbProfiles: " ) << nbProfiles << endl; }
    g_eeprom.write( nbProfiles );
    int profileAddr = 1 + nbProfiles * 2;
    for( int id = 0; id < nbProfiles; ++id ){
      g_eeprom.setPosition( 1 + (int)id * 2 );
      g_eeprom.writeInt( profileAddr );
      g_eeprom.setPosition( profileAddr );
      int nbValues = s.parseInt();
      g_eeprom.write( nbValues );
      for( int i = 0; i < nbValues; ++i ){
        g_eeprom.write( s.parseInt() ); // daysOfWeek
        g_eeprom.write( s.parseInt() ); // startTime
        g_eeprom.write( s.parseInt() ); // value/temp
      }
      profileAddr = g_eeprom.getPosition();
    }
  }
  
  byte g_nbValues;

} // namespace TemperatureProfiles

