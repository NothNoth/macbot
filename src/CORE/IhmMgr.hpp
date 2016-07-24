#include <vector>
#include <string>
#include <SDL.h>
#include <SDL_ttf.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
//#include "Exception.hpp"

using namespace std;
typedef vector<string> vLogQueue;


class IhmMgr
{
  public:
    static IhmMgr * GetInstance()
    {
      if (_IhmMgr == NULL)
      {
        _IhmMgr = new IhmMgr;
      }
      return _IhmMgr;
    }
    static void DeleteInstance()
    {
      if (!_IhmMgr) return;
      delete(_IhmMgr);
      _IhmMgr = NULL;
    }
    int Run();
    int Run(int iWidth, int iHeight);
    int RunFullScreen();
    int Refresh();
    void log(string _log);
    void StartRemoteIhm();
    void * SocketThread(void * pUserData);
  private:
    IhmMgr();
    ~IhmMgr();
    int _RefreshServos();  
    int _RefreshCam();   
    int _RefreshTargets(); 
    static IhmMgr * _IhmMgr;
    SDL_Surface *   hScreen;
    TTF_Font *      hFont;
    int             iWidth;
    int             iHeight;
    unsigned int    _queueSize;
    vLogQueue       _logQueue;
    pthread_mutex_t _lockLog;
    unsigned int    _prevNbTargets;
    SDL_Surface *   _lastFrame;
    pthread_mutex_t _lockFrame;

    bool            _exitFlag;
    int             _socket;
    pthread_t       _socketThread;
};
/* End of File */
