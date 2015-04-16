
// I_SOUND.C

#include <stdio.h>
#include "h2def.h"
#include "dmx.h"
#include "sounds.h"
#include "i_sound.h"


// UNIX hack, to be removed.
#ifdef SNDSERV
// Separate sound server process.
FILE*	sndserver=0;
char*	sndserver_filename = "./sndserver ";
#elif SNDINTR

// Update all 30 millisecs, approx. 30fps synchronized.
// Linux resolution is allegedly 10 millisecs,
//  scale is microseconds.
#define SOUND_INTERVAL     500

// Get the interrupt. Set duration in millisecs.
int I_SoundSetTimer( int duration_of_tick );
void I_SoundDelTimer( void );
#else
// None?
#endif


// A quick hack to establish a protocol between
// synchronous mix buffer updates and asynchronous
// audio writes. Probably redundant with gametic.
//static int flag = 0;

// The number of internal mixing channels,
//  the samples calculated for each mixing step,
//  the size of the 16bit, 2 hardware channel (stereo)
//  mixing buffer, and the samplerate of the raw data.


// Needed for calling the actual sound output.
#define SAMPLECOUNT		512
#define NUM_CHANNELS		8
// It is 2 for 16bit, and 2 for two channels.
#define BUFMUL                  4
#define MIXBUFFERSIZE		(SAMPLECOUNT*BUFMUL)

#define SAMPLERATE		11025	// Hz
#define SAMPLESIZE		2   	// 16bit

// The actual lengths of all sound effects.
int 		lengths[NUMSFX];

// The actual output device.
int	audio_fd;

// The global mixing buffer.
// Basically, samples from all active internal channels
//  are modifed and added, and stored in the buffer
//  that is submitted to the audio device.
signed short	mixbuffer[MIXBUFFERSIZE];

// The channel step amount...
unsigned int	channelstep[NUM_CHANNELS];
// ... and a 0.16 bit remainder of last step.
unsigned int	channelstepremainder[NUM_CHANNELS];


// The channel data pointers, start and end.
unsigned char*	channels[NUM_CHANNELS];
unsigned char*	channelsend[NUM_CHANNELS];


// Time/gametic that the channel started playing,
//  used to determine oldest, which automatically
//  has lowest priority.
// In case number of active sounds exceeds
//  available channels.
int		channelstart[NUM_CHANNELS];

// The sound in channel handles,
//  determined on registration,
//  might be used to unregister/stop/modify,
//  currently unused.
int 		channelhandles[NUM_CHANNELS];

// SFX id of the playing sound effect.
// Used to catch duplicates (like chainsaw).
int		channelids[NUM_CHANNELS];			

// Pitch to stepping lookup, unused.
int		steptable[256];

// Volume lookups.
int		vol_lookup[128*256];

// Hardware left and right channel volume lookup.
int*		channelleftvol_lookup[NUM_CHANNELS];
int*		channelrightvol_lookup[NUM_CHANNELS];

// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
int             snd_SfxVolume = 15;



WINDATA* pmData;

/*
===============
=
= I_StartupTimer
=
===============
*/

int tsm_ID = -1;

void I_StartupTimer (void)
{
#ifndef NOTIMER
	extern int I_TimerISR(void);

	ST_Message("  I_StartupTimer()");
	// installs master timer.  Must be done before StartupTimer()!
#ifndef NO_DMX
	TSM_Install(SND_TICRATE);
	tsm_ID = TSM_NewService (I_TimerISR, 35, 255, 0); // max priority
	if (tsm_ID == -1)
	{
		I_Error("Can't register 35 Hz timer w/ DMX library");
	}
#endif
#endif
}

void I_ShutdownTimer (void)
{
#ifndef NO_DMX
	TSM_DelService(tsm_ID);
	TSM_Remove();
#endif
}

/*
 *
 *                           SOUND HEADER & DATA
 *
 *
 */

// sound information
#if 0
const char *dnames[] = {"None",
			"PC_Speaker",
			"Adlib",
			"Sound_Blaster",
			"ProAudio_Spectrum16",
			"Gravis_Ultrasound",
			"MPU",
			"AWE32"
			};
