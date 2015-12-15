#ifndef TemperatureProfiles_h
#define TemperatureProfiles_h

#include "Tools.h"

namespace TemperatureProfiles{

  byte getNbProfiles();
  void setCurrentProfile( byte id );
  float value( byte day, byte hour, byte min );
  float value( byte day, byte hour, byte min, bool& activeFlag );
  bool highresActiveFlag( byte day, byte hour, byte min, byte sec );
  float temp( byte day, byte hour, byte min );
  float temp( byte day, byte hour, byte min, bool& activeFlag );
  void writeSettings( Stream& s );
  void readSettings( Stream& s );
  void printProfiles( Stream& s );
  void printProfile( uint8_t id, Stream& s );
  void resetProfile( uint8_t id, bool heating );
  void addProfile( bool heating );
  void setEntry( uint8_t profile, uint8_t entry, Stream& in );
  void addEntry( uint8_t profile, Stream& in );
  void removeEntry( uint8_t profile, uint8_t entry );

  extern byte g_nbValues;

} // namespace TemperatureProfiles

#endif // TemperatureProfiles_h

