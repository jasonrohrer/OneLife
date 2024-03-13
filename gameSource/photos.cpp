#include "groundSprites.h"
#include "accountHmac.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/formats/encodingUtils.h"
#include "minorGems/game/game.h"

#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"

#include "minorGems/graphics/filters/FastBlurFilter.h"
#include "minorGems/graphics/filters/BoxBlurFilter.h"

#include "minorGems/graphics/RGBAImage.h"


#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"


#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb_image_resize.h"


extern char *userEmail;
extern char *serverIP;
extern int ourID;

extern char isAHAP;


extern double viewWidth;
extern double viewHeight;
extern double visibleViewWidth;


Image *photoBorder;


void initPhotos() {
    photoBorder = readTGAFile( "photoBorder.tga" );
    }


static int sequenceNumberWebRequest = -1;
static int submitPhotoWebRequest = -1;

static char *postedPhotoID = NULL;


char *getPostedPhotoID() {
    char *returnVal = postedPhotoID;
    
    postedPhotoID = NULL;

    return returnVal;
    }



void freePhotos() {
    delete photoBorder;

    if( sequenceNumberWebRequest != -1 ) {
        clearWebRequest( sequenceNumberWebRequest );
        sequenceNumberWebRequest = -1;
        }
    if( submitPhotoWebRequest != -1 ) {
        clearWebRequest( submitPhotoWebRequest );
        submitPhotoWebRequest = -1;
        }
    }


static int nextSequenceNumber = -1;


void stepPhotos() {

    if( sequenceNumberWebRequest != -1 ) {
        int result = stepWebRequest( sequenceNumberWebRequest );
        
        if( result == -1 ) {
            clearWebRequest( sequenceNumberWebRequest );
            sequenceNumberWebRequest = -1;
            }
        else if( result == 1 ) {

            char *resultString = getWebResult( sequenceNumberWebRequest );
            clearWebRequest( sequenceNumberWebRequest );
            
            if( resultString != NULL ) {
                sscanf( resultString, "%d", &nextSequenceNumber );
                delete [] resultString;
                }
            }
        // else result == 0, still waiting
        }
    else if( submitPhotoWebRequest != -1 ) {
        int result = stepWebRequest( submitPhotoWebRequest );
        
        if( result == -1 ) {
            printf( "submitPhoto web request failed\n" );
            clearWebRequest( submitPhotoWebRequest );
            submitPhotoWebRequest = -1;
            
            if( postedPhotoID != NULL ) {
                delete [] postedPhotoID;
                }
            postedPhotoID = stringDuplicate( "" );
            }
        else if( result == 1 ) {

            char *resultString = getWebResult( submitPhotoWebRequest );
            clearWebRequest( submitPhotoWebRequest );
            submitPhotoWebRequest = -1;

            if( resultString != NULL ) {
                printf( "submitPhoto web request result:  %s\n", resultString );
                
                char *okayPos = strstr( resultString, "OK" );
                if( okayPos != NULL && 
                    strlen( okayPos ) > 3 ) {
                    
                    // there's stuff after OK
                    // it is the photo ID

                    // skip OK and the \n character
                    char *photoID = &( okayPos[3] );

                    printf( "submitPhoto got photo ID:  %s\n", photoID );
                    
                    if( postedPhotoID != NULL ) {
                        delete [] postedPhotoID;
                        }
                    postedPhotoID = stringDuplicate( photoID );
                    }
                else {
                    if( postedPhotoID != NULL ) {
                        delete [] postedPhotoID;
                        }
                    postedPhotoID = stringDuplicate( "" );
                    }
                
                delete [] resultString;
                }
            else {
                if( postedPhotoID != NULL ) {
                    delete [] postedPhotoID;
                    }
                postedPhotoID = stringDuplicate( "" );
                }
            }
        // else result == 0, still waiting
        }
    }



// result destroyed by caller
static char *getPhotoServerURL() {
    char *url;
    if( isAHAP ) {
        url = SettingsManager::getStringSetting( "ahapPhotoServerURL", "" );
        }
    else {
        url = SettingsManager::getStringSetting( "photoServerURL", "" );
        }
    return url;
    }



