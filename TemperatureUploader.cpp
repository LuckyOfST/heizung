#include <SD.h> // currently only for FreeRam() used - maybe this dependency could be reduced...
#include <Time.h>

#include "Actor.h"
#include "FTPClient.h"
#include "Network.h"
#include "Sensors.h"
#include "TemperatureUploader.h"
#include "Tools.h"

#ifdef SUPPORT_TemperatureUploader

TemperaturesUploader::TemperaturesUploader()
  :_retryCnt( 5 )
  ,_lastSuccess( true )
{
}

unsigned long TemperaturesUploader::doJob(){
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
          ftpClient << ',' << getSensor( i )._name;
        }
        ftpClient << endl;
      }
      ftpClient << lz(day()) << '.' << lz(month()) << '.' << year() << ',' << lz(hour()) << ':' << lz(minute()) << ':' << lz(second());
      ftpClient << ',' << Actor::getCurrentUsedAmperage() << ',' << Actor::getCurrentRequiredAmperage() << ',' << FreeRam();
      for( unsigned short i = 0; i < SENSOR_COUNT; ++i ){
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

#endif // SUPPORT_TemperatureUploader

