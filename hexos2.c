//**************************************************************************
//** OS/2 port by Jostein Ullestad, 
//** mailto:jullesta@c2i.net
//** http://www.powerutilities.no
//**
//** Compiling with Watcom 11.0 with the following switches
//** -w4       Warning level 4
//** -e25      Error count 25
//** -j        Change char default to signed
//** -ei       Force enums to be type int
//** -zp1      1 byte alignement
//** -zq       Quiet operation
//** -od       No optimisation //-otexan   Fastest possible code
//** -d1       Line number information
//** -bm       Multithreaded application
//** -4r       80486 Register based calling
//** -bt=os2   Target OS/2
//** -mf       32bit Flat model
//**
//**
//**************************************************************************
/*
   Original options
   class
   net
   record
   playdemo
   timedemo
   loadgame
   warp
   file
   debugfile
   recordfrom
   devsnd
   nomouse
   nojoy
   novideo
   externdriver
   nosound
   nosfx
   nomusic
   debug
   config
   timer

   New OS/2 Options
   fps
   nomousecapture
   framespeed
   nopause
   novram
*/
#include "hexos2.h"
#include "st_start.h"
#include "tmr0_ioc.h"
#include "joyos2.h"
#include "i_sound.h"

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

//#define DEBUG_OS2

//These are missing in Watcom 11.0
#define WM_VRNDISABLED             0x007e
#define WM_VRNENABLED              0x007f
/*********************************************************************/
/* mciDriverNotify  Message Types                                    */
/*********************************************************************/
#define MM_MCINOTIFY                        0x0500
#define MM_MCIPASSDEVICE                    0x0501
#define MM_MCIPOSITIONCHANGE                0x0502
#define MM_MCICUEPOINT                      0x0503
#define MM_MCIPLAYLISTMESSAGE               0x0504
#define MM_MCIEVENT                         0x0505
#define MM_MCISYNCH                         0x0506

#define MCI_LOSING_USE                      0x00000001L
#define MCI_GAINING_USE                     0x00000002L
/*********************************************************************/
/* Flags for mciDriverNotify                                         */
/*********************************************************************/
#define MCI_NOTIFY_SUCCESSFUL               0x0000
#define MCI_NOTIFY_SUPERSEDED               0x0001
#define MCI_NOTIFY_ABORTED                  0x0002
#define MCI_NOTIFY_ERROR                    0x0003

#define MCI_ACQUIREDEVICE               23
#define MCI_ACQUIRE_QUEUE                   0x00000400L

BOOL APIENTRY WinSetVisibleRegionNotify (HWND hwnd, BOOL fEnable);
ULONG APIENTRY WinQueryVisibleRegion( HWND hwnd, HRGN hrgn);


//*************************************************************************
// Global variables
//*************************************************************************
HAB   hab;
HWND  hWndFrame,
      hWndClient,
      hWndMenu;
char  szTitle[64];
int   gargc;      //Hexen code makes use of this
char  **gargv;    //Hexen code makes use of this

BOOL gbGamePlaying;

void clearBackGround (HPS hps, long x, long y, long cx, long cy, ULONG color);
BOOL gbCleanFromHexen = TRUE;
void cleanUp ();
void displaySetting (HWND hwnd);
PWINDATA  pwindata = NULL;             /* Pointer to window data               */

