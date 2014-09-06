#include <DallasTemperature.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <SD.h>
#include <SPI.h>
#include <Streaming.h>
#include <Time.h>
#include <OneWire.h>

#include "Defines.h"
#include "Job.h"
#include "Network.h"
#include "Sensors.h"
#include "TemperatureProfiles.h"
#include "Tools.h"

// TODO / Fehler
// - Unterstützung DHT Hygrometer / Temperatursensor
// - Steuerung der Temperatur über die Woche (Profile)
//   - minimale Temperatur beschränken
//   - Vorausschauende Auswertung von Profilen, um Temperatur zu Zielzeitpunkt zu erreichen.
// - Hysterese des Zweipunktreglers
// - Regler mit mehreren Sensoren
// - PID-Regler
// - Minimale Bodentemperatur / Fusswarmer Boden: Minimales Level im Actor einbauen. Diesen im EEPROM ablegen.
// - SRAM sparen:
//   - strcmp -> strcmp_P
//   - Datenstruktur Sensors überarbeiten (-> PROGMEM)

float g_totalAmperage = 0.f; // current sum of all actors
float g_requiredAmperage = 0.f;
byte g_buffer[ 48 ];
boolean g_actorStateChanged = true;

extern PROGMEM const char* g_actorNames[];

class Actor
:public Job
{
public:
  typedef enum{ Standard, ForceOff, ForceOn } Mode;

  static Actor* findActor( const char* name );

  Actor()
  :_level( 0 )
  ,_open( false )
  ,_state( false )
  ,_mode( Standard )
  ,_currentAmperage( 0.f )
  ,_waitingForPower( false )
  {
  }

  bool isOpen() const{ return _state; }
  
  virtual void setup( int i, int amount ){
    Job::setup( i, amount );
    //pinMode( _ioPin, OUTPUT );
    //digitalWrite( _ioPin, LOW );
  }
  
  void setLevel( float level ){
    if ( level < 0.f ){
      level = 0.f;
    }
    if ( level > 1.f ){
      level = 1.f;
    }
    _level = level;
    _delayMillis = (unsigned long)( ACTOR_CYCLE_INTERVAL * ( _open ? _level : ( 1.f - _level ) ) * 1000.f );
  }
  
  virtual unsigned long doJob(){
    if ( _waitingForPower ){
      unsigned long delay = tryWrite( HIGH );
      if ( _waitingForPower ){
        return delay;
      }
      _open = !_open;
    }
    if ( _mode == Standard ){
      _open = !_open;
      unsigned long delay = (unsigned long)( ACTOR_CYCLE_INTERVAL * ( _open ? _level : ( 1.f - _level ) ) * 1000.f );
      if ( delay ){
        //digitalWrite( _ioPin, _open ? HIGH : LOW );
        delay = tryWrite( _open ? HIGH : LOW, delay );
      }
      return delay;
    } else if ( _mode == ForceOn ){
      //digitalWrite( _ioPin, HIGH );
      //return 1000;
      return tryWrite( HIGH, 1000 );
    }
    //digitalWrite( _ioPin, LOW );
    //return 1000;
    return tryWrite( LOW, 1000 );
  }
  
  void setMode( Mode mode ){
    _mode = mode;
    _delayMillis = 0;
  }
  
  unsigned long tryWrite( int level, unsigned long delay = 0 ){
    g_totalAmperage -= _currentAmperage;
    g_requiredAmperage -= _currentAmperage;
    if ( level == LOW ){
      // we always can go to LOW state...
      //digitalWrite( _ioPin, LOW );
      g_actorStateChanged |= _state;
      _state = false;
      _currentAmperage = 0.f;
      _waitingForPower = false;
      return delay;
    }
    if ( _currentAmperage == 0.f ){
      if ( g_totalAmperage + ACTOR_SWITCH_ON_SURGE_AMPERAGE > ACTORS_MAX_AMPERAGE ){
        //Serial << F("Actor '") << _name << F("' waits for enough power.") << endl;
        if ( !_waitingForPower ){
          g_requiredAmperage += ACTOR_SWITCH_ON_SURGE_AMPERAGE;
        }
        _waitingForPower = true;
        return 1000;
      }
      if ( _waitingForPower ){
        g_requiredAmperage -= ACTOR_SWITCH_ON_SURGE_AMPERAGE;
      }
      _waitingForPower = false;
      _currentAmperage = ACTOR_SWITCH_ON_SURGE_AMPERAGE;
      _switchOnTime = millis();
      //digitalWrite( _ioPin, HIGH );
      g_actorStateChanged |= !_state;
      _state = true;
      g_actorStateChanged = true;
    }
    g_totalAmperage += _currentAmperage;
    g_requiredAmperage += _currentAmperage;
    return delay;
  } 

  virtual void update(){
    if ( _currentAmperage == 0.f ){
      return;
    }
    g_totalAmperage -= _currentAmperage;
    g_requiredAmperage -= _currentAmperage;
    unsigned long elapsed = millis() - _switchOnTime;
    _currentAmperage = elapsed < ACTOR_SWITCH_ON_SURGE_DURATION ? ACTOR_SWITCH_ON_SURGE_AMPERAGE : ACTOR_WORKING_AMPERAGE;
    g_totalAmperage += _currentAmperage;
    g_requiredAmperage += _currentAmperage;
  }
  
  float _level;
  bool _open;
  bool _state;
  float _currentAmperage;
  unsigned long _switchOnTime;
  bool _waitingForPower;
private:
  Mode _mode;
};

