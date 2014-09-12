#include "Network.h"
#include "NTPClient.h"
#include "Webserver.h"

#ifdef SUPPORT_Network

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static IPAddress ip( 192, 168, 178, 177 );
static IPAddress gateway( 192, 168, 178, 1 );
static IPAddress subnet( 255, 255, 255, 0 );

#ifdef SUPPORT_FTP
  IPAddress ftpServer( 192, 168, 178, 1 );
#endif

#ifdef SUPPORT_NTP
  IPAddress timeServer( 192, 168, 178, 1 ); // fritz.box
  //IPAddress timeServer2( 65, 55, 56, 206 ); // time.windows.com
  //IPAddress timeServer3( 24, 56, 178, 140 ); // time.nist.gov
  //IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server
  // IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov NTP server
  // IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov NTP server
#endif // SUPPORT_NTP

EthernetUDP Udp;
EthernetClient client;

void setupNetwork(){
  Serial << F("  Initializing Ethernet controller.") << endl;
  pinMode( 10, OUTPUT );
  digitalWrite( 10, LOW );
  Ethernet.begin( mac, ip, gateway, subnet );
#ifdef SUPPORT_NTP
  setupNTP();
#endif // SUPPORT_NTP
#ifdef SUPPORT_Webserver
  setupWebserver();
  Serial << "    Server is at " << Ethernet.localIP() << ":80" << endl;
#endif // SUPPORT_Webserver
}

#endif // SUPPORT_Network