HMQ   hmq;
QMSG  qmsg;
//*************************************************************************
// The start up function
//*************************************************************************
int main (int argc, char **argv)
{
   ULONG flFrameFlags   = FCF_TITLEBAR | FCF_SYSMENU | FCF_ICON | FCF_DLGBORDER | 
                          FCF_MENU     | FCF_TASKLIST| FCF_SHELLPOSITION;
   LONG WIDTH = 640, HEIGHT = 480;
   CHAR szClientClass[] = "HEXEN_CLIENT";
   ULONG ulFlag = SWP_SIZE | SWP_MOVE | SWP_ACTIVATE | SWP_SHOW;
   HPS hPS;


   onexit (cleanUp); //Hexen is a DOS game, we need to clean up when exiting
   hab = WinInitialize (0);
   hmq = WinCreateMsgQueue (hab, 0);

   WinRegisterClass (hab, (PSZ)szClientClass, (PFNWP)ClientWndProc, CS_MOVENOTIFY, 0);
   WinLoadString (hab, 0, ID_APPNAME, sizeof(szTitle), (PSZ)szTitle);

   // Allocate a buffer for the window data
   pwindata = (PWINDATA) malloc (sizeof(WINDATA));
   pmData = pwindata;
   memset (pwindata, 0, sizeof(WINDATA));
   pwindata->hab            = hab;
   pwindata->ulToEnd        = 0;
   pwindata->ulWidth        = WIDTH;
   pwindata->ulHeight       = HEIGHT;
   pwindata->fChgSrcPalette = FALSE;
   pwindata->fStartBlit     = FALSE;
   pwindata->fDataInProcess = FALSE;

   pwindata->hwndFrame = WinCreateStdWindow (HWND_DESKTOP, WS_VISIBLE,
                  &flFrameFlags, (PSZ)szClientClass, (PSZ)szTitle, 0, 0,
                  ID_APPNAME, &(pwindata->hwndClient));
   WinSetWindowULong (pwindata->hwndClient, 0, (ULONG)pwindata);
  
   hWndFrame = pwindata->hwndFrame;
   hWndClient= pwindata->hwndClient;

   WinSetWindowPos (hWndFrame, HWND_TOP, 0, 0, WIDTH, HEIGHT, ulFlag);
   resize (hWndClient, WIDTH, HEIGHT);

   hPS = WinGetPS (hWndClient);
   clearBackGround (hPS, 0, 0, WIDTH, HEIGHT, CLR_BLACK);
   WinReleasePS (hPS);

   WinSetVisibleRegionNotify (hWndClient, TRUE);
   // show main window
   WinShowWindow( hWndFrame, TRUE);

   GFX_init (hWndClient, WIDTH, HEIGHT);
   gargc = argc;
   gargv = argv;

   while (WinGetMsg (hab, &qmsg, 0, 0, 0))
      WinDispatchMsg (hab, &qmsg);

   gbCleanFromHexen = FALSE;
   cleanUp ();
   WinDestroyWindow (hWndFrame);
   WinDestroyMsgQueue (hmq);
   WinTerminate (hab);
   return (0);
}

//*************************************************************************
// Show about message
//*************************************************************************
void about (HWND hWnd)
{
   CHAR cBuffText[1024];
   CHAR cBuffTitle[] = "Product information";

   strcpy (cBuffText, "Hexen for OS/2 Warp\n\n");

   strcat (cBuffText, "Version 0.14\n\n");

   strcat (cBuffText, "Ported to OS/2 Warp by\n");
   strcat (cBuffText, "              Jostein Ullestad\n\n");

   strcat (cBuffText, "http://www.powerutilities.no\n");
   strcat (cBuffText, "mailto:jostein@powerutilities.no\n\n");

   strcat (cBuffText, "Based on the DOS source\n");
   strcat (cBuffText, "code by Activision and Raven\n\n");

   strcat (cBuffText, "Credits:\n");
   strcat (cBuffText, "Craig D. Miller for the logo\n");
   strcat (cBuffText, "Martin Amodeo for the SFX code\n");
   strcat (cBuffText, "Yuri Dario for DOOM MIDI code\n");
   strcat (cBuffText, "Darrell Spice Jr. for the joystick code\n");

//   strcat (cBuffText, "for more information goto\n");
//   strcat (cBuffText, "http://www.hexenworld.com");
   WinMessageBox (HWND_DESKTOP, hWnd, (PSZ)&cBuffText[0], (PSZ)&cBuffTitle[0], 0, MB_INFORMATION | MB_OK | MB_MOVEABLE);
}

//*************************************************************************
//
//*************************************************************************
void clearBackGround (HPS hps, long x, long y, long cx, long cy, ULONG color)
{
   POINTL   ptlPoint;

   //Clear background
   GpiSetColor (hps, color);
   ptlPoint.x = x; ptlPoint.y = y;
   GpiMove (hps, &ptlPoint);
   ptlPoint.x = cx; ptlPoint.y = cy;
   GpiBox (hps, DRO_OUTLINEFILL, &ptlPoint, 0, 0);
}

//*************************************************************************
void mainHexen(int argc, char **argv); //Hexen function
void H2_GameRealLoop (void);           //Hexen function
// Found in i_ibm.c ----
void I_KeyboardISR (ULONG *msg);
void I_ReadMouse (SHORT x, SHORT y);

BOOL  bTreadGame;
BOOL  bLostFocus   = FALSE;
BOOL  bPause       = FALSE;
BOOL  bProcessingDLG = FALSE;
BOOL  gbJoystickOn = FALSE;
ULONG ulOpenFlag = OPEN_ACTION_OPEN_IF_EXISTS;
ULONG ulOpenMode = OPEN_FLAGS_FAIL_ON_ERROR | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE;
HMTX  hmtxDive;
TID   tidGame = 0;
ULONG gulFPS = 0; //Count Frames Per Sec.
BOOL  gbShowFPS = FALSE;         //-fps parameter
BOOL  gbPauseOnFocusLost = TRUE; //-nopause parameter
BOOL  gbNoNouseCapture = FALSE;  //-nomousecapture parameter
INT   giFrameSpeed = 35;         //-framespeed parameter
BOOL  gbNoVRAM = FALSE;          //-novram parameter

