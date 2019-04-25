#include "groundSprites.h"
#include "accountHmac.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/formats/encodingUtils.h"
#include "minorGems/game/game.h"

#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"


#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"


extern char *userEmail;
extern char *serverIP;
extern int ourID;



void initPhotos() {
    }

void freePhotos() {
    }



void stepPhotos() {
    }



static SimpleVector<unsigned char> jpegBytes;

void jpegWriteFunc( void *context, void *data, int size ) {
    jpegBytes.appendArray( (unsigned char *)data, size );
    }



// for testing, force web request now, blocking until done
static char *forceWebRequest( const char *inMethod, const char *inURL,
                              const char *inBody ) {
    
    int handle = startWebRequest( inMethod, inURL, inBody );

    int result = 0;
    
    while( result == 0 ) {
        result = stepWebRequest( handle );
        }
    
    if( result != 1 ) {
        clearWebRequest( handle );
        return NULL;
        }
    

    char *resultString = getWebResult( handle );
    clearWebRequest( handle );
    
    return resultString;
    }



void takePhoto( doublePair inCamerLocation, int inCameraFacing ) {
    int screenWidth, screenHeight;
    getScreenDimensions( &screenWidth, &screenHeight );
        
    int rectStartX = lrint( inCamerLocation.x );
    if( inCameraFacing == -1 ) {
        rectStartX -= 400 + CELL_D;
        }
    else {
        rectStartX += CELL_D;
        }
    if( rectStartX < 0 ) {
        rectStartX = 0;
        }
    if( rectStartX >= screenWidth - 400 ) {
        rectStartX = screenWidth - 401;
        }
        
    int rectStartY = lrint( inCamerLocation.y ) - 200;
    
    if( rectStartY < 0 ) {
        rectStartY = 0;
        }
    if( rectStartY >= screenHeight - 400 ) {
        rectStartY = screenHeight - 401;
        }

    Image *im = getScreenRegionRaw( rectStartX, rectStartY, 400, 400 );
    
    double *r = im->getChannel( 0 );
    double *g = im->getChannel( 1 );
    double *b = im->getChannel( 2 );
    
    int w = im->getWidth();
    int h = im->getHeight();
    int numPix = w * h;
    
    unsigned char *data = new unsigned char[ numPix * 3 ];
    
    int i = 0;
    
    for( int p=0; p<numPix; p++ ) {
        data[i++] = lrint( r[p] * 255 );
        data[i++] = lrint( g[p] * 255 );
        data[i++] = lrint( b[p] * 255 );
        }
    


    int result = stbi_write_jpg_to_func( 
        jpegWriteFunc, NULL, 
        w, h, 
        3, data, 90 );


    if( result == 0 ) {
        // error
        }
    else {
        // success
        // test
        FILE *testFile = fopen( "test.jpg", "w" );
        for( int i=0; i<jpegBytes.size(); i++ ) {
            fwrite( jpegBytes.getElement(i), 1, 1, testFile );
            }
        fclose( testFile );

        char *url = SettingsManager::getStringSetting( "photoServerURL", "" );
        

        char *encodedEmail = URLUtils::urlEncode( userEmail );

        char *getURL = autoSprintf( "%s?action=get_sequence_number&email=%s", 
                                   url, encodedEmail );

        delete [] encodedEmail;
        
        char *result = forceWebRequest( "GET", getURL, NULL );
        
        delete [] getURL;
        
        if( result != NULL ) {
            int seqNumber = 0;
        
            sscanf( result, "%d", &seqNumber );
            delete [] result;
        
            char *seqNumberString = autoSprintf( "%d", seqNumber );

            char *pureKey = getPureAccountKey();

            char *keyHash = hmac_sha1( pureKey, seqNumberString );

            delete [] seqNumberString;
            delete [] pureKey;
            
            
            unsigned char *jpegData = jpegBytes.getElementArray();
            
            // FIXME:  add hash as jpg comment

            char *jpegBase64 = 
                base64Encode( jpegData, jpegBytes.size(), false );

            delete [] jpegData;
            

            char *jpegURL = URLUtils::urlEncode( jpegBase64 );
            
            encodedEmail = URLUtils::urlEncode( userEmail );

            char *postBody = 
                autoSprintf( "action=submit_photo"
                             "&email=%s"
                             "&sequence_number=%d"
                             "&hash_value=%s"
                             "&server_name=%s"
                             "&photo_author_id=%d"
                             "&photo_subject_ids="
                             "&photo_author_name=JASON+ROHRER"
                             "&photo_subjects_name="
                             "&jpg_base64=%s"
                             , 
                             encodedEmail,
                             seqNumber,
                             keyHash,
                             serverIP,
                             ourID,
                             jpegURL
                             );
            delete [] encodedEmail;
            delete [] jpegURL;
            

            FILE *bodyFile = fopen( "body.txt", "w" );
            fprintf( bodyFile, "%s", postBody );
            fclose( bodyFile );
            
            delete [] jpegBase64;

            result = forceWebRequest( "POST", url, postBody );
            
            delete [] postBody;

            printf( "Result of jpg posting = %s\n", result );
            

            delete [] result;
            
            // FIXME:  finish this code to test

            /*
            server.php

action=submit_photo
&email=[email address]
&sequence_number=[int]
&hash_value=[hash value]
&server_name=[string]
&photo_author_id=[int]
&photo_subjects_ids=[string]
&photo_author_name=[string]
&photo_subjects_names=[string]
&jpg_base64=[jpg file as base 64]
            */
            }
        }

    jpegBytes.deleteAll();
    }

