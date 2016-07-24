/*
 *      ihm_main.cpp
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
#include <stdio.h>
#include <SDL_gfxPrimitives.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>
#include "IhmMgr.hpp"
#include "ServoMgr.hpp"
#include "CamMgr.hpp"
#include "TargetMgr.hpp"
#include "VoiceMgr.hpp"
#include "Exception.hpp"
using namespace std;

IhmMgr * IhmMgr::_IhmMgr = NULL;
#define REMOTE_IHM_PORT 1555

#define STD_GRAY 0xFF444444
#define STD_BLUE 0xFF4444AA
#ifdef _LINUX
#define DEFAULT_FONT "/usr/share/fonts/truetype/freefont/FreeMono.ttf"
#else
#define DEFAULT_FONT "/Applications/Microsoft Office 2004/Office/Fonts/MS Gothic.ttf"
#endif

IhmMgr::IhmMgr()
{
  this->iWidth = 480;
  this->iHeight = 640;
  pthread_mutexattr_t hMutt, hMutt2;
  pthread_mutexattr_init(&hMutt);
	pthread_mutex_init (&_lockLog, &hMutt); //LEAK ???
  pthread_mutexattr_init(&hMutt2);
	pthread_mutex_init (&_lockFrame, &hMutt2); //LEAK ???

  TTF_Init();
  hFont = TTF_OpenFont(DEFAULT_FONT, 10);


  if (!hFont)
  {
    printf("Can't open font !\n");
    exit(0);
  }
  _queueSize = 5;
  _prevNbTargets = 0;
  _exitFlag = false;

  _lastFrame = NULL;
  _socketThread = 0;
}

IhmMgr::~IhmMgr()
{
  TTF_CloseFont(hFont);
  TTF_Quit();
  SDL_Quit();
  pthread_mutex_destroy(&_lockLog);
  _exitFlag = true;
  if (_socketThread != 0)
  {
    cout << "Waiting for frame thread,to join ..." << endl; 
    pthread_join(_socketThread, NULL);
    cout << "Joined !" << endl;
  }
  if (_lastFrame) SDL_FreeSurface(_lastFrame);
  pthread_mutex_destroy(&_lockFrame);
  printf("Ihm stopped\n");
}
    
int IhmMgr::Run()
{
  return Run(640, 480);
}    

int IhmMgr::RunFullScreen()
{
  return Run(-1, -1);
}


/***************************************************
 * Init Ihm
 ***************************************************/
int IhmMgr::Run(int iWidth, int iHeight)
{
  const SDL_VideoInfo * pstVI;
  int iFlags = SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE;
  
  if ((iWidth == -1) && (iHeight == -1))
    iFlags |= SDL_FULLSCREEN;

  printf("init vid...\n");
  if (SDL_Init(iFlags) == -1)
  {
    printf("SDL Init failed\n");
    return -1;
  }
  if (SDL_VideoInit(NULL, 0) == -1)
  {
    printf("SDL VideoInit failed\n");
    return -1;
  }
  if (iFlags & SDL_FULLSCREEN)
  {
    pstVI = SDL_GetVideoInfo();
    if (!pstVI) 
    {
      printf("Can't get Video Info\n");
      return -1;
    }
    printf("Current resolution : %dx%d\n", pstVI->current_w, pstVI->current_h);
    this->iWidth = pstVI->current_w;
    this->iHeight = pstVI->current_h; 
  }
  else
  {
    this->iWidth = iWidth;
    this->iHeight = iHeight;
  }
#ifdef _LOCAL_IHM
  SDL_SetVideoMode(this->iWidth, this->iHeight, 32, SDL_DOUBLEBUF|SDL_HWSURFACE);
  hScreen = SDL_GetVideoSurface();
  SDL_FillRect(hScreen, NULL, 0xFF000000);
  SDL_Flip(hScreen);
#endif
  _lastFrame = SDL_CreateRGBSurface(SDL_HWSURFACE, 
                                    this->iWidth, 
                                    this->iHeight, 
                                    32, 
                                    0x00FF0000,
                                    0x0000FF00,
                                    0x000000FF,
                                    0xFF000000);

  printf("done.\n");
  return 0;
}


