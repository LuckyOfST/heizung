#ifndef Defines_h
#define Defines_h

//#define DEBUG if(true)
#define DEBUG if(false)

//#define DEBUG2 if(true)
#define DEBUG2 if(false)

//////////////////////////////////////////////////////////////////////////////

#define SUPPORT_Network
#define SUPPORT_FTP
#define SUPPORT_NTP
#define SUPPORT_TemperatureUploader
#define SUPPORT_UDP_messages
#define SUPPORT_Webserver
//#define SUPPORT_eBus
#define SUPPORT_MQTT

//////////////////////////////////////////////////////////////////////////////

#ifndef SUPPORT_Network
  #undef SUPPORT_FTP
  #undef SUPPORT_NTP
  #undef SUPPORT_UDP_messages
  #undef SUPPORT_Webserver
  #undef SUPPORT_MQTT
#else
  #ifdef SUPPORT_NTP
    #define SUPPORT_UDP_messages
  #endif
#endif // SUPPORT_Network

#ifndef SUPPORT_FTP
  #undef SUPPORT_TemperatureUploader
#endif // SUPPORT_FTP

//////////////////////////////////////////////////////////////////////////////

#ifdef SUPPORT_UDP_messages
  #include "Network.h"

  // Bitmask for UDP debugging informations:
  // 0 - sensor states / temperatures
  // 1 - controller / actor states
  // 2 - job executions
  // 3 - webserver executions
  // 4 - FTP executions
  extern int g_debugMask; // hint: variable is defined in Tools.cpp

  #define BEGINMSG(bit) if(bit==-1||(g_debugMask&(1<<bit))){ Udp.beginPacket(0xffffffff,12888);Udp <<
  #define ENDMSG ; Udp.endPacket(); }
#else
  #define BEGINMSG(bit) if(false){ Serial <<
  #define ENDMSG ; }
#endif

#endif // Defines_h

