#include <Time.h>

#include "Defines.h"
#include "Tools.h"

bool htmlMode = false;

EEPROMStream g_eeprom;

unsigned char g_buffer[ BUFFER_LENGTH + 1 ];

time_t g_startTime = 0;

#ifdef SUPPORT_UDP_messages
int g_debugMask = 0;
#endif

char* strlower( char* s ){
  for( char* p = s; *p; ++p ){
    *p = tolower( *p );
  }
  return s;
}

bool hasStartTime(){
  return g_startTime != 0;
}

void setStartTime(){
  if ( g_startTime == 0 && year() > 2014 ){
    g_startTime = now();
  }
}

void writeTime( Stream& out ){
  //out << F("Current time is ")  << lz(day()) << '.' << lz(month()) << '.' << year() << ' ' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second()) << endl;
  out << F( "Current time is " );
  writeTime( out, now() );
  endline( out );
}

void writeTime( Stream& out, const time_t& t ){
  out << lz(day(t)) << '.' << lz(month(t)) << '.' << year(t) << ' ' << lz(hour(t)) << ':' << lz(minute(t)) << ':' << lz(second(t));
}

const char* readText( Stream& s ){
  static char text[ 256 ];
  byte length = s.readBytesUntil( ' ', text, sizeof( text ) / sizeof( char ) );
  text[ length ] = 0;
  return text;
}

void read( Stream& s, char* buffer, unsigned short bufferSize ){
  --bufferSize; // reduce buffer size by one for terminating 0.
  unsigned short i = 0;
  while ( s.available() && i < bufferSize ){
    char c = s.read();
    if ( c == 10 || c == 13 || c == 32 ){
      if ( i == 0 ){
        continue;
      }
      break;
    }
    buffer[ i ] = c;
    ++i;
  }
  buffer[ i ] = 0;
  while ( s.available() ){
    char c = s.peek();
    if ( c != 10 && c != 13 && c != 32 ){
      break;
    }
  }
}

