#ifndef Tools_h
#define Tools_h

#include <EEPROM.h>
#include <Streaming.h>
#include <Time.h>

#include "Network.h"

////////////////////////////////////////////////////////////////////////////////////////////

// IMPORTANT:
// The profiles base must be at least CONTROLLER_COUNT * EEPROM_CONTROLLER_SETTING_SIZE + 6 bytes higher than the temp base.
// Set the reset define whenever the profile base changes, execute the program and send
// a 'setProfiles 0' command to initialize the EEPROM correctly.

//#define EEPROM_FORCE_PROFILE_RESET
#define EEPROM_TEMP_BASE 0

#ifdef SUPPORT_FTP
  #define EEPROM_FTPSERVERADDR 118
#endif

#ifdef SUPPORT_NTP
  #define EEPROM_NTPSERVERADDR 122
#endif

#ifdef SUPPORT_UDP_messages
  #define EEPROM_UDP_LOGGING_MASK_BASE 126
#endif

#define EEPROM_PROFILES_BASE 128
#define EEPROM_CONTROLLER_SETTING_SIZE 4


////////////////////////////////////////////////////////////////////////////////////////////

#define lz(i) (i<10?"0":"") << i

/////////////////////////////////////////////////////////////////////////////// StringStream
////////////////////////////////////////////////////////////////////////////////////////////

class StringStream
:public Stream
{
public:
  StringStream()
    :_position( 0 )
  {}
  StringStream( const String& s )
    :_string( s )
    ,_position( 0 )
  {}
  StringStream( const char* s )
    :_string( s )
    ,_position( 0 )
  {}
  virtual int available(){ return _string.length() - _position; }
  virtual int read(){ return _position < (int)_string.length() ? _string[ _position++ ] : -1; }
  virtual int peek(){ return _position < (int)_string.length() ? _string[ _position ] : -1; }
  virtual void flush(){};
  virtual size_t write( uint8_t c ){ _string += (char)c; return 1; }
  const String& str() const{ return _string; }
  const char* c_str() const{ return _string.c_str(); }
  void clear(){ _string = ""; }
private:
  String _string;
  int _position;
};

extern bool htmlMode;

inline void endline( Stream& out ){
  if ( htmlMode ){
    out << F( "</p>" );
    return;
  }
  out.println();
}

/////////////////////////////////////////////////////////////////////////////// EEPROMStream
////////////////////////////////////////////////////////////////////////////////////////////

class EEPROMStream{
public:
  EEPROMStream()
    :_position( 0 )
  {}
  void setCurrentBaseAddr( int baseAddr ){ _baseAddr = baseAddr; _position = 0; }
  void setPosition( int pos ){ _position = pos; };
  int getPosition() const{ return _position; };
  int readInt(){ return read() * 256 + read(); }
  void writeInt( int i ){ write( i >> 8 ); write( i & 0xff ); }
  int at( int ofs ) const{ return EEPROM.read( _baseAddr + _position + ofs ); };
  int read(){ return EEPROM.read( _baseAddr + _position++ ); }
  int peek(){ return EEPROM.read( _baseAddr + _position ); }
  void write( uint8_t c ){ EEPROM.write( _baseAddr + _position++, c ); }
private:
  int _baseAddr;
  int _position;
};

extern char* strlower( char* s );
extern bool hasStartTime();
extern void setStartTime();
extern void writeTime( Stream& out );
extern void writeTime( Stream& out, const time_t& t );
extern const char* readText( Stream& s );
extern void read( Stream& s, char* buffer, unsigned short bufferSize );

extern EEPROMStream g_eeprom;

// BUFFER_LENGTH must be at least 48 bytes for NTPClient
#undef BUFFER_LENGTH
#define BUFFER_LENGTH 48
extern unsigned char g_buffer[ BUFFER_LENGTH + 1 ];

// The start time is used to remember since when the sketch is running. It is the first time a valid time was set.
extern time_t g_startTime;

#endif // Tools_h