#endif

const char snd_prefixen[] = { 'P', 'P', 'A', 'S', 'S', 'S', 'M',
  'M', 'M', 'S' };

int snd_Channels;
int snd_DesiredMusicDevice, snd_DesiredSfxDevice;
int snd_MusicDevice,    // current music card # (index to dmxCodes)
	snd_SfxDevice,      // current sfx card # (index to dmxCodes)
	snd_MaxVolume,      // maximum volume for sound
	snd_MusicVolume;    // maximum volume for music
int dmxCodes[NUM_SCARDS]; // the dmx code for a given card

int     snd_SBport, snd_SBirq, snd_SBdma;       // sound blaster variables
int     snd_Mport;                              // midi variables

extern boolean  snd_MusicAvail, // whether music is available
		snd_SfxAvail;   // whether sfx are available




void I_PauseSong(int handle)
{
#ifndef NO_DMX
  MUS_PauseSong(handle);
#endif
   handle = 0;
   PauseMIDI (pmData);
}

void I_ResumeSong(int handle)
{
#ifndef NO_DMX
  MUS_ResumeSong(handle);
#endif
   handle = 0;
   ResumeMIDI( pmData);
}

void I_SetMusicVolume(int volume)
{
#ifndef NO_DMX
  MUS_SetMasterVolume(volume*8);
#endif

  // Internal state variable.
  snd_MusicVolume = volume;
  ST_Message ("I_SetMusicVolume %d", volume);
  // Now set volume on output device.
  SetMIDIVolume( pmData, volume);
}

void I_SetSfxVolume(int volume)
{
  ST_Message ("I_SetSfxVolume %d", volume);
  snd_MaxVolume = volume; // THROW AWAY?
  snd_SfxVolume = volume;
}

/*
 *
 *                              SONG API
 *
 */

int I_RegisterSong(void *data, long len)
{
  int rc = 0;
#ifndef NO_DMX
  rc = MUS_RegisterSong(data);
#endif

   rc = RegisterMIDI( pmData, data, len);
#ifdef SNDDEBUG
  if (rc<0) ST_Message("    MUS_Reg() returned %d\n", rc);
#endif
   return rc;
}

void I_UnRegisterSong(int handle)
{
#ifndef NO_DMX
   int rc = 0;
  rc = MUS_UnregisterSong(handle);
#ifdef SNDDEBUG
  if (rc < 0) ST_Message("    MUS_Unreg() returned %d\n", rc);
#endif
#endif
   handle = 0;
   // remove files
   unlink( "hexen.mid");
   unlink( "hexen.mus");
}

int I_QrySongPlaying(int handle)
{
   int rc = 0;
#ifndef NO_DMX
  rc = MUS_QrySongPlaying(handle);
#ifdef SNDDEBUG
  if (rc < 0) ST_Message("    MUS_QrySP() returned %d\n", rc);
#endif
#endif

  return rc;
}

// Stops a song.  MUST be called before I_UnregisterSong().

void I_StopSong(int handle)
{
//  int rc;
#ifndef NO_DMX
  rc = MUS_StopSong(handle);
#ifdef SNDDEBUG
  if (rc < 0) ST_Message("    MUS_StopSong() returned %d\n", rc);
#endif
#endif
  ShutdownMIDI (pmData);
}

void I_PlaySong(int handle, boolean looping)
{
//  int rc;
#ifndef NO_DMX
  rc = MUS_ChainSong(handle, looping ? handle : -1);
#ifdef SNDDEBUG
  if (rc < 0) ST_Message("    MUS_ChainSong() returned %d\n", rc);
#endif
  rc = MUS_PlaySong(handle, snd_MusicVolume);
#ifdef SNDDEBUG
  if (rc < 0) ST_Message("    MUS_PlaySong() returned %d\n", rc);
#endif
#endif

   PlayMIDI( pmData, looping);
   // need to set volume again, because before midi device was closed
   SetMIDIVolume( pmData, snd_MusicVolume);

}

/*
 *
 *                                 SOUND FX API
 *
 */
