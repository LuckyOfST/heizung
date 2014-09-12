#ifndef Network_h
#define Network_h

#include <Ethernet.h>
#include <Streaming.h>

#include "Defines.h"

#ifdef SUPPORT_Network

#ifdef SUPPORT_FTP
  extern IPAddress ftpServer;
#endif // SUPPORT_FTP

#ifdef SUPPORT_NTP
  extern IPAddress timeServer;
#endif // SUPPORT_NTP

extern EthernetUDP Udp;
extern EthernetClient client;

extern void setupNetwork();

#endif // SUPPORT_Network
#endif // Network_h