INT giSrcW, giSrcH, giSrcWxgiSrcH;

BOOL  fVrnDisabled = TRUE;

#define INI_SECTION (PSZ)"options"
#define INI_FILE    "hexos2.ini"
void readINIfile (CHAR file[]);

//*************************************************************************
// The game thread, does all the action and painting
//*************************************************************************
void __stdcall hexenGameThread (LONG pInfo)
{
   HFILE timer = NULL;
   ULONG action;
   ULONG size = sizeof(ULONG);
   ULONG ulThreadGameSpeed = 5;

   HMQ   hmqThread;
   HAB   habThread;

   //Since we are going to show WinMessageBox from thread
   //it needs a messagequeue.
   habThread = WinInitialize (0L);
   hmqThread = WinCreateMsgQueue (habThread, 0L);

   if (DosOpen ((PSZ)"TIMER0$  ", &timer, &action, 0, 0, ulOpenFlag, ulOpenMode, NULL))
      timer = NULL;

   while (bTreadGame)
   {
      if (!bPause && !bLostFocus && !bProcessingDLG)
      {
         if (DosRequestMutexSem (hmtxDive, SEM_INDEFINITE_WAIT) == 0)
         {
            H2_GameRealLoop ();
            DosReleaseMutexSem (hmtxDive);
         }
      }
      if (timer == (HFILE)NULL)
         DosSleep (ulThreadGameSpeed);
      else
         DosDevIOCtl(timer, HRT_IOCTL_CATEGORY, HRT_BLOCKUNTIL, &ulThreadGameSpeed, size, &size, NULL, 0, NULL);
   }
   DosClose(timer);

   WinDestroyMsgQueue (hmqThread);
   WinTerminate (habThread);
   DosExit (EXIT_THREAD, 0);
}

//*************************************************************************
// Handles the OS/2 PM messages
//*************************************************************************
MRESULT EXPENTRY ClientWndProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   BOOL        bHandled = TRUE;
   MRESULT     mReturn = 0;
   PWINDATA    this;              /* Pointer to window data               */
   static BOOL bPostOpenRan = FALSE;   //Do not capture the mouse to early
#ifdef DEBUG_OS2
   ULONG       ulTimer;
