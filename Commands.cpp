#include <SD.h>
#include <Time.h>

#include "Actor.h"
#include "Commands.h"
#include "Controller.h"
#include "Sensors.h"
#include "TemperatureProfiles.h"
#include "Tools.h"

extern void (*delayedCmd)();

void cmdHelp( Stream& in, Stream& out ){
  out << F("Supported Commands:") << endl;
  commandText( out, true );
  while( commandText( out ) ){
    commandText( out );
    out << ": ";
    commandText( out );
    out << endl;
  }
}

void cmdDetect( Stream& in, Stream& out ){
  out << F("Detection started...") << endl;
  detectSensors( out );
  out << F("Detection finished.") << endl;
}

void cmdErrors( Stream& in, Stream& out ){
  writeSensorsErrors( out );
}

void cmdResetErrors( Stream& in, Stream& out ){
  for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
    getSensor( i )._errorCount = 0;
  }
  out << F("Error counters resetted.") << endl;
}

void cmdStatus( Stream& in, Stream& out ){
  writeTime( out );
  out << F( "Running since " );
  writeTime( out, g_startTime );
  out << endl << F("Begin status") << endl;
  out << F("  SENSORS:") << endl;
  for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
    Sensor& s = getSensor( i );
    if ( !s.valid() ){
      out << F("    '") << s._name << F("' invalid.") << endl;
      continue;
    }
    // requesting an update leads to a long time until the result is presented...
    //s.update( true );
    int age = now() - s._lastChange;
    out << F("    '") << s._name << F("': ") << _FLOAT( s._temp, 1 ) << F(" C (") << s._errorCount << F(" errors, age: ") << age << F(" s)") << endl;
    s.sendStatus();
  }
  out << F("  CONTROLLERS:") << endl;
  for( unsigned short i = 0; i < CONTROLLER_COUNT; ++i ){
    out << F("    '") << g_controller[ i ]->getName() << F("' ");
    if ( !g_controller[ i ]->working() ){
      out << F("[OUT OF FUNCTION] ");
    }
    if ( g_controller[ i ]->getForcedLevel() == -1 ){
      out << F("set to ");
      if ( g_controller[ i ]->isProfileActive() ){
        out << F("profile ") << g_controller[ i ]->getProfileID() << F(", currently ");
      }
      out << _FLOAT( g_controller[ i ]->getT(), 1 ) << F(" C. ");
    } else {
      out << F("forced to level ") << g_controller[ i ]->getForcedLevel() << ". ";
    }
    g_controller[ i ]->printStatus( out );
    out << endl;
  }
  out << F( "  SWITCHES:" ) << endl;
  for ( unsigned short i = HEATING_ACTOR_COUNT; i < ACTOR_COUNT; ++i ){
    out << F( "    '" ) << g_actors[ i ]->getName() << F( "' " );
    if ( g_actors[ i ]->getMode() != Actor::Standard ){
      out << F( "FORCED " );
    }
    out << ( g_actors[ i ]->isOpen() ? F( "ON" ) : F( "OFF" ) ) << endl;
  }
  out << F("  TEMPERATURE PROFILES:") << endl;
  out << F("    ");
  //g_tempProfile.writeSettings( out );
  TemperatureProfiles::writeSettings( out );
  out << F("  CURRENT AMPERAGE: ") << Actor::getCurrentUsedAmperage() << endl;
  out << F("  FREE RAM: ") << FreeRam() << F(" bytes") << endl;
  out << F("End status") << endl;
}

void cmdSet( Stream& in, Stream& out ){
  const char* name = readText( in );
  Controller* c = Controller::find( name );
  if ( c ){
    float oldT = c->getTargetT();
    c->readSettings( in );
    out << F("Changed controller '") << name << F("' from ") << _FLOAT( oldT, 1 ) << F(" C to ") << _FLOAT( c->getTargetT(), 1 ) << F(" C target temperature. (0 means profile mode)") << endl;
  } else {
    out << F("Unable to find controller '") << name << F("'.") << endl;
  }
}

void cmdSetMinLevel( Stream& in, Stream& out ){
  const char* name = readText( in );
  float l = in.parseFloat();
  if ( !strcmp( name, "all" ) ){
    for( int i = 0; i < CONTROLLER_COUNT; ++i ){
      g_controller[ i ]->setMinimumLevel( l );
    }
    out << F("Changed all controller minimum levels to ") << _FLOAT( l, 2 ) << '.' << endl;
  } else {
    Controller* c = Controller::find( name );
    if ( c ){
      float oldL = c->getMinimumLevel();
      c->setMinimumLevel( l );
      out << F("Changed controller '") << name << F("' minimum level from ") << _FLOAT( oldL, 1 ) << F(" to ") << _FLOAT( l, 2 ) << '.' << endl;
    } else {
      out << F("Unable to find controller '") << name << F("'.") << endl;
    }
  }
}

void cmdResetControllerTemperatures( Stream& in, Stream& out ){
  for( int i = 0; i < CONTROLLER_COUNT; ++i ){
    g_controller[ i ]->setTargetT( 21. );
    g_controller[ i ]->setMinimumLevel( 0. );
  }
  out << F("All controller target temperatures and minimum levels resetted to 21 C / 0%.") << endl;
}

