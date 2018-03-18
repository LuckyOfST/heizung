#pragma once

#include "Defines.h"

#ifdef SUPPORT_MQTT

namespace MQTT{

  extern void setup();
  extern void loop();
  extern void publishTemp( const String& name, float t );
  extern void publishTempSetup( const String& name, float t, float minLevel, int profile );
  extern void publishSwitch( const String& name, bool on );

} // namespace MQTT

#endif
