#ifndef Defines_h
#define Defines_h

//#define DEBUG if(true)
#define DEBUG if(false)

//#define DEBUG2 if(true)
#define DEBUG2 if(false)

//////////////////////////////////////////////////////////////////////////////

#define SUPPORT_FTP
#define SUPPORT_Network
#define SUPPORT_NTP
#define SUPPORT_TemperatureUploader
#define SUPPORT_UDP_messages
#define SUPPORT_Webserver

//////////////////////////////////////////////////////////////////////////////

#ifndef SUPPORT_Network
  #undef SUPPORT_FTP
  #undef SUPPORT_NTP
  #undef SUPPORT_UDP_messages
  #undef SUPPORT_Webserver
#endif // SUPPORT_Network

#ifndef SUPPORT_FTP
  #undef SUPPORT_TemperatureUploader
#endif // SUPPORT_FTP

//////////////////////////////////////////////////////////////////////////////

#ifdef SUPPORT_UDP_messages
  #define BEGINMSG if(true){ Udp.beginPacket(0xffffffff,12888);Udp <<
  #define ENDMSG ; Udp.endPacket(); }
#else
  #define BEGINMSG if(false){ Serial <<
  #define ENDMSG ; }
#endif

#endif // Defines_h

