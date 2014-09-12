#include "Tools.h"

EEPROMStream g_eeprom;

unsigned char g_buffer[ 48 ];

char* strlower( char* s ){
  for( char* p = s; *p; ++p ){
    *p = tolower( *p );
  }
  return s;
}

