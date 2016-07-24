/* Parallax driver */
#define _POSIX_SOURCE 1

#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "ServoMgr.hpp"

typedef enum
{
  eNS = 0,
  eStopped,
  eForward,
  eBackward
} tContinuousStatus;
typedef struct
{
  char szId[64];
  int iOffset;
  short sCurrentPosition;
  short sTargetPosition;
  short sSpeed;
  short bEnabled;
  short bContinuous;
  short sCForward;
  short sCBackward;
  short sCStop;
  tContinuousStatus eCStatus;
} tServoConfig;

tServoConfig aConfServos[16];
static char  szDevice[255] = "";
static int   hSerial = -1;
static short bServoDone = 0;
static pthread_t hThread;

short ReadConfig(char * szFilename)
{
  FILE * f;
  char szLine[255];
  char szId[64];
  int iChannel, iOffset;

  f = fopen(szFilename, "rb");
  if (!f)
    return -1;

  while (!feof(f))
  {
    if ((int)fgets(szLine, 255, f) == EOF) break;

    if ((szLine[0] == '#')||(szLine[0] == '\n')||(!strlen(szLine)))
    {
    }
    else if (!strncmp(szLine, "DEVICE=", 7)) /* DEVICE LINE ... */
    {
      szDevice[0] = '\0';
      sscanf(szLine+7, "%s\n", szDevice);
    }
    else if (!strncmp(szLine, "STD ", 4)) /* STD LINE */
    {
      short bEnabled = 0;
       iChannel = -1;
      sscanf(szLine+4, "%d %s %d %d\n", &iChannel, szId, &iOffset, (int *)&bEnabled);
      if ((iChannel >= 0) && (iChannel < 16))
      {
        strcpy(aConfServos[iChannel].szId, szId);
        aConfServos[iChannel].iOffset     = iOffset;
        aConfServos[iChannel].bContinuous = 0;
        aConfServos[iChannel].sCForward   = 0;
        aConfServos[iChannel].sCBackward  = 0;
        aConfServos[iChannel].sCStop      = 0;
        aConfServos[iChannel].eCStatus    = eNS;
        aConfServos[iChannel].bEnabled    = bEnabled;
      }
      else
      {
        printf("Err : Invalid line \"%s\"\n", szLine);
      }
    }
    else if (!strncmp(szLine, "CONTINUOUS ", 11)) /* CONTINUOUS LINE */
    {
      int bEnabled = -1;
      int sForward = -1, sBackward = -1, sStop = -1;
      iChannel = -1;
      sscanf(szLine+11, "%d %s %d %d %d %d\n", (int *)&iChannel, szId, (int *)&sForward, (int *)&sBackward, (int *)&sStop, (int *)&bEnabled);
      if ((iChannel >= 0) && (iChannel < 16))
      {
        strcpy(aConfServos[iChannel].szId, szId);
        aConfServos[iChannel].iOffset       = 0;
        aConfServos[iChannel].bContinuous   = 1;
        aConfServos[iChannel].sCForward     = sForward;
        aConfServos[iChannel].sCBackward    = sBackward;
        aConfServos[iChannel].sCStop        = sStop;
        aConfServos[iChannel].eCStatus    = eStopped;
        aConfServos[iChannel].bEnabled      = bEnabled;
      }
      else
      {
        printf("Err : Invalid line \"%s\"\n", szLine);
      }
    }
    else
    {
      printf("Invalid line ! (%c)\n", szLine[0]);
    }
  }
  fclose(f);
  
  if (1)
  {
    int i;
    for (i = 0; i < 16; i++)
    {
      printf("- Channel %d - Enabled : %s\n", i, aConfServos[i].bEnabled?"\E[32mYes\E[m":"\E[31mNo\E[m");
      printf("\tType : %s\n", aConfServos[i].bContinuous?"Continuous":"Classic");
      if (aConfServos[i].bContinuous)
      {
        printf("\tForward  : %d\n", aConfServos[i].sCForward);
        printf("\tBackward : %d\n", aConfServos[i].sCBackward);
        printf("\tStop     : %d\n", aConfServos[i].sCStop);
      }
      printf("\tOffset : %d\n", aConfServos[i].iOffset);
      printf("\tName : %s\n", aConfServos[i].szId);

    }
  }
  return 0;
}

