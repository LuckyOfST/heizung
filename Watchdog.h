#ifndef Watchdog_h
#define Actor_h

// Starts a watchdog timer which will reset the Arduino if stopWatchdog() is not called within approx. 15 minutes.
extern void startWatchdog();

// Stops the watchdog timer. Never forget to call this after startWatchdog() has been called!
extern void stopWatchdog();

#endif