#endif

   // Get the pointer to window data
   this = (PWINDATA)WinQueryWindowULong (hWnd, 0);

   switch (msg)
   {
      #ifdef DEBUG_OS2
      case WM_TIMER:
         if (!bLostFocus && !bPause && !bProcessingDLG)
         {
            if (DosRequestMutexSem (hmtxDive, SEM_INDEFINITE_WAIT) == 0)
            {
               H2_GameRealLoop ();
               DosReleaseMutexSem (hmtxDive);
            }
         }
         break;
      #endif
      case WM_CREATE:
      {
         HFILE timer;
         ULONG action;

         bHandled = FALSE;
         DosCreateMutexSem (NULL, &hmtxDive, 0L, FALSE);
         //Just test the timer0
         if (DosOpen ((PSZ)"TIMER0$  ", &timer, &action, 0, 0, ulOpenFlag, ulOpenMode, NULL))
         {
            ST_Message ("Could not open TIMER0 device driver.");
            WinPostMsg (hWnd, WM_CLOSE, (MPARAM)0, (MPARAM)0);
         }
         else
         {
            DosClose(timer);
            WinPostMsg (hWnd, WM_VRNENABLED,0,0);
            WinPostMsg (hWnd, WM_POSTOPEN, 0, 0);
         }
      }
         break;
      case WM_POSTOPEN:
         if (DosRequestMutexSem (hmtxDive, SEM_INDEFINITE_WAIT) == 0)
         {
            mainHexen (gargc, gargv);
            readINIfile (INI_FILE);
            DosReleaseMutexSem (hmtxDive);
         }
         bTreadGame = TRUE;
         #ifdef DEBUG_OS2
         ulTimer = WinStartTimer (hab, hWnd, 0, 5UL);
         #else
         DosCreateThread (&tidGame, (PFNTHREAD)hexenGameThread, 0, 0, 1024*40);
         #endif
         bPostOpenRan = TRUE;
      break;

      case WM_GAME_RUNNING:
         if (gbNoNouseCapture != TRUE)
         {
            if ((BOOL)mp1)
               WinSetCapture (HWND_DESKTOP, hWnd);
            else
               WinSetCapture (HWND_DESKTOP, NULLHANDLE);
         }
      break;

      case WM_CLOSE:
         #ifdef DEBUG_OS2
         WinStopTimer (hab , hWnd, ulTimer);
         #endif
         WinSetCapture (HWND_DESKTOP, NULLHANDLE);
         bHandled = FALSE;
         break;

      case WM_CHAR:
         I_KeyboardISR (&msg);
         break;

      case WM_TRANSLATEACCEL:
         if (bPause)
            bHandled = FALSE;
         break;

      case WM_MOUSEMOVE:
      {
         SHORT x, y;
         RECTL rectl;

         WinQueryWindowRect (hWnd, &rectl);
         x = SHORT1FROMMP(mp1);
         y = SHORT2FROMMP(mp1);

         x = 3 * ((x * 100) / (rectl.xRight - rectl.xLeft));
         y = 2 * ((y * 100) / (rectl.yTop   - rectl.yBottom));
         I_ReadMouse (x-160, y-100);

         bHandled = FALSE;
      }
         break;

      case WM_FOCUSCHANGE:
      {
         static BOOL bMouseCapture = FALSE;

         bHandled = FALSE;
         if ((USHORT)mp2)  //Getting the focus
         {
            if (bPostOpenRan && bMouseCapture) WinSendMsg (hWnd, WM_GAME_RUNNING, (MPARAM)TRUE, (MPARAM)0);
            bLostFocus = FALSE;
         }
         else  //Loosing focus
         {
            bMouseCapture = (BOOL) (hWnd == WinQueryCapture (HWND_DESKTOP));
            if (bPostOpenRan) WinSendMsg (hWnd, WM_GAME_RUNNING, FALSE, 0);
            bLostFocus = TRUE;
         }
         if (!gbPauseOnFocusLost)
            bLostFocus = FALSE;
         break;
      }
/*
      case WM_MOVE:
         if (DosRequestMutexSem (hmtxDive, SEM_INDEFINITE_WAIT) == 0)
         {
            GFX_blitterSetup (hWnd);
            DosReleaseMutexSem (hmtxDive);
         }
         bHandled = FALSE;
         break;
*/
         case WM_COMMAND:
         {
            static BOOL bMouseCpt = FALSE;

            bMouseCpt = (BOOL) (hWnd == WinQueryCapture (HWND_DESKTOP));
            if (bPostOpenRan) WinSendMsg (hWnd, WM_GAME_RUNNING, FALSE, 0);
            bProcessingDLG = TRUE;

            switch ((ULONG)mp1)
            {
            case IDM_EXIT:
               WinPostMsg( hWnd, WM_CLOSE, (MPARAM)0, (MPARAM)0 );
               break;
            case IDM_ABOUT:
               about (hWnd);
               break;
            case IDM_SETTINGS:
               displaySetting (hWnd);
               break;

            case IDM_320x200:
               resize (hWnd, 320, 200);
               break;
            case IDM_640x400:
               resize (hWnd, 640, 400);
               break;
            case IDM_640x480:
               resize (hWnd, 640, 480);
               break;
            case IDM_800x500:
               resize (hWnd, 800, 500);
               break;
            case IDM_800x600:
               resize (hWnd, 800, 600);
               break;
            }
            bHandled = FALSE;
            if (bPostOpenRan && bMouseCpt)
               WinPostMsg (hWnd, WM_GAME_RUNNING, (MPARAM)TRUE, (MPARAM)0);
            bProcessingDLG = FALSE;
         }
         break;

      case WM_ERASEBACKGROUND:
         if (bPause || bLostFocus || bProcessingDLG)
         {
            long x, y, cx, cy;
            RECTL *pr;

            pr = (PRECTL)mp2;
            x = pr->xLeft;
            y  = pr->yBottom;
            cx= pr->xRight;// - x;
            cy= pr->yTop;// - y;
            clearBackGround ((HPS)mp1, x, y, cx, cy, CLR_BLACK);
         }
         bHandled = FALSE;
      break;

      case WM_VRNDISABLED:
            bPause = TRUE;
            DiveSetupBlitter (ghDive, 0 );
            fVrnDisabled = TRUE;
            bPause = FALSE;
      break;

      case WM_VRNENABLED:
            bPause = TRUE;
            GFX_blitterSetup (hWnd);
            fVrnDisabled = FALSE;
            bPause = FALSE;
      break;

      default:
         bHandled = FALSE;
      break;
   }

   if (!bHandled)
      mReturn = WinDefWindowProc (hWnd, msg, mp1, mp2);

  return mReturn;

}


