#include "SoundUsage.h"

#include <stdlib.h>

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"



SoundUsage blankSoundUsage = { 0, NULL, NULL };


SoundUsage scanSoundUsage( char *inString ) {
    SimpleVector<int> idVector;
    SimpleVector<double> volVector;

    int numParts = 0;

    char **parts = split( inString, "#", &numParts );
    

    for( int i=0; i<numParts; i++ ) {
        int id = -1;
        double vol = 1.0;
        
        sscanf( parts[i], "%d:%lf", &id, &vol );
        
        if( id != -1 && vol >=0 && vol <= 1.0 ) {
            idVector.push_back( id );
            volVector.push_back( vol );
            }
        delete [] parts[i];
        }
    delete [] parts;

    
    if( idVector.size() > 0 ) {
        
        SoundUsage u = { idVector.size(), idVector.getElementArray(),
                         volVector.getElementArray() };
        return u;
        }
    else {
        return blankSoundUsage;
        }
    }




static int printBufferLength = 0;
static char *printBuffer = NULL;



const char *printSoundUsage( SoundUsage inUsage ) {
    
    if( printBufferLength < inUsage.numSubSounds * 20 ) {
        printBufferLength = 0;
        delete [] printBuffer;
        printBuffer = NULL;    
        }
    
    if( printBuffer == NULL ) {
        printBufferLength = inUsage.numSubSounds * 20;
        printBuffer = new char[ printBufferLength ];
        }


    int numPrinted = 0;
    
    for( int i=0; i<inUsage.numSubSounds; i++ ) {
        const char *sep = "#";
        
        if( i == 0 ) {
            sep = "";
            }
        
        numPrinted += 
            sprintf( &( printBuffer[numPrinted] ), "%s%d:%f",
                     sep, inUsage.ids[i], inUsage.volumes[i] );
        }

    return printBuffer;
    }



void clearSoundUsage( SoundUsage inUsage ) {
    if( inUsage.ids != NULL ) {
        delete [] inUsage.ids;
        inUsage.ids = NULL;
        }
    if( inUsage.volumes != NULL ) {
        delete [] inUsage.volumes;
        inUsage.volumes = NULL;
        }
    inUsage.numSubSounds = 0;
    }    




void freeSoundUsagePrintBuffer() {
    if( printBuffer != NULL ) {
        delete [] printBuffer;
        printBuffer = NULL;
        }
    printBufferLength = 0;
    }

