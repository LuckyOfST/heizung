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
  
  extern byte g_nbValues;

} // namespace TemperatureProfiles

#endif // TemperatureProfiles_h

