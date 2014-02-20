// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip( 192, 168, 178, 177 );
IPAddress gateway( 192, 168, 178, 1 );
IPAddress subnet( 255, 255, 255, 0 );
IPAddress ftpServer( 192, 168, 178, 1 );

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server( 80 );

IPAddress timeServer( 192, 168, 178, 1 ); // fritz.box
//IPAddress timeServer2( 65, 55, 56, 206 ); // time.windows.com
//IPAddress timeServer3( 24, 56, 178, 140 ); // time.nist.gov
//IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov NTP server
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov NTP server

EthernetUDP Udp;
unsigned int localNtpPort = 8888;  // local port to listen for UDP packets

EthernetClient client;

void setupSDCard(){
  Serial << F("  Initializing SD card reader.") << endl;
  pinMode( 4, OUTPUT );
  digitalWrite( 4, LOW );
  if ( !SD.begin( 4 ) ){
    Serial << F("  Initialization failed!") << endl;
  }
}

EthernetClient ftpClient;
char outBuf[ 64 ];

bool ftpOpen(){
  if ( !client.connect( ftpServer, 21 ) || !ftpOK( false ) ){
    BEGINMSG F( "FTP connection failed" ) ENDMSG
    client.stop();
    return false;
  }
  // login to FTP server
  client << F( "USER heizung" ) << endl;
  if( !ftpOK( false ) ){
    BEGINMSG F("FTP user failed.") ENDMSG
    return false;
  }
  client << F( "PASS hallo" ) << endl;
  if( !ftpOK( false ) ){
    BEGINMSG F("FTP password failed.") ENDMSG
    return false;
  }
  /*client << F( "SYST" ) << endl;
  if ( !ftpOK( false ) ){
    return false;
  }*/
  client << F( "PASV" ) << endl;
  if ( !ftpOK( false ) ){
    return false;
  }
  char *s = strtok( outBuf, "(," );
  int array_pasv[ 6 ];
  for ( int i = 0; i < 6; ++i ){
    s = strtok( 0, "(," );
    if( s == 0 ){
      BEGINMSG F( "Bad PASV Answer" ) ENDMSG
      break;
    }
    array_pasv[ i ] = atoi( s );
  }

  unsigned int port = ( array_pasv[ 4 ] << 8 ) | ( array_pasv[ 5 ] & 255 );
  //Serial << F( "Data port: " ) << port << endl;

  if ( !ftpClient.connect( ftpServer, port ) ){
    BEGINMSG F( "Data connection failed" ) ENDMSG
    client.stop();
    return false;
  }

  return true;
}

bool ftpFileExists( const char* fname ){
  client << F("SIZE ") << fname << endl;
  if ( !ftpOK( true ) ){
    return false;
  }
  return outBuf[ 0 ] != '0';
}
    
bool ftpSetMode( const char* fname, const char* mode, bool ignoreErrors ){
  client << mode << F(" ") << fname << endl;

  if( !ftpOK( ignoreErrors ) && !ignoreErrors ){
    ftpClient.stop();
    return false;
  }
  return true;
}

void ftpClose(){
  ftpClient.stop();
  //Serial << "stopped" << endl;
  //Serial << "send quit" << endl;
  client << F( "QUIT" ) << endl;
  if( !ftpOK( false ) ){
    return;
  }
  //Serial << "quit" << endl;
  client.stop();
  //Serial << "client stopped" << endl;
  //Serial << F( "Command disconnected" ) << endl;
}
  
bool ftpOK( bool ignoreErrors ){
  //DEBUG{ Serial << F("ftpOK: wait for client") << endl; }
  while( client.connected() && !client.available() ){
    delay( 1 );
  }
  //Serial << "client available" << endl;
  byte respCode = client.peek();
  byte outCount = 0;
  //DEBUG{ Serial << F("ftpOK: read response") << endl; }
  while( client.available() ){  
    byte thisByte = client.read();    
    //Serial << (char)thisByte;
    if( outCount < sizeof( outBuf ) - 1 ){
      outBuf[ outCount ] = thisByte;
      ++outCount;
    }
  }
  //DEBUG{ Serial << F("ftpOK: received response") << endl; }
  outBuf[ outCount ] = 0;
  if( respCode >= '4' ){
    if ( ignoreErrors ){
      return false;
    }
    client << F( "QUIT" ) << endl;
    while( client.connected() && !client.available() ){
      delay(1);
    }
    while( client.available() ){  
      byte thisByte = client.read();    
      Serial << (char)thisByte;
    }
    client.stop();
    BEGINMSG F( "FTP disconnected" ) ENDMSG
    return false;
  }
  return true;
}

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
//byte packetBuffer[ NTP_PACKET_SIZE ]; //buffer to hold incoming & outgoing packets
#define packetBuffer g_buffer

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
        BEGINMSG F("Time was synchronized to NTP time ") << lz(te.Day) << F(".") << lz(te.Month) << F(".") << 1970 + te.Year << F(" ") << lz(te.Hour) << F(":") << lz(te.Minute) << F(":") << lz(te.Second) << F(".") ENDMSG
      }
      return t;
    }
  }
  if ( showFailure ){
    showFailure = false;
    showSuccess = true;
    BEGINMSG F("ERROR: No NTP time available! Time update could not be executed!") ENDMSG
  }
  return 0; // return 0 if unable to get the time
}

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

void setupNTP(){
  Serial << F("  Initializing time using NTP.") << endl;
  Udp.begin( localNtpPort );
  setSyncProvider( getNtpTime );
  setSyncInterval( 60 * 60 * 24 );
}