//*************************************************************************
//--------- DIVE GFX------
SETUP_BLITTER SetupBlitter;
HDIVE         ghDive = 0;
ULONG         gulimageBuffer;
PBYTE         GFXscreenBuffer;
HWND          gHwnd;
BOOL          gbDirectVRAM = FALSE;
//*************************************************************************
//
//*************************************************************************
void GFX_init (HWND hWnd, INT W, INT H)
{
   ULONG rc;
   FOURCC    fccFormats[100] = {0};        // Color format code
   DIVE_CAPS DiveCaps;


   ST_Message ("Looking for DIVE");
   // Get the screen capabilities, and if the system support only 16 colors
   // the sample should be terminated.
   DiveCaps.pFormatData = fccFormats;
   DiveCaps.ulFormatLength = 120;
   DiveCaps.ulStructLen = sizeof(DIVE_CAPS);

   DiveQueryCaps ( &DiveCaps, DIVE_BUFFER_SCREEN );

   if (DiveCaps.ulDepth < 8)
   {
      WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
          (PSZ)"The program can not run on this system environment.",
          (PSZ)"Note", 0, MB_OK | MB_INFORMATION  | MB_MOVEABLE);
      return;
   }

   gbDirectVRAM = DiveCaps.fScreenDirect;
   if (gbDirectVRAM && !gbNoVRAM)
      ST_Message ("System allows direct screen access (VRAM).");
   else
      ST_Message ("System does not allows direct screen access (VRAM).");

   ST_Message ("Trying to open DIVE");
   // Get an instance of DIVE APIs.
   rc = DiveOpen (&ghDive, FALSE, 0);
   if ( rc != DIVE_SUCCESS)
   {
      WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
               (PSZ)"Unable to open a DIVE instance.",
               (PSZ)"Error", 0, MB_OK | MB_INFORMATION  | MB_MOVEABLE);
      return;
   }

   gHwnd = hWnd;
   giSrcW = W;
   giSrcH = H;
   giSrcWxgiSrcH = giSrcW * giSrcH; //Just to speed ut up

   ST_Message ("Allocating DIVE memory");
   DosAllocMem((PPVOID)&GFXscreenBuffer, giSrcWxgiSrcH, PAG_COMMIT | PAG_EXECUTE | PAG_READ | PAG_WRITE);
   memset (GFXscreenBuffer, 0, giSrcWxgiSrcH);
   gulimageBuffer = 0;

   if (gbDirectVRAM)
      rc = DiveAllocImageBuffer (ghDive, &gulimageBuffer, FOURCC_LUT8, giSrcW, giSrcH, 0, 0);
   else
      rc = DiveAllocImageBuffer (ghDive, &gulimageBuffer, FOURCC_LUT8, giSrcW, giSrcH, 0, GFXscreenBuffer);

   if (rc == DIVE_SUCCESS)
      ST_Message ("DIVE ok");
   else
      ST_Message ("Unable to allocate Image DIVE buffer");

   GFX_blitterSetup (hWnd);
}

//*************************************************************************
//
//*************************************************************************
void GFX_blitterSetup (HWND hWnd)
{
   SWP      swp;//, swpParent;
//   HWND     hWndParent;
   RECTL    rcl;
   ULONG    rc;
   HRGN     hrgn;
   HPS      hps;
   RECTL    rctls[50];
   RGNRECT  rgnCtl;
   POINTL   pointl;


   if (!ghDive) return;

   hps = WinGetPS (hWnd);
   if (!hps) return;

   hrgn = GpiCreateRegion (hps, 0, NULL);
   if (!hrgn)
   {
      WinReleasePS (hps);
      return;
   }

   WinQueryVisibleRegion (hWnd, hrgn);
   rgnCtl.ircStart = 0;
   rgnCtl.crc = 50;
   rgnCtl.ulDirection = RECTDIR_LFRT_TOPBOT;

   GpiQueryRegionRects (hps, hrgn, NULL, &rgnCtl, rctls);
   WinQueryWindowPos (hWnd, &swp);

   pointl.x = swp.x;
   pointl.y = swp.y;
   WinMapWindowPoints (hWndFrame, HWND_DESKTOP, &pointl, 1);

//   hWndParent = WinQueryWindow (hWnd, QW_PARENT);
//   WinQueryWindowPos (hWndParent, &swpParent);
//   WinQueryWindowPos (hWnd, &swp);

   rcl.xLeft   = 0;
   rcl.yBottom = 0;
   rcl.xRight  = swp.cx;
   rcl.yTop    = swp.cy;

   SetupBlitter.ulStructLen         = sizeof ( SETUP_BLITTER );
   SetupBlitter.fInvert             = FALSE;
   SetupBlitter.fccSrcColorFormat   = FOURCC_LUT8;
   SetupBlitter.ulSrcWidth          = giSrcW;
   SetupBlitter.ulSrcHeight         = giSrcH;
   SetupBlitter.ulSrcPosX           = 0;
   SetupBlitter.ulSrcPosY           = 0;
   SetupBlitter.ulDitherType        = 0;

   SetupBlitter.fccDstColorFormat   = FOURCC_SCRN;
   SetupBlitter.ulDstWidth          = swp.cx;
   SetupBlitter.ulDstHeight         = swp.cy;
   SetupBlitter.lDstPosX            = 0;
   SetupBlitter.lDstPosY            = 0;
//   SetupBlitter.lScreenPosX         = swpParent.x + swp.x;
//   SetupBlitter.lScreenPosY         = swpParent.y + swp.y;
   SetupBlitter.lScreenPosX         = pointl.x;
   SetupBlitter.lScreenPosY         = pointl.y;
   SetupBlitter.ulNumDstRects       = rgnCtl.crcReturned;
   SetupBlitter.pVisDstRects        = rctls;
//   SetupBlitter.ulNumDstRects       = 1;
//   SetupBlitter.pVisDstRects        = &rcl;
   rc = DiveSetupBlitter (ghDive, &SetupBlitter);

   GpiDestroyRegion (hps, hrgn);
   WinReleasePS (hps);

   if (rc != DIVE_SUCCESS)
      ST_Message ("Error while running DiveSetupBlitter");
}

