#include <SD.h>
#include <TimeLib.h>

#include "Actor.h"
#include "Commands.h"
#include "Controller.h"
#include "FTPClient.h"
#include "NTPClient.h"
#include "Sensors.h"
#include "TemperatureProfiles.h"
#include "Tools.h"

#ifdef SUPPORT_FTP
#include "Network.h"

void printIP( const IPAddress& ip, Stream& out ){
  out << ip[ 0 ] << '.' << ip[ 1 ] << '.' << ip[ 2 ] << '.' << ip[ 3 ];
}

void getIP( Stream& in, IPAddress& ip ){
  for ( int i = 0; i < 4; ++i ){
    ip[ i ] = (uint8_t)in.parseInt();
  }
}
#endif

extern void( *delayedCmd )();

void cmdHelp( Stream& in, Stream& out ){
  out << F( "Supported Commands:" );
  endline( out );
  commandText( out, true );
  while( commandText( out ) ){
    commandText( out );
    out << ": ";
    commandText( out );
    endline( out );
  }
}

void cmdDetect( Stream& in, Stream& out ){
  out << F( "Detection started..." );
  endline( out );
  detectSensors( out );
  out << F( "Detection finished." );
  endline( out );
}

void cmdErrors( Stream& in, Stream& out ){
  writeSensorsErrors( out );
}

void cmdResetErrors( Stream& in, Stream& out ){
  for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
    getSensor( i )._errorCount = 0;
  }
  out << F( "Error counters resetted." );
  endline( out );
}

void status( Stream& in, Stream& out, bool includeActors ){
  //htmlMode = true;
  out << F( "<style>{font-family:Arial;}table{border:1px solid gray;empty-cells:show;border-collapse:collapse;}td{text-align:right;}</style>" );
  writeTime( out );
  out << F( "Running since " );
  writeTime( out, g_startTime );
  endline( out );
  out << F("Begin status");
  endline( out );
  out << F( "<h1>Sensors</h1><table><tr><th>Sensor<th>Temp<th>Errors<th>Age" );
  //endline( out );
  for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
    Sensor& s = getSensor( i );
    if ( !s.valid() ){
      //out << F("    '") << s._name << F("' invalid.");
      //endline( out );
      out << F( "<tr><td align=left>" ) << s._name << F( "<td>invalid" );
      continue;
    }
    // requesting an update leads to a long time until the result is presented...
    //s.update( true );
    int age = now() - s._lastChange;
    //out << F( "    '" ) << s._name << F( "': " ) << _FLOAT( s._temp, 1 ) << F( " C (" ) << s._errorCount << F( " errors, age: " ) << age << F( " s)" );
    out << F( "<tr><td align=left>" ) << s._name << F( "<td>" ) << _FLOAT( s._temp, 1 ) << F( " C<td>" ) << s._errorCount << F( "<td>" ) << age << F( " s" );
    //endline( out );
    s.sendStatus();
  }
  out << F( "</table><h1>Controllers</h1><table><tr><th>Controller<th>Profile<th>Temp<th>Status<th>Actors" );
  endline( out );
  for( unsigned short i = 0; i < CONTROLLER_COUNT; ++i ){
    if ( g_controller[ i ]->isSwitch() ){
      continue;
    }
    out << F("<tr><td align=left>") << g_controller[ i ]->getName() << F("<td>");
    if ( g_controller[ i ]->getForcedLevel() == -1 ){
      if ( g_controller[ i ]->isProfileActive() ){
        out << g_controller[ i ]->getProfileID();
      }
      out << F( "<td>" ) << _FLOAT( g_controller[ i ]->getT(), 1 ) << F( " C<td>" );
    } else {
      out << F("<td><td>forced to level ") << g_controller[ i ]->getForcedLevel();
    }
    if ( !g_controller[ i ]->working() ){
      out << F( "[OUT OF FUNCTION] " );
    }
    g_controller[ i ]->printStatus( out );
    out << F( "<td>" );
    if ( includeActors ){
      g_controller[ i ]->printActors( out );
    }
    //endline( out );
  }
  out << F( "</table>" );
  out << (includeActors ? F( "  ACTORS:" ) : F( "  SWITCHES:" ));
  endline( out );
  for ( unsigned short i = includeActors ? 0 : HEATING_ACTOR_COUNT; i < ACTOR_COUNT; ++i ){
    out << F( "    '" ) << g_actors[ i ]->getName() << F( "' " );
    if ( g_actors[ i ]->getMode() != Actor::Standard ){
      out << F( "FORCED " );
    }
    out << (g_actors[ i ]->isOpen() ? F( "ON" ) : F( "OFF" ));
    endline( out );
  }
  out << F( "  TEMPERATURE PROFILES:" );
  endline( out );
  out << F("    ");
  //g_tempProfile.writeSettings( out );
  TemperatureProfiles::writeSettings( out );
  TemperatureProfiles::printProfiles( out );
  out << F( "  CURRENT AMPERAGE: " ) << Actor::getCurrentUsedAmperage();
  endline( out );
  out << F( "  FREE RAM: " ) << FreeRam() << F( " bytes" );
  endline( out );
