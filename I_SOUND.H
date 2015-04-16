#ifndef __SOUND__
#define __SOUND__



#include <io.h>
#include "soundst.h"

#define SND_TICRATE             140             // tic rate for updating sound
#define SND_MAXSONGS    40              // max number of songs in game
#define SND_SAMPLERATE  11025   // sample rate of sound effects

typedef enum
{
  snd_none,
  snd_PC,
  snd_Adlib,
  snd_SB,
  snd_PAS,
  snd_GUS,
  snd_MPU,
  snd_MPU2,
  snd_MPU3,
  snd_AWE,
  snd_CDMUSIC,
  NUM_SCARDS
} cardenum_t;

void I_PauseSong(int handle);
void I_ResumeSong(int handle);
void I_SetMusicVolume(int volume);
void I_SetSfxVolume(int volume);
int I_RegisterSong(void *data, long len);
void I_UnRegisterSong(int handle);
int I_QrySongPlaying(int handle);
void I_StopSong(int handle);
void I_PlaySong(int handle, int looping);
int I_GetSfxLumpNum(sfxinfo_t *sound);
int I_StartSound (int id, void *data, int vol, int sep, int pitch, int priority);
void I_StopSound(int handle);
int I_SoundIsPlaying(int handle);
void I_UpdateSoundParams(int handle, int vol, int sep, int pitch);
void I_sndArbitrateCards(void);
void I_StartupSound (void);
void I_ShutdownSound (void);
void I_SetChannels(int channels);


/*********************************************************************/
/* parameter structure for the MCI_BUFFER    message                 */
/*********************************************************************/
typedef struct _MCI_MIX_BUFFER
{
   ULONG      ulStructLength;   /* Length of the structure          */
   PVOID      pBuffer;          /* Pointer to a buffer              */
   ULONG      ulBufferLength;   /* Length of the buffer             */
   ULONG      ulFlags;          /* Flags                            */
   ULONG      ulUserParm;       /* Caller parameter                 */
   ULONG      ulTime;           /* OUT--Current time in MS          */
   ULONG      ulReserved1;      /* Unused.                          */
   ULONG      ulReserved2;      /* Unused.                          */
} MCI_MIX_BUFFER;

typedef MCI_MIX_BUFFER *PMCI_MIX_BUFFER;

typedef LONG (APIENTRY MIXERPROC)
              ( ULONG            ulHandle,
              PMCI_MIX_BUFFER  pBuffer,
              ULONG            ulFlags        );

typedef MIXERPROC *PMIXERPROC;


typedef LONG (APIENTRY MIXEREVENT)
              ( ULONG            ulStatus,
              PMCI_MIX_BUFFER  pBuffer,
              ULONG            ulFlags        );

typedef MIXEREVENT  *PMIXEREVENT;


/*********************************************************************/
/* parameter structure for the MCI_MIXSETUP_PARMS message            */
/*********************************************************************/
typedef struct _MCI_MIXSETUP_PARMS
{
   HWND         hwndCallback;     /* PM window handle for MCI notify message */
   ULONG        ulBitsPerSample;  /* IN Number of Bits per Sample            */
   ULONG        ulFormatTag;      /* IN Format Tag                           */
   ULONG        ulSamplesPerSec;  /* IN Sampling Rate                        */
   ULONG        ulChannels;       /* IN Number of channels                   */
   ULONG        ulFormatMode;     /* IN Either MCI_RECORD or MCI_PLAY        */
   ULONG        ulDeviceType;     /* IN MCI_DEVTYPE (i.e. DEVTYPE_WAVEFORM e */
   ULONG        ulMixHandle;      /* OUT--mixer returns handle for write/read*/
   PMIXERPROC   pmixWrite;        /* OUT-Mixer Write Routine entry point     */
   PMIXERPROC   pmixRead;         /* OUT-Mixer Read Routine entry point      */
   PMIXEREVENT  pmixEvent;        /* IN--Mixer Read Routine entry point      */
   PVOID        pExtendedInfo;    /* Ptr to extended wave information        */
   ULONG        ulBufferSize;     /* OUT--suggested buffer size for current m*/
   ULONG        ulNumBuffers;     /* OUT--suggested # of buffers for current */
} MCI_MIXSETUP_PARMS;