//*************************************************************************
//
//*************************************************************************
void GFX_done (HWND hWnd)
{
   if (ghDive)
   {
      DiveFreeImageBuffer (ghDive, gulimageBuffer);
      DiveClose (ghDive);
      DosFreeMem (GFXscreenBuffer);
      ghDive = 0;
   }
}

//*************************************************************************
//
//*************************************************************************
void GFX_screenCpy (unsigned char *source, PBYTE pbImageBuffer)
{
   LONG    p;
   BYTE    c;
   ULONG   rc;

   for (p = 0; p < giSrcWxgiSrcH; p++)
   {
      c = (BYTE)source[p];
       *(pbImageBuffer + p) = c;
   }
   rc = DiveBlitImage (ghDive, gulimageBuffer, DIVE_BUFFER_SCREEN);
}

//*************************************************************************
//
//*************************************************************************
void FPS_measure (void)
{
   ULONG  msTime;
   static ULONG ulLast = 0;
   static ULONG fps    = 0;

   DosQuerySysInfo (QSV_MS_COUNT, QSV_MS_COUNT, (PVOID)&msTime, 4);
   if ((msTime >= ulLast) || (msTime < 1000))       //handle "overflow"
   {
      ulLast = msTime + 1000;
      gulFPS = fps;
      fps = 0;
   }

   fps++;
}

//*************************************************************************
//
//*************************************************************************
void GFX_blitter (HWND hWnd)
{
   PBYTE    pbImageBuffer;
   ULONG    ulScanLineBytes, ulScanLines;
   RECTL    rect;
   ULONG    rc;

   if(fVrnDisabled)
      return;

   if (gbShowFPS) FPS_measure ();

   if (DosRequestMutexSem (hmtxDive, 0) == 0)
   {
      rect.xLeft   = rect.yBottom = 0;
      rect.xRight  = SetupBlitter.ulDstWidth;
      rect.yTop    = SetupBlitter.ulDstHeight;

      if (gbDirectVRAM)
      {
         if (DiveAcquireFrameBuffer (ghDive, &rect) == DIVE_SUCCESS)
         {
            rc = DiveBeginImageBufferAccess (ghDive, gulimageBuffer, &pbImageBuffer, &ulScanLineBytes, &ulScanLines);
            memcpy (pbImageBuffer, GFXscreenBuffer, giSrcWxgiSrcH);
            rc = DiveBlitImage (ghDive, gulimageBuffer, DIVE_BUFFER_SCREEN);
            rc = DiveEndImageBufferAccess (ghDive, gulimageBuffer);
            rc = DiveDeacquireFrameBuffer (ghDive);
         }
      }
      else
         rc = DiveBlitImage (ghDive, gulimageBuffer, DIVE_BUFFER_SCREEN);

      DosReleaseMutexSem (hmtxDive);
   }
}

//*************************************************************************
//
//*************************************************************************
typedef struct {byte r, g, b;} palElement;
palElement GFXpallet[16];
void GFX_setrgb (INT i, INT r, INT g, INT b)
{
   BYTE p[1][4];
   ULONG rc;

   p[0][0] = b <<2;
   p[0][1] = g <<2;
   p[0][2] = r <<2;
   p[0][3] = 0;
   rc = DiveSetSourcePalette (ghDive, i, 1, (PBYTE)&p);
   GFXpallet[i].r = r;
   GFXpallet[i].g = g;
   GFXpallet[i].b = b;
}

//*************************************************************************
//
//*************************************************************************
void GFX_setrgbBlock (BYTE rgb[256][4])
{
   DiveSetSourcePalette (ghDive, 0, 256, (PBYTE)&(rgb[0][0]));
}

