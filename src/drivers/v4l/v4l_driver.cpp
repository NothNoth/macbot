/*
 *  V4L2 adapter
 **/

#include <stdio.h>
#include <stdlib.h>
#include <cvaux.h>
#include <highgui.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "CamMgr.hpp"
#define CAM_DEVICE CV_CAP_V4L2+0


CvCapture * hCap = NULL;

static IplImage * pImgCache = NULL;
static pthread_t hThread;
pthread_mutex_t hImgLock;
static short bDone = 0;


void * _GrabThread(void * pUserData)
{
	while (!bDone)
	{
		IplImage * pImg;
		pImg = cvQueryFrame(hCap);
    if (pImg) /* On a bien une nouvelle image */
    {
		  pImg = cvCloneImage(pImg); 
		  if (pImg)
		  {
			  pthread_mutex_lock(&hImgLock);
			  cvReleaseImage(&pImgCache);
			  pImgCache = pImg;
			  pthread_mutex_unlock(&hImgLock);
		  }
    }

		usleep(100000);
	}
	return NULL;
}

IplImage * CaptureCb (void * pUserData)
{
	IplImage * pImg;
	if (!pImgCache)
	{
		return NULL;
	}
	pthread_mutex_lock(&hImgLock);
	pImg = cvCloneImage(pImgCache);
	pthread_mutex_unlock(&hImgLock);
	
	return pImg;
}


void DestroyCb(void * pUserData)
{
	bDone = 1;
  pthread_join(hThread, NULL);
	pthread_mutex_destroy(&hImgLock);
	cvReleaseCapture(&hCap); 
	hCap = NULL;
}

extern "C" int InitDriver()
{

  pthread_attr_t stAttr;
	pthread_mutexattr_t hMut;
	hCap =  cvCreateCameraCapture(CAM_DEVICE);
	printf("Init camera...\n");


	if (!hCap)
	{
		printf("Initializing camera failed !\n");
		return -1;
	}
  
  pthread_attr_init(&stAttr);
  if (pthread_create(&hThread, &stAttr, _GrabThread, NULL) != 0)
  {
    printf("Can't create grab thread !!\n");
    cvReleaseCapture(&hCap); 
    return -1;
  }
	pthread_mutexattr_init(&hMut);
	pthread_mutex_init (&hImgLock, &hMut);
  
  CamMgr::GetInstance()->Register((unsigned int)cvGetCaptureProperty(hCap, CV_CAP_PROP_FRAME_WIDTH), 
																	(unsigned int)cvGetCaptureProperty(hCap, CV_CAP_PROP_FRAME_HEIGHT),
                                  CaptureCb,
                                  DestroyCb,
                                  NULL);
  return 0;
}

/* End of File */

