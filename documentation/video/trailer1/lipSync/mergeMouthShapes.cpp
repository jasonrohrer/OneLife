#include <stdlib.h>
#include <math.h>


#include "minorGems/io/file/File.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"

#include "minorGems/sound/formats/aiff.h"

#include "minorGems/util/random/JenkinsRandomSource.h"

JenkinsRandomSource randSource( 150347 );




int numMouths;
File **mouthFiles = NULL;
int numFrames;
File **frameFiles = NULL;


void freeFiles() {
    if( mouthFiles != NULL ) {
        for( int i=0; i<numMouths; i++ ) {
            delete mouthFiles[i];
            }
        delete [] mouthFiles;
        mouthFiles = NULL;
        }
    if( frameFiles != NULL ) {
        for( int i=0; i<numFrames; i++ ) {
            delete frameFiles[i];
            }
        delete [] frameFiles;
        frameFiles = NULL;
        }
    }



void usage() {

    freeFiles();


    printf( "\nUsage:\n" );
    printf( "mergeMouthShapes "
            "mouthsDir framesDir outDir\n\n" );
    
    printf( "Example:\n" );
    printf( "mergeMouthShapes "
            "test/mouths test/frames test/out\n\n" );
    
    printf( "mouths and frames must be TGA files.\n\n" );
    printf( "mouthsDir must have one mouth per frame.\n\n" );
    printf( "Mouth and frame files must be in alphabetical order.\n\n" );

    printf( "Frames must have magenta target pixel with "
            "magenta-ish neighbor pixels.\n\n" );

    exit( 1 );
    }



void mergeMouth( Image *inFrame, Image *inMouth, 
                 int *inOutLastPosX, int *inOutLastPosY ) {

    int frameW = inFrame->getWidth();
    int frameH = inFrame->getHeight();

    int mouthW = inMouth->getWidth();
    int mouthH = inMouth->getHeight();
        

    // avoid very edge so we can safely check neighbor pixels in loop
    // and so we can blindly place mouth on frame without worrying
    // about it going off the edge

    int searchStartX = mouthW;
    int searchStartY = mouthH;

    
    int searchEndX = frameW - mouthW;
    int searchEndY = frameH - mouthH;

    int rad = 10;
    
    int posX = *inOutLastPosX;
    int posY = *inOutLastPosY;
    
    
    

    if( posX != -1 &&
        posY != -1 ) {
        
        searchStartX = posX - rad;
        searchEndX = posX + rad;

        searchStartY = posY - rad;
        searchEndY = posY + rad;
        
        if( searchStartX < mouthW ) {
            searchStartX = mouthW;
            }
        if( searchStartY < mouthH ) {
            searchStartY = mouthH;
            }

        if( searchEndX > frameW - mouthW ) {
            searchEndX = frameH - mouthW;
            }
        if( searchEndY > frameW - mouthH ) {
            searchEndY = frameH - mouthH;
            }
        }

    posX = -1;
    posY = -1;
    
    double *r = inFrame->getChannel( 0 );
    double *g = inFrame->getChannel( 1 );
    double *b = inFrame->getChannel( 2 );
    
    for( int y=searchStartY; y<searchEndY; y++ ) {
        for( int x=searchStartX; x<searchEndX; x++ ) {
            int i = y * frameW + x;

            if( r[i] == 1.0 && g[i] == 0 && b[i] == 1.0 ) {
                // candidate
                
                // check that neighbors are magenta-ish
                int n[4] = { i - 1, 
                             i + 1, 
                             i - frameW, 
                             i + frameW };

                char allNeighborsMagenta = true;
                for( int j=0; j<4; j++ ) {
                    if( g[ n[j] ] == 0 && 
                        r[ n[j] ] == b[ n[j] ] &&
                        r[ n[j] ] > 0.5 &&
                        b[ n[j] ] > 0.5 ) {
                        // a magenta-ish neighbor
                        }
                    else {
                        // neither
                        allNeighborsMagenta = false;
                        break;
                        }
                    }
                
                if( allNeighborsMagenta ) {
                    posX = x;
                    posY = y;
                    break;
                    }
                }
            }

        if( posX != -1 &&
            posY != -1 ) {
            break;
            }
        }
    

    
    if( posX != -1 &&
        posY != -1 ) {
        // found anchor pos
        

        double *mouthR = inMouth->getChannel(0);
        double *mouthG = inMouth->getChannel(1);
        double *mouthB = inMouth->getChannel(2);
        double *mouthA = inMouth->getChannel(3);

        double *frameR = inFrame->getChannel(0);
        double *frameG = inFrame->getChannel(1);
        double *frameB = inFrame->getChannel(2);

                // find skin color to cover mouth anchor with

        int skinX = posX - 5;
        int skinY = posY + 3;
        int skinI = skinY * frameW + skinX;

        double skinR = frameR[skinI];
        double skinG = frameG[skinI];
        double skinB = frameB[skinI];
        
        int coverStartX = posX - 3;
        int coverStartY = posY - 3;
        int coverEndX = posX + 3;
        int coverEndY = posY + 3;
        
        for( int y=coverStartY; y<coverEndY; y++ ) {
            for( int x=coverStartX; x<coverEndX; x++ ) {
                int coverI = y * frameW + x;
                frameR[ coverI ] = skinR;
                frameG[ coverI ] = skinG;
                frameB[ coverI ] = skinB;
                }
            }




        int startX = posX - mouthW / 2;
        int startY = posY - mouthH / 2;
        
        int endX = startX + mouthW;
        int endY = startY + mouthH;
        

        int mouthY = 0;
        for( int y=startY; y<endY; y++ ) {
            int mouthX = 0;
            for( int x=startX; x<endX; x++ ) {
                
                int frameI = y * frameW + x;
                int mouthI = mouthY * mouthW + mouthX;
                
                double mWeight = mouthA[ mouthI ];
                double fWeight = 1 - mWeight;
                
                frameR[ frameI ] =
                    mWeight * mouthR[ mouthI ] + fWeight * frameR[ frameI ];
                frameG[ frameI ] =
                    mWeight * mouthG[ mouthI ] + fWeight * frameG[ frameI ];
                frameB[ frameI ] =
                    mWeight * mouthB[ mouthI ] + fWeight * frameB[ frameI ];

                mouthX++;
                }
            mouthY++;
            }
        
        }
    


    *inOutLastPosX = posX;
    *inOutLastPosY = posY;
    }



