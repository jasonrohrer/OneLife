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

static double crossFadeSeconds = 5;

static int crossFadeSamples;

static int numChunks = 0;

static int numTimesReachedEnd = 0;

static int musicNumSamples = 0;

static char musicStarted = false;

static int currentSamplePos = 0;



// one per chunk
// the crossFadeSamples samples that occur BEFORE the coresponding chunk
static float **chunkPreFadeSamplesL = NULL;
static float **chunkPreFadeSamplesR = NULL;




void initMusicPlayer() {
    File musicFile( NULL, "music.ogg" );
    
    musicOGG = openOGG( &musicFile );
    
    int sampleRate = getSampleRate();

    loudnessChangePerSample = 1.0 / sampleRate;

    crossFadeSamples = sampleRate * crossFadeSeconds;

    if( musicOGG != NULL ) {
        musicNumSamples = getOGGTotalSamples( musicOGG );    

        double numSeconds = musicNumSamples / (double) sampleRate;
        
        numChunks = (int)floor( numSeconds / chunkLengthSeconds );


        
        chunkPreFadeSamplesL = new float*[ numChunks ];
        chunkPreFadeSamplesR = new float*[ numChunks ];
        
        // 0 is last chunk in song
        for( int i=0; i<numChunks; i++ ) {
            chunkPreFadeSamplesL[i] = new float[ crossFadeSamples ];
            chunkPreFadeSamplesR[i] = new float[ crossFadeSamples ];
            
            for( int s=0; s<crossFadeSamples; s++ ) {
                chunkPreFadeSamplesL[i][s] = 0;
                chunkPreFadeSamplesR[i][s] = 0;
                }


            int chunkStartSample = musicNumSamples - 
                ( i + 1 ) * chunkLengthSeconds * sampleRate;
            
            int preStartSample = chunkStartSample - crossFadeSamples;
            
            int preOffset = 0;
            
            int numToRead = crossFadeSamples;
            
            if( preStartSample < 0 ) {
                preOffset = -preStartSample;
                
                numToRead += preStartSample;
                
                preStartSample = 0;
                }
            
            seekOGG( musicOGG, preStartSample );
            

            readNextSamplesOGG( 
                musicOGG, numToRead,
                &( chunkPreFadeSamplesL[i][ preOffset ] ),
                &( chunkPreFadeSamplesR[i][ preOffset ] ) );
            }
        }
    }


void restartMusic( double inAge ) {

    // for testing
    //inAge = 0;
    

    double ageSeconds = inAge * 60;

    double numChunksAlreadyPassed = ageSeconds / chunkLengthSeconds;
    

    int sampleRate = getSampleRate();
    
        
    numTimesReachedEnd = 0;
    
    // walk through part of song that we've already passed
    while( numChunksAlreadyPassed > numTimesReachedEnd + 1 ) {
        numChunksAlreadyPassed -= ( numTimesReachedEnd + 1 );
        
        numTimesReachedEnd++;
        }
    
    int numChunksToPlayThisTime = numTimesReachedEnd + 1;
    
    if( numChunksToPlayThisTime > numChunks ) {
        // music already done!
        return;
        }
    
    int numExtraSamples = 
        (int)( numChunksAlreadyPassed * chunkLengthSeconds *sampleRate );

    
    currentSamplePos = musicNumSamples + 
        numExtraSamples - 
        numChunksToPlayThisTime * 
        (int)( chunkLengthSeconds * sampleRate );
    
    seekOGG( musicOGG, currentSamplePos );
    
    
    musicStarted = true;
    
    // fade in at start of music
    musicLoudness = 0;
    }


void instantStopMusic() {
    musicStarted = false;
    }


static float *samplesL = NULL;
static float *samplesR = NULL;



void freeMusicPlayer() {
    if( musicOGG != NULL ) {
        closeOGG( musicOGG );
        musicOGG = NULL;

        for( int i=0; i<numChunks; i++ ) {
            delete [] chunkPreFadeSamplesL[i];
            delete [] chunkPreFadeSamplesR[i];
            }
        
        delete [] chunkPreFadeSamplesL;
        delete [] chunkPreFadeSamplesR;
        
        chunkPreFadeSamplesL = NULL;
        chunkPreFadeSamplesR = NULL;
        }

    
    if( samplesL != NULL ) {
        delete [] samplesL;
        samplesL = NULL;
        }
    if( samplesR != NULL ) {
        delete [] samplesR;
        samplesR = NULL;
        }
    }



