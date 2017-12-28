#include "Network.h"
#include "NTPClient.h"
#include "Tools.h"
#include "Webserver.h"

#ifdef SUPPORT_Network

#define USE_DHCP

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static IPAddress ip( 192, 168, 178, 177 );
static IPAddress gateway( 192, 168, 178, 1 );
static IPAddress subnet( 255, 255, 255, 0 );

#ifdef SUPPORT_FTP
  IPAddress ftpServer( 192, 168, 178, 1 );
#endif // SUPPORT_FTP

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
  Serial << F("Initializing ethernet controller.") << endl;
  pinMode( 10, OUTPUT );
  digitalWrite( 10, LOW );
#ifdef USE_DHCP
  if ( Ethernet.begin( mac ) ){
    Serial << F("  Receiving IP using DHCP.") << endl;
  } else {
    Serial << F("  Failed receiving IP using DHCP.") << endl;
    Ethernet.begin( mac, ip, gateway, subnet );
  }
#else
  Ethernet.begin( mac, ip, gateway, subnet );
#endif // USE_DHCP
  Serial << F("  IP: ") << Ethernet.localIP() << endl;
#ifdef SUPPORT_NTP
  g_eeprom.setCurrentBaseAddr( EEPROM_NTPSERVERADDR );
  for ( int i = 0; i < 4; ++i ) {
    timeServer[ i ] = g_eeprom.read();
  }
  setupNTP();
#endif // SUPPORT_NTP
#ifdef SUPPORT_Webserver
  setupWebserver();
#endif // SUPPORT_Webserver
#ifdef SUPPORT_FTP
  g_eeprom.setCurrentBaseAddr( EEPROM_FTPSERVERADDR );
  for ( int i = 0; i < 4; ++i ) {
    ftpServer[ i ] = g_eeprom.read();
  }
#endif // SUPPORT_FTP
}

#endif // SUPPORT_Network

