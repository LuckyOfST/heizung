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
// uint8 nbValues: bit 0-6 number of values (data packets) in current profile; bit 7: 0=heating profile, 1=switch profile
// nbValues times / for each data packet:
// uint8 daysOfWeek: bit encoded days the value is valid: bit 0: monday .. bit 6: sunday; Bit 7: Min level heating activation / Switch activation

// heating:
// uint8 startTime: start time for the given value relative to midnight of the current day (using 24 hour system, 15 min resolution): hour * 10 + min / 15
// uint8 value: temperature 10th degrees [0-25.5 degrees C] or value in 1/255 steps [0-255]

// switch:
// uint8 startTime lb: ( hour*1800 + min*30 + sec/2 ) & 0xff
// uint8 startTime hb: ( hour*1800 + min*30 + sec/2 ) / 256

// example: setProfiles 2 2 255 50 210 127 200 160
// 2 - 2 profiles
// DO NOT INCLUDE IN SET COMMAND: 5, 12 - 2 bytes for each entry + 1 byte header => first profile at addr 5

// 2 - 2 packets in profile for heating
// 255 - everyday; min level heating on
// 50 - start at 5:00
// 210 - 21°
// 127 - everyday; min level heating off
// 200 - start at 20:00
// 160 - 16°

// 2 - 2 packets in profile for switch
// 255 - everyday; switch on
// 136 - lb of startTime 17:00 => 17 * 1800 + 0 * 30 + 0/2 = 30600 & 0xff = 136
// 119 - hb of startTime
// 127 - everyday; switch off
// 144 - lb of startTime 18:00 => 18 * 1800 + 0 * 30 + 0/2 = 32400 & 0xff = 144
// 126 - hb of startTime

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
    
  uint16_t getStartTime16( int idx ){
    return ((uint16_t)g_eeprom.at( idx * 3 + 1 )) | ((uint16_t)g_eeprom.at( idx * 3 + 2 )) << 8;
  }

  bool highresActiveFlag( byte day, byte hour, byte min, byte sec ){
    bool found = false;
    bool activeFlag = false;
    int d = day;
    do{
      byte dow = 1 << d;
      uint16_t now = ((uint16_t)hour) * 1800 + min * 30 + ( sec >> 1 );
      uint16_t next = 0;
      for ( int i = 0; i < g_nbValues&0x7f; ++i ){
        uint16_t startTime = getStartTime16( i );
        if ( getDays( i ) & dow && startTime <= now && startTime >= next ){
          next = startTime;
          activeFlag = (getDays( i ) & 0x80) != 0;
          found = true;
        }
      }
      hour = 23;
      min = 59;
      --d;
      if ( d == -1 ){
        d = 6;
      }
    } while ( !found && d != day );
    return activeFlag;
  }
  
  float value( byte day, byte hour, byte min, bool& activeFlag ){
    float value = -1.f;
    int d = day;
    do{
      byte dow = 1 << d;
      byte now  = hour * 10 + min / 15;
      byte next = 0;
      for( int i = 0; i < g_nbValues&0x7f; ++i ){
        byte startTime = getStartTime( i );
        if ( getDays( i ) & dow && startTime <= now && startTime >= next ){
          next = startTime;
          value = getValue( i );
          activeFlag = (getDays( i ) & 0x80) != 0;
        }
      }
      hour = 23;
      min = 59;
      --d;
      if ( d == -1 ){
        d = 6;
      }
    } while ( value == -1.f && d != day );
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
      s << ' ' << (g_nbValues&0x7f);
      for( int i = 0; i < g_nbValues&0x7f; ++i ){
        s << ' ' << g_eeprom.at( i * 3 + 0 ) << ' ' << g_eeprom.at( i * 3 + 1 ) << ' ' << g_eeprom.at( i * 3 + 2 );
      }
    }
    endline( s );
  }
  
  void readSettings( Stream& s ){
    DEBUG{ Serial << F( "Write settings to EEPROM" ) << endl; }
    g_eeprom.setCurrentBaseAddr( EEPROM_PROFILES_BASE );
    int nbProfiles = s.parseInt();
    DEBUG{ Serial << F( "nbProfiles: " ) << nbProfiles << endl; }
    g_eeprom.write( nbProfiles );
    int profileAddr = 1 + nbProfiles * 2;
    for( int id = 0; id < nbProfiles; ++id ){
      g_eeprom.setPosition( 1 + id * 2 );
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
  
  class Profile{
  public:
    Profile()
      : _id( 0 )
      , _nbEntries( 0 )
      , _heatingProfile( true )
      , _entries( 0 )
    {
    }
    ~Profile(){
      delete[] _entries;
    }
    void initialize( bool heatingProfile, uint8_t nbEntries = 0 ){
      _heatingProfile = heatingProfile;
      _nbEntries = nbEntries;
      delete[] _entries;
      _entries = new uint8_t[ nbEntries * 3 ];
    }
    void readFromEEPROM( uint8_t id ){
      _id = id;
      setCurrentProfile( _id );
      uint8_t nbEntries = g_nbValues;
      initialize( !(nbEntries & 0x80), nbEntries & 0x7f );
      nbEntries *= 3;
      for ( int i = 0; i < nbEntries; ++i ){
        _entries[ i ] = g_eeprom.at( i );
      }
    }
    int addEntry(){
      uint8_t* entries = new uint8_t[ _nbEntries * 3 + 3 ];
      memcpy( entries, _entries, _nbEntries * 3 );
      entries[ _nbEntries * 3 + 0 ] = 0;
      entries[ _nbEntries * 3 + 1 ] = 0;
      entries[ _nbEntries * 3 + 2 ] = 0;
      delete[] _entries;
      _entries = entries;
      ++_nbEntries;
      return _nbEntries - 1;
    }
    void removeEntry( uint8_t i ){
      if ( i >= 0 && i < _nbEntries ){
        memcpy( _entries + i * 3, _entries + i * 3 + 3, ( _nbEntries - i - 1 ) * 3 );
        --_nbEntries;
      }
    }
    void print( Stream& s ){
      s << F("Profile ") << _id << F(": ");
      if ( !_nbEntries ){
        s << F( "empty" );
        endline( s );
        return;
      }
      if ( _heatingProfile ){
        s << F( "heating" );
        endline( s );
        for ( int i = 0; i < _nbEntries; ++i ){
          s << i << F( ": " );
          bool on = printDOW( i, s );
          uint16_t startTime = _entries[ i * 3 + 1 ];
          uint8_t hour = startTime / 10;
          uint8_t min = (startTime - hour * 10) * 15;
          s << lz( hour ) << ':' << lz( min ) << F(" with ");
          float value = _entries[ i * 3 + 2 ] / 10.;
          s << value << F( " C" );
          if ( on ){
            s << F( " (min level active)" );
          }
          endline( s );
        }
      } else {
        s << F( "switch" ) << endl;
        for ( int i = 0; i < _nbEntries; ++i ){
          s << i << F( ": " );
          bool on = printDOW( i, s );
          uint16_t startTime = _entries[ i * 3 + 1 ] + ( _entries[ i * 3 + 2 ] << 8 );
          uint8_t hour = startTime / 1800;
          uint8_t min = (startTime - hour * 1800) / 30;
          uint8_t sec = (startTime % 30) * 2;
          s << lz( hour ) << ':' << lz( min ) << ':' << lz( sec ) << ' ' << (on ? F( "on" ) : F( "off" ));
          endline( s );
        }
      }
    }
    bool isHeatingProfile() const{ return _heatingProfile; }
    void setHeatingEntry( uint8_t eid, uint8_t dow, uint8_t h, uint8_t m, float value, bool minActive ){
      if ( minActive ){
        dow |= 0x80;
      }
      _entries[ eid * 3 + 0 ] = dow;
      _entries[ eid * 3 + 1 ] = h * 10 + (m / 15);
      _entries[ eid * 3 + 2 ] = value * 10;
    }
    void setSwitchEntry( uint8_t eid, uint8_t dow, uint8_t h, uint8_t m, uint8_t s, bool on ){
      if ( on ){
        dow |= 0x80;
      }
      _entries[ eid * 3 + 0 ] = dow;
      uint16_t time = h * 1800 + m * 30 + (s >> 1);
      _entries[ eid * 3 + 1 ] = time & 0xff;
      _entries[ eid * 3 + 2 ] = time >> 8;
    }
    void takeEntries(){
      _nbEntries = 0;
      _entries = 0;
    }
  private:
    void writeToEEPROM(){
      //setCurrentProfile( _id );
      //g_eeprom.write( _heatingProfile ? _nbEntries : ( _nbEntries | 0x80 ) );
      g_eeprom.write( _nbEntries );
      for ( int i = 0; i < _nbEntries * 3; ++i ){
        g_eeprom.write( _entries[ i ] );
      }
    }
    bool printDOW( uint8_t i, Stream& s ){
      uint8_t daysOfWeek = _entries[ i * 3 ];
      char days[] = "mo di mi do fr sa so";
      for ( int j = 0; j < 7; ++j ){
        if ( !(daysOfWeek&(1 << j)) ){
          days[ j * 3 + 0 ] = '-';
          days[ j * 3 + 1 ] = '-';
        }
      }
      s << days << F( " from " );
      return daysOfWeek & 0x80;
    }
    uint8_t _id;
    bool _heatingProfile;
    uint8_t _nbEntries;
    uint8_t* _entries;

    friend class Profiles;
  };

  class Profiles{
  public:
    Profiles()
      : _nbProfiles( 0 )
      , _profiles( 0 )
    {
    }
    ~Profiles(){
      delete[] _profiles;
    }
    void readFromEEPROM(){
      delete[] _profiles;
      _nbProfiles = getNbProfiles();
      _profiles = new Profile[ _nbProfiles ];
      for ( int i = 0; i < _nbProfiles; ++i ){
        _profiles[ i ].readFromEEPROM( i );
      }
    }
    void writeToEEPROM(){
      g_eeprom.setCurrentBaseAddr( EEPROM_PROFILES_BASE );
      g_eeprom.write( _nbProfiles );
      int profileAddr = 1 + int(_nbProfiles) * 2;
      for( int id = 0; id < _nbProfiles; ++id ){
        g_eeprom.setPosition( 1 + id * 2 );
        g_eeprom.writeInt( profileAddr );
        g_eeprom.setPosition( profileAddr );
        _profiles[ id ].writeToEEPROM();
        profileAddr = g_eeprom.getPosition();
      }
    }
    void resetProfile( uint8_t id, bool heating ){
      if ( id >= 0 && id < _nbProfiles ){
        _profiles[ id ].initialize( heating );
      }
    }
    void addProfile( bool heating ){
      Profile* profiles = new Profile[ _nbProfiles + 1 ];
      memcpy( profiles, _profiles, _nbProfiles * sizeof( Profile ) );
      profiles[ _nbProfiles ].initialize( heating );
      for ( int i = 0; i < _nbProfiles; ++i ){
        _profiles[ i ].takeEntries();
      }
      delete[] _profiles;
      _profiles = profiles;
      ++_nbProfiles;
    }
    void removeEntry( uint8_t profile, uint8_t entry ){
      if ( profile >= 0 && profile < _nbProfiles ){
        _profiles[ profile ].removeEntry( entry );
      }
    }
    void setEntry( uint8_t profile, uint8_t entry, Stream& in ){
      if ( profile >= 0 && profile < _nbProfiles ){
        while ( isWhitespace( in.peek() ) ){
          in.read();
        }
        char daysOfWeek[ 7 ];
        in.readBytes( daysOfWeek, 7 );
        uint8_t dow = 0;
        for ( int i = 0; i < 7; ++i ){
          if ( daysOfWeek[ i ] != '-' && daysOfWeek[ i ] != '.' ){
            dow |= 1 << i;
          }
        }
        uint8_t h = in.parseInt();
        uint8_t m = in.parseInt();
        if ( _profiles[ profile ].isHeatingProfile() ){
          float value = in.parseFloat();
          int on = in.parseInt();
          _profiles[ profile ].setHeatingEntry( entry, dow, h, m, value, on );
        } else {
          uint8_t s = in.parseInt();
          int on = in.parseInt();
          _profiles[ profile ].setSwitchEntry( entry, dow, h, m, s, on );
        }
      }

    }
    void addEntry( uint8_t profile, Stream& in ){
      if ( profile >= 0 && profile < _nbProfiles ){
        uint8_t entry = _profiles[ profile ].addEntry();
        setEntry( profile, entry, in );
      }
    }
  private:
    uint8_t _nbProfiles;
    Profile* _profiles;
  };

  void printProfile( uint8_t id, Stream& s ){
    Profile p;
    p.readFromEEPROM( id );
    p.print( s );
  }

  void printProfiles( Stream& s ){
    uint8_t nbProfiles = getNbProfiles();
    s << nbProfiles << F( " profiles" );
    endline( s );
    for ( uint8_t id = 0; id < nbProfiles; ++id ){
      printProfile( id, s );
    }
  }

  void resetProfile( uint8_t id, bool heating ){
    Profiles profiles;
    profiles.readFromEEPROM();
    profiles.resetProfile( id, heating );
    profiles.writeToEEPROM();
  }

  void addProfile( bool heating ){
    Profiles profiles;
    profiles.readFromEEPROM();
    profiles.addProfile( heating );
    profiles.writeToEEPROM();
  }

  void setEntry( uint8_t profile, uint8_t entry, Stream& in ){
    Profiles profiles;
    profiles.readFromEEPROM();
    profiles.setEntry( profile, entry, in );
    profiles.writeToEEPROM();
  }

  void addEntry( uint8_t profile, Stream& in ){
    Profiles profiles;
    profiles.readFromEEPROM();
    profiles.addEntry( profile, in );
    profiles.writeToEEPROM();
  }

  void removeEntry( uint8_t profile, uint8_t entry ){
    Profiles profiles;
    profiles.readFromEEPROM();
    profiles.removeEntry( profile, entry );
    profiles.writeToEEPROM();
  }

  uint8_t g_nbValues;

} // namespace TemperatureProfiles