int main( int inNumArgs, char **inArgs ) {

    if( inNumArgs != 4 ) {
        usage();
        }
    

    File mouthsDir( NULL, inArgs[1] );
    
    if( ! mouthsDir.exists() ||
        ! mouthsDir.isDirectory() ) {        
        usage();
        }
    
    File framesDir( NULL, inArgs[2] );

    if( ! framesDir.exists() ||
        ! framesDir.isDirectory() ) {
        usage();
        }

    File destDir( NULL, inArgs[3] );

    if( ! destDir.exists() ||
        ! destDir.isDirectory() ) {
        usage();
        }

    int numMouths;
    File **mouthFiles = mouthsDir.getChildFilesSorted( &numMouths );

    int numFrames;
    File **frameFiles = framesDir.getChildFilesSorted( &numFrames );
    

    if( numFrames == 0 ) {
        
        usage();
        }
    
    int numToProcess = numFrames;
    
    if( numMouths < numToProcess ) {
        numToProcess = numMouths;
        }

    
    int lastPosX = -1;
    int lastPosY = -1;
    
    TGAImageConverter converter;
    
    for( int i=0; i<numToProcess; i++ ) {
        FileInputStream frameInput( frameFiles[i] );
        Image *frame = converter.deformatImage( &frameInput );

        if( frame == NULL ) {
            usage();
            }

        FileInputStream mouthInput( mouthFiles[i] );
        Image *mouth = converter.deformatImage( &mouthInput );
        
        if( mouth == NULL ) {
            delete frame;
            usage();
            }
        
        mergeMouth( frame, mouth, &lastPosX, &lastPosY );

        char *outFileName = autoSprintf( "out%05d.tga", i + 1 );
        
        File *outFile = destDir.getChildFile( outFileName );
        delete [] outFileName;
        
        FileOutputStream outStream( outFile );
        
        converter.formatImage( frame, &outStream );
        delete outFile;

        delete frame;
        delete mouth;

        if( lastPosX == -1 ||
            lastPosY == -1 ) {
            char *fileName = frameFiles[i]->getFileName();
            
            printf( "Failed to find mouth anchor in frame %d (%s)\n",
                    i, fileName );
            delete [] fileName;
            }
        }
    
    freeFiles();
    
    return 0;
    }