void setupWebserver(){
  Serial << F("  Initializing Ethernet controller.")
  << endl;
  pinMode( 10, OUTPUT );
  digitalWrite( 10, LOW );
  Ethernet.begin( mac, ip, gateway, subnet );
  server.begin();
  Serial << "    Server is at " << Ethernet.localIP() << ":" << 80 << endl;
  setupSDCard();
  setupNTP();  
}

const char* readLine( Stream& stream ){
  static char text[ 256 ];
  byte length = stream.readBytesUntil( ' ', text, sizeof( text ) / sizeof( char ) );
  text[ length ] = 0;
  return text;
}

// How big our line buffer should be. 100 is plenty!
#define BUFSIZ 100

void execWebserver(){
  if ( timeStatus() == timeNotSet || timeStatus() == timeNeedsSync ){
    DEBUG{ Serial << F("getNtpTime START") << endl; }
    time_t t = 0;//getNtpTime();
    DEBUG{ Serial << F("getNtpTime END") << endl; }
    if ( t != 0 ){
      setTime( t );
    }
  }
  
  // listen to ethernet for requests
  client = server.available();
  if ( client ){
    static int requestCounter = 0;
    ++requestCounter;
    DEBUG{ Serial << requestCounter << ". ethernet request detected." << endl; }
 
    char clientline[BUFSIZ];
    int index = 0;
  
    // an http request ends with a blank line
    boolean current_line_is_blank = true;
    
    DEBUG{ Serial << F("ethernet START") << endl; }
    while ( client.connected() ){
      if ( client.available() ){
        char c = client.read();
        // If it isn't a new line, add the character to the buffer
        if ( c != '\n' && c != '\r' ){
          clientline[ index ] = c;
          ++index;
          // are we too big for the buffer? start tossing out data
          if ( index >= BUFSIZ ){ 
            index = BUFSIZ - 1;
          }
          // continue to read more data!
          continue;
        }
        
        // got a \n or \r new line, which means the string is done
        clientline[ index ] = 0;

        Serial << clientline << endl;
        
        // Look for substring such as a request to get the root file
        if ( strstr( clientline, "GET /" ) != 0 ){
          // this time no space after the /, so a sub-file!
          char* filename;
          
          filename = clientline + 5; // look after the "GET /" (5 chars)
          // a little trick, look for the " HTTP/1.1" string and 
          // turn the first character of the substring into a 0 to clear it out.
          ( strstr( clientline, " HTTP" ) )[ 0 ] = 0;

          if ( *filename == 0 ){
            filename = "index.htm";
          }
          
          if ( *filename == '?' ){
            ++filename;
            if ( !strncmp( filename, "cmd=", 4 ) ){
              filename += 4;
              client << "HTTP/1.1 200 OK" << endl;
              client << "Content-Type: text/plain" << endl << endl;
              String s( filename );
              s.replace( "%20", " " );
              StringStream ss( s );
              interpret( ss, client );
              break;
            }
          }
          
          File file = SD.open( filename, FILE_READ );
          if ( !file ){
            client << "HTTP/1.1 404 Not Found" << endl;
            client << "Content-Type: text/html" << endl;
            client << endl;
            client << "<h2>File Not Found!</h2>" << endl;
            break;
          }
          
          strlower( filename );          
          client << "HTTP/1.1 200 OK" << endl;
          if ( strstr( filename, ".htm" ) != 0 ){
            client << "Content-Type: text/html" << endl;
          } else if ( strstr( filename, ".jpg" ) != 0 ){
            client << "Content-Type: image/jpg" << endl;
          } else if ( strstr( filename, ".js" ) != 0 ){
            client << "Content-Type: text/html" << endl;
            client << "Expires: Thu, 15 Apr 2100 20:00:00 GMT" << endl;
          } else {
            client << "Content-Type: text/plain" << endl;
          }
          client << endl;
          
          int16_t c;
          uint8_t buffer[ 256 ];
          int pos = 0;
          while ( ( c = file.read() ) >= 0 ){
            char ch = (char)c;
            if ( ch == (char)'§' ){
              if ( pos > 0 ){
                client.write( buffer, pos );
                pos = 0;
              }
              ch = (char)file.read();
              switch ( ch ){
              case (char)'C':
                {
                  file.read();
                  const char* id = readLine( file );
                  client << "not supported" << endl;
                } break;
              case (char)'T':
                {
                  file.read();
                  const char* id = readLine( file );
                  const Sensor* sensor = findSensor( id );
                  if ( sensor ){
                    client << _FLOAT( sensor->_temp, 1 );
                  } else {
                    client << 0;
                  }
                } break;
              case (char)'R':
                {
                  client << FreeRam();
                } break;
              case (char)'N':
                {
                  TimeElements te;
                  breakTime( now(), te );
                  client << lz( te.Day ) << "." << lz( te.Month ) << "." << lz( 1970 + te.Year ) << " " << lz( te.Hour ) << ":" << lz( te.Minute ) << ":" << lz( te.Second );
                } break;
              }
              continue;
            }
            if ( pos == sizeof( buffer ) ){
              client.write( buffer, pos );
              pos = 0;
            }
            //client << ch;
            buffer[ pos ] = ch;
            ++pos;
          }
          if ( pos > 0 ){
            client.write( buffer, pos );
            pos = 0;
          }
          file.close();
        } else {
          // everything else is a 404
          client << "HTTP/1.1 404 Not Found" << endl;
          client << "Content-Type: text/html" << endl;
          client << endl;
          client << "<h2>File Not Found!</h2>" << endl;
        }
        break;
      }
    }
    client.flush();
    // give the web browser time to receive the data
    delay( 1 );
    client.stop();
    DEBUG{ Serial << F("ethernet STOP") << endl; }
  }
}

char* strlower( char* s ){
  for( char* p = s; *p; ++p ){
    *p = tolower( *p );
  }
  return s;
}