extern Actor* g_actors[ ACTOR_COUNT + 1 ];

Actor* Actor::findActor( const char* name ){
  for( int i = 0; i < ACTOR_COUNT; ++i ){
    if ( !strcmp_P( name, (PGM_P)pgm_read_word(&g_actorNames[ i ] )) ){
      return g_actors[ i ];
    }
  }
  return 0;
}

extern PROGMEM const char* g_controllerNames[];

class Controller
:public Job
{
public:
  static Controller* find( const char* name );

  Controller( uint8_t id )
  :_id( id )
  ,_forcedLevel( -1 )
  {
  }
  
  uint8_t getID() const{ return _id; }
  
  const char* getName() const{
    strcpy_P( (char*)g_buffer, (char*)pgm_read_word(&(g_controllerNames[ _id ])) );
    return (char*)g_buffer;
  }
  
  virtual void setup( int i, int amount ){
    Job::setup( i, amount );
    _delayMillis = (unsigned long)( (float)CONTROLLER_UPDATE_INTERVAL / amount * i );
  }

  uint8_t getProfileID() const{
    return EEPROM.read( EEPROM_TEMP_BASE + _id * 3 );
  }
  
  void setProfileID( uint8_t id ){
    EEPROM.write( EEPROM_TEMP_BASE + _id * 3, id );
  }

  bool isProfileActive() const{ return getTargetT() == 0; }
  
  void setTargetT( float t ){
    DEBUG{ Serial << F("setTargetT to ") << t << endl; }
    if ( t == 0.f ){
      // intentionally no action: this case is used to work in profiled mode!
    } else if ( t < 5.f ){
      t = 5.f; // minimum temperature (anti-frost)
    } else if ( t > 25 ){
      t = 25.f; // maximum temperature (protect wooden floors)
    }
    int targetT = t * 10;
    DEBUG{ Serial << F("setTargetT to (int) ") << targetT << endl; }
    EEPROM.write( EEPROM_TEMP_BASE + _id * 3 + 1, targetT >> 8 );
    EEPROM.write( EEPROM_TEMP_BASE + _id * 3 + 2, targetT & 0xff );
  }
  
  float getTargetT() const{
    int targetT = EEPROM.read( EEPROM_TEMP_BASE + _id * 3 + 1 ) * 256 + EEPROM.read( EEPROM_TEMP_BASE + _id * 3 + 2 );
    return targetT / 10.f;
  }

  float getT() const{
    float targetT = getTargetT();
    if ( targetT == 0 ){
      TemperatureProfiles::setCurrentProfile( getProfileID() );
      time_t t = now();
      byte day = weekday( t ) - 2;
      if ( day == -1 ){
        day = 6;
      }
      return TemperatureProfiles::temp( day, hour( t ), minute( t ) );
    }
    return targetT;
  }
  
  virtual void writeSettings( Stream& s ){ s << getTargetT(); }
  virtual void readSettings( Stream& s ){ setTargetT( s.parseFloat() ); }
  
  virtual bool working() const{ return false; }
  
  void setForcedLevel( float level ){ _forcedLevel = level; _delayMillis = 0; }
  float getForcedLevel() const{ return _forcedLevel; }
  
protected:
  // level: 0=off; 1=on (100%)
  virtual void heat( float level ) =0;

  unsigned long specialFunctions( float t ){
    // forced level
    if ( _forcedLevel != -1 ){
      DEBUG{ Serial << "force level -------------------------------------------------------" << endl; }
      heat( _forcedLevel );
      _lastMillis = millis();
      return 1000ul * 10;
    }
    
    // frost protection
    if ( t < 5.f ){
      heat( 1.f );
      return 1000ul * 60 * 5;
    }
    
    return 0;
  }
  
  uint8_t _id;
  float _forcedLevel;
};

extern Controller* g_controller[ CONTROLLER_COUNT + 1 ];

Controller* Controller::find( const char* name ){
  for( int i = 0; i < CONTROLLER_COUNT; ++i ){
    if ( !strcmp_P( name, (PGM_P)pgm_read_word(&g_controllerNames[ i ] )) ){
      return g_controller[ i ];
    }
  }
  return 0;
}