typedef MCI_MIXSETUP_PARMS   *PMCI_MIXSETUP_PARMS;

#define  MAX_BUFFERS    256

/*********************************************************************/
/* parameter structure for the MCI_BUFFER    message                 */
/*********************************************************************/
typedef struct _MCI_BUFFER_PARMS
{
   HWND       hwndCallback;     // PM window handle for MCI notify messag
   ULONG      ulStructLength;   // Length of the MCI Buffer command
   ULONG      ulNumBuffers;     // Number of buffers MCI driver should us
   ULONG      ulBufferSize;     // Size of buffers MCI driver should use
   ULONG      ulMinToStart;     // Min number of buffers to create a stre
   ULONG      ulSrcStart;       // # of EMPTY buffers required to start S
   ULONG      ulTgtStart;       // # of FULL buffers required to start Ta

   PVOID      pBufList;         // Pointer to a list of buffers

} MCI_BUFFER_PARMS;
typedef MCI_BUFFER_PARMS   *PMCI_BUFFER_PARMS;

typedef CHAR       *LPSTR,  *PSTR;
typedef ULONG      DWORD,   *PDWORD,  *LPDWORD;
typedef USHORT     WORD,    *PWORD,   *LPWORD;

/**********************************************************/
/* parameters for the AMP #define message                */
/**********************************************************/
typedef struct _MCI_AMP_OPEN_PARMS
{
   DWORD   dwCallback;      /* PM window handle for MCI notify message */
   USHORT  usDeviceID;       /* Device ID returned to user              */
   USHORT  usReserved0;      /* Reserved field                          */
   PSZ     pszDeviceType; /* Device name from SYSTEM.INI             */
   LPSTR   lpstrElementName;/* Typically a file name or NULL           */
   LPSTR   lpstrAlias;      /* Optional device alias                   */
   ULONG   ulDevDataPtr;    /* Pointer to device data                  */
} MCI_AMP_OPEN_PARMS;
typedef MCI_AMP_OPEN_PARMS   *LPMCI_AMP_OPEN_PARMS;

typedef struct _MCI_OPEN_PARMS
{
   DWORD   dwCallback;      /* PM window handle for MCI notify message */
   USHORT  usDeviceID;       /* Device ID returned to user              */
   WORD    wReserved0;      /* Reserved                                */
   LPSTR   lpstrDeviceType; /* Device name from SYSTEM.INI             */
   PSZ     pszElementName;/* Typically a file name or NULL           */
   LPSTR   lpstrAlias;      /* Optional device alias                   */
} MCI_OPEN_PARMS;
typedef MCI_OPEN_PARMS   *LPMCI_OPEN_PARMS;

 /*********************************************************************/
 /* parameters for default command messages with empty parameter lists*/
 /*********************************************************************/
 typedef struct _MCI_GENERIC_PARMS
 {
    HWND   hwndCallback;     /* PM window handle for MCI notify message */
 } MCI_GENERIC_PARMS;
 typedef MCI_GENERIC_PARMS   *PMCI_GENERIC_PARMS;

typedef struct _MCI_PLAY_PARMS
{
   HWND    hwndCallback;      /* PM window handle for MCI notify message */
   DWORD   dwFrom;          /* Play from this position                 */
   DWORD   dwTo;            /* Play to this position                   */
} MCI_PLAY_PARMS;
typedef MCI_PLAY_PARMS   *LPMCI_PLAY_PARMS;

typedef struct _MCI_SET_PARMS
{
   DWORD   dwCallback;      /* PM window handle for MCI notify message*/
   DWORD   dwTimeFormat;    /* Time format to be used by the device*/
   DWORD   dwSpeedFormat;   /* Speed format used by this device*/
   ULONG   ulAudio;         /* Channel number for this operation*/
   /* (MCI_SET_AUDIO_LEFT, MCI_SET_AUDIO_RIGHT,      */
   /*  MCI_SET_AUDIO_ALL)                            */
   ULONG   ulLevel;         /* Volume, treble or bass level as % of max.*/
   DWORD   dwOver;          /* Delay time for vectored change in millisec*/
   DWORD   dwItem;          /* Item field for set item flags*/
   DWORD   dwValue;         /* Value associated with item flag*/
} MCI_SET_PARMS;
typedef MCI_SET_PARMS       *LPMCI_SET_PARMS;


