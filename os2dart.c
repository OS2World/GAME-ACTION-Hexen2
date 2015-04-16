/*
Code from Marty
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <process.h>
#define INCL_DOS
#include <os2.h>

#define THREAD_STACK_SIZE 32768    // thread stack size in bytes
#define MAX_BUFFERS 32		   // 32 Max number of DART buffers to use

#ifndef _HAVE_MM_DEFINES_
#include "xtramm.h"
#endif

extern struct sDevice **psActiveDevices;

static struct _SoundSys {
    void    (*SoundSucker)(UINT8 *, UINT32);
    ULONG   nBufferSize;        // DART or MMPM/2 buffer size
    HEV     semDARTMutex;       // Mutex semaphore for manipulating DART
                                // buffer head and tail
    HEV     semNeedsBuffer;     // Hungry for more
    ULONG   ulThread;           // Audio updating thread
    MCI_OPEN_PARMS mop;

    unsigned short usDeviceID; // DART Amp-mixer device ID
    ULONG ulBufferHead;        // Current DART stream buffer
    ULONG ulBufferTail;        // DART stream buffer tail
    ULONG ulNumBuffers;        // Number of DART buffers
    MCI_MIX_BUFFER       MixBuffers[MAX_BUFFERS]; // Device buffers
    MCI_MIXSETUP_PARMS   MixSetupParms;           // Mixer parameters
    MCI_BUFFER_PARMS     BufferParms;             // Device buffer parms

    HQUEUE mqMessageQueueHandle; // Message queue used to handle messages
                                 // from the DART subsystem.  This is a non-
                                 // PM queue, allowing this to work for 
                                 // console-based apps as well as windowed.
} SoundSys;

HMTX  htmx_sucker_sem;
#define SUCKER_SEM "\\SEM32\\DART_SUCKER_SEM"

LONG APIENTRY DARTEventSound ( ULONG ulStatus,
                        PMCI_MIX_BUFFER  pBuffer,
                        ULONG  ulFlags  )
{
   ULONG junk;
   switch( ulFlags )
   {
   case MIX_STREAM_ERROR | MIX_WRITE_COMPLETE:  // error occur in device
        SoundSys.MixSetupParms.pmixWrite( SoundSys.MixSetupParms.ulMixHandle,
                                 &(SoundSys.MixBuffers[SoundSys.ulBufferTail]),
                                 1 );
	break;
   case MIX_WRITE_COMPLETE:           // for playback
        // Enter mutex region.  Do not block and wait for it.
        DosRequestMutexSem( SoundSys.semDARTMutex, 0 );
        if (SoundSys.ulBufferTail!=SoundSys.ulBufferHead) {
             SoundSys.ulBufferTail=(SoundSys.ulBufferTail+1)%SoundSys.ulNumBuffers;
        } 
        DosReleaseMutexSem( SoundSys.semDARTMutex );
        DosResetEventSem(SoundSys.semNeedsBuffer,&junk);
        DosPostEventSem(SoundSys.semNeedsBuffer);
        SoundSys.MixSetupParms.pmixWrite( SoundSys.MixSetupParms.ulMixHandle,
                                 &(SoundSys.MixBuffers[SoundSys.ulBufferTail]),
                                 1 );
        break;

   } // end switch
   return( TRUE );
}

//void MixingThread(void *data)
void __stdcall MixingThread(void *data)
{
   ULONG junk;
   do {
      // This reset MUST be here.  I don't understand why, but if it is NOT
	   // here, it will lock the system :(
      DosResetEventSem (SoundSys.semNeedsBuffer, &junk);
	   DosWaitEventSem (SoundSys.semNeedsBuffer, -1);

      // Get buffer from mixing library
      DosRequestMutexSem( SoundSys.semDARTMutex, 100 );
      // Request mutual exclusion.  Time out after 100ms.
	   SoundSys.ulBufferHead=(SoundSys.ulBufferHead+1)%SoundSys.ulNumBuffers;
      DosReleaseMutexSem( SoundSys.semDARTMutex );
	   SoundSys.SoundSucker(SoundSys.MixBuffers[SoundSys.ulBufferHead].pBuffer, SoundSys.nBufferSize);
	} while (1); // enddo
}


void SoundUpdate8Mono(UINT8 *p, UINT32 size)
{
}
void SoundUpdate8Stereo(UINT8 *p, UINT32 size)
{
}
void SoundUpdate16Mono(UINT8 *p, UINT32 size)
{
}
#define SOUNBUFFERSIZE 65*1024
extern UINT8 soundBuffer[SOUNBUFFERSIZE+1];

void SoundUpdate16Stereo (UINT8 *p, UINT32 size)
{
   ULONG s, t;
   #define blank (UINT8)128

   memset (p, blank, size * sizeof(UINT8));

   if (DosRequestMutexSem (htmx_sucker_sem, 0 ) == 0)
   {
//   DosEnterCritSec ();

   for (s=0; s < size ;s++)
   {
      *p = soundBuffer[s];// -0x80;
      p++;
   }
   
   for (t=0;s <SOUNBUFFERSIZE; s++)
      soundBuffer[t++] = soundBuffer[s];

   for (;t <SOUNBUFFERSIZE; t++)
      soundBuffer[t] = blank;

//   DosExitCritSec ();

   DosReleaseMutexSem (htmx_sucker_sem);
   }
}


UINT8 OpenDARTAudio(UINT32 dwSamplingRate, UINT8 bDepth, 
UINT8 bMonoStereo, UINT8 bSignedUnsigned, UINT32 dwBufferSize)
{
    MCI_AMP_OPEN_PARMS AmpOpenParms;
    ULONG              rc, ulIndex, OwnerPID;
//    char               shareaudio=0;
    char               shareaudio=1;

    // clean the OS/2 driver state
    memset(&SoundSys, 0, sizeof(SoundSys));
/*
  if ( bDepth == 8 && bMonoStereo == SOUND_MONO ) {
     SoundSys.SoundSucker = &SoundUpdate8Mono;
  } else if ( bDepth == 16 && bMonoStereo == SOUND_MONO ) {
     SoundSys.SoundSucker = (void (*)(UINT8 *, UINT32))&SoundUpdate16Mono;
  } else if ( bDepth == 8 && bMonoStereo != SOUND_MONO ) {
     SoundSys.SoundSucker = &SoundUpdate8Stereo;
  } else if ( bDepth == 16 && bMonoStereo != SOUND_MONO ) {
     */
     SoundSys.SoundSucker = (void (*)(UINT8 *, UINT32))&SoundUpdate16Stereo;