#ifdef SUPPORT_FTP
  out << F( "  FTP SERVER: " );
  printIP( ftpServer, out );
  endline( out );
#endif
#ifdef SUPPORT_NTP
  out << F( "  NTP SERVER: " );
  printIP( timeServer, out );
  endline( out );
#endif
#ifdef SUPPORT_UDP_messages
  out << F( "  UDP LOGGING MASK: " ) << g_debugMask;
  endline( out );
#endif
  out << F( "End status" );
  endline( out );
}

void cmdStatus( Stream& in, Stream& out ){
  status( in, out, false );
}

void cmdFullStatus( Stream& in, Stream& out ){
  status( in, out, true );
}

void cmdSet( Stream& in, Stream& out ){
  const char* name = readText( in );
  Controller* c = Controller::find( name );
  if ( c ){
    float oldT = c->getTargetT();
    c->readSettings( in );
    out << F( "Changed controller '" ) << name << F( "' from " ) << _FLOAT( oldT, 1 ) << F( " C to " ) << _FLOAT( c->getTargetT(), 1 ) << F( " C target temperature. (0 means profile mode)" );
    endline( out );
  } else {
    out << F( "Unable to find controller '" ) << name << F( "'." );
    endline( out );
  }
}

void cmdSetMinLevel( Stream& in, Stream& out ){
  const char* name = readText( in );
  float l = in.parseFloat();
  if ( !strcmp( name, "all" ) ){
    for( int i = 0; i < CONTROLLER_COUNT; ++i ){
      if ( !g_controller[ i ]->isSwitch() ){
        g_controller[ i ]->setMinimumLevel( l );
      }
    }
    out << F( "Changed all controller minimum levels to " ) << _FLOAT( l, 2 ) << '.';
    endline( out );
  } else {
    Controller* c = Controller::find( name );
    if ( c ){
      float oldL = c->getMinimumLevel();
      c->setMinimumLevel( l );
      out << F( "Changed controller '" ) << name << F( "' minimum level from " ) << _FLOAT( oldL, 1 ) << F( " to " ) << _FLOAT( l, 2 ) << '.';
      endline( out );
    } else {
      out << F( "Unable to find controller '" ) << name << F( "'." );
      endline( out );
    }
  }
}

void cmdResetControllerTemperatures( Stream& in, Stream& out ){
  for( int i = 0; i < CONTROLLER_COUNT; ++i ){
    g_controller[ i ]->setTargetT( 21. );
    g_controller[ i ]->setMinimumLevel( 0. );
  }
  out << F( "All controller target temperatures and minimum levels resetted to 21 C / 0%." );
  endline( out );
}

void cmdResetControllerProfiles( Stream& in, Stream& out ){
  for( int i = 0; i < CONTROLLER_COUNT; ++i ){
    g_controller[ i ]->setProfileID( 0 );
  }
  out << F( "All controller temperature profiles resetted to profile 0." );
  endline( out );
}

void cmdSetProfile( Stream& in, Stream& out ){
  const char* name = readText( in );
  Controller* c = Controller::find( name );
  if ( c ){
    int i = in.parseInt();
    c->setProfileID( i );
    out << F( "Profile for controller '" ) << name << F( "' set to profile " ) << i;
    endline( out );
  }
}

void cmdGet( Stream& in, Stream& out ){
  const char* name = readText( in );
  Controller* c = Controller::find( name );
  if ( c ){
    out << F( "Target temperature for controller '" ) << name << F( "' is set to " ) << _FLOAT( c->getTargetT(), 1 ) << F( " C." );
    endline( out );
  } else {
    out << F( "Unable to find controller '" ) << name << F( "'." );
    endline( out );
  }
}