// Gets lump nums of the named sound.  Returns pointer which will be
// passed to I_StartSound() when you want to start an SFX.  Must be
// sure to pass this to UngetSoundEffect() so that they can be
// freed!

#define MCI_OWNERISPARENT   0x0001
#define MCI_STOPACTIVE      0x0002
#define MCI_ASYNCRENDEZVOUS 0x0004
#define MCI_RENDEZVOUS      0x0008
#define MCI_ASYNC           0x0010
#define MCI_REPEAT          0x0020
#define MCI_STOPONSUSPEND   0x0040

ULONG EXPENTRY mciPlayFile (HWND hwndOwner,        /* Ownerwindow */
PSZ  pszFile,          /* File */
ULONG ulFlags,         /* Flags */
PSZ  pszTitle,         /* Title */
HWND hwndViewport);    /* Viewport Window */


int I_GetSfxLumpNum(sfxinfo_t *sound)
{
  return W_GetNumForName(sound->lumpname);

}

// OS/2 DART stuff
// There are two buffers (for double buffering)
#define UINT8 signed short
#define UINT32 LONG

int iSoundThatIsPlaying = 0;
void CloseDARTAudio(VOID);
extern struct sfxinfo_s S_sfx[NUMSFX];

UINT8 OpenDARTAudio(UINT32 dwSamplingRate, UINT8 bDepth, UINT8 bMonoStereo, UINT8 bSignedUnsigned, UINT32 dwBufferSize);

#define SOUNBUFFERSIZE 65*1024
UINT8 soundBuffer[SOUNBUFFERSIZE+1];
extern HMTX  htmx_sucker_sem;
long biggest = 0;
//
// Retrieve the raw data lump index
//  for a given SFX name.
//


int I_StartSound (int id, void *data, int vol, int sep, int pitch, int priority)
{
#ifndef NO_DMX
	return SFX_PlayPatch(data, pitch, sep, vol, 0, 0);
#else
   static int first = 1;
   LONG size;

   if (first) {
      int i,j;
      // Generates volume lookup tables
      //  which also turn the unsigned samples
      //  into signed samples.
      for (i=0 ; i<128 ; i++)
         for (j=0 ; j<256 ; j++)
         vol_lookup[i*256+j] = (i*(j-128)*256)/127;

      // fill first sound buffer
      I_UpdateSound();
      // play it, so we can start events
      PlayDART (pmData);
      first = 0;
   }
   //iSoundThatIsPlaying = id;

   if (id < 1 || id > NUMSFX)
      return 0;

   size = W_LumpLength (S_sfx[id].lumpnum);

   if (size>0)
   {

      ULONG l;
      int paddedsize, slot = 0;
      unsigned char *raw;
    int         rightvol;
    int         leftvol;
  int           volume;



      // Separation, that is, orientation/stereo.
      //  range is: 1 - 256
      sep += 1;
      leftvol = volume - ((volume * sep * sep) >> 16); ///(256*256);
      sep = sep  - 257;
      rightvol = volume -((volume * sep * sep) >> 16);

      // Sanity check, clamp volume.
      if (rightvol < 0 || rightvol > 127)
         rightvol = 0;
         //I_Error("rightvol out of bounds");

      if (leftvol < 0 || leftvol > 127)
          leftvol = 0;  
         //I_Error("leftvol out of bounds");

      // Get the proper lookup table piece
      //  for this volume level???
      channelleftvol_lookup[slot]  = &vol_lookup[leftvol*256];
      channelrightvol_lookup[slot] = &vol_lookup[rightvol*256];

      raw = (unsigned char *)data;
      if (size > SOUNBUFFERSIZE) size = SOUNBUFFERSIZE;

      paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;
      
//      DosEnterCritSec ();
      if (DosRequestMutexSem (htmx_sucker_sem, 0 ) == 0)
      {
      for (l=0; l<size; l+=2)
      {
         signed short a, b;
         signed short c;
         unsigned int sample;
         int          dl;
         int          dr;

         sample = (unsigned int) *raw;
         dl = 0;
         dr = 0;
         // Add left and right part
         //  for this channel (sound)
         //  to the current data.
         // Adjust volume accordingly.
         dl += channelleftvol_lookup[ slot ][sample];
         dr += channelrightvol_lookup[ slot ][sample];

         a = (signed short)soundBuffer[l];
         dl = a + dl;
         dl = dl / (signed long)2;

         a = (signed short)soundBuffer[l+1];
         dr = a + dr;
         dr = dr / (signed long)2;

         if (dl > 0x7fff)
            soundBuffer[l] = 0x7fff;
         else if (dl < -0x8000)
            soundBuffer[l] = -0x8000;
         else
            soundBuffer[l] = dl;

         // Same for right hardware channel.
         if (dr > 0x7fff)
            soundBuffer[l+1] = 0x7fff;
         else if (dr < -0x8000)
            soundBuffer[l+1] = -0x8000;
         else
            soundBuffer[l+1] = dr;

         raw++;
         raw++;
      }
//      for (l=size ; l<paddedsize+8 ; l++)
//         soundBuffer[l] = (UINT8)128;

//      DosExitCritSec ();
      DosReleaseMutexSem (htmx_sucker_sem);
      }
   }
   return id;
#endif
}

