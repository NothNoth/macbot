#include <iostream>
#include <SDL.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close */
#include <signal.h>
#include <zlib.h>
#include <SDL_gfxPrimitives.h>
using namespace std;



short bDone = 0;
SDL_Surface * currentSurf = NULL;
pthread_mutex_t    _lockSurf;
int sock = -1;
float loaded = 0.0;
bool verbose;
void SigHandler(int iSignal)
{
  bDone = 1;
  printf("Sending exit signal to main loop\n");
}



bool sendCommand(const char * cmd)
{
  char tmp[16];
  memset(tmp, 0x00, 16);
  strncpy(tmp, cmd, 15); // tronque a 15o
  if (verbose) cout << "Sending " << tmp << endl;
  if (send(sock, tmp, 16*sizeof(char), 0) != 16)
  {
    cout << "Error while writing command : " << tmp << endl;
    return false;
  }
  return true;;
}

void * _updateThread(void * pUserData)
{
  char ** args = (char **) pUserData; 
  struct sockaddr_in localAddr, servAddr;
  struct hostent * host;
  int remoteSock = -1;
  SDL_Surface * loadedSurf, * tmpSurf;

  cout << "Connecting to " << args[1] << ":" << args[2] << endl;
  host = gethostbyname(args[1]);
  if(host == NULL) 
  {
    cout << "Host unknown" << endl;
    return NULL;
  }
  
  servAddr.sin_family = host->h_addrtype;
  memcpy((char *) &servAddr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
  servAddr.sin_port = htons(atoi(args[2]));

   /* create socket */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0) 
  {
    cout << "Can't open socket" << endl;
    return NULL;
  }
  if (verbose) cout << "Socket OK" << endl;
  /* bind any port number */
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(0);
   
  remoteSock = bind(sock, (struct sockaddr *) &localAddr, sizeof(localAddr));
  if(remoteSock < 0) 
  {
    cout << "Can't bind" << endl;
    if (sock != -1)
      close(sock); 
    return NULL;
  }
  if (verbose) cout << "Bind OK" << endl;          
   /* connect to server */
  remoteSock = connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr));
  if(remoteSock < 0) 
  {
    cout << "Can't connect to remote host !" << endl;
    if (remoteSock != -1)
      close(remoteSock);
    if (sock != -1)
      close(sock); 
    return NULL;
  }
  if (verbose) cout << "Connect OK" << endl;
  while (!bDone)
  {
    int sizeRead = 0;
    int decompsize = -1;
    int compsize;
    int w;
    int h;
    char * compbuf, * decompbuf, * currentDecompBuf = NULL;
    int header[4];

    /* Read data size ... */
    sizeRead = recv(sock, header, 4*sizeof(int), 0);
    if (sizeRead != 0)
    {
      if (sizeRead != 4*sizeof(int))
      {
        cout << "Error while reading img header : " << sizeRead << endl;
        break;
      }
      decompsize = header[0];
      compsize = header[1];
      w = header[2];
      h = header[3];
      if (verbose) cout << "Reading " << compsize << " bytes for image " << w << "x" << h << "..." << endl;
      compbuf = (char *)malloc(compsize);
      sizeRead = 0;
      do
      {
        int n;
        n = recv(sock, compbuf+sizeRead, compsize-sizeRead<512?compsize-sizeRead:512, 0);
        if (n <= 0) break;
        sizeRead += n;
        loaded = sizeRead*100.0/compsize;
      }
      while (sizeRead < compsize);
      if (sizeRead != compsize)
      {
        cout << "Error while reading block data " << sizeRead << "/" << compsize << " bytes read." << endl;
        free(compbuf);
        break;
      }
      decompbuf = (char *) malloc(decompsize); 
      if (uncompress((Bytef*)decompbuf, (uLong *) &decompsize, (Bytef *) compbuf,  (uLong) compsize) != Z_OK)
        cout << "Can't decompress" << endl;
      //RGBA
	    loadedSurf = SDL_CreateRGBSurfaceFrom((void*) decompbuf,
																					    w,
																					    h,
																					    4*8,
																					    w*4,
                                              0x00FF0000,
                                              0x0000FF00,
                                              0x000000FF,
                                              0x00000000);

      if (loadedSurf)
      {
        pthread_mutex_lock(&_lockSurf);
        tmpSurf = currentSurf;
        currentSurf = loadedSurf;
        if (tmpSurf) SDL_FreeSurface(tmpSurf);
        if (currentDecompBuf) free(currentDecompBuf);
        currentDecompBuf = decompbuf;
        pthread_mutex_unlock(&_lockSurf);
        free(compbuf);
      }
    }
  }  

  if (remoteSock != -1)
    close(remoteSock);
  if (sock != -1)
    close(sock); 
  if (verbose) cout << "Socket thread stopped." << endl;
  return NULL;
}


