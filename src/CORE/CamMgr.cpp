/*
 *      Cam_mgr.cpp
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
#include "CamMgr.hpp"
CamMgr * CamMgr::_CamMgr = NULL;


CamMgr::CamMgr()
{
  printf("Starting Cam manager...\n");
  stCam.fDestroy = NULL;
  stCam.fGetPicture = NULL;
  stCam.pUserData = NULL;
}

CamMgr::~CamMgr()
{
  printf("Halting Cam manager\n");
  if (stCam.fDestroy)
    stCam.fDestroy(stCam.pUserData);	
}



void CamMgr::Dump()
{

}


void CamMgr::Register(unsigned int uiWidth, unsigned int uiHeight, tCamGetPicture fGetPictureCb, tCamDestroy fDestroyCb, void * pUserData)
{
  if (stCam.fDestroy)
  {
    printf("Camera already registered !\n");
    return;
  }
  stCam.fDestroy    = fDestroyCb;
  stCam.fGetPicture = fGetPictureCb;
  stCam.pUserData   = pUserData;
  stCam.uiWidth     = uiWidth;
  stCam.uiHeight    = uiHeight;
	printf("New camera registered : %dx%d\n", uiWidth, uiHeight);
}

IplImage * CamMgr::GetPicture()
{
  if (!stCam.fDestroy)
    return NULL;  
  return stCam.fGetPicture(stCam.pUserData);
}

/* EOF */