/* Channel (0-15) , iAngle (0 - 180), iSpeed (0 - 63) */
void SetPositionCb(short sAngle, short sSpeed, void * pUserData)
{
  aConfServos[(int) pUserData].sTargetPosition = sAngle;
  aConfServos[(int) pUserData].sSpeed    = sSpeed;
}

short GetPositionCb(void * pUserData)
{
  return aConfServos[(int) pUserData].sCurrentPosition;
}


void DestroyCb(void * pUserData)
{
  hSerial = -1;
  bServoDone = 1;
  pthread_join(hThread, NULL);
}

void * _ServoThread(void * pUserData)
{
  int i;

  while (!bServoDone)
  {
    //on veut currentpos = targetpos
    for (i = 0; i < 16; i++)
    {
      if (strlen(aConfServos[i].szId) && (aConfServos[i].sCurrentPosition != aConfServos[i].sTargetPosition))
      {
        short sStep;
        if (aConfServos[i].sCurrentPosition < aConfServos[i].sTargetPosition)
          sStep = 65-aConfServos[i].sSpeed;
        else
          sStep = aConfServos[i].sSpeed-65;
          /*
          printf("[%d] %d -> %d at speed %d. Step : %d\n", i,
           aConfServos[i].sCurrentPosition,
           aConfServos[i].sTargetPosition,
           aConfServos[i].sSpeed,
           sStep);
            */
        if ((aConfServos[i].sCurrentPosition < aConfServos[i].sTargetPosition) && 
            (aConfServos[i].sCurrentPosition + sStep > aConfServos[i].sTargetPosition)) 
          aConfServos[i].sCurrentPosition  = aConfServos[i].sTargetPosition;
        else if ((aConfServos[i].sCurrentPosition > aConfServos[i].sTargetPosition) && 
            (aConfServos[i].sCurrentPosition + sStep < aConfServos[i].sTargetPosition)) 
          aConfServos[i].sCurrentPosition  = aConfServos[i].sTargetPosition;
        else
          aConfServos[i].sCurrentPosition += sStep;
      }
    }    
    usleep(100000);
    //TODO bouger les servos pas encore en place.
  }

  return NULL;
}

short isContinuous(int hSerial, short iChannel)
{
  return aConfServos[iChannel].bContinuous;
}

short continuousForward(int hSerial, short iChannel)
{
  aConfServos[iChannel].eCStatus = eForward;
  return 0;
}

short continuousBackward(int hSerial, short iChannel)
{
  aConfServos[iChannel].eCStatus = eBackward;
  return 0;
}

short continuousStop(int hSerial, short iChannel)
{
  aConfServos[iChannel].eCStatus = eStopped;
  return 0;
}

short IsContinuous(void * pUserData)
{
  return isContinuous(hSerial, (int) pUserData);
}

short ContinuousForward(void * pUserData)
{
  return continuousForward(hSerial, (int) pUserData);
}

short ContinuousBackward(void * pUserData)
{
  return continuousBackward(hSerial, (int) pUserData);
}

short ContinuousStop(void * pUserData)
{
  return continuousStop(hSerial, (int) pUserData);
}


extern "C" int InitDriver()
{
  char * szHome;
  char szConfigFile[255];
  int i;
  pthread_attr_t stAttr;
  szHome = getenv("HOME");
  sprintf(szConfigFile, "%s/.macbot/parallax.conf", szHome);
  memset(aConfServos, 0x00, 16 *sizeof(tServoConfig));
  if (ReadConfig(szConfigFile) == -1)
  {
    printf("Error while reading config file : %s\n", szConfigFile);
    return -1;
  }

  for (i = 0; i < 16; i++)
  {
    if (strlen(aConfServos[i].szId) && aConfServos[i].bEnabled)
    {
      printf("Registering %s on channel %d\n", aConfServos[i].szId, i);
      ServoMgr::GetInstance()->Register(aConfServos[i].szId, 
                                        SetPositionCb, GetPositionCb,
                                        IsContinuous,
                                        ContinuousForward, 
                                        ContinuousBackward, 
                                        ContinuousStop,
                                        DestroyCb, (void *) i);
    }
  }
  memset(&stAttr, 0x00, sizeof(pthread_attr_t));
  pthread_create(&hThread, &stAttr, _ServoThread, NULL);
  return 0;
}


/* EOF */
