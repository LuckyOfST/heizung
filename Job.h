#ifndef Job_h
#define Job_h

class Job{
public:
  static void setupJobs( Job** job );
  static unsigned long executeJobs( unsigned long minDelay );

  Job();
  virtual ~Job();
  
  virtual void setup( int i, int amount );
  
protected:

  virtual void update();
  virtual unsigned long doJob() =0;
  
  unsigned long _lastMillis;
  unsigned long _delayMillis;

private:
  static Job* g_headJob;

  Job* nextJob() const{ return _nextJob; }
  unsigned long exec();
  void wait();

  Job* _previousJob;
  Job* _nextJob;
};

#endif // Job_h