/***************************************************
 * Fetch & Blit servos positions
 ***************************************************/
int IhmMgr::_RefreshServos()
{
  SDL_Rect stRect;
  int i;
  int iNbServos = ServoMgr::GetInstance()->GetNbServos();

  /* Motors zone */
  stRect.x = 0;
  stRect.y = 100;
  stRect.w = iWidth;
  stRect.h = 2;
  SDL_FillRect(_lastFrame, &stRect, STD_GRAY);
  stRect.x = 0;
  stRect.y = 0;
  stRect.w = 2;
  stRect.h = 100;

  for (i = 0; i < iNbServos; i++)
  {
    int j;
    char szLabel[255];
    int iOrigin = (i * iWidth)/(iNbServos) + (int) (iWidth/(iNbServos*2.0)) - 14;
    SDL_Surface * hLabel;
    SDL_Color hColor;
    SDL_Rect stRect2;
    stRect.x = (i * iWidth)/(iNbServos);
    SDL_FillRect(_lastFrame, &stRect, STD_GRAY);

    /* Draw servo status */
    // Servo's drawing
    for (j = 0; j < 28; j += 4)
    {
      stRect2.x = iOrigin + j;
      stRect2.y = 15;
      stRect2.w = 2;
      stRect2.h = 40;
      SDL_FillRect(_lastFrame, &stRect2, STD_GRAY);
      
      stRect2.y = 60;
      stRect2.h = 4;
      if (ServoMgr::GetInstance()->IsMoving(ServoMgr::GetInstance()->GetServoIdByIdx(i)))
        SDL_FillRect(_lastFrame, &stRect2, STD_BLUE);
      else
        SDL_FillRect(_lastFrame, &stRect2, STD_GRAY);
    }
    if (ServoMgr::GetInstance()->IsContinuous (ServoMgr::GetInstance()->GetServoIdByIdx(i)))
    {
      stRect2.x = iOrigin;
      stRect2.y = 65;
      stRect2.w = 26;
      stRect2.h = 4;
      SDL_FillRect(_lastFrame, &stRect2, STD_GRAY);
    }
    // Servo's label
    sprintf(szLabel, "[ %s ]", ServoMgr::GetInstance()->GetServoIdByIdx(i));
    hColor.r = 130;
    hColor.g = 130;
    hColor.b = 130;
    hLabel = TTF_RenderUTF8_Solid(hFont, szLabel, hColor);
    stRect2.x = (i * iWidth)/(iNbServos) + (int) (iWidth/(iNbServos*2.0)) - (int)(hLabel->w/2.0);
    stRect2.y = 70;
    stRect2.w = 0;
    stRect2.h = 0;
    SDL_BlitSurface(hLabel, NULL, _lastFrame, &stRect2);
    SDL_FreeSurface(hLabel);
    //Servo's position
    sprintf(szLabel, "Pos: %d°", ServoMgr::GetInstance()->GetPosition(ServoMgr::GetInstance()->GetServoIdByIdx(i)));
    hLabel = TTF_RenderUTF8_Solid(hFont, szLabel, hColor);
    stRect2.x = (i * iWidth)/(iNbServos) + (int) (iWidth/(iNbServos*2.0)) - (int)(hLabel->w/2.0);
    stRect2.y = 85;
    stRect2.w = 0;
    stRect2.h = 0;
    SDL_BlitSurface(hLabel, NULL, _lastFrame, &stRect2);    
    SDL_FreeSurface(hLabel);
  }
  return 0;
}

/***************************************************
 * Fetch & Blit targets
 ***************************************************/
