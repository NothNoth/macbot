/*
 *      macbot.cpp
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

#define _BSD_SOURCE
#define __USE_BSD

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <signal.h>

#include "Exception.hpp"
#include "ServoMgr.hpp"
#include "IhmMgr.hpp"
#include "TargetMgr.hpp"
#include "VoiceMgr.hpp"
#include "MotionDetect.hpp"
int LoadPlugins(char * szPath)
{
  DIR * hD;
  struct dirent * hEnt;
  int iLoadedPlugins = 0;
  printf("> Looking for plugins in %s\n", szPath);
  hD = opendir(szPath);
  if (!hD)
  {
    printf("Can't load dir :%s\n", szPath);
    return -1;
  }
  while ((hEnt = readdir(hD)))
  {
    void * hLib;
    char szLib[255];
    char * szErr;
    struct stat hStat;
    sprintf(szLib, "%s/%s", szPath, hEnt->d_name);
    stat(szLib, &hStat);
    if (S_ISREG(hStat.st_mode))
    {
      hLib = dlopen(szLib, RTLD_NOW);
      szErr = dlerror();
      if (!szErr)
      {
        typedef int (*tInitCb)();
        tInitCb f = (tInitCb) dlsym(hLib, "InitDriver");
        if (f)
        {
          printf("- Trying to initialize %s ...\n", hEnt->d_name);
          if (f() != -1)
          {
            printf("OK\n");
            iLoadedPlugins ++;
          }
          else
          {
            printf("Failed !\n");
            return -1;
          }
        }
        else
        {
          printf("InitDriver not found in %s\n",hEnt->d_name); 
        } 
      }
      else
      {
        printf("Err : %s\n", szErr);
      }
    }
  }
  closedir(hD);
  return iLoadedPlugins;
}


short bDone = 0;
void SigHandler(int signal)
{
  switch (signal)
  {
    case SIGINT:
      bDone = 1;
      printf("Sending exit signal to main loop\n");
    break;
    case SIGPIPE:
    break;
    default:
    break;
  }
}


int main(int argc, char** argv)
{
  int iRet;
  if (argc < 2)
  {
    printf("Usage : %s <lib dir>\n", argv[0]);
    return 0;
  }
  try
  {
    ServoMgr::GetInstance();
    TargetMgr::GetInstance();
    IhmMgr::GetInstance();
    MotionDetect::GetInstance();
    MotionDetect::GetInstance()->startProcessing();
    signal(SIGINT, SigHandler);
    signal(SIGPIPE, SigHandler);
    iRet = LoadPlugins(argv[1]);
    if (iRet == -1)
    {
      printf("Error while loading plugins !\n");
      return -1;
    }
    printf("%d plugins loaded\n", iRet); 
    ServoMgr::GetInstance()->Dump();

    VoiceMgr::GetInstance();
    VoiceMgr::GetInstance()->startSpeaking();
    VoiceMgr::GetInstance()->mute(true);
    VoiceMgr::GetInstance()->say("Let's go !");


    TargetMgr::GetInstance()->startAnalyse();
    


    printf("Stopping servos...\n");
    ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_DROIT"));
    usleep(100000);
    ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_GAUCHE"));
    printf("Servos stopped.\n");

    if (IhmMgr::GetInstance()->Run() == -1)
      bDone = -1;
    IhmMgr::GetInstance()->StartRemoteIhm();;

    int iStep = 0;
    while (!bDone)
    {
      usleep(200000);
      if (IhmMgr::GetInstance()->Refresh() != 0) bDone = 1;
#if 0
      switch (iStep)
      {
        case 0:
          ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_DROIT"));
          usleep(100000);
          ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_GAUCHE"));
          if (ServoMgr::GetInstance()->SetPosition((char *)"TEST1", 0, 4) != 0)  
           printf("set position failed\n");
        break;
        case 10:
          ServoMgr::GetInstance()->ContinuousForward((char *) ("AVT_DROIT"));
          usleep(100000);
          ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_GAUCHE"));
          if (ServoMgr::GetInstance()->SetPosition((char *)"TEST1", 90, 4) != 0)  
           printf("set position failed\n");
        break;
        case 20:
          ServoMgr::GetInstance()->ContinuousBackward((char *) ("AVT_DROIT"));
          usleep(100000);
          ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_GAUCHE"));
          if (ServoMgr::GetInstance()->SetPosition((char *)"TEST1", 180, 4) != 0)  
           printf("set position failed\n");
        break;

        case 30:
          ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_DROIT"));
          usleep(100000);
          ServoMgr::GetInstance()->ContinuousForward((char *) ("AVT_GAUCHE"));
          if (ServoMgr::GetInstance()->SetPosition((char *)"TEST1", 0, 4) != 0)  
           printf("set position failed\n");
        break;


        case 40:
          ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_DROIT"));
          usleep(100000);
          ServoMgr::GetInstance()->ContinuousBackward((char *) ("AVT_GAUCHE"));
          if (ServoMgr::GetInstance()->SetPosition((char *)"TEST1", 90, 4) != 0)  
           printf("set position failed\n");
        break;

        case 50:
          iStep = -1;
        break;
      }
      iStep ++;
#endif
    }
    cout << "Exitted from main loop" << endl;
    MotionDetect::DeleteInstance();
    cout << "Stopping servos ..." << endl;
    ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_DROIT"));
    usleep(100000);
    ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_GAUCHE"));
    cout << "Done" << endl;

    cout << "Stopping Ihm ..." << endl;
    IhmMgr::DeleteInstance();
    cout << "Done" << endl;

    cout << "Stopping Servos manager ..." << endl;
    ServoMgr::DeleteInstance();
    cout << "Done" << endl;

    cout << "Stopping Targets manager ..." << endl;
    TargetMgr::DeleteInstance();
    cout << "Done" << endl;
  }
  catch (Exception ex) {cout << "Error : " << ex.GetMessage() << endl;}

	return 0;
}
