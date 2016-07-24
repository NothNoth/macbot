/* Parallax driver */
#define _POSIX_SOURCE 1

#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#ifndef _LOCALTEST
#include "ServoMgr.hpp"
#endif
#include "SerialStream.h"

using namespace LibSerial ;  
short aOffsets[16];
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
  short bEnabled;
  short bContinuous;
  short sCForward;
  short sCBackward;
  short sCStop;
  tContinuousStatus eCStatus;
} tServoConfig;

tServoConfig aConfServos[16];
static char szDevice[255] = "";
static SerialStream * _serial = NULL;

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
        aConfServos[iChannel].eCStatus      = eStopped;
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
short setPosition(SerialStream * _serial, short iChannel, short iAngle, int iSpeed)
{
  char pCmd[8];
  short sPos;
  unsigned char * c;
  pCmd[0] = '!';
  pCmd[1] = 'S';
  pCmd[2] = 'C';
  
  pCmd[3] = iChannel;
  pCmd[4] = iSpeed;
  
  sPos = (short)(iAngle * 1000.0 / 180.0) + 200 + aConfServos[iChannel].iOffset;
  printf("Setting angle to : %d (arg : %d) on channel %d\n", iAngle, sPos, iChannel);
  c = (unsigned char *) &sPos;
  pCmd[5] = *c;
  pCmd[6] = *(c+1);
  pCmd[7] = 0x0D;
  _serial->write(pCmd, 8);
  usleep(120000);
  return 0;
}

short getPosition(SerialStream * _serial, short iChannel)
{

  char pCmd[8];
  char channel;
  char p1, p2;
  short sPos;
return -1;
  pCmd[0] = '!';
  pCmd[1] = 'S';
  pCmd[2] = 'C';
  pCmd[3] = 'R';
  pCmd[4] = 'S';
  pCmd[5] = 'P';
  pCmd[6] = iChannel;
  pCmd[7] = 0x0D;
  
  _serial->write(pCmd, 8);

  usleep(120000);

  _serial->get(channel);
  _serial->get(p1);
  _serial->get(p2);
//  printf("Lecture Pos : %d %02x %02x\n", pCmd[0], pCmd[1], pCmd[2]);
  sPos = p1 << 8 | p2;
  if (channel == iChannel)
    return sPos;
  else
    return -1;
}


short isContinuous(SerialStream * _serial, short iChannel)
{
  return aConfServos[iChannel].bContinuous;
}

short continuousForward(SerialStream * _serial, short iChannel)
{
  aConfServos[iChannel].eCStatus = eForward;
  return setPosition(_serial, iChannel, aConfServos[iChannel].sCForward, 1);
}

short continuousBackward(SerialStream * _serial, short iChannel)
{
  aConfServos[iChannel].eCStatus = eBackward;
  return setPosition(_serial, iChannel, aConfServos[iChannel].sCBackward, 1);
}

short continuousStop(SerialStream * _serial, short iChannel)
{
  aConfServos[iChannel].eCStatus = eStopped;
  return setPosition(_serial, iChannel, aConfServos[iChannel].sCStop, 1);
}


void SetPositionCb(short sAngle, short sSpeed, void * pUserData)
{
  setPosition(_serial, (int) pUserData, sAngle, sSpeed);
}

short GetPositionCb(void * pUserData)
{
  return getPosition(_serial, (int) pUserData);
}


void DestroyCb(void * pUserData)
{
  if (_serial != NULL)
  {
    _serial->Close() ;
    delete(_serial);
    _serial = NULL;
  }
}


short IsContinuous(void * pUserData)
{
  return isContinuous(_serial, (int) pUserData);
}

short ContinuousForward(void * pUserData)
{
  return continuousForward(_serial, (int) pUserData);
}

short ContinuousBackward(void * pUserData)
{
  return continuousBackward(_serial, (int) pUserData);
}

short ContinuousStop(void * pUserData)
{
  return continuousStop(_serial, (int) pUserData);
}