int IhmMgr::_RefreshTargets()
{
  tTarget * t;
  SDL_Color  hColor;
  char szStr[255];
  hColor.r = 255;
  hColor.g = 50;
  hColor.b = 50;

  TargetMgr::GetInstance()->lock();
  if (TargetMgr::GetInstance()->getNbTargets() < _prevNbTargets)
  {
    sprintf(szStr, "%d targets lost", _prevNbTargets-TargetMgr::GetInstance()->getNbTargets());
    VoiceMgr::GetInstance()->say(szStr);
    log(szStr);
  }
  else if (TargetMgr::GetInstance()->getNbTargets() > _prevNbTargets)
  {
    sprintf(szStr, "%d new targets found", TargetMgr::GetInstance()->getNbTargets() - _prevNbTargets);
    VoiceMgr::GetInstance()->say(szStr);
    log(szStr);
  }
  _prevNbTargets = TargetMgr::GetInstance()->getNbTargets();
  for (unsigned int i = 0; i < TargetMgr::GetInstance()->getNbTargets(); i++)
  {
    SDL_Rect hRect;

    int prevX, prevY;
    tTargetTracker * _tt;
    _tt = TargetMgr::GetInstance()->getTargetTracker(i);

    /* Draw Extrapolation */
    t = TargetMgr::GetInstance()->GetExtrapolation(i);
    if (t)
    {
      rectangleColor(_lastFrame, t->x, t->y, t->x + t->width, t->y + t->height, 0xF69E12FF);
      aalineColor(_lastFrame, 
                    t->x + (t->width/2.0), t->y + (t->height/2.0), 
                   _tt->_history[0]->x + (_tt->_history[0]->width/2.0), _tt->_history[0]->y + (_tt->_history[0]->height/2.0),
                   0xF69E1266);
      delete(t);
    }

    /* Draw last detection */
    t = _tt->_history[0];
    rectangleColor(_lastFrame, t->x, t->y, t->x + t->width, t->y + t->height, 0x00FF00FF);
    sprintf(szStr, "%s [%dx%d]", t->type->targetName.c_str(), t->width, t->height);
    SDL_Surface * hLabel = TTF_RenderUTF8_Solid(hFont, szStr, hColor);
    hRect.x = t->x;
    hRect.y = t->y + t->height + 5;
    hRect.w = 0;
    hRect.h = 0;
    SDL_BlitSurface(hLabel, NULL, _lastFrame, &hRect);
    SDL_FreeSurface(hLabel);

    /* Draw Track history */
    prevX = t->x + (t->width/2.0);
    prevY = t->y + (t->height/2.0);
    for (unsigned int j = 1; j < _tt->_history.size(); j++)
    {
      t = _tt->_history[j];
      aalineColor(_lastFrame, prevX, prevY, t->x + (t->width/2.0), t->y + (t->height/2.0), 0x00FF0066);
      prevX = t->x + (t->width/2.0);
      prevY = t->y + (t->height/2.0);
    }
  }
  TargetMgr::GetInstance()->unlock();
  return 0;
}


/***************************************************
 * Fetch & Blit camera picture
 ***************************************************/
int IhmMgr::_RefreshCam()
{
  IplImage * pImg;
  SDL_Rect stRect;
  SDL_Surface * hCam;
  if (!CamMgr::GetInstance()->IsRegistered())
    return -1;

  pImg = CamMgr::GetInstance()->GetPicture();
  if (!pImg)
  {
    printf("refresh cam.. -> no pic\n");
    return 0;
  }
	hCam = SDL_CreateRGBSurfaceFrom((void*)pImg->imageData,
																					pImg->width,
																					pImg->height,
																					pImg->depth*pImg->nChannels, // depth (8 x 3)
																					pImg->widthStep, // pitch (w * 3)
																					0x00FF0000,
																					0x0000FF00,
																					0x000000FF,
																					0x00000000);
	stRect.x = 0;
  stRect.y = 0;
  stRect.w = -1;
  stRect.h = -1;
  SDL_BlitSurface(hCam, NULL, _lastFrame, &stRect);    
  SDL_FreeSurface(hCam);
	if (pImg)
		cvReleaseImage(&pImg);
  return 0;
}

/***************************************************
 * Main refresh method
 ***************************************************/