class SingleSourceTwoLevelController
:public Controller
{
public:
  typedef enum{ Idle, WaitingForValue } SensorMode;
  
  SingleSourceTwoLevelController( uint8_t id, unsigned short sensor, unsigned short actor )
  :Controller( id )
  ,_sensor( sensor )
  ,_actor( actor )
  ,_heat( 0.f )
  ,_sensorMode( Idle )
  {
  }

  virtual bool working() const{ return g_sensors[ _sensor ].valid(); }

  virtual unsigned long doJob(){
#ifndef DEBUG_IGNORE_SENSORS
    if ( !g_sensors[ _sensor ].valid() ){
      //Serial << _name << F(": skipped due missing sensor '") << g_sensors[ _sensor ]._name << F("'.") << endl;
      return CONTROLLER_UPDATE_INTERVAL;
    }
    if ( _sensorMode == Idle ){
      g_sensors[ _sensor ].requestValue();
      _sensorMode = WaitingForValue;
    }
    if ( _sensorMode == WaitingForValue ){
      if ( !g_sensors[ _sensor ].isAvailable() ){
        return 100;
      }
      _sensorMode = Idle;
    }
    if ( !g_sensors[ _sensor ].update() ){
      Serial << getName() << F(": error in temperature update!") << endl;
      return CONTROLLER_UPDATE_INTERVAL;
    }

    float t = g_sensors[ _sensor ]._temp;
#else
    float t = 21.f;
#endif
    
    // execute special functions (like weekly water flow in heating system)
    unsigned long dt = specialFunctions( t );
    if ( dt > 0 ){
      return dt;
    }

    if ( t < getT() ){
      heat( 1.f );
    } else if ( t > getT() ){
      heat( 0.f );
    }
    return CONTROLLER_UPDATE_INTERVAL - 100;
  }

protected:
  virtual void heat( float level ){
    if ( level != _heat ){
      _heat = level;
      DEBUG2{ Serial << lz(day()) << '.' << lz(month()) << '.' << year() << ',' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second()) << F(" : ") << getName() << ( ( level > 0.f ) ? F(": start") : F(": stop") ) << F(" heating. (level ") << _FLOAT( level, 2 ) << F(")") << endl; }
      g_actors[ _actor ]->setLevel( level );
    }
  }
  
private:
  unsigned short _sensor;
  unsigned short _actor;
  float _heat;
  SensorMode _sensorMode;
};

extern EthernetClient ftpClient;
 
class TemperaturesUploader
:public Job
{
public:
  TemperaturesUploader()
    :_retryCnt( 5 )
    ,_lastSuccess( true )
  {}
  virtual unsigned long doJob(){
    if ( timeStatus() == timeNotSet ){
      BEGINMSG F("TermperaturesUploader IGNORED UPLOAD due to missing time.") ENDMSG
      return 60000ul;
    }
    DEBUG{ Serial << F("TemperaturesUploader START") << endl; }
    bool success = ftpOpen();
    if ( success ){
      StringStream dir;
      dir << F("Heizung");
      ftpSetMode( dir.c_str(), FTP_MAKEDIR, true );
      StringStream fname;
      //fname << year() << lz(month()) << lz(day()) << '/' << lz(hour()) << lz(minute()) << F(".txt");
      fname << F("Heizung/") << year() << lz(month()) << lz(day()) << F(".txt");
      bool fileExists = ftpFileExists( fname.c_str() );
      if ( ftpSetMode( fname.c_str(), FTP_APPEND, false ) ){
        //if ( fname.str() != _lastFName ){
        if ( !fileExists ){
          //_lastFName = fname.str();
          ftpClient << F("date,time,currentAmperage,requiredAmperage,freeRAM");
          for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
            ftpClient << ',' << g_sensors[ i ]._name;
          }
          ftpClient << endl;
        }
        ftpClient << lz(day()) << '.' << lz(month()) << '.' << year() << ',' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second());
        ftpClient << ',' << g_totalAmperage << ',' << g_requiredAmperage << ',' << FreeRam();
        for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
          ftpClient << ',';
          if ( g_sensors[ i ].valid() ){
            ftpClient << _FLOAT( g_sensors[ i ]._temp, 1 );
          }
        }
        ftpClient << endl;
      }
      ftpClose();
    }
    if ( _lastSuccess != success ){
      _lastSuccess = success;
      Serial << F("Temperatures upload ") << ( success ? F("succeded") : F("failed") ) << endl;
    }
    // upload the temperatures every 5 minutes; in case the ftp server coult not be reached retry in 30 minutes.
    if ( success ){
      _retryCnt = 5;
    }
    if ( _retryCnt ){
      --_retryCnt;
    }
    DEBUG{ Serial << F("TemperaturesUploader END") << endl; }
    return success || _retryCnt ? 5ul * 60ul * 1000ul : 30ul * 60ul * 1000ul;
    //return success || _retryCnt ? 15ul * 1000ul : 60ul * 1000ul;
  }
private:
  //String _lastFName;
  int _retryCnt;
  bool _lastSuccess;
};
  
void setupSensors(){
  Serial << F("  Initializing sensors.") << endl;
  for( int i = 0; i < BUS_COUNT; ++i ){
    g_temperatures[ i ].begin();
    g_temperatures[ i ].setWaitForConversion( false );
    if ( g_temperatures[ i ].isParasitePowerMode() ){
      Serial << F("    WARNING: Bus ") << i << F(" is in parasite power mode.") << endl;
    }
  }
  detectSensors( Serial );
  for( int i = 0; i < SENSOR_COUNT; ++i ){
    Sensor& sensor = g_sensors[ i ];
    if ( !sensor.valid() ){
      continue;
    }
    DallasTemperature& temp = g_temperatures[ sensor._bus ];
    temp.setResolution( sensor._addr, 12 );
    if ( temp.getResolution( sensor._addr ) != 12 ){
      Serial << F("    WARNING: Sensor '") << sensor._name << F("' could not be set to 12 bit resolution.") << endl;
    }
  }
  
  Job::setupJobs( (Job**)g_actors );
  Job::setupJobs( (Job**)g_controller );
}