int getNextPhotoSequenceNumber() {
    
    if( nextSequenceNumber != -1 ) {
        // result from last request ready
        
        int returnVal = nextSequenceNumber;
        nextSequenceNumber = -1;
        return returnVal;
        }
    else if( sequenceNumberWebRequest == -1 ) {
        // start a new request
        
        char *url = getPhotoServerURL();
        
        
        char *encodedEmail = URLUtils::urlEncode( userEmail );
        
        char *getURL = autoSprintf( "%s?action=get_sequence_number&email=%s", 
                                    url, encodedEmail );
        
        delete [] encodedEmail;
        delete [] url;
        
        
        sequenceNumberWebRequest = startWebRequest( "GET", getURL, NULL );
        
        delete [] getURL;
        }
    
    // result not ready yet
    return -1;
    }



static SimpleVector<unsigned char> jpegBytes;

void jpegWriteFunc( void *context, void *data, int size ) {
    jpegBytes.appendArray( (unsigned char *)data, size );
    }



static inline double intDist( int inXA, int inYA, int inXB, int inYB ) {
    double dx = (double)inXA - (double)inXB;
    double dy = (double)inYA - (double)inYB;

    return sqrt(  dx * dx + dy * dy );
    }




static char *getJpeg64Bytes( unsigned char *inData, int inW, int inH,
                             char *inKeyHash ) {
    int result = stbi_write_jpg_to_func( 
        jpegWriteFunc, NULL, 
        inW, inH, 
        1, inData, 90 );

    if( result == 0 ) {
        // failed to write jpg
        return NULL;
        }
    
    // success
    // test
    /*
      FILE *testFile = fopen( "test.jpg", "w" );
      for( int i=0; i<jpegBytes.size(); i++ ) {
      fwrite( jpegBytes.getElement(i), 1, 1, testFile );
      }
      fclose( testFile );
    */


    // insert comment at end with hash value
    // header describes length of comment (002A is 42 bytes)
    unsigned char *commentHeader = hexDecode( (char*)"FFEE002A" );
    
    // the last two bytes of a JPG should be FFD9
    // delete these for now, and add them back later
    jpegBytes.deleteElement( jpegBytes.size() - 1 );
    jpegBytes.deleteElement( jpegBytes.size() - 1 );
    
    jpegBytes.appendArray( commentHeader, 4 );
    delete [] commentHeader;
        
    jpegBytes.appendArray( (unsigned char *)inKeyHash, 
                           strlen( inKeyHash ) );
        
    // add jpg footer again
    jpegBytes.push_back( 0xFF );
    jpegBytes.push_back( 0xD9 );
    
    unsigned char *jpegData = jpegBytes.getElementArray();
    
    
    char *jpegBase64 = 
        base64Encode( jpegData, jpegBytes.size(), false );
    
    delete [] jpegData;
    

    jpegBytes.deleteAll();
    
    return jpegBase64;
    }

                             



// inCameraLocation is in world pixels relative to screen center
// (World pixel units assume CELL_D pixels per tile).
// Note that world pixels may differ from screen pixels if screen
// size does not match our native game resolution