int IhmMgr::Refresh()
{
  SDL_Event hEvent;
  SDL_Rect hRect;
  /* Check Events*/
  SDL_PumpEvents();
  while (SDL_PollEvent(&hEvent))
  {
    if (hEvent.type == SDL_QUIT)
      return 1;
  }
  pthread_mutex_lock(&_lockFrame);
  SDL_FillRect(_lastFrame, NULL, 0xFF000000);
	_RefreshCam();
	_RefreshTargets();
  _RefreshServos();
  if (VoiceMgr::GetInstance()->isSpeaking())
  {
    hRect.x = 5;
    hRect.y = 5;
    hRect.w = 5;
    hRect.h = 5;   
    SDL_FillRect(_lastFrame, &hRect, 0xFF00FF00);  
  }
  /* Affiche les logs */
  pthread_mutex_lock(&_lockLog);
  for (unsigned int i = 0; i < _logQueue.size(); i++)
  {
    SDL_Color  hColor;
    hColor.r = 255;
    hColor.g = 50;
    hColor.b = 50;

    hRect.x = 20;
    hRect.y = this->iHeight - 200 + i*10;
    hRect.w = 0;
    hRect.h = 0;
    SDL_Surface * hLabel = TTF_RenderUTF8_Solid(hFont, _logQueue[i].c_str(), hColor);
    SDL_BlitSurface(hLabel, NULL, _lastFrame, &hRect);
    SDL_FreeSurface(hLabel);
  }
  pthread_mutex_unlock(&_lockLog);
#ifdef _LOCAL_IHM
  SDL_BlitSurface(_lastFrame, NULL, hScreen, NULL);
  SDL_Flip(hScreen);
#endif
  pthread_mutex_unlock(&_lockFrame);
  return 0;
}

void IhmMgr::log(string _log)
{
  pthread_mutex_lock(&_lockLog);
  _logQueue.push_back(_log);
  if (_logQueue.size() > _queueSize)
    _logQueue.erase(_logQueue.begin());
  pthread_mutex_unlock(&_lockLog);
}

/* Remote Ihm */
void * _wrapSocketThread(void * pUserData)
{
  return IhmMgr::GetInstance()->SocketThread(pUserData);
}

void IhmMgr::StartRemoteIhm()
{
  pthread_attr_t stAttr;
  pthread_attr_init(&stAttr);

  if (pthread_create(&_socketThread, &stAttr, _wrapSocketThread, NULL) != 0)
    throw Exception("Can't create Ihm thread !!\n");
}

char * SDL_SurfaceToBuffer(SDL_Surface * surf, unsigned int * size)
{
  char * buf;
  int i;
  SDL_LockSurface(surf);
// 4 bytes per pix - 32 bits per pix
  *size = surf->w*surf->h*4;
  buf = (char *) malloc(*size);
  memset(buf, 0x00, *size);
  for (i = 0; i < surf->h; i++)
  {
    memcpy(buf+i*surf->w*4, ((char*)surf->pixels)+i*surf->pitch, surf->w*4);
  }
  SDL_UnlockSurface(surf);
  return buf;
}

