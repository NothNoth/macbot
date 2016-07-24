/* Main header for Cam manager */

#ifndef _CAM_MGR_
#define _CAM_MGR_
#include <cv.h>
#include <cvaux.hpp>

typedef IplImage * (*tCamGetPicture)    (void * pUserData);
typedef void       (*tCamDestroy)       (void * pUserData);

typedef struct
{
  unsigned int    uiWidth;
  unsigned int    uiHeight;
  tCamGetPicture  fGetPicture;
  tCamDestroy     fDestroy;
  void *          pUserData;
} tCam;

class CamMgr
{
  public:
    static CamMgr * GetInstance()
    {
      if (_CamMgr == NULL)
      {
        _CamMgr = new CamMgr;
      }
      return _CamMgr;
    }
    static void DeleteInstance()
    {
      if (!_CamMgr) return;
      delete(_CamMgr);
      _CamMgr = NULL;
    }
    void  Dump();
    short IsRegistered()     {return stCam.fDestroy?1:0;}
    unsigned int GetWidth()  {return stCam.uiWidth;     }
    unsigned int GetHeight() {return stCam.uiHeight;    }
    void  Register(unsigned int uiWidth, unsigned int uiHeight, tCamGetPicture fGetPicture, tCamDestroy fDestroy, void * pUserData);
    IplImage * GetPicture();
  private:
    tCam stCam;
    CamMgr();
    ~CamMgr();  
    static CamMgr * _CamMgr;
};


#endif
/* EOF */
