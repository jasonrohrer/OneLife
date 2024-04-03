#ifndef SOUND_BANK_INCLUDED
#define SOUND_BANK_INCLUDED


#include "minorGems/game/game.h"

#include "SoundUsage.h"



typedef struct SoundRecord {
        int id;

        unsigned int hash;

        // full SHA1 hash
        // not computed for every sound in bank, since we can always
        // check the file data
        // but for live-only sounds (like for mods) we compute
        // this, since we don't have file data to check
        char *sha1Hash;
        

        // NULL if sound not loaded
        SoundSpriteHandle sound;
        SoundSpriteHandle reverbSound;
        

        char loading;

        int numStepsUnused;
        

        int liveUseageCount;

        double lengthInSeconds;
        
    } SoundRecord;



// returns number of sounds that need to be loaded (or reverbs regenerated)
int initSoundBankStart( char *outRebuildingCache,
                        char inComputeSoundHashes = false );


// returns progress... ready for Finish when progress == 1.0
float initSoundBankStep();
void initSoundBankFinish();



// can only be called after bank init is complete
int getMaxSoundID();



void freeSoundBank();


// processing for dynamic, asynchronous sound loading
void stepSoundBank();


// returns NULL if asynchronous loading process hasn't failed
// returns internally-allocated string (destroyed internally) if
//    loading process fails.  String is name of file that failed to load
char *getSoundBankLoadFailure();



// music headroom is fraction of full volume reserved for music
// sound effect volume will be scaled so that inMaxSimultaneousSoundEffects
// can be played together without clipping (each SoundUsage can have its
// own volume scaling which can further reduce its volume)
// This is per channel (left and right)
void setVolumeScaling( int inMaxSimultaneousSoundEffects,
                       double inMusicHeadroom );


// defaults to 1.0
void setSoundEffectsLoudness( double inLoudness );

double getSoundEffectsLoudness();

// defaults to on
void setSoundEffectsOff( char inOff );



// reverb playing defaults to on
// note that with new mixing method, even if inReverbMix is 0, reverb
// plays at specified volume (inReverbMix controls dry sound level).
void disableReverb( char inDisable );


void playSound( int inID, double inVolumeTweak = 1.0,
                double inStereoPosition  = 0.5, 
                double inReverbMix = 0.0 );

void playSound( SoundUsage inUsage,
                double inStereoPosition  = 0.5, 
                double inReverbMix = 0.0 );


// applies stereo position, distance fade, and distance reverb automatically
// sounds at 0,0 are dead center and full volume
// vector is in world tile units
void playSound( SoundUsage inUsage,
                doublePair inVectorFromCameraToSoundSource );


// leverage stereo positioning code on a raw sound sprite
// still offer a volume tweak on top of stereo positioning
void playSound( SoundSpriteHandle inSoundSprite,
                double inVolumeTweak,
                doublePair inVectorFromCameraToSoundSource );



// gets length of a sound in bank
double getSoundLengthInSeconds( int inID );




// true if started
char startRecordingSound();

// auto-trims and EQs sound and returns ID of new sound in bank
// returns -1 if recording fails
int stopRecordingSound();


// returns true if sound is already loaded
char markSoundLive( int inID );

// returns true if all subsounds are already loaded
char markSoundUsageLive( SoundUsage inUsage );



// for tracking of sounds used by recording widgets that may or 
// may not be saved in objects or animations (for auto-delete of
// sounds that no one is referencing anymore)
void countLiveUse( int inID );

void unCountLiveUse( int inID );

void countLiveUse( SoundUsage inUsage );

void unCountLiveUse( SoundUsage inUsage );



void checkIfSoundStillNeeded( int inID );


void printOrphanedSoundReport();


char doesSoundExist( int inID );




// returns ID of sound if one exists matching these settings
// returns -1 if not
int doesSoundRecordExist(
    int inNumSoundFileBytes,
    unsigned char *inSoundFileData );



// adds sound to bank from raw sound file data
// inType can also be OGG
// inSoundFileData destroyed by caller
// set inSaveToDisk to true to save into sounds folder on disk
// returns new sound ID, or -1 if parsing sound from inSoundFileData fails
int addSoundToBank( int inNumSoundFileBytes,
                    unsigned char *inSoundFileData,
                    const char *inType = "AIFF",
                    char inSaveToDisk = false,
                    const char *inAuthorTag = NULL );


// frees memory associated with reverb filter
// This is allocated during initSoundBankStart
// it is also used by addSoundToLiveBank
// So, after loading mods is done, call doneApplyingReverb
// to free the filter resources.
// (future calls to addSoundToLiveBank won't apply a reverb)
void doneApplyingReverb();


#endif
