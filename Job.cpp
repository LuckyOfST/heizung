#include <arduino.h>
#include <Streaming.h>

#include "Defines.h"
#include "Job.h"

Job* Job::g_headJob = 0;

void Job::setupJobs( Job** job ){
  int amount = 0;
  Job** j = job;
  while ( *j ){
    ++amount;
    ++j;
  }
  int i = 0;
  while ( *job ){
    (*job)->setup( i, amount );
    ++i;
    ++job;
  }
}

unsigned long Job::executeJobs( unsigned long minDelay ){
  unsigned long start = millis();
  DEBUG{ Serial << F("exec jobs START") << endl; }
  Job* job = g_headJob;
  while ( job ){
    //minDelay = min( minDelay, job->exec() );
    unsigned long d = job->exec();
    if ( d < minDelay ){
      minDelay = d;
    }
    job = job->nextJob();
  }
  DEBUG{ Serial << F("exec jobs END (") << ( millis() - start ) << F(" ms)") << endl; }
  return minDelay;
}

Job::Job()
:_lastMillis( 0ul )
,_delayMillis( 0ul )
,_previousJob( 0 )
,_nextJob( 0 )
{
}

Job::~Job(){
  if ( _previousJob ){
    _previousJob->_nextJob = _nextJob;
  } else {  
    g_headJob = _nextJob;
  }
  if ( _nextJob ){
    _nextJob->_previousJob = _previousJob;
  }
}

void Job::setup( int i, int amount ){
  _lastMillis = millis();
  _nextJob = g_headJob;
  g_headJob = this;
}
 
unsigned long Job::exec(){
  update();
  unsigned long elapsed = millis() - _lastMillis;
  if ( elapsed >= _delayMillis ){
    _lastMillis += _delayMillis;
    _delayMillis = doJob();
    elapsed = 0;
  }
  return _delayMillis - elapsed;
}

void Job::wait(){
  unsigned long now = millis();
  unsigned long delta = now - _lastMillis;
  if ( delta < _delayMillis ){
    delay( _delayMillis - delta );
  }
}
  
void Job::update(){
}

