/* Main header for servo manager */

#ifndef _SERVO_MGR_
#define _SERVO_MGR_

#define MAX_SERVO 16

typedef short  (*tSrvIsContinuous)        (void * pUserData);
typedef short  (*tSrvContinuousForward)   (void * pUserData);
typedef short  (*tSrvContinuousBackward)  (void * pUserData);
typedef short  (*tSrvContinuousStop)      (void * pUserData);

typedef void  (*tSrvSetPosition)          (short sAngle, short sSpeed, void * pUserData);
typedef short (*tSrvGetPosition)          (void * pUserData);
typedef void (*tSrvDestroy)               (void * pUserData);

typedef struct
{
  char            szId[64];
  tSrvSetPosition fSetPosition;
  tSrvGetPosition fGetPosition;
  tSrvDestroy     fDestroy;
  
  tSrvIsContinuous       fIsContinuous;
  tSrvContinuousForward  fContinuousForward;
  tSrvContinuousBackward fContinuousBackward;
  tSrvContinuousStop     fContinuousStop;
  short                  bIsContinuousMoving;
  
  void *          pUserData;
  short           sLastAngleSet;
} tServo;

class ServoMgr
{
  public:
    static ServoMgr * GetInstance()
    {
      if (_ServoMgr == NULL)
      {
        _ServoMgr = new ServoMgr;
      }
      return _ServoMgr;
    }
    static void DeleteInstance()
    {
      if (!_ServoMgr) return;
      delete(_ServoMgr);
      _ServoMgr = NULL;
    }
    void  Dump();
    void  Register(char * szId, 
                   tSrvSetPosition fSetPosCb, 
                   tSrvGetPosition fGetPosCb, 
                   tSrvIsContinuous          fIsContinuous, 
                   tSrvContinuousForward     fContinuousForward,
                   tSrvContinuousBackward    fContinuousBackward,
                   tSrvContinuousStop        fContinuousStop,
                   tSrvDestroy fDestroy, void * pUserData);
    int   SetPosition(char * szId, short sAngle, short sSpeed);
    short GetPosition(char * szId);
    short IsContinuous(char * szId);
    short ContinuousForward(char * szId);
    short ContinuousBackward(char * szId);
    short ContinuousStop(char * szId);
    short GetNbServos()
    {
      return uiNbServos;
    }
    short Exists(char * szId);
    char * GetServoIdByIdx(int iIdx);
    short IsMoving(char * szId);
  private:
    ServoMgr();
    ~ServoMgr();  
    static ServoMgr * _ServoMgr;
    tServo aServos[MAX_SERVO];
    inline int GetById(char * szId);
    unsigned int uiNbServos;
};


#endif
/* EOF */