void I_StopSound(int handle)
{
//  extern volatile long gDmaCount;
//  long waittocount;
#ifndef NO_DMX
  SFX_StopPatch(handle);
#endif
//  waittocount = gDmaCount + 2;
//  while (gDmaCount < waittocount) ;
   iSoundThatIsPlaying = 0;
}

int I_SoundIsPlaying(int handle)
{
#ifndef NO_DMX
  return SFX_Playing(handle);
#else
   return iSoundThatIsPlaying;
#endif
}

void I_UpdateSoundParams(int handle, int vol, int sep, int pitch)
{
#ifndef NO_DMX
  SFX_SetOrigin(handle, pitch, sep, vol);
#endif
}


//
// This function loops all active (internal) sound
//  channels, retrieves a given number of samples
//  from the raw sound data, modifies it according
//  to the current (internal) channel parameters,
//  mixes the per channel samples into the global
//  mixbuffer, clamping it to the allowed range,
//  and sets up everything for transferring the
//  contents of the mixbuffer to the (two)
//  hardware channels (left and right, that is).
//
// This function currently supports only 16bit.
//
void I_UpdateSound( void )
{
#ifdef SNDINTR
  // Debug. Count buffer misses with interrupt.
  static int misses = 0;
#endif


  // Mix current sound data.
  // Data, from raw sound, for right and left.
  register unsigned int	sample;
  register int		dl;
  register int		dr;

  // Pointers in global mixbuffer, left, right, end.
  signed short*		leftout;
  signed short*		rightout;
  signed short*		leftend;
  // Step in mixbuffer, left and right, thus two.
  int				step;

  // Mixing channel index.
  int				chan;

    // Left and right channel
    //  are in global mixbuffer, alternating.
   leftout = pmData->MixBuffers[ pmData->FillBuffer].pBuffer;
   rightout = leftout+1;// pmData->BufferParms.ulBufferSize/2;
      // next fill buffer
   pmData->FillBuffer++;
   if (pmData->FillBuffer >= pmData->BufferParms.ulNumBuffers)
       pmData->FillBuffer = 0;
   step = 2;

      // Determine end, for left channel only
      //  (right channel is implicit).
      // ulBufferSize is len in bytes (8 bit), ulBufferSize/2 is 16bit length
   leftend = leftout + pmData->BufferParms.ulBufferSize/2;

    // Mix sounds into the mixing buffer.
    // Loop over step*SAMPLECOUNT,
    //  that is 512 values for two channels.
   while (leftout != leftend)
   {
      // Reset left/right value.
      dl = 0;
      dr = 0;

      // Love thy L2 chache - made this a loop.
      // Now more channels could be set at compile time
      //  as well. Thus loop those  channels.
      for ( chan = 0; chan < NUM_CHANNELS; chan++ )
      {
         // Check channel, if active.
         if (channels[ chan ])
         {
            // Get the raw data from the channel.
            sample = *channels[ chan ];
            // Add left and right part
            //  for this channel (sound)
            //  to the current data.
            // Adjust volume accordingly.
            dl += channelleftvol_lookup[ chan ][sample];
            dr += channelrightvol_lookup[ chan ][sample];
            // Increment index ???
            channelstepremainder[ chan ] += channelstep[ chan ];
            // MSB is next sample???
            channels[ chan ] += channelstepremainder[ chan ] >> 16;
            // Limit to LSB???
            channelstepremainder[ chan ] &= 65536-1;

            // Check whether we are done.
            if (channels[ chan ] >= channelsend[ chan ])
                channels[ chan ] = 0;
         }
      }

      // Clamp to range. Left hardware channel.
      // Has been char instead of short.
      // if (dl > 127) *leftout = 127;
      // else if (dl < -128) *leftout = -128;
      // else *leftout = dl;

      dl <<= 4;
      dr <<= 4;
      if (dl > 0x7fff)
          *leftout = 0x7fff;
      else if (dl < -0x8000)
          *leftout = -0x8000;
      else
          *leftout = dl;

      // Same for right hardware channel.
      if (dr > 0x7fff)
          *rightout = 0x7fff;
      else if (dr < -0x8000)
          *rightout = -0x8000;
      else
          *rightout = dr;

      // Increment current pointers in mixbuffer.
      leftout += step;
      rightout += step;
   }
/*
#ifndef OS2
#ifdef SNDINTR
    // Debug check.
    if ( flag )
    {
      misses += flag;
      flag = 0;
    }

    if ( misses > 10 )
    {
      fprintf( stderr, "I_SoundUpdate: missed 10 buffer writes\n");
      misses = 0;
    }

    // Increment flag for update.
    flag++;
#endif
#endif
*/
}