//  }

  if ((rc = DosCreateQueue(&(SoundSys.mqMessageQueueHandle), QUE_FIFO,
     (PSZ)"\\QUEUES\\SEALDARTQueue"))) { if (rc!=332) return -1; }
   
  if ((rc = DosOpenQueue(&OwnerPID, &(SoundSys.mqMessageQueueHandle), 
     (PSZ)"\\QUEUES\\SEALDARTQueue")))  { return -2; }


   DosCreateMutexSem((PSZ)SUCKER_SEM, &htmx_sucker_sem, 0, 0);

   DosCreateMutexSem((PSZ)"\\SEM32\\DARTMutexSem", &(SoundSys.semDARTMutex), 0, 0);
   DosCreateEventSem((PSZ)"\\SEM32\\DARTEventSem", &(SoundSys.semNeedsBuffer), DC_SEM_SHARED, 0);

   // Start off the thread to fill the audio buffers.  This thread will snooze
   //  until the rest of the audio system is up and running.

   SoundSys.ulThread = _beginthread((void(*)(void*))MixingThread,NULL,THREAD_STACK_SIZE,NULL);

   DosSetPriority(PRTYS_THREAD, PRTYC_TIMECRITICAL, 31, SoundSys.ulThread );


   // open the mixer device
   memset ( &AmpOpenParms, 0, sizeof ( MCI_AMP_OPEN_PARMS ) );

   AmpOpenParms.usDeviceID    = ( USHORT ) 0;
   AmpOpenParms.pszDeviceType = ( PSZ ) MCI_DEVTYPE_AUDIO_AMPMIX;
   AmpOpenParms.hwndCallback  = SoundSys.mqMessageQueueHandle;

   rc = mciSendCommand( 0,
                       MCI_OPEN,
                       MCI_DOS_QUEUE | MCI_WAIT | MCI_OPEN_TYPE_ID | (shareaudio ? MCI_OPEN_SHAREABLE:0),
                       ( PVOID ) &AmpOpenParms,
                       0 );

   if ( rc != MCIERR_SUCCESS )
   {
      return( -3 );
   }

   SoundSys.usDeviceID = AmpOpenParms.usDeviceID;

   // Set the MixSetupParms data structure to match the loaded file.
   // This is a global that is used to setup the mixer.
   memset( &(SoundSys.MixSetupParms), 0, sizeof( MCI_MIXSETUP_PARMS ) );

   SoundSys.MixSetupParms.ulBitsPerSample = bDepth;
   SoundSys.MixSetupParms.ulFormatTag = MCI_WAVE_FORMAT_PCM;
   SoundSys.MixSetupParms.ulSamplesPerSec = dwSamplingRate;
   SoundSys.MixSetupParms.ulChannels = (bMonoStereo == SOUND_MONO ? 1 : 2);

   // Setup the mixer for playback of wave data
   SoundSys.MixSetupParms.ulFormatMode = MCI_PLAY;
   SoundSys.MixSetupParms.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;
   SoundSys.MixSetupParms.pmixEvent    = DARTEventSound;

   rc = mciSendCommand( SoundSys.usDeviceID,
                        MCI_MIXSETUP,
                        MCI_WAIT | MCI_MIXSETUP_INIT,
                        ( PVOID ) &(SoundSys.MixSetupParms),
                        0 );

   if ( rc != MCIERR_SUCCESS )
   {
      return( -4 );
   }

   SoundSys.MixSetupParms.ulBufferSize = dwBufferSize * (bMonoStereo == SOUND_MONO ? 1 : 2);
   SoundSys.nBufferSize=SoundSys.MixSetupParms.ulBufferSize;

   // This code was stolen from a DART sample program.  I don't 
   // particularly understand it, but it seems to work very well.
   SoundSys.ulNumBuffers = ((1 * dwSamplingRate * (SoundSys.MixSetupParms.ulBitsPerSample/8)
       * SoundSys.MixSetupParms.ulChannels) / SoundSys.nBufferSize) +1;

   if (SoundSys.ulNumBuffers>MAX_BUFFERS) SoundSys.ulNumBuffers=MAX_BUFFERS;

   // Set up the BufferParms data structure and allocate
   // device buffers from the Amp-Mixer
   SoundSys.BufferParms.ulNumBuffers = SoundSys.ulNumBuffers;
   SoundSys.BufferParms.ulBufferSize = SoundSys.MixSetupParms.ulBufferSize;
   SoundSys.BufferParms.pBufList = &(SoundSys.MixBuffers);

   rc = mciSendCommand( SoundSys.usDeviceID,
                        MCI_BUFFER,
                        MCI_WAIT | MCI_ALLOCATE_MEMORY,
                        ( PVOID ) &(SoundSys.BufferParms),
                        0 );

    if ( rc != MCIERR_SUCCESS )
    {
      return( -5 );
    }

   // Set up all device buffers
   for( ulIndex = 0; ulIndex < SoundSys.ulNumBuffers; ulIndex++)
   {
      memset( SoundSys.MixBuffers[ ulIndex ].pBuffer, 0, SoundSys.nBufferSize );
      SoundSys.MixBuffers[ ulIndex ].ulBufferLength = SoundSys.nBufferSize;
   }

   SoundSys.nBufferSize /= (bDepth>>3)*((bMonoStereo!=SOUND_MONO)+1);

