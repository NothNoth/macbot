/* Main header for Target manager */

#ifndef _TARGET_MGR_
#define _TARGET_MGR_
#include <cv.h>
#include <cvaux.hpp>
#include <iostream>
#include <string>
#include <vector>
using namespace std;


typedef struct
{
  string targetName;
  string haarFile;
  CvMemStorage* cvStorage;
  CvHaarClassifierCascade* cvCascade;
} tTargetType;
typedef vector<tTargetType *> vTargetTypes;


typedef struct
{
  int           x;
  int           y;
  int           width;
  int           height;
  tTargetType * type;
  long          _time;
  bool          _assigned;
} tTarget;
typedef vector<tTarget *> vTargets;

typedef struct
{
  vTargets      _history;
  tTargetType * type;
  long          _lastSeen;
} tTargetTracker;
typedef vector<tTargetTracker *> vTargetTrackers;


class TargetMgr
{
  public:
    static TargetMgr * GetInstance()
    {
      if (_TargetMgr == NULL)
      {
        _TargetMgr = new TargetMgr;
      }
      return _TargetMgr;
    }
    static void DeleteInstance()
    {
      if (!_TargetMgr) return;
      delete(_TargetMgr);
      _TargetMgr = NULL;
    }
    void startAnalyse();
    void  dump();
    void * updateThread(void * pUserData);
    int   detectTargets(IplImage * _img);
    unsigned int getNbTargets();
    unsigned int getMaxHistorySize();
    tTargetTracker *     getTargetTracker(unsigned int idx);
    tTarget        *     GetExtrapolation(unsigned int trackId);
    tTarget        *     GetExtrapolation(unsigned int trackId, long time);
    void lock();
    void unlock();
  private:
    TargetMgr();
    ~TargetMgr();
    static TargetMgr * _TargetMgr;
    vTargetTypes       _targetTypes;
    vTargetTrackers    _targets;
    bool               _exitFlag;
    pthread_t          _thread;
    pthread_mutex_t    _lockTargets;
};


#endif
/* EOF */
