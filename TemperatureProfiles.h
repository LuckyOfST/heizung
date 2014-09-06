#ifndef TemperatureProfiles_h
#define TemperatureProfiles_h

#include "Tools.h"

namespace TemperatureProfiles{

  byte getNbProfiles();
  void setCurrentProfile( byte id );
  byte getDays( int idx );
  byte getStartTime( int idx );
  float getTemp( int idx );
  float temp( byte day, byte hour, byte min );
  void writeSettings( Stream& s );
  void readSettings( Stream& s );
  
  extern byte g_nbValues;

} // namespace TemperatureProfiles

#endif // TemperatureProfiles_h

