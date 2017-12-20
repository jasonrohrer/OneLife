#include "SoundUsage.h"

#include <stdlib.h>

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/util/random/JenkinsRandomSource.h"


static JenkinsRandomSource randSource( 4583 );



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
        
        if( idVector.size() == 1 &&
            idVector.getElementDirect( 0 ) == -1 ) {
            return blankSoundUsage;
            }

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
    if( inUsage.numSubSounds == 0 ) {
        return "-1:0.0";
        }


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



void clearSoundUsage( SoundUsage *inUsage ) {
    if( inUsage->numSubSounds > 0 ) {
        if( inUsage->ids != NULL ) {
            delete [] inUsage->ids;
            }
        if( inUsage->volumes != NULL ) {
            delete [] inUsage->volumes;
            }
        }
    
    inUsage->ids = NULL;
    inUsage->volumes = NULL;
            
    inUsage->numSubSounds = 0;
    }    




void freeSoundUsagePrintBuffer() {
    if( printBuffer != NULL ) {
        delete [] printBuffer;
        printBuffer = NULL;
        }
    printBufferLength = 0;
    }




char doesUseSound( SoundUsage inUsage, int inID ) {
    for( int i=0; i<inUsage.numSubSounds; i++ ) {
        if( inUsage.ids[i] == inID ) {
            return true;
            }
        }
    return false;
    }



SoundUsage copyUsage( SoundUsage inUsage ) {
    SoundUsage out = blankSoundUsage;

    if( inUsage.numSubSounds == 0 ) {
        return out;
        }
    
    out.numSubSounds = inUsage.numSubSounds;
    
    out.ids = new int[ out.numSubSounds ];
    out.volumes = new double[ out.numSubSounds ];
    
    memcpy( out.ids, inUsage.ids, out.numSubSounds * sizeof( int ) );

    memcpy( out.volumes, inUsage.volumes, 
            out.numSubSounds * sizeof( double ) );

    return out;
    }



SoundUsagePlay playRandom( SoundUsage inUsage ) {
    int pick = randSource.getRandomBoundedInt( 0, inUsage.numSubSounds - 1 );
    
    SoundUsagePlay p = { inUsage.ids[pick], inUsage.volumes[pick] };
    return p;
    }



void addSound( SoundUsage *inUsage, int inID, double inVolume ) {
    int newNum = inUsage->numSubSounds + 1;
    int *newIDs = new int[ newNum ];
    double *newVols = new double[ newNum ];
    
    if( inUsage->numSubSounds > 0 ) {
        memcpy( newIDs, inUsage->ids, inUsage->numSubSounds * sizeof( int ) );
        memcpy( newVols, inUsage->volumes, 
                inUsage->numSubSounds * sizeof( double ) );
        }
    
    newIDs[ inUsage->numSubSounds ] = inID;
    newVols[ inUsage->numSubSounds ] = inVolume;
    

    clearSoundUsage( inUsage );
    
    inUsage->numSubSounds = newNum;
    inUsage->ids = newIDs;
    inUsage->volumes = newVols;
    }



void removeSound( SoundUsage *inUsage, int inIndex ) {
    if( inUsage->numSubSounds > inIndex ) {
        
        int newNum = inUsage->numSubSounds - 1;
        int *newIDs = new int[ newNum ];
        double *newVols = new double[ newNum ];
    
        // before removed
        memcpy( newIDs, inUsage->ids, inIndex * sizeof( int ) );
        memcpy( newVols, inUsage->volumes, inIndex * sizeof( double ) );
        
        if( inIndex < inUsage->numSubSounds - 1 ) {
            // after removed
            memcpy( &( newIDs[inIndex] ), 
                    &( inUsage->ids[ inIndex + 1 ] ), 
                    ( inUsage->numSubSounds - inIndex - 1 ) 
                    * sizeof( int ) );
            
            memcpy( &( newVols[inIndex] ), 
                    &( inUsage->volumes[ inIndex + 1 ] ), 
                    ( inUsage->numSubSounds - inIndex - 1 ) 
                    * sizeof( double ) );
            }
        clearSoundUsage( inUsage );
        
        inUsage->numSubSounds = newNum;
        inUsage->ids = newIDs;
        inUsage->volumes = newVols;
        }
    }




char equal( SoundUsage inA, SoundUsage inB ) {
    if( inA.numSubSounds == inB.numSubSounds ) {
        
        for( int i=0; i< inA.numSubSounds; i++ ) {
            if( inA.ids[i] != inB.ids[i]
                ||
                inA.volumes[i] != inB.volumes[i] ) {
                
                return false;
                }
            }
        
        return true;
        }
    return false;
    }




    

