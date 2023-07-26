#include "photoCache.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/io/file/File.h"
#include "minorGems/game/game.h"
#include "minorGems/util/SettingsManager.h"


#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"



static char *currentPhotoID = NULL;
static char currentPhotoNegative = false;

static int currentWebRequest = -1;
static char currentFailed = false;


static char hardFailure = false;



void initPhotoCache() {
    File photoCacheDir( NULL, "photoCache" );
            
    if( ! photoCacheDir.exists() ) {
        photoCacheDir.makeDirectory();
        }
    }



void freePhotoCache() {
    if( currentPhotoID != NULL ) {
        delete [] currentPhotoID;
        }
    currentPhotoID = NULL;
    
    if( currentWebRequest != -1 ) {

        clearWebRequest( currentWebRequest );
        }
    currentWebRequest = -1;
    }



SpriteHandle getCachedPhoto( char *inPhotoID, char inNegative ) {
    if( hardFailure ) {
        return NULL;
        }

    // are we in the middle of fetching it?
    
    if( currentWebRequest != -1 &&
        currentPhotoID != NULL &&
        strcmp( currentPhotoID, inPhotoID ) == 0 &&
        inNegative == currentPhotoNegative ) {
        
        if( currentFailed ) {
            // don't bother re-stepping a failed request,
            // or re-print erro messages
            // we don't want to re-parse bad JPG data, etc.
            return NULL;
            }
        

        int result = stepWebRequest( currentWebRequest );

        if( result == 0 ) {
            // still fetching
            return NULL;
            }
        else if( result == -1 ) {
            // error
            currentFailed = true;

            printf( "\nWeb request for photo %s (negative=%d) failed\n",
                    inPhotoID, inNegative );
            
            // don't clear our request
            // otherwise, client will keep retrying, since we can't return
            // an error code
            // It should be fine to let the client keep checking forever
            // until they pick up another photo
            return NULL;
            }
        else if( result == 1 ) {
            // done!
            
            int resultSize;
            
            unsigned char *resultData = getWebResult( currentWebRequest, 
                                                      &resultSize );
            
            // first, parse data to make sure it's a good JPG

            int w, h, numChan;
            
            // request 4 channels, we expect photos to be grayscale jpgs
            // which have 3 channels (1-channel jpgs aren't common)
            // but we want to convert to RGBA for making a sprite
            unsigned char *imageData = 
                stbi_load_from_memory( 
                    resultData, resultSize, &w, &h, &numChan, 4 );

            if( imageData == NULL ) {
                currentFailed = true;
                printf( "Parsing JPG data for photo %s (negative=%d) failed\n",
                        inPhotoID, inNegative );
            
                delete [] resultData;
                
                return NULL;
                }
            
            if( w != 400 || h != 400 || numChan != 3 ) {
                // numChan is how many channels the image data had
                // we ask for it to be converted to 4
                // but the original image should have had 3

                currentFailed = true;
                printf( "JPG for photo %s (negative=%d) has unexpected "
                        "dimensions: %dx%d with %d channels\n",
                        inPhotoID, inNegative,
                        w, h, numChan );
                
                free( imageData );
                
                delete [] resultData;
                
                return NULL;
                }
            
            // JPG is good

            // save it to cache
            File photoCacheDir( NULL, "photoCache" );

            if( ! photoCacheDir.exists() || ! photoCacheDir.isDirectory() ) {
                hardFailure = true;
                
                printf( "photoCache directory not found\n" );
                
                free( imageData );
                
                delete [] resultData;
                
                return NULL;
                }


            const char *negativeTag = "";
            
            if( inNegative ) {
                negativeTag = "_negative";
                }
                
            char *fileName = autoSprintf( "%s%s.jpg",
                                          inPhotoID, negativeTag );
            
            File *file = photoCacheDir.getChildFile( fileName );
            delete [] fileName;
            
            file->writeToFile( resultData, resultSize );
            
            delete file;
            
            delete [] resultData;

            
            // now make sprite for the parsed image data

            SpriteHandle newSprite = fillSprite( imageData, w, h );
            
            free( imageData );
            
            clearWebRequest( currentWebRequest );
            currentWebRequest = -1;
            
            delete [] currentPhotoID;
            currentPhotoID = NULL;

            printf( "PHOTO CACHE:  Got sprite!\n" );            

            return newSprite;
            }
        else {
            // unexpected return value from stepWebRequest
            return NULL;
            }
        
        }
    else {
        // not in the middle of fetching it

        // see if it is cached
        File photoCacheDir( NULL, "photoCache" );
        
        if( ! photoCacheDir.exists() || ! photoCacheDir.isDirectory() ) {
            hardFailure = true;
            
            printf( "photoCache directory not found\n" );
            return NULL;
            }


        const char *negativeTag = "";
        
        if( inNegative ) {
            negativeTag = "_negative";
            }
        
        char *fileName = autoSprintf( "%s%s.jpg",
                                      inPhotoID, negativeTag );
        
        File *file = photoCacheDir.getChildFile( fileName );
        
        if( file->exists() ) {
            int dataLength;
            
            unsigned char *fileData = file->readFileContents( &dataLength );
            
            delete file;

            if( fileData == NULL ) {
                printf( "Failed to read from cached photo file %s\n",
                        fileName );
                
                delete [] fileName;
                
                return NULL;
                }

            int w, h, numChan;
            
            // request 4 channels, we expect photos to be grayscale jpgs
            // which actually have 3 channels (there aren't really 1-channel
            // jpgs, at least not commonly)
            // but we want to convert to RGBA for making a sprite
            unsigned char *imageData = 
                stbi_load_from_memory( 
                    fileData, dataLength, &w, &h, &numChan, 4 );
            
            delete [] fileData;

            // assume that the cached file is good, don't check dimensions, etc

            if( imageData == NULL ) {
                printf( "Failed to read from cached photo file %s\n",
                        fileName );
                delete [] fileName;
                
                return NULL;
                }

            delete [] fileName;

            SpriteHandle newSprite = fillSprite( imageData, w, h );
            
            free( imageData );
            
            printf( "PHOTO CACHE:  Got sprite!\n" );
            return newSprite;
            }
        else {
            delete file;
            
            // file not cached
            // start a new web request for it
            char *url = 
                SettingsManager::getStringSetting( "photoServerURL", "" );
            
            char *serverPos = strstr( url, "server.php" );
            
            if( serverPos == NULL ) {
                hardFailure = true;
                
                printf( "Failed to find server.php in photoServerURL %s\n",
                        url );
                
                delete [] url;
                delete [] fileName;
                
                return NULL;
                }
            
            // strip off server.php
            serverPos[0] = '\0';
            
            char *fullURL = autoSprintf( "%sphotos/%s", url, fileName );
            
            delete [] url;
            delete [] fileName;
            
            // replace current request with this one
            if( currentWebRequest != -1 ) {
                clearWebRequest( currentWebRequest );
                currentWebRequest = -1;
                }
            if( currentPhotoID != NULL ) {
                delete [] currentPhotoID;
                currentPhotoID = NULL;
                }
            currentFailed = false;

            currentPhotoID = stringDuplicate( inPhotoID );
            currentPhotoNegative = inNegative;
            
            currentWebRequest = startWebRequest( "GET", fullURL, NULL );
            
            // wait for URL to load
            return NULL;
            }
        }
    }