void cmdGetMinLevels( Stream& in, Stream& out ){
  out << F("Controller minimum levels") << endl;
  for( unsigned short i = 0; i < CONTROLLER_COUNT; ++i ){
    out << F( "  '" ) << g_controller[ i ]->getName() << F( "': " ) << _FLOAT( g_controller[ i ]->getMinimumLevel(), 2 );
    endline( out );
  }
}

void cmdGetProfiles( Stream& in, Stream& out ){
  TemperatureProfiles::writeSettings( out );
}

void cmdSetProfiles( Stream& in, Stream& out ){
  TemperatureProfiles::readSettings( in );
}

void cmdGetTemp( Stream& in, Stream& out ){
  int id = in.parseInt();
  int dow = in.parseInt();
  int h = in.parseInt();
  int m = in.parseInt();
  TemperatureProfiles::setCurrentProfile( id );
  out << _FLOAT( TemperatureProfiles::temp( dow, h, m ), 1 );
  endline( out );
}

void cmdForceActor( Stream& in, Stream& out ){
  const char* name = readText( in );
  out << F( "Actor " ) << name;
  int m;
  if ( !strcmp( name, "all" ) ){
    m = in.parseInt() + 1;
    for( int i = 0; i < ACTOR_COUNT; ++i ){
      if ( !g_actors[ i ]->isSwitch() ){
        g_actors[ i ]->setMode( (Actor::Mode)( 1 << m ) );
      }
    }
  } else {
    Actor* a = Actor::findActor( name );
    if ( !a ){
      out << F( " not found." );
      endline( out );
      return;
    }
    m = in.parseInt() + 1;
    a->setMode( (Actor::Mode)( 1 << m ) );
  }
  switch ( m ){
  case 0:
    out << F( " set to standard operating mode." );
    endline( out );
    break;
  case 1:
    out << F( " forced to be OFF." );
    endline( out );
    break;
  case 2:
    out << F( " forced to be ON." );
    endline( out );
    break;
  }
}

void cmdForceController( Stream& in, Stream& out ){
  const char* name = readText( in );
  float level = in.parseFloat();
  if ( !strcmp( name, "all" ) ){
    for( unsigned short i = 0; i < CONTROLLER_COUNT; ++i ){
      if ( !g_controller[ i ]->isSwitch() ){
        g_controller[ i ]->setForcedLevel( level );
      }
    }
    out << F("All controllers");
  } else {
    out << F( "Controller " ) << name;
    Controller* c = Controller::find( name );
    if ( !c ){
      out << F( " not found." );
      endline( out );
      return;
    }
    c->setForcedLevel( level );
  }
  if ( level == -1 ){
    out << F( " set to standard operating mode." );
    endline( out );
  } else {
    out << F( " forced to level " ) << level << '.';
    endline( out );
  }
}
    
void cmdSetTime( Stream& in, Stream& out ){
  int d = in.parseInt();
  int mo = in.parseInt();
  int y = in.parseInt();
  int h = in.parseInt();
  int m = in.parseInt();
  int s = in.parseInt();
  if ( d > 0 ){
#ifdef SUPPORT_NTP
    setSyncProvider( 0 );
    setSyncInterval( 60 * 60 * 6 );
#endif
    setTime( h, m, s, d, mo, y );
    setStartTime();
    out << F( "Time set to " );
  }
#ifdef SUPPORT_NTP
  else{
    setSyncProvider( getNtpTime );
    setSyncInterval( 60 * 60 * 6 );
    getNtpTime();
    out << F( "Activated NTP: " );
  }
#endif
  out << lz( day() ) << '.' << lz( month() ) << '.' << year() << ' ' << lz( hour() ) << ':' << lz( minute() ) << ':' << lz( second() );
  endline( out );
}

void cmdGetTime( Stream& in, Stream& out ){
  writeTime( out );
}

void cmdRequestTemp( Stream& in, Stream& out ){
  out << F( "Request received..." );
  endline( out );
  delayedCmd = &requestTemp;
}

void cmdTestActors( Stream& in, Stream& out ){
  testActors();
}

#if defined( ACTORS_COMMTYPE_CRYSTAL64IO )
void cmdResetI2C( Stream& in, Stream& out ){
  out << F( "Trying to re-initialize i2c bus..." );
  endline( out );
  Actor::setupI2C();
}
#endif

#ifdef SUPPORT_FTP
void cmdSetFtpServer( Stream& in, Stream& out ) {
  getIP( in, ftpServer );
  g_eeprom.setCurrentBaseAddr( EEPROM_FTPSERVERADDR );
  for ( int i = 0; i < 4; ++i ) {
    g_eeprom.write( ftpServer[ i ] );
  }
  out << F( "FTP server set to " );
  printIP( ftpServer, out );
  endline( out );
}
#endif