extern "C" int InitDriver()
{
  char * szHome;
  char szConfigFile[255];
  int i;
  szHome = getenv("HOME");
  sprintf(szConfigFile, "%s/.macbot/parallax.conf", szHome);
  memset(aConfServos, 0x00, 16 *sizeof(tServoConfig));
  if (ReadConfig(szConfigFile) == -1)
  {
    printf("Error while reading config file : %s\n", szConfigFile);
    return -1;
  }
  _serial = new SerialStream();
  _serial->Open(szDevice);
  if (!_serial->good())
  {
    printf("Can't open device %s\n", szDevice);
    delete(_serial);
    _serial = NULL;
    return -1;
  }
  
  _serial->SetBaudRate(SerialStreamBuf::BAUD_2400);
  if (!_serial->good())
  {
    printf("Can't configure device %s\n", szDevice);
    _serial->Close();
    delete(_serial);
    _serial = NULL;
    return -1;
  }

  _serial->SetCharSize(SerialStreamBuf::CHAR_SIZE_8);
  if (!_serial->good())
  {
    printf("Can't configure device %s\n", szDevice);
    _serial->Close();
    delete(_serial);
    _serial = NULL;
    return -1;
  }

  _serial->SetParity(SerialStreamBuf::PARITY_NONE);
  if (!_serial->good())
  {
    printf("Can't configure device %s\n", szDevice);
    _serial->Close();
    delete(_serial);
    _serial = NULL;
    return -1;
  }

  _serial->SetNumOfStopBits(2);
  if (!_serial->good())
  {
    printf("Can't configure device %s\n", szDevice);
    _serial->Close();
    delete(_serial);
    _serial = NULL;
    return -1;
  }

  _serial->SetFlowControl( SerialStreamBuf::FLOW_CONTROL_NONE ) ;
  if (!_serial->good())
  {
    printf("Can't configure device %s\n", szDevice);
    _serial->Close();
    delete(_serial);
    _serial = NULL;
    return -1;
  }

  for (i = 0; i < 16; i++)
  {
    if (strlen(aConfServos[i].szId) && aConfServos[i].bEnabled)
    {
      printf("Registering %s on channel %d\n", aConfServos[i].szId, i);
#ifndef _LOCALTEST
      ServoMgr::GetInstance()->Register(aConfServos[i].szId, 
                                        SetPositionCb, GetPositionCb, 
                                        IsContinuous, ContinuousForward, ContinuousBackward, ContinuousStop, 
                                        DestroyCb, (void *) i);
#endif
    }
    else if (aConfServos[i].bEnabled)
    {
      setPosition(_serial, i, 90, 1);
    }
  } 
  return 0;
}

#ifdef _LOCALTEST
int main()
{
  char pCmd[8];
  char c;
  printf("Opening serial port ...");
  InitDriver();
  printf(" Done\n");
  pCmd[0] = '!';
  pCmd[1] = 'S';
  pCmd[2] = 'C';
  pCmd[3] = 'V';
  pCmd[4] = 'E';
  pCmd[5] = 'R';
  pCmd[6] = '?';
  pCmd[7] = 0x0D;
  printf("Sending command ...");
  _serial->write(pCmd, 8);
  printf(" Done\n");
  printf("Waiting for answer ...");
  //while(_serial->rdbuf()->in_avail() == 0) 
  usleep(120000) ;
  printf(" Done (%d bytes)\n",_serial->rdbuf()->in_avail());
  printf("Version : ");
  while (_serial->rdbuf()->in_avail() != 0)
  {
    _serial->get(c);
    printf("%c\n", c);
  }
  printf("\n");



  continuousStop(_serial, 0);
  continuousStop(_serial, 1);
  printf("All stopped\n");
  sleep(1);

  setPosition(_serial, 15, 0, 10);
  sleep(5);
  setPosition(_serial, 15, 85, 10);
  sleep(5);
  setPosition(_serial, 15, 170, 10);
  sleep(5);
  setPosition(_serial, 15, 85, 10);

  while (1)
  {
    printf("Forward\n");
    continuousForward(_serial, 0);
    continuousForward(_serial, 1);
    sleep(2);

    continuousStop(_serial, 0);
    continuousStop(_serial, 1);
    printf("All stopped\n");
    sleep(1);

    printf("Bacward\n");
    continuousBackward(_serial, 0);
    continuousBackward(_serial, 1);
    sleep(2);

    continuousBackward(_serial, 0);
    continuousForward(_serial, 1);
    sleep(1);

    continuousForward(_serial, 0);
    continuousBackward(_serial, 1);
    sleep(1);


    continuousStop(_serial, 0);
    continuousStop(_serial, 1);
    printf("All stopped\n");
    sleep(1);
    break;
  }
}
#endif
/* EOF */