void hintBufferSize( int inLengthToFillInBytes ) {
    // 2 bytes for each channel of stereo sample
    int numSamples = inLengthToFillInBytes / 4;

    if( samplesL != NULL ) {
        delete [] samplesL;
        samplesL = NULL;
        }
    if( samplesR != NULL ) {
        delete [] samplesR;
        samplesR = NULL;
        }
    

    samplesL = new float[ numSamples ];
    samplesR = new float[ numSamples ];

    for( int i=0; i<numSamples; i++ ) {
        samplesL[i] = 0;
        samplesR[i] = 0;
        }    
    }



// called by platform to get more samples
void getSoundSamples( Uint8 *inBuffer, int inLengthToFillInBytes ) {
    
    if( musicOGG == NULL || !musicStarted ) {
        return;
        }


    int sampleRate = getSampleRate();
    

    // 2 bytes for each channel of stereo sample
    int numSamples = inLengthToFillInBytes / 4;

    if( samplesL == NULL ) {
        // never got hint
        hintBufferSize( inLengthToFillInBytes );
        }


    int startSamplePos = currentSamplePos;
    
    int numRead = readNextSamplesOGG( musicOGG, numSamples,
                                      samplesL, samplesR );

    currentSamplePos += numRead;
    
    int endSamplePos = startSamplePos + numRead;

    int crossFadeSamples = crossFadeSeconds * sampleRate;

    if( numTimesReachedEnd < numChunks - 1
        && 
        musicNumSamples - endSamplePos < crossFadeSamples ) {

        // at end of song that needs to crossfade

        
        // find first chunk to play next time
        int numChunksToPlayNextTime = numTimesReachedEnd + 2;
        
        float *samplesLNext = 
            chunkPreFadeSamplesL[ numChunksToPlayNextTime - 1 ];
        float *samplesRNext = 
            chunkPreFadeSamplesR[ numChunksToPlayNextTime - 1 ];
        
        for( int i=0; i<numRead; i++ ) {
                
            int absIndex = i + startSamplePos;
                
            if( musicNumSamples - absIndex <= crossFadeSamples ) {
                    
                int crossFadeI = crossFadeSamples -
                    ( musicNumSamples - absIndex );
                    
                // t in [-1, 1]
                double t = 
                    ( 2 * crossFadeI / (double)crossFadeSamples ) - 1;
                
                
                double thisWeight = sqrt( 0.5 * ( 1 - t ) );
                double nextWeight = sqrt( 0.5 * ( 1 + t ) );
                
                    
                samplesL[i] = 
                    thisWeight * samplesL[i] + 
                    nextWeight * samplesLNext[crossFadeI];
                    
                samplesR[i] = 
                    thisWeight * samplesR[i] + 
                    nextWeight * samplesRNext[crossFadeI];
                
                }
            }
        }
    
    
    
    if( numRead < numSamples ) {
        // hit end

        // jump back to next starting point
        numTimesReachedEnd++;
        
        int numChunksToPlayThisTime = numTimesReachedEnd + 1;
        
        if( numChunksToPlayThisTime > numChunks ) {
            // end of music
            musicStarted = false;
            
            // fill rest with zero
            for( int i=numRead; i<numSamples; i++ ) {
                samplesL[i] = 0;
                samplesR[i] = 0;
                }
            }
        else {
            
            currentSamplePos =
                musicNumSamples - 
                numChunksToPlayThisTime * 
                (int)( chunkLengthSeconds * sampleRate );
            
            seekOGG( musicOGG, currentSamplePos );
            
            int numLeft = numSamples - numRead;
            
            int numReadB = readNextSamplesOGG( musicOGG, numLeft,
                                               &( samplesL[numRead] ), 
                                               &( samplesR[numRead ] ) );
        
            currentSamplePos += numReadB;
            
            if( numReadB != numLeft ) {
                // error
                closeOGG( musicOGG );
                musicOGG = NULL;
                musicStarted = false;
                
                // fill rest with zero
                for( int i=numRead + numReadB; i<numSamples; i++ ) {
                    samplesL[i] = 0;
                    samplesR[i] = 0;
                    }
                }
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
