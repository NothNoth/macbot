/*
 *      servo_mgr.cpp
 *      
 *      Copyright 2009 Noth <noth@minus>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */
#include <iostream>
#include <string.h>
#include "ServoMgr.hpp"
ServoMgr * ServoMgr::_ServoMgr = NULL;


ServoMgr::ServoMgr()
{
  printf("Starting servo manager...\n");
  memset(this->aServos, 0x00, MAX_SERVO*sizeof(tServo));
  uiNbServos = 0;
}

ServoMgr::~ServoMgr()
{
  int i;
  printf("Halting servo manager\n");
  for (i = 0; i < MAX_SERVO; i++)
  {
    if (this->aServos[i].fGetPosition)
      this->aServos[i].fDestroy(aServos[i].pUserData);	
  }
}



void ServoMgr::Dump()
{
  int i;
  printf("Registered servos :\n");
  for (i = 0; i < MAX_SERVO; i++)
  {
    if (this->aServos[i].fGetPosition)
      printf("\tServo %d : %s\n", i, this->aServos[i].szId);	
  }
}
char * ServoMgr::GetServoIdByIdx(int iIdx)
{
  if ((iIdx >= MAX_SERVO) || 
      (!aServos[iIdx].fGetPosition))
    return NULL;
  return aServos[iIdx].szId;
}

inline int ServoMgr::GetById(char * szId)
{
  int i;
  for (i = 0; i < MAX_SERVO; i++)
  {
    if (!strcmp(this->aServos[i].szId, szId))
      return i;
  } 
  return -1;  
}


void ServoMgr::Register(char * szId, 
                        tSrvSetPosition fSetPosCb, tSrvGetPosition fGetPosCb, 
                        tSrvIsContinuous          fIsContinuous, 
                        tSrvContinuousForward     fContinuousForward,
                        tSrvContinuousBackward    fContinuousBackward,
                        tSrvContinuousStop        fContinuousStop,
                        tSrvDestroy fDestroyCb, void * pUserData)
{
  int i;
  if (Exists (szId))
  {
    printf("Err : servo alreéady registered : %s\n", szId);
    return;
  }
  for (i = 0; i < MAX_SERVO; i++)
  {
    if (!strlen(aServos[i].szId))
    {
      strcpy(aServos[i].szId, szId);
      aServos[i].fSetPosition = fSetPosCb;
      aServos[i].fGetPosition = fGetPosCb;
      aServos[i].fDestroy     = fDestroyCb;
      
      aServos[i].fIsContinuous        = fIsContinuous;
      aServos[i].fContinuousForward   = fContinuousForward;
      aServos[i].fContinuousBackward  = fContinuousBackward;
      aServos[i].fContinuousStop      = fContinuousStop;

      
      aServos[i].pUserData            = pUserData;
      aServos[i].sLastAngleSet        = 0;
      aServos[i].bIsContinuousMoving  = 0;
      this->uiNbServos ++;
      return;
	  }	
  }
  printf("Err : too many servos registered !\n");
}

int ServoMgr::SetPosition(char * szId, short sAngle, short sSpeed)
{
  int iIdx;
  iIdx = GetById(szId);
  if (iIdx == -1)
    return -1;  
  aServos[iIdx].sLastAngleSet = sAngle;
  aServos[iIdx].fSetPosition(sAngle, sSpeed, aServos[iIdx].pUserData);
  return 0;
}

short ServoMgr::GetPosition(char * szId)
{
  int iIdx;
  iIdx = GetById(szId);
  if (iIdx == -1)
    return -1;  
  
  return aServos[iIdx].fGetPosition(aServos[iIdx].pUserData);
}

short ServoMgr::IsMoving(char * szId)
{
  int iIdx;
  iIdx = GetById(szId);
  if (iIdx == -1)
    return -1;  
  if ((aServos[iIdx].sLastAngleSet != GetPosition(szId)) ||
  (aServos[iIdx].bIsContinuousMoving))
    return 1;
  else
    return 0;
}
short ServoMgr::Exists(char * szId)
{
  int iIdx;
  iIdx = GetById(szId);
  if (iIdx == -1)
    return 0;  
  return 1;
}

short ServoMgr::IsContinuous(char * szId)
{
  int iIdx;
  iIdx = GetById(szId);
  if (iIdx == -1)
    return -1;  
  
  return aServos[iIdx].fIsContinuous(aServos[iIdx].pUserData);
}

short ServoMgr::ContinuousForward(char * szId)
{
  int iIdx;
  iIdx = GetById(szId);
  if (iIdx == -1)
    return -1;  
  aServos[iIdx].bIsContinuousMoving = 1;
  return aServos[iIdx].fContinuousForward(aServos[iIdx].pUserData);
}

short ServoMgr::ContinuousBackward(char * szId)
{
  int iIdx;
  iIdx = GetById(szId);
  if (iIdx == -1)
    return -1;  
  aServos[iIdx].bIsContinuousMoving = 1;
  return aServos[iIdx].fContinuousBackward(aServos[iIdx].pUserData);
}

short ServoMgr::ContinuousStop(char * szId)
{
  int iIdx;
  iIdx = GetById(szId);
  if (iIdx == -1)
    return -1;  
  aServos[iIdx].bIsContinuousMoving = 0;
  return aServos[iIdx].fContinuousStop(aServos[iIdx].pUserData);
}
/* EOF */