void cmdResetControllerProfiles( Stream& in, Stream& out ){
  for( int i = 0; i < CONTROLLER_COUNT; ++i ){
    g_controller[ i ]->setProfileID( 0 );
  }
  out << F("All controller temperature profiles resetted to profile 0.") << endl;
}

void cmdSetProfile( Stream& in, Stream& out ){
  const char* name = readText( in );
  Controller* c = Controller::find( name );
  if ( c ){
    int i = in.parseInt();
    c->setProfileID( i );
    out << F("Profile for controller '") << name << F("' set to profile ") << i << endl;
  }
}

void cmdGet( Stream& in, Stream& out ){
  const char* name = readText( in );
  Controller* c = Controller::find( name );
  if ( c ){
    out << F("Target temperature for controller '") << name << F("' is set to ") << _FLOAT( c->getTargetT(), 1 ) << F(" C.") << endl;
  } else {
    out << F("Unable to find controller '") << name << F("'.") << endl;
  }
}

void cmdGetMinLevels( Stream& in, Stream& out ){
  out << F("Controller minimum levels") << endl;
  for( unsigned short i = 0; i < CONTROLLER_COUNT; ++i ){
    out << F("  '") << g_controller[ i ]->getName() << F("': ") << _FLOAT( g_controller[ i ]->getMinimumLevel(), 2 ) << endl;
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
  out << _FLOAT( TemperatureProfiles::temp( dow, h, m ), 1 ) << endl;
}

void cmdForceActor( Stream& in, Stream& out ){
  const char* name = readText( in );
  out << F( "Actor " ) << name;
  int m;
  if ( !strcmp( name, "all" ) ){
    m = in.parseInt() + 1;
    for( int i = 0; i < ACTOR_COUNT; ++i ){
      g_actors[ i ]->setMode( (Actor::Mode)m );
    }
  } else {
    Actor* a = Actor::findActor( name );
    if ( !a ){
      out << F( " not found." ) << endl;
      return;
    }
    m = in.parseInt() + 1;
    a->setMode( (Actor::Mode)m );
  }
  switch ( m ){
  case 0:
    out << F( " set to standard operating mode." ) << endl;
    break;
  case 1:
    out << F( " forced to be OFF." ) << endl;
    break;
  case 2:
    out << F(" forced to be ON." ) << endl;
    break;
  }
}

void cmdForceController( Stream& in, Stream& out ){
  const char* name = readText( in );
  float level = in.parseFloat();
  if ( !strcmp( name, "all" ) ){
    for( unsigned short i = 0; i < CONTROLLER_COUNT; ++i ){
      g_controller[ i ]->setForcedLevel( level );
    }
    out << F("All controllers");
  } else {
    out << F( "Controller " ) << name;
    Controller* c = Controller::find( name );
    if ( !c ){
      out << F( " not found." ) << endl;
      return;
    }
    c->setForcedLevel( level );
  }
  if ( level == -1 ){
    out << F( " set to standard operating mode." ) << endl;
  } else {
    out << F(" forced to level " ) << level << '.' << endl;
  }
}
    
void cmdSetTime( Stream& in, Stream& out ){
  int d = in.parseInt();
  int mo = in.parseInt();
  int y = in.parseInt();
  int h = in.parseInt();
  int m = in.parseInt();
  int s = in.parseInt();
  setTime( h, m, s, d, mo, y);
  setStartTime();
  out << F("Time set to ")  << lz(day()) << '.' << lz(month()) << '.' << year() << ' ' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second()) << endl;
}

void cmdGetTime( Stream& in, Stream& out ){
  writeTime( out );
}

void cmdRequestTemp( Stream& in, Stream& out ){
  out << F("Request received...") << endl;
  delayedCmd = &requestTemp;
}

void cmdTestActors( Stream& in, Stream& out ){
  testActors();
}

#if defined( ACTORS_COMMTYPE_CRYSTAL64IO )
void cmdResetI2C( Stream& in, Stream& out ){
  out << F("Trying to re-initialize i2c bus...") << endl;
  Actor::setupI2C();
}
#endif

Command commands[] = {
  cmdHelp,
  cmdDetect,
  cmdErrors,
  cmdResetErrors,
  cmdStatus,
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
  cmdTestActors
};

const char commandDescs[] PROGMEM =
  "help\0\0" "Shows this help info.\0"
  "detect\0\0" "Detect all missing and new devices (sensors).\0"
  "errors\0\0" "Show the error count for all sensors.\0"
  "reseterrors\0\0" "Resets the error count for all sensors.\0"
  "status\0\0" "Shows the current temperatures of all sensors.\0"
  "set\0 CID T\0" "Sets the target temperature of controller CID to T C.\0"
  "setMinLevel\0 CID L\0" "Sets the minim level of controller CID (or 'all') to L [0..1].\0"
  "resetControllerTemperatures\0\0" "Resets the target temperature of all controllers to 21 C.\0"
  "resetControllerProfiles\0\0" "Resets all controller profiles to 0 (no profile).\0"
  "setProfile\0 CID P\0" "Sets the profile for controller CID to profile nr. P (0 for no profile).\0"
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