int main(int argc, char * argv[])
{
  bool isMovingFwd = false;
  bool isMovingBwd = false;
  SDL_Surface * mainLayer;
  pthread_t          _thread;
  pthread_attr_t stAttr;
  pthread_mutexattr_t hMut;
  unsigned int color          = 0xFFFFFF88;
  unsigned int colorSelected  = 0x0000FF88;
      
  SDL_Surface * hScreen;
  SDL_Rect OSD_back;
  SDL_Rect OSD_up;
  SDL_Rect OSD_down;
  SDL_Rect OSD_left;
  SDL_Rect OSD_right;
  SDL_Rect OSD_stop;
  OSD_back.x  = 10;
  OSD_back.y  = 320;
  OSD_back.w  = 70;
  OSD_back.h  = 70;
  OSD_up.x    = OSD_back.x + OSD_back.w/2 - 10;
  OSD_up.y    = OSD_back.y + 10;
  OSD_up.w    = 20;
  OSD_up.h    = 10;
  OSD_down.x  = OSD_back.x + OSD_back.w/2 - 10;
  OSD_down.y  = OSD_back.y + OSD_back.h-20;
  OSD_down.w  = 20;
  OSD_down.h  = 10;  
  OSD_left.x  = OSD_back.x + 10;
  OSD_left.y  = OSD_back.y + OSD_back.h/2 - 10;
  OSD_left.w  = 10;
  OSD_left.h  = 20;  
  OSD_right.x = OSD_back.x + OSD_back.w - 20;
  OSD_right.y = OSD_back.y + OSD_back.h/2 - 10;
  OSD_right.w = 10;
  OSD_right.h = 20;
  OSD_stop.x = OSD_back.x + OSD_back.w/2 - 5;
  OSD_stop.y = OSD_back.y + OSD_back.h/2 - 5;
  OSD_stop.w = 10;
  OSD_stop.h = 10;  
  if (argc < 3)
  {
    cout << "Usage %s <HOST> <PORT> [-v]" << endl;
    return -1;
  }
  if ((argc == 4)&&(!strcmp(argv[3], "-v")))
    verbose = true;

  signal(2, SigHandler);
	pthread_mutexattr_init(&hMut);
	pthread_mutex_init (&_lockSurf, &hMut);
  pthread_attr_init(&stAttr);



  if (pthread_create(&_thread, &stAttr, _updateThread, (void *) argv) != 0)
  {
    printf("Can't create update thread !!\n");
    goto done;
  }

  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE) == -1)
  {
    printf("SDL Init failed\n");
    goto done;
  }
  if (SDL_VideoInit(NULL, 0) == -1)
  {
    printf("SDL VideoInit failed\n");
    goto done;
  }

  SDL_SetVideoMode(640, 480, 32, SDL_DOUBLEBUF|SDL_HWSURFACE);
  mainLayer = SDL_GetVideoSurface();
  hScreen = SDL_CreateRGBSurface(SDL_HWSURFACE, mainLayer->w, mainLayer->h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
  SDL_FillRect(hScreen, NULL, 0xFF000000);
  SDL_BlitSurface(hScreen, NULL, mainLayer, NULL);
  SDL_Flip(mainLayer);
  if (verbose) cout << "Ihm ready..." << endl;
  while (!bDone)
  {
    SDL_Event hEvent;
    SDL_Rect lo;
    /* Check Events*/
    SDL_PumpEvents();
    while (SDL_PollEvent(&hEvent))
    {
      if (hEvent.type == SDL_QUIT)
        goto done;

      else if (hEvent.type == SDL_MOUSEBUTTONDOWN)
      {
        if ((hEvent.button.x > OSD_up.x) && (hEvent.button.x < OSD_up.x + OSD_up.w) &&
            (hEvent.button.y > OSD_up.y) && (hEvent.button.y < OSD_up.y + OSD_up.h))
        {
          isMovingBwd = false;
          if (isMovingFwd == false)
          {
            isMovingFwd = true;
            sendCommand("FORWARD");
          }
          else
          {
            isMovingFwd = false;
            sendCommand("STOP");
          }
        }
        else if ((hEvent.button.x > OSD_down.x) && (hEvent.button.x < OSD_down.x + OSD_down.w) &&
            (hEvent.button.y > OSD_down.y) && (hEvent.button.y < OSD_down.y + OSD_down.h))
        {
          isMovingFwd = false;
          if (isMovingBwd == false)
          {
            isMovingBwd = true;
            sendCommand("BACKWARD");
          }
          else
          {
            isMovingBwd = false;
            sendCommand("STOP");
          }
        }
        else if ((hEvent.button.x > OSD_left.x) && (hEvent.button.x < OSD_left.x + OSD_left.w) &&
            (hEvent.button.y > OSD_left.y) && (hEvent.button.y < OSD_left.y + OSD_left.h))
        {
          if (isMovingFwd || isMovingBwd)
          {
            sendCommand("STOP");
            isMovingFwd = false;
            isMovingBwd = false; 
          } 
          sendCommand("LEFT"); 
        }
        else if ((hEvent.button.x > OSD_right.x) && (hEvent.button.x < OSD_right.x + OSD_right.w) &&
            (hEvent.button.y > OSD_right.y) && (hEvent.button.y < OSD_right.y + OSD_right.h))
        {
          if (isMovingFwd || isMovingBwd)
          {
            sendCommand("STOP");
            isMovingFwd = false;
            isMovingBwd = false; 
          } 
          sendCommand("RIGHT");
        }
        else if ((hEvent.button.x > OSD_stop.x) && (hEvent.button.x < OSD_stop.x + OSD_stop.w) &&
            (hEvent.button.y > OSD_stop.y) && (hEvent.button.y < OSD_stop.y + OSD_stop.h))
        {
          sendCommand("STOP");
          isMovingFwd = false;
          isMovingBwd = false; 
        }
      }
    }

    
    pthread_mutex_lock(&_lockSurf);
    if (currentSurf)
    {
      SDL_BlitSurface(currentSurf, NULL, hScreen, NULL);
      //--- OSD ---
      //up
      filledTrigonColor(hScreen, 
                        OSD_up.x+OSD_up.w/2, OSD_up.y, 
                        OSD_up.x, OSD_up.y+OSD_up.h, 
                        OSD_up.x+OSD_up.w, OSD_up.y+OSD_up.h, 
                        isMovingFwd?colorSelected:color);
      //down
      filledTrigonColor(hScreen, 
                        OSD_down.x+OSD_down.w/2,  OSD_down.y + OSD_down.h, 
                        OSD_down.x,               OSD_down.y, 
                        OSD_down.x+OSD_down.w,    OSD_down.y, 
                        isMovingBwd?colorSelected:color);
      //left
      filledTrigonColor(hScreen, 
                        OSD_left.x, OSD_left.y+OSD_left.h/2, 
                        OSD_left.x + OSD_left.w, OSD_left.y, 
                        OSD_left.x + OSD_left.w, OSD_left.y+OSD_left.h, 
                        color);
      //right
      filledTrigonColor(hScreen, 
                        OSD_right.x+OSD_right.w,  OSD_right.y+OSD_right.h/2, 
                        OSD_right.x,              OSD_right.y, 
                        OSD_right.x,              OSD_right.y+OSD_right.h, 
                        color);     
      //stop
      SDL_FillRect(hScreen, &OSD_stop, color);
    }
    pthread_mutex_unlock(&_lockSurf);

    lo.x = 440;
    lo.y = 400;
    lo.w = (int)loaded;
    lo.h = 10;
    rectangleColor(hScreen, 440, 400, 540, 410, 0x88FFFFFF);
    SDL_FillRect(hScreen, &lo, color);
    stringColor(hScreen, lo.x+2, lo.y+2, "Loading...", 0x880000FF);
    SDL_BlitSurface(hScreen, NULL, mainLayer, NULL);
    SDL_Flip(mainLayer);


    usleep(10);
  }

done:
  bDone = true;
  pthread_join(_thread, NULL);
	pthread_mutex_destroy(&_lockSurf);
  SDL_FreeSurface(hScreen);
  SDL_Quit();
  return 0;
}

/* EOF */