void setup(){
  Serial.begin( 19200 );
  Serial << F( "Setup starting..." ) << endl;
  setupSensors();
  setupWebserver();  
  Serial << F( "  Initializing actors." ) << endl;
  pinMode( ACTORS_SER_PIN, OUTPUT );
  pinMode( ACTORS_RCLK_PIN, OUTPUT );
  pinMode( ACTORS_SRCLK_PIN, OUTPUT );
  Serial << F( "  Initializing status LED." ) << endl;
  pinMode( 13, OUTPUT );
  Serial << F( "  Downloading settings." ) << endl;
  execWebserver();
  //( new SettingsUploader() )->setup( 0, 1 );
  ( new TemperaturesUploader() )->setup( 0, 1 );
  Serial << F( "Setup finished." ) << endl;
}

const char* readText( Stream& s ){
  static char text[ 256 ];
  byte length = s.readBytesUntil( ' ', text, sizeof( text ) / sizeof( char ) );
  text[ length ] = 0;
  return text;
}

void read( Stream& s, char* buffer, unsigned short bufferSize ){
  unsigned short i = 0;
  while ( s.available() && i < bufferSize - 1 ){
    char c = s.read();
    if ( c == 10 || c == 13 || c == 32 ){
      if ( i == 0 ){
        continue;
      }
      break;
    }
    buffer[ i ] = c;
    ++i;
  }
  buffer[ i ] = 0;
  while ( s.available() ){
    char c = s.peek();
    if ( c != 10 || c != 13 || c != 32 ){
      break;
    }
  }
}

void writeTime( Stream& out ){
  out << F("Current time is ")  << lz(day()) << '.' << lz(month()) << '.' << year() << ' ' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second()) << endl;
}

void (*delayedCmd)() =0;

void requestTemp(){
  bool ok = false;
  for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
    Sensor& s = g_sensors[ i ];
    if ( !s.valid() ){
      continue;
    }
    s.update( true );
    ok = true;
  }
  if ( !ok ){
    Udp.beginPacket( 0xffffffff, 12888 );
    Udp << F("No working sensor found.");
    Udp.endPacket();
  }
}