//   printf("Audio samples in buffer: %ld\n", SoundSys.nBufferSize );

   // Set the "end-of-stream" flag
   SoundSys.MixBuffers[SoundSys.ulNumBuffers - 1].ulFlags = MIX_BUFFER_EOS;

   DosPostEventSem(SoundSys.semNeedsBuffer);

// Must write at least two buffers to start mixer
   SoundSys.MixSetupParms.pmixWrite( SoundSys.MixSetupParms.ulMixHandle,
                         &(SoundSys.MixBuffers[SoundSys.ulBufferTail]),
                         2 );

   return( 0 );
}

void CloseDARTAudio(VOID)
{
   MCI_GENERIC_PARMS    GenericParms = {0};
   ULONG                rc;

   if (SoundSys.usDeviceID)
   {
      rc = mciSendCommand( SoundSys.usDeviceID,
                        MCI_BUFFER,
                        MCI_WAIT | MCI_DEALLOCATE_MEMORY,
                        ( PVOID )&(SoundSys.BufferParms),
                        0 );
//   if ( rc != MCIERR_SUCCESS ) return;

      rc = mciSendCommand( SoundSys.usDeviceID,
                        MCI_CLOSE,
                        MCI_WAIT ,
                        ( PVOID )&GenericParms,
                        0 );

//   if ( rc != MCIERR_SUCCESS ) return;
      DosKillThread (SoundSys.ulThread);
      DosCloseEventSem (SoundSys.semNeedsBuffer);
      DosCloseMutexSem (SoundSys.semDARTMutex);
      DosCloseMutexSem (htmx_sucker_sem);
      DosPurgeQueue(SoundSys.mqMessageQueueHandle);
      DosCloseQueue(SoundSys.mqMessageQueueHandle);
   }

   SoundSys.usDeviceID = 0;

   return;
}

