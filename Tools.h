#ifndef Tools_h
#define Tools_h

#include <EEPROM.h>
#include <Streaming.h>

// IMPORTANT:
// The profiles base must be at least CONTROLLER_COUNT * 3 bytes higher than the temp base.
// Set the reset define whenever the profile base changes, execute the program and send
// a 'setProfiles 0' command to initialize the EEPROM correctly.

//#define EEPROM_FORCE_PROFILE_RESET
#define EEPROM_TEMP_BASE 0
#define EEPROM_PROFILES_BASE 128

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
  virtual int read(){ return _position < _string.length() ? _string[ _position++ ] : -1; }
  virtual int peek(){ return _position < _string.length() ? _string[ _position ] : -1; }
  virtual void flush(){};
  virtual size_t write( uint8_t c ){ _string += (char)c; }
  const String& str() const{ return _string; }
  const char* c_str() const{ return _string.c_str(); }
private:
  String _string;
  int _position;
};

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

extern EEPROMStream g_eeprom;;


#endif // Tools_h

