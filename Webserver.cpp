#include <SD.h>
#include <Time.h>

#include "Network.h"
#include "Sensors.h"
#include "Tools.h"
#include "Webserver.h"

#ifdef SUPPORT_Webserver

extern void interpret( Stream& stream, Stream& out );

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
static EthernetServer server( 80 );

void setupSDCard(){
  Serial << F("Initializing SD card reader.") << endl;
  pinMode( 4, OUTPUT );
  digitalWrite( 4, LOW );
  if ( !SD.begin( 4 ) ){
    Serial << F("  Initialization failed!") << endl;
  }
}

void setupWebserver(){
  setupSDCard();
  Serial << F("Initializing webserver.") << endl;
  Serial << F("  Server is at ") << Ethernet.localIP() << ":80" << endl;
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
  // listen to ethernet for requests
  client = server.available();
  if ( client ){
    static int requestCounter = 0;
    ++requestCounter;
    DEBUG{ Serial << requestCounter << F(". ethernet request detected.") << endl; }
 
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
            //filename = "index.htm";
            filename = "?cmd=status";
          }
          
          if ( *filename == '?' ){
            ++filename;
            if ( !strncmp( filename, "cmd=", 4 ) ){
              filename += 4;
              client << F("HTTP/1.1 200 OK") << endl;
              client << F("Content-Type: text/plain") << endl << endl;
              String s( filename );
              s.replace( "%20", " " );
              StringStream ss( s );
              interpret( ss, client );
              break;
            }
          }
          
          File file = SD.open( filename, FILE_READ );
          if ( !file ){
            client << F("HTTP/1.1 404 Not Found") << endl;
            client << F("Content-Type: text/html") << endl;
            client << endl;
            client << F("<h2>File Not Found!</h2>") << endl;
            break;
          }
          
          strlower( filename );          
          client << F("HTTP/1.1 200 OK") << endl << F("Content-Type: ");
          if ( strstr( filename, ".htm" ) != 0 ){
            client << F("text/html") << endl;
          } else if ( strstr( filename, ".jpg" ) != 0 ){
            client << F("image/jpg") << endl;
          } else if ( strstr( filename, ".js" ) != 0 ){
            client << F("text/html") << endl;
            client << F("Expires: Thu, 15 Apr 2100 20:00:00 GMT") << endl;
          } else {
            client << F("text/plain") << endl;
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
                  client << F("not supported") << endl;
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
          client << F("HTTP/1.1 404 Not Found") << endl;
          client << F("Content-Type: text/html") << endl;
          client << endl;
          client << F("<h2>File Not Found!</h2>") << endl;
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

#endif // SUPPORT_Webserver