void interpret( Stream& stream, Stream& out ){
  //const char* cmd = readText();
  char cmd[ 256 ];
  read( stream, cmd, sizeof( cmd ) );
  if ( !strcmp( cmd, "help" ) ){
    out << F("'detect': Detect all missing and new devices (sensors).") << endl;
    out << F("'errors': Show the error count for all sensors.") << endl;
    out << F("'reseterrors': Resets the error count for all sensors.") << endl;
    out << F("'status': Shows the current temperatures of all sensors.") << endl;
    out << F("'set CID T': Sets the target temperature of controller CID to T°C.") << endl;
    out << F("'resetControllerTemperatures': Resets the target temperature of all controllers to 21°C.") << endl;
    out << F("'resetControllerProfiles': Resets all controller profiles to 0 (no profile).") << endl;
    out << F("'setProfile CID P': Sets the profile for controller CID to profile nr. P (0 for no profile).") << endl; 
    out << F("'get CID': Shows the current target temperature of controller CID.") << endl;
    out << F("'getProfiles': Shows the profiles settings.") << endl;
    out << F("'setProfiles XXX': Sets the profiles (see documentation for format details).") << endl;
    out << F("'getTemp PID DOW H M': Gets the temperature set in profile PID for Day-Of-Week (0-6) DOW, Time H:M.") << endl;
    out << F("'forceActor AID X': forces the actor AID (or 'all') to bo on (X=1) or off (X=0) or to work in standard mode (X=-1).") << endl;
    out << F("'forceController CID X': forces the controller CID to set the actors' level to X [0..1]. Set X to -1 to work in standard operating mode.") << endl;
    out << F("'setTime DD MM YY H M S': Sets the time to day (DD), month (MM), year (YY); hour (H) minute (M) second (S).") << endl;
    out << F("'getTime': Shows the current time.") << endl;
    out << F("'requestTemp': Updates all sensors and sends their values via UDP (port 12888).") << endl;
    
  } else if ( !strcmp( cmd, "detect" ) || !strcmp( cmd, "d" ) ){
    out << F("Detection started...") << endl;
    detectSensors( out );
    out << F("Detection finished.") << endl;
    
  } else if ( !strcmp( cmd, "errors" ) ){
    bool errorsFound = false;
    for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
      const Sensor& s = g_sensors[ i ];
      if ( s._errorCount > 0 ){
        errorsFound = true;
        out << s._errorCount << F(" errors detected for sensor '") << s._name << F("'.") << endl;
      }
    }
    if ( !errorsFound ){
      out << F("No errors detected.") << endl;
    }
    
  } else if ( !strcmp( cmd, "reseterrors" ) ){
    for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
      g_sensors[ i ]._errorCount = 0;
    }
    out << F("Error counters resetted.") << endl;
    
  } else if ( !strcmp( cmd, "status" ) ){
    writeTime( out );
    out << F("Begin status") << endl;
    out << F("  SENSORS:") << endl;
    for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
      Sensor& s = g_sensors[ i ];
      if ( !s.valid() ){
        out << F("    '") << s._name << F("' invalid.") << endl;
        continue;
      }
      s.update( true );
      out << F("    '") << s._name << F("': ") << _FLOAT( s._temp, 1 ) << F(" C (") << s._errorCount << F(" errors)") << endl;
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
        out << _FLOAT( g_controller[ i ]->getT(), 1 ) << F(" C.") << endl;
      } else {
        out << F("forced to level ") << g_controller[ i ]->getForcedLevel() << '.' << endl;
      }
      
      /*if ( g_controller[ i ]->working() ){
        out << F("set to ");
        if ( g_controller[ i ]->isProfileActive() ){
         out << F("profile ") << g_controller[ i ]->getProfileID() << F(", currently ");
        }
        out << _FLOAT( g_controller[ i ]->getT(), 1 ) << F(" C.") << endl;
      } else {
        out << F("out of function. (set to ") << _FLOAT( g_controller[ i ]->getTargetT(), 1 ) << F(" C.) with profile ") << g_controller[ i ]->getProfileID() << endl;
      }*/
    }
    out << F("  TEMPERATURE PROFILES:") << endl;
    out << F("    ");
    //g_tempProfile.writeSettings( out );
    TemperatureProfiles::writeSettings( out );
    out << F("  CURRENT AMPERAGE: ") << g_totalAmperage << endl;
    out << F("  FREE RAM: ") << FreeRam() << F(" bytes") << endl;
    out << F("End status") << endl;
    
  } else if ( !strcmp( cmd, "set" ) ){
    const char* name = readText( stream );
    Controller* c = Controller::find( name );
    if ( c ){
      float oldT = c->getTargetT();
      c->readSettings( stream );
      out << F("Changed controller '") << name << F("' from ") << _FLOAT( oldT, 1 ) << F(" C to ") << _FLOAT( c->getTargetT(), 1 ) << F(" C target temperature. (0 means profile mode)") << endl;
    } else {
      out << F("Unable to find controller '") << name << F("'.") << endl;
    }
    
  } else if ( !strcmp( cmd, "resetControllerTemperatures" ) ){
    for( int i = 0; i < CONTROLLER_COUNT; ++i ){
      g_controller[ i ]->setTargetT( 21. );
    }
    out << F("All controller target temperatures resetted to 21C.") << endl;
    
  } else if ( !strcmp( cmd, "resetControllerProfiles" ) ){
    for( int i = 0; i < CONTROLLER_COUNT; ++i ){
      g_controller[ i ]->setProfileID( 0 );
    }
    out << F("All controller temperature profiles resetted to profile 0.") << endl;
    
  } else if ( !strcmp( cmd, "setProfile" ) ){
    const char* name = readText( stream );
    Controller* c = Controller::find( name );
    if ( c ){
      int i = stream.parseInt();
      c->setProfileID( i );
      out << F("Profile for controller '") << name << F("' set to profile ") << i << endl;
    }
    
  } else if ( !strcmp( cmd, "get" ) ){
    const char* name = readText( stream );
    Controller* c = Controller::find( name );
    if ( c ){
      out << F("Target temperature for controller '") << name << F("' is set to ") << _FLOAT( c->getTargetT(), 1 ) << F(" C.") << endl;
    } else {
      out << F("Unable to find controller '") << name << F("'.") << endl;
    }
    
  } else if ( !strcmp( cmd, "getProfiles" ) ){
    TemperatureProfiles::writeSettings( out );
    
  } else if ( !strcmp( cmd, "setProfiles" ) ){
    TemperatureProfiles::readSettings( stream );
    
  } else if ( !strcmp( cmd, "getTemp" ) ){
    int id = stream.parseInt();
    int dow = stream.parseInt();
    int h = stream.parseInt();
    int m = stream.parseInt();
    TemperatureProfiles::setCurrentProfile( id );
    out << _FLOAT( TemperatureProfiles::temp( dow, h, m ), 1 ) << endl;
    
  //} else if ( !strcmp( cmd, "files" ) ){
  //  out << F("Files on SD card") << endl;
    
  } else if ( !strcmp( cmd, "forceActor" ) ){
    const char* name = readText( stream );
    out << F( "Actor " ) << name;
    int m;
    if ( !strcmp( name, "all" ) ){
      m = stream.parseInt() + 1;
      for( int i = 0; i < ACTOR_COUNT; ++i ){
        g_actors[ i ]->setMode( (Actor::Mode)m );
      }
    } else {
      Actor* a = Actor::findActor( name );
      if ( !a ){
        out << F( " not found." ) << endl;
        return;
      }
      m = stream.parseInt() + 1;
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
    
  } else if ( !strcmp( cmd, "forceController" ) ){
    const char* name = readText( stream );
    out << F( "Controller " ) << name;
    Controller* c = Controller::find( name );
    if ( !c ){
      out << F( " not found." ) << endl;
      return;
    }
    float level = stream.parseFloat();
    c->setForcedLevel( level );
    if ( level == -1 ){
      out << F( " set to standard operating mode." ) << endl;
    } else {
      out << F(" forced to level " ) << level << '.' << endl;
    }
    
  } else if ( !strcmp( cmd, "setTime" ) ){
    int d = stream.parseInt();
    int mo = stream.parseInt();
    int y = stream.parseInt();
    int h = stream.parseInt();
    int m = stream.parseInt();
    int s = stream.parseInt();
    setTime( h, m, s, d, mo, y);
    out << F("Time set to ")  << lz(day()) << '.' << lz(month()) << '.' << year() << ' ' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second()) << endl;
    
  } else if ( !strcmp( cmd, "getTime" ) ){
    writeTime( out );

  } else if ( !strcmp( cmd, "requestTemp" ) ){
    out << F("Request received...") << endl;
    delayedCmd = &requestTemp;
    
  } else {
    out << F("Unknwon command '") << cmd << F("'.") << endl;
  }
}