/*
 *
 *                                                      SOUND STARTUP STUFF
 *
 *
 */

//
// Why PC's Suck, Reason #8712
//

void I_sndArbitrateCards(void)
{
//	char tmp[160];
  boolean gus, adlib, pc, sb, midi;
//  int rc, opltype, wait, dmxlump, pi, mputype;

  snd_MusicDevice = snd_DesiredMusicDevice;
  snd_SfxDevice = snd_DesiredSfxDevice;

  // check command-line parameters- overrides config file
  //
  if (M_CheckParm("-nosound")) snd_MusicDevice = snd_SfxDevice = snd_none;
  if (M_CheckParm("-nosfx")) snd_SfxDevice = snd_none;
  if (M_CheckParm("-nomusic")) snd_MusicDevice = snd_none;

  if (snd_MusicDevice > snd_MPU && snd_MusicDevice <= snd_MPU3)
	snd_MusicDevice = snd_MPU;
  if (snd_MusicDevice == snd_SB)
	snd_MusicDevice = snd_Adlib;
  if (snd_MusicDevice == snd_PAS)
	snd_MusicDevice = snd_Adlib;

  // figure out what i've got to initialize
  //
  gus = snd_MusicDevice == snd_GUS || snd_SfxDevice == snd_GUS;
  sb = snd_SfxDevice == snd_SB || snd_MusicDevice == snd_SB;
  adlib = snd_MusicDevice == snd_Adlib ;
  pc = snd_SfxDevice == snd_PC;
  midi = snd_MusicDevice == snd_MPU;

#ifndef NO_DMX
  // initialize whatever i've got
  //
  if (gus)
  {
	if (GF1_Detect()) ST_Message("    Dude.  The GUS ain't responding.\n");
	else
	{
	  dmxlump = W_GetNumForName("dmxgus");
	  GF1_SetMap(W_CacheLumpNum(dmxlump, PU_CACHE), lumpinfo[dmxlump].size);
	}

  }
  if (sb)
  {
	if(debugmode)
	{
	  ST_Message("  Sound cfg p=0x%x, i=%d, d=%d\n",
	  	snd_SBport, snd_SBirq, snd_SBdma);
	}
	if (SB_Detect(&snd_SBport, &snd_SBirq, &snd_SBdma, 0))
	{
	  ST_Message("    SB isn't responding at p=0x%x, i=%d, d=%d\n",
	  	snd_SBport, snd_SBirq, snd_SBdma);
	}
	else SB_SetCard(snd_SBport, snd_SBirq, snd_SBdma);

	if(debugmode)
	{
	  ST_Message("    SB_Detect returned p=0x%x, i=%d, d=%d\n",
	  	snd_SBport, snd_SBirq, snd_SBdma);
	}
  }

  if (adlib)
  {
	if (AL_Detect(&wait,0))
	{
	  	ST_Message("    Dude.  The Adlib isn't responding.\n");
	}
	else
	{
		AL_SetCard(wait, W_CacheLumpName("genmidi", PU_STATIC));
	}
  }

  if (midi)
  {
	if (debugmode)
	{
		ST_Message("    cfg p=0x%x\n", snd_Mport);
	}

	if (MPU_Detect(&snd_Mport, &i))
	{
	  ST_Message("    The MPU-401 isn't reponding @ p=0x%x.\n", snd_Mport);
	}
	else MPU_SetCard(snd_Mport);
  }
#endif
}

