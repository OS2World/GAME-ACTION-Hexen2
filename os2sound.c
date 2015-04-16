//
//   Sound stubs...
//
#ifdef __OS2_VERSION_JUL__

void TSM_DelService (int tsm_ID)
{
}
void TSM_Remove (void)
{
}
int TSM_NewService (void *aa, int a, int b, int c)
{
   return 1;
}
void TSM_Install(int SND_TICRATE)
{
}

void MUS_PauseSong (int handle)
{
   handle = 0;
   PauseMIDI( pmData);
}
void MUS_ResumeSong (int handle)
{
   handle = 0;
   ResumeMIDI( pmData);
}
void MUS_SetMasterVolume (int volume)
{
   snd_SfxVolume = volume;
}
int MUS_RegisterSong (int *data)
{
   int len;

   return RegisterMIDI( pmData, data, len);
}
int MUS_UnregisterSong (int handle)
{
   handle = 0;
   // remove files
   unlink( "hexen.mid");
   unlink( "hexen.mus");
   return 1;
}
int MUS_QrySongPlaying (int handle)
{
   return 0;
}
int MUS_StopSong (int handle)
{
   return 0;
}
int MUS_ChainSong (int handle, int looping)
{
   return 1;
}
int MUS_PlaySong (int handle,int snd_MusicVolume)
{
   PlayMIDI( pmData, looping);
   // need to set volume again, because before midi device was closed
   SetMIDIVolume( pmData, snd_MusicVolume);
}

void SFX_SetOrigin (int handle, int pitch, int sep, int vol)
{
}
void SFX_StopPatch (int handle)
{
}
int SFX_PlayPatch (int *data, int pitch, int sep, int vol, int a, int b)
{
   return 1;
}
int SFX_Playing (int handle)
{
   return 1;
}

int DMX_Init (int SND_TICRATE, int SND_MAXSONGS, int a, int b)
{
   return 1;
}
int AL_Detect(int *a, int b)
{
   return 1;
}
int MPU_Detect(int *snd_Mport, int *i)
{
   return 1;
}
void MPU_SetCard (int snd_Mport)
{
}
void DMX_DeInit()
{
}
void WAV_PlayMode (int channels, int SND_SAMPLERATE)
{
}
void AL_SetCard (int wait, void *a)
{
}
int SB_Detect(int *snd_SBport, int *snd_SBirq, int *snd_SBdma, int a)
{
   return 1;
}
int GF1_Detect()
{
   return 1;
}
void SB_SetCard (int snd_SBport, int snd_SBirq, int snd_SBdma)
{
}void GF1_SetMap (void *a, int size)
{
}
#endif