// Don't muck with this code in a way that tricks the photo server.
// read OneLife/photoServer/protocol.txt for your very serious warning
// about this.
void takePhoto( doublePair inCameraLocation, int inCameraFacing,
                int inSequenceNumber,
                char *inServerSig,
                int inAuthorID,
                char *inAuthorName,
                SimpleVector<int> *inSubjectIDs,
                SimpleVector<char*> *inSubjectNames ) {

    if( submitPhotoWebRequest != -1 ) {
        // one at a time
        return;
        }

    char *serverSig = stringDuplicate( inServerSig );
    
    int screenWidth, screenHeight;
    getScreenDimensions( &screenWidth, &screenHeight );
        
    double targetAspectRatio = visibleViewWidth / (double)viewHeight;
    
    double screenAspectRatio = screenWidth / (double)screenHeight;
    
    double screenScale;
    
    if( screenAspectRatio > targetAspectRatio ) {
        screenScale = screenHeight / (double) viewHeight;
        }
    else {
        screenScale = screenWidth / visibleViewWidth;
        }

    // now we can convert cam position into screen pixels, relative to center of
    // screen
    inCameraLocation = mult( inCameraLocation, screenScale );
    
    // now that it's in screen pixels, we can compute it relative to
    // corner of screen
    inCameraLocation.x += screenWidth / 2;
    inCameraLocation.y += screenHeight / 2;


    int rectStartX = lrint( inCameraLocation.x );
    if( inCameraFacing == -1 ) {
        rectStartX -= screenScale * ( 400 + CELL_D / 2 );
        }
    else {
        rectStartX += screenScale * ( CELL_D / 2 );
        }
    if( rectStartX < 0 ) {
        rectStartX = 0;
        }
    if( rectStartX >= screenWidth - screenScale * 400 ) {
        rectStartX = screenWidth - screenScale * 400 - 1;
        }
        
    int rectStartY = lrint( inCameraLocation.y ) - screenScale * CELL_D;
    
    if( rectStartY < 0 ) {
        rectStartY = 0;
        }
    if( rectStartY >= screenHeight - screenScale * 400 ) {
        rectStartY = screenHeight - screenScale * 400 - 1;
        }
    
    Image *im = getScreenRegionRaw( rectStartX, rectStartY, 
                                    screenScale * 400, screenScale * 400 );

    if( screenScale != 1.0 ) {
        // resize the image to 400x400

        unsigned char *bytes = RGBAImage::getRGBABytes( im );
        
        unsigned char *outBytes = new unsigned char[ 400 * 400 * 4 ];

        int result = stbir_resize_uint8( bytes, 
                                         im->getWidth(), im->getHeight(),
                                         0, outBytes, 400, 400, 0, 4 );
        if( result == 1 ) {
            delete im;
            
            im = RGBAImage::getImageFromBytes( outBytes, 400, 400, 4 );
            }

        delete [] bytes;
        delete [] outBytes;
        }
    
    
    double *r = im->getChannel( 0 );
    double *g = im->getChannel( 1 );
    double *b = im->getChannel( 2 );
    
    int w = im->getWidth();
    int h = im->getHeight();
    int numPix = w * h;

    Image grayIm( w, h, 1, false );
    

    double *gray = grayIm.getChannel( 0 );
    
    double rWeight = 0.2989;
    double gWeight = 0.5870;
    double bWeight = 0.114;
    
    for( int p=0; p<numPix; p++ ) {
        gray[p] = 
            r[p] * rWeight + 
            g[p] * gWeight + 
            b[p] * bWeight;
        }

    
    FastBlurFilter blur;
    
    double startTime = game_getCurrentTime();
    /*for( int i=0; i<10; i++ ) {
        grayIm.filter( &blur );
        }
    printf( "10x fast blur = %f sec\n", game_getCurrentTime() - startTime );
    */

    Image *blurGray = grayIm.copy();
    Image *blurGrayLess = grayIm.copy();
    
    BoxBlurFilter blur2( 10 );
    startTime = game_getCurrentTime();
    for( int i=0; i<3; i++ ) {
        blurGray->filter( &blur2 );
        }
    printf( "3x box blur = %f sec\n", game_getCurrentTime() - startTime );


    

    BoxBlurFilter blur3( 1 );
    startTime = game_getCurrentTime();
    for( int i=0; i<2; i++ ) {
        blurGrayLess->filter( &blur3 );
        }
    printf( "3x box blur = %f sec\n", game_getCurrentTime() - startTime );
    

    // wash out
    double *blurChan = blurGray->getChannel( 0 );
    
    for( int p=0; p<numPix; p++ ) {
        gray[p] += 0.3 * blurChan[p];
        if( gray[p] > 1 ) {
            gray[p] = 1;
            }
        }

    double *blurLessChan = blurGrayLess->getChannel( 0 );

    // now cross to blur less chan as we get farther from center

    int centerX = w/2;
    int centerY = h/2;
    
    for( int p=0; p<numPix; p++ ) {
        int y = p / w;
        int x = p - y * w;
        
        double d = intDist( x, y, centerX, centerY );
        
        double edgeMix = 1;
        double edgeMix2 = 1;
        
        if( d > 50 ) {
            d -= 50;
            
            if( d >= 300 ) {
                edgeMix = 0;
                }
            else {
                edgeMix = ( cos( M_PI * d / 300 ) * 0.5 + 0.5 );
                }
            
            if( d >= 200 ) {
                edgeMix2 = 0;
                }
            else {
                edgeMix2 = ( cos( M_PI * d / 200 ) * 0.5 + 0.5 );
                }
            }
        
        gray[p] = edgeMix2 * gray[p] + (1-edgeMix2) * blurLessChan[p];

        // vignette
        gray[p] -= 0.7 * ( 1 - edgeMix );
        if( gray[p] < 0 ) {
            gray[p] = 0;
            }
        }
    
    delete blurGray;
    delete blurGrayLess;


    // make negative too
    Image negativeGrayIm( w, h, 1, false );

    double *negativeGray = negativeGrayIm.getChannel( 0 );
    
    for( int p=0; p<numPix; p++ ) {
        negativeGray[p] = 1.0 - gray[p];
        }


    
    double *borderAlpha = photoBorder->getChannel( 3 );

    for( int p=0; p<numPix; p++ ) {
        // both regular image and negative image have white borders
        gray[p] = borderAlpha[p] * 1 + (1 - borderAlpha[p]) * gray[p];
        negativeGray[p] = borderAlpha[p] * 1 + 
            (1 - borderAlpha[p]) * negativeGray[p];
        }

    


    
    unsigned char *data = new unsigned char[ numPix ];
    unsigned char *negativeData = new unsigned char[ numPix ];
    
    for( int p=0; p<numPix; p++ ) {
        data[p] = lrint( gray[p] * 255 );
        negativeData[p] = lrint( negativeGray[p] * 255 );
        }
    
    

    delete im;


    char *seqNumberString = autoSprintf( "%d", inSequenceNumber );
    
    char *pureKey = getPureAccountKey();
    
    char *keyHash = hmac_sha1( pureKey, seqNumberString );
    
    delete [] seqNumberString;
    delete [] pureKey;

    
    char *jpegBase64 = getJpeg64Bytes( data, w, h, keyHash );
    char *negativeJpegBase64 = getJpeg64Bytes( negativeData, w, h, keyHash );

    delete [] data;
    delete [] negativeData;
    

    if( jpegBase64 == NULL ||
        negativeJpegBase64 == NULL ) {
        // error
        }
    else {
        char *url = getPhotoServerURL();

        char *subjectIDs;
        if( inSubjectIDs->size() == 0 ) {
            subjectIDs = stringDuplicate( "" );
            }
        else {
            int *listInts = inSubjectIDs->getElementArray();
            
            SimpleVector<char*> stringList;
            
            for( int i=0; i< inSubjectIDs->size(); i++ ) {
                stringList.push_back( autoSprintf( "%d", listInts[i] ) );
                }

            char **list = stringList.getElementArray();
            subjectIDs = join( list, stringList.size(), "," );
            delete [] list;
            stringList.deallocateStringElements();
            }
        

        char *subjectNames;
        if( inSubjectNames->size() == 0 ) {
            subjectNames = stringDuplicate( "" );
            }
        else {
            char **list = inSubjectNames->getElementArray();
            subjectNames = join( list, inSubjectNames->size(), "," );
            delete [] list;
            
            char *enc = URLUtils::urlEncode( subjectNames );
            delete [] subjectNames;
            subjectNames = enc;
            }
        
        

        
        char *jpegURL = URLUtils::urlEncode( jpegBase64 );
        char *negativeJpegURL = URLUtils::urlEncode( negativeJpegBase64 );
        
        char *encodedEmail = URLUtils::urlEncode( userEmail );
        
        char *postBody = 
            autoSprintf( "action=submit_photo"
                         "&email=%s"
                         "&sequence_number=%d"
                         "&hash_value=%s"
                         "&server_sig=%s"
                         "&server_name=%s"
                         "&photo_author_id=%d"
                         "&photo_subjects_ids=%s"
                         "&photo_author_name=%s"
                         "&photo_subjects_names=%s"
                         "&jpg_base64=%s"
                         "&negative_jpg_base64=%s"
                         , 
                         encodedEmail,
                         inSequenceNumber,
                         keyHash,
                         serverSig,
                         serverIP,
                         inAuthorID,
                         subjectIDs,
                         inAuthorName,
                         subjectNames,
                         jpegURL,
                         negativeJpegURL );
        delete [] encodedEmail;
        delete [] jpegURL;
        delete [] negativeJpegURL;

        delete [] subjectIDs;
        delete [] subjectNames;
        
        /*
        FILE *bodyFile = fopen( "body.txt", "w" );
        fprintf( bodyFile, "%s", postBody );
        fclose( bodyFile );
        */  

        submitPhotoWebRequest = startWebRequest( "POST", url, postBody );
            
        delete [] postBody;
    
        delete [] url;
        }
    
    if( jpegBase64 != NULL ) {
        delete [] jpegBase64;
        }
    if( negativeJpegBase64 != NULL ) {
        delete [] negativeJpegBase64;
        }

    delete [] serverSig;
    delete [] keyHash;
    }