void loop(){
  digitalWrite( 13, HIGH );

  DEBUG{ Serial << F("FreeRam: ") << FreeRam() << endl; }

  DEBUG{ Serial << F("execWebserver START") << endl; }
  execWebserver();
  DEBUG{ Serial << F("execWebserver END") << endl; }
  
  // listen to serial port for commands
  if ( Serial.available() ){
    DEBUG{ Serial << F("interpret START") << endl; }
    interpret( Serial, Serial );
    DEBUG{ Serial << F("interpret END") << endl; }
  }

  if ( delayedCmd ){
    delayedCmd();
    delayedCmd = 0;
  }
  
  // process all jobs (sensors/controllers/actors)
  unsigned long minDelay = 50;

  unsigned long start = millis();
  DEBUG{ Serial << F("exec jobs START") << endl; }
  Job* job = Job::g_headJob;
  while ( job ){
    //minDelay = min( minDelay, job->exec() );
    unsigned long d = job->exec();
    if ( d < minDelay ){
      minDelay = d;
    }
    job = job->nextJob();
  }
  if ( g_actorStateChanged ){
    DEBUG{ Serial << F("Actors state: "); }
    digitalWrite( ACTORS_RCLK_PIN, LOW );
    for( int i = ACTOR_COUNT - 1; i >=  0; i-- ){
      digitalWrite( ACTORS_SRCLK_PIN, LOW );
      DEBUG{ Serial << ( g_actors[ i ]->isOpen() ? F("1") : F("0") ); }
#ifdef INVERT_RELAIS_LEVEL
      digitalWrite( ACTORS_SER_PIN, g_actors[ i ]->isOpen() ? LOW : HIGH );
#else
      digitalWrite( ACTORS_SER_PIN, g_actors[ i ]->isOpen() ? HIGH : LOW );
#endif
      digitalWrite( ACTORS_SRCLK_PIN, HIGH );
    }
    DEBUG{ Serial << endl; }
    digitalWrite( ACTORS_RCLK_PIN, HIGH );
  }
  g_actorStateChanged = false;
  DEBUG{ Serial << F("exec jobs END (") << ( millis() - start ) << F(" ms)") << endl; }

  digitalWrite( 13, LOW );
  DEBUG{ Serial << F("minDelay=") << minDelay << endl; }
  delay( minDelay );
}

// elements = number of digital IO pin the 1-wire-bus is connected to
OneWire g_busses[ BUS_COUNT ] = {
  46, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45
};

// proxy classes to read temperatures from the busses. One entry for each bus.
DallasTemperature g_temperatures[ BUS_COUNT ] = {
  &g_busses[ 0 ],
  &g_busses[ 1 ],
  &g_busses[ 2 ],
  &g_busses[ 3 ],
  &g_busses[ 4 ],
  &g_busses[ 5 ],
  &g_busses[ 6 ],
  &g_busses[ 7 ],
  &g_busses[ 8 ],
  &g_busses[ 9 ],
  &g_busses[ 10 ],
  &g_busses[ 11 ],
  &g_busses[ 12 ],
  &g_busses[ 13 ],
  &g_busses[ 14 ],
  &g_busses[ 15 ],
  &g_busses[ 16 ],
  &g_busses[ 17 ],
  &g_busses[ 18 ],
  &g_busses[ 19 ],
  &g_busses[ 20 ],
  &g_busses[ 21 ],
  &g_busses[ 22 ],
  &g_busses[ 23 ]
};