//*************************************************************************
//
//*************************************************************************
void GFX_getrgb (INT i, INT *r, INT *g, INT *b)
{
   *r = GFXpallet[i].r>>2;
   *g = GFXpallet[i].g>>2;
   *b = GFXpallet[i].b>>2;
}

//*************************************************************************
//
//*************************************************************************
void resize (HWND hWnd, INT w, INT h)
{
   RECTL rectl;
   ULONG ulFlag = SWP_SIZE | SWP_MOVE | SWP_SHOW | SWP_ACTIVATE;

   bPause = TRUE;
   WinSetWindowPos (WinQueryWindow (ghWnd, QW_PARENT), HWND_TOP, 0, 0, w, h, ulFlag);
   WinQueryWindowRect (ghWnd, &rectl);
   rectl.xRight = w - rectl.xRight - rectl.xLeft;
   rectl.yTop   = h- rectl.yTop    - rectl.yBottom;
   WinSetWindowPos (WinQueryWindow (ghWnd, QW_PARENT), HWND_TOP, 0,
                                                                 rectl.yTop,
                                                                 rectl.xRight + w,
                                                                 rectl.yTop + h,
                                                                 ulFlag);
   bPause = FALSE;
}


//*************************************************************************
//
//*************************************************************************
struct settings {
   BOOL bFramePrSec;
   BOOL bNoMouseCapture;
   BOOL bNoPause;
   BOOL bNoVRAM;
   LONG lFrameSpeed;
} gstr_settings;

void setGlobalVariableSettings (void)
{
   INT parm;
   //Fill the global variables, command param override ini file
   // gbShowFPS        = (BOOL)M_CheckParm ("-fps");
   if (M_CheckParm ("-fps"))
      gbShowFPS = TRUE;
   else
      gbShowFPS = gstr_settings.bFramePrSec;

   //gbPauseOnFocusLost = !(BOOL)M_CheckParm("-nopause");
   if (M_CheckParm ("-nopause"))
      gbPauseOnFocusLost = FALSE;
   else
      gbPauseOnFocusLost = !gstr_settings.bNoPause;

   //gbNoNouseCapture = (BOOL)M_CheckParm ("-nomousecapture");
   if (M_CheckParm ("-nomousecapture"))
      gbNoNouseCapture = TRUE;
   else
      gbNoNouseCapture = gstr_settings.bNoMouseCapture;

   if (M_CheckParm ("-novram"))
      gbNoVRAM = TRUE;
   else
      gbNoVRAM = FALSE;

   parm = M_CheckParm("-framespeed");
   if(parm && parm < gargc-1)
   {
      giFrameSpeed = atoi(gargv[parm+1]);
      if ((giFrameSpeed < 1) || (giFrameSpeed > 100))
         giFrameSpeed = 35;
   }
   else
      giFrameSpeed = gstr_settings.lFrameSpeed;
}
void readINIfile (CHAR file[])
{
   HINI hini;
   INT  i;

   hini = PrfOpenProfile (hab, (PSZ)file);
   gstr_settings.bFramePrSec     = (BOOL)PrfQueryProfileInt (hini, INI_SECTION, (PSZ)"fps"        , 0);
   gstr_settings.bNoMouseCapture = (BOOL)PrfQueryProfileInt (hini, INI_SECTION, (PSZ)"nomouse"    , 1);
   gstr_settings.bNoPause        = (BOOL)PrfQueryProfileInt (hini, INI_SECTION, (PSZ)"nopause"    , 1);
   gstr_settings.bNoVRAM         = (BOOL)PrfQueryProfileInt (hini, INI_SECTION, (PSZ)"novram"     , 1);
   i = PrfQueryProfileInt (hini, INI_SECTION, (PSZ)"framespeed" , 35);
   gstr_settings.lFrameSpeed     = (LONG)i;
   PrfCloseProfile (hini);

   setGlobalVariableSettings ();
}

void writeINIfile (CHAR file[])
{
   HINI  hini;
   char  cBuffer[255];
   SWP   swp;

   WinQueryWindowPos (hWndFrame, (PSWP)&swp);

   hini = PrfOpenProfile (hab, (PSZ)file);
   sprintf (cBuffer, "%d", gstr_settings.bFramePrSec);     PrfWriteProfileString (hini, INI_SECTION, (PSZ)"fps",        (PSZ)cBuffer);
   sprintf (cBuffer, "%d", gstr_settings.bNoMouseCapture); PrfWriteProfileString (hini, INI_SECTION, (PSZ)"nomouse",    (PSZ)cBuffer);
   sprintf (cBuffer, "%d", gstr_settings.bNoPause);        PrfWriteProfileString (hini, INI_SECTION, (PSZ)"nopause",    (PSZ)cBuffer);
   sprintf (cBuffer, "%d", gstr_settings.bNoVRAM);         PrfWriteProfileString (hini, INI_SECTION, (PSZ)"novram",     (PSZ)cBuffer);
   sprintf (cBuffer, "%d", (INT)gstr_settings.lFrameSpeed);PrfWriteProfileString (hini, INI_SECTION, (PSZ)"framespeed", (PSZ)cBuffer);
   PrfCloseProfile (hini);
   setGlobalVariableSettings ();
}