#ifdef SUPPORT_NTP
void cmdSetNtpServer( Stream& in, Stream& out ) {
  getIP( in, timeServer );
  g_eeprom.setCurrentBaseAddr( EEPROM_NTPSERVERADDR );
  for ( int i = 0; i < 4; ++i ) {
    g_eeprom.write( timeServer[ i ] );
  }
  out << F( "NTP server set to " );
  printIP( timeServer, out );
  endline( out );
  setupNTP();
}
#endif

void cmdShowProfiles( Stream& in, Stream& out ){
  TemperatureProfiles::printProfiles( out );
}

void cmdResetProfile( Stream& in, Stream& out ){
  uint8_t pid = in.parseInt();
  bool heating = in.parseInt();
  TemperatureProfiles::resetProfile( pid, heating );
  TemperatureProfiles::printProfiles( out );
}

void cmdAddProfile( Stream& in, Stream& out ){
  bool heating = in.parseInt();
  TemperatureProfiles::addProfile( heating );
  TemperatureProfiles::printProfiles( out );
}

void cmdAddEntry( Stream& in, Stream& out ){
  uint8_t pid = in.parseInt();
  TemperatureProfiles::addEntry( pid, in );
  TemperatureProfiles::printProfile( pid, out );
}

void cmdSetEntry( Stream& in, Stream& out ){
  uint8_t pid = in.parseInt();
  uint8_t eid = in.parseInt();
  TemperatureProfiles::setEntry( pid, eid, in );
  TemperatureProfiles::printProfile( pid, out );
}

void cmdRemoveEntry( Stream& in, Stream& out ){
  uint8_t pid = in.parseInt();
  uint8_t eid = in.parseInt();
  TemperatureProfiles::removeEntry( pid, eid );
  TemperatureProfiles::printProfile( pid, out );
}

#ifdef SUPPORT_FTP
void cmdSaveSettings( Stream& in, Stream& out ){
  bool success = ftpOpen();
  if ( success ){
    StringStream dir;
    dir << F( "HeizungSettings" );
    ftpSetMode( dir.c_str(), FTP_MAKEDIR, true );
    StringStream fname;
    fname << F( "HeizungSettings/" ) << readText( in ) << F( ".txt" );
    if ( ftpSetMode( fname.c_str(), FTP_STORE, false ) ){
      ftpClient << F( "date,time,currentAmperage,requiredAmperage,freeRAM" );
      for ( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
        ftpClient << ',' << getSensor( i )._name;
      }
      ftpClient << endl;
      ftpClient << lz( day() ) << '.' << lz( month() ) << '.' << year() << ',' << lz( hour() ) << ':' << lz( minute() ) << ':' << lz( second() );
      ftpClient << ',' << Actor::getCurrentUsedAmperage() << ',' << Actor::getCurrentRequiredAmperage() << ',' << FreeRam();
      for ( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
        ftpClient << ',';
        const Sensor& sensor = getSensor( i );
        if ( sensor.valid() ){
          ftpClient << _FLOAT( sensor._temp, 1 );
        }
      }
      ftpClient << endl;
    }
    ftpClose();
  }
  Serial << F( "Temperatures upload " ) << (success ? F( "succeded" ) : F( "failed" )) << endl;
}
#endif

#ifdef SUPPORT_UDP_messages
void cmdSetUdpLoggingMask( Stream& in, Stream& out ) {
  g_debugMask = in.parseInt();
  g_eeprom.setCurrentBaseAddr( EEPROM_UDP_LOGGING_MASK_BASE );
  g_eeprom.writeInt( g_debugMask );
  out << F( "Debug mask set to " ) << g_debugMask;
  endline( out );
}
#endif

Command commands[] = {
  cmdHelp,
  cmdDetect,
  cmdErrors,
  cmdResetErrors,
  cmdStatus,
  cmdFullStatus,
  cmdSet,
  cmdSetMinLevel,
  cmdResetControllerTemperatures,
  cmdResetControllerProfiles,
  cmdSetProfile,
  cmdGet,
  cmdGetMinLevels,
  cmdGetProfiles,
  cmdSetProfiles,
  cmdGetTemp,
  cmdForceActor,
  cmdForceController,
  cmdSetTime,
  cmdGetTime,
  cmdRequestTemp,
#if defined( ACTORS_COMMTYPE_CRYSTAL64IO )
  cmdResetI2C,
#endif
  cmdTestActors,
#ifdef SUPPORT_FTP
  cmdSetFtpServer,
#endif
#ifdef SUPPORT_NTP
  cmdSetNtpServer,
#endif
#ifdef SUPPORT_UDP_messages
  cmdSetUdpLoggingMask,
#endif
  cmdShowProfiles,
  cmdResetProfile,
  cmdAddProfile,
  cmdAddEntry,
  cmdSetEntry,
  cmdRemoveEntry
};

