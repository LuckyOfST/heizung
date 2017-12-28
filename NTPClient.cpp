#include "NTPClient.h"
#include "Tools.h"

#ifdef SUPPORT_NTP

unsigned int localNtpPort = 8888;  // local port to listen for UDP packets

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
//byte packetBuffer[ NTP_PACKET_SIZE ]; //buffer to hold incoming & outgoing packets
#define packetBuffer g_buffer

// send an NTP request to the time server at the given address
void sendNTPpacket( IPAddress& address ){
  // set all bytes in the buffer to 0
  memset( packetBuffer, 0, NTP_PACKET_SIZE );
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[ 0 ] = 0b11100011;   // LI, Version, Mode
  packetBuffer[ 1 ] = 0;     // Stratum, or type of clock
  packetBuffer[ 2 ] = 6;     // Polling Interval
  packetBuffer[ 3 ] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[ 12 ]  = 49;
  packetBuffer[ 13 ]  = 0x4E;
  packetBuffer[ 14 ]  = 49;
  packetBuffer[ 15 ]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket( address, 123 ); //NTP requests are to port 123
  Udp.write( (const uint8_t*)packetBuffer, NTP_PACKET_SIZE );
  Udp.endPacket();
}

time_t getNtpTime(){
  static bool showSuccess = true;
  static bool showFailure = true;
  while ( Udp.parsePacket() > 0 ); // discard any previously received packets
  //Serial << "    Updating NTP time..." << endl;
  sendNTPpacket( timeServer );
  uint32_t beginWait = millis();
  while ( millis() - beginWait < 1500 ){
    int size = Udp.parsePacket();
    if ( size >= NTP_PACKET_SIZE ){
      Udp.read( packetBuffer, NTP_PACKET_SIZE );  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[ 40 ] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[ 41 ] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[ 42 ] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[ 43 ];
      time_t t = secsSince1900 - 2208988800UL;// + timeZone * SECS_PER_HOUR;
      // adjust time zone (europe with daylight saving time)
      TimeElements te;
      breakTime( t, te );
      // last sunday of march
      int beginDSTDate = ( 31 - ( 5 * te.Year / 4 + 4 ) % 7 );
      int beginDSTMonth = 3;
      //last sunday of october
      int endDSTDate = ( 31 - ( 5 * te.Year / 4 + 1 ) % 7 );
      int endDSTMonth = 10;
      // DST is valid as:
      if ( ( ( te.Month > beginDSTMonth ) && ( te.Month < endDSTMonth ) )
         || ( ( te.Month == beginDSTMonth ) && ( te.Day >= beginDSTDate ) ) 
         || ( ( te.Month == endDSTMonth ) && ( te.Day <= endDSTDate ) ) )
      {
        t += 7200;
      } else {
        t += 3600;
      }
      breakTime( t, te );
      if ( showSuccess ){
        showFailure = true;
        showSuccess = false;
        BEGINMSG(-1) F("Time was synchronized to NTP time ") << lz(te.Day) << F(".") << lz(te.Month) << F(".") << 1970 + te.Year << F(" ") << lz(te.Hour) << F(":") << lz(te.Minute) << F(":") << lz(te.Second) << F(".") ENDMSG
      }
      if ( !hasStartTime() ){
        setSyncInterval( 60 * 60 * 6 );
        setTime( t );
        setStartTime();
      }
      return t;
    }
  }
  if ( showFailure ){
    showFailure = false;
    showSuccess = true;
    BEGINMSG(-1) F("ERROR: No NTP time available! Time update could not be executed!") ENDMSG
  }
  return 0; // return 0 if unable to get the time
}

bool updateNTP(){
  if ( timeStatus() == timeNotSet || timeStatus() == timeNeedsSync ){
    DEBUG{ Serial << F("getNtpTime START") << endl; }
    time_t t = getNtpTime();
    DEBUG{ Serial << F("getNtpTime END") << endl; }
    if ( t != 0 ){
      return true;
    }
    return false;
  }
  return true;
}

void setupNTP(){
  Serial << F("Initializing time using NTP.") << endl;
  Udp.begin( localNtpPort );
  setSyncProvider( getNtpTime );
  setSyncInterval( 30 ); // initially we try to sync every 30 seconds until we get the first time; this will change the interval to once a day.
  if ( !updateNTP() ){
    Serial << F("  Failed to get time using NTP.") << endl;
  } else {
    Serial << F("  ");
    writeTime( Serial );
  }
}

#endif // SUPPORT_NTP

