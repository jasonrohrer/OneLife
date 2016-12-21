#include "audioNoClip.h"

#include <math.h>
#include <stdio.h>



static int holdTime;
static int decayTime;

static int currentHoldTime;


static double gain;

static double maxVolume;

static double gainDecayPerSample;



void resetAudioNoClip( double inMaxVolume, int inHoldTimeInSamples, 
                       int inDecayTimeInSamples ) {
    maxVolume = inMaxVolume;

    gain = 1.0;

    decayTime = inDecayTimeInSamples;

    holdTime = inHoldTimeInSamples;
    
    currentHoldTime = 0;
    }



void audioNoClip( double *inSamplesL, double *inSamplesR, int inNumSamples ) {
    
    for( int i=0; i<inNumSamples; i++ ) {
        
        double maxValL = fabs( inSamplesL[i] );
        
        double maxValR = fabs( inSamplesR[i] );
        
        double maxVal = maxValL;
        
        if( maxValR > maxValL ) {
            maxVal = maxValR;
            }
        
        // do nothing if signal is not clipping and gain is full
        if( gain != 1.0 || maxVal > maxVolume ) {
            
            if( gain != 1.0 && 
                ( gain + gainDecayPerSample ) * maxVal <= maxVolume ) {
                
                currentHoldTime++;
                
                if( currentHoldTime > holdTime ) {
                    
                    // printf( "Gain decay step at sample %d, "
                    //        "current hold time %d\n", i, currentHoldTime );
                    
                    gain += gainDecayPerSample;
                
                    if( gain > 1.0 ) {
                        gain = 1.0;
                        }
                    }
                }
            else if( maxVal * gain > maxVolume ) {
                
                // new peak

                // restart hold
                currentHoldTime = 0;
                
                // printf( "Current hold time resetA at sample %d\n", i );
                
                // prevent clipping
                
                // printf( "Max val = %f, old gain = %f\n", maxVal, gain );

                gain = maxVolume / maxVal;
                // printf( "   new gain = %f\n", gain );
                
                gainDecayPerSample = ( 1.0 - gain ) / decayTime;
                }
            else {
                // not a new peak, not way under peak value
                // hit old peak again, continue holding
                currentHoldTime = 0;
                //printf( "Current hold time resetB at sample %d\n", i );

                }
            

            inSamplesL[i] *= gain;
            inSamplesR[i] *= gain;
            }
        }

    
    }