void * IhmMgr::SocketThread(void * pUserData)
{  
  struct sockaddr_in servAddr;

  _socket = socket(AF_INET, SOCK_STREAM, 0);
  if (_socket < 0)
    throw Exception("Can't open socket !\n");

  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(REMOTE_IHM_PORT);

  if(bind(_socket, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
     throw Exception("Can't bind to port !\n");
  
  
  listen(_socket, 2);
   
  while(_exitFlag != true) 
  {
    bool killClient = false;
    socklen_t length;
    int clientSocket;
    struct sockaddr_in remoteAddr;

    /* Accept connection */
    length = sizeof(remoteAddr);
    fcntl(_socket, F_SETFL, O_NONBLOCK); 
    clientSocket = accept(_socket, (struct sockaddr *) &remoteAddr, &length);

    if(clientSocket > 0) 
    {
      /* Client loop */
      printf("Connection from %s:%d\n", inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port));
      VoiceMgr::GetInstance()->say("Remote client connected !");
      while ((_exitFlag != true) && (killClient != true))
      {
        int sended = 0;
        char * buf, * compbuf;
        int size;
        int compsize;
        int head[4];

        /* Read input commands */
        /* FIXME : a intégrer dans un gestionnaire de commandes externes */  
        int sizeRead;
        char readBuf[16];
        fd_set rdfs;
        struct timeval tv;
        int retval;
        FD_ZERO(&rdfs);
        FD_SET(clientSocket, &rdfs);

        tv.tv_sec = 0;
        tv.tv_usec = 5;
        retval = select(clientSocket+1, &rdfs, NULL, NULL, &tv);
        if (retval)
        {
          sizeRead = recv(clientSocket, readBuf, 16*sizeof(char), 0);
          if (sizeRead)
          {
            cout << "Received distant command : " << readBuf << endl;
            if (!strcmp(readBuf, "STOP"))
            {
              ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_DROIT"));
              ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_GAUCHE"));
            }
            else if (!strcmp(readBuf, "FORWARD"))
            {
              ServoMgr::GetInstance()->ContinuousForward((char *) ("AVT_DROIT"));
              ServoMgr::GetInstance()->ContinuousForward((char *) ("AVT_GAUCHE"));
            }
            else if (!strcmp(readBuf, "BACKWARD"))
            {
              ServoMgr::GetInstance()->ContinuousBackward((char *) ("AVT_DROIT"));
              ServoMgr::GetInstance()->ContinuousBackward((char *) ("AVT_GAUCHE"));
            }
            else if (!strcmp(readBuf, "LEFT"))
            {
              ServoMgr::GetInstance()->ContinuousForward((char *) ("AVT_DROIT"));
              ServoMgr::GetInstance()->ContinuousBackward((char *) ("AVT_GAUCHE"));
              sleep(1);
              ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_DROIT"));
              ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_GAUCHE"));
            }
            else if (!strcmp(readBuf, "RIGHT"))
            {
              ServoMgr::GetInstance()->ContinuousBackward((char *) ("AVT_DROIT"));
              ServoMgr::GetInstance()->ContinuousForward((char *) ("AVT_GAUCHE"));
              sleep(1);
              ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_DROIT"));
              ServoMgr::GetInstance()->ContinuousStop((char *) ("AVT_GAUCHE"));
            }
          }
        }
        
        /* Write frames */
        // Prepare frame
        pthread_mutex_lock(&_lockFrame);
        buf = SDL_SurfaceToBuffer(_lastFrame, (unsigned int *) &size);
        pthread_mutex_unlock(&_lockFrame);
        compsize = compressBound(size);
        compbuf = (char *) malloc(compsize);
        if (compress2((Bytef *) compbuf, (uLong*) &compsize, (Bytef *) buf, (uLong) size, Z_BEST_SPEED) != Z_OK)
          cout << "Compression failed" << endl;
        free(buf);
        head[0] = size;
        head[1] = compsize;
        head[2] = _lastFrame->w;
        head[3] = _lastFrame->h;
        //send frame
        if (send(clientSocket, head, 4*sizeof(int), 0) != 4*sizeof(int))
        {
          cout << "Error while writing img header." << endl;
          killClient = true;
        }
        sended = 0;
        do
        {
          int n;
          n = send(clientSocket, compbuf + sended, (compsize - sended)<512?(compsize - sended):512, 0);
          sended += n;
          if (n < 512) break; // done or error
        }
        while (sended < compsize);
        free(compbuf);
        if (sended != compsize)
        {
          printf("Error while sending frame %d/%d bytes sent\n", sended, compsize);
          killClient = true;
        }
        usleep(100);
      }
      cout << "Client disconnected" << endl;
      VoiceMgr::GetInstance()->say("Remote client disconnected !");
      close(clientSocket);
    }
  }
  close(_socket);
  _socket = -1;
  return NULL;
}
/* EOF */
