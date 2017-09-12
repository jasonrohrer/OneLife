#include <stdlib.h>
#include <math.h>


#include "minorGems/io/file/File.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"

#include "minorGems/sound/formats/aiff.h"

#include "minorGems/util/random/JenkinsRandomSource.h"

JenkinsRandomSource randSource( 150347 );


int numSamples = 0;
int sampleRate = 0;
int16_t *samples = NULL;

int numShapes = 0;
File **shapeFiles = NULL;

int fps = 0;



void usage() {

    if( samples != NULL ) {
        delete [] samples;
        samples = NULL;
        }

    if( shapeFiles != NULL ) {
        for( int i=0; i<numShapes; i++ ) {
            delete [] shapeFiles[i];
            }
        delete [] shapeFiles;
        shapeFiles = NULL;
        }
    

    printf( "\nUsage:\n" );
    printf( "generateMouthShapes "
            "voiceAIFF fps moves_per_second thresh shapesDir "
            "outDir outList\n\n" );
    
    printf( "Example:\n" );
    printf( "generateMouthShapes "
            "test.aiff 30 3 0.25 test/mouthShapes "
            "test/output test/list.txt\n\n" );
    
    printf( "Voice file must be mono 16-bit.\n\n" );

    printf( "Shape files must be in alphabetical order with closed-mouth "
            "shape first and at least 2 files.\n\n" );
    
    exit( 1 );
    }




char getTalking( int inFrameNumber, double inThresh ) {
    double sec = inFrameNumber / (double)fps;
    
    int sampleStart = lrint( sec * sampleRate );
    
    int samplesInFrame = lrint( ( 1.0 / fps ) * sampleRate );

    int sampleEnd = sampleStart + samplesInFrame;

    
    if( sampleEnd >= numSamples ) {
        sampleEnd = numSamples;
        }
    
    samplesInFrame = sampleEnd - sampleStart;
    

    int sum = 0;

    for( int i=sampleStart; i<sampleEnd; i++ ) {
        sum += abs( samples[i] );
        }

    // pure square wave of full amplitude
    int maxSum = pow( 2, 15 ) * samplesInFrame;
    

    double sumFraction = (double)sum  / (double)maxSum;
    
    
    if( sumFraction > inThresh ) {
        return true;
        }
    return false;
    }



int main( int inNumArgs, char **inArgs ) {

    if( inNumArgs != 8 ) {
        usage();
        }
    
    File voiceFile( NULL, inArgs[1] );

    if( ! voiceFile.exists() ||
        voiceFile.isDirectory() ) {
        usage();
        }

    int dataLength;
    unsigned char *aiffData = voiceFile.readFileContents( &dataLength );
    
    if( aiffData == NULL ) {
        usage();
        }
    
    
    samples = readMono16AIFFData( aiffData, dataLength, &numSamples,
                                  &sampleRate );

    delete [] aiffData;

    if( samples == NULL ) {
        usage();
        }
    
    

    
    int numRead = sscanf( inArgs[2], "%d", &fps );
    
    if( numRead != 1 ) {
        usage();
        }

    int mps = 0;
    
    numRead = sscanf( inArgs[3], "%d", &mps );
    
    if( numRead != 1 ) {
        usage();
        }
    
    int framesPerMove = ceil( (double)fps / (double)mps );
    
    printf( "%d frames per move\n", framesPerMove );

    double thresh = 0;
    
    numRead = sscanf( inArgs[4], "%lf", &thresh );
    
    if( numRead != 1 ) {
        usage();
        }



    File shapesDir( NULL, inArgs[5] );
    
    if( ! shapesDir.exists() ||
        ! shapesDir.isDirectory() ) {        
        usage();
        }
    
    File destDir( NULL, inArgs[6] );

    if( ! destDir.exists() ||
        ! destDir.isDirectory() ) {
        usage();
        }


    File listFile( NULL, inArgs[7] );

    if( listFile.isDirectory() ) {
        usage();
        }
    
    SimpleVector<char> list;
    

    shapeFiles = shapesDir.getChildFilesSorted( &numShapes );
    
    if( numShapes < 2 ) {
        usage();
        }
    char **shapeStrings = new char*[numShapes];
    

    for( int i=0; i<numShapes; i++ ) {
        shapeStrings[i] = autoSprintf( "%d\n", i );
        }
    
    
    
    double numSec = numSamples / (double)sampleRate;
    
    int numFrames = ceil( numSec * fps );
    
    File **frameShapes = new File*[numFrames];
    
    // default to closed mouth
    for( int i=0; i<numFrames; i++ ) {
        frameShapes[i] = shapeFiles[0];
        }

    int i = 0;
    
    while( i < numFrames ) {

        // avoid flicker by holding quiet frame longer
        int framesQuiet = 0;
        while( i < numFrames && 
               ( ! getTalking( i, thresh ) ||
                 framesQuiet < framesPerMove ) ) {
            frameShapes[i] = shapeFiles[0];
            
            list.appendElementString( shapeStrings[0] );
                                      
            i++;
            framesQuiet++;
            }
        // talking now
        
        int framesTalking = 0;
        int pick = randSource.getRandomBoundedInt( 1, numShapes - 1 );
        File *curShape = shapeFiles[ pick ];
        
        while( i < numFrames && 
               ( getTalking( i, thresh ) ||
                 framesTalking < framesPerMove ) ) {
            frameShapes[i] = curShape;

            list.appendElementString( shapeStrings[pick] );

            framesTalking ++;
            
            if( framesTalking % framesPerMove == 0 ) {
                File *newCurShape = curShape;

                // pick next randomly, but force change
                while( newCurShape == curShape ) {
                    pick = randSource.getRandomBoundedInt( 1, numShapes - 1 );
                    
                    
                    newCurShape = shapeFiles[ pick ];
                    }
                curShape = newCurShape;
                }
            i++;
            }
        }
    
    for( int i=0; i<numFrames; i++ ) {
        char *name = autoSprintf( "frame%05d.tga", i+1 );
        File *dest = destDir.getChildFile( name );
    
        frameShapes[i]->copy( dest );

        delete [] name;
        delete dest;
        }
    
    char *listString = list.getElementString();
    
    listFile.writeToFile( listString );
    

    for( int i=0; i<numShapes; i++ ) {
        delete shapeFiles[i];
        delete [] shapeStrings[i];
        }
    delete [] shapeFiles;
    
    
    delete [] samples;
    
    return 0;
    }

