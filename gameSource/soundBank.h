#ifndef SOUND_BANK_INCLUDED
#define SOUND_BANK_INCLUDED


#include "minorGems/game/game.h"

#include "SoundUsage.h"



typedef struct SoundRecord {
        int id;

        // NULL if image not loaded
        SoundSpriteHandle sound;

        char loading;

        int numStepsUnused;
        

        int liveUseageCount;
        
    } SoundRecord;



// no caching needed, so do it in one step
void initSoundBank();


// can only be called after bank init is complete
int getMaxSoundID();



void freeSoundBank();


// processing for dynamic, asynchronous sound loading
void stepSoundBank();


// music headroom is fraction of full volume reserved for music
// sound effect volume will be scaled so that inMaxSimultaneousSoundEffects
// can be played together without clipping (each SoundUsage can have its
// own volume scaling which can further reduce its volume)
// This is per channel (left and right)
void setVolumeScaling( int inMaxSimultaneousSoundEffects,
                       double inMusicHeadroom );


void playSound( int inID, double inVolumeTweak = 1.0,
                double inStereoPosition  = 0.5 );

void playSound( SoundUsage inUsage,
                double inStereoPosition  = 0.5 );


// applies stereo position, distance fade, and distance reverb automatically
// sounds at 0,0 are dead center and full volume
// vector is in world tile units
void playSound( SoundUsage inUsage,
                doublePair inVectorFromCameraToSoundSource );



// true if started
char startRecordingSound();

// auto-trims sound and returns ID of new sound in bank
// returns -1 if recording fails
int stopRecordingSound();


// returns true if sound is already loaded
char markSoundLive( int inID );



// for tracking of sounds used by recording widgets that may or 
// may not be saved in objects or animations (for auto-delete of
// sounds that no one is referencing anymore)
void countLiveUse( int inID );

void unCountLiveUse( int inID );


void checkIfSoundStillNeeded( int inID );

#endif