/*********************
 * WAVEFORM datatypes
 *********************/
#define  DATATYPE_WAVEFORM       0x0001L     /* WAVEFORM audio (PCM)*/


#define MCI_DEVTYPE_AUDIO_AMPMIX        9
#define MCI_DEVTYPE_WAVEFORM_AUDIO      7

#define MCI_BUFFER                      62
#define MCI_MIXSETUP                    63


#define MCI_OPEN                        1
#define MCI_CLOSE                       2

#define MCI_PAUSE                       7
#define MCI_PLAY                        4
#define MCI_NOTIFY                          0x00000001L
#define MCI_RESUME                      29
#define MCI_SET                         12
#define MCI_SET_AUDIO                       0x00001000L
#define MCI_SET_VOLUME                      0x00080000L
#define MCI_WAIT                        0x00000002L
#define MCI_MIXSETUP_INIT            0x00010000L
#define MCI_OPEN_TYPE_ID                0x00001000L
#define MCI_OPEN_SHAREABLE              0x00002000L
#define  MCI_ALLOCATE_MEMORY      0x00040000L
#define  MCI_DEALLOCATE_MEMORY    0x00080000L
#define MCI_OPEN_ELEMENT                    0x00000100L

#define MCIERR_SUCCESS                  0
#define MCI_WAVE_FORMAT_PCM                 DATATYPE_WAVEFORM

/*******************************************************/
/* Values for the dwAudio field                        */
/*******************************************************/
#define MCI_SET_AUDIO_ALL                   0x00000000L
#define MCI_SET_AUDIO_LEFT                  0x00000001L
#define MCI_SET_AUDIO_RIGHT                 0x00000002L


/************************
 * Bits per Sample
 ************************/
#define         BPS_4                   4               /* 4 bits/sample (ADPCM*/
#define         BPS_8                   8               /* 8 bits/sample (PCM)*/
#define         BPS_16                  16              /* 16 bits/sample(PCM)*/

/************************
 * Sampling Rate
 ************************/
#define         HZ_8000                 8000            /* 8.0 kHz*/
#define         HZ_11025                11025           /* 11.025 kHz*/
#define         HZ_14700                14700           /* 14.700 kHz (SPV/2)*/
#define         HZ_18900                18900           /* 18.900 kHz (CD/XA LV*/
#define         HZ_22050                22050           /* 22.050 kHz*/
#define         HZ_37800                37800           /* 37.800 kHz (CD/XA LV*/
#define         HZ_44100                44100           /* 44.100 kHz*/

/************************
 * Channels
 ************************/
#define         CH_1                    1               /* Mono*/
#define         CH_2                    2               /* Stereo*/
#define         CH_4                    4               /* Quad*/


/*********************************************************************/
/* valid returns for the ulFlags param of the MIXEREVENT entry       */
/*********************************************************************/


#define MIX_STREAM_ERROR             0x00000080L
#define MIX_READ_COMPLETE            0x00000001L
#define MIX_WRITE_COMPLETE           0x00000002L

#define ULONG_LOWD(ul)  (*(USHORT *)((ULONG *)(&ul)))     /* Low  Word*/

void I_UpdateSound(void);

/*********************************************************************/
/*                                                                   */
/*  MCI Function prototypes                                          */
/*                                                                   */
/*********************************************************************/

ULONG APIENTRY mciSendCommand   (USHORT   usDeviceID,
                                  USHORT   usMessage,
                                  ULONG    ulParam1,
                                  PVOID    pParam2,
                                  USHORT   usUserParm);

ULONG APIENTRY mciGetErrorString(ULONG   ulError,
                                 PSZ     pszBuffer,
                                 USHORT  usLength);