// elements = name, bus, address of sensor
// "name" is a unique human readable string to identify the temperature sensor
// "bus" is the index of the bus in g_busses the sensor is connected to
// "address" is the DS18B20 internal address (use "detect" command to get this)
Sensor g_sensors[ SENSOR_COUNT ] = {
  { "S_UG_Waschen",        0, { 0x28, 0xE2, 0x12, 0xE2, 0x04, 0x00, 0x00, 0x76 } }, // 
  //{ "S_UG_Waschen",        0 }, // DHT temperature/humidity sensor (no address)
  { "S_UG_Gast",           0, { 0x28, 0xFB, 0x2E, 0xE1, 0x04, 0x00, 0x00, 0x96 } }, // 
  { "S_UG_Hobby",          0, { 0x28, 0x7E, 0x14, 0xE1, 0x04, 0x00, 0x00, 0xAE } }, // 
  { "S_UG_Treppe",        20, { 0x28, 0xEE, 0x32, 0xE1, 0x04, 0x00, 0x00, 0x7B } }, // 
  { "S_UG_Flur",           8, { 0x28, 0x8F, 0x28, 0xE1, 0x04, 0x00, 0x00, 0x4E } }, // 
  { "S_UG_Vorrat",         0, { 0x28, 0x06, 0x58, 0xCE, 0x04, 0x00, 0x00, 0xB5 } }, // 
  { "S_UG_Bad",            0, { 0x28, 0x57, 0xDE, 0xE0, 0x04, 0x00, 0x00, 0x9E } }, // 
  { "S_UG_Keller",        14, { 0x28, 0x07, 0x25, 0xE1, 0x04, 0x00, 0x00, 0xE9 } }, // 
  { "S_EG_Treppe",         0, { 0x28, 0xFA, 0x77, 0xE0, 0x04, 0x00, 0x00, 0x48 } }, // 
  { "S_EG_Garderobe",     16, { 0x28, 0x46, 0x71, 0xE0, 0x04, 0x00, 0x00, 0xAE } }, // 
  { "S_EG_Flur",           0, { 0x28, 0xDF, 0x43, 0xE1, 0x04, 0x00, 0x00, 0x01 } }, // 
  { "S_EG_Bad",            0, { 0x28, 0xF0, 0xAD, 0xE2, 0x04, 0x00, 0x00, 0x63 } }, // 
  { "S_EG_Zimmer",         0, { 0x28, 0xEC, 0x86, 0xE2, 0x04, 0x00, 0x00, 0xCD } }, // 
  { "S_EG_Wohnen_Flur",   10, { 0x28, 0xA2, 0x6B, 0xE3, 0x04, 0x00, 0x00, 0x12 } }, // 
  { "S_EG_Wohnen_Kueche", 22, { 0x28, 0xA4, 0x6B, 0xE3, 0x04, 0x00, 0x00, 0xA0 } }, // 
  { "S_EG_Kueche",         4, { 0x28, 0x39, 0x23, 0xE3, 0x04, 0x00, 0x00, 0x8C } }, // 
  { "S_OG_Kind2",         12, { 0x28, 0xC8, 0x6D, 0xE2, 0x04, 0x00, 0x00, 0x0D } }, // 
  { "S_OG_Kind3",          0, { 0x28, 0xA0, 0x32, 0xE1, 0x04, 0x00, 0x00, 0x1D } }, // 
  { "S_OG_Bad",            2, { 0x28, 0x8E, 0x53, 0xE1, 0x04, 0x00, 0x00, 0x64 } }, // 
  { "S_OG_WC",             6, { 0x28, 0x45, 0x36, 0xE1, 0x04, 0x00, 0x00, 0xC0 } }, // 
  { "S_OG_Arbeit",         0, { 0x28, 0x57, 0x45, 0xE1, 0x04, 0x00, 0x00, 0xD6 } }, // 
  { "S_OG_Eltern",        18, { 0x28, 0x4C, 0x00, 0x9D, 0x04, 0x00, 0x00, 0xF4 } }, // 
  { "S_OG_Treppe",         0, { 0x28, 0x51, 0x3B, 0xE1, 0x04, 0x00, 0x00, 0xAB } }, // 
  { "S_OG_Flur",           0, { 0x28, 0x70, 0x6D, 0xE0, 0x04, 0x00, 0x00, 0xAC } }, // 
  { "S_DG",                0, { 0x28, 0x98, 0x76, 0xE0, 0x04, 0x00, 0x00, 0x28 } }, //
};

Actor* g_actors[ ACTOR_COUNT + 1 ] = {
  //new Actor( 22, "a1", ACTOR_CYCLE_INTERVAL, 0.2f, 60000, 0.075f ),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  new Actor(),
  0 // define 0 as last entry!
};

char g_an0[] PROGMEM = "a0";
char g_an1[] PROGMEM = "a1";
char g_an2[] PROGMEM = "a2";
char g_an3[] PROGMEM = "a3";
char g_an4[] PROGMEM = "a4";
char g_an5[] PROGMEM = "a5";
char g_an6[] PROGMEM = "a6";
char g_an7[] PROGMEM = "a7";
char g_an8[] PROGMEM = "b0";
char g_an9[] PROGMEM = "b1";
char g_an10[] PROGMEM = "b2";
char g_an11[] PROGMEM = "b3";
char g_an12[] PROGMEM = "b4";
char g_an13[] PROGMEM = "b5";
char g_an14[] PROGMEM = "b6";
char g_an15[] PROGMEM = "b7";
char g_an16[] PROGMEM = "c0";
char g_an17[] PROGMEM = "c1";
char g_an18[] PROGMEM = "c2";
char g_an19[] PROGMEM = "c3";
char g_an20[] PROGMEM = "c4";
char g_an21[] PROGMEM = "c5";
char g_an22[] PROGMEM = "c6";
char g_an23[] PROGMEM = "c7";

