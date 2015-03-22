#include <Time.h>

#include "Defines.h"
#include "Tools.h"

EEPROMStream g_eeprom;

unsigned char g_buffer[ BUFFER_LENGTH + 1 ];

time_t g_startTime;

char* strlower( char* s ){
  for( char* p = s; *p; ++p ){
    *p = tolower( *p );
  }
  return s;
}

void setStartTime(){
  static bool first = true;
  if ( first ){
    first = false;
    g_startTime = now();
  }
}

void writeTime( Stream& out ){
  //out << F("Current time is ")  << lz(day()) << '.' << lz(month()) << '.' << year() << ' ' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second()) << endl;
  out << F( "Current time is " );
  writeTime( out, now() );
  out << endl;
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
  unsigned short i = 0;
  while ( s.available() && i < bufferSize - 1 ){
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
    if ( c != 10 || c != 13 || c != 32 ){
      break;
    }
  }
}

