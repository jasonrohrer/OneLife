#include "minorGems/game/game.h"
#include <math.h>


// overal loudness of music
static double musicLoudness = 0;
static double musicTargetLoudness = 0;

extern double musicHeadroom;


// set so that a full 0-to-1 loudness transition happens 
// in 1 second
static double loudnessChangePerSample;


#include "ogg.h"


OGGHandle musicOGG = NULL;

static double chunkLengthSeconds = 30;

static int numChunks = 0;

static int numTimesReachedEnd = 0;

static int musicNumSamples = 0;



void initMusicPlayer() {
    File musicFile( NULL, "music.ogg" );
    
    musicOGG = openOGG( &musicFile );
    
    int sampleRate = getSampleRate();

    loudnessChangePerSample = 1.0 / sampleRate;


    if( musicOGG != NULL ) {
        musicNumSamples = getOGGTotalSamples( musicOGG );
        
        double numSeconds = musicNumSamples / (double) sampleRate;
        
        numChunks = (int)floor( numSeconds / chunkLengthSeconds );
        
        numTimesReachedEnd = 0;
        
        // start on final segment

        seekOGG( musicOGG, 
                 musicNumSamples - (int)( chunkLengthSeconds * sampleRate ) );
        }
    }


void freeMusicPlayer() {
    if( musicOGG != NULL ) {
        closeOGG( musicOGG );
        musicOGG = NULL;
        }
    }




// called by platform to get more samples
void getSoundSamples( Uint8 *inBuffer, int inLengthToFillInBytes ) {
    
    if( musicOGG == NULL ) {
        return;
        }


    // 2 bytes for each channel of stereo sample
    int numSamples = inLengthToFillInBytes / 4;

    
    float *samplesL = new float[ numSamples ];
    float *samplesR = new float[ numSamples ];

    int numRead = readNextSamplesOGG( musicOGG, numSamples,
                                      samplesL, samplesR );
    
    if( numRead < numSamples ) {
        // hit end

        // jump back to next starting point
        numTimesReachedEnd++;
        
        int numSegmentsToPlayThisTime = numTimesReachedEnd + 1;
        
        if( numSegmentsToPlayThisTime > numChunks ) {
            // start over from beginning
            numSegmentsToPlayThisTime = 1;
            }
        
        seekOGG( musicOGG, 
                 musicNumSamples - 
                 numSegmentsToPlayThisTime * 
                 (int)( chunkLengthSeconds * getSampleRate() ) );
        
        int numLeft = numSamples - numRead;
        
        int numReadB = readNextSamplesOGG( musicOGG, numLeft,
                                           &( samplesL[numRead] ), 
                                           &( samplesR[numRead ] ) );
        
        if( numReadB != numLeft ) {
            // error
            closeOGG( musicOGG );
            musicOGG = NULL;
            }
        }
    

    // now copy samples into Uint8 buffer
    // while also adjusting loudness of whole mix
    char loudnessChanging = false;
    if( musicLoudness != musicTargetLoudness ) {
        loudnessChanging = true;
        }
    
    int streamPosition = 0;
    for( int i=0; i != numSamples; i++ ) {
        samplesL[i] *= musicLoudness * musicHeadroom;
        samplesR[i] *= musicLoudness * musicHeadroom;
    
        if( loudnessChanging ) {
            
            if( musicLoudness < musicTargetLoudness ) {
                musicLoudness += loudnessChangePerSample;
                if( musicLoudness > musicTargetLoudness ) {
                    musicLoudness = musicTargetLoudness;
                    loudnessChanging = false;
                    }
                }
            else if( musicLoudness > musicTargetLoudness ) {
                musicLoudness -= loudnessChangePerSample;
                if( musicLoudness < musicTargetLoudness ) {
                    musicLoudness = musicTargetLoudness;
                    loudnessChanging = false;
                    }
                }
            }
        
        
        
        Sint16 intSampleL = (Sint16)( lrint( 32767 * samplesL[i] ) );
        Sint16 intSampleR = (Sint16)( lrint( 32767 * samplesR[i] ) );
        
        inBuffer[ streamPosition ] = (Uint8)( intSampleL & 0xFF );
        inBuffer[ streamPosition + 1 ] = (Uint8)( ( intSampleL >> 8 ) & 0xFF );
        
        inBuffer[ streamPosition + 2 ] = (Uint8)( intSampleR & 0xFF );
        inBuffer[ streamPosition + 3 ] = (Uint8)( ( intSampleR >> 8 ) & 0xFF );
        
        streamPosition += 4;
        }

    }



// need to synch these with audio thread

void setMusicLoudness( double inLoudness ) {
    lockAudio();
    
    musicTargetLoudness = inLoudness;
    
    unlockAudio();
    }



double getMusicLoudness() {
    return musicLoudness;
    }
