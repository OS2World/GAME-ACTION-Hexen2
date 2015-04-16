//**************************************************************************
//** OS/2 port by Jostein Ullestad, 
//** mailto:jullesta@c2i.net
//** http://www.powerutilities.no
//**************************************************************************
#ifndef __OS2_VERSION_JUL__
#define __OS2_VERSION_JUL__

//Make it possible to play the demo version of the hexen.wad file
#define __OS2_DEMO_VERSION_JUL__

#define INCL_WIN
#define INCL_GPI
#define INCL_DOS

#include <os2.h>
//--------- DIVE GFX------
#define  _MEERROR_H_
#include <mmioos2.h>                   /* It is from MMPM toolkit           */
#include <dive.h>
#include <fourcc.h>

#include "setting.h"

extern HDIVE ghDive;
extern ULONG gulimageBuffer;
extern SETUP_BLITTER SetupBlitter;
extern unsigned char *GFXscreenBuffer;
extern HWND ghWnd;
extern BOOL gbH2_MAIN_DONE;

void GFX_init (HWND hWnd, INT w, INT h);
void GFX_done (HWND hWnd);
void GFX_blitter (HWND hWnd);
void GFX_setrgbBlock (BYTE rgb[256][4]);
void GFX_setrgb (INT i, INT r, INT g, INT b);
void GFX_getrgb (INT i, INT *r, INT *g, INT *b);

void GFX_blitterSetup (HWND hWnd);
void resize (HWND hWnd, INT w, INT h);

//------------
MRESULT EXPENTRY ClientWndProc (HWND, ULONG, MPARAM, MPARAM);
extern HAB   hab;
extern HWND  hWndFrame;
extern HWND  hWndClient;
extern HWND  hWndMenu;
extern char  szTitle[64];

#define exitHexenOS2() {WinSendMsg (hWndClient, WM_CLOSE, (MPARAM)0, (MPARAM)0 ); exit(1);}
#define WM_POSTOPEN	WM_USER+200
#define WM_GAME_RUNNING WM_USER+201

void mainHexen(int argc, char **argv); //found in I_IBM.C

#define ID_APPNAME      1
#define IDM_ABOUT       2
#define IDM_EXIT        3
#define IDM_320x200	4
#define IDM_640x480	5
#define IDM_800x600	6
#define IDM_640x400	7
#define IDM_800x500	8
#define IDM_SETTINGS    9
//--- Hexen stubs --------------------------------------------------
#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef enum {false, true} boolean;
typedef unsigned char byte;
#endif

//We have no sound support, yet
//therefor we need to add these stubs
void TSM_DelService (int tsm_ID);
void TSM_Remove (void);
int TSM_NewService (void *, int a, int b, int c);
void TSM_Install(int SND_TICRATE);

void MUS_PauseSong (int handle);
void MUS_ResumeSong (int handle);
void MUS_SetMasterVolume (int volume);
int MUS_RegisterSong (int *data);
int MUS_UnregisterSong (int handle);
int MUS_QrySongPlaying (int handle);
int MUS_StopSong (int handle);
int MUS_ChainSong (int handle, int looping);
int MUS_PlaySong (int handle,int snd_MusicVolume);

void SFX_SetOrigin (int handle, int pitch, int sep, int vol);
void SFX_StopPatch (int handle);
int SFX_PlayPatch (int *data, int pitch, int sep, int vol, int a, int b);
int SFX_Playing (int handle);

int DMX_Init (int SND_TICRATE, int SND_MAXSONGS, int a, int b);
int AL_Detect(int *a, int b);
int MPU_Detect(int *snd_Mport, int *i);
void MPU_SetCard (int snd_Mport);
void DMX_DeInit();
void WAV_PlayMode (int channels, int SND_SAMPLERATE);

void AL_SetCard (int wait, void *a);
int SB_Detect(int *snd_SBport, int *snd_SBirq, int *snd_SBdma, int a);
int GF1_Detect();
void SB_SetCard (int snd_SBport, int snd_SBirq, int snd_SBdma);
void GF1_SetMap (void *a, int size);

#endif //__OS2_VERSION_JUL__
