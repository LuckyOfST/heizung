#include "FTPClient.h"

#ifdef SUPPORT_FTP

static char outBuf[ 64 ];

EthernetClient ftpClient;

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
    BEGINMSG(4) F( "FTP disconnected" ) ENDMSG
    return false;
  }
  return true;
}

bool ftpOpen(){
  if ( !client.connect( ftpServer, 21 ) || !ftpOK( false ) ){
    BEGINMSG(4) F( "FTP connection failed" ) ENDMSG
    client.stop();
    return false;
  }
  // login to FTP server
  client << F( "USER heizung" ) << endl;
  if( !ftpOK( false ) ){
    BEGINMSG(4) F("FTP user failed.") ENDMSG
    return false;
  }
  client << F( "PASS hallo" ) << endl;
  if( !ftpOK( false ) ){
    BEGINMSG(4) F("FTP password failed.") ENDMSG
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
      BEGINMSG(4) F( "Bad PASV Answer" ) ENDMSG
      break;
    }
    array_pasv[ i ] = atoi( s );
  }

  unsigned int port = ( array_pasv[ 4 ] << 8 ) | ( array_pasv[ 5 ] & 255 );
  //Serial << F( "Data port: " ) << port << endl;

  if ( !ftpClient.connect( ftpServer, port ) ){
    BEGINMSG(4) F( "Data connection failed" ) ENDMSG
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
  
#endif // SUPPORT_FTP

