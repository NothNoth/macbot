/* Main header for MotionDetect */

#ifndef _MOTION_DETECT_
#define _MOTION_DETECT_
#include <cv.h>
#include <cvaux.hpp>
#include <iostream>
#include <string>
#include <list>
#include <vector>
using namespace std;

typedef struct
{
  IplImage * img;
  
} tMotionDetect_Frame;

class MotionDetect
{
  public:
    static MotionDetect * GetInstance()
    {
      if (_MotionDetect == NULL)
      {
        _MotionDetect = new MotionDetect;
      }
      return _MotionDetect;
    }
    static void DeleteInstance()
    {
      if (!_MotionDetect) return;
      delete(_MotionDetect);
      _MotionDetect = NULL;
    }
    void * updateThread(void * pUserData);
    void startProcessing();
  private:
    MotionDetect();
    ~MotionDetect();
    static MotionDetect * _MotionDetect;
    bool               _exitFlag;
    pthread_t          _thread;

    list<tMotionDetect_Frame *> _cache;
};


#endif
/* EOF */
