/* Main header for Voice manager */

#ifndef _VOICE_MGR_
#define _VOICE_MGR_
#include <iostream>
#include <string>
#include <vector>
#ifndef _MACOS
#include <flite.h>
#endif
using namespace std;
typedef vector<string> vText;


class VoiceMgr
{
  public:
    static VoiceMgr * GetInstance()
    {
      if (_VoiceMgr == NULL)
      {
        _VoiceMgr = new VoiceMgr;
      }
      return _VoiceMgr;
    }
    static void DeleteInstance()
    {
      if (!_VoiceMgr) return;
      delete(_VoiceMgr);
      _VoiceMgr = NULL;
    }
    void say(string _text);
    void * speakThread(void * pUserData);
    void  startSpeaking();
    bool isSpeaking();
    void mute(bool _mute);
  private:
    VoiceMgr();
    ~VoiceMgr();
    short _exitFlag;
    static VoiceMgr * _VoiceMgr;
    vText _spool;
    pthread_t       _thread;
    pthread_mutex_t _lockSpool;
#ifndef _MACOS
    cst_voice *_voice;
#endif
    bool       _isSpeaking;
    bool       _mute;
 };


#endif
/* EOF */
