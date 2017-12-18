#ifndef SOUND_USAGE_INCLUDED
#define SOUND_USAGE_INCLUDED


typedef struct SoundUsage {
        int numSubSounds;
        
        int *ids;

        double *volumes;
        
    } SoundUsage;

extern SoundUsage blankSoundUsage;


// scans a usage from a string
// format:
// id:vol#id:vol#id:vol
SoundUsage scanSoundUsage( char *inString );


// returns a internally-allocated buffer
// NOT destroyed by caller, and not thread safe
const char *printSoundUsage( SoundUsage inUsage );


// frees memory and sets sound usage back to 0 subsounds
void clearSoundUsage( SoundUsage inUsage );


// don't need to call if printSoundUsage never called
void freeSoundUsagePrintBuffer();



#endif
