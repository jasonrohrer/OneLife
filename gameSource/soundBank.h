#ifndef SOUND_BANK_INCLUDED
#define SOUND_BANK_INCLUDED


#include "minorGems/game/game.h"

#include "SoundUsage.h"



typedef struct SoundRecord {
        int id;

        // NULL if sound not loaded
        SoundSpriteHandle sound;
        SoundSpriteHandle reverbSound;
        

        char loading;

        int numStepsUnused;
        

        int liveUseageCount;
        
    } SoundRecord;



// returns number of sounds that need to be loaded (or reverbs regenerated)
int initSoundBankStart( char *outRebuildingCache );


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


#endif
