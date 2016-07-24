/*
 *      TargetMgr.cpp
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
#include "TargetMgr.hpp"
#include "CamMgr.hpp"
TargetMgr * TargetMgr::_TargetMgr = NULL;
#define _HISTORY_SIZE  10
#define TARGET_TIMEOUT 4000
void * _wrapTargetMgrThread(void * pUserData);


TargetMgr::TargetMgr()
{
  FILE * f;
  char szLine[255];
  printf("Starting Target manager...\n");
  char szConfigFile[255];
  tTargetType * _currentTarget = NULL;
	pthread_mutexattr_t hMut;
  sprintf(szConfigFile, "%s/.macbot/targets.conf", getenv("HOME"));
  cout << "Initializing targets manager ..." << endl;
  f = fopen(szConfigFile, "rb");
  if (!f)
  {
    printf("Can't find config file %s\n", szConfigFile);
    return;
  }
  while (!feof(f))
  {
    if ((int)fgets(szLine, 255, f) == EOF) break;
    szLine[strlen(szLine) - 1] = '\0';
    if ((szLine[0] == '#')||(szLine[0] == '\n')||(!strlen(szLine)))
    {
    }
    else if (strstr(szLine, "TARGET="))
    {
      if (_currentTarget)
        _targetTypes.push_back(_currentTarget);
      _currentTarget = new tTargetType();
      _currentTarget->targetName = szLine+7;
    }
    else if (strstr(szLine, "HAARFILE="))
    {
      _currentTarget->haarFile = szLine+9;
    }
    else
    {
      cout << "Unknown entry : " << szLine << endl;
    }
  }
  if (_currentTarget)
    _targetTypes.push_back(_currentTarget);
  for (unsigned int i = 0; i < _targetTypes.size(); i++)
  {
    _targetTypes[i]->cvCascade = (CvHaarClassifierCascade*)cvLoad( _targetTypes[i]->haarFile.c_str(), 0, 0, 0 );
    _targetTypes[i]->cvStorage = cvCreateMemStorage(0);
    if (!_targetTypes[i]->cvCascade)
      cout << "Can't load target " << _targetTypes[i]->targetName <<  "haaar file not found !" << endl;
  }
  dump();
  _exitFlag = 0;
	pthread_mutexattr_init(&hMut);
	pthread_mutex_init (&_lockTargets, &hMut);
  cout << "Targets Manager started." << endl;
}

TargetMgr::~TargetMgr()
{
  _exitFlag = 1;
  pthread_join(_thread, NULL);
	pthread_mutex_destroy(&_lockTargets);
  for (unsigned int i = 0; i < _targetTypes.size(); i++)
    delete(_targetTypes[i]);
  cout << "Targets Manager halted" << endl;
}



void TargetMgr::dump()
{
  cout << "Known Targets ;" << endl;
  for (unsigned int i = 0; i < _targetTypes.size(); i++)
    cout << _targetTypes[i]->targetName << "-> " << _targetTypes[i]->haarFile << endl;
}

int TargetMgr::detectTargets(IplImage * _img)
{
  int _nb = 0;
  struct tms tt;
  vTargets _newTargets;
  long _time;

  _time = (long) times(&tt);
  /* Detects all targets in picture */
  for (unsigned int i = 0; i < _targetTypes.size(); i++)
  {
    cvClearMemStorage(_targetTypes[i]->cvStorage);
    CvSeq* _matches = cvHaarDetectObjects( _img, _targetTypes[i]->cvCascade, _targetTypes[i]->cvStorage,
                                      1.1, 
																			2, 
																			CV_HAAR_DO_CANNY_PRUNING,
                                      cvSize(40, 40) );
    for(int j = 0; j < (_matches ? _matches->total : 0); j++ )
    {
      CvRect* r = (CvRect*)cvGetSeqElem(_matches, j);
      tTarget * t = new tTarget();
      t->x = r->x;
      t->y = r->y;
      t->width = r->width;
      t->height = r->height;
      t->type = _targetTypes[i];
      
      t->_time = _time;
      t->_assigned = false;
      _newTargets.push_back(t);
    }
    _nb += _matches->total;
  }

  lock();
  /* Assign new targets to previous targets ... -> targets tracking */
  for (unsigned int i = 0; i < _targets.size(); i++)
  {
    int _minDist = 4000000;
    int _bestMatch = -1;

    tTarget * _curTarget = _targets[i]->_history[0];
    for (unsigned int j = 0; j < _newTargets.size(); j++)
    {
      if ((_newTargets[j]->type == _curTarget->type) && (_newTargets[j]->_assigned == false))
      {
        int _curDist = abs( (_newTargets[j]->x - _curTarget->x)*(_newTargets[j]->x - _curTarget->x) + 
                            (_newTargets[j]->y - _curTarget->y)*(_newTargets[j]->y - _curTarget->y));
        if (_curDist < _minDist)
        {
          _bestMatch = j;
          _minDist = _curDist;          
        }
      }
    }
    if (_bestMatch != -1) /* Target found for track */
    {
      // Assign _newTargets[_bestMatch] to _targets[i]
      tTarget * _match = _newTargets[_bestMatch];
      _newTargets[_bestMatch]->_assigned = true;
      _targets[i]->_history.insert(_targets[i]->_history.begin(), _match);
      _targets[i]->_lastSeen = _time; 
    }
    else
    {
      //cout << "No target for track " << i << " ..." << endl;      
    }
    /* Delete old entries for track */
    if (_targets[i]->_history.size() > _HISTORY_SIZE)
    {
      tTarget * last;
      last = _targets[i]->_history[_HISTORY_SIZE];
      _targets[i]->_history.pop_back();
      delete(last);
    }      
  }

  /* Unassigned targets ar new tracks */
  for (unsigned int i = 0; i < _newTargets.size(); i++)
  {
    if (_newTargets[i]->_assigned == false)
    {
      tTargetTracker * _newTrack = new tTargetTracker();
      _newTrack->_history.push_back(_newTargets[i]);
      _newTrack->_lastSeen = _time;
      _newTrack->type = _newTargets[i]->type;
      _targets.push_back(_newTrack);
    }
  }
  _newTargets.clear();
  unlock();

  return _nb;
}