typedef struct _WINDATA
{
   HAB    hab;
   ULONG  ulToEnd;                  /* stop running threads                */
   int    loading;                  /* Doom/2 is loading                   */
   BOOL   fStartBlit;

   BOOL   fChgSrcPalette;           /* Flag for change of source palette   */
   BOOL   fDataInProcess;           /* ????  Visual region enable/disable  */

   HWND   hwndFrame;                /* Frame window handle                 */
   HWND   hwndClient;               /* Client window handle                */
   DIVE_CAPS DiveCaps;
   FOURCC fccFormats[100];          /* Color format code                   */
   ULONG  ulColorBits;              /* Color bit depth                     */
   ULONG  ulNumColors;              /* Number of colors in bitmaps         */
   ULONG  ulWidth;                  /* Bitmap width in pels                */
   ULONG  ulHeight;                 /* Bitmap Height in pels               */
   FOURCC fccColorFormat;           /* Bitmap color format                 */
   
   TID      tidDoomThread;               /* Thread ID for Doom/2           */
   TID      tidBlitThread;               /* Thread ID for blitting routine */

   HDIVE    hDive;                       /* DIVE handle                    */
   BOOL     fFSBase;                     /* FS DIVE support in base OS     */
   BOOL     fSwitching;                  /* 1 to prevent action of WindowSetBlit */
   ULONG    ulWindowStyle;               /* current window style           */
   ULONG    ulImage;                     /* dive image buffer              */
   PBYTE    pbBuffer;                    /* dive image buffer              */
   HMTX     hmtxEventSem;                /* event queue mutex              */

   USHORT   usDartID;                    /* Amp Mixer device id     */
   ULONG    ulBufferCount;               /* Current file buffer     */
   ULONG    ulNumBuffers;                /* Number of file buffers  */
   ULONG    ulNumReturnedBuffers;        /* Num of returned buffers */
   MCI_MIX_BUFFER       MixBuffers[MAX_BUFFERS];   /* Device buffers          */
   MCI_MIXSETUP_PARMS   MixSetupParms;   /* Mixer parameters        */
   MCI_BUFFER_PARMS     BufferParms;     /* Device buffer parms     */
   int                  flag;            /* flag for buffer play    */
   int                  FillBuffer;      /* current fill buffer     */
   int                  PlayBuffer;      /* current play buffer     */

   USHORT   usMidiID;                    /* Midi device id          */
   int      looping;                     /* loop music flag         */
   int      midiVolume;                  /* Midi music volume       */
   BOOL     fPassedDevice;

   BOOL fShiftPressed; 		/* Status of the SHIFT key */
   BOOL fAltPressed;		/* Status of the ALT key   */
   BOOL fRunMode;		/* Status of Run Mode      */
   int  last_char;		/* Last ASCII char pressed */

} WINDATA, *PWINDATA;

//  Sound FX volume has default, 0 - 15
//  Music volume has default, 0 - 15
// These are multiplied by 8.
extern int snd_SfxVolume;      // maximum volume for sound
extern int snd_MusicVolume;    // maximum volume for music

// extern OS/2 data
//
extern WINDATA* pmData;

//
// function prototypes
//

void  InitDART( PWINDATA);
void  ShutdownDART( PWINDATA);
void  PlayDART( PWINDATA);
int   RegisterMIDI( PWINDATA, void*, int);
void  OpenMIDI( PWINDATA);
int   PlayMIDI( PWINDATA, int looping);
void  PauseMIDI( PWINDATA);
void  ResumeMIDI( PWINDATA);
void  ShutdownMIDI( PWINDATA);
void  SetMIDIVolume( PWINDATA midiData, int vol);
void  MciError(ULONG);


#endif

#ifndef __ICDMUS__
#define __ICDMUS__

#define CDERR_NOTINSTALLED   10		// MSCDEX not installed
#define CDERR_NOAUDIOSUPPORT 11		// CD-ROM Doesn't support audio
#define CDERR_NOAUDIOTRACKS  12		// Current CD has no audio tracks
#define CDERR_BADDRIVE       20		// Bad drive number
#define CDERR_BADTRACK       21		// Bad track number
#define CDERR_IOCTLBUFFMEM   22		// Not enough low memory for IOCTL
#define CDERR_DEVREQBASE     100	// DevReq errors

extern int cd_Error;

int I_CDMusInit(void);
int I_CDMusPlay(int track);
int I_CDMusStop(void);
int I_CDMusResume(void);
int I_CDMusSetVolume(int volume);
int I_CDMusFirstTrack(void);
int I_CDMusLastTrack(void);
int I_CDMusTrackLength(int track);

#endif

