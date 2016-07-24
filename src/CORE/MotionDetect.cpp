/*
 *      MotionDetect.cpp
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
#include <sys/times.h>
#include <string.h>
#include <pthread.h>
#include "MotionDetect.hpp"
#include "CamMgr.hpp"
MotionDetect * MotionDetect::_MotionDetect = NULL;

void * _wrapMotionDetectThread(void * pUserData);

MotionDetect::MotionDetect()
{
  cout << "Starting MotionDetect..." << endl;
  _exitFlag = false;
}

MotionDetect::~MotionDetect()
{
  cout << "MotionDetect halted" << endl;
  _exitFlag = true;
}

void MotionDetect::startProcessing()
{
  pthread_attr_t stAttr;
  pthread_attr_init(&stAttr);

  if (pthread_create(&_thread, &stAttr, _wrapMotionDetectThread, NULL) != 0)
  {
    printf("Can't create motion detect thread !!\n");
  }  
}

void * MotionDetect::updateThread(void * pUserData)
{
  while (this->_exitFlag == false)
  {
    IplImage * img, *grayScaleImage;
    img = CamMgr::GetInstance()->GetPicture();
    if (img)
    {
      tMotionDetect_Frame * f;
      cout << "Got picture, processing ..." << endl;
      //conversion NB
      grayScaleImage = cvCreateImage(cvSize(img->width,img->height),8,1);
      cvCvtColor(img, grayScaleImage, CV_BGR2GRAY);
      cvReleaseImage(&img);

      if (_cache.size() >= 3)
      {
      
        f = _cache.front();
        _cache.pop_front();
        cvReleaseImage(&f->img);
        delete(f);
      }

      f = new tMotionDetect_Frame();
      f->img = img;
      _cache.push_back(f);
    }
    /*
TODO :
  - moyenne des contrastes verticaux et horz
  - calcule du vecteur deplacement par rapport a l'image precedente
  - soustraction des n images en cache avec leur déplacement
  - filtrage
  - detection de zones de mouvement
    */
    sleep(1);
  }
  return NULL;
}

void * _wrapMotionDetectThread(void * pUserData)
{
  return MotionDetect::GetInstance()->updateThread(pUserData);
}

/* EOF */