// inits all sound stuff

void I_StartupSound (void)
{
   int rc = 0;
//   int i = 0;

   if (debugmode)
      ST_Message("Preparing DART");

   //MIDI part
   InitDART (pmData);
   //SFX part
   memset (soundBuffer, 128, SOUNBUFFERSIZE);
   rc = OpenDARTAudio (SAMPLERATE, 8, 2, 0, MIXBUFFERSIZE);
   //rc = OpenDARTAudio (11025, 8, 2, 0, 1024);
   //SamplingRate
   //Depth, bits pr sample
   //bMonoStereo  1 = mono 2 = stereo
   //bSignedUnSigned not used
   //bufferSize
   if (debugmode)
      ST_Message("OpenDARTAudio: %d", rc);

   rc = 0;
   if (debugmode)
      ST_Message("I_StartupSound: Hope you hear a pop.");

  // initialize dmxCodes[]
  dmxCodes[0] = 0;
  dmxCodes[snd_PC] = AHW_PC_SPEAKER;
  dmxCodes[snd_Adlib] = AHW_ADLIB;
  dmxCodes[snd_SB] = AHW_SOUND_BLASTER;
  dmxCodes[snd_PAS] = AHW_MEDIA_VISION;
  dmxCodes[snd_GUS] = AHW_ULTRA_SOUND;
  dmxCodes[snd_MPU] = AHW_MPU_401;
  dmxCodes[snd_MPU2] = AHW_MPU_401;
  dmxCodes[snd_MPU3] = AHW_MPU_401;
  dmxCodes[snd_AWE] = AHW_AWE32;
  dmxCodes[snd_CDMUSIC] = 0;

  // inits sound library timer stuff
  I_StartupTimer();

  // pick the sound cards i'm going to use
  //
  I_sndArbitrateCards();

  if (debugmode)
  {
	ST_Message("    Music device #%d & dmxCode=%d,", snd_MusicDevice,
	  dmxCodes[snd_MusicDevice]);
	ST_Message(" Sfx device #%d & dmxCode=%d\n", snd_SfxDevice,
	  dmxCodes[snd_SfxDevice]);
  }

  // inits DMX sound library
  ST_Message("    Calling DMX_Init...");
#ifndef NO_DMX
  rc = DMX_Init(SND_TICRATE, SND_MAXSONGS, dmxCodes[snd_MusicDevice],
	dmxCodes[snd_SfxDevice]);
#endif

  if (debugmode)
  {
	ST_Message(" DMX_Init() returned %d\n", rc);
  }

}

// shuts down all sound stuff

void I_ShutdownSound (void)
{
#ifndef NO_DMX
   DMX_DeInit();
#endif
   I_ShutdownTimer();
   CloseDARTAudio ();
}

void I_SetChannels(int channels)
{
#ifndef NO_DMX
  WAV_PlayMode(channels, SND_SAMPLERATE);
#endif
}

