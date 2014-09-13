#ifndef NTPClient_h
#define NTPClient_h

#include "Network.h"

#ifdef SUPPORT_NTP

extern void setupNTP();
extern bool updateNTP();

#endif // SUPPORT_NTP
#endif // NTPClient_h
