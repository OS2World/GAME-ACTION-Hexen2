#ifndef __XTRA_MM_DEFS__
#define __XTRA_MM_DEFS__

#define UINT8 signed short
//#define UINT8 signed char
#define UINT32 LONG
#define SOUND_MONO 1

/*
 * This set of definitions is necessary for those who are compiling with
 * EMX because the Multimedia routines and structures are not prototyped
 * in the standard set of H files.  This also contains some DART-specific
 * definitions for which some older build environments may not have their
 * own support.
 *
 */

#define SEMWAIT_OPERATION     9      // Semaphore wait operation for playlist
#define SEMPOST_OPERATION     10     // Semaphore post operation for playlist
#define MCI_DOS_QUEUE         8L     // Tell MCI to use a non-PM message queue
#define MCI_BUFFER            62
#define MCI_MIXSETUP          63
#define MCI_MIXSETUP_INIT     0x10000
#define MCI_ALLOCATE_MEMORY   0x40000
#define MCI_DEALLOCATE_MEMORY 0x80000
#define MIX_WRITE_COMPLETE    0x00002
#define MIX_BUFFER_EOS	      0x00001
#define MIX_STREAM_ERROR      0x00080
#define MCI_DEVTYPE_AUDIO_AMPMIX        9
#define MCI_DEVTYPE_WAVEFORM_AUDIO      7
#define MCI_OPEN                        1
#define MCI_CLOSE                       2
#define MCI_NOTIFY                      0x00000001L
#define MCI_WAIT                        0x00000002L
#define MCI_PLAY                        4
#define MCI_OPEN_TYPE_ID                0x00001000L
#define MCI_OPEN_SHAREABLE              0x00002000L
#define MCIERR_SUCCESS                  0
#define MCI_WAVE_FORMAT_PCM             DATATYPE_WAVEFORM
#define DATATYPE_WAVEFORM               1L

ULONG APIENTRY mciSendCommand   (USHORT   usDeviceID,
                                 USHORT   usMessage,
                                 ULONG    ulParam1,
                                 PVOID    pParam2,
                                 USHORT   usUserParm);

typedef struct _MCI_GENERIC_PARMS {
    HWND   hwndCallback;      /* PM window handle for MCI notify message */
} MCI_GENERIC_PARMS;
typedef MCI_GENERIC_PARMS   *PMCI_GENERIC_PARMS;

typedef struct _MCI_OPEN_PARMS {
    HWND    hwndCallback;    /* PM window handle for MCI notify message */
    USHORT  usDeviceID;      /* Device ID returned to user              */
    USHORT  usReserved0;     /* Reserved                                */
    PSZ     pszDeviceType;   /* Device name from SYSTEM.INI             */
    PSZ     pszElementName;  /* Typically a file name or NULL           */
    PSZ     pszAlias;        /* Optional device alias                   */
} MCI_OPEN_PARMS;
typedef MCI_OPEN_PARMS   *PMCI_OPEN_PARMS;

typedef struct _MCI_AMP_OPEN_PARMS {
    HWND    hwndCallback;    /* PM window handle for MCI notify message */
    USHORT  usDeviceID;      /* Device ID returned to user              */
    USHORT  usReserved0;     /* Reserved field                          */
    PSZ     pszDeviceType;   /* Device name from SYSTEM.INI             */
    PSZ     pszElementName;  /* Typically a file name or NULL           */
    PSZ     pszAlias;        /* Optional device alias                   */
    PVOID   pDevDataPtr;     /* Pointer to device data                  */
} MCI_AMP_OPEN_PARMS;
typedef MCI_AMP_OPEN_PARMS   *PMCI_AMP_OPEN_PARMS;

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
      ULONG            ulFlags );
      
typedef MIXERPROC *PMIXERPROC;
typedef LONG (APIENTRY MIXEREVENT)
    ( ULONG            ulStatus,
      PMCI_MIX_BUFFER  pBuffer,
      ULONG            ulFlags );
      
typedef MIXEREVENT  *PMIXEREVENT;

typedef struct _MCI_MIXSETUP_PARMS  
{
    HWND         hwndCallback;     /* PM window handle for MCI notify message      */
    ULONG        ulBitsPerSample;  /* IN Number of Bits per Sample                 */
    ULONG        ulFormatTag;      /* IN Format Tag                                */
    ULONG        ulSamplesPerSec;  /* IN Sampling Rate                             */
    ULONG        ulChannels;       /* IN Number of channels                        */
    ULONG        ulFormatMode;     /* IN Either MCI_RECORD or MCI_PLAY             */
    ULONG        ulDeviceType;     /* IN MCI_DEVTYPE (i.e. DEVTYPE_WAVEFORM etc.)  */
    ULONG        ulMixHandle;      /* OUT--mixer returns handle for write/read     */
    PMIXERPROC   pmixWrite;        /* OUT-Mixer Write Routine entry point          */
    PMIXERPROC   pmixRead;         /* OUT-Mixer Read Routine entry point           */
    PMIXEREVENT  pmixEvent;        /* IN--Mixer Read Routine entry point           */
    PVOID        pExtendedInfo;    /* Ptr to extended wave information             */
    ULONG        ulBufferSize;     /* OUT--suggested buffer size for current mode  */
    ULONG        ulNumBuffers;     /* OUT--suggested # of buffers for current mode */
} MCI_MIXSETUP_PARMS;
      
typedef MCI_MIXSETUP_PARMS   *PMCI_MIXSETUP_PARMS;

typedef struct _MCI_BUFFER_PARMS  
{
    HWND       hwndCallback;     /* PM window handle for MCI notify message    */
    ULONG      ulStructLength;   /* Length of the MCI Buffer command           */
    ULONG      ulNumBuffers;     /* Number of buffers MCI driver should use    */
    ULONG      ulBufferSize;     /* Size of buffers MCI driver should use      */
    ULONG      ulMinToStart;     /* Min number of buffers to create a stream.  */
    ULONG      ulSrcStart;       /* # of EMPTY buffers required to start Source*/
    ULONG      ulTgtStart;       /* # of FULL buffers required to start Target */
       
    PVOID      pBufList;         /* Pointer to a list of buffers               */
         
} MCI_BUFFER_PARMS;

typedef MCI_BUFFER_PARMS   *PMCI_BUFFER_PARMS;

void CloseDARTAudio(VOID);
UINT8 OpenDARTAudio(UINT32 dwSamplingRate, UINT8 bDepth, UINT8 bMonoStereo, UINT8 bSignedUnsigned, UINT32 dwBufferSize);

#endif
