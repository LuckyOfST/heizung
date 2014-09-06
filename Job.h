#ifndef Job_h
#define Job_h

class Job{
public:
  static void setupJobs( Job** job );
  static Job* g_headJob;

  Job();
  virtual ~Job();
  
  Job* nextJob() const{ return _nextJob; }
  
  virtual void setup( int i, int amount );
  unsigned long exec();
  
  void wait();
  
protected:
  virtual void update();
  virtual unsigned long doJob() =0;
  
  unsigned long _lastMillis;
  unsigned long _delayMillis;

private:
  Job* _previousJob;
  Job* _nextJob;
};

#endif // Job_h

