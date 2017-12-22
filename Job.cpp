#include <arduino.h>
#include <Streaming.h>

#include "Defines.h"
#include "Job.h"
#include "Tools.h"

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
#ifdef SEND_JOB_EXECUTIONS
  StringStream jobExecutions;
  jobExecutions << F( "jobs:" );
#endif
  Job* job = g_headJob;
  while ( job ){
    //minDelay = min( minDelay, job->exec() );
    bool executed = false;
    unsigned long d = job->exec( executed );
    if ( d < minDelay ){
      minDelay = d;
    }
#ifdef SEND_JOB_EXECUTIONS
    if ( executed ) {
      jobExecutions << ' ' << job->title() << '(' << d << ')';
    }
#endif
    job = job->nextJob();
  }
#ifdef SEND_JOB_EXECUTIONS
  BEGINMSG jobExecutions.str() ENDMSG
#endif
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
 
unsigned long Job::exec( bool& executed ){
  update();
  unsigned long elapsed = millis() - _lastMillis;
  if ( elapsed >= _delayMillis ){
    _lastMillis += _delayMillis;
    _delayMillis = doJob();
    elapsed = 0;
    executed = true;
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

