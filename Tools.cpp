#include <Time.h>

#include "Defines.h"
#include "Tools.h"

EEPROMStream g_eeprom;

unsigned char g_buffer[ 48 ];

char* strlower( char* s ){
  for( char* p = s; *p; ++p ){
    *p = tolower( *p );
  }
  return s;
}

void writeTime( Stream& out ){
  out << F("Current time is ")  << lz(day()) << '.' << lz(month()) << '.' << year() << ' ' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second()) << endl;
}


