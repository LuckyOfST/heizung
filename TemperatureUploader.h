#ifndef TemperatureUploader_h
#define TemperatureUploader_h

#include "Defines.h"
#include "Job.h"

#ifdef SUPPORT_TemperatureUploader

class TemperaturesUploader
:public Job
{
public:
  TemperaturesUploader();

  virtual unsigned long doJob();

private:
  int _retryCnt;
  bool _lastSuccess;
};

#endif // SUPPORT_TemperatureUploader  
#endif // TemperatureUploader_h

