#include "Defines.h"
#include "TemperatureProfiles.h"
#include "Tools.h"

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
    return g_eeprom.at( idx * 3 + 0 ) / 10.f;
  }
    
  byte getStartTime( int idx ){
    return g_eeprom.at( idx * 3 + 1 );
  }
    
  float getTemp( int idx ){
    return g_eeprom.at( idx * 3 + 2 ) / 10.f;
  }
    
  float temp( byte day, byte hour, byte min ){
    float temp = -1.f;
    do{
      byte dow = 1 << day;
      byte now  = hour * 10 + min / 15;
      byte next = 0;
      for( int i = 0; i < g_nbValues; ++i ){
        byte startTime = getStartTime( i );
        if ( getDays( i ) & dow && startTime <= now && startTime >= next ){
          next = startTime;
          temp = getTemp( i );
        }
      }
      hour = 23;
      min = 59;
      ++day;
    } while ( temp == -1.f && day < 7 );
    return temp;
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
        g_eeprom.write( s.parseInt() ); // temp
      }
      profileAddr = g_eeprom.getPosition();
    }
  }
  
  byte g_nbValues;

} // namespace TemperatureProfiles