unsigned int TargetMgr::getMaxHistorySize()
{
  return _HISTORY_SIZE;
}

void * TargetMgr::updateThread(void * pUserData)
{
  IplImage * pImg; 
  long t, total;
  int nb;
  struct tms tt;
  while (this->_exitFlag == false)
  {
    pImg = CamMgr::GetInstance()->GetPicture();
    t = (long) times(&tt);
	  if (pImg)
    {
      nb = detectTargets(pImg);
      total = (times(&tt) - t) * 10;
      cout << nb << " targets found in " << total << "ms" << endl;
		  cvReleaseImage(&pImg);
    }
    else
    {
      //cout << "No pic available for targets" << endl;
    }

    lock();
    bool done = true;
    unsigned int i;
    do
    {
      done = true;
      for (i = 0; i < _targets.size(); i++)
      {
        if ((t - _targets[i]->_history[0]->_time)*10 > TARGET_TIMEOUT)
        {
          _targets.erase(_targets.begin()+i);
          done = false;
          break;
        }
      }
    } while (!done);
    unlock();
    usleep(10);
  }

  return NULL;
}

void TargetMgr::startAnalyse()
{
  pthread_attr_t stAttr;
  pthread_attr_init(&stAttr);

  if (pthread_create(&_thread, &stAttr, _wrapTargetMgrThread, NULL) != 0)
  {
    printf("Can't create target thread !!\n");
    return;
  }
}

void * _wrapTargetMgrThread(void * pUserData)
{
  return TargetMgr::GetInstance()->updateThread(pUserData);
}

unsigned int TargetMgr::getNbTargets()
{
  return _targets.size();
}

tTargetTracker * TargetMgr::getTargetTracker(unsigned int idx)
{
  if (idx >= _targets.size())
    return NULL;
  return _targets[idx];
}


void TargetMgr::lock()
{
  pthread_mutex_lock(&_lockTargets);
}

void TargetMgr::unlock()
{
  pthread_mutex_unlock(&_lockTargets);
}

tTarget * TargetMgr::GetExtrapolation(unsigned int trackId)
{
  return GetExtrapolation(trackId, times(NULL));
}

tTarget * TargetMgr::GetExtrapolation(unsigned int trackId, long time)
{
  tTargetTracker * track;
  if (trackId > _targets.size()) return NULL;

  track = _targets[trackId];
  if (track->_history.size() < 2) return NULL;

  // Create extrapolated target
  tTarget * extrap = new tTarget();
  extrap->_time  = time;
  int x1 = track->_history[0]->x;
  int y1 = track->_history[0]->y;
  int t1 = track->_history[0]->_time;
  int x0 = track->_history[1]->x;
  int y0 = track->_history[1]->y;
  int t0 = track->_history[1]->_time;
  long ex = ((float)((x1 - x0)/(float)(t1 - t0))) * (time - t1) + x1;
  long ey = ((float)((y1 - y0)/(float)(t1 - t0))) * (time - t1) + y1;

  extrap->x      = (int) ex;
  extrap->y      = (int) ey;
  extrap->width  = track->_history[0]->width;
  extrap->height = track->_history[0]->height;

  extrap->type = track->type;
  return extrap;
}

/* EOF */

