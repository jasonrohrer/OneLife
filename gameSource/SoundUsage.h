#ifndef SOUND_USAGE_INCLUDED
#define SOUND_USAGE_INCLUDED


typedef struct SoundUsage {
        int numSubSounds;
        
        int *ids;

        double *volumes;
        
    } SoundUsage;

extern SoundUsage blankSoundUsage;



// for an individual play instance of a SoundUsage
typedef struct SoundUsagePlay {
        int id;
        double volume;
    } SoundUsagePlay;



// scans a usage from a string
// format:
// id:vol#id:vol#id:vol
SoundUsage scanSoundUsage( char *inString );


// returns a internally-allocated buffer
// NOT destroyed by caller, and not thread safe
const char *printSoundUsage( SoundUsage inUsage );


// frees memory and sets sound usage back to 0 subsounds
void clearSoundUsage( SoundUsage *inUsage );


// don't need to call if printSoundUsage never called
void freeSoundUsagePrintBuffer();


// true if sound id inID is used by inUsage
char doesUseSound( SoundUsage inUsage, int inID );


// copies usage, allocating new memory
SoundUsage copyUsage( SoundUsage inUsage );


SoundUsagePlay playRandom( SoundUsage inUsage );


// causes reallocation
void addSound( SoundUsage *inUsage, int inID, double inVolume );
void removeSound( SoundUsage *inUsage, int inIndex );


char equal( SoundUsage inA, SoundUsage inB );



#endif