const char commandDescs[] PROGMEM =
"help\0\0" "Shows this help info.\0"
"detect\0\0" "Detect all missing and new devices (sensors).\0"
"errors\0\0" "Show the error count for all sensors.\0"
"reseterrors\0\0" "Resets the error count for all sensors.\0"
"status\0\0" "Shows the current temperatures of all sensors and working modes of all controllers.\0"
"fullStatus\0\0" "Like status plus current mode of all actors.\0"
"set\0 CID T\0" "Sets the target temperature of controller CID to T C. T=0 activates profile mode.\0"
"setMinLevel\0 CID L\0" "Sets the minim level of controller CID (or 'all') to L [0..1].\0"
"resetControllerTemperatures\0\0" "Resets the target temperature of all controllers to 21 C.\0"
"resetControllerProfiles\0\0" "Resets all controller profiles to 0 (no profile).\0"
"setProfile\0 CID P\0" "Sets the profile for controller CID to profile nr. P (0 is the first profile). Use 'set CID 0' to activate profile mode.\0"
"get\0 CID\0" "Shows the current target temperature of controller CID.\0"
"getMinLevels\0\0" "Shows the minimum levels of all controllers.\0"
"getProfiles\0\0" "Shows the profiles settings.\0"
"setProfiles\0 XXX\0" "Sets the profiles (see documentation for format details).\0"
"getTemp\0 PID DOW H M\0" "Gets the temperature set in profile PID for Day-Of-Week (0-6) DOW, Time H:M.\0"
"forceActor\0 AID X\0" "forces the actor AID (or 'all') to bo on (X=1) or off (X=0) or to work in standard mode (X=-1).\0"
"forceController\0 CID X\0" "forces the controller CID (or 'all') to level X [0..1] or to work in standard mode (X=-1).\0"
"setTime\0 DD MM YY H M S\0" "Sets the time to day (DD), month (MM), year (YY); hour (H) minute (M) second (S).\0"
"getTime\0\0" "Shows the current time.\0"
"requestTemp\0\0" "Updates all sensors and sends their values via UDP (port 12888).\0"
#if defined( ACTORS_COMMTYPE_CRYSTAL64IO )
"resetI2C\0\0" "Reset the I2C bus.\0"
#endif
"testActors\0\0" "Test all actors by force them on/off.\0"
#ifdef SUPPORT_FTP
"setFtpServer\0 X.X.X.X\0" "Sets the ftp server address where the temperature protocol is stored.\0"
#endif
#ifdef SUPPORT_NTP
"setNtpServer\0 X.X.X.X\0" "Sets the time server address used for time synchronization.\0"
#endif
#ifdef SUPPORT_UDP_messages
"setUdpLoggingMask\0 mask\0" "Sets the bitmask to control the informations sent to UDP port 12888.\0"
#endif
"showProfiles\0\0" "Shows all profiles.\0"
"resetProfile\0 PID HS\0" "Resets the profile PID; profile mode is heating (HS=0) or switch (HS=1)\0"
"addProfile\0 HS\0" "Adds a new profile in mode heating (HS=0) or switch (HS=1)\0"
"addEntry\0 PID mdmdfss HH MM T\0" "Adds a new entry to profile PID.\0"
"setEntry\0 PID EID mdmdfss HH MM T\0" "Sets the values of entry EID of profile PID.\0"
"removeEntry\0 PID EID\0" "Removes the entry EID of profile PID.\0"
;

bool commandText( Stream& out, bool reset ){
  static const char* t = commandDescs;
  if ( reset ){
    t = commandDescs;
    return true;
  }
  size_t length = strlen_P( t );
  if ( !length ){
    ++t;
    return false;
  }
  g_buffer[ BUFFER_LENGTH ] = 0;
  size_t ofs = 0;
  while ( ofs < length ){
    strncpy_P( (char*)g_buffer, t + ofs, BUFFER_LENGTH );
    out << (const char*)g_buffer;
    ofs += BUFFER_LENGTH;
  }
  t += length + 1;
  return true;
}