MRESULT EXPENTRY settingDlgProc (HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   BOOL    bHandled = TRUE;
   MRESULT mReturn  = 0;

   switch (msg)
   {           
      case WM_INITDLG:
         WinCheckButton (hWnd, ID_FPS           , gstr_settings.bFramePrSec);
         WinCheckButton (hWnd, ID_NOMOUSECAPTURE, gstr_settings.bNoMouseCapture);
         WinCheckButton (hWnd, ID_NOPAUSE       , gstr_settings.bNoPause);
         WinCheckButton (hWnd, ID_NOVRAM        , gstr_settings.bNoVRAM);

         WinSendDlgItemMsg (hWnd, ID_FRAMESPEED, SPBM_SETTEXTLIMIT,   (MPARAM)2 , (MPARAM)0);
         WinSendDlgItemMsg (hWnd, ID_FRAMESPEED, SPBM_SETLIMITS   ,   (MPARAM)99, (MPARAM)1);

         WinSendDlgItemMsg (hWnd, ID_FRAMESPEED, SPBM_SETCURRENTVALUE, (MPARAM)gstr_settings.lFrameSpeed, (MPARAM)0L);

         break;

      case WM_COMMAND:
      {
         switch ((ULONG)mp1)
         {
            case DID_OK:
               gstr_settings.bFramePrSec     = WinQueryButtonCheckstate (hWnd, ID_FPS);
               gstr_settings.bNoMouseCapture = WinQueryButtonCheckstate (hWnd, ID_NOMOUSECAPTURE);
               gstr_settings.bNoPause        = WinQueryButtonCheckstate (hWnd, ID_NOPAUSE);
               gstr_settings.bNoVRAM         = WinQueryButtonCheckstate (hWnd, ID_NOVRAM);
               WinSendDlgItemMsg (hWnd, ID_FRAMESPEED, SPBM_QUERYVALUE, (MPARAM)&gstr_settings.lFrameSpeed, 0L);
               break;
            case DID_CANCEL:
               break;
            default:
               bHandled = FALSE;
            break;
         }
      }
      default:
         bHandled = FALSE;
         break;
   }
   if (!bHandled)
      mReturn = WinDefDlgProc (hWnd, msg, mp1, mp2);

   return (mReturn);
}

void displaySetting (HWND hWnd)
{
   ULONG rtc;

   readINIfile (INI_FILE);
   rtc = WinDlgBox (HWND_DESKTOP, hWnd, settingDlgProc, NULLHANDLE, ID_SETTINGDLG, NULL);

   if (rtc == DID_OK)
      writeINIfile (INI_FILE);
}

//*************************************************************************
//
//*************************************************************************
extern int debugmode;
void CloseDARTAudio (void);

void cleanUp (BOOL bHexen)
{
   static BOOL bCleaned = FALSE;

   if (bCleaned) return;

   bCleaned = TRUE;  //One time cleaning is plenty...

   WinSetVisibleRegionNotify (hWndClient, FALSE);

   DosRequestMutexSem (hmtxDive, SEM_INDEFINITE_WAIT);
   bTreadGame = FALSE;
   bPause = TRUE;
   DosKillThread (tidGame);
   DosCloseMutexSem (hmtxDive);

   CloseDARTAudio ();

   if (pwindata)
   {
      if (pwindata->usMidiID) ShutdownMIDI (pwindata);
      if (pwindata->usDartID) ShutdownDART (pwindata);
      free (pwindata);
      pwindata = NULL;
   }

   if (ghDive != 0) GFX_done (NULL);
   if (gbJoystickOn)
   {
      INT rc;
      JoystickOff();
      rc = JoystickSaveCalibration();
      if (rc)
         ST_Message ("Calibration information saved!");
      else
         ST_Message ("Warning - was not able to save calibration information!");
   }
   if (debugmode)
      WinAlarm (HWND_DESKTOP, WA_NOTE);

   if (gbCleanFromHexen)
   {
      WinDestroyWindow (hWndFrame);
      WinDestroyMsgQueue (hmq);
      WinTerminate (hab);
   }
}

