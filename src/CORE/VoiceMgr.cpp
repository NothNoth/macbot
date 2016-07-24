/*
 *      Voice_mgr.cpp
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

#include "VoiceMgr.hpp"
VoiceMgr * VoiceMgr::_VoiceMgr = NULL;
void * _wrapVoiceThread(void * pUserData);

#ifndef _MACOS
extern "C" {
cst_voice *register_cmu_us_kal();
}
#endif

VoiceMgr::VoiceMgr()
{
  pthread_mutexattr_t hMut;
  printf("Starting Voice manager...\n");
	pthread_mutexattr_init(&hMut);
	pthread_mutex_init (&_lockSpool, &hMut);
  _exitFlag = 0;
#ifndef _MACOS
  flite_init();
  _voice = register_cmu_us_kal();
#endif
  _isSpeaking = false;
}

VoiceMgr::~VoiceMgr()
{
  _exitFlag = 1;
  pthread_join(_thread, NULL);
	pthread_mutex_destroy(&_lockSpool);
}



void VoiceMgr::say(string _text)
{
  pthread_mutex_lock(&_lockSpool);
  _spool.push_back(_text);
  pthread_mutex_unlock(&_lockSpool);
}

void * VoiceMgr::speakThread(void * pUserData)
{
  while (!_exitFlag)
  {
    string _text;
    pthread_mutex_lock(&_lockSpool);
    if (!_spool.empty())
    {
      _text = _spool.back();
      _isSpeaking = true;
#ifndef _MACOS
      if (!_mute)  flite_text_to_speech(_text.c_str(), _voice ,"play");
#endif
      _isSpeaking = false;
      _spool.pop_back();
    }
    pthread_mutex_unlock(&_lockSpool);
    usleep(100);
  }
  return NULL;
}

bool VoiceMgr::isSpeaking()
{
  return _isSpeaking;
}


void VoiceMgr::startSpeaking()
{
  pthread_attr_t stAttr;
  pthread_attr_init(&stAttr);

  if (pthread_create(&_thread, &stAttr, _wrapVoiceThread, NULL) != 0)
  {
    printf("Can't create voice thread !!\n");
    return;
  }
}

void * _wrapVoiceThread(void * pUserData)
{
  return VoiceMgr::GetInstance()->speakThread(pUserData);
}

void VoiceMgr::mute(bool _mute)
{
  this->_mute = _mute;
}
/* EOF */

