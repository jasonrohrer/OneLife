#ifndef SOUND_BANK_INCLUDED
#define SOUND_BANK_INCLUDED


#include "minorGems/game/game.h"


typedef struct SoundRecord {
        int id;

        // NULL if image not loaded
        SoundSpriteHandle sound;

        char loading;

        int numStepsUnused;

    } SoundRecord;



// no caching needed, so do it in one step
void initSoundBank();


// can only be called after bank init is complete
int getMaxSoundID();



void freeSoundBank();


// processing for dynamic, asynchronous sound loading
void stepSoundBank();



void playSound( int inID, double inVolumeTweak = 1.0,
                double inStereoPosition  = 0.5  );

// true if started
char startRecordingSound();

// auto-trims sound and returns ID of new sound in bank
// returns -1 if recording fails
int stopRecordingSound();


// returns true if sound is already loaded
char markSoundLive( int inID );


#endif