PGM_P g_actorNames[ ACTOR_COUNT ] PROGMEM = {
  g_an0,
  g_an1,
  g_an2,
  g_an3,
  g_an4,
  g_an5,
  g_an6,
  g_an7,
  g_an8,
  g_an9,
  g_an10,
  g_an11,
  g_an12,
  g_an13,
  g_an14,
  g_an15,
  g_an16,
  g_an17,
  g_an18,
  g_an19,
  g_an20,
  g_an21,
  g_an22,
  g_an23
};

// elements = new XXXController( ... )
// Each controller uses one or more temperature sensors and one actor to reach the target temperature of the controller.
// Implemented controllers:
//   SingleSourceTwoLevelController( name, sensor, actor ): This Controller is a very simple two-point-controller without
//   histersis function. It simply turns the actor on if the temperature is below the target temperature and turns the actor
//   off otherwise.
//     "sensor" is the index in g_sensors of the sensor used as current temperature source
//     "actor" is the index in g_actors of the actor to be controlled by this controller.
Controller* g_controller[ CONTROLLER_COUNT + 1 ] = {
  new SingleSourceTwoLevelController( 0, 0, 0 ),
  new SingleSourceTwoLevelController( 1, 1, 1 ),
  new SingleSourceTwoLevelController( 2, 2, 2 ),
  new SingleSourceTwoLevelController( 3, 3, 3 ),
  new SingleSourceTwoLevelController( 4, 4, 4 ),
  new SingleSourceTwoLevelController( 5, 5, 5 ),
  new SingleSourceTwoLevelController( 6, 6, 6 ),
  new SingleSourceTwoLevelController( 7, 7, 7 ),
  new SingleSourceTwoLevelController( 8, 8, 8 ),
  new SingleSourceTwoLevelController( 9, 9, 9 ),
  new SingleSourceTwoLevelController( 10, 10, 10 ),
  new SingleSourceTwoLevelController( 11, 11, 11 ),
  new SingleSourceTwoLevelController( 12, 12, 12 ),
  new SingleSourceTwoLevelController( 13, 13, 13 ),
  new SingleSourceTwoLevelController( 14, 14, 14 ),
  new SingleSourceTwoLevelController( 15, 15, 15 ),
  new SingleSourceTwoLevelController( 16, 16, 16 ),
  new SingleSourceTwoLevelController( 17, 17, 17 ),
  new SingleSourceTwoLevelController( 18, 18, 18 ),
  new SingleSourceTwoLevelController( 19, 19, 19 ),
  new SingleSourceTwoLevelController( 20, 20, 20 ),
  new SingleSourceTwoLevelController( 21, 21, 21 ),
  new SingleSourceTwoLevelController( 22, 22, 22 ),
  new SingleSourceTwoLevelController( 23, 23, 23 ),
  0 // define 0 as last entry!
};

char g_cnC0[] PROGMEM = "C0";
char g_cnC1[] PROGMEM = "C1";
char g_cnC2[] PROGMEM = "C2";
char g_cnC3[] PROGMEM = "C3";
char g_cnC4[] PROGMEM = "C4";
char g_cnC5[] PROGMEM = "C5";
char g_cnC6[] PROGMEM = "C6";
char g_cnC7[] PROGMEM = "C7";
char g_cnC8[] PROGMEM = "C8";
char g_cnC9[] PROGMEM = "C9";
char g_cnC10[] PROGMEM = "C10";
char g_cnC11[] PROGMEM = "C11";
char g_cnC12[] PROGMEM = "C12";
char g_cnC13[] PROGMEM = "C13";
char g_cnC14[] PROGMEM = "C14";
char g_cnC15[] PROGMEM = "C15";
char g_cnC16[] PROGMEM = "C16";
char g_cnC17[] PROGMEM = "C17";
char g_cnC18[] PROGMEM = "C18";
char g_cnC19[] PROGMEM = "C19";
char g_cnC20[] PROGMEM = "C20";
char g_cnC21[] PROGMEM = "C21";
char g_cnC22[] PROGMEM = "C22";
char g_cnC23[] PROGMEM = "C23";

PGM_P g_controllerNames[ CONTROLLER_COUNT ] PROGMEM = {
  g_cnC0,
  g_cnC1,
  g_cnC2,
  g_cnC3,
  g_cnC4,
  g_cnC5,
  g_cnC6,
  g_cnC7,
  g_cnC8,
  g_cnC9,
  g_cnC10,
  g_cnC11,
  g_cnC12,
  g_cnC13,
  g_cnC14,
  g_cnC15,
  g_cnC16,
  g_cnC17,
  g_cnC18,
  g_cnC19,
  g_cnC20,
  g_cnC21,
  g_cnC22,
  g_cnC23
};

