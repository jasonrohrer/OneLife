#include "LivingLifePage.h"

#include "objectBank.h"
#include "spriteBank.h"
#include "transitionBank.h"
#include "categoryBank.h"
#include "soundBank.h"
#include "whiteSprites.h"
#include "message.h"
#include "groundSprites.h"

#include "accountHmac.h"

#include "liveObjectSet.h"
#include "ageControl.h"
#include "musicPlayer.h"

#include "emotion.h"

#include "photos.h"

#include "spriteDrawColorOverride.h"


#include "liveAnimationTriggers.h"

#include "../commonSource/fractalNoise.h"
#include "../commonSource/sayLimit.h"


#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/MinPriorityQueue.h"


#include "minorGems/game/Font.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/io/file/File.h"

#include "minorGems/formats/encodingUtils.h"

#include "minorGems/system/Thread.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/crypto/hashes/sha1.h"

#include <stdlib.h>//#include <math.h>


#define OHOL_NON_EDITOR 1
#include "ObjectPickable.h"

static ObjectPickable objectPickable;



#define MAP_D 64
#define MAP_NUM_CELLS 4096

extern int versionNumber;
extern int dataVersionNumber;

extern const char *clientTag;


extern double frameRateFactor;

extern Font *mainFont;
extern Font *numbersFontFixed;
extern Font *mainFontReview;
extern Font *handwritingFont;
extern Font *pencilFont;
extern Font *pencilErasedFont;


// seconds
static double maxCurseTagDisplayGap = 15.0;


// to make all erased pencil fonts lighter
static float pencilErasedFontExtraFade = 0.75;


extern doublePair lastScreenViewCenter;

static char shouldMoveCamera = true;


extern double viewWidth;
extern double viewHeight;

extern int screenW, screenH;

extern char usingCustomServer;
extern char *serverIP;
extern int serverPort;

extern char *userEmail;
extern char *userTwinCode;
extern int userTwinCount;

extern char userReconnect;

static char vogMode = false;
static char vogModeActuallyOn = false;

static doublePair vogPos = { 0, 0 };

static char vogPickerOn = false;

    

extern float musicLoudness;


static JenkinsRandomSource randSource( 340403 );
static JenkinsRandomSource remapRandSource( 340403 );


static int lastScreenMouseX, lastScreenMouseY;
static char mouseDown = false;
static int mouseDownFrames = 0;

static int minMouseDownFrames = 30;


static int screenCenterPlayerOffsetX, screenCenterPlayerOffsetY;


static float lastMouseX = 0;
static float lastMouseY = 0;


// set to true to render for teaser video
static char teaserVideo = false;


static int showBugMessage = 0;
static const char *bugEmail = "jason" "rohrer" "@" "fastmail.fm";



// if true, pressing S key (capital S)
// causes current speech and mask to be saved to the screenShots folder
static char savingSpeechEnabled = false;
static char savingSpeech = false;
static char savingSpeechColor = false;
static char savingSpeechMask = false;

static char savingSpeechNumber = 1;

static char takingPhoto = false;
static GridPos takingPhotoGlobalPos;
static char takingPhotoFlip = false;
static int photoSequenceNumber = -1;
static char waitingForPhotoSig = false;
static char *photoSig = NULL;

// no moving for first 20 seconds of life
static double noMoveAge = 0.20;


static double emotDuration = 10;


static int historyGraphLength = 100;

static char showFPS = false;
static double frameBatchMeasureStartTime = -1;
static int framesInBatch = 0;
static double fpsToDraw = -1;

#define MEASURE_TIME_NUM_CATEGORIES 3


static double timeMeasures[ MEASURE_TIME_NUM_CATEGORIES ];

    
    
static const char *timeMeasureNames[ MEASURE_TIME_NUM_CATEGORIES ] = 
{ "message",
  "updates",
  "drawing" };

static FloatColor timeMeasureGraphColors[ MEASURE_TIME_NUM_CATEGORIES ] = 
{ { 1, 1, 0, 1 },
  { 0, 1, 0, 1 },
  { 1, 0, 0, 1 } };


static double timeMeasureToDraw[ MEASURE_TIME_NUM_CATEGORIES ];


typedef struct TimeMeasureRecord {
        double timeMeasureAverage[ MEASURE_TIME_NUM_CATEGORIES ];
        double total;
    } TimeMeasureRecord;


double runningPixelCount = 0;

double spriteCountToDraw = 0;
double uniqueSpriteCountToDraw = 0;

double pixelCountToDraw = 0;



static SimpleVector<double> fpsHistoryGraph;
static SimpleVector<TimeMeasureRecord> timeMeasureHistoryGraph;
static SimpleVector<double> spriteCountHistoryGraph;
static SimpleVector<double> uniqueSpriteHistoryGraph;
static SimpleVector<double> pixelCountHistoryGraph;


static char showNet = false;
static double netBatchMeasureStartTime = -1;
static int messagesInPerSec = -1;
static int messagesOutPerSec = -1;
static int bytesInPerSec = -1;
static int bytesOutPerSec = -1;
static int messagesInCount = 0;
static int messagesOutCount = 0;
static int bytesInCount = 0;
static int bytesOutCount = 0;

static SimpleVector<double> messagesInHistoryGraph;
static SimpleVector<double> messagesOutHistoryGraph;
static SimpleVector<double> bytesInHistoryGraph;
static SimpleVector<double> bytesOutHistoryGraph;


static SimpleVector<GridPos> graveRequestPos;
static SimpleVector<GridPos> ownerRequestPos;


// IDs of pointed-to offspring that don't exist yet
static SimpleVector<int> ourUnmarkedOffspring;



static char showPing = false;
static double pingSentTime = -1;
static double pongDeltaTime = -1;
static double pingDisplayStartTime = -1;


static double culvertFractalScale = 20;
static double culvertFractalRoughness = 0.62;
static double culvertFractalAmp = 98;


static int usedToolSlots = 0;
static int totalToolSlots = 0;


typedef struct Homeland {
        int x, y;
        char *familyName;
    } Homeland;
    

static SimpleVector<Homeland> homelands;


static Homeland *getHomeland( int inCenterX, int inCenterY ) {
    for( int i=0; i<homelands.size(); i++ ) {
        Homeland *h = homelands.getElement( i );
        
        if( h->x == inCenterX && h->y == inCenterY ) {
            return h;
            }
        }
    return NULL;
    }




typedef struct LocationSpeech {
        doublePair pos;
        char *speech;
        double fade;
        // wall clock time when speech should start fading
        double fadeETATime;
    } LocationSpeech;



SimpleVector<LocationSpeech> locationSpeech;


static void clearLocationSpeech() {
    for( int i=0; i<locationSpeech.size(); i++ ) {
        delete [] locationSpeech.getElementDirect( i ).speech;
        }
    locationSpeech.deleteAll();
    }




// most recent home at end

typedef struct {
        GridPos pos;
        char ancient;
        char temporary;
        char tempPerson;
        int personID;
        const char *tempPersonKey;
        // 0 if not set
        double temporaryExpireETA;
    } HomePos;

    

static SimpleVector<HomePos> homePosStack;

static SimpleVector<HomePos> oldHomePosStack;

// used on reconnect to decide whether to delete old home positions
static int lastPlayerID = -1;



static void processHomePosStack() {
    int num = homePosStack.size();
    if( num > 0 ) {
        HomePos *r = homePosStack.getElement( num - 1 );
        
        if( r->temporary && r->temporaryExpireETA != 0 &&
            game_getCurrentTime() > r->temporaryExpireETA ) {
            // expired
            homePosStack.deleteElement( num - 1 );
            }
        }
    }



static HomePos *getHomePosRecord() {
    processHomePosStack();
    
    int num = homePosStack.size();
    if( num > 0 ) {
        return homePosStack.getElement( num - 1 );
        }
    else {
        return NULL;
        }
    }


// returns pointer to record, NOT destroyed by caller, or NULL if 
// home unknown
static  GridPos *getHomeLocation( char *outTemp, char *outTempPerson,
                                  const char **outTempPersonKey,
                                  char inAncient ) {
    *outTemp = false;
    
    if( inAncient ) {
        GridPos *returnPos = NULL;
        
        if( homePosStack.size() > 0 ) {
            HomePos *r = homePosStack.getElement( 0 );
            if( r->ancient ) {
                returnPos = &( r->pos );
                }
            }
        return returnPos;
        }
        

    HomePos *r = getHomePosRecord();

    // don't consider ancient marker here, if it's the only one
    if( r != NULL && ! r->ancient ) {
        *outTemp = r->temporary;
        *outTempPerson = r->tempPerson;
        *outTempPersonKey = r->tempPersonKey;
        return &( r->pos );
        }
    else {
        return NULL;
        }
    }



static void removeHomeLocation( int inX, int inY ) {
    for( int i=0; i<homePosStack.size(); i++ ) {
        GridPos p = homePosStack.getElementDirect( i ).pos;
        
        if( p.x == inX && p.y == inY ) {
            homePosStack.deleteElement( i );
            break;
            }
        }
    }


static void removeAllTempHomeLocations() {
    for( int i=0; i<homePosStack.size(); i++ ) {
        if( homePosStack.getElementDirect( i ).temporary ) {
            homePosStack.deleteElement( i );
            i --;
            break;
            }
        }
    }



static void addHomeLocation( int inX, int inY ) {
    removeAllTempHomeLocations();

    removeHomeLocation( inX, inY );
    GridPos newPos = { inX, inY };
    HomePos p;
    p.pos = newPos;
    p.ancient = false;
    p.temporary = false;
    
    homePosStack.push_back( p );
    }



// make leader arrow higher priority if /LEADER command typed manually
// (automatic leader arrows are lower priority)
static char leaderCommandTyped = false;



// inPersonKey can be NULL for map temp locations
static int getLocationKeyPriority( const char *inPersonKey ) {
    if( inPersonKey == NULL ||
        // these are all triggered by explicit user actions
        // where they are asking for an arrow to something
        // explicitly touched an expert waystone
        strcmp( inPersonKey, "expt" ) == 0 ||
        // explicitly touched a locked gate
        strcmp( inPersonKey, "owner" ) == 0 ||
        // manually-requested leader arrow
        ( leaderCommandTyped && strcmp( inPersonKey, "lead" ) == 0 ) ) {
        
        leaderCommandTyped = false;
        return 1;
        }
    // this is automatic, out of user control, when they inherit some
    // property
    else if( strcmp( inPersonKey, "property" ) == 0 ) {
        return 2;
        }
    // non-manual leader arrow (like when you receive an order)
    // and supp arrow (automatic, when you issue an order)
    else if( strcmp( inPersonKey, "lead" ) == 0 ||
             strcmp( inPersonKey, "supp" ) == 0 ) {
        return 3;
        }
    else if( strcmp( inPersonKey, "baby" ) == 0 ) {
        return 4;
        }
    else if( strcmp( inPersonKey, "visitor" ) == 0 ) {
        // don't bug owner with spurious visitor arrows, unless there
        // is nothing else going on
        return 5;
        }
    else {
        return 6;
        }
    }
    

// inPersonKey can be NULL for map temp locations
// enforces priority for different classes of temp home locations
static char doesNewTempLocationTrumpPrevious( const char *inPersonKey ) {
    
    // see what our current one is
    const char *currentKey = NULL;
    char currentFound = false;
    
    for( int i=0; i<homePosStack.size(); i++ ) {
        if( homePosStack.getElementDirect( i ).temporary ) {            
            currentKey = homePosStack.getElementDirect( i ).tempPersonKey;
            currentFound = true;
            break;
            }
        }
    
    
    if( ! currentFound ) {
        // no temp location currently
        // all new ones can replace this state
        return true;
        }
    
    if( getLocationKeyPriority( inPersonKey ) <= 
        getLocationKeyPriority( currentKey ) ) {
        return true;
        }
    else {
        return false;
        }
    }



static void addTempHomeLocation( int inX, int inY, 
                                 char inPerson, int inPersonID,
                                 LiveObject *inPersonO,
                                 const char *inPersonKey ) {
    if( ! doesNewTempLocationTrumpPrevious( inPersonKey ) ) {
        // existing key has higher priority
        // don't replace with this new key
        return;
        }
    
    removeAllTempHomeLocations();
    
    GridPos newPos = { inX, inY };
    HomePos p;
    p.pos = newPos;
    p.ancient = false;
    p.temporary = true;
    // no expiration for now
    // until we drop the map
    p.temporaryExpireETA = 0;
    
    p.tempPerson = inPerson;

    p.personID = -1;
    
    p.tempPersonKey = NULL;

    if( inPerson ) {
        // person pointer does not depend on held map
        p.temporaryExpireETA = game_getCurrentTime() + 60;

        if( strcmp( inPersonKey, "expt" ) == 0 ) {
            // 3 minutes total when searching for an expert
            p.temporaryExpireETA += 120;
            }
        
        p.personID = inPersonID;
        p.tempPersonKey = inPersonKey;
                                            
        if( inPersonO != NULL && 
            inPersonO->currentSpeech != NULL &&
            inPersonO->speechIsOverheadLabel ) {
            // clear any old label speech 
            // to make room for new label
            delete [] inPersonO->currentSpeech;
            inPersonO->currentSpeech = NULL;
            inPersonO->speechIsOverheadLabel = false;
            }
        }

    homePosStack.push_back( p );
    }



static void updatePersonHomeLocation( int inPersonID, int inX, int inY ) {
    for( int i=0; i<homePosStack.size(); i++ ) {
        HomePos *p = homePosStack.getElement( i );
        
        if( p->tempPerson && p->personID == inPersonID ) {
            p->pos.x = inX;
            p->pos.y = inY;
            }
        }
    }


char isAncientHomePosHell = false;

static void addAncientHomeLocation( int inX, int inY ) {
    removeHomeLocation( inX, inY );

    // remove all ancient pos
    // there can be only one ancient
    for( int i=0; i<homePosStack.size(); i++ ) {
        if( homePosStack.getElementDirect( i ).ancient ) {
            homePosStack.deleteElement( i );
            i--;
            }
        }

    GridPos newPos = { inX, inY };
    HomePos p;
    p.pos = newPos;
    p.ancient = true;
    p.temporary = false;
    
    homePosStack.push_front( p );
    }






// returns if -1 no home needs to be shown (home unknown)
// otherwise, returns 0..7 index of arrow
static int getHomeDir( doublePair inCurrentPlayerPos, 
                       double *outTileDistance = NULL,
                       char *outTooClose = NULL,
                       char *outTemp = NULL,
                       char *outTempPerson = NULL,
                       const char **outTempPersonKey = NULL,
                       // 1 for ancient marker
                       int inIndex = 0 ) {
    char temporary = false;
    
    char tempPerson = false;
    const char *tempPersonKey;
    
    GridPos *p = getHomeLocation( &temporary, &tempPerson, &tempPersonKey,
                                  ( inIndex == 1 ) );
    
    if( p == NULL ) {
        return -1;
        }
    
    if( outTemp != NULL ) {
        *outTemp = temporary;
        }
    if( outTempPerson != NULL ) {
        *outTempPerson = tempPerson;
        }
    
    if( outTooClose != NULL ) {
        *outTooClose = false;
        }
    
    if( outTempPersonKey != NULL ) {
        *outTempPersonKey = tempPersonKey;
        }
    

    doublePair homePos = { (double)p->x, (double)p->y };
    
    doublePair vector = sub( homePos, inCurrentPlayerPos );

    double dist = length( vector );

    if( outTileDistance != NULL ) {
        *outTileDistance = dist;
        }

    if( dist < 5 ) {
        // too close

        if( outTooClose != NULL ) {
            *outTooClose = true;
            }
        
        if( dist == 0 ) {
            // can't compute angle
            return -1;
            }
        }
    
    
    double a = angle( vector );

    // north is 0
    a -= M_PI / 2; 

    
    if( a <  - M_PI / 8 ) {
        a += 2 * M_PI;
        }
    
    int index = lrint( 8 * a / ( 2 * M_PI ) );
    
    return index;
    }





char *getRelationName( SimpleVector<int> *ourLin, 
                       SimpleVector<int> *theirLin, 
                       int ourID, int theirID,
                       int ourDisplayID, int theirDisplayID,
                       double ourAge, double theirAge,
                       int ourEveID, int theirEveID ) {
    

    ObjectRecord *theirDisplayO = getObject( theirDisplayID );
    
    char theyMale = false;
    
    if( theirDisplayO != NULL ) {
        theyMale = theirDisplayO->male;
        }
    

    if( ourLin->size() == 0 && theirLin->size() == 0 ) {
        // both eve, no relation
        return NULL;
        }

    const char *main = "";
    char grand = false;
    int numGreats = 0;
    int cousinNum = 0;
    int cousinRemovedNum = 0;
    
    char found = false;

    for( int i=0; i<theirLin->size(); i++ ) {
        if( theirLin->getElementDirect( i ) == ourID ) {
            found = true;
            
            if( theyMale ) {
                main = translate( "son" );
                }
            else {
                main = translate( "daughter" );
                }
            if( i > 0  ) {
                grand = true;
                }
            numGreats = i - 1;
            }
        }

    if( ! found ) {
        for( int i=0; i<ourLin->size(); i++ ) {
            if( ourLin->getElementDirect( i ) == theirID ) {
                found = true;
                main = translate( "mother" );
                if( i > 0  ) {
                    grand = true;
                    }
                numGreats = i - 1;
                }
            }
        }
    
    char big = false;
    char little = false;
    char twin = false;
    char identical = false;

    if( ! found ) {
        // not a direct descendent or ancestor

        // look for shared relation
        int ourMatchIndex = -1;
        int theirMatchIndex = -1;
        
        for( int i=0; i<ourLin->size(); i++ ) {
            for( int j=0; j<theirLin->size(); j++ ) {
                
                if( ourLin->getElementDirect( i ) == 
                    theirLin->getElementDirect( j ) ) {
                    ourMatchIndex = i;
                    theirMatchIndex = j;
                    break;
                    }
                }
            if( ourMatchIndex != -1 ) {
                break;
                }
            }
        
        if( ourMatchIndex == -1 ) {
            
            if( ourEveID != -1 && theirEveID != -1 &&
                ourEveID == theirEveID ) {
                // no shared lineage, but same eve beyond lineage cuttoff
                return stringDuplicate( translate( "distantRelative" ) );
                }

            return NULL;
            }
        
        found = true;

        if( theirMatchIndex == 0 && ourMatchIndex == 0 ) {
            if( theyMale ) {
                main = translate( "brother" );
                }
            else {
                main = translate( "sister" );
                }
            
            if( ourAge < theirAge - 0.1 ) {
                big = true;
                }
            else if( ourAge > theirAge + 0.1 ) {
                little = true;
                }
            else {
                // close enough together in age
                twin = true;
                
                if( ourDisplayID == theirDisplayID ) {
                    identical = true;
                    }
                }
            }
        else if( theirMatchIndex == 0 ) {
            if( theyMale ) {
                main = translate( "uncle" );
                }
            else {
                main = translate( "aunt" );
                }
            numGreats = ourMatchIndex - 1;
            }
        else if( ourMatchIndex == 0 ) {
            if( theyMale ) {
                main = translate( "nephew" );
                }
            else {
                main = translate( "niece" );
                }
            numGreats = theirMatchIndex - 1;
            }
        else {
            // cousin of some kind
            
            main = translate( "cousin" );
            
            // shallowest determines cousin number
            // diff determines removed number
            if( ourMatchIndex <= theirMatchIndex ) {
                cousinNum = ourMatchIndex;
                cousinRemovedNum = theirMatchIndex - ourMatchIndex;
                }
            else {
                cousinNum = theirMatchIndex;
                cousinRemovedNum = ourMatchIndex - theirMatchIndex;
                }
            }
        }


    SimpleVector<char> buffer;
    
    buffer.appendElementString( translate( "your" ) );
    buffer.appendElementString( " " );


    if( numGreats <= 4 ) {    
        for( int i=0; i<numGreats; i++ ) {
            buffer.appendElementString( translate( "great" ) );
            }
        }
    else {
        char *greatCount = autoSprintf( "%dX %s", 
                                        numGreats, translate( "great") );
        buffer.appendElementString( greatCount );
        delete [] greatCount;
        }
    
    
    if( grand ) {
        buffer.appendElementString( translate( "grand" ) );
        }
    
    if( cousinNum > 0 ) {
        int remainingCousinNum = cousinNum;

        if( cousinNum >= 30 ) {
            buffer.appendElementString( translate( "distant" ) );
            remainingCousinNum = 0;
            }
        
        if( cousinNum > 20 && cousinNum < 30 ) {
            buffer.appendElementString( translate( "twentyHyphen" ) );
            remainingCousinNum = cousinNum - 20;
            }

        if( remainingCousinNum > 0  ) {
            char *numth = autoSprintf( "%dth", remainingCousinNum );
            buffer.appendElementString( translate( numth ) );
            delete [] numth;
            }
        buffer.appendElementString( " " );
        }

    if( little ) {
        buffer.appendElementString( translate( "little" ) );
        buffer.appendElementString( " " );
        }
    else if( big ) {
        buffer.appendElementString( translate( "big" ) );
        buffer.appendElementString( " " );
        }
    else if( twin ) {
        if( identical ) {
            buffer.appendElementString( translate( "identical" ) );
            buffer.appendElementString( " " );
            }
        
        buffer.appendElementString( translate( "twin" ) );
        buffer.appendElementString( " " );
        }
    
    
    buffer.appendElementString( main );
    
    if( cousinRemovedNum > 0 ) {
        buffer.appendElementString( " " );
        
        if( cousinRemovedNum > 9 ) {
            buffer.appendElementString( translate( "manyTimes" ) );
            }
        else {
            char *numTimes = autoSprintf( "%dTimes", cousinRemovedNum );
            buffer.appendElementString( translate( numTimes ) );
            delete [] numTimes;
            }
        buffer.appendElementString( " " );
        buffer.appendElementString( translate( "removed" ) );
        }

    return buffer.getElementString();
    }



char *getRelationName( LiveObject *inOurObject, LiveObject *inTheirObject ) {
    SimpleVector<int> *ourLin = &( inOurObject->lineage );
    SimpleVector<int> *theirLin = &( inTheirObject->lineage );
    
    int ourID = inOurObject->id;
    int theirID = inTheirObject->id;

    
    return getRelationName( ourLin, theirLin, ourID, theirID,
                            inOurObject->displayID, inTheirObject->displayID,
                            inOurObject->age,
                            inTheirObject->age,
                            inOurObject->lineageEveID,
                            inTheirObject->lineageEveID );
    }








// base speed for animations that aren't speeded up or slowed down
// when player moving at a different speed, anim speed is modified
#define BASE_SPEED 3.75


int numServerBytesRead = 0;
int numServerBytesSent = 0;

int overheadServerBytesSent = 0;
int overheadServerBytesRead = 0;


static char hideGuiPanel = false;




static char *lastMessageSentToServer = NULL;


// destroyed internally if not NULL
static void replaceLastMessageSent( char *inNewMessage ) {
    if( lastMessageSentToServer != NULL ) {
        delete [] lastMessageSentToServer;
        }
    lastMessageSentToServer = inNewMessage;
    }




SimpleVector<unsigned char> serverSocketBuffer;

static char serverSocketConnected = false;
static char serverSocketHardFail = false;
static float connectionMessageFade = 1.0f;
static double connectedTime = 0;

static char forceDisconnect = false;


// reads all waiting data from socket and stores it in buffer
// returns false on socket error
static char readServerSocketFull( int inServerSocket ) {

    if( forceDisconnect ) {
        forceDisconnect = false;
        return false;
        }
    

    unsigned char buffer[512];
    
    int numRead = readFromSocket( inServerSocket, buffer, 512 );
    
    
    while( numRead > 0 ) {
        if( ! serverSocketConnected ) {    
            serverSocketConnected = true;
            connectedTime = game_getCurrentTime();
            }
        
        serverSocketBuffer.appendArray( buffer, numRead );
        numServerBytesRead += numRead;
        bytesInCount += numRead;
        
        numRead = readFromSocket( inServerSocket, buffer, 512 );
        }    

    if( numRead == -1 ) {
        printf( "Failed to read from server socket at time %f\n",
                game_getCurrentTime() );
        return false;
        }
    
    return true;
    }



static double timeLastMessageSent = 0;



void LivingLifePage::sendToServerSocket( char *inMessage ) {
    timeLastMessageSent = game_getCurrentTime();
    
    printf( "Sending message to server: %s\n", inMessage );
    
    replaceLastMessageSent( stringDuplicate( inMessage ) );    

    int len = strlen( inMessage );
    
    int numSent = sendToSocket( mServerSocket, (unsigned char*)inMessage, len );
    
    if( numSent == len ) {
        numServerBytesSent += len;
        overheadServerBytesSent += 52;
        
        messagesOutCount++;
        bytesOutCount += len;
        }
    else {
        printf( "Failed to send message to server socket "
                "at time %f "
                "(tried to send %d, but numSent=%d)\n", 
                game_getCurrentTime(), len, numSent );
        closeSocket( mServerSocket );
        mServerSocket = -1;

        if( mFirstServerMessagesReceived  ) {
            
            if( mDeathReason != NULL ) {
                delete [] mDeathReason;
                }
            mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );
            
            handleOurDeath( true );
            }
        else {
            setWaiting( false );
            
            if( userReconnect ) {
                setSignal( "reconnectFailed" );
                }
            else {
                setSignal( "loginFailed" );
                }
            }
        }
    
    }




static char equal( GridPos inA, GridPos inB ) {
    if( inA.x == inB.x && inA.y == inB.y ) {
        return true;
        }
    return false;
    }


static double distance2( GridPos inA, GridPos inB ) {
    int dX = inA.x - inB.x;
    int dY = inA.y - inB.y;
    
    return dX * dX + dY * dY;
    }



static void printPath( GridPos *inPath, int inLength ) {
    for( int i=0; i<inLength; i++ ) {
        printf( "(%d,%d) ", inPath[i].x, inPath[i].y );
        }
    printf( "\n" );
    }


static void removeDoubleBacksFromPath( GridPos **inPath, int *inLength ) {
    
    SimpleVector<GridPos> filteredPath;
    
    int dbA = -1;
    int dbB = -1;
    
    int longestDB = 0;

    GridPos *path = *inPath;
    int length = *inLength;

    for( int e=0; e<length; e++ ) {
                                    
        for( int f=e; f<length; f++ ) {
            
            if( equal( path[e],
                       path[f] ) ) {
                                            
                int dist = f - e;
                                            
                if( dist > longestDB ) {
                                                
                    dbA = e;
                    dbB = f;
                    longestDB = dist;
                    }
                }
            }
        }
                                
    if( longestDB > 0 ) {
                                    
        printf( "Removing loop with %d elements\n",
                longestDB );

        for( int e=0; e<=dbA; e++ ) {
            filteredPath.push_back( 
                path[e] );
            }
                                    
        // skip loop between

        for( int e=dbB + 1; e<length; e++ ) {
            filteredPath.push_back( 
                path[e] );
            }
                                    
        *inLength = filteredPath.size();
                                    
        delete [] path;
                                    
        *inPath = 
            filteredPath.getElementArray();
        }
    }



static double computeCurrentAgeNoOverride( LiveObject *inObj ) {
    if( inObj->finalAgeSet ) {
        return inObj->age;
        }
    else {
        return inObj->age + 
            inObj->ageRate * ( game_getCurrentTime() - inObj->lastAgeSetTime );
        }
    }




static double computeCurrentAge( LiveObject *inObj ) {
    if( inObj->finalAgeSet ) {
        return inObj->age;
        }
    else {
        if( inObj->tempAgeOverrideSet ) {
            double curTime = game_getCurrentTime();
            
            if( curTime - inObj->tempAgeOverrideSetTime < 5 ) {
                // baby cries for 5 seconds each time they speak
            
                // update age using clock
                return inObj->tempAgeOverride + 
                    inObj->ageRate * 
                    ( curTime - inObj->tempAgeOverrideSetTime );
                }
            else {
                // temp override over
                inObj->tempAgeOverrideSet = false;
                }
            }
        
        return computeCurrentAgeNoOverride( inObj );
        }
    
    }






static char *getDisplayObjectDescription( int inID ) {
    ObjectRecord *o = getObject( inID );
    if( o == NULL ) {
        return NULL;
        }
    char *upper = stringToUpperCase( o->description );
    stripDescriptionComment( upper );
    return upper;
    }



typedef enum messageType {
    SHUTDOWN,
    SERVER_FULL,
	SEQUENCE_NUMBER,
    ACCEPTED,
    NO_LIFE_TOKENS,
    REJECTED,
    MAP_CHUNK,
    MAP_CHANGE,
    PLAYER_UPDATE,
    PLAYER_MOVES_START,
    PLAYER_OUT_OF_RANGE,
    BABY_WIGGLE,
    PLAYER_SAYS,
    LOCATION_SAYS,
    PLAYER_EMOT,
    FOOD_CHANGE,
    HEAT_CHANGE,
    LINEAGE,
    CURSED,
    CURSE_TOKEN_CHANGE,
    CURSE_SCORE,
    NAMES,
    APOCALYPSE,
    APOCALYPSE_DONE,
    DYING,
    HEALED,
    POSSE_JOIN,
    MONUMENT_CALL,
    GRAVE,
    GRAVE_MOVE,
    GRAVE_OLD,
    OWNER,
    FOLLOWING,
    EXILED,
    VALLEY_SPACING,
    FLIGHT_DEST,
    BAD_BIOMES,
    VOG_UPDATE,
    PHOTO_SIGNATURE,
    FORCED_SHUTDOWN,
    GLOBAL_MESSAGE,
    WAR_REPORT,
    LEARNED_TOOL_REPORT,
    TOOL_EXPERTS,
    TOOL_SLOTS,
    HOMELAND,
    FLIP,
    CRAVING,
    PONG,
    COMPRESSED_MESSAGE,
    UNKNOWN
    } messageType;



messageType getMessageType( char *inMessage ) {
    char *copy = stringDuplicate( inMessage );
    
    char *firstBreak = strstr( copy, "\n" );
    
    if( firstBreak == NULL ) {
        delete [] copy;
        return UNKNOWN;
        }
    
    firstBreak[0] = '\0';
    
    messageType returnValue = UNKNOWN;

    if( strcmp( copy, "CM" ) == 0 ) {
        returnValue = COMPRESSED_MESSAGE;
        }
    else if( strcmp( copy, "MC" ) == 0 ) {
        returnValue = MAP_CHUNK;
        }
    else if( strcmp( copy, "MX" ) == 0 ) {
        returnValue = MAP_CHANGE;
        }
    else if( strcmp( copy, "PU" ) == 0 ) {
        returnValue = PLAYER_UPDATE;
        }
    else if( strcmp( copy, "PM" ) == 0 ) {
        returnValue = PLAYER_MOVES_START;
        }
    else if( strcmp( copy, "PO" ) == 0 ) {
        returnValue = PLAYER_OUT_OF_RANGE;
        }
    else if( strcmp( copy, "BW" ) == 0 ) {
        returnValue = BABY_WIGGLE;
        }
    else if( strcmp( copy, "PS" ) == 0 ) {
        returnValue = PLAYER_SAYS;
        }
    else if( strcmp( copy, "LS" ) == 0 ) {
        returnValue = LOCATION_SAYS;
        }
    else if( strcmp( copy, "PE" ) == 0 ) {
        returnValue = PLAYER_EMOT;
        }
    else if( strcmp( copy, "FX" ) == 0 ) {
        returnValue = FOOD_CHANGE;
        }
    else if( strcmp( copy, "HX" ) == 0 ) {
        returnValue = HEAT_CHANGE;
        }
    else if( strcmp( copy, "LN" ) == 0 ) {
        returnValue = LINEAGE;
        }
    else if( strcmp( copy, "CU" ) == 0 ) {
        returnValue = CURSED;
        }
    else if( strcmp( copy, "CX" ) == 0 ) {
        returnValue = CURSE_TOKEN_CHANGE;
        }
    else if( strcmp( copy, "CS" ) == 0 ) {
        returnValue = CURSE_SCORE;
        }
    else if( strcmp( copy, "NM" ) == 0 ) {
        returnValue = NAMES;
        }
    else if( strcmp( copy, "AP" ) == 0 ) {
        returnValue = APOCALYPSE;
        }
    else if( strcmp( copy, "AD" ) == 0 ) {
        returnValue = APOCALYPSE_DONE;
        }
    else if( strcmp( copy, "DY" ) == 0 ) {
        returnValue = DYING;
        }
    else if( strcmp( copy, "HE" ) == 0 ) {
        returnValue = HEALED;
        }
    else if( strcmp( copy, "PJ" ) == 0 ) {
        returnValue = POSSE_JOIN;
        }
    else if( strcmp( copy, "MN" ) == 0 ) {
        returnValue = MONUMENT_CALL;
        }
    else if( strcmp( copy, "GV" ) == 0 ) {
        returnValue = GRAVE;
        }
    else if( strcmp( copy, "GM" ) == 0 ) {
        returnValue = GRAVE_MOVE;
        }
    else if( strcmp( copy, "GO" ) == 0 ) {
        returnValue = GRAVE_OLD;
        }
    else if( strcmp( copy, "OW" ) == 0 ) {
        returnValue = OWNER;
        }
    else if( strcmp( copy, "FW" ) == 0 ) {
        returnValue = FOLLOWING;
        }
    else if( strcmp( copy, "EX" ) == 0 ) {
        returnValue = EXILED;
        }
    else if( strcmp( copy, "VS" ) == 0 ) {
        returnValue = VALLEY_SPACING;
        }
    else if( strcmp( copy, "FD" ) == 0 ) {
        returnValue = FLIGHT_DEST;
        }
    else if( strcmp( copy, "BB" ) == 0 ) {
        returnValue = BAD_BIOMES;
        }
    else if( strcmp( copy, "VU" ) == 0 ) {
        returnValue = VOG_UPDATE;
        }
    else if( strcmp( copy, "PH" ) == 0 ) {
        returnValue = PHOTO_SIGNATURE;
        }
    else if( strcmp( copy, "PONG" ) == 0 ) {
        returnValue = PONG;
        }
    else if( strcmp( copy, "SHUTDOWN" ) == 0 ) {
        returnValue = SHUTDOWN;
        }
    else if( strcmp( copy, "SERVER_FULL" ) == 0 ) {
        returnValue = SERVER_FULL;
        }
    else if( strcmp( copy, "SN" ) == 0 ) {
        returnValue = SEQUENCE_NUMBER;
        }
    else if( strcmp( copy, "ACCEPTED" ) == 0 ) {
        returnValue = ACCEPTED;
        }
    else if( strcmp( copy, "REJECTED" ) == 0 ) {
        returnValue = REJECTED;
        }
    else if( strcmp( copy, "NO_LIFE_TOKENS" ) == 0 ) {
        returnValue = NO_LIFE_TOKENS;
        }
    else if( strcmp( copy, "SD" ) == 0 ) {
        returnValue = FORCED_SHUTDOWN;
        }
    else if( strcmp( copy, "MS" ) == 0 ) {
        returnValue = GLOBAL_MESSAGE;
        }
    else if( strcmp( copy, "WR" ) == 0 ) {
        returnValue = WAR_REPORT;
        }
    else if( strcmp( copy, "LR" ) == 0 ) {
        returnValue = LEARNED_TOOL_REPORT;
        }
    else if( strcmp( copy, "TE" ) == 0 ) {
        returnValue = TOOL_EXPERTS;
        }
    else if( strcmp( copy, "TS" ) == 0 ) {
        returnValue = TOOL_SLOTS;
        }
    else if( strcmp( copy, "HL" ) == 0 ) {
        returnValue = HOMELAND;
        }
    else if( strcmp( copy, "FL" ) == 0 ) {
        returnValue = FLIP;
        }
    else if( strcmp( copy, "CR" ) == 0 ) {
        returnValue = CRAVING;
        }
    
    delete [] copy;
    return returnValue;
    }



doublePair getVectorFromCamera( int inMapX, int inMapY ) {
    doublePair vector = 
        { inMapX - lastScreenViewCenter.x / CELL_D, 
          inMapY - lastScreenViewCenter.y / CELL_D };
    
    return vector;
    }

                                 


char *pendingMapChunkMessage = NULL;
int pendingCompressedChunkSize;

char pendingCMData = false;
int pendingCMCompressedSize = 0;
int pendingCMDecompressedSize = 0;


SimpleVector<char*> readyPendingReceivedMessages;

static double lastServerMessageReceiveTime = 0;

// while player action pending, measure largest gap between sequential 
// server messages
// This is an approximation of our outtage time.
static double largestPendingMessageTimeGap = 0;

static char waitForFrameMessages = false;


// NULL if there's no full message available
char *getNextServerMessageRaw() {        

    if( pendingMapChunkMessage != NULL ) {
        // wait for full binary data chunk to arrive completely
        // after message before we report that the message is ready

        if( serverSocketBuffer.size() >= pendingCompressedChunkSize ) {
            char *returnMessage = pendingMapChunkMessage;
            pendingMapChunkMessage = NULL;

            messagesInCount++;

            return returnMessage;
            }
        else {
            // wait for more data to arrive before saying this MC message
            // is ready
            return NULL;
            }
        }
    
    if( pendingCMData ) {
        if( serverSocketBuffer.size() >= pendingCMCompressedSize ) {
            pendingCMData = false;
            
            unsigned char *compressedData = 
                new unsigned char[ pendingCMCompressedSize ];
            
            for( int i=0; i<pendingCMCompressedSize; i++ ) {
                compressedData[i] = serverSocketBuffer.getElementDirect( i );
                }
            serverSocketBuffer.deleteStartElements( pendingCMCompressedSize );
            
            unsigned char *decompressedMessage =
                zipDecompress( compressedData, 
                               pendingCMCompressedSize,
                               pendingCMDecompressedSize );

            delete [] compressedData;

            if( decompressedMessage == NULL ) {
                printf( "Decompressing CM message failed\n" );
                return NULL;
                }
            else {
                char *textMessage = new char[ pendingCMDecompressedSize + 1 ];
                memcpy( textMessage, decompressedMessage,
                       pendingCMDecompressedSize );
                textMessage[ pendingCMDecompressedSize ] = '\0';
                
                delete [] decompressedMessage;
                
                messagesInCount++;
                return textMessage;
                }
            }
        else {
            // wait for more data to arrive
            return NULL;
            }
        }
    


    // find first terminal character #

    int index = serverSocketBuffer.getElementIndex( '#' );
        
    if( index == -1 ) {
        return NULL;
        }

    // terminal character means message arrived

    double curTime = game_getCurrentTime();
    
    double gap = curTime - lastServerMessageReceiveTime;
    
    if( gap > largestPendingMessageTimeGap ) {
        largestPendingMessageTimeGap = gap;
        }

    lastServerMessageReceiveTime = curTime;


    
    char *message = new char[ index + 1 ];
    
    for( int i=0; i<index; i++ ) {
        message[i] = (char)( serverSocketBuffer.getElementDirect( i ) );
        }
    // delete message and terminal character
    serverSocketBuffer.deleteStartElements( index + 1 );
    
    message[ index ] = '\0';

    if( getMessageType( message ) == MAP_CHUNK ) {
        pendingMapChunkMessage = message;
        
        int sizeX, sizeY, x, y, binarySize;
        sscanf( message, "MC\n%d %d %d %d\n%d %d\n", 
                &sizeX, &sizeY,
                &x, &y, &binarySize, &pendingCompressedChunkSize );


        return getNextServerMessageRaw();
        }
    else if( getMessageType( message ) == COMPRESSED_MESSAGE ) {
        pendingCMData = true;
        
        printf( "Got compressed message header:\n%s\n\n", message );

        sscanf( message, "CM\n%d %d\n", 
                &pendingCMDecompressedSize, &pendingCMCompressedSize );

        delete [] message;
        return NULL;
        }
    else {
        messagesInCount++;
        return message;
        }
    }



char serverFrameReady;
static SimpleVector<char*> serverFrameMessages;


// either returns a pending recieved message (one that was received earlier
// or held back
//
// or receives the next message from the server socket (if we are not waiting
// for full frames of messages)
//
// or returns NULL until a full frame of messages is available, and
// then returns the first message from the frame
char *getNextServerMessage() {
    
    if( readyPendingReceivedMessages.size() > 0 ) {
        char *message = readyPendingReceivedMessages.getElementDirect( 0 );
        readyPendingReceivedMessages.deleteElement( 0 );
        printf( "Playing a held pending message\n" );
        return message;
        }
    
    if( !waitForFrameMessages ) {
        return getNextServerMessageRaw();
        }
    else {
        if( !serverFrameReady ) {
            // read more and look for end of frame
            
            char *message = getNextServerMessageRaw();
            
            while( message != NULL ) {
                messageType t = getMessageType( message );
                
                if( strstr( message, "FM" ) == message ) {
                    // end of frame, discard the marker message
                    delete [] message;
                    
                    if( serverFrameMessages.size() > 0 ) {
                        serverFrameReady = true;
                        // see end of frame, stop reading more messages
                        // for now (they are part of next frame)
                        // and start returning message to caller from
                        // this frame
                        break;
                        }
                    }
                else if( t == MAP_CHUNK ||
                         t == PONG ||
                         t == FLIGHT_DEST ||
                         t == PHOTO_SIGNATURE ) {
                    // map chunks are followed by compressed data
                    // they cannot be queued
                    
                    // PONG messages should be returned instantly
                    
                    // FLIGHT_DEST messages also should be returned instantly
                    // otherwise, they will be queued and seen by 
                    // the client after the corresponding MC message
                    // for the new location.
                    // which will invalidate the map around player's old
                    // location
                    return message;
                    }
                else {
                    // some other message in the middle of the frame
                    // keep it
                    serverFrameMessages.push_back( message );
                    }
                
                // keep reading messages, until we either see the 
                // end of the frame or read all available messages
                message = getNextServerMessageRaw();
                }
            }

        if( serverFrameReady ) {
            char *message = serverFrameMessages.getElementDirect( 0 );
            
            serverFrameMessages.deleteElement( 0 );

            if( serverFrameMessages.size() == 0 ) {
                serverFrameReady = false;
                }
            return message;
            }
        else {
            return NULL;
            }
        }
    }







doublePair gridToDouble( GridPos inGridPos ) {
    doublePair d = { (double) inGridPos.x, (double) inGridPos.y };
    
    return d;
    }





static char isGridAdjacent( int inXA, int inYA, int inXB, int inYB ) {
    if( ( abs( inXA - inXB ) == 1 && inYA == inYB ) 
        ||
        ( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {
        
        return true;
        }

    return false;
    }



static GridPos sub( GridPos inA, GridPos inB ) {
    GridPos result = { inA.x - inB.x, inA.y - inB.y };
    
    return result;
    }






// measure a possibly truncated path, compensating for diagonals
static double measurePathLength( LiveObject *inObject,
                                 int inPathLength ) {
    // diags are square root of 2 in length
    double diagLength = 1.41421356237;
    

    double totalLength = 0;
    
    if( inPathLength < 2 ) {
        return totalLength;
        }
    

    GridPos lastPos = inObject->pathToDest[0];
    
    for( int i=1; i<inPathLength; i++ ) {
        
        GridPos thisPos = inObject->pathToDest[i];
        
        if( thisPos.x != lastPos.x &&
            thisPos.y != lastPos.y ) {
            totalLength += diagLength;
            }
        else {
            // not diag
            totalLength += 1;
            }
        lastPos = thisPos;
        }
    
    return totalLength;
    }




double LivingLifePage::computePathSpeedMod( LiveObject *inObject,
                                            int inPathLength ) {
    
    if( inPathLength < 1 ) {
        return 1;
        }    

    GridPos lastPos = inObject->pathToDest[0];
    
    int mapI = getMapIndex( lastPos.x, lastPos.y );

    if( mapI == -1 ) {
        return 1;
        }
    int floor = mMapFloors[ mapI ];
    
    if( floor <= 0 ) {
        return 1;
        }
    double speedMult = getObject( floor )->speedMult;
    
    if( speedMult == 1 ) {
        return 1;
        }

    for( int i=1; i<inPathLength; i++ ) {
        
        GridPos thisPos = inObject->pathToDest[i];

        mapI = getMapIndex( thisPos.x, thisPos.y );

        if( mapI == -1 ) {
            return 1;
            }
        int thisFloor = mMapFloors[ mapI ];
    
        if( ! sameRoadClass( floor, thisFloor ) ) {
            return 1;
            }
        }
    return speedMult;
    }





// youngest last
SimpleVector<LiveObject> gameObjects;

// for determining our ID when we're not youngest on the server
// (so we're not last in the list after receiving the first PU message)
int recentInsertedGameObjectIndex = -1;



static LiveObject *getGameObject( int inID ) {
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == inID ) {
            return o;
            }
        }
    return NULL;
    }



extern char autoAdjustFramerate;
extern int baseFramesPerSecond;


void updateMoveSpeed( LiveObject *inObject ) {
    double etaSec = inObject->moveEtaTime - game_getCurrentTime();
    
    double moveLeft = measurePathLength( inObject, inObject->pathLength ) -
        measurePathLength( inObject, inObject->currentPathStep + 1 );
    

    // count number of turns, which we execute faster than we should
    // because of path smoothing,
    // and use them to reduce overall speed to compensate
    int numTurns = 0;

    if( inObject->currentPathStep < inObject->pathLength - 1 ) {
        GridPos lastDir = sub( 
            inObject->pathToDest[inObject->currentPathStep + 1],
            inObject->pathToDest[inObject->currentPathStep] );
        
        for( int p=inObject->currentPathStep+1; 
             p<inObject->pathLength -1; p++ ) {
            
            GridPos dir = sub( 
                inObject->pathToDest[p+1],
                inObject->pathToDest[p] );

            if( !equal( dir, lastDir ) ) {
                numTurns++;
                lastDir = dir;
                }
            }

        }
    
    double turnTimeBoost = 0.08 * numTurns;

    etaSec += turnTimeBoost;
    
    if( etaSec < 0.1 ) {
        // less than 1/10 of a second
        // this includes 0 values and negative values
        // we DO NOT want infinite or negative move speeds

        printf( "updateMoveSpeed sees etaSec of %f, too low, "
                "upping to 0.1 sec\n", etaSec );
        
        etaSec = 0.1;
        }
    

    double speedPerSec = moveLeft / etaSec;


    // pretend that frame rate is constant
    double fps = baseFramesPerSecond / frameRateFactor;
    
    inObject->currentSpeed = speedPerSec / fps;
    printf( "fixed speed = %f\n", inObject->currentSpeed );
    
    inObject->currentGridSpeed = speedPerSec;
    
    // slow move speed for testing
    //inObject->currentSpeed *= 0.25;

    inObject->timeOfLastSpeedUpdate = game_getCurrentTime();
    }



static void fixSingleStepPath( LiveObject *inObject ) {
    
    printf( "Fix for overtruncated, single-step path for player %d\n", 
            inObject->id );
    
    // trimmed path too short
    // needs to have at least
    // a start and end pos
    
    // give it an artificial
    // start pos
    

    doublePair nextWorld =
        gridToDouble( 
         inObject->pathToDest[0] );

    
    doublePair vectorAway;

    if( ! equal( 
            inObject->currentPos,
            nextWorld ) ) {
            
        vectorAway = normalize(
            sub( 
                inObject->
                currentPos,
                nextWorld ) );
        }
    else {
        vectorAway.x = 1;
        vectorAway.y = 0;
        }
    
    GridPos oldPos =
        inObject->pathToDest[0];
    
    delete [] inObject->pathToDest;
    inObject->pathLength = 2;
    
    inObject->pathToDest =
        new GridPos[2];
    inObject->pathToDest[0].x =
        oldPos.x + vectorAway.x;
    inObject->pathToDest[0].y =
        oldPos.y + vectorAway.y;
    
    inObject->pathToDest[1] =
        oldPos;
    }




char LivingLifePage::isBadBiome( int inMapI ) {
    int b = mMapBiomes[inMapI];
        
    if( mMapFloors[inMapI] == 0 &&
        mBadBiomeIndices.getElementIndex( b ) != -1 ) {
        return true;
        }
    return false;
    }



// should match limits on server
static int pathFindingD = 32;
static int maxChunkDimension = 32;



static char isAutoClick = false;


static void findClosestPathSpot( LiveObject *inObject ) {
    
    GridPos start;
    start.x = lrint( inObject->currentPos.x );
    start.y = lrint( inObject->currentPos.y );

    // make sure start is on our last path, if we have one
    if( inObject->pathToDest != NULL ) {
        char startOnPath = false;

        for( int i=0; i<inObject->pathLength; i++ ) {
            if( inObject->pathToDest[i].x == start.x &&
                inObject->pathToDest[i].y == start.y ) {
                startOnPath = true;
                break;
                }
            }
        
        if( ! startOnPath ) {
            // find closest spot on old path
            int closestI = -1;
            int closestD2 = 9999999;
            
            for( int i=0; i<inObject->pathLength; i++ ) {
                int dx = inObject->pathToDest[i].x - start.x;
                int dy = inObject->pathToDest[i].y - start.y;
                
                int d2 = dx * dx + dy * dy;
                
                if( d2 < closestD2 ) {
                    closestD2 = d2;
                    closestI = i;
                    }
                }

            start = inObject->pathToDest[ closestI ];
            }
        }
    else {
        // no path exists to find closest point on
        // our last known grid position is our closest point
        start.x = inObject->xServer;
        start.y = inObject->yServer;
        }
    
    inObject->closestPathPos = start;
    }




void LivingLifePage::computePathToDest( LiveObject *inObject ) {
    
    GridPos start = inObject->closestPathPos;
    
    
    int startInd = getMapIndex( start.x, start.y );
    
    char startBiomeBad = false;
    char startPointBad = false;
    
    int startPointBadBiome = -1;

    if( startInd != -1 ) {
        // count as bad if we're not already standing on edge of bad biome
        // or in it
        startPointBad = isBadBiome( startInd );

        if( startPointBad ) {
            startPointBadBiome = mMapBiomes[ startInd ];
            }
        
        if( startPointBad ||
            isBadBiome( startInd - 1 ) ||
            isBadBiome( startInd + 1 ) ||
            isBadBiome( startInd - mMapD ) ||
            isBadBiome( startInd + mMapD ) ) {
            
            startBiomeBad = true;
            }

        if( isAutoClick && ! startPointBad ) {
            // don't allow auto clicking into bad biome from good
            startBiomeBad = false;
            }
        }
    
    GridPos end = { inObject->xd, inObject->yd };


    char destBiomeBad = isBadBiome( getMapIndex( end.x, end.y ) );    

    char ignoreBad = false;
    
    if( inObject->holdingID > 0 &&
        getObject( inObject->holdingID )->rideable ) {
        // ride through bad biomes without stopping at edges
        ignoreBad = true;
        }
    


    if( inObject->pathToDest != NULL ) {
        delete [] inObject->pathToDest;
        inObject->pathToDest = NULL;
        }


        
    // window around player's start position
    int numPathMapCells = pathFindingD * pathFindingD;
    char *blockedMap = new char[ numPathMapCells ];

    // assume all blocked
    memset( blockedMap, true, numPathMapCells );

    int pathOffsetX = pathFindingD/2 - start.x;
    int pathOffsetY = pathFindingD/2 - start.y;


    for( int y=0; y<pathFindingD; y++ ) {
        int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;
        
        for( int x=0; x<pathFindingD; x++ ) {
            int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;
            
            if( mapY >= 0 && mapY < mMapD &&
                mapX >= 0 && mapX < mMapD ) { 

                int mapI = mapY * mMapD + mapX;
            
                // note that unknowns (-1) count as blocked too
                if( mMap[ mapI ] == 0
                    ||
                    ( mMap[ mapI ] != -1 && 
                      ! getObject( mMap[ mapI ] )->blocksWalking ) ) {
                    
                    blockedMap[ y * pathFindingD + x ] = false;
                    }

                if( ! ignoreBad 
                    && 
                    ( ! startBiomeBad || ! destBiomeBad )
                    &&
                    ! startPointBad
                    &&
                    mMapFloors[ mapI ] == 0 &&
                    mBadBiomeIndices.getElementIndex( mMapBiomes[ mapI ] ) 
                    != -1 ) {
                    // route around bad biomes on long paths

                    blockedMap[ y * pathFindingD + x ] = true;
                    }
                else if( ! ignoreBad &&
                         startPointBad &&
                         startPointBadBiome != -1 &&
                         mMapFloors[ mapI ] == 0 &&
                         mBadBiomeIndices.getElementIndex( mMapBiomes[ mapI ] ) 
                         != -1 
                         && 
                         mMapBiomes[ mapI ] != startPointBadBiome ) {
                    // crossing from one bad biome to another
                    blockedMap[ y * pathFindingD + x ] = true;
                    }
                }
            }
        }

    // now add extra blocked spots for wide objects
    for( int y=0; y<pathFindingD; y++ ) {
        int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;
        
        for( int x=0; x<pathFindingD; x++ ) {
            int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;
            
            if( mapY >= 0 && mapY < mMapD &&
                mapX >= 0 && mapX < mMapD ) { 

                int mapI = mapY * mMapD + mapX;
                
                if( mMap[ mapI ] > 0 ) {
                    ObjectRecord *o = getObject( mMap[ mapI ] );
                    
                    if( o->wide ) {
                        
                        for( int dx = - o->leftBlockingRadius;
                             dx <= o->rightBlockingRadius; dx++ ) {
                            
                            int newX = x + dx;
                            
                            if( newX >=0 && newX < pathFindingD ) {
                                blockedMap[ y * pathFindingD + newX ] = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    
    
    start.x += pathOffsetX;
    start.y += pathOffsetY;
    
    end.x += pathOffsetX;
    end.y += pathOffsetY;
    
    double startTime = game_getCurrentTime();

    GridPos closestFound;
    
    char pathFound = false;
    
    if( inObject->useWaypoint ) {
        GridPos waypoint = { inObject->waypointX, inObject->waypointY };
        waypoint.x += pathOffsetX;
        waypoint.y += pathOffsetY;
        
        pathFound = pathFind( pathFindingD, pathFindingD,
                              blockedMap, 
                              start, waypoint, end, 
                              &( inObject->pathLength ),
                              &( inObject->pathToDest ),
                              &closestFound );
        if( pathFound && inObject->pathToDest != NULL &&
            inObject->pathLength > inObject->maxWaypointPathLength ) {
            
            // path through waypoint too long, use waypoint as dest
            // instead
            delete [] inObject->pathToDest;
            pathFound = pathFind( pathFindingD, pathFindingD,
                                  blockedMap, 
                                  start, waypoint, 
                                  &( inObject->pathLength ),
                                  &( inObject->pathToDest ),
                                  &closestFound );
            inObject->xd = inObject->waypointX;
            inObject->yd = inObject->waypointY;
            inObject->destTruncated = false;
            }
        }
    else {
        pathFound = pathFind( pathFindingD, pathFindingD,
                              blockedMap, 
                              start, end, 
                              &( inObject->pathLength ),
                              &( inObject->pathToDest ),
                              &closestFound );
        }
        

    if( pathFound && inObject->pathToDest != NULL ) {
        printf( "Path found in %f ms\n", 
                1000 * ( game_getCurrentTime() - startTime ) );

        // move into world coordinates
        for( int i=0; i<inObject->pathLength; i++ ) {
            inObject->pathToDest[i].x -= pathOffsetX;
            inObject->pathToDest[i].y -= pathOffsetY;
            }

        inObject->shouldDrawPathMarks = false;
        
        // up, down, left, right
        int dirsInPath[4] = { 0, 0, 0, 0 };
        
        for( int i=1; i<inObject->pathLength; i++ ) {
            if( inObject->pathToDest[i].x > inObject->pathToDest[i-1].x ) {
                dirsInPath[3]++;
                }
            if( inObject->pathToDest[i].x < inObject->pathToDest[i-1].x ) {
                dirsInPath[2]++;
                }
            if( inObject->pathToDest[i].y > inObject->pathToDest[i-1].y ) {
                dirsInPath[1]++;
                }
            if( inObject->pathToDest[i].y < inObject->pathToDest[i-1].y ) {
                dirsInPath[0]++;
                }
            }
        
        if( ( dirsInPath[0] > 1 && dirsInPath[1] > 1 )
            ||
            ( dirsInPath[2] > 1 && dirsInPath[3] > 1 ) ) {

            // path contains switchbacks, making in confusing without
            // path marks
            inObject->shouldDrawPathMarks = true;
            }
        
        GridPos aGridPos = inObject->pathToDest[0];
        GridPos bGridPos = inObject->pathToDest[1];
        
        doublePair aPos = { (double)aGridPos.x, (double)aGridPos.y };
        doublePair bPos = { (double)bGridPos.x, (double)bGridPos.y };
        
        inObject->currentMoveDirection =
            normalize( sub( bPos, aPos ) );
        }
    else {
        printf( "Path not found in %f ms\n", 
                1000 * ( game_getCurrentTime() - startTime ) );
        
        if( !pathFound ) {
            
            inObject->closestDestIfPathFailedX = 
                closestFound.x - pathOffsetX;
            
            inObject->closestDestIfPathFailedY = 
                closestFound.y - pathOffsetY;
            }
        else {
            // degen case where start == end?
            inObject->closestDestIfPathFailedX = inObject->xd;
            inObject->closestDestIfPathFailedY = inObject->yd;
            }
        
        
        }
    
    inObject->currentPathStep = 0;
    inObject->numFramesOnCurrentStep = 0;
    inObject->onFinalPathStep = false;
    
    delete [] blockedMap;
    }



static void addNewAnimDirect( LiveObject *inObject, AnimType inNewAnim ) {
    inObject->lastAnim = inObject->curAnim;
    inObject->curAnim = inNewAnim;
    inObject->lastAnimFade = 1;
            
    inObject->lastAnimationFrameCount = inObject->animationFrameCount;

    if( inObject->lastAnim == moving ) {
        inObject->frozenRotFrameCount = inObject->lastAnimationFrameCount;
        inObject->frozenRotFrameCountUsed = false;
        }
    else if( inObject->curAnim == moving &&
             inObject->lastAnim != moving &&
             inObject->frozenRotFrameCountUsed ) {
        // switching back to moving
        // resume from where frozen
        inObject->animationFrameCount = inObject->frozenRotFrameCount;
        }
    else if( ( inObject->curAnim == ground || inObject->curAnim == ground2 )
             &&
             inObject->lastAnim == held ) {
        // keep old frozen frame count as we transition away
        // from held
        }
    }



static void addNewHeldAnimDirect( LiveObject *inObject, AnimType inNewAnim ) {
    inObject->lastHeldAnim = inObject->curHeldAnim;
    inObject->curHeldAnim = inNewAnim;
    inObject->lastHeldAnimFade = 1;
    
    inObject->lastHeldAnimationFrameCount = 
        inObject->heldAnimationFrameCount;

    if( inObject->lastHeldAnim == moving ) {
        inObject->heldFrozenRotFrameCount = 
            inObject->lastHeldAnimationFrameCount;
        inObject->heldFrozenRotFrameCountUsed = false;
        }
    else if( inObject->curHeldAnim == moving &&
             inObject->lastHeldAnim != moving &&
             inObject->heldFrozenRotFrameCountUsed ) {
        // switching back to moving
        // resume from where frozen
        inObject->heldAnimationFrameCount = inObject->heldFrozenRotFrameCount;
        }
    else if( ( inObject->curHeldAnim == ground || 
               inObject->curHeldAnim == ground2 ) 
             &&
             inObject->lastHeldAnim == held ) {
        // keep old frozen frame count as we transition away
        // from held
        }
    }


static void addNewAnimPlayerOnly( LiveObject *inObject, AnimType inNewAnim ) {
    
    if( inObject->curAnim != inNewAnim || 
        inObject->futureAnimStack->size() > 0 ) {
        

        // if we're still in the middle of fading, finish the fade,
        // by pushing this animation on the stack...
        // ...but NOT if we're fading TO ground.
        // Cut that off, and start our next animation right away.
        if( inObject->lastAnimFade != 0 && 
            inObject->curAnim != ground &&
            inObject->curAnim != ground2 ) {
                        
            // don't double stack
            if( inObject->futureAnimStack->size() == 0 ||
                inObject->futureAnimStack->getElementDirect(
                    inObject->futureAnimStack->size() - 1 ) 
                != inNewAnim ) {
                
                // don't push another animation after ground
                // that animation will replace ground (no need to go to
                // ground between animations.... can just go straight 
                // to the next animation
                char foundNonGround = false;
                
                while( ! foundNonGround &&
                       inObject->futureAnimStack->size() > 0 ) {
                    
                    int prevAnim =
                        inObject->futureAnimStack->getElementDirect(
                            inObject->futureAnimStack->size() - 1 );
                    
                    if( prevAnim == ground || prevAnim == ground2 ) {
                        inObject->futureAnimStack->deleteElement(
                            inObject->futureAnimStack->size() - 1 );
                        }
                    else {
                        foundNonGround = true;
                        }
                    }
                
                inObject->futureAnimStack->push_back( inNewAnim );
                }
            }
        else {
            addNewAnimDirect( inObject, inNewAnim );
            }
        }
    }


static void addNewAnim( LiveObject *inObject, AnimType inNewAnim ) {
    addNewAnimPlayerOnly( inObject, inNewAnim );

    AnimType newHeldAnim = inNewAnim;
    
    if( inObject->holdingID < 0 ) {
        // holding a baby
        // never show baby's moving animation
        // baby always stuck in held animation when being held

        newHeldAnim = held;
        }
    else if( inObject->holdingID > 0 && 
             ( newHeldAnim == ground || newHeldAnim == ground2 || 
               newHeldAnim == doing || newHeldAnim == eating ) ) {
        // ground is used when person comes to a hault,
        // but for the held item, we should still show the held animation
        // same if person is starting a doing or eating animation
        newHeldAnim = held;
        }

    if( inObject->curHeldAnim != newHeldAnim ) {

        if( inObject->lastHeldAnimFade != 0 ) {
            
            // don't double stack
            if( inObject->futureHeldAnimStack->size() == 0 ||
                inObject->futureHeldAnimStack->getElementDirect(
                    inObject->futureHeldAnimStack->size() - 1 ) 
                != newHeldAnim ) {
                
                inObject->futureHeldAnimStack->push_back( newHeldAnim );
                }
            }
        else {
            addNewHeldAnimDirect( inObject, newHeldAnim );
            }
        }    
        
    }






// if user clicks to initiate an action while still moving, we
// queue it here
static char *nextActionMessageToSend = NULL;
static char nextActionEating = false;
static char nextActionDropping = false;


// block move until next PLAYER_UPDATE received after action sent
static char playerActionPending = false;
static int playerActionTargetX, playerActionTargetY;
static char playerActionTargetNotAdjacent = false;


static char waitingForPong = false;
static int lastPingSent = 0;
static int lastPongReceived = 0;


int ourID;

static int valleySpacing = 40;
static int valleyOffset = 0;


char lastCharUsed = 'A';

char mapPullMode = false;
int mapPullStartX = -10;
int mapPullEndX = 10;
int mapPullStartY = -10;
int mapPullEndY = 10;

int mapPullCurrentX;
int mapPullCurrentY;
char mapPullCurrentSaved = false;
char mapPullCurrentSent = false;
char mapPullModeFinalImage = false;

Image *mapPullTotalImage = NULL;
int numScreensWritten = 0;


static int apocalypseInProgress = false;
static double apocalypseDisplayProgress = 0;
static double apocalypseDisplaySeconds = 6;

static double remapPeakSeconds = 60;
static double remapDelaySeconds = 30;


static Image *expandToPowersOfTwoWhite( Image *inImage ) {
    
    int w = 1;
    int h = 1;
                    
    while( w < inImage->getWidth() ) {
        w *= 2;
        }
    while( h < inImage->getHeight() ) {
        h *= 2;
        }
    
    return inImage->expandImage( w, h, true );
    }



static void splitAndExpandSprites( const char *inTgaFileName, int inNumSprites,
                                   SpriteHandle *inDestArray ) {
    
    Image *full = readTGAFile( inTgaFileName );
    if( full != NULL ) {
        
        int spriteW = full->getWidth() / inNumSprites;
        
        int spriteH = full->getHeight();
        
        for( int i=0; i<inNumSprites; i++ ) {
            
            Image *part = full->getSubImage( i * spriteW, 0, 
                                          spriteW, spriteH );
            Image *partExpanded = expandToPowersOfTwoWhite( part );
            
            
            delete part;

            inDestArray[i] = fillSprite( partExpanded, false );
            
            delete partExpanded;
            }

        delete full;
        }

    }



void LivingLifePage::clearMap() {
    for( int i=0; i<mMapD *mMapD; i++ ) {
        // -1 represents unknown
        // 0 represents known empty
        mMap[i] = -1;
        mMapBiomes[i] = -1;
        mMapFloors[i] = -1;
        
        mMapAnimationFrameCount[i] = randSource.getRandomBoundedInt( 0, 10000 );
        mMapAnimationLastFrameCount[i] = 
            randSource.getRandomBoundedInt( 0, 10000 );
        
        mMapAnimationFrozenRotFrameCount[i] = 0;
        mMapAnimationFrozenRotFrameCountUsed[i] = false;
        
        mMapFloorAnimationFrameCount[i] = 
            randSource.getRandomBoundedInt( 0, 10000 );

        mMapCurAnimType[i] = ground;
        mMapLastAnimType[i] = ground;
        mMapLastAnimFade[i] = 0;

        mMapDropOffsets[i].x = 0;
        mMapDropOffsets[i].y = 0;
        mMapDropRot[i] = 0;
        mMapDropSounds[i] = blankSoundUsage;

        mMapMoveOffsets[i].x = 0;
        mMapMoveOffsets[i].y = 0;
        mMapMoveSpeeds[i] = 0;
        
        mMapTileFlips[i] = false;
        
        mMapPlayerPlacedFlags[i] = false;
        }
    }



LivingLifePage::LivingLifePage() 
        : mServerSocket( -1 ), 
          mForceRunTutorial( 0 ),
          mTutorialNumber( 0 ),
          mGlobalMessageShowing( false ),
          mGlobalMessageStartTime( 0 ),
          mFirstServerMessagesReceived( 0 ),
          mMapGlobalOffsetSet( false ),
          mMapD( MAP_D ),
          mMapOffsetX( 0 ),
          mMapOffsetY( 0 ),
          mEKeyEnabled( false ),
          mEKeyDown( false ),
          mGuiPanelSprite( loadSprite( "guiPanel.tga", false ) ),
          mGuiBloodSprite( loadSprite( "guiBlood.tga", false ) ),
          mNotePaperSprite( loadSprite( "notePaper.tga", false ) ),
          mFloorSplitSprite( loadSprite( "floorSplit.tga", false ) ),
          mCellBorderSprite( loadWhiteSprite( "cellBorder.tga" ) ),
          mCellFillSprite( loadWhiteSprite( "cellFill.tga" ) ),
          mHintArrowSprite( loadSprite( "hintArrow.tga" ) ),
          mHomeSlipSprite( loadSprite( "homeSlip.tga", false ) ),
          mHomeSlip2Sprite( loadSprite( "homeSlip2.tga", false ) ),
          mLastMouseOverID( 0 ),
          mCurMouseOverID( 0 ),
          mChalkBlotSprite( loadWhiteSprite( "chalkBlot.tga" ) ),
          mPathMarkSprite( loadWhiteSprite( "pathMark.tga" ) ),
          mSayField( handwritingFont, 0, 1000, 10, true, NULL,
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-,'?!/ " ),
          mDeathReason( NULL ),
          mShowHighlights( true ),
          mUsingSteam( false ),
          mZKeyDown( false ),
          mXKeyDown( false ),
          mObjectPicker( &objectPickable, +510, 90 ) {


    if( SettingsManager::getIntSetting( "useSteamUpdate", 0 ) ) {
        mUsingSteam = true;
        }


    const char *badgeSettingsNames[3] = { "badgeObjects",
                                          "badgeObjectsHalfX",
                                          "badgeObjectsFullX" };
    for( int i=0; i<3; i++ ) {
        SimpleVector<int> *badgeSetting = 
            SettingsManager::getIntSettingMulti( badgeSettingsNames[i] );
        
        mLeadershipBadges[i].push_back_other( badgeSetting );
        delete badgeSetting;
        }
    
    mFullXObjectID = SettingsManager::getIntSetting( "fullX", 0 );
    

    mHomeSlipSprites[0] = mHomeSlipSprite;
    mHomeSlipSprites[1] = mHomeSlip2Sprite;
    

    mForceGroundClick = false;
    
    mYumSlipSprites[0] = loadSprite( "yumSlip1.tga", false );
    mYumSlipSprites[1] = loadSprite( "yumSlip2.tga", false );
    mYumSlipSprites[2] = loadSprite( "yumSlip3.tga", false );
    mYumSlipSprites[3] = loadSprite( "yumSlip4.tga", false );

    
    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;
    mCurMouseOverCellFade = 0.0f;
    mCurMouseOverCellFadeRate = 0.04;
    mLastClickCell.x = -1;
    mLastClickCell.y = -1;

    // we're not showing a cursor on note paper, so arrow key behavior
    // is confusing.
    mSayField.setIgnoreArrowKeys( true );
    // drawn under world at (0,1000), don't allow click to focus
    mSayField.setIgnoreMouse( true );

    // allow ctrl-v to paste into chat from clipboard
    mSayField.usePasteShortcut( true );
    
    initLiveTriggers();

    for( int i=0; i<4; i++ ) {
        char *name = autoSprintf( "ground_t%d.tga", i );    
        mGroundOverlaySprite[i] = loadSprite( name, false );
        delete [] name;
        }
    

    mMapGlobalOffset.x = 0;
    mMapGlobalOffset.y = 0;

    emotDuration = SettingsManager::getFloatSetting( "emotDuration", 10 );
          
    hideGuiPanel = SettingsManager::getIntSetting( "hideGameUI", 0 );

    mHungerSound = loadSoundSprite( "otherSounds", "hunger.aiff" );
    mPulseHungerSound = false;
    

    if( mHungerSound != NULL ) {
        toggleVariance( mHungerSound, true );
        }


    mTutorialSound = loadSoundSprite( "otherSounds", "tutorialChime.aiff" );

    if( mTutorialSound != NULL ) {
        toggleVariance( mTutorialSound, true );
        }

    mCurseSound = loadSoundSprite( "otherSounds", "curseChime.aiff" );

    if( mCurseSound != NULL ) {
        toggleVariance( mCurseSound, true );
        }
    

    mHungerSlipSprites[0] = loadSprite( "fullSlip.tga", false );
    mHungerSlipSprites[1] = loadSprite( "hungrySlip.tga", false );
    mHungerSlipSprites[2] = loadSprite( "starvingSlip.tga", false );
    

    // not visible, drawn under world at 0, 0, and doesn't move with camera
    // still, we can use it to receive/process/filter typing events
    addComponent( &mSayField );
    
    mSayField.unfocus();
    
    
    mNotePaperHideOffset.x = -282;
    mNotePaperHideOffset.y = -420;


    mHomeSlipHideOffset[0].x = -41;
    mHomeSlipHideOffset[0].y = -360;

    mHomeSlipHideOffset[1].x =  30;
    mHomeSlipHideOffset[1].y = -360;

    mHomeSlipShowDelta[0] = 68;
    mHomeSlipShowDelta[1] = 68;
    


    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {    
        mYumSlipHideOffset[i].x = -600;
        mYumSlipHideOffset[i].y = -330;
        }
    
    mYumSlipHideOffset[2].x += 70;
    mYumSlipHideOffset[3].x += 80;

    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {    
        mYumSlipPosOffset[i] = mYumSlipHideOffset[i];
        mYumSlipPosTargetOffset[i] = mYumSlipHideOffset[i];
        }
    

    for( int i=0; i<3; i++ ) {    
        mHungerSlipShowOffsets[i].x = -558;
        mHungerSlipShowOffsets[i].y = -250;
    
        mHungerSlipHideOffsets[i].x = -558;
        mHungerSlipHideOffsets[i].y = -370;
        
        mHungerSlipWiggleTime[i] = 0;
        mHungerSlipWiggleAmp[i] = 0;
        mHungerSlipWiggleSpeed[i] = 0.05;
        }
    mHungerSlipShowOffsets[2].y += 20;
    mHungerSlipHideOffsets[2].y -= 20;

    mHungerSlipShowOffsets[2].y -= 50;
    mHungerSlipShowOffsets[1].y -= 30;
    mHungerSlipShowOffsets[0].y += 18;


    mHungerSlipWiggleAmp[1] = 0.5;
    mHungerSlipWiggleAmp[2] = 0.5;

    mHungerSlipWiggleSpeed[2] = 0.075;

    mStarvingSlipLastPos[0] = 0;
    mStarvingSlipLastPos[1] = 0;
    

    for( int i=0; i<3; i++ ) {    
        mHungerSlipPosOffset[i] = mHungerSlipHideOffsets[i];
        mHungerSlipPosTargetOffset[i] = mHungerSlipPosOffset[i];
        }
    
    mHungerSlipVisible = -1;

    
    
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        char *name = autoSprintf( "hintSheet%d.tga", i + 1 );    
        mHintSheetSprites[i] = loadSprite( name, false );
        delete [] name;
        
        mHintHideOffset[i].x = 900;
        mHintHideOffset[i].y = -370;
        
        mHintTargetOffset[i] = mHintHideOffset[i];
        mHintPosOffset[i] = mHintHideOffset[i];
        
        mHintExtraOffset[i].x = 0;
        mHintExtraOffset[i].y = 0;

        mHintMessage[i] = NULL;
        mHintMessageIndex[i] = 0;
        
        mNumTotalHints[i] = 0;
        mHintSheetXTweak[i] = 0;
        }
    // manual tweaks
    mHintSheetXTweak[1] = -6;
    mHintSheetXTweak[2] = -4;

    mLiveHintSheetIndex = -1;

    mForceHintRefresh = false;
    mCurrentHintObjectID = 0;
    mCurrentHintIndex = 0;
    
    mNextHintObjectID = 0;
    mNextHintIndex = 0;
    
    mLastHintSortedSourceID = 0;
    
    for( int i=0; i<2; i++ ) {
        mCurrentHintTargetObject[i] = 0;
        mCurrentHintTargetPointerBounce[i] = 0;
        mCurrentHintTargetPointerFade[i] = 0;
        
        mLastHintTargetPos[i].x = 0;
        mLastHintTargetPos[i].y = 0;
        }
    

    int maxObjectID = getMaxObjectID();
    
    mHintBookmarks = new int[ maxObjectID + 1 ];

    for( int i=0; i<=maxObjectID; i++ ) {
        mHintBookmarks[i] = 0;
        }
    
    mHintFilterString = NULL;
    mHintFilterStringNoMatch = false;
    
    mLastHintFilterString = NULL;
    mPendingFilterString = NULL;
    
    

    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        
        mTutorialHideOffset[i].x = -914;
        mTutorialFlips[i] = false;
        
        if( i % 2 == 1 ) {
            // odd on right side of screen
            mTutorialHideOffset[i].x = 914;
            mTutorialFlips[i] = true;
            }
        
        mTutorialHideOffset[i].y = 430;
        
        mTutorialTargetOffset[i] = mTutorialHideOffset[i];
        mTutorialPosOffset[i] = mTutorialHideOffset[i];

        mTutorialExtraOffset[i].x = 0;
        mTutorialExtraOffset[i].y = 0;
        
        mTutorialMessage[i] = "";


        mCravingHideOffset[i].x = -932;
        
        mCravingHideOffset[i].y = -370;
        
        mCravingTargetOffset[i] = mCravingHideOffset[i];
        mCravingPosOffset[i] = mCravingHideOffset[i];

        mCravingExtraOffset[i].x = 0;
        mCravingExtraOffset[i].y = 0;
        
        mCravingMessage[i] = NULL;        
        }
    
    mLiveTutorialSheetIndex = -1;
    mLiveTutorialTriggerNumber = -1;
    
    mLiveCravingSheetIndex = -1;


    mMap = new int[ mMapD * mMapD ];
    mMapBiomes = new int[ mMapD * mMapD ];
    mMapFloors = new int[ mMapD * mMapD ];
    
    mMapCellDrawnFlags = new char[ mMapD * mMapD ];

    mMapContainedStacks = new SimpleVector<int>[ mMapD * mMapD ];
    mMapSubContainedStacks = 
        new SimpleVector< SimpleVector<int> >[ mMapD * mMapD ];
    
    mMapAnimationFrameCount =  new double[ mMapD * mMapD ];
    mMapAnimationLastFrameCount =  new double[ mMapD * mMapD ];
    mMapAnimationFrozenRotFrameCount =  new double[ mMapD * mMapD ];

    mMapAnimationFrozenRotFrameCountUsed =  new char[ mMapD * mMapD ];
    
    mMapFloorAnimationFrameCount =  new int[ mMapD * mMapD ];

    mMapCurAnimType =  new AnimType[ mMapD * mMapD ];
    mMapLastAnimType =  new AnimType[ mMapD * mMapD ];
    mMapLastAnimFade =  new double[ mMapD * mMapD ];
    
    mMapDropOffsets = new doublePair[ mMapD * mMapD ];
    mMapDropRot = new double[ mMapD * mMapD ];
    mMapDropSounds = new SoundUsage[ mMapD * mMapD ];

    mMapMoveOffsets = new doublePair[ mMapD * mMapD ];
    mMapMoveSpeeds = new double[ mMapD * mMapD ];

    mMapTileFlips = new char[ mMapD * mMapD ];
    
    mMapPlayerPlacedFlags = new char[ mMapD * mMapD ];
    

    clearMap();


    splitAndExpandSprites( "hungerBoxes.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxSprites );
    splitAndExpandSprites( "hungerBoxFills.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxFillSprites );

    splitAndExpandSprites( "hungerBoxesErased.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxErasedSprites );
    splitAndExpandSprites( "hungerBoxFillsErased.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxFillErasedSprites );

    splitAndExpandSprites( "tempArrows.tga", NUM_TEMP_ARROWS, 
                           mTempArrowSprites );
    splitAndExpandSprites( "tempArrowsErased.tga", NUM_TEMP_ARROWS, 
                           mTempArrowErasedSprites );

    splitAndExpandSprites( "hungerDashes.tga", NUM_HUNGER_DASHES, 
                           mHungerDashSprites );
    splitAndExpandSprites( "hungerDashesErased.tga", NUM_HUNGER_DASHES, 
                           mHungerDashErasedSprites );

    splitAndExpandSprites( "hungerBars.tga", NUM_HUNGER_DASHES, 
                           mHungerBarSprites );
    splitAndExpandSprites( "hungerBarsErased.tga", NUM_HUNGER_DASHES, 
                           mHungerBarErasedSprites );

    
    splitAndExpandSprites( "homeArrows.tga", NUM_HOME_ARROWS, 
                           mHomeArrowSprites );
    splitAndExpandSprites( "homeArrowsErased.tga", NUM_HOME_ARROWS, 
                           mHomeArrowErasedSprites );

    
    SimpleVector<int> *culvertStoneIDs = 
        SettingsManager::getIntSettingMulti( "culvertStoneSprites" );
    
    for( int i=0; i<culvertStoneIDs->size(); i++ ) {
        int id = culvertStoneIDs->getElementDirect( i );
        
        if( getSprite( id ) != NULL ) {
            mCulvertStoneSpriteIDs.push_back( id );
            }
        }
    delete culvertStoneIDs;


    mCurrentArrowI = 0;
    mCurrentArrowHeat = -1;
    mCurrentDes = NULL;
    mCurrentLastAteString = NULL;

    mShowHighlights = 
        SettingsManager::getIntSetting( "showMouseOverHighlights", 1 );

    mEKeyEnabled = 
        SettingsManager::getIntSetting( "eKeyForRightClick", 0 );


    if( teaserVideo ) {
        mTeaserArrowLongSprite = loadWhiteSprite( "teaserArrowLong.tga" );
        mTeaserArrowMedSprite = loadWhiteSprite( "teaserArrowMed.tga" );
        mTeaserArrowShortSprite = loadWhiteSprite( "teaserArrowShort.tga" );
        mTeaserArrowVeryShortSprite = 
            loadWhiteSprite( "teaserArrowVeryShort.tga" );
        
        mLineSegmentSprite = loadWhiteSprite( "lineSegment.tga" );
        }
          
    
    int tutorialDone = SettingsManager::getIntSetting( "tutorialDone", 0 );
    
    if( ! tutorialDone ) {
        mTutorialNumber = 1;
        }
    }




void LivingLifePage::runTutorial( int inNumber ) {
    mForceRunTutorial = inNumber;
    }



void LivingLifePage::clearLiveObjects() {
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *nextObject =
            gameObjects.getElement( i );
        
        nextObject->pendingReceivedMessages.deallocateStringElements();

        if( nextObject->containedIDs != NULL ) {
            delete [] nextObject->containedIDs;
            }

        if( nextObject->subContainedIDs != NULL ) {
            delete [] nextObject->subContainedIDs;
            }
        
        if( nextObject->pathToDest != NULL ) {
            delete [] nextObject->pathToDest;
            }

        if( nextObject->currentSpeech != NULL ) {
            delete [] nextObject->currentSpeech;
            }

        if( nextObject->relationName != NULL ) {
            delete [] nextObject->relationName;
            }

        if( nextObject->curseName != NULL ) {
            delete [] nextObject->curseName;
            }
        
        if( nextObject->name != NULL ) {
            delete [] nextObject->name;
            }
        
        if( nextObject->leadershipNameTag != NULL ) {
            delete [] nextObject->leadershipNameTag;
            }

        delete nextObject->futureAnimStack;
        delete nextObject->futureHeldAnimStack;
        }
    
    gameObjects.deleteAll();
    }




LivingLifePage::~LivingLifePage() {
    printf( "Total received = %d bytes (+%d in headers), "
            "total sent = %d bytes (+%d in headers)\n",
            numServerBytesRead, overheadServerBytesRead,
            numServerBytesSent, overheadServerBytesSent );
    
    mBadBiomeNames.deallocateStringElements();

    mGlobalMessagesToDestroy.deallocateStringElements();

    freeLiveTriggers();

    readyPendingReceivedMessages.deallocateStringElements();

    serverFrameMessages.deallocateStringElements();
    
    if( pendingMapChunkMessage != NULL ) {
        delete [] pendingMapChunkMessage;
        pendingMapChunkMessage = NULL;
        }
    

    clearLiveObjects();

    mOldDesStrings.deallocateStringElements();
    if( mCurrentDes != NULL ) {
        delete [] mCurrentDes;
        }

    mOldLastAteStrings.deallocateStringElements();
    if( mCurrentLastAteString != NULL ) {
        delete [] mCurrentLastAteString;
        }

    mSentChatPhrases.deallocateStringElements();
    
    if( mServerSocket != -1 ) {
        closeSocket( mServerSocket );
        }
    
    for( int j=0; j<2; j++ ) {
        mPreviousHomeDistStrings[j].deallocateStringElements();
        mPreviousHomeDistFades[j].deleteAll();
        }
    
    
    delete [] mMapAnimationFrameCount;
    delete [] mMapAnimationLastFrameCount;
    delete [] mMapAnimationFrozenRotFrameCount;
    
    delete [] mMapAnimationFrozenRotFrameCountUsed;

    delete [] mMapFloorAnimationFrameCount;

    delete [] mMapCurAnimType;
    delete [] mMapLastAnimType;
    delete [] mMapLastAnimFade;
    
    delete [] mMapDropOffsets;
    delete [] mMapDropRot;
    delete [] mMapDropSounds;

    delete [] mMapMoveOffsets;
    delete [] mMapMoveSpeeds;

    delete [] mMapTileFlips;
    
    delete [] mMapContainedStacks;
    delete [] mMapSubContainedStacks;
    
    delete [] mMap;
    delete [] mMapBiomes;
    delete [] mMapFloors;

    delete [] mMapCellDrawnFlags;

    delete [] mMapPlayerPlacedFlags;

    if( nextActionMessageToSend != NULL ) {
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }

    if( lastMessageSentToServer != NULL ) {
        delete [] lastMessageSentToServer;
        lastMessageSentToServer = NULL;
        }
    
    if( mHungerSound != NULL ) {    
        freeSoundSprite( mHungerSound );
        }

    if( mTutorialSound != NULL ) {    
        freeSoundSprite( mTutorialSound );
        }

    if( mCurseSound != NULL ) {    
        freeSoundSprite( mCurseSound );
        }
    
    for( int i=0; i<3; i++ ) {
        freeSprite( mHungerSlipSprites[i] );
        }

    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {
        freeSprite( mYumSlipSprites[i] );
        }

    freeSprite( mGuiPanelSprite );
    freeSprite( mGuiBloodSprite );
    
    freeSprite( mFloorSplitSprite );
    
    freeSprite( mCellBorderSprite );
    freeSprite( mCellFillSprite );
    freeSprite( mHintArrowSprite );

    freeSprite( mNotePaperSprite );
    freeSprite( mChalkBlotSprite );
    freeSprite( mPathMarkSprite );
    freeSprite( mHomeSlipSprite );
    freeSprite( mHomeSlip2Sprite );
    
    if( teaserVideo ) {
        freeSprite( mTeaserArrowLongSprite );
        freeSprite( mTeaserArrowMedSprite );
        freeSprite( mTeaserArrowShortSprite );
        freeSprite( mTeaserArrowVeryShortSprite );
        freeSprite( mLineSegmentSprite );
        }
    
    for( int i=0; i<4; i++ ) {
        freeSprite( mGroundOverlaySprite[i] );
        }
    

    for( int i=0; i<NUM_HUNGER_BOX_SPRITES; i++ ) {
        freeSprite( mHungerBoxSprites[i] );
        freeSprite( mHungerBoxFillSprites[i] );
        
        freeSprite( mHungerBoxErasedSprites[i] );
        freeSprite( mHungerBoxFillErasedSprites[i] );
        }

    for( int i=0; i<NUM_TEMP_ARROWS; i++ ) {
        freeSprite( mTempArrowSprites[i] );
        freeSprite( mTempArrowErasedSprites[i] );
        }

    for( int i=0; i<NUM_HUNGER_DASHES; i++ ) {
        freeSprite( mHungerDashSprites[i] );
        freeSprite( mHungerDashErasedSprites[i] );
        freeSprite( mHungerBarSprites[i] );
        freeSprite( mHungerBarErasedSprites[i] );
        }
    
    for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
        freeSprite( mHomeArrowSprites[i] );
        freeSprite( mHomeArrowErasedSprites[i] );
        }



    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        freeSprite( mHintSheetSprites[i] );
        
        if( mHintMessage[i] != NULL ) {
            delete [] mHintMessage[i];
            }
        if( mCravingMessage[i] != NULL ) {
            delete [] mCravingMessage[i];
            }
        }
    
    delete [] mHintBookmarks;

    if( mHintFilterString != NULL ) {
        delete [] mHintFilterString;
        mHintFilterString = NULL;
        }

    if( mLastHintFilterString != NULL ) {
        delete [] mLastHintFilterString;
        mLastHintFilterString = NULL;
        }

    if( mPendingFilterString != NULL ) {
        delete [] mPendingFilterString;
        mPendingFilterString = NULL;
        }

    if( mDeathReason != NULL ) {
        delete [] mDeathReason;
        }

    for( int i=0; i<mGraveInfo.size(); i++ ) {
        delete [] mGraveInfo.getElement(i)->relationName;
        }
    mGraveInfo.deleteAll();

    clearOwnerInfo();

    clearLocationSpeech();

    if( photoSig != NULL ) {
        delete [] photoSig;
        photoSig = NULL;
        }

    for( int i=0; i<homelands.size(); i++ ) {
        delete [] homelands.getElementDirect( i ).familyName;
        }
    homelands.deleteAll();
    }




void LivingLifePage::clearOwnerInfo() {
    
    for( int i=0; i<mOwnerInfo.size(); i++ ) {
        delete mOwnerInfo.getElement( i )->ownerList;
        }
    mOwnerInfo.deleteAll();
    }



char LivingLifePage::isMapBeingPulled() {
    return mapPullMode;
    }



void LivingLifePage::adjustAllFrameCounts( double inOldFrameRateFactor,
                                           double inNewFrameRateFactor ) {
    int numMapCells = mMapD * mMapD;
    
    for( int i=0; i<numMapCells; i++ ) {
        
        double timeVal = inOldFrameRateFactor * mMapAnimationFrameCount[ i ];
        mMapAnimationFrameCount[i] = lrint( timeVal / inNewFrameRateFactor );
        
        timeVal = inOldFrameRateFactor * mMapAnimationFrozenRotFrameCount[ i ];
        mMapAnimationFrozenRotFrameCount[i] = 
            lrint( timeVal / inNewFrameRateFactor );

        timeVal = inOldFrameRateFactor * mMapAnimationLastFrameCount[ i ];
        mMapAnimationLastFrameCount[i] = 
            lrint( timeVal / inNewFrameRateFactor );
        
        timeVal = 
            inOldFrameRateFactor * mMapFloorAnimationFrameCount[ i ];
        mMapFloorAnimationFrameCount[i] = 
            lrint( timeVal / inNewFrameRateFactor );
        }

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );

        double timeVal = inOldFrameRateFactor * o->animationFrameCount;
        o->animationFrameCount = timeVal / inNewFrameRateFactor;
        
        timeVal = inOldFrameRateFactor * o->heldAnimationFrameCount;
        o->heldAnimationFrameCount = timeVal / inNewFrameRateFactor;
        
        timeVal = inOldFrameRateFactor * o->lastAnimationFrameCount;
        o->lastAnimationFrameCount = timeVal / inNewFrameRateFactor;

        timeVal = inOldFrameRateFactor * o->lastHeldAnimationFrameCount;
        o->lastHeldAnimationFrameCount = timeVal / inNewFrameRateFactor;

        timeVal = inOldFrameRateFactor * o->frozenRotFrameCount;
        o->frozenRotFrameCount = timeVal / inNewFrameRateFactor;

        timeVal = inOldFrameRateFactor * o->heldFrozenRotFrameCount;
        o->heldFrozenRotFrameCount = timeVal / inNewFrameRateFactor;
        }
    }



LiveObject *LivingLifePage::getOurLiveObject() {
    
    return getLiveObject( ourID );
    }




LiveObject *LivingLifePage::getLiveObject( int inID ) {
    
    LiveObject *obj = NULL;

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == inID ) {
            obj = o;
            break;
            }
        }
    return obj;
    }



SimpleVector<char*> *splitLines( const char *inString,
                                 double inMaxWidth ) {
    
    // break into lines
    SimpleVector<char *> *tokens = 
        tokenizeString( inString );
    
    
    // collect all lines before drawing them
    SimpleVector<char *> *lines = new SimpleVector<char*>();
    
    
    if( tokens->size() > 0 ) {
        // start with firt token
        char *firstToken = tokens->getElementDirect( 0 );
        
        lines->push_back( firstToken );
        
        tokens->deleteElement( 0 );
        }
    
    
    while( tokens->size() > 0 ) {
        char *nextToken = tokens->getElementDirect( 0 );
        
        char *currentLine = lines->getElementDirect( lines->size() - 1 );
         
        char *expandedLine = autoSprintf( "%s %s", currentLine, nextToken );
         
        if( handwritingFont->measureString( expandedLine ) <= inMaxWidth ) {
            // replace current line
            delete [] currentLine;
            lines->deleteElement(  lines->size() - 1 );
             
            lines->push_back( expandedLine );
            }
        else {
            // expanded is too long
            // put token at start of next line
            delete [] expandedLine;
             
            lines->push_back( stringDuplicate( nextToken ) );
            }
         

        delete [] nextToken;
        tokens->deleteElement( 0 );
        }
    
    delete tokens;
    
    return lines;
    }


static Image *speechColorImage = NULL;
static Image *speechMaskImage = NULL;



// forces uppercase
void LivingLifePage::drawChalkBackgroundString( doublePair inPos, 
                                                const char *inString,
                                                double inFade,
                                                double inMaxWidth,
                                                LiveObject *inSpeaker,
                                                int inForceMinChalkBlots,
                                                FloatColor *inForceBlotColor,
                                                FloatColor *inForceTextColor ) {
    
    char *stringUpper = stringToUpperCase( inString );

    SimpleVector<char*> *lines = splitLines( inString, inMaxWidth );
    
    delete [] stringUpper;

    
    if( lines->size() == 0 ) {
        delete lines;
        return;
        }

    double lineSpacing = handwritingFont->getFontHeight() / 2 + 5;
    
    double firstLineY =  inPos.y + ( lines->size() - 1 ) * lineSpacing;
    
    if( firstLineY > lastScreenViewCenter.y + 330 ) {
        // off top of screen
        firstLineY = lastScreenViewCenter.y + 330;
        }
    
    if( inPos.y < lastScreenViewCenter.y - 280 ) {
        // off bottom of screen
        double lastLineY = lastScreenViewCenter.y - 280;
        
        firstLineY = lastLineY + ( lines->size() - 1 ) * lineSpacing;
        }
    

    double widestLine = 0;
    for( int i=0; i<lines->size(); i++ ) {
        char *line = lines->getElementDirect( i );

        double length = handwritingFont->measureString( line );
        if( length > widestLine ) {
            widestLine = length;
            }
        }
    
    if( inPos.x < lastScreenViewCenter.x - 615 ) {
        inPos.x = lastScreenViewCenter.x - 615;
        }
    if( inPos.x + widestLine > lastScreenViewCenter.x + 610 ) {
        inPos.x = lastScreenViewCenter.x + 610 - widestLine;
        }
    
    
    
    if( inForceBlotColor != NULL ) {
        setDrawColor( *inForceBlotColor );
        setDrawFade( inFade );
        }
    else if( inSpeaker != NULL && inSpeaker->dying ) {
        if( inSpeaker->sick ) {
            // sick-ish yellow
            setDrawColor( 0.874510, 0.658824, 0.168627, inFade );
            }
        else {
            // wounded, blood red
            setDrawColor( .65, 0, 0, inFade );
            }
        }
    else if( inSpeaker != NULL && inSpeaker->curseLevel > 0 ) {
        setDrawColor( 0, 0, 0, inFade );
        }
    else {
        setDrawColor( 1, 1, 1, inFade );
        }



    char maskOnly = false;
    char colorOnly = false;
    
    if( savingSpeech && savingSpeechColor && inFade == 1.0 ) {
        drawSquare( inPos, 1024 );
        colorOnly = true;
        }
    else if( savingSpeech && savingSpeechMask && inFade == 1.0 ) {
        setDrawColor( 0, 0, 0, 1.0 );
        drawSquare( inPos, 1024 );
        setDrawColor( 1, 1, 1, 1 );
        maskOnly = true;
        }



    // with a fixed seed
    JenkinsRandomSource blotRandSource( 0 );
        
    for( int i=0; i<lines->size(); i++ ) {
        char *line = lines->getElementDirect( i );
        

        double length = handwritingFont->measureString( line );
            
        int numBlots = lrint( 0.25 + length / 20 ) + 1;
        
        if( inForceMinChalkBlots != -1 && numBlots < inForceMinChalkBlots ) {
            numBlots = inForceMinChalkBlots;
            }
    
        doublePair blotSpacing = { 20, 0 };
    
        doublePair firstBlot = 
            { inPos.x, firstLineY - i * lineSpacing};

        
        if( numBlots == 1 ) {
            // center first and only blot on line of text 
            // (probably one char of text)
            firstBlot.x += length / 2;
            }
        else {
            // stretch blots so they just perfectly cover this line
            blotSpacing.x = length / ( numBlots - 1 );
            }

        
        for( int b=0; b<numBlots; b++ ) {
            doublePair blotPos = add( firstBlot, mult( blotSpacing, b ) );
        
            double rot = blotRandSource.getRandomDouble();
            drawSprite( mChalkBlotSprite, blotPos, 1.0, rot );
            drawSprite( mChalkBlotSprite, blotPos, 1.0, rot );
            
            // double hit vertically
            blotPos.y += 5;
            rot = blotRandSource.getRandomDouble();
            drawSprite( mChalkBlotSprite, blotPos, 1.0, rot );
            
            blotPos.y -= 10;
            rot = blotRandSource.getRandomDouble();
            drawSprite( mChalkBlotSprite, blotPos, 1.0, rot );
            }
        }
    
    
    if( inForceTextColor != NULL ) {
        setDrawColor( *inForceTextColor );
        setDrawFade( inFade );
        }
    else if( inSpeaker != NULL && inSpeaker->dying && ! inSpeaker->sick ) {
        setDrawColor( 1, 1, 1, inFade );
        }
    else if( inSpeaker != NULL && inSpeaker->curseLevel > 0 ) {
        setDrawColor( 1, 1, 1, inFade );
        if( inSpeaker->speechIsSuccessfulCurse ) {
            setDrawColor( 0.875, 0, 0.875, inFade );
            }
        }
    else if( inSpeaker != NULL && inSpeaker->speechIsSuccessfulCurse ) {
        setDrawColor( 0.5, 0, 0.5, inFade );
        }
    else {
        setDrawColor( 0, 0, 0, inFade );
        }
    

    if( maskOnly ) {
        // font should add to opacity of mask too
        setDrawColor( 1, 1, 1, 1 );
        }

    
    for( int i=0; i<lines->size(); i++ ) {
        char *line = lines->getElementDirect( i );
        
        doublePair lineStart = 
            { inPos.x, firstLineY - i * lineSpacing};
        
        handwritingFont->drawString( line, lineStart, alignLeft );
        delete [] line;
        }

    delete lines;


    if( colorOnly ) {
        saveScreenShot( "speechColor", &speechColorImage );
        savingSpeechColor = false;
        savingSpeechMask = true;
        }
    else if( maskOnly ) {
        saveScreenShot( "speechMask", &speechMaskImage );
        savingSpeechMask = false;
        savingSpeech = false;
        }
    
    if( speechColorImage != NULL && speechMaskImage != NULL ) {
        // both screen shot requests are done

        Image *subColor = speechColorImage->getSubImage( 0, 0, 1280, 500 );
        Image *subMask = speechMaskImage->getSubImage( 0, 0, 1280, 500 );
        
        int w = subColor->getWidth();
        int h = subColor->getHeight();
        
        Image blend( w, h, 4, true );
        blend.paste( subColor );
        double *alpha = blend.getChannel( 3 );
        
        memcpy( alpha, subMask->getChannel( 0 ),
                w * h * sizeof( double ) );
        
        int minX = w -1;
        int maxX = 0;
        int minY = h -1;
        int maxY = 0;
        
        for( int y=0; y<h; y++ ) {
            for( int x=0; x<w; x++ ) {
                if( alpha[ y * w + x ] > 0 ) {
                    
                    if( x < minX ) {
                        minX = x;
                        }
                    if( x > maxX ) {
                        maxX = x;
                        }

                    if( y < minY ) {
                        minY = y;
                        }
                    if( y > maxY ) {
                        maxY = y;
                        }
                    }
                }
            }
        
        // expand 1 pixel to be safe
        if( minX > 0 ) {
            minX --;
            }
        if( minY > 0 ) {
            minY --;
            }
        if( maxX < w - 1 ) {
            maxX ++;
            }
        if( maxY < h - 1 ) {
            maxY ++;
            }
        

        Image *trimmed = blend.getSubImage( minX, minY,
                                            maxX - minX,
                                            maxY - minY );
                
        File screenShots( NULL, "screenShots" );
        
        char *fileName = autoSprintf( "speechBlend%04d.tga", 
                                      savingSpeechNumber );
        savingSpeechNumber++;
        
        File *tgaFile = screenShots.getChildFile( fileName );
        
        delete [] fileName;

        char *tgaPath = tgaFile->getFullFileName();

        delete tgaFile;

        writeTGAFile( tgaPath, trimmed );
        
        delete [] tgaPath;
        
        delete trimmed;
        

        delete subColor;
        delete subMask;


        delete speechColorImage;
        speechColorImage = NULL;
        delete speechMaskImage;
        speechMaskImage = NULL;
        }
    }




typedef struct OffScreenSound {
        doublePair pos;

        double fade;
        // wall clock time when should start fading
        double fadeETATime;

        char red;
        
        char specialChar;
        
        int sourcePlayerID;
    } OffScreenSound;

SimpleVector<OffScreenSound> offScreenSounds;
    



static void addOffScreenSound( int inSourcePlayerID,
                               double inPosX, double inPosY,
                               char *inDescription,
                               double inFadeSec = 4 ) {

    char red = false;
    char specialChar = 0;
    
    char *stringPos = strstr( inDescription, "offScreenSound" );
    
    if( stringPos != NULL ) {
        stringPos = &( stringPos[ strlen( "offScreenSound" ) ] );
        
        if( strstr( stringPos, "_red" ) == stringPos ) {
            // _red flag next
            red = true;
            }
        else {
            char *underscorePos = strstr( stringPos, "_" );
            
            if( underscorePos != NULL && strlen( underscorePos ) >= 2 ) {
                specialChar = underscorePos[1];
                }
            }
        }
    
    double fadeETATime = game_getCurrentTime() + inFadeSec;
    
    doublePair pos = { inPosX, inPosY };
    
    OffScreenSound s = { pos, 1.0, fadeETATime, red, 
                         specialChar, inSourcePlayerID };
    
    offScreenSounds.push_back( s );
    }



void LivingLifePage::drawOffScreenSounds() {
    
    if( offScreenSounds.size() == 0 ) {
        return;
        }
    
    double xRadius = viewWidth / 2 - 32;
    double yRadius = viewHeight / 2 - 32;
    
    FloatColor red = { 0.65, 0, 0, 1 };
    FloatColor white = { 1, 1, 1, 1 };
    FloatColor black = { 0, 0, 0, 1 };
    

    double curTime = game_getCurrentTime();
    
    for( int i=0; i<offScreenSounds.size(); i++ ) {
        OffScreenSound *s = offScreenSounds.getElement( i );
        
        if( s->fadeETATime <= curTime ) {
            s->fade -= 0.05 * frameRateFactor;

            if( s->fade <= 0 ) {
                offScreenSounds.deleteElement( i );
                i--;
                continue;
                }
            }

        
        if( fabs( s->pos.x - lastScreenViewCenter.x ) > xRadius
            ||
            fabs( s->pos.y - lastScreenViewCenter.y ) > yRadius ) {
            
            // off screen
            
            // relative vector
            doublePair v = sub( s->pos, lastScreenViewCenter );
            
            doublePair normV = normalize( v );
            
            // first extend in x dir to edge
            double xScale = fabs( xRadius / normV.x );
            
            doublePair edgeV = mult( normV, xScale );
            

            if( fabs( edgeV.y ) > yRadius ) {
                // off top/bottom
                
                // extend in y dir instead
                double yScale = fabs( yRadius / normV.y );
            
                edgeV = mult( normV, yScale );
                }
            
            if( edgeV.y < -270 ) {
                edgeV.y = -270;
                }
            

            doublePair drawPos = add( edgeV, lastScreenViewCenter );
            
            FloatColor *textColor = &black;
            FloatColor *bgColor = &white;
            
            if( s->red ) {
                textColor = &white;
                bgColor = &red;
                }
            
            const char *stringToDraw = "!";
            
            if( s->sourcePlayerID != -1 ) {
                // are they in the posse chasing us?
                LiveObject *o = getGameObject( s->sourcePlayerID );
                
                if( o != NULL && o->chasingUs ) {
                    stringToDraw = "! !";
                    }
                }
            
            char *strToDelete = NULL;
            if( s->specialChar > 0 ) {
                stringToDraw = autoSprintf( "%c", s->specialChar );
                strToDelete = (char*)stringToDraw;
                }
            

            drawChalkBackgroundString( drawPos,
                                       stringToDraw,
                                       s->fade,
                                       100,
                                       NULL,
                                       -1,
                                       bgColor, textColor );
            
            if( strToDelete != NULL ) {
                delete [] strToDelete;
                }
            }    
        }
    }






void LivingLifePage::handleAnimSound( int inSourcePlayerID, 
                                      int inObjectID, double inAge, 
                                      AnimType inType,
                                      int inOldFrameCount, int inNewFrameCount,
                                      double inPosX, double inPosY ) {    
    
            
    double oldTimeVal = frameRateFactor * inOldFrameCount / 60.0;
            
    double newTimeVal = frameRateFactor * inNewFrameCount / 60.0;
                
    if( inType == ground2 ) {
        inType = ground;
        }


    AnimationRecord *anim = getAnimation( inObjectID, inType );
    if( anim != NULL ) {
                    
        for( int s=0; s<anim->numSounds; s++ ) {
            
            if( anim->soundAnim[s].sound.numSubSounds == 0 ) {
                continue;
                }
            
            
            if( ( anim->soundAnim[s].ageStart != -1 &&
                  inAge < anim->soundAnim[s].ageStart )
                ||
                ( anim->soundAnim[s].ageEnd != -1 &&
                  inAge >= anim->soundAnim[s].ageEnd ) ) {
                
                continue;
                }
            
            
            double hz = anim->soundAnim[s].repeatPerSec;
                        
            double phase = anim->soundAnim[s].repeatPhase;
            
            if( hz != 0 ) {
                double period = 1 / hz;
                
                double startOffsetSec = phase * period;
                
                int oldPeriods = 
                    lrint( 
                        floor( ( oldTimeVal - startOffsetSec ) / 
                               period ) );
                
                int newPeriods = 
                    lrint( 
                        floor( ( newTimeVal - startOffsetSec ) / 
                               period ) );
                
                if( newPeriods > oldPeriods ) {
                    SoundUsage u = anim->soundAnim[s].sound;
                    
                    
                    if( anim->soundAnim[s].footstep ) {
                        
                        // check if we're on a floor

                        int x = lrint( inPosX );
                        int y = lrint( inPosY );

                        int i = getMapIndex( x, y );
                        
                        if( i != -1 && mMapFloors[i] > 0 ) {
                            
                            ObjectRecord *f = getObject( mMapFloors[i] );
                            
                            if( f->usingSound.numSubSounds > 0 ) {
                                u = f->usingSound;
                                }
                            }
                        }
                    
                    
                    
                    playSound( u,
                               getVectorFromCamera( inPosX, inPosY ) );

                    char *des = getObject( inObjectID )->description;
                    
                    if( inSourcePlayerID != ourID &&
                        strstr( des, "offScreenSound" ) != NULL ) {
                        // this object has offscreen-visible sounds
                        // AND its animation has sounds
                        // renew offscreen sound for each new sound played
                        
                        // these have very short fade
                        // so that we don't have a bunch of overlap
                        addOffScreenSound(
                            inSourcePlayerID,
                            inPosX *
                            CELL_D, 
                            inPosY *
                            CELL_D,
                            des,
                            0.5 );
                        }
                    }
                }
            }
        }
    }




void LivingLifePage::drawMapCell( int inMapI, 
                                  int inScreenX, int inScreenY,
                                  char inHighlightOnly,
                                  char inNoTimeEffects ) {
            
    int oID = mMap[ inMapI ];

    int objectHeight = 0;
    
    if( oID > 0 ) {
        
        objectHeight = getObjectHeight( oID );
        
        double oldFrameCount = mMapAnimationFrameCount[ inMapI ];

        if( !mapPullMode && !inHighlightOnly && !inNoTimeEffects ) {
            
            if( mMapCurAnimType[ inMapI ] == moving ) {
                double animSpeed = 1.0;
                ObjectRecord *movingObj = getObject( oID );
            
                if( movingObj->speedMult < 1.0 ) {
                    // only slow anims down don't speed them up
                    animSpeed *= movingObj->speedMult;
                    }

                mMapAnimationFrameCount[ inMapI ] += animSpeed;
                mMapAnimationLastFrameCount[ inMapI ] += animSpeed;
                mMapAnimationFrozenRotFrameCount[ inMapI ] += animSpeed;
                mMapAnimationFrozenRotFrameCountUsed[ inMapI ] = false;
                }
            else {
                mMapAnimationFrameCount[ inMapI ] ++;
                mMapAnimationLastFrameCount[ inMapI ] ++;
                }

            
            if( mMapLastAnimFade[ inMapI ] > 0 ) {
                mMapLastAnimFade[ inMapI ] -= 0.05 * frameRateFactor;
                if( mMapLastAnimFade[ inMapI ] < 0 ) {
                    mMapLastAnimFade[ inMapI ] = 0;
                    
                    AnimType newType;

                    if( mMapMoveSpeeds[ inMapI ] == 0 ) {
                        newType = ground;
                        }
                    else if( mMapMoveSpeeds[ inMapI ] > 0 ) {
                        // transition to moving now
                        newType = moving;
                        }

                    if( mMapCurAnimType[ inMapI ] != newType ) {
                        
                        mMapLastAnimType[ inMapI ] = mMapCurAnimType[ inMapI ];
                        mMapCurAnimType[ inMapI ] = newType;
                        mMapLastAnimFade[ inMapI ] = 1;
                        
                        mMapAnimationLastFrameCount[ inMapI ] =
                            mMapAnimationFrameCount[ inMapI ];
                        
                        
                        if( newType == moving &&
                            mMapAnimationFrozenRotFrameCountUsed[ inMapI ] ) {
                            mMapAnimationFrameCount[ inMapI ] = 
                                mMapAnimationFrozenRotFrameCount[ inMapI ];
                            }
                        }
                    
                    }
                }
            }

        doublePair pos = { (double)inScreenX, (double)inScreenY };
        double rot = 0;
        
        fixWatchedObjectDrawPos( pos );

        if( mMapDropOffsets[ inMapI ].x != 0 ||
            mMapDropOffsets[ inMapI ].y != 0 ) {
            
            doublePair nullOffset = { 0, 0 };
                    

            doublePair delta = sub( nullOffset, 
                                    mMapDropOffsets[ inMapI ] );
                    
            double step = frameRateFactor * 0.0625;
            double rotStep = frameRateFactor * 0.03125;
                    
            if( length( delta ) < step ) {
                        
                mMapDropOffsets[ inMapI ].x = 0;
                mMapDropOffsets[ inMapI ].y = 0;
                }
            else {
                mMapDropOffsets[ inMapI ] =
                    add( mMapDropOffsets[ inMapI ],
                         mult( normalize( delta ), step ) );
                }
            
            if( mMapDropOffsets[ inMapI ].x == 0 &&
                mMapDropOffsets[ inMapI ].y == 0 ) {
                // done dropping into place
                if( mMapDropSounds[ inMapI ].numSubSounds > 0 ) {
                    
                    playSound( mMapDropSounds[ inMapI ],
                               getVectorFromCamera( 
                                   (double)inScreenX / CELL_D,
                                   (double)inScreenY / CELL_D ) );
                    mMapDropSounds[ inMapI ] = blankSoundUsage;
                    }
                
                }
            
                
            
            double rotDelta = 0 - mMapDropRot[ inMapI ];
            
            if( rotDelta > 0.5 ) {
                rotDelta = rotDelta - 1;
                }
            else if( rotDelta < -0.5 ) {
                rotDelta = 1 + rotDelta;
                }

            if( fabs( rotDelta ) < rotStep ) {
                mMapDropRot[ inMapI ] = 0;
                }
            else {
                double rotSign = 1;
                if( rotDelta < 0 ) {
                    rotSign = -1;
                    }
                
                mMapDropRot[ inMapI ] = 
                    mMapDropRot[ inMapI ] + rotSign * rotStep;
                }
            

                                        
            // step offset BEFORE applying it
            // (so we don't repeat starting position)
            pos = add( pos, mult( mMapDropOffsets[ inMapI ], CELL_D ) );
            
            rot = mMapDropRot[ inMapI ];
            }
        
        if( mMapMoveSpeeds[inMapI] > 0 &&
            ( mMapMoveOffsets[ inMapI ].x != 0 ||
              mMapMoveOffsets[ inMapI ].y != 0  ) ) {

            ignoreWatchedObjectDraw( true );

            pos = add( pos, mult( mMapMoveOffsets[ inMapI ], CELL_D ) );
            }
        


        setDrawColor( 1, 1, 1, 1 );
                
        AnimType curType = ground;
        AnimType fadeTargetType = ground;
        double animFade = 1;

        if( mMapMoveSpeeds[ inMapI ] > 0 ) {
            curType = moving;
            fadeTargetType = moving;
            animFade = 1;
            }
        


        double timeVal = frameRateFactor * 
            mMapAnimationFrameCount[ inMapI ] / 60.0;
                
        double frozenRotTimeVal = frameRateFactor *
            mMapAnimationFrozenRotFrameCount[ inMapI ] / 60.0;

        double targetTimeVal = timeVal;

        if( mMapLastAnimFade[ inMapI ] != 0 ) {
            animFade = mMapLastAnimFade[ inMapI ];
            curType = mMapLastAnimType[ inMapI ];
            fadeTargetType = mMapCurAnimType[ inMapI ];
            
            timeVal = frameRateFactor * 
                mMapAnimationLastFrameCount[ inMapI ] / 60.0;
            }
                
                
            
        char flip = mMapTileFlips[ inMapI ];
        
        ObjectRecord *obj = getObject( oID );
        if( obj->noFlip ||
            ( obj->permanent && 
              ( obj->blocksWalking || obj->drawBehindPlayer || 
                obj->anySpritesBehindPlayer) ) ) {
            // permanent, blocking objects (e.g., walls) 
            // or permanent behind-player objects (e.g., roads) 
            // are never drawn flipped
            flip = false;
            // remember that this tile is NOT flipped, so that it
            // won't flip back strangely if it changes to something
            // that doesn't have a noFlip status
            mMapTileFlips[ inMapI ] = false;
            }
        
        char highlight = false;
        float highlightFade = 1.0f;
        
        if( mCurMouseOverID > 0 &&
            ! mCurMouseOverSelf &&
            mCurMouseOverSpot.y * mMapD + mCurMouseOverSpot.x == inMapI ) {
            
            if( mCurMouseOverBehind ) {
                highlight = inHighlightOnly;
                }
            else {
                highlight = true;
                }
        
            highlightFade = mCurMouseOverFade;
            }
        else {
            for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
                GridPos prev = mPrevMouseOverSpots.getElementDirect( i );
                
                if( prev.y * mMapD + prev.x == inMapI ) {
                    if( mPrevMouseOverSpotsBehind.getElementDirect( i ) ) {
                        highlight = inHighlightOnly;
                        }
                    else {
                        highlight = true;
                        }
                    highlightFade = 
                        mPrevMouseOverSpotFades.getElementDirect(i);
                    }    
                }
            }
        
        if( ! mShowHighlights ) {
            if( inHighlightOnly ) {
                return;
                }
            highlight = false;
            }
        

        if( !mapPullMode && !inHighlightOnly && !inNoTimeEffects ) {
            handleAnimSound( -1,
                             oID, 0, mMapCurAnimType[ inMapI ], oldFrameCount, 
                             mMapAnimationFrameCount[ inMapI ],
                             pos.x / CELL_D,
                             pos.y / CELL_D );
            }
        
        
        if( highlight && obj->noHighlight ) {
            if( inHighlightOnly ) {
                return;
                }
            highlight = false;
            }

        
        int numPasses = 1;
        int startPass = 0;
        
        if( highlight ) {
            
            // first pass normal draw
            // then three stencil passes (second and third with a subtraction)
            numPasses = 6;
            
            if( highlightFade != 1.0f ) {
                //fadeHandle = addGlobalFade( highlightFade );
                }

            if( inHighlightOnly ) {
                startPass = 1;
                }
            }
        
        for( int i=startPass; i<numPasses; i++ ) {
            
            doublePair passPos = pos;
            
            if( highlight ) {
                
                switch( i ) {
                    case 0:
                        // normal color draw
                        break;
                    case 1:
                        // opaque portion
                        startAddingToStencil( false, true, .99f );
                        break;
                    case 2:
                        // first fringe
                        startAddingToStencil( false, true, .07f );
                        break;
                    case 3:
                        // subtract opaque from fringe to get just first fringe
                        startAddingToStencil( false, false, .99f );
                        break;
                    case 4:
                        // second fringe
                        // ignore totally transparent stuff
                        // like invisible animation layers
                        startAddingToStencil( false, true, 0.01f );
                        break;
                    case 5:
                        // subtract first fringe from fringe to get 
                        // just secon fringe
                        startAddingToStencil( false, false, .07f );
                        break;
                    default:
                        break;
                    }

                }

        if( mMapContainedStacks[ inMapI ].size() > 0 ) {
            int *stackArray = 
                mMapContainedStacks[ inMapI ].getElementArray();
            SimpleVector<int> *subStackArray =
                mMapSubContainedStacks[ inMapI ].getElementArray();
            
            drawObjectAnim( oID, 
                            curType, timeVal,
                            animFade,
                            fadeTargetType,
                            targetTimeVal,
                            frozenRotTimeVal,
                            &( mMapAnimationFrozenRotFrameCountUsed[ inMapI ] ),
                            endAnimType,
                            endAnimType,
                            passPos, rot, false, flip,
                            -1,
                            false, false, false,
                            getEmptyClothingSet(),
                            NULL,
                            mMapContainedStacks[ inMapI ].size(),
                            stackArray, subStackArray );
            delete [] stackArray;
            delete [] subStackArray;
            }
        else {
            drawObjectAnim( oID, 2, 
                            curType, timeVal,
                            animFade,
                            fadeTargetType, 
                            targetTimeVal,
                            frozenRotTimeVal,
                            &( mMapAnimationFrozenRotFrameCountUsed[ inMapI ] ),
                            endAnimType,
                            endAnimType,
                            passPos, rot,
                            false,
                            flip, -1,
                            false, false, false,
                            getEmptyClothingSet(), NULL );
            }
        
        ObjectRecord *oRecord = getObject( oID );
        
        if( oRecord->hasBadgePos ) {
            doublePair badgePos = pos;
            badgePos.x += oRecord->badgePos.x;
            badgePos.y += oRecord->badgePos.y;
            
            int badgeID = -1;
            FloatColor badgeColor = { 1, 1, 1, 1 };
            

            int x = inMapI % mMapD;
            int y = inMapI / mMapD;
            
            int worldY = y + mMapOffsetY - mMapD / 2;

            int worldX = x + mMapOffsetX - mMapD / 2;

            for( int g=0; g<mOwnerInfo.size(); g++ ) {
                OwnerInfo *gI = mOwnerInfo.getElement( g );
                
                if( gI->worldPos.x == worldX &&
                    gI->worldPos.y == worldY &&
                    gI->ownerList->size() >= 1 ) {
                    
                    int ownerID = gI->ownerList->getElementDirect( 0 );
                    
                    LiveObject *ownerO = getLiveObject( ownerID );
                    
                    if( ownerO != NULL ) {
                        badgeID = getBadgeObjectID( ownerO ); 
                        badgeColor = ownerO->badgeColor;
                        }
                    }
                }
            if( badgeID != -1 ) {
                spriteColorOverrideOn = true;
                spriteColorOverride = badgeColor;
                char used;
                drawObjectAnim( 
                    badgeID, 2, 
                    curType, timeVal,
                    animFade,
                    fadeTargetType, 
                    targetTimeVal,
                    frozenRotTimeVal,
                    &used,
                    endAnimType,
                    endAnimType,
                    badgePos, rot,
                    false,
                    flip, -1,
                    false, false, false,
                    getEmptyClothingSet(), NULL );
                
                drawObjectAnim( badgeID, 2, 
                                ground, timeVal,
                                0,
                                ground, 
                                timeVal,
                                timeVal,
                                &used,
                                ground,
                                ground,
                                badgePos, 0,
                                false,
                                false, -1,
                                false, false, false,
                                getEmptyClothingSet(), NULL );
                
                spriteColorOverrideOn = false;
                }
            }
        




        if( highlight ) {
            
            
            float mainFade = .35f;
        
            toggleAdditiveBlend( true );
            
            doublePair squarePos = passPos;
            
            if( objectHeight > 1.5 * CELL_D ) {
                squarePos.y += 192;
                }
            
            int squareRad = 306;
            
            switch( i ) {
                case 0:
                    // normal color draw
                    break;
                case 1:
                    // opaque portion
                    startDrawingThroughStencil( false );

                    setDrawColor( 1, 1, 1, highlightFade * mainFade );
                    
                    drawSquare( squarePos, squareRad );
                    
                    stopStencil();
                    break;
                case 2:
                    // first fringe
                    // wait until next pass to isolate fringe
                    break;
                case 3:
                    // now first fringe is isolated in stencil
                    startDrawingThroughStencil( false );

                    setDrawColor( 1, 1, 1, highlightFade * mainFade * .5 );

                    drawSquare( squarePos, squareRad );

                    stopStencil();                    
                    break;
                case 4:
                    // second fringe
                    // wait until next pass to isolate fringe
                    break;
                case 5:
                    // now second fringe is isolated in stencil
                    startDrawingThroughStencil( false );
                    
                    setDrawColor( 1, 1, 1, highlightFade * mainFade *.25 );
                    
                    drawSquare( squarePos, squareRad );
                    
                    stopStencil();
                    break;
                default:
                    break;
                }
            toggleAdditiveBlend( false );
            }

        
            }
        
        
        ignoreWatchedObjectDraw( false );
        
        }
    else if( oID == -1 ) {
        // unknown
        doublePair pos = { (double)inScreenX, (double)inScreenY };
                
        setDrawColor( 0, 0, 0, 0.5 );
        drawSquare( pos, 14 );
        }

    }


SimpleVector<doublePair> trail;
SimpleVector<FloatColor> trailColors;
double pathStepDistFactor = 0.2;

FloatColor trailColor = { 0, 0.5, 0, 0.25 };



GridPos LivingLifePage::getMapPos( int inWorldX, int inWorldY ) {
    GridPos p =
        { inWorldX - mMapOffsetX + mMapD / 2,
          inWorldY - mMapOffsetY + mMapD / 2 };
    
    return p;
    }



int LivingLifePage::getMapIndex( int inWorldX, int inWorldY ) {
    GridPos mapTarget = getMapPos( inWorldX, inWorldY );
    
    if( mapTarget.y >= 0 && mapTarget.y < mMapD &&
        mapTarget.x >= 0 && mapTarget.x < mMapD ) {
                    
        return mapTarget.y * mMapD + mapTarget.x;
        }
    return -1;
    }



int LivingLifePage::getBadgeObjectID( LiveObject *inPlayer ) {
    int badge = -1;
    
    
    int badgeXIndex = 0;
        
    if( inPlayer->isDubious ) {
        badgeXIndex = 1;
        }
    if( inPlayer->isExiled ) {
        badgeXIndex = 2;
        }
        
    if( inPlayer->leadershipLevel < mLeadershipBadges[badgeXIndex].size() ) {
        badge = mLeadershipBadges[badgeXIndex].
            getElementDirect( inPlayer->leadershipLevel );
        }
    else if( mLeadershipBadges[badgeXIndex].size() > 0 ) {
        badge = mLeadershipBadges[badgeXIndex].
            getElementDirect( mLeadershipBadges[badgeXIndex].size() - 1 );
        }
    return badge;
    }



ObjectAnimPack LivingLifePage::drawLiveObject( 
    LiveObject *inObj,
    SimpleVector<LiveObject *> *inSpeakers,
    SimpleVector<doublePair> *inSpeakersPos ) {    


    ObjectAnimPack returnPack;
    returnPack.inObjectID = -1;


    if( inObj->hide || inObj->outOfRange ) {
        return returnPack;
        }


    inObj->onScreen = true;



    if( ! inObj->allSpritesLoaded ) {
        // skip drawing until fully loaded
        return returnPack;
        }

    // current pos
                    
    doublePair pos = mult( inObj->currentPos, CELL_D );
    

    if( inObj->heldByDropOffset.x != 0 ||
        inObj->heldByDropOffset.y != 0 ) {
                    
        doublePair nullOffset = { 0, 0 };
                    
        
        doublePair delta = sub( nullOffset, 
                                inObj->heldByDropOffset );
                    
        double step = frameRateFactor * 0.0625;

        if( length( delta ) < step ) {
            
            inObj->heldByDropOffset.x = 0;
            inObj->heldByDropOffset.y = 0;
            
            ObjectRecord *displayObj = 
                getObject( inObj->displayID );
            
            if( displayObj->usingSound.numSubSounds > 0 ) {
                // play baby's using sound as they are put down
                // we no longer have access to parent's using sound
                playSound( displayObj->usingSound,
                           getVectorFromCamera(
                               inObj->currentPos.x, inObj->currentPos.y ) );
                }
            }
        else {
            inObj->heldByDropOffset =
                add( inObj->heldByDropOffset,
                     mult( normalize( delta ), step ) );
            }
                            
        // step offset BEFORE applying it
        // (so we don't repeat starting position)
        pos = add( pos, mult( inObj->heldByDropOffset, CELL_D ) );
        }
    

    doublePair actionOffset = { 0, 0 };
    
    if( false )
    if( inObj->curAnim == moving ) {
        trail.push_back( pos );
        trailColors.push_back( trailColor );

        while( trail.size() > 1000 ) {
            trail.deleteElement( 0 );
            trailColors.deleteElement( 0 );
            }
        }
    

    int targetX = playerActionTargetX;
    int targetY = playerActionTargetY;
    
    if( inObj->id != ourID ) {
        targetX = inObj->actionTargetX;
        targetY = inObj->actionTargetY;
        }
    else {
        setClothingHighlightFades( inObj->clothingHighlightFades );
        }
    
                
    if( inObj->curAnim != eating &&
        inObj->lastAnim != eating &&
        inObj->pendingActionAnimationProgress != 0 ) {
                    
        // wiggle toward target

        
        int trueTargetX = targetX + inObj->actionTargetTweakX;
        int trueTargetY = targetY + inObj->actionTargetTweakY;
        
                    
        float xDir = 0;
        float yDir = 0;
                
        doublePair dir = { trueTargetX - inObj->currentPos.x,
                           trueTargetY - inObj->currentPos.y };
        
        if( dir.x != 0 || dir.y != 0 ) {    
            dir = normalize( dir );
            }
        
        xDir = dir.x;
        yDir = dir.y;
    
        if( inObj->currentPos.x < trueTargetX ) {
            if( inObj->currentSpeed == 0 ) {
                inObj->holdingFlip = false;
                }
            }
        if( inObj->currentPos.x > trueTargetX ) {
            if( inObj->currentSpeed == 0 ) {
                inObj->holdingFlip = true;
                }
            }

        double wiggleMax = CELL_D *.5 *.90;
        
        double halfWiggleMax = wiggleMax * 0.5;

        if( xDir == 0 && yDir == 0 ) {
            // target where we're standing
            // wiggle tiny bit down
            yDir = -1;
            
            halfWiggleMax *= 0.25;
            }
        else if( xDir == 0 && yDir == -1 ) {
            // moving down causes feet to cross object in our same tile
            // move less
            halfWiggleMax *= 0.5;
            }
        

        
        double offset =
            halfWiggleMax - 
            halfWiggleMax * 
            cos( 2 * M_PI * inObj->pendingActionAnimationProgress );
                    
                    
        actionOffset.x += xDir * offset;
        actionOffset.y += yDir * offset;
        }
                
                
    // bare hands action OR holding something
    // character wiggle
    if( inObj->pendingActionAnimationProgress != 0 ) {
                    
        pos = add( pos, actionOffset );
        }                

                
    AnimType curType = inObj->curAnim;
    AnimType fadeTargetType = inObj->curAnim;
                
    double animFade = 1.0;
                
    double timeVal = frameRateFactor * 
        inObj->animationFrameCount / 60.0;
    
    double targetTimeVal = timeVal;

    double frozenRotTimeVal = frameRateFactor * 
        inObj->frozenRotFrameCount / 60.0;

    if( inObj->lastAnimFade > 0 ) {
        curType = inObj->lastAnim;
        fadeTargetType = inObj->curAnim;
        animFade = inObj->lastAnimFade;
        
        timeVal = frameRateFactor * 
            inObj->lastAnimationFrameCount / 60.0;
        }
                

    setDrawColor( 1, 1, 1, 1 );
    //setDrawColor( red, 0, blue, 1 );
    //mainFont->drawString( string, 
    //                      pos, alignCenter );
                
    double age = computeCurrentAge( inObj );


    ObjectRecord *heldObject = NULL;

    int hideClosestArm = 0;
    char hideAllLimbs = false;


    if( inObj->holdingID != 0 ) { 
        if( inObj->holdingID > 0 ) {
            heldObject = getObject( inObj->holdingID );
            }
        else if( inObj->holdingID < 0 ) {
            // held baby
            LiveObject *babyO = getGameObject( - inObj->holdingID );
            
            if( babyO != NULL ) {    
                heldObject = getObject( babyO->displayID );
                }
            }
        }
    
    
    getArmHoldingParameters( heldObject, &hideClosestArm, &hideAllLimbs );
    

    // override animation types for people who are riding in something
    // animationBank will freeze the time on their arms whenever they are
    // in a moving animation.
    AnimType frozenArmType = endAnimType;
    AnimType frozenArmFadeTargetType = endAnimType;
    
    if( hideClosestArm == -2 
        ||
        ( inObj->holdingID > 0 && getObject( inObj->holdingID )->rideable )
        ||
        ( inObj->lastHoldingID > 0 && 
          getObject( inObj->lastHoldingID )->rideable ) ) {
    
        if( curType == ground2 || curType == moving ) {
            frozenArmType = moving;
            }
        if( fadeTargetType == ground2 || fadeTargetType == moving ) {
            frozenArmFadeTargetType = moving;
            }
        }
    
    char alreadyDrawnPerson = false;
    
    HoldingPos holdingPos;
    holdingPos.valid = false;

    int badge = -1;
    if( inObj->hasBadge && inObj->clothing.tunic != NULL ) {
        badge = getBadgeObjectID( inObj );
        }
    else if( inObj->isExiled ) {
        // exiled and no badge visible
        // show straight X
        badge = mFullXObjectID;
        }
    

    if( inObj->holdingID > 0 &&
        heldObject->rideable ) {
        // don't draw now,
        // wait until we know rideable's offset
        }
    else {
        alreadyDrawnPerson = true;
        doublePair personPos = pos;
        
        // decay away from riding offset, if any
        if( inObj->ridingOffset.x != 0 ||
            inObj->ridingOffset.y != 0 ) {
            
            doublePair nullOffset = { 0, 0 };
            
            
            doublePair delta = sub( nullOffset, inObj->ridingOffset );
            
            double step = frameRateFactor * 8;

            if( length( delta ) < step ) {
            
                inObj->ridingOffset.x = 0;
                inObj->ridingOffset.y = 0;
                }
            else {
                inObj->ridingOffset =
                    add( inObj->ridingOffset,
                         mult( normalize( delta ), step ) );
                }
                            
            // step offset BEFORE applying it
            // (so we don't repeat starting position)
            personPos = add( personPos, inObj->ridingOffset );
            }
        

        setAnimationEmotion( inObj->currentEmot );
        addExtraAnimationEmotions( &( inObj->permanentEmots ) );
        
        // draw young baby lying flat
        double rot = 0;
        if( computeCurrentAgeNoOverride( inObj ) < noMoveAge ) {
            hidePersonShadows( true );
            
            double shiftScale = 1.0;
            
            // slide into the "lying down" shift as they finish getting dropped
            if( inObj->heldByDropOffset.x != 0 ||
                inObj->heldByDropOffset.y != 0 ) {
                doublePair z = { 0, 0 };
                
                double d = distance( z, inObj->heldByDropOffset );
                
                if( d > 0.5 ) {
                    shiftScale = 0;
                    }
                else {
                    shiftScale = ( 0.5 - d ) / 0.5;
                    }
                }
            

            rot = shiftScale * 0.25;
            personPos.x -= shiftScale * 32;
            }
        
        
        // draw on bare skin only if plain X
        setAnimationBadge( badge, 
                           ( badge == mFullXObjectID ) );
        if( badge != -1 ) {
            if( badge == mFullXObjectID ) {
                FloatColor white = { 1, 1, 1, 1 };
                setAnimationBadgeColor( white );
                }
            else {
                setAnimationBadgeColor( inObj->badgeColor );
                }
            }

        holdingPos =
            drawObjectAnim( inObj->displayID, 2, curType, 
                            timeVal,
                            animFade,
                            fadeTargetType,
                            targetTimeVal,
                            frozenRotTimeVal,
                            &( inObj->frozenRotFrameCountUsed ),
                            frozenArmType,
                            frozenArmFadeTargetType,
                            personPos,
                            rot,
                            false,
                            inObj->holdingFlip,
                            age,
                            // don't actually hide body parts until
                            // held object is done sliding into place
                            hideClosestArm,
                            hideAllLimbs,
                            inObj->heldPosOverride && 
                            ! inObj->heldPosOverrideAlmostOver,
                            inObj->clothing,
                            inObj->clothingContained );
        hidePersonShadows( false );

        setAnimationBadge( -1 );

        setAnimationEmotion( NULL );
        }
    
        
    if( inObj->holdingID != 0 ) { 
        doublePair holdPos;
        
        double holdRot = 0;
        
        computeHeldDrawPos( holdingPos, pos,
                            heldObject,
                            inObj->holdingFlip,
                            &holdPos, &holdRot );

                
        doublePair heldObjectDrawPos = holdPos;
        
        if( heldObject != NULL && heldObject->rideable ) {
            heldObjectDrawPos = pos;
            }
        

        heldObjectDrawPos = mult( heldObjectDrawPos, 1.0 / CELL_D );
        
        if( inObj->currentSpeed == 0 &&
            inObj->heldPosOverride && 
            ! inObj->heldPosOverrideAlmostOver &&
            ! equal( heldObjectDrawPos, inObj->heldObjectPos ) ) {
                        
            doublePair delta = sub( heldObjectDrawPos, inObj->heldObjectPos );
            double rotDelta = holdRot - inObj->heldObjectRot;

            if( rotDelta > 0.5 ) {
                rotDelta = rotDelta - 1;
                }
            else if( rotDelta < -0.5 ) {
                rotDelta = 1 + rotDelta;
                }

            // as slide gets longer, we speed up
            double longSlideModifier = 1;
            
            double slideTime = inObj->heldPosSlideStepCount * frameRateFactor;
            
            if( slideTime > 30 ) {
                // more than a half second
                longSlideModifier = pow( slideTime / 30, 2 );
                }

            double step = frameRateFactor * 0.0625 * longSlideModifier;
            double rotStep = frameRateFactor * 0.03125;
            
            if( length( delta ) < step ) {
                inObj->heldObjectPos = heldObjectDrawPos;
                inObj->heldPosOverrideAlmostOver = true;
                }
            else {
                inObj->heldObjectPos =
                    add( inObj->heldObjectPos,
                         mult( normalize( delta ),
                               step ) );
                
                heldObjectDrawPos = inObj->heldObjectPos;
                }

            if( fabs( rotDelta ) < rotStep ) {
                inObj->heldObjectRot = holdRot;
                }
            else {

                double rotDir = 1;
                if( rotDelta < 0 ) {
                    rotDir = -1;
                    }

                inObj->heldObjectRot =
                    inObj->heldObjectRot + rotDir * rotStep;
                
                holdRot = inObj->heldObjectRot;
                }

            inObj->heldPosSlideStepCount ++;
            }
        else {
            inObj->heldPosOverride = false;
            inObj->heldPosOverrideAlmostOver = false;
            // track it every frame so we have a good
            // base point for smooth move when the object
            // is dropped
            inObj->heldObjectPos = heldObjectDrawPos;
            inObj->heldObjectRot = holdRot;
            }
          
        doublePair worldHoldPos = heldObjectDrawPos;
          
        heldObjectDrawPos = mult( heldObjectDrawPos, CELL_D );
        
        if( heldObject == NULL || 
            ! heldObject->rideable ) {
            
            holdPos = heldObjectDrawPos;
            }

        setDrawColor( 1, 1, 1, 1 );
                    
                    
        AnimType curHeldType = inObj->curHeldAnim;
        AnimType fadeTargetHeldType = inObj->curHeldAnim;
                
        double heldAnimFade = 1.0;
                    
        double heldTimeVal = frameRateFactor * 
            inObj->heldAnimationFrameCount / 60.0;
        
        double targetHeldTimeVal = heldTimeVal;
        
        double frozenRotHeldTimeVal = frameRateFactor * 
            inObj->heldFrozenRotFrameCount / 60.0;

        
        char heldFlip = inObj->holdingFlip;

        if( heldObject != NULL &&
            heldObject->noFlip ) {
            heldFlip = false;
            }


        if( !alreadyDrawnPerson ) {
            doublePair personPos = pos;
            
            doublePair targetRidingOffset = sub( personPos, holdPos );

            
            ObjectRecord *personObj = getObject( inObj->displayID );

            targetRidingOffset = 
                sub( targetRidingOffset, 
                     getAgeBodyOffset( 
                         age,
                         personObj->spritePos[ 
                             getBodyIndex( personObj, age ) ] ) );
            
            // step toward target to smooth
            doublePair delta = sub( targetRidingOffset, 
                                    inObj->ridingOffset );
            
            double step = frameRateFactor * 8;

            if( length( delta ) < step ) {            
                inObj->ridingOffset = targetRidingOffset;
                }
            else {
                inObj->ridingOffset =
                    add( inObj->ridingOffset,
                         mult( normalize( delta ), step ) );
                }

            personPos = add( personPos, inObj->ridingOffset );

            setAnimationEmotion( inObj->currentEmot );
            addExtraAnimationEmotions( &( inObj->permanentEmots ) );
            
            if( heldObject->anySpritesBehindPlayer ) {
                // draw part that is behind player
                prepareToSkipSprites( heldObject, true );
                
                if( inObj->numContained == 0 ) {
                    drawObjectAnim(
                        inObj->holdingID, curHeldType, 
                        heldTimeVal,
                        heldAnimFade,
                        fadeTargetHeldType,
                        targetHeldTimeVal,
                        frozenRotHeldTimeVal,
                        &( inObj->heldFrozenRotFrameCountUsed ),
                        endAnimType,
                        endAnimType,
                        heldObjectDrawPos,
                        holdRot,
                        false,
                        heldFlip, -1, false, false, false,
                        getEmptyClothingSet(), NULL,
                        0, NULL, NULL );
                    }
                else {
                    drawObjectAnim( 
                        inObj->holdingID, curHeldType, 
                        heldTimeVal,
                        heldAnimFade,
                        fadeTargetHeldType,
                        targetHeldTimeVal,
                        frozenRotHeldTimeVal,
                        &( inObj->heldFrozenRotFrameCountUsed ),
                        endAnimType,
                        endAnimType,
                        heldObjectDrawPos,
                        holdRot,
                        false,
                        heldFlip,
                        -1, false, false, false,
                        getEmptyClothingSet(),
                        NULL,
                        inObj->numContained,
                        inObj->containedIDs,
                        inObj->subContainedIDs );
                    }
                
                restoreSkipDrawing( heldObject );
                }

            
            setAnimationBadge( badge );
            if( badge != -1 ) {
                setAnimationBadgeColor( inObj->badgeColor );
                }

            // rideable object
            if( ! heldObject->hideRider )
            holdingPos =
                drawObjectAnim( inObj->displayID, 2, curType, 
                                timeVal,
                                animFade,
                                fadeTargetType,
                                targetTimeVal,
                                frozenRotTimeVal,
                                &( inObj->frozenRotFrameCountUsed ),
                                frozenArmType,
                                frozenArmFadeTargetType,
                                personPos,
                                0,
                                false,
                                inObj->holdingFlip,
                                age,
                                // don't actually hide body parts until
                                // held object is done sliding into place
                                hideClosestArm,
                                hideAllLimbs,
                                inObj->heldPosOverride && 
                                ! inObj->heldPosOverrideAlmostOver,
                                inObj->clothing,
                                inObj->clothingContained );
            
            setAnimationEmotion( NULL );
       
            setAnimationBadge( -1 );
            }
        


        // animate baby with held anim just like any other held object
        if( inObj->lastHeldAnimFade > 0 ) {
            curHeldType = inObj->lastHeldAnim;
            fadeTargetHeldType = inObj->curHeldAnim;
            heldAnimFade = inObj->lastHeldAnimFade;
            
            heldTimeVal = frameRateFactor * 
                inObj->lastHeldAnimationFrameCount / 60.0;
            }
        
                    
        if( inObj->holdingID < 0 ) {
            // draw baby here
            int babyID = - inObj->holdingID;
            
            LiveObject *babyO = getGameObject( babyID );
            
            if( babyO != NULL ) {
                
                // save flip so that it sticks when baby set down
                babyO->holdingFlip = inObj->holdingFlip;
                
                // save world hold pos for smooth set-down of baby
                babyO->lastHeldByRawPosSet = true;
                babyO->lastHeldByRawPos = worldHoldPos;

                int hideClosestArmBaby = 0;
                char hideAllLimbsBaby = false;

                if( babyO->holdingID > 0 ) {
                    ObjectRecord *babyHoldingObj = 
                        getObject( babyO->holdingID );
                    
                    getArmHoldingParameters( babyHoldingObj, 
                                             &hideClosestArmBaby,
                                             &hideAllLimbsBaby );
                    }
                
                
                setAnimationEmotion( babyO->currentEmot );
                addExtraAnimationEmotions( &( babyO->permanentEmots ) );

                doublePair babyHeldPos = holdPos;
                
                if( babyO->babyWiggle ) {
                    
                    babyO->babyWiggleProgress += 0.04 * frameRateFactor;
                    
                    if( babyO->babyWiggleProgress > 1 ) {
                        babyO->babyWiggle = false;
                        }
                    else {

                        // cosine from pi to 3 pi has smooth start and finish
                        int wiggleDir = 1;
                        if( heldFlip ) {
                            wiggleDir = -1;
                            }
                        babyHeldPos.x += wiggleDir * 8 *
                            ( cos( babyO->babyWiggleProgress * 2 * M_PI +
                                   M_PI ) * 0.5 + 0.5 );
                        }
                    }

                returnPack =
                    drawObjectAnimPacked( 
                                babyO->displayID, curHeldType, 
                                heldTimeVal,
                                heldAnimFade,
                                fadeTargetHeldType,
                                targetHeldTimeVal,
                                frozenRotHeldTimeVal,
                                &( inObj->heldFrozenRotFrameCountUsed ),
                                endAnimType,
                                endAnimType,
                                babyHeldPos,
                                // never apply held rot to baby
                                0,
                                false,
                                heldFlip,
                                computeCurrentAge( babyO ),
                                hideClosestArmBaby,
                                hideAllLimbsBaby,
                                false,
                                babyO->clothing,
                                babyO->clothingContained,
                                0, NULL, NULL );
                
                setAnimationEmotion( NULL );

                if( babyO->currentSpeech != NULL ) {
                    
                    inSpeakers->push_back( babyO );
                    inSpeakersPos->push_back( holdPos );
                    }
                }
            }
        else if( inObj->numContained == 0 ) {
                        
            returnPack = 
                drawObjectAnimPacked(
                            inObj->holdingID, curHeldType, 
                            heldTimeVal,
                            heldAnimFade,
                            fadeTargetHeldType,
                            targetHeldTimeVal,
                            frozenRotHeldTimeVal,
                            &( inObj->heldFrozenRotFrameCountUsed ),
                            endAnimType,
                            endAnimType,
                            heldObjectDrawPos,
                            holdRot,
                            false,
                            heldFlip, -1, false, false, false,
                            getEmptyClothingSet(), NULL,
                            0, NULL, NULL );
            }
        else {
            returnPack =
                drawObjectAnimPacked( 
                            inObj->holdingID, curHeldType, 
                            heldTimeVal,
                            heldAnimFade,
                            fadeTargetHeldType,
                            targetHeldTimeVal,
                            frozenRotHeldTimeVal,
                            &( inObj->heldFrozenRotFrameCountUsed ),
                            endAnimType,
                            endAnimType,
                            heldObjectDrawPos,
                            holdRot,
                            false,
                            heldFlip,
                            -1, false, false, false,
                            getEmptyClothingSet(),
                            NULL,
                            inObj->numContained,
                            inObj->containedIDs,
                            inObj->subContainedIDs );
            }
        }
                
    if( inObj->currentSpeech != NULL ) {
                    
        inSpeakers->push_back( inObj );
        inSpeakersPos->push_back( pos );
        }

    if( inObj->id == ourID ) {
        setClothingHighlightFades( NULL );
        }
    
    return returnPack;
    }



void LivingLifePage::drawHungerMaxFillLine( doublePair inAteWordsPos,
                                            int inMaxFill,
                                            SpriteHandle *inBarSprites,
                                            SpriteHandle *inDashSprites,
                                            char inSkipBar,
                                            char inSkipDashes ) {
    
    
    
    doublePair barPos = { lastScreenViewCenter.x - 590, 
                          lastScreenViewCenter.y - 334 };
    barPos.x -= 12;
    barPos.y -= 10;
    
    
    barPos.x += 30 * inMaxFill;

    if( ! inSkipBar ) {    
        drawSprite( inBarSprites[ inMaxFill %
                                  NUM_HUNGER_DASHES ], 
                    barPos );
        }
    

    if( inSkipDashes ) {
        return;
        }

    doublePair dashPos = inAteWordsPos;
            
    dashPos.y -= 6;
    dashPos.x -= 5;

    int numDashes = 0;
            
    JenkinsRandomSource dashRandSource( 0 );

    while( dashPos.x > barPos.x + 9 ) {
        
        doublePair drawPos = dashPos;
        
        //drawPos.x += dashRandSource.getRandomBoundedInt( -2, 2 );
        //drawPos.y += dashRandSource.getRandomBoundedInt( -1, 1 );
        
        drawSprite( inDashSprites[ numDashes %
                                   NUM_HUNGER_DASHES ], 
                    drawPos );
        dashPos.x -= 15;
        //numDashes += dashRandSource.getRandomBoundedInt( 1, 10 );
        numDashes += 1;
        
        // correct shortness of last one
        if( numDashes % NUM_HUNGER_DASHES == 0 ) {
            dashPos.x += 3;
            }
        }
            
    // draw one more to connect to bar
    dashPos.x = barPos.x + 6;
    drawSprite( inDashSprites[ numDashes %
                               NUM_HUNGER_DASHES ], 
                dashPos );
    }




static void drawLine( SpriteHandle inSegmentSprite,
                      doublePair inStart, doublePair inEnd,
                      FloatColor inColor ) {
    
    doublePair dir = normalize( sub( inEnd, inStart ) );
    
    doublePair perpDir = { -dir.y, dir.x };
    
    perpDir = mult( perpDir, 2 );
    

    doublePair spriteVerts[4] = 
        { { inStart.x - perpDir.x,
            inStart.y - perpDir.y },
          { inEnd.x - perpDir.x,
            inEnd.y - perpDir.y },
          { inEnd.x + perpDir.x,
            inEnd.y + perpDir.y },
          { inStart.x + perpDir.x,
            inStart.y + perpDir.y } };
    
    FloatColor spriteColors[4] = 
        { inColor, inColor, inColor, inColor };
    
                                
    drawSprite( inSegmentSprite,
                spriteVerts, spriteColors );
    }



static double getBoundedRandom( int inX, int inY,
                                double inUpper, double inLower ) {
    double val = getXYRandom( inX, inY );
    
    return val * ( inUpper - inLower ) + inLower;
    }




static char isInBounds( int inX, int inY, int inMapD ) {
    if( inX < 0 || inY < 0 || inX > inMapD - 1 || inY > inMapD - 1 ) {
        return false;
        }
    return true;
    }



static void drawFixedShadowString( const char *inString, doublePair inPos ) {
    
    FloatColor faceColor = getDrawColor();
    
    setDrawColor( 0, 0, 0, 1 );
    numbersFontFixed->drawString( inString, inPos, alignLeft );
            
    setDrawColor( faceColor );
    
    inPos.x += 2;
    inPos.y -= 2;
    numbersFontFixed->drawString( inString, inPos, alignLeft );
    }



static void drawFixedShadowStringWhite( const char *inString, 
                                        doublePair inPos ) {
    setDrawColor( 1, 1, 1, 1 );
    drawFixedShadowString( inString, inPos );
    }



static void addToGraph( SimpleVector<double> *inHistory, double inValue ) {
    inHistory->push_back( inValue );
                
    while( inHistory->size() > historyGraphLength ) {
        inHistory->deleteElement( 0 );
        }
    }


static void addToGraph( SimpleVector<TimeMeasureRecord> *inHistory, 
                        double inValue[ MEASURE_TIME_NUM_CATEGORIES ] ) {
    TimeMeasureRecord r;
    r.total = 0;
    for( int i=0; i<MEASURE_TIME_NUM_CATEGORIES; i++ ) {
        r.timeMeasureAverage[i] = inValue[i];
        r.total += inValue[i];
        }
    
    inHistory->push_back( r );
                
    while( inHistory->size() > historyGraphLength ) {
        inHistory->deleteElement( 0 );
        }
    }



static void drawGraph( SimpleVector<double> *inHistory, doublePair inPos,
                       FloatColor inColor ) {
    double max = 0;
    for( int i=0; i<inHistory->size(); i++ ) {
        double val = inHistory->getElementDirect( i );
        if( val > max ) {
            max = val;
            }
        }

    setDrawColor( 0, 0, 0, 0.5 );

    double graphHeight = 40;

    drawRect( inPos.x - 2, 
              inPos.y - 2,
              inPos.x + historyGraphLength + 2,
              inPos.y + graphHeight + 2 );
        
    

    setDrawColor( inColor.r, inColor.g, inColor.b, 0.75 );
    for( int i=0; i<inHistory->size(); i++ ) {
        double val = inHistory->getElementDirect( i );

        double scaledVal = val / max;
        
        drawRect( inPos.x + i, 
                  inPos.y,
                  inPos.x + i + 1,
                  inPos.y + scaledVal * graphHeight );
        }
    }




static void drawGraph( SimpleVector<TimeMeasureRecord> *inHistory, 
                       doublePair inPos,
                       FloatColor inColor[MEASURE_TIME_NUM_CATEGORIES] ) {
    double max = 0;
    for( int i=0; i<inHistory->size(); i++ ) {
        double val = inHistory->getElementDirect( i ).total;
        if( val > max ) {
            max = val;
            }
        }

    setDrawColor( 0, 0, 0, 0.5 );

    double graphHeight = 40;

    drawRect( inPos.x - 2, 
              inPos.y - 2,
              inPos.x + historyGraphLength + 2,
              inPos.y + graphHeight + 2 );
        
    

    for( int i=0; i<inHistory->size(); i++ ) {

        for( int m=MEASURE_TIME_NUM_CATEGORIES - 1; m >= 0; m-- ) {
            
            double sum = 0;
            
            for( int n=m; n>=0; n-- ) {
            
                sum += inHistory->getElementDirect( i ).timeMeasureAverage[n];
                }

            FloatColor c = timeMeasureGraphColors[m];
            
            setDrawColor( c.r, c.g, c.b, 0.75 );
            
            double scaledVal = sum / max;
            
            drawRect( inPos.x + i, 
                      inPos.y,
                      inPos.x + i + 1,
                      inPos.y + scaledVal * graphHeight );
            }
        }
    }


// found here:
// https://stackoverflow.com/questions/1449805/how-to-format-a-number-from-1123456789-to-1-123-456-789-in-c/24795133#24795133
size_t stringFormatIntGrouped( char dst[16], int num ) {
    char src[16];
    char *p_src = src;
    char *p_dst = dst;

    const char separator = ',';
    int num_len, commas;

    num_len = sprintf( src, "%d", num );

    if( *p_src == '-' ) {
        *p_dst++ = *p_src++;
        num_len--;
        }

    for( commas = 2 - num_len % 3;
         *p_src;
         commas = (commas + 1) % 3 ) {
        
        *p_dst++ = *p_src++;
        if( commas == 1 ) {
            *p_dst++ = separator;
            }
        }
    *--p_dst = '\0';

    return (size_t)( p_dst - dst );
    }



// true if holding "sick" object
char isSick( LiveObject *inPlayer ) {
    if( inPlayer->holdingID <= 0 ){
        return false;
        }
    ObjectRecord *o = getObject( inPlayer->holdingID );
    
    if( ! o->permanent ) {
        return false;
        }
    if( strstr( o->description, " sick" ) != NULL ) {
        return true;
        }
    return false;
    }



#define NUM_NUMBER_KEYS 21
static const char *numberKeys[ NUM_NUMBER_KEYS ] = { 
    "zero",
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine",
    "ten",
    "eleven",
    "twelve",
    "thirteen",
    "fourteen",
    "fifteen",
    "sixteen",
    "seventeen",
    "eighteen",
    "nineteen",
    "twenty"
    };


static const char *numberTenKeys[ 8 ] = { 
    "twenty",
    "thirty",
    "forty",
    "fifty",
    "sixty",
    "seventy",
    "eighty",
    "ninety"
    };



// for numbers < 999
char *getSmallNumberString( int inNumber, 
                            const char *inUnits = "",
                            const char *inPadding = "" ) {
    
    int numLeft = inNumber;
    
    int hundreds = numLeft / 100;
    numLeft -= hundreds * 100;
    
    int tens = numLeft / 10;
    
    numLeft -= tens * 10;
    
    int ones = numLeft;

    int tensPlusOnes = tens * 10 + ones;
    
    char *onesString;
    char *tensString;
    char *hundredsString;

    if( tensPlusOnes < NUM_NUMBER_KEYS ) {
        onesString = stringDuplicate( "" );
        if( tensPlusOnes > 0 ) {
            tensString = 
                stringDuplicate( translate( numberKeys[tensPlusOnes] ) );
            }
        else {
            tensString = stringDuplicate( "" );
            }
        }
    else {
        if( ones > 0 ) {
            onesString = stringDuplicate( translate( numberKeys[ones] ) );
            }
        else {
            onesString = stringDuplicate( "" );
            }
        
        const char *hyphen = "";
        if( ones > 0 ) {
            hyphen = "-";
            }
        tensString = 
            autoSprintf( "%s%s", 
                         translate( numberTenKeys[ tens - 2 ] ),
                         hyphen );
        }
    if( hundreds > 0 ) {
        const char *andString = "";
        const char *andSpace = "";
        if( tens > 0 || ones > 0 ) {
            andString = translate( "and" );
            andSpace = " ";
            }

        char *rawHundredString;
        // sometimes we are fed something like 2200, etc
        if( hundreds < NUM_NUMBER_KEYS ) {
            rawHundredString = 
                stringDuplicate( translate( numberKeys[hundreds] ) );
            }
        else {
            // recurse 
            rawHundredString = getSmallNumberString( hundreds );
            }
        
        
        hundredsString = autoSprintf( "%s %s%s%s",
                                      rawHundredString,
                                      translate( "hundred" ),
                                      andString, andSpace );
        delete [] rawHundredString;
        }
    else {
        hundredsString = stringDuplicate( "" );
        }
    
    const char *unitsSpace = "";
    if( strcmp( inUnits, "" ) != 0 ) {
        unitsSpace = " ";
        }
    
    char *final = autoSprintf( "%s%s%s%s%s%s", hundredsString, 
                               tensString, onesString, unitsSpace,
                               inUnits,inPadding );
    
    delete [] hundredsString;
    delete [] tensString;
    delete [] onesString;

    return final;
    }




char *getSpokenNumber( unsigned int inNumber, int inSigFigs = 2 ) {
    if( inNumber < NUM_NUMBER_KEYS ) {
        return stringDuplicate( translate( numberKeys[ inNumber ] ) );
        }

    int numDigits = 0;
    
    unsigned int numLeft = inNumber;
    
    while( numLeft > 0 ) {
        numDigits ++;
        numLeft /= 10;
        }

    
    // round to specified sig figs
    
    int figsToDiscard = numDigits - inSigFigs;
    
    if( figsToDiscard < 0 ) {
        figsToDiscard = 0;
        }
    
    numLeft = inNumber;
    
    int figsLeft = figsToDiscard;
    while( figsLeft > 0 ) {
        numLeft /= 10;
        figsLeft --;
        }

    
    // restore size
    figsLeft = figsToDiscard;
    while( figsLeft > 0 ) {
        numLeft *= 10;
        figsLeft --;
        }

    double remainder = inNumber - numLeft;
    // turn into decimal
    figsLeft = figsToDiscard;
    while( figsLeft > 0 ) {
        remainder /= 10.0;
        figsLeft --;
        }
    
    remainder = round( remainder );

    if( remainder == 1 ) {
        // round up
        figsLeft = figsToDiscard;
        while( figsLeft > 0 ) {
            remainder *= 10;
            figsLeft --;
            }
        numLeft += remainder;
        }
    
    
    
    int billions = numLeft / 1000000000;
    numLeft -= billions * 1000000000;
    
    int millions = numLeft / 1000000.0;
    numLeft -= millions * 1000000;

    int thousands = numLeft / 1000;
    numLeft -= thousands * 1000;

    int hundreds = numLeft;


    if( thousands >= 1 && hundreds >= 100 && thousands <= 9 ) {
        // roll into hundreds
        hundreds += thousands * 1000;
        thousands = 0;
        }
    
    char *billionsString;
    if( billions > 0 ) {
        const char *padding = "";
        if( millions > 0 ) {
            padding = " ";
            }
        billionsString = getSmallNumberString( billions, 
                                               translate( "billion" ),
                                               padding );
        }
    else {
        billionsString = stringDuplicate( "" );
        }

    char *millionsString;
    if( millions > 0 ) {
        const char *padding = "";
        if( thousands > 0 ) {
            padding = " ";
            }
        millionsString = getSmallNumberString( millions,
                                               translate( "million" ),
                                               padding );
        }
    else {
        millionsString = stringDuplicate( "" );
        }

    char *thousandsString;
    if( thousands > 0 ) {
        char *padding = (char*)"";
        char *paddingToDelete = NULL;
        
        if( hundreds > 0 ) {
            padding = (char*)" ";
            
            if( hundreds < 100 ) {
                padding = autoSprintf( "%s ", translate( "and" ) );
                paddingToDelete = padding;
                }
            }
        thousandsString = getSmallNumberString( thousands,
                                                translate( "thousand" ),
                                                padding );
        if( paddingToDelete != NULL ) {
            delete [] paddingToDelete;
            }
        }
    else {
        thousandsString = stringDuplicate( "" );
        }


    char *hundredsString;
    if( hundreds > 0 ) {
        hundredsString = getSmallNumberString( hundreds );
        }
    else {
        hundredsString = stringDuplicate( "" );
        }

    char *result = autoSprintf( "%s%s%s%s", billionsString, millionsString,
                                thousandsString, hundredsString );
    
    delete [] billionsString;
    delete [] millionsString;
    delete [] thousandsString;
    delete [] hundredsString;
    
    return result;
    }



static char mapHintEverDrawn[2] = { false, false };
static char personHintEverDrawn[2] = { false, false };
static const char *personHintEverDrawnKey[2] = { "", "" };



void LivingLifePage::drawHomeSlip( doublePair inSlipPos, int inIndex ) {
    doublePair slipPos = inSlipPos;
    
    setDrawColor( 1, 1, 1, 1 );
    drawSprite( mHomeSlipSprites[inIndex], slipPos );

        
    doublePair arrowPos = slipPos;
    
    if( inIndex == 0 ) {
        arrowPos.y += 35;
        }
    else {
        arrowPos.y += 35;
        }
    
    LiveObject *ourLiveObject = getOurLiveObject();

    if( ourLiveObject != NULL ) {
            
        double homeDist = 0;
        char tooClose = false;
        char temporary = false;
        char tempPerson = false;
        const char *tempPersonKey = NULL;
        
        int arrowIndex = getHomeDir( ourLiveObject->currentPos, &homeDist,
                                     &tooClose, &temporary, 
                                     &tempPerson, &tempPersonKey, inIndex );
            
        if( arrowIndex == -1 || 
            ! mHomeArrowStates[inIndex][arrowIndex].solid ) {
            // solid change

            // fade any solid
                
            int foundSolid = -1;
            for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
                if( mHomeArrowStates[inIndex][i].solid ) {
                    mHomeArrowStates[inIndex][i].solid = false;
                    foundSolid = i;
                    }
                }
            if( foundSolid != -1 ) {
                for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
                    if( i != foundSolid ) {
                        mHomeArrowStates[inIndex][i].fade -= 0.0625;
                        if( mHomeArrowStates[inIndex][i].fade < 0 ) {
                            mHomeArrowStates[inIndex][i].fade = 0;
                            }
                        }
                    }                
                }
            }
            
        if( arrowIndex != -1 ) {
            mHomeArrowStates[inIndex][arrowIndex].solid = true;
            mHomeArrowStates[inIndex][arrowIndex].fade = 1.0;
            }
            
        toggleMultiplicativeBlend( true );
            
        toggleAdditiveTextureColoring( true );
        
        for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
            HomeArrow a = mHomeArrowStates[inIndex][i];
                
            if( ! a.solid ) {
                    
                float v = 1.0 - a.fade;
                setDrawColor( v, v, v, 1 );
                drawSprite( mHomeArrowErasedSprites[i], arrowPos );
                }
            }
            
        toggleAdditiveTextureColoring( false );


            
        if( arrowIndex != -1 ) {
                
                
            setDrawColor( 1, 1, 1, 1 );
                
            drawSprite( mHomeArrowSprites[arrowIndex], arrowPos );
            }
                            
        toggleMultiplicativeBlend( false );
            
        char drawTopAsErased = true;
            
        doublePair distPos = arrowPos;
        doublePair mapHintPos = arrowPos;

        if( inIndex == 0 ) {
            distPos.y -= 47;
            mapHintPos.y -= 47;
            }
        else {
            distPos.y -= 47;
            mapHintPos.y -= 47;
            }
        

        setDrawColor( 0, 0, 0, 1 );

        if( inIndex == 1 ) {
            doublePair bellPos = distPos;
            bellPos.y += 20;    
            
            const char *arrowWord = "BELL";
            if( isAncientHomePosHell ) {
                arrowWord = "HELL";
                }
            handwritingFont->drawString( arrowWord, bellPos, alignCenter );
            }
        
        if( temporary ) {
            // push distance label further down
            if( inIndex == 0 ) {
                distPos.y -= 20;
                }
            else {
                distPos.y -= 20;
                }
            
            if( tempPerson ) {
                personHintEverDrawn[inIndex] = true;
                personHintEverDrawnKey[inIndex] = tempPersonKey;
                
                pencilFont->drawString( translate( tempPersonKey ), 
                                        mapHintPos, alignCenter );

                if( mapHintEverDrawn[inIndex] ) {
                    pencilErasedFont->drawString( translate( "map" ), 
                                              mapHintPos, alignCenter );
                    }
                }
            else {
                mapHintEverDrawn[inIndex] = true;
                pencilFont->drawString( translate( "map" ), 
                                        mapHintPos, alignCenter );
                
                if( personHintEverDrawn[inIndex] ) {
                    pencilErasedFont->drawString( 
                        translate( personHintEverDrawnKey[inIndex] ), 
                        mapHintPos, alignCenter );
                    }
                }
            }
        else {
            if( mapHintEverDrawn[inIndex] ) {
                if( inIndex == 0 ) {
                    distPos.y -= 20;
                    }
                else {
                    distPos.y -= 20;
                    }
                pencilErasedFont->drawString( translate( "map" ), 
                                              mapHintPos, alignCenter );
                }
            if( personHintEverDrawn[inIndex] ) {
                if( inIndex == 0 ) {
                    distPos.y -= 20;
                    }
                else {
                    distPos.y -= 20;
                    }
                pencilErasedFont->drawString( 
                    translate( personHintEverDrawnKey[inIndex] ), 
                    mapHintPos, alignCenter );
                }
            }
            

        if( homeDist > 1000 ) {
            drawTopAsErased = false;
                
            setDrawColor( 0, 0, 0, 1 );
                
            char *distString = NULL;

            double thousands = homeDist / 1000;
                
            if( thousands < 1000 ) {
                if( thousands < 10 ) {
                    distString = autoSprintf( "%.1fK", thousands );
                    }
                else {
                    distString = autoSprintf( "%.0fK", 
                                              thousands );
                    }
                }
            else {
                double millions = homeDist / 1000000;
                if( millions < 1000 ) {
                    if( millions < 10 ) {
                        distString = autoSprintf( "%.1fM", millions );
                        }
                    else {
                        distString = autoSprintf( "%.0fM", millions );
                        }
                    }
                else {
                    double billions = homeDist / 1000000000;
                        
                    distString = autoSprintf( "%.1fG", billions );
                    }
                }

                
                
            pencilFont->drawString( distString, distPos, alignCenter );

            char alreadyOld = false;

            for( int i=0; i<mPreviousHomeDistStrings[inIndex].size(); i++ ) {
                char *oldString = 
                    mPreviousHomeDistStrings[inIndex].getElementDirect( i );
                    
                if( strcmp( oldString, distString ) == 0 ) {
                    // hit
                    alreadyOld = true;
                    // move to top
                    mPreviousHomeDistStrings[inIndex].deleteElement( i );
                    mPreviousHomeDistStrings[inIndex].push_back( oldString );
                        
                    mPreviousHomeDistFades[inIndex].deleteElement( i );
                    mPreviousHomeDistFades[inIndex].push_back( 1.0f );
                    break;
                    }
                }
                
            if( ! alreadyOld ) {
                // put new one top
                mPreviousHomeDistStrings[inIndex].push_back( distString );
                mPreviousHomeDistFades[inIndex].push_back( 1.0f );
                    
                // fade old ones
                for( int i=0; i<mPreviousHomeDistFades[inIndex].size() - 1; 
                     i++ ) {
                    float fade = 
                        mPreviousHomeDistFades[inIndex].getElementDirect( i );
                        
                    if( fade > 0.5 ) {
                        fade -= 0.20;
                        }
                    else {
                        fade -= 0.1;
                        }
                        
                    *( mPreviousHomeDistFades[inIndex].getElement( i ) ) =
                        fade;
                        
                    if( fade <= 0 ) {
                        mPreviousHomeDistFades[inIndex].deleteElement( i );
                        mPreviousHomeDistStrings[inIndex].
                            deallocateStringElement( i );
                        i--;
                        }
                    }
                }
            else {
                delete [] distString;
                }
            }
            
        int numPrevious = mPreviousHomeDistStrings[inIndex].size();
            
        if( numPrevious > 1 ||
            ( numPrevious == 1 && drawTopAsErased ) ) {
                
            int limit = mPreviousHomeDistStrings[inIndex].size() - 1;
                
            if( drawTopAsErased ) {
                limit += 1;
                }
            for( int i=0; i<limit; i++ ) {
                float fade = 
                    mPreviousHomeDistFades[inIndex].getElementDirect( i );
                char *string = 
                    mPreviousHomeDistStrings[inIndex].getElementDirect( i );
                    
                setDrawColor( 0, 0, 0, fade * pencilErasedFontExtraFade );
                pencilErasedFont->drawString( 
                    string, distPos, alignCenter );
                }
            }    
        }
    }



typedef struct DrawOrderRecord {
        char person;
        // if person
        LiveObject *personO;
        
        // if cell
        int mapI;
        int screenX, screenY;

        char extraMovingObj;
        // if extra moving obj
        int extraMovingIndex;
        
    } DrawOrderRecord;
        


char drawAdd = true;
char drawMult = true;

double multAmount = 0.15;
double addAmount = 0.25;


char blackBorder = false;
                                
char whiteBorder = true;



char LivingLifePage::isCoveredByFloor( int inTileIndex ) {
    int i = inTileIndex;
    
    int fID = mMapFloors[ i ];

    if( fID > 0 && 
        ! getObject( fID )->noCover ) {
        return true;
        }
    return false;
    }



void LivingLifePage::draw( doublePair inViewCenter, 
                           double inViewSize ) {
    
    double drawStartTime = showFPS ? game_getCurrentTime() : 0;


    setViewCenterPosition( lastScreenViewCenter.x,
                           lastScreenViewCenter.y );

    char stillWaitingBirth = false;
    

    if( mFirstServerMessagesReceived != 3 ) {
        // haven't gotten first messages from server yet
        stillWaitingBirth = true;
        }
    else if( mFirstServerMessagesReceived == 3 ) {
        if( !mDoneLoadingFirstObjectSet ) {
            stillWaitingBirth = true;
            }
        }


    if( stillWaitingBirth ) {
        
        if( getSpriteBankLoadFailure() != NULL ||
            getSoundBankLoadFailure() != NULL ) {    
            setSignal( "loadFailure" );
            }
        
        // draw this to cover up utility text field, but not
        // waiting icon at top
        setDrawColor( 0, 0, 0, 1 );
        drawSquare( lastScreenViewCenter, 100 );
        
        setDrawColor( 1, 1, 1, 1 );
        doublePair pos = { lastScreenViewCenter.x, lastScreenViewCenter.y };
        

       
        if( connectionMessageFade > 0 ) {
            
            if( serverSocketConnected ) {    
                connectionMessageFade -= 0.05 * frameRateFactor;
                
                if( connectionMessageFade < 0 ) {
                    connectionMessageFade = 0;
                    }       
                }
            
            
            doublePair conPos = pos;
            conPos.y += 128;
            drawMessage( "connecting", conPos, false, connectionMessageFade );
            }

        
        setDrawColor( 1, 1, 1, 1 );

        if( usingCustomServer ) {
            char *upperIP = stringToUpperCase( serverIP );
            
            char *message = autoSprintf( translate( "customServerMesssage" ),
                                         upperIP, serverPort );
            delete [] upperIP;
            
            doublePair custPos = pos;
            custPos.y += 192;
            drawMessage( message, custPos );
            
            delete [] message;
            }
        
        

        if( ! serverSocketConnected ) {
            // don't draw waiting message, not connected yet
            if( userReconnect ) {
                drawMessage( "waitingReconnect", pos );
                }
            }
        else if( userReconnect ) {
            drawMessage( "waitingReconnect", pos );
            }
        else if( mPlayerInFlight ) {
            drawMessage( "waitingArrival", pos );
            }
        else if( userTwinCode == NULL ) {
            drawMessage( "waitingBirth", pos );
            }
        else {
            const char *sizeString = translate( "twins" );
            
            if( userTwinCount == 3 ) {
                sizeString = translate( "triplets" );
                }
            else if( userTwinCount == 4 ) {
                sizeString = translate( "quadruplets" );
                }
            char *message = autoSprintf( translate( "waitingBirthFriends" ),
                                         sizeString );

            drawMessage( message, pos );
            delete [] message;

            if( !mStartedLoadingFirstObjectSet ) {
                doublePair tipPos = pos;
                tipPos.y -= 200;
                
                drawMessage( translate( "cancelWaitingFriends" ), tipPos );
                }
            }
        
        
        // hide map loading progress, because for now, it's almost
        // instantaneous
        if( false && mStartedLoadingFirstObjectSet ) {
            
            pos.y -= 100;
            drawMessage( "loadingMap", pos );

            // border
            setDrawColor( 1, 1, 1, 1 );
    
            drawRect( pos.x - 100, pos.y - 120, 
                      pos.x + 100, pos.y - 100 );

            // inner black
            setDrawColor( 0, 0, 0, 1 );
            
            drawRect( pos.x - 98, pos.y - 118, 
                      pos.x + 98, pos.y - 102 );
    
    
            // progress
            setDrawColor( .8, .8, .8, 1 );
            drawRect( pos.x - 98, pos.y - 118, 
                      pos.x - 98 + mFirstObjectSetLoadingProgress * ( 98 * 2 ), 
                      pos.y - 102 );
            }
        
        return;
        }


    //setDrawColor( 1, 1, 1, 1 );
    //drawSquare( lastScreenViewCenter, viewWidth );
    

    //if( currentGamePage != NULL ) {
    //    currentGamePage->base_draw( lastScreenViewCenter, viewWidth );
    //    }
    
    setDrawColor( 1, 1, 1, 1 );

    int gridCenterX = 
        lrintf( lastScreenViewCenter.x / CELL_D ) - mMapOffsetX + mMapD/2;
    int gridCenterY = 
        lrintf( lastScreenViewCenter.y / CELL_D ) - mMapOffsetY + mMapD/2;
    
    // more on left and right of screen to avoid wide object tops popping in
    int xStart = gridCenterX - 7;
    int xEnd = gridCenterX + 7;

    // more on bottom of screen so that tall objects don't pop in
    int yStart = gridCenterY - 6;
    int yEnd = gridCenterY + 4;

    if( xStart < 0 ) {
        xStart = 0;
        }
    if( xStart >= mMapD ) {
        xStart = mMapD - 1;
        }
    
    if( yStart < 0 ) {
        yStart = 0;
        }
    if( yStart >= mMapD ) {
        yStart = mMapD - 1;
        }

    if( xEnd < 0 ) {
        xEnd = 0;
        }
    if( xEnd >= mMapD ) {
        xEnd = mMapD - 1;
        }
    
    if( yEnd < 0 ) {
        yEnd = 0;
        }
    if( yEnd >= mMapD ) {
        yEnd = mMapD - 1;
        }



    // don't bound floor start and end here
    // 
    // we want to show unknown biome off edge
    // instead, check before using to index mMapBiomes mid-loop
    
    // note that we can't check mMapCellDrawnFlags outside of map boundaries
    // which will result in some over-drawing out there (whole sheets with
    // tiles drawn on top).  However, given that we're not drawing anything
    // else out there, this should be okay from a performance standpoint.

    int yStartFloor = gridCenterY - 4;
    int yEndFloor = gridCenterY + 4;

    int xStartFloor = gridCenterX - 6;
    int xEndFloor = gridCenterX + 6;

    


    int numCells = mMapD * mMapD;

    memset( mMapCellDrawnFlags, false, numCells );

    // draw underlying ground biomes

    // two passes
    // once for biomes
    // second time for overlay shading on y-culvert lines
    for( int pass=0; pass<2; pass++ )
    for( int y=yEndFloor; y>=yStartFloor; y-- ) {

        int screenY = CELL_D * ( y + mMapOffsetY - mMapD / 2 );

        int tileY = -lrint( screenY / CELL_D );

        int tileWorldY = - tileY;
        

        // slight offset to compensate for tile overlaps and
        // make biome tiles more centered on world tiles
        screenY -= 32;
        
        for( int x=xStartFloor; x<=xEndFloor; x++ ) {
            int mapI = y * mMapD + x;
            
            char inBounds = isInBounds( x, y, mMapD );

            if( pass == 0 && inBounds && mMapCellDrawnFlags[mapI] ) {
                continue;
                }

            int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );
            
            int tileX = lrint( screenX / CELL_D );

            // slight offset to compensate for tile overlaps and
            // make biome tiles more centered on world tiles
            screenX += 32;
            
            int b = -1;
            
            if( inBounds ) {
                b = mMapBiomes[mapI];
                }
            
            GroundSpriteSet *s = NULL;
            
            setDrawColor( 1, 1, 1, 1 );

            if( b >= 0 && b < groundSpritesArraySize ) {
                s = groundSprites[ b ];
                }
            else if( b == -1 ) {
                // unknown biome image at end
                s = groundSprites[ groundSpritesArraySize - 1 ];
                }
            
            if( s == NULL ) {
                
                // use end image with random color
                s = groundSprites[ groundSpritesArraySize - 1 ];
                
                // random draw color
                setDrawColor( getXYRandom( b, b ),
                              getXYRandom( b, b + 100 ),
                              getXYRandom( b, b + 300 ), 1 );
                /*
                // find another
                for( int i=0; i<groundSpritesArraySize && s == NULL; i++ ) {
                    s = groundSprites[ i ];
                    }
                */
                }
            
            
            if( s != NULL ) {
                
                
                            
                doublePair pos = { (double)screenX, (double)screenY };
                
                
                // wrap around
                int setY = tileY % s->numTilesHigh;
                int setX = tileX % s->numTilesWide;
                
                if( setY < 0 ) {
                    setY += s->numTilesHigh;
                    }
                if( setX < 0 ) {
                    setX += s->numTilesHigh;
                    }
                
                if( pass == 0 )
                if( setY == 0 && setX == 0 ) {
                    
                    // check if we're on corner of all-same-biome region that
                    // we can fill with one big sheet

                    char allSameBiome = true;
                    
                    // check borders of would-be sheet too
                    for( int nY = y+1; nY >= y - s->numTilesHigh; nY-- ) {
                        
                        if( nY >=0 && nY < mMapD ) {
                            
                            for( int nX = x-1; 
                                 nX <= x + s->numTilesWide; nX++ ) {
                                
                                if( nX >=0 && nX < mMapD ) {
                                    int nI = nY * mMapD + nX;
                                    
                                    int nB = -1;
                                    
                                    if( isInBounds( nX, nY, mMapD ) ) {
                                        nB = mMapBiomes[nI];
                                        }

                                    if( nB != b ) {
                                        allSameBiome = false;
                                        break;
                                        }
                                    }
                                }

                            }
                        if( !allSameBiome ) {
                            break;
                            }
                        }
                    
                    if( allSameBiome ) {
                        
                        doublePair lastCornerPos = 
                            { pos.x + ( s->numTilesWide - 1 ) * CELL_D, 
                              pos.y - ( s->numTilesHigh - 1 ) * CELL_D };

                        doublePair sheetPos = mult( add( pos, lastCornerPos ),
                                                    0.5 );
                        
                        drawSprite( s->wholeSheet, sheetPos );
                        
                        // mark all cells under sheet as drawn
                        for( int sY = y; sY > y - s->numTilesHigh; sY-- ) {
                        
                            if( sY >=0 && sY < mMapD ) {
                            
                                for( int sX = x; 
                                     sX < x + s->numTilesWide; sX++ ) {
                                
                                    if( sX >=0 && sX < mMapD ) {
                                        int sI = sY * mMapD + sX;
                                        
                                        mMapCellDrawnFlags[sI] = true;
                                        }
                                    }
                                }
                            }
                        }
                    }

                if( pass == 0 )
                if( ! inBounds || ! mMapCellDrawnFlags[mapI] ) {
                    // not drawn as whole sheet
                    
                    int aboveB = -1;
                    int leftB = -1;
                    int diagB = -1;
                    
                    if( isInBounds( x -1, y, mMapD ) ) {    
                        leftB = mMapBiomes[ mapI - 1 ];
                        }
                    if( isInBounds( x, y + 1, mMapD ) ) {    
                        aboveB = mMapBiomes[ mapI + mMapD ];
                        }
                    
                    if( isInBounds( x + 1, y + 1, mMapD ) ) {    
                        diagB = mMapBiomes[ mapI + mMapD + 1 ];
                        }
                    
                    char floorAt = isCoveredByFloor( mapI );
                    char floorR = false;
                    char floorB = false;
                    char floorBR = false;
                    
                    if( isInBounds( x +1, y, mMapD ) ) {    
                        floorR = isCoveredByFloor( mapI + 1 );
                        }
                    if( isInBounds( x, y - 1, mMapD ) ) {    
                        floorB = isCoveredByFloor( mapI - mMapD );
                        }
                    if( isInBounds( x +1, y - 1, mMapD ) ) {    
                        floorBR = isCoveredByFloor( mapI - mMapD + 1 );
                        }



                    if( leftB == b &&
                        aboveB == b &&
                        diagB == b ) {
                        
                        // surrounded by same biome above and to left
                        // AND diagonally to the above-right
                        // draw square tile here to save pixel fill time
                        
                        // skip if biome square completely covered by floors
                        if( !( floorAt && floorR && floorB && floorBR ) ) {
                            drawSprite( s->squareTiles[setY][setX], pos );
                            }
                        }
                    else {
                        // non-square piece
                        // avoid drawing if completely overdrawn by floors
                        // in 3x3 grid around
                        
                        char floorL = false;
                        char floorA = false;
                        char floorAL = false;
                        char floorAR = false;
                        char floorBL = false;
                    
                        if( isInBounds( x -1, y, mMapD ) ) {    
                            floorL = isCoveredByFloor( mapI - 1 );
                            }
                        if( isInBounds( x, y+1, mMapD ) ) {    
                            floorA = isCoveredByFloor( mapI + mMapD );
                            }
                        if( isInBounds( x-1, y+1, mMapD ) ) {    
                            floorAL = isCoveredByFloor( mapI + mMapD - 1 );
                            }
                        if( isInBounds( x+1, y+1, mMapD ) ) {    
                            floorAR = isCoveredByFloor( mapI + mMapD + 1 );
                            }
                        if( isInBounds( x-1, y-1, mMapD ) ) {    
                            floorBL = isCoveredByFloor( mapI - mMapD - 1 );
                            }

                        if( !( floorAt && floorR && floorB && floorBR &&
                               floorL && floorA && floorAL && floorAR &&
                               floorBL ) ) {
                            drawSprite( s->tiles[setY][setX], pos );
                            }
                        }
                    if( inBounds ) {
                        mMapCellDrawnFlags[mapI] = true;
                        }
                    }
                
                if( pass == 1 ) {
                    
                    int yMod = abs( tileWorldY + valleyOffset ) % valleySpacing;
                    
                    // on a culvert fault line?
                    if( yMod == 0 ) {
                        
                        setDrawColor( 0, 0, 0, 0.625 );
                        
                        JenkinsRandomSource stonePicker( tileX );
                        
                        if( mCulvertStoneSpriteIDs.size() > 0 ) {
                            
                            for( int s=0; s<2; s++ ) {
                                
                                int stoneIndex = 
                                    stonePicker.getRandomBoundedInt( 
                                        0,
                                        mCulvertStoneSpriteIDs.size() - 1 );
                                
                                int stoneSpriteID =
                                    mCulvertStoneSpriteIDs.getElementDirect( 
                                        stoneIndex );
                                
                                doublePair stoneJigglePos = pos;
                                
                                if( s == 1 ) {
                                    stoneJigglePos.x += CELL_D / 2;
                                    }
                                
                                stoneJigglePos.y -= 16;
                                
                                stoneJigglePos.y +=
                                    getXYFractal( stoneJigglePos.x,
                                                  stoneJigglePos.y,
                                                  culvertFractalRoughness, 
                                                  culvertFractalScale ) * 
                                    culvertFractalAmp;

                                drawSprite( getSprite( stoneSpriteID ), 
                                            stoneJigglePos  );
                                }
                            }
                        
                        }
                    }
                }
            }
        }
    

    if( showFPS ) startCountingSpritePixelsDrawn();

    double hugR = CELL_D * 0.6;

    // draw floors on top of biome
    for( int y=yEnd; y>=yStart; y-- ) {
        
        int worldY = y + mMapOffsetY - mMapD / 2;

        int screenY = CELL_D * worldY;        

        for( int x=xStart; x<=xEnd; x++ ) {
            
            int worldX = x + mMapOffsetX - mMapD / 2;


            int mapI = y * mMapD + x;

            int oID = mMapFloors[mapI];

            
            int screenX = CELL_D * worldX;
            
            doublePair pos = { (double)screenX, (double)screenY };


            char drawHuggingFloor = false;
            
            // for main floor, and left and right hugging floor
            // 0 to skip a pass
            int passIDs[3] = { 0, 0, 0 };
            
            if( oID > 0 ) {
                passIDs[0] = oID;
                }
            


            if( oID <= 0) {

                
                int cellOID = mMap[mapI];
                
                if( cellOID > 0 && getObject( cellOID )->floorHugging ) {
                    
                    if( x > 0 && mMapFloors[ mapI - 1 ] > 0 ) {
                        // floor to our left
                        passIDs[1] = mMapFloors[ mapI - 1 ];
                        drawHuggingFloor = true;
                        }
                    
                    if( x < mMapD - 1 && mMapFloors[ mapI + 1 ] > 0 ) {
                        // floor to our right
                        passIDs[2] = mMapFloors[ mapI + 1 ];
                        drawHuggingFloor = true;
                        
                        }
                    }
                

                if( ! drawHuggingFloor ) {
                    continue;
                    }
                }
            
            
                    
            int oldFrameCount = 
                mMapFloorAnimationFrameCount[ mapI ];
            
            if( ! mapPullMode ) {
                mMapFloorAnimationFrameCount[ mapI ] ++;
                }



            for( int p=0; p<3; p++ ) {
                if( passIDs[p] == 0 ) {
                    continue;
                    }
                
                oID = passIDs[p];
            
                if( p > 0 ) {    
                    setDrawColor( 1, 1, 1, 1 );
                    startAddingToStencil( false, true );
                    }
                
                if( p == 1 ) {    
                    drawRect( pos.x - hugR, pos.y + hugR, 
                              pos.x, pos.y - hugR );
                    }
                else if( p == 2 ) {
                        
                    drawRect( pos.x, pos.y + hugR, 
                              pos.x + hugR, pos.y - hugR );
                    }

                if( p > 0 ) {
                    startDrawingThroughStencil();
                    }
                

            
                if( !mapPullMode ) {                    
                    handleAnimSound( -1, oID, 0, ground, oldFrameCount, 
                                     mMapFloorAnimationFrameCount[ mapI ],
                                     (double)screenX / CELL_D,
                                     (double)screenY / CELL_D );
                    }
            
                double timeVal = frameRateFactor * 
                    mMapFloorAnimationFrameCount[ mapI ] / 60.0;
                

                if( p > 0 ) {
                    // floor hugging pass
                    
                    int numLayers = getObject( oID )->numSprites;
                    
                    if( numLayers > 1 ) {    
                        // draw all but top layer of floor
                        setAnimLayerCutoff( numLayers - 1 );
                        }
                    }
                
                
                char used;
                drawObjectAnim( oID, 2, 
                                ground, timeVal,
                                0,
                                ground, 
                                timeVal,
                                timeVal,
                                &used,
                                ground,
                                ground,
                                pos, 0,
                                false,
                                false, -1,
                                false, false, false,
                                getEmptyClothingSet(), NULL );

                if( p > 0 ) {
                    stopStencil();
                    }
                }
            
            if( passIDs[1] != passIDs[2] ) {
                setDrawColor( 1, 1, 1, 1 );
                pos.y += 10;
                drawSprite( mFloorSplitSprite, pos );
                }
            }
        }
    
    if( showFPS ) runningPixelCount += endCountingSpritePixelsDrawn();



    // draw overlay evenly over all floors and biomes
    doublePair groundCenterPos;

    int groundWTile = getSpriteWidth( mGroundOverlaySprite[0] );
    int groundHTile = getSpriteHeight( mGroundOverlaySprite[0] );
    
    int groundW = groundWTile * 2;
    int groundH = groundHTile * 2;

    groundCenterPos.x = lrint( lastScreenViewCenter.x / groundW ) * groundW;
    groundCenterPos.y = lrint( lastScreenViewCenter.y / groundH ) * groundH;

    toggleMultiplicativeBlend( true );
    
    // use this to lighten ground overlay
    toggleAdditiveTextureColoring( true );
    setDrawColor( multAmount, multAmount, multAmount, 1 );
    
    for( int y=-1; y<=1; y++ ) {

        doublePair pos = groundCenterPos;

        pos.y = groundCenterPos.y + y * groundH;

        for( int x=-1; x<=1; x++ ) {

            pos.x = groundCenterPos.x + x * groundW;
            
            if( drawMult ) {
                for( int t=0; t<4; t++ ) {
                    
                    doublePair posTile = pos;
                    if( t % 2 == 0 ) {
                        posTile.x -= groundWTile / 2;
                        }
                    else {
                        posTile.x += groundWTile / 2;
                        }
                    
                    if( t < 2 ) {
                        posTile.y += groundHTile / 2;
                        }
                    else {
                        posTile.y -= groundHTile / 2;
                        }
                    
                    drawSprite( mGroundOverlaySprite[t], posTile );
                    }
                }            
            }
        }
    toggleAdditiveTextureColoring( false );
    toggleMultiplicativeBlend( false );


    toggleAdditiveBlend( true );
    
    // use this to lighten ground overlay
    //toggleAdditiveTextureColoring( true );
    setDrawColor( 1, 1, 1, addAmount );
    
    for( int y=-1; y<=1; y++ ) {

        doublePair pos = groundCenterPos;
        
        pos.x += 512;
        pos.y += 512;

        pos.y = groundCenterPos.y + y * groundH;

        for( int x=-1; x<=1; x++ ) {

            pos.x = groundCenterPos.x + x * groundW;

            if( drawAdd )  {
                for( int t=0; t<4; t++ ) {
                    
                    doublePair posTile = pos;
                    if( t % 2 == 0 ) {
                        posTile.x -= groundWTile / 2;
                        }
                    else {
                        posTile.x += groundWTile / 2;
                        }
                    
                    if( t < 2 ) {
                        posTile.y += groundHTile / 2;
                        }
                    else {
                        posTile.y -= groundHTile / 2;
                        }
                    
                    drawSprite( mGroundOverlaySprite[t], posTile );
                    }
                }
            }
        }
    //toggleAdditiveTextureColoring( false );
    toggleAdditiveBlend( false );
    

    LiveObject *ourLiveObject = getOurLiveObject();
    
    if( ourLiveObject != NULL &&
        ( mCurrentHintTargetObject[0] > 0 ||
          mCurrentHintTargetObject[1] > 0 ) ) {

        int actualWatchTargets[2] = { 0, 0 };
        
        int heldID = getObjectParent( ourLiveObject->holdingID );            

        char hit = false;

        // are we holding the target of our current hint?
        // if so, hide hint arrows
        if( heldID > 0 &&
            mLastHintSortedList.size() > mCurrentHintIndex ) {    
            
            TransRecord *t = 
                mLastHintSortedList.getElementDirect( mCurrentHintIndex );
            
            char holdingActor = false;
            char holdingTarget = false;
            
            int heldParent = getObjectParent( heldID );

            if( t->actor > 0 &&
                getObjectParent( t->actor ) == heldParent ) {
                holdingActor = true;
                }
            if( t->target > 0 &&
                getObjectParent( t->target ) == heldParent ) {
                holdingTarget = true;
                }
            

            
            if( t->newActor > 0 && 
                t->newActor == heldID  && ! holdingActor ) {
                hit = true;
                }
            else if( t->newTarget > 0 && 
                     t->newTarget == heldID && ! holdingTarget ) {
                hit = true;
                }
            }
        


        if( ! hit ) {
            // remove any of our held object from the list
            // so that we don't show a bouncing arrow on the ground
            // for something that we're currently holding

            char forceShow = false;
            if( mCurrentHintTargetObject[0] == mCurrentHintTargetObject[1] ) {
                // both are the same, need to use A on A for result
                // thus, even if holding A, show arrow pointing at neares
                // other A
                forceShow = true;
                }
            
            for( int i=0; i<2; i++ ) {
                
                if( forceShow || heldID != mCurrentHintTargetObject[i] ) {
                    actualWatchTargets[i] = mCurrentHintTargetObject[i];
                    }
                }
            }
        
        
        startWatchForClosestObjectDraw( actualWatchTargets,
                                        mult( ourLiveObject->currentPos,
                                              CELL_D ) );
        }



    if( showFPS ) startCountingSpritePixelsDrawn();

    
    float maxFullCellFade = 0.5;
    float maxEmptyCellFade = 0.75;


    if( mShowHighlights 
        &&
        mCurMouseOverCellFade > 0 
        &&
        mCurMouseOverCell.x >= 0 && mCurMouseOverCell.x < mMapD
        &&
        mCurMouseOverCell.y >= 0 && mCurMouseOverCell.y < mMapD ) {
        
        int worldY = mCurMouseOverCell.y + mMapOffsetY - mMapD / 2;
        
        int screenY = CELL_D * worldY;
        
        int screenX = 
            CELL_D * ( mCurMouseOverCell.x + mMapOffsetX - mMapD / 2 );        
        
        int mapI = mCurMouseOverCell.y * mMapD + mCurMouseOverCell.x;
        
        int id = mMap[mapI];
        
        float fill = 0;
        float border = 1;        
        char drawFill = false;
        float maxFade = maxEmptyCellFade;
        
        if( id > 0 ) {
            fill = 1;
            border = 0;
            drawFill = true;
            maxFade = maxFullCellFade;
            }


        doublePair cellPos = { (double)screenX, (double)screenY };
            
        cellPos.x += 2;

        if( drawFill ) {    
            setDrawColor( fill, fill, fill, maxFade * mCurMouseOverCellFade );
            drawSprite( mCellFillSprite, cellPos );
            }
        
        
        setDrawColor( border, border, border,
                      0.75  * maxFade * mCurMouseOverCellFade );
        drawSprite( mCellBorderSprite, cellPos );        
        }

    
    if( mShowHighlights )
    for( int i=0; i<mPrevMouseOverCells.size(); i++ ) {
        float fade = mPrevMouseOverCellFades.getElementDirect( i );
        
        if( fade <= 0 ) {
            continue;
            }

        GridPos prev = mPrevMouseOverCells.getElementDirect( i );
        
        if( prev.x < 0 || prev.x >= mMapD
            ||
            prev.y < 0 || prev.y >= mMapD ) {
            
            continue;
            }
        
            
        int worldY = prev.y + mMapOffsetY - mMapD / 2;
        
        int screenY = CELL_D * worldY;
        
        int screenX = 
            CELL_D * ( prev.x + mMapOffsetX - mMapD / 2 );        

        
        int mapI = prev.y * mMapD + prev.x;
        
        int id = mMap[mapI];
        
        float fill = 0;
        float border = 1;
        char drawFill = false;
        float maxFade = maxEmptyCellFade;
                
        if( id > 0 ) {
            fill = 1;
            border = 0;
            drawFill = true;
            maxFade = maxFullCellFade;
            }

            
        doublePair cellPos = { (double)screenX, (double)screenY };
            
        cellPos.x += 2;
        
        if( drawFill ) {    
            setDrawColor( fill, fill, fill, maxFade * fade );
            drawSprite( mCellFillSprite, cellPos );
            }
        
        setDrawColor( border, border, border, 0.75 * maxFade * fade );
        drawSprite( mCellBorderSprite, cellPos );
        }




    if( mShowHighlights )
    for( int i=0; i<mPrevMouseClickCells.size(); i++ ) {
        float fade = mPrevMouseClickCellFades.getElementDirect( i );
        
        if( fade <= 0 ) {
            continue;
            }

        GridPos prev = mPrevMouseClickCells.getElementDirect( i );
        
        if( prev.x < 0 || prev.x >= mMapD
            ||
            prev.y < 0 || prev.y >= mMapD ) {
            
            continue;
            }
        
            
        int worldY = prev.y + mMapOffsetY - mMapD / 2;
        
        int screenY = CELL_D * worldY;
        
        int screenX = 
            CELL_D * ( prev.x + mMapOffsetX - mMapD / 2 );
        
        float border = 1;
        float maxFade = maxEmptyCellFade;
        
            
        doublePair cellPos = { (double)screenX, (double)screenY };
            
        cellPos.x += 2;
        
        setDrawColor( border, border, border, 0.75 * maxFade * fade );
        drawSprite( mCellBorderSprite, cellPos );
        }
    
    
    //int worldXStart = xStart + mMapOffsetX - mMapD / 2;
    //int worldXEnd = xEnd + mMapOffsetX - mMapD / 2;

    //int worldYStart = xStart + mMapOffsetY - mMapD / 2;
    //int worldYEnd = xEnd + mMapOffsetY - mMapD / 2;


    // capture pointers to objects that are speaking and visible on 
    // screen

    // draw speech on top of everything at end

    SimpleVector<LiveObject *> speakers;
    SimpleVector<doublePair> speakersPos;


    // draw long path for our character
    
    if( ourLiveObject != NULL ) {
        
        if( ourLiveObject->currentPos.x != ourLiveObject->xd 
            || ourLiveObject->currentPos.y != ourLiveObject->yd ) {
            
            if( ourLiveObject->pathToDest != NULL &&
                ourLiveObject->shouldDrawPathMarks &&
                mShowHighlights ) {
                // highlight path

                JenkinsRandomSource pathRand( 340930281 );
                
                GridPos pathSpot = ourLiveObject->pathToDest[ 0 ];
                

                GridPos endGrid = 
                    ourLiveObject->pathToDest[ ourLiveObject->pathLength - 1 ];
                
                doublePair endPos;
                endPos.x = endGrid.x * CELL_D;
                endPos.y = endGrid.y * CELL_D;
                

                doublePair playerPos = mult( ourLiveObject->currentPos,
                                             CELL_D );
                
                double distFromEnd = distance( playerPos, endPos );
                
                float endFade = 1.0f;
                

                if( distFromEnd < 2 * CELL_D ) {
                    endFade = distFromEnd / ( 2 * CELL_D );
                    }


                doublePair curPos;
                
                curPos.x = pathSpot.x * CELL_D;
                curPos.y = pathSpot.y * CELL_D;


                GridPos pathSpotB = ourLiveObject->pathToDest[ 1 ];
                    

                doublePair nextPosB;
                
                nextPosB.x = pathSpotB.x * CELL_D;
                nextPosB.y = pathSpotB.y * CELL_D;
                
                
                doublePair curDir = normalize( sub( nextPosB, curPos ) );
                

                double turnFactor = .25;
                
                int numStepsSinceDrawn = 0;
                int drawOnStep = 6;
                

                for( int p=1; p< ourLiveObject->pathLength; p++ ) {
                
                    
                    GridPos pathSpotB = ourLiveObject->pathToDest[ p ];
                    

                    doublePair nextPos;
                
                    nextPos.x = pathSpotB.x * CELL_D;
                    nextPos.y = pathSpotB.y * CELL_D;
                    
                    int closeDist = 60;
                    
                    if( p == ourLiveObject->pathLength - 1 ) {
                        closeDist = 20;
                        }
                    
                    
                    while( distance( curPos, nextPos ) > closeDist  ) {
    
                        doublePair dir = normalize( sub( nextPos, curPos ) );

                        
                        if( dot( dir, curDir ) >= 0 ) {
                            curDir = 
                                normalize( 
                                    add( curDir, 
                                         mult( dir, turnFactor ) ) );
                        
                            }
                        else {
                            // path doubles back on itself
                            // smooth turning never resolves here
                            // just make a sharp point in path instead
                            curDir = dir;
                            }
                        
                        curPos = add( curPos,
                                      mult( curDir, 6 ) );
                        
                        setDrawColor( 0, 0, 0, 
                                      ourLiveObject->pathMarkFade * endFade );
                        

                        doublePair drawPos = curPos;
                        
                        if( numStepsSinceDrawn == 0 ) {
                            
                            drawSprite( mPathMarkSprite, drawPos, 1.0, 
                                    -angle( curDir ) / ( 2 * M_PI ) + .25  );
                            }
                        
                        numStepsSinceDrawn ++;
                        if( numStepsSinceDrawn == drawOnStep ) {
                            numStepsSinceDrawn = 0;
                            }
                        }
                    }
                
                if( ourLiveObject->pathMarkFade < 1 ) {
                    ourLiveObject->pathMarkFade += 0.1 * frameRateFactor;
                    
                    if( ourLiveObject->pathMarkFade > 1 ) {
                        ourLiveObject->pathMarkFade = 1;
                        }
                    }
                }
            }
        else {
            ourLiveObject->pathMarkFade = 0;
            }
        }
    

    // FIXME:  skip these that are off screen
    // of course, we may not end up drawing paths on screen anyway
    // (probably only our own destination), so don't fix this for now

    // draw paths and destinations under everything
    
    // debug overlay
    if( false )
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );


        if( o->currentPos.x != o->xd || o->currentPos.y != o->yd ) {
            // destination
            
            char *string = autoSprintf( "[%c]", o->displayChar );
        
            doublePair pos;
            pos.x = o->xd * CELL_D;
            pos.y = o->yd * CELL_D;
        
            setDrawColor( 1, 0, 0, 1 );
            mainFont->drawString( string, 
                                  pos, alignCenter );
            delete [] string;


            if( o->pathToDest != NULL ) {
                // highlight path
                
                for( int p=0; p< o->pathLength; p++ ) {
                    GridPos pathSpot = o->pathToDest[ p ];
                    
                    pos.x = pathSpot.x * CELL_D;
                    pos.y = pathSpot.y * CELL_D;

                    setDrawColor( 1, 1, 0, 1 );
                    mainFont->drawString( "P", 
                                          pos, alignCenter );
                    }

                // draw waypoint
                
                setDrawColor( 0, 1, 1, 1 );
                pos.x = o->waypointX * CELL_D;
                pos.y = o->waypointY * CELL_D;
                mainFont->drawString( "W", 
                                      pos, alignCenter );
                    
                }
            else {
                pos.x = o->closestDestIfPathFailedX * CELL_D;
                pos.y = o->closestDestIfPathFailedY * CELL_D;
                
                setDrawColor( 1, 0, 1, 1 );
                mainFont->drawString( "P", 
                                      pos, alignCenter );
                }
            
            
            }
        }

    
    

    // reset flags so that only drawn objects get flagged
    for( int i=0; i<gameObjects.size(); i++ ) {
        gameObjects.getElement( i )->onScreen = false;
        }
    
    int numMoving = 0;
    int movingObjectsIndices[ MAP_NUM_CELLS ];
    
    // prevent double-drawing when cell goes from moving to non-moving
    // mid-draw
    char cellDrawn[ MAP_NUM_CELLS ];
    
    memset( cellDrawn, false, MAP_NUM_CELLS );
    
    for( int y=0; y<mMapD; y++ ) {
        for( int x=0; x<mMapD; x++ ) {
            int mapI = y * mMapD + x;
            
            if( mMap[ mapI ] > 0 &&
                mMapMoveSpeeds[ mapI ] > 0 ) {
                
                movingObjectsIndices[ numMoving ] = mapI;

                numMoving++;
                }
            }
        }
    

    for( int y=yEnd; y>=yStart; y-- ) {
        
        int worldY = y + mMapOffsetY - mMapD / 2;

        int screenY = CELL_D * worldY;
        

        // draw marked objects behind everything else, including players
        
        for( int x=xStart; x<=xEnd; x++ ) {
            
            int worldX = x + mMapOffsetX - mMapD / 2;


            int mapI = y * mMapD + x;

            if( cellDrawn[mapI] ) {
                continue;
                }

            int screenX = CELL_D * worldX;
            
            if( mMap[ mapI ] > 0 && 
                mMapMoveSpeeds[ mapI ] == 0 ) {
               
                ObjectRecord *o = getObject( mMap[ mapI ] );

                if( o->drawBehindPlayer ) {
                    drawMapCell( mapI, screenX, screenY );
                    cellDrawn[mapI] = true;
                    }
                else if( o->anySpritesBehindPlayer ) {
                    
                    // draw only behind layers now
                    prepareToSkipSprites( o, true );
                    drawMapCell( mapI, screenX, screenY, false, 
                                 // no time effects, because we'll draw
                                 // again later
                                 true );
                    restoreSkipDrawing( o );
                    }
                
                }

            

            /*
              // debugging grid

            doublePair cellPos = { (double)screenX, (double)screenY };

            if( mMap[ mapI ] < 0 ) {
                setDrawColor( 0, 0, 0, 0.5 );
                }
            else {
                if( lrint( abs(worldY) ) % 2 == 0 ) {
                    if( lrint( abs(worldX) ) % 2 == 1 ) {
                        setDrawColor( 1, 1, 1, 0.25 );
                        }
                    else {
                        setDrawColor( 0, 0, 0, 0.25 );
                        }
                    }
                else {
                    if( lrint( abs(worldX) ) % 2 == 1 ) {
                        setDrawColor( 0, 0, 0, 0.25 );
                        }
                    else {
                        setDrawColor( 1, 1, 1, 0.25 );
                        }
                    }
                
                }
            
            doublePair cellPos = { (double)screenX, (double)screenY };
            drawSquare( cellPos, CELL_D / 2 );
            
            FloatColor c = getDrawColor();

            c.r = 1 - c.r;
            c.g = 1 - c.g;
            c.b = 1 - c.b;
            c.a = .75;
            
            setDrawColor( c );
            
            char *xString =  autoSprintf( "x:%d", worldX );
            char *yString =  autoSprintf( "y:%d", worldY );
            
            doublePair xDrawPos = cellPos;
            doublePair yDrawPos = cellPos;
            
            xDrawPos.y += CELL_D / 6;
            yDrawPos.y -= CELL_D / 6;
            
            xDrawPos.x -= CELL_D / 3;
            yDrawPos.x -= CELL_D / 3;
            

            mainFont->drawString( xString, xDrawPos, alignLeft );
            mainFont->drawString( yString, yDrawPos, alignLeft );
            
            delete [] yString;
            delete [] xString;
            */
            }

        
        SimpleVector<ObjectAnimPack> heldToDrawOnTop;

        // sorted queue of players and moving objects in this row
        // build it, then draw them in sorted order
        MinPriorityQueue<DrawOrderRecord> drawQueue;

        // draw players behind the objects in this row

        // run this loop twice, once for
        // adults, and then afterward for recently-dropped babies
        // that are still sliding into place (so that they remain
        // visibly on top of the adult who dropped them
        for( int d=0; d<2; d++ )
        for( int x=xStart; x<=xEnd; x++ ) {
            int worldX = x + mMapOffsetX - mMapD / 2;


            for( int i=0; i<gameObjects.size(); i++ ) {
        
                LiveObject *o = gameObjects.getElement( i );
                
                if( o->heldByAdultID != -1 ) {
                    // held by someone else, don't draw now
                    continue;
                    }

                if( d == 0 &&
                    ( o->heldByDropOffset.x != 0 ||
                      o->heldByDropOffset.y != 0 ) ) {
                    // recently dropped baby, skip
                    continue;
                    }
                else if( d == 1 &&
                         o->heldByDropOffset.x == 0 &&
                         o->heldByDropOffset.y == 0 ) {
                    // not a recently-dropped baby, skip
                    continue;
                    }
                
                
                int oX = o->xd;
                int oY = o->yd;
                
                if( o->currentSpeed != 0 ) {
                    oX = lrint( o->currentPos.x );
                    oY = lrint( o->currentPos.y - 0.20 );
                    }
                
                
                if( oY == worldY && oX == worldX ) {
                    
                    // there's a player here, insert into sort queue
                    
                    DrawOrderRecord drawRec;
                    drawRec.person = true;
                    drawRec.personO = o;
                    

                    double depth = 0 - o->currentPos.y;
                    
                    if( lrint( depth ) - depth == 0 ) {
                        // break ties (co-occupied cells) by drawing 
                        // younger players in front
                        // (so that babies born appear in front of 
                        //  their mothers)

                        // vary by a tiny amount, so we don't change
                        // the way they are sorted relative to other objects
                        depth += ( 60.0 - o->age ) / 6000.0;
                        }
                    
                    drawQueue.insert( drawRec, depth );
                    }
                }
            }
        
        // now sort moving objects that fall in this row
        for( int i=0; i<numMoving; i++ ) {
            
            int mapI = movingObjectsIndices[i];
            
            if( cellDrawn[mapI] ) {
                continue;
                }
            

            int oX = mapI % mMapD;
            int oY = mapI / mMapD;
            
            int movingX = lrint( oX + mMapMoveOffsets[mapI].x );

            double movingTrueCellY = oY + mMapMoveOffsets[mapI].y;
            
            double movingTrueY =  movingTrueCellY - 0.1;
            
            int movingCellY = lrint( movingTrueCellY - 0.40 );

            if( movingCellY == y && movingX >= xStart && movingX <= xEnd ) {
                
                int movingScreenX = CELL_D * ( oX + mMapOffsetX - mMapD / 2 );
                int movingScreenY = CELL_D * ( oY + mMapOffsetY - mMapD / 2 );
                
                double worldMovingY =  movingTrueY + mMapOffsetY - mMapD / 2;
                
                
                //drawMapCell( mapI, movingScreenX, movingScreenY );
                
                // add to depth sorting queue
                DrawOrderRecord drawRec;
                drawRec.person = false;
                drawRec.extraMovingObj = false;
                drawRec.mapI = mapI;
                drawRec.screenX = movingScreenX;
                drawRec.screenY = movingScreenY;
                drawQueue.insert( drawRec, 0 - worldMovingY );
                
                cellDrawn[mapI] = true;
                }
            }



        

        // now sort extra moving objects that fall in this row
        
        int worldXStart = xStart + mMapOffsetX - mMapD / 2;
        int worldXEnd = xEnd + mMapOffsetX - mMapD / 2;
        
        for( int i=0; i<mMapExtraMovingObjects.size(); i++ ) {
            
            GridPos movingWorldPos = 
                mMapExtraMovingObjectsDestWorldPos.getElementDirect( i );

            ExtraMapObject *extraO = 
                mMapExtraMovingObjects.getElement( i );

            
            int movingX = lrint( movingWorldPos.x + extraO->moveOffset.x );

            double movingTrueCellY = movingWorldPos.y + extraO->moveOffset.y;
            
            double movingTrueY =  movingTrueCellY - 0.2;
            
            int movingCellY = lrint( movingTrueCellY - 0.50 );

            if( movingCellY == worldY && 
                movingX >= worldXStart && 
                movingX <= worldXEnd ) {
                
                int mapX = movingWorldPos.x - mMapOffsetX + mMapD / 2;
                int mapY = movingWorldPos.y - mMapOffsetY + mMapD / 2;
                    
                int mapI = mapY * mMapD + mapX;


                int movingScreenX = CELL_D * movingWorldPos.x;
                int movingScreenY = CELL_D * movingWorldPos.y;
                
                double worldMovingY =  movingTrueY + mMapOffsetY - mMapD / 2;
                
                
                //drawMapCell( mapI, movingScreenX, movingScreenY );
                
                // add to depth sorting queue
                DrawOrderRecord drawRec;
                drawRec.person = false;
                drawRec.extraMovingObj = true;
                drawRec.extraMovingIndex = i;
                drawRec.mapI = mapI;
                drawRec.screenX = movingScreenX;
                drawRec.screenY = movingScreenY;
                
                drawQueue.insert( drawRec, 0 - worldMovingY );
                }
            }




        
        // now move through queue in order, drawing
        int numQueued = drawQueue.size();
        for( int q=0; q<numQueued; q++ ) {
            DrawOrderRecord drawRec = drawQueue.removeMin();
            
            if( drawRec.person ) {
                LiveObject *o = drawRec.personO;
                
                ignoreWatchedObjectDraw( true );

                ObjectAnimPack heldPack =
                    drawLiveObject( o, &speakers, &speakersPos );
                
                if( heldPack.inObjectID != -1 ) {
                    // holding something, not drawn yet
                    
                    if( o->holdingID < 0 ) {
                        LiveObject *babyO = getLiveObject( - o->holdingID );
                        if( babyO != NULL 
                            && babyO->dying && babyO->holdingID > 0  ) {
                            // baby still holding something while dying,
                            // likely a wound
                            // add to pack to draw it on top of baby
                            heldPack.additionalHeldID =
                                babyO->holdingID;
                            }
                        }


                    if( ! o->heldPosOverride ) {
                        // not sliding into place
                        // draw it now
                        
                        char skippingSome = false;
                        if( heldPack.inObjectID > 0 &&
                            getObject( heldPack.inObjectID )->rideable &&
                            getObject( heldPack.inObjectID )->
                            anySpritesBehindPlayer ) {
                            skippingSome = true;
                            }
                        if( skippingSome ) {
                            prepareToSkipSprites( 
                                getObject( heldPack.inObjectID ),
                                false );
                            }

                        if( heldPack.inObjectID > 0 ) {
                            ObjectRecord *heldO = 
                                getObject( heldPack.inObjectID );
                            
                            if( heldO->toolSetIndex != -1 &&
                                ! o->heldLearned ) {
                                // unleared tool


                                if( heldO->heldInHand ) {
                                    // rotate 180
                                    
                                    doublePair newCenterOffset = 
                                        getObjectCenterOffset( heldO );
                                    
                                    if( heldPack.inFlipH ) {
                                        newCenterOffset.x *= -1;
                                        }
                                    newCenterOffset = 
                                        rotate( newCenterOffset,
                                                - heldPack.inRot * 2 *  M_PI );
                                    
                                    heldPack.inPos =
                                        add( heldPack.inPos,
                                             mult( newCenterOffset, 2 ) );
                                    
                                    
                                    doublePair newHeldOffset = heldO->heldOffset;
                                    
                                    if( heldPack.inFlipH ) {
                                        newHeldOffset.x *= -1;
                                        }
                                    
                                    // add a small tweak here, because
                                    // held offset is relative to wrist of
                                    // character, not center of hand
                                    newHeldOffset.y += 3;
                                    
                                    newHeldOffset =
                                        rotate( newHeldOffset,
                                                - heldPack.inRot * 2 *  M_PI );
                                    
                                    heldPack.inPos =
                                        sub( heldPack.inPos, 
                                             mult( newHeldOffset, 2 ) );
                                    
                                    
                                    heldPack.inRot += .5;
                                    }
                                }
                            }


                        drawObjectAnim( heldPack );
                        if( skippingSome ) {
                            restoreSkipDrawing( 
                                getObject( heldPack.inObjectID ) );
                            }
                        }
                    else {
                        heldToDrawOnTop.push_back( heldPack );
                        }
                    }
                ignoreWatchedObjectDraw( false );
                }
            else if( drawRec.extraMovingObj ) {
                ignoreWatchedObjectDraw( true );
                
                ExtraMapObject *mO = mMapExtraMovingObjects.getElement(
                    drawRec.extraMovingIndex );
                
                // hold non-moving dest object
                ExtraMapObject curO = copyFromMap( drawRec.mapI );
                
                // temporarily insert extra object for drawing
                putInMap( drawRec.mapI, mO );

                drawMapCell( drawRec.mapI, drawRec.screenX, drawRec.screenY );

                // copy back out of map to preserve effects of draw call
                // (frame count update, etc.)
                *mO = copyFromMap( drawRec.mapI );

                // put original one back
                putInMap( drawRec.mapI, &curO );
                
                ignoreWatchedObjectDraw( false );
                }
            else {
                drawMapCell( drawRec.mapI, drawRec.screenX, drawRec.screenY );
                }
            }
        

        // now draw non-behind-marked map objects in this row
        // OVER the player objects in this row (so that pick up and set down
        // looks correct, and so players are behind all same-row objects)

        // we determine what counts as a wall through wallLayer flag

        // first permanent, non-wall objects
        for( int x=xStart; x<=xEnd; x++ ) {
            int mapI = y * mMapD + x;
            
            if( cellDrawn[ mapI ] ) {
                continue;
                }
            
            int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );


            if( mMap[ mapI ] > 0 ) {
                ObjectRecord *o = getObject( mMap[ mapI ] );
                
                if( ! o->drawBehindPlayer &&
                    ! o->wallLayer &&
                    o->permanent &&
                    mMapMoveSpeeds[ mapI ] == 0 ) {
                    
                    if( o->anySpritesBehindPlayer ) {
                        // draw only non-behind layers now
                        prepareToSkipSprites( o, false );
                        }                    

                    drawMapCell( mapI, screenX, screenY );

                    if( o->anySpritesBehindPlayer ) {
                        restoreSkipDrawing( o );
                        }

                    cellDrawn[ mapI ] = true;
                    }
                }
            }


        // then non-permanent, non-wall objects
        for( int x=xStart; x<=xEnd; x++ ) {
            int mapI = y * mMapD + x;
            
            if( cellDrawn[ mapI ] ) {
                continue;
                }
            
            int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );


            if( mMap[ mapI ] > 0 ) {
                ObjectRecord *o = getObject( mMap[ mapI ] );
                
                if( ! o->drawBehindPlayer &&
                    ! o->wallLayer &&
                    ! o->permanent &&
                    mMapMoveSpeeds[ mapI ] == 0 ) {
                    
                    if( o->anySpritesBehindPlayer ) {
                        // draw only non-behind layers now
                        prepareToSkipSprites( o, false );
                        }                    

                    drawMapCell( mapI, screenX, screenY );

                    if( o->anySpritesBehindPlayer ) {
                        restoreSkipDrawing( o );
                        }

                    cellDrawn[ mapI ] = true;
                    }
                }
            }


        // now draw held flying objects on top of objects in this row
        // but still behind walls in this row
        ignoreWatchedObjectDraw( true );
        for( int i=0; i<heldToDrawOnTop.size(); i++ ) {
            drawObjectAnim( heldToDrawOnTop.getElementDirect( i ) );
            }
        ignoreWatchedObjectDraw( false );



        // then permanent, non-container, wall objects
        for( int x=xStart; x<=xEnd; x++ ) {
            int mapI = y * mMapD + x;
            
            if( cellDrawn[ mapI ] ) {
                continue;
                }
            
            int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );


            if( mMap[ mapI ] > 0 ) {
                ObjectRecord *o = getObject( mMap[ mapI ] );
                
                if( ! o->drawBehindPlayer &&
                    o->wallLayer &&
                    o->permanent &&
                    ! o->frontWall &&
                    mMapMoveSpeeds[ mapI ] == 0 ) {
                
                    if( o->anySpritesBehindPlayer ) {
                        // draw only non-behind layers now
                        prepareToSkipSprites( o, false );
                        }                    

                    drawMapCell( mapI, screenX, screenY );

                    if( o->anySpritesBehindPlayer ) {
                        restoreSkipDrawing( o );
                        }

                    cellDrawn[ mapI ] = true;
                    }
                }
            }

        // then permanent, container, wall objects (walls with signs)
        for( int x=xStart; x<=xEnd; x++ ) {
            int mapI = y * mMapD + x;
            
            if( cellDrawn[ mapI ] ) {
                continue;
                }
            
            int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );


            if( mMap[ mapI ] > 0 ) {
                ObjectRecord *o = getObject( mMap[ mapI ] );
                
                if( ! o->drawBehindPlayer &&
                    o->wallLayer &&
                    o->permanent &&
                    o->frontWall &&
                    mMapMoveSpeeds[ mapI ] == 0 ) {
                
                    if( o->anySpritesBehindPlayer ) {
                        // draw only non-behind layers now
                        prepareToSkipSprites( o, false );
                        }                    

                    drawMapCell( mapI, screenX, screenY );

                    if( o->anySpritesBehindPlayer ) {
                        restoreSkipDrawing( o );
                        }

                    cellDrawn[ mapI ] = true;
                    }
                }
            }
        } // end loop over rows on screen
    

    // finally, draw any highlighted our-placements
    if( ! takingPhoto )
    if( mCurMouseOverID > 0 && ! mCurMouseOverSelf && mCurMouseOverBehind ) {
        int worldY = mCurMouseOverSpot.y + mMapOffsetY - mMapD / 2;
        
        int screenY = CELL_D * worldY;
        
        int mapI = mCurMouseOverSpot.y * mMapD + mCurMouseOverSpot.x;
        int screenX = 
            CELL_D * ( mCurMouseOverSpot.x + mMapOffsetX - mMapD / 2 );
        
        // highlights only
        drawMapCell( mapI, screenX, screenY, true );
        }

    if( ! takingPhoto )
    for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
        if( mPrevMouseOverSpotsBehind.getElementDirect( i ) ) {
                
            GridPos prev = mPrevMouseOverSpots.getElementDirect( i );
            
            int worldY = prev.y + mMapOffsetY - mMapD / 2;
        
            int screenY = CELL_D * worldY;
        
            int mapI = prev.y * mMapD + prev.x;
            int screenX = 
                CELL_D * ( prev.x + mMapOffsetX - mMapD / 2 );
        
            // highlights only
            drawMapCell( mapI, screenX, screenY, true );
            }
        }
    

    char pointerDrawn[2] = { false, false };

    for( int i=0; i<2; i++ ) {
        
        if( ! takingPhoto && mCurrentHintTargetObject[i] > 0 ) {
            // draw pointer to closest hint target object
        
            char drawn = false;
            doublePair targetPos = getClosestObjectDraw( &drawn, i );

        

            if( drawn ) {
            
                // round to closest cell pos
                targetPos.x = CELL_D * lrint( targetPos.x / CELL_D );
                targetPos.y = CELL_D * lrint( targetPos.y / CELL_D );
                
                // move up
                targetPos.y += 64;

                if( !equal( targetPos, mLastHintTargetPos[i] ) ) {
                    // reset bounce when target changes
                    pushOldHintArrow( i );
                    mCurrentHintTargetPointerBounce[i] = 0;
                    mCurrentHintTargetPointerFade[i] = 0;
                    mLastHintTargetPos[i] = targetPos;        
                    }
            
                if( mCurrentHintTargetPointerFade[i] == 0 ) {
                    // invisible
                    // take this opportunity to hard-sync with the other
                    // arrow
                    if( i == 0 ) {
                        mCurrentHintTargetPointerBounce[i] =
                            mCurrentHintTargetPointerBounce[1] +
                            M_PI / 2;
                        }
                    else {
                        mCurrentHintTargetPointerBounce[i] =
                            mCurrentHintTargetPointerBounce[0] +
                            M_PI / 2;
                        }
                    }
                
            
                targetPos.y += 16 * cos( mCurrentHintTargetPointerBounce[i] );
            
                double deltaRate = 6 * frameRateFactor / 60.0; 

                mCurrentHintTargetPointerBounce[i] += deltaRate;
                
                if( mCurrentHintTargetPointerFade[i] < 1 ) {
                    mCurrentHintTargetPointerFade[i] += deltaRate;
                    
                    if( mCurrentHintTargetPointerFade[i] > 1 ) {
                        mCurrentHintTargetPointerFade[i] = 1;
                        }
                    }
                    
                setDrawColor( 1, 1, 1, mCurrentHintTargetPointerFade[i] );

                drawSprite( mHintArrowSprite, targetPos );
            
                pointerDrawn[i] = true;
                }
            }

        if( ! pointerDrawn[i] ) {
            // reset bounce
            pushOldHintArrow( i );
            mCurrentHintTargetPointerBounce[i] = 0;
            mCurrentHintTargetPointerFade[i] = 0;
            mLastHintTargetPos[i].x = 0;
            mLastHintTargetPos[i].y = 0;
            }
        }
    
    if( !takingPhoto ) {
        for( int i=0; i<mOldHintArrows.size(); i++ ) {
            OldHintArrow *h = mOldHintArrows.getElement( i );

            // make sure it hasn't been blocked by reposition of real hint arrow
            if( ( mCurrentHintTargetObject[0] > 0 &&
                  equal( h->pos, mLastHintTargetPos[0] ) )
                ||
                ( mCurrentHintTargetObject[1] > 0 &&
                  equal( h->pos, mLastHintTargetPos[1] ) ) ) {
                
                mOldHintArrows.deleteElement( i );
                i--;
                continue;
                }
            doublePair targetPos = h->pos;
            
            targetPos.y += 16 * cos( h->bounce );
            
            // twice as fast as fade-in
            double deltaRate = 2 * 6 * frameRateFactor / 60.0; 

            h->bounce += deltaRate;
            
            if( h->fade > 0 ) {
                h->fade -= deltaRate;
                }
            
            if( h->fade <= 0 ) {
                mOldHintArrows.deleteElement( i );
                i--;
                continue;
                }
            
            setDrawColor( 1, 1, 1, h->fade );

            drawSprite( mHintArrowSprite, targetPos );
            }
        }
        


    
    if( ! takingPhoto )
    for( int i=0; i<speakers.size(); i++ ) {
        LiveObject *o = speakers.getElementDirect( i );
        
        doublePair pos = speakersPos.getElementDirect( i );
        
        
        doublePair speechPos = pos;

        speechPos.y += 84;

        ObjectRecord *displayObj = getObject( o->displayID );
 

        double age = computeCurrentAge( o );
        
        doublePair headPos = 
            displayObj->spritePos[ getHeadIndex( displayObj, age ) ];
        
        doublePair bodyPos = 
            displayObj->spritePos[ getBodyIndex( displayObj, age ) ];

        doublePair frontFootPos = 
            displayObj->spritePos[ getFrontFootIndex( displayObj, age ) ];
        
        headPos = add( headPos, 
                       getAgeHeadOffset( age, headPos, 
                                         bodyPos, frontFootPos ) );
        headPos = add( headPos,
                       getAgeBodyOffset( age, bodyPos ) );
        
        speechPos.y += headPos.y;
        
        int width = 250;
        int widthLimit = 250;
        
        double fullWidth = 
            handwritingFont->measureString( o->currentSpeech );
        
        if( fullWidth < width ) {
            width = (int)fullWidth;
            }
        
        
        speechPos.x -= width / 2;

        
        drawChalkBackgroundString( speechPos, o->currentSpeech, 
                                   o->speechFade, widthLimit,
                                   o );
        }



    if( ! takingPhoto )
    for( int i=0; i<locationSpeech.size(); i++ ) {
        LocationSpeech *ls = locationSpeech.getElement( i );
        
        doublePair pos = ls->pos;
        
        
        doublePair speechPos = pos;


        speechPos.y += 84;
        
        int width = 250;
        int widthLimit = 250;
        
        double fullWidth = 
            handwritingFont->measureString( ls->speech );
        
        if( fullWidth < width ) {
            width = (int)fullWidth;
            }
        
        speechPos.x -= width / 2;
        
        drawChalkBackgroundString( speechPos, ls->speech, 
                                   ls->fade, widthLimit );
        }
    

    if( showFPS ) runningPixelCount += endCountingSpritePixelsDrawn();

    
    if( ! takingPhoto )
    drawOffScreenSounds();
    
    

    /*
      // for debugging home location

    ClickRecord *home = getHomeLocation();
    
    if( home != NULL ) {
        setDrawColor( 1, 0, 0, 0.5 );
        
        int screenY = CELL_D * home->y;
        
        int screenX = CELL_D * home->x;
        
        doublePair pos = { (double)screenX, (double)screenY };
        
        drawSquare( pos, CELL_D * .45 );

        int startX = CELL_D * ourLiveObject->currentPos.x;
        int startY = CELL_D * ourLiveObject->currentPos.y;
        
        doublePair start = { (double)startX, (double)startY };

        doublePair delta = sub( pos, start );
        
        int numSteps = length( delta ) / CELL_D;

        delta = mult( delta, 1.0 / numSteps );
        
        for( int i=0; i<numSteps; i++ ) {
            
            pos = add( start, mult( delta, i ) );
            
            drawSquare( pos, CELL_D * .25 );
            }
        }
    */
    
    /*
    // for debugging click counts
    for( int i=0; i<NUM_CLICK_RECORDS; i++ ) {
        if( clicks[i].count > 0 ) {
            setDrawColor( 1, 0, 0, 1 );
            
            doublePair pos;
            pos.x = clicks[i].x * CELL_D;
            pos.y = clicks[i].y * CELL_D;
            
            char *string = autoSprintf( "%d", clicks[i].count );
            
            mainFont->drawString( string, pos, alignCenter );
            
            delete [] string;
            }
        }
    */

        
    //doublePair lastChunkCenter = { (double)( CELL_D * mMapOffsetX ), 
    //                               (double)( CELL_D * mMapOffsetY ) };
    
    setDrawColor( 0, 1, 0, 1 );
    
    // debug overlay
    //mainFont->drawString( "X", 
    //                      lastChunkCenter, alignCenter );
    
    




    
    if( false )
    for( int i=0; i<trail.size(); i++ ) {
        doublePair *p = trail.getElement( i );

        setDrawColor( trailColors.getElementDirect( i ) );

        mainFont->drawString( ".", 
                              *p, alignCenter );
        }
        

    setDrawColor( 0, 0, 0, 0.125 );
    
    int screenGridOffsetX = lrint( lastScreenViewCenter.x / CELL_D );
    int screenGridOffsetY = lrint( lastScreenViewCenter.y / CELL_D );
    
    // debug overlay
    if( false )
    for( int y=-5; y<=5; y++ ) {
        for( int x=-8; x<=8; x++ ) {
            
            doublePair pos;
            pos.x = ( x + screenGridOffsetX ) * CELL_D;
            pos.y = ( y + screenGridOffsetY ) * CELL_D;
            
            drawSquare( pos, CELL_D * .45 );
            }
        }    
    
    if( mapPullMode ) {
        
        float progress;
        
        if( ! mapPullCurrentSaved && 
            isLiveObjectSetFullyLoaded( &progress ) ) {
            
            int screenWidth, screenHeight;
            getScreenDimensions( &screenWidth, &screenHeight );
            
            Image *screen = 
                getScreenRegionRaw( 0, 0, screenWidth, screenHeight );

            int startX = lastScreenViewCenter.x - screenW / 2;
            int startY = lastScreenViewCenter.y + screenH / 2;
            startY = -startY;
            double scale = (double)screenWidth / (double)screenW;
            startX = lrint( startX * scale );
            startY = lrint( startY * scale );

            int totalW = mapPullTotalImage->getWidth();
            int totalH = mapPullTotalImage->getHeight();
            
            int totalImStartX = startX + totalW / 2;
            int totalImStartY = startY + totalH / 2;

            double gridCenterOffsetX = ( mapPullEndX + mapPullStartX ) / 2.0;
            double gridCenterOffsetY = ( mapPullEndY + mapPullStartY ) / 2.0;
            
            totalImStartX -= lrint( gridCenterOffsetX * CELL_D  * scale );
            totalImStartY += lrint( gridCenterOffsetY * CELL_D * scale );
            
            //totalImStartY =  totalH - totalImStartY;

            if( totalImStartX >= 0 && totalImStartX < totalW &&
                totalImStartY >= 0 && totalImStartY < totalH ) {
                
                mapPullTotalImage->setSubImage( totalImStartX,
                                                totalImStartY,
                                                screenWidth, screenHeight,
                                                screen );
                numScreensWritten++;
                }
            

            delete screen;
            
            mapPullCurrentSaved = true;
            
            if( mapPullModeFinalImage ) {
                mapPullMode = false;

                writeTGAFile( "mapOut.tga", mapPullTotalImage );
                delete mapPullTotalImage;
                

                // pull over

                // auto-quit
                printf( "Map pull done, auto-quitting game\n" );
                quitGame();
                }
            }
        
        // skip gui
        return;
        }


    // special mode for teaser video
    if( teaserVideo ) {
        //setDrawColor( 1, 1, 1, 1 );
        //drawRect( lastScreenViewCenter, 640, 360 );
        
        // two passes
        // first for arrows, second for labels
        // make sure arrows don't cross labels
        for( int pass=0; pass<2; pass++ )
        for( int y=yEnd; y>=yStart; y-- ) {
        
            int worldY = y + mMapOffsetY - mMapD / 2;
            //printf( "World y = %d\n", worldY );
            
            if( worldY == 1 || worldY == 0 || worldY == -10 ) {

                int xLimit = 0;
                
                if( worldY == -10 ) {
                    xLimit = -20;
                    }

                int screenY = CELL_D * worldY;
        
                
                for( int x=xStart; x<=xEnd; x++ ) {
            
                    int worldX = x + mMapOffsetX - mMapD / 2;

                    if( worldX >= xLimit ) {
                        
                        int mapI = y * mMapD + x;
                    
                        int screenX = CELL_D * worldX;
                        
                        int arrowTipX = screenX;
                        int arrowTipY = screenY;
                        
                        const char *baseDes = NULL;
                        
                        char numbered = false;
                        
                        if( mMap[ mapI ] > 0 ) {
                            ObjectRecord *o = getObject( mMap[mapI] );
                            
                            baseDes = o->description;
                            
                            // don't number natural objects
                            // or objects that occur before the
                            // start of our line
                            // or use dummies (berry bush is picked from
                            // at end of teaser video).
                            numbered = ( o->mapChance == 0 && worldX > 10 
                                         &&
                                         ! o->isUseDummy );
                            }
                        else if( worldX == -20 && worldY == -10 &&
                                 ( gameObjects.size() == 1 ||
                                   gameObjects.size() == 2 ) ) {
                            baseDes = "PLAYER ONE";
                            LiveObject *o =  gameObjects.getElement( 0 );
                            arrowTipX = o->currentPos.x * CELL_D;
                            arrowTipY += 64;
                            }
                        else if( worldX == -10 && worldY == -10 &&
                                 gameObjects.size() == 3 ) {
                            baseDes = "PLAYER TWO";
                            // point to parent location
                            // they pick up baby and keep walking
                            LiveObject *o =  gameObjects.getElement( 0 );
                            arrowTipX = o->currentPos.x * CELL_D;
                            arrowTipY += 64;
                            }
                        
                        
                        if( baseDes != NULL ) {
                            
                            doublePair labelPos;

                            
                            // speeds are center weights picked from
                            // 7/8, 8/8, or 9/8
                            // 8/8 means the label sticks to the x-center
                            // of the screen.
                            // 7/8 means it lags behind the center a bit
                            // 9/8 means it moves faster than the center a bit
                            double centerWeight = 8.0/8.0;
                            

                            // pick a new speed order every 6 tiles
                            double randOrder =
                                getXYRandom( worldX/6, worldY + 300 );
                            
                            int orderOffsets[3] = { 0, 2, 4 };
                            
                            // even distribution among 0,1,2
                            int orderIndex = 
                                lrint( ( randOrder * 2.98 ) - 0.49 );
                            
                            int orderOffset = orderOffsets[ orderIndex ];
                            
                            int worldXOffset = worldX - 11 + orderOffset;

                            if( ( worldX - 11 ) % 2 == 0 ) {
                                
                                if( worldXOffset % 6 == 0 ) {    
                                    centerWeight = 8.0 / 8.0;
                                    }
                                else if( worldXOffset % 6 == 2 ) {
                                    centerWeight = 7.0/8.0;
                                    }
                                else if( worldXOffset % 6 == 4 ) {
                                    centerWeight = 9.0/8.0;
                                    }
                                }
                            else {
                                
                                if( worldXOffset % 6 == 1 ) {
                                    centerWeight = 7.0 / 8.0;
                                    }
                                else if( worldXOffset % 6 == 3 ) {
                                    centerWeight = 8.0/8.0;
                                    }
                                else if( worldXOffset % 6 == 5 ) {
                                    centerWeight = 6.0/8.0;
                                    }
                                }
                            

                            labelPos.x = 
                                (1 - centerWeight ) * screenX + 
                                centerWeight * lastScreenViewCenter.x;
                            
                            char labelAbove = false;

                            double xWiggle = 
                                getXYRandom( worldX, 924 + worldY );
                            
                            labelPos.x += lrint( (xWiggle - 0.5) *  512 );


                            labelPos.y = lastScreenViewCenter.y;

                            double yWiggle = 
                                getXYRandom( worldX, 29 + worldY );
                            
                            labelPos.y += lrint( (yWiggle - 0.5) * 32 );
                            
                            SpriteHandle arrowSprite = mTeaserArrowMedSprite;

                            if( worldY != 0 ||
                                ( worldX > 0 && worldX < 7 ) ) {
                                
                                if( ( worldX - 11 ) % 2 == 0 ) {
                                    labelAbove = true;
                                    labelPos.y += 214;
                                    
                                    if( ( worldX - 11 ) % 6 == 2 ) {
                                        labelPos.y -= 106;
                                        arrowSprite = 
                                            mTeaserArrowVeryShortSprite;
                                        }
                                    else if( ( worldX - 11 ) % 6 == 4 ) {
                                        labelPos.y += 86;
                                        arrowSprite = mTeaserArrowLongSprite;
                                        
                                        // prevent top row from
                                        // going off top of screen
                                        if( labelPos.y - 
                                            lastScreenViewCenter.y
                                            > 224 + 86 ) {
                                            labelPos.y = lastScreenViewCenter.y
                                                + 224 + 86;
                                            }
                                        }
                                    }
                                else {
                                    labelPos.y -= 202;
                                    arrowTipY -= 48;
                                    
                                    if( ( worldX - 11 ) % 6 == 3 ) {
                                        labelPos.y -= 106;
                                        arrowSprite = mTeaserArrowLongSprite;
                                        }
                                    else if( ( worldX - 11 ) % 6 == 5 ) {
                                        labelPos.y += 86;
                                        arrowSprite = mTeaserArrowShortSprite;
                                        }
                                    }        
                                }
                            else {
                                if( worldX % 2 == 0 ) {
                                    labelPos.y = lastScreenViewCenter.y + 224;
                                    labelAbove = true;
                                    }
                                else {
                                    labelPos.y = lastScreenViewCenter.y - 224;
                                    labelAbove = false;
                                    }
                                }
                            
                            // special case loincloth for baby
                            if( worldX == -1 && worldY == -10 ) {
                                labelPos.y = lastScreenViewCenter.y - 224;
                                labelAbove = false;
                                arrowTipY -= 48;
                                }
                            // phonograph sleve
                            if( worldX == 3 && worldY == 0 ) {
                                labelPos.y = lastScreenViewCenter.y - 224;
                                labelAbove = false;
                                arrowTipY -= 48;
                                arrowSprite = mTeaserArrowVeryShortSprite;
                                }
                            // phonograph
                            if( worldX == 4 && worldY == 0 ) {
                                labelPos.y = lastScreenViewCenter.y + 224;
                                labelAbove = true;
                                labelPos.x += 256;
                                arrowTipY += 48;
                                }
                            
                            double fade = 0;
                            
                            if( fabs( lastScreenViewCenter.x - screenX )
                                < 400 ) {
                                
                                if( fabs( lastScreenViewCenter.x - screenX )
                                    < 300 ) {
                                    fade = 1;
                                    }
                                else {
                                    fade = 
                                        ( 400 - fabs( lastScreenViewCenter.x - 
                                                      screenX ) ) / 100.0;
                                    }
                                }
                            
                            if( fade > 0  ) {
                                if( fabs( lastScreenViewCenter.y - screenY )
                                    > 200 ) {
                                    fade = 0;
                                    }
                                else if( 
                                    fabs( lastScreenViewCenter.y - screenY )
                                    > 100 ) {
                                    fade = ( 200 - 
                                             fabs( lastScreenViewCenter.y - 
                                                   screenY ) ) / 100.0;
                                    }
                                }
                            

                            


                            if( pass == 0 ) {
                                double arrowStart = 27;
                                
                                if( labelAbove ) {
                                    arrowStart = -21;
                                    }
                                
                                double w = 11;
                                
                                double deltaX = arrowTipX - labelPos.x;
                                double deltaY = arrowTipY - 
                                    ( labelPos.y + arrowStart );
                                
                                double a = atan( deltaX / deltaY );
                                
                                a = 0.85 * a;
                                
                                w = w / cos( a );

                                doublePair spriteVerts[4] = 
                                    { { labelPos.x - w, 
                                        labelPos.y + arrowStart },
                                      { labelPos.x + w, 
                                        labelPos.y + arrowStart },
                                      { (double)arrowTipX + w, 
                                        (double)arrowTipY },
                                      { (double)arrowTipX - w, 
                                        (double)arrowTipY } };

                                FloatColor spriteColors[4] = 
                                    { { 1, 1, 1, (float)fade },
                                      { 1, 1, 1, (float)fade },
                                      { 1, 1, 1, (float)fade * .25f },
                                      { 1, 1, 1, (float)fade * .25f } };
                                
                                drawSprite( arrowSprite,
                                            spriteVerts, spriteColors );
                                }
                            else {
                                char *des = stringDuplicate( baseDes );
                                
                                char *poundPos = strstr( des, "#" );
                                
                                if( poundPos != NULL ) {
                                    // terminate at pound
                                    poundPos[0] = '\0';
                                    }
                                
                                SimpleVector<char*> *words = 
                                    tokenizeString( des );
                                
                                delete [] des;

                                SimpleVector<char*> finalWords;
                                for( int i=0; i< words->size(); i++ ) {
                                    char *word = words->getElementDirect( i );
                                    
                                    if( word[0] >= 65 &&
                                        word[0] <= 90 ) {
                                        
                                        char *u = stringToUpperCase( word );
                                        
                                        delete [] word;
                                        finalWords.push_back( u );
                                        }
                                    else {
                                        finalWords.push_back( word );
                                        }
                                    }
                                delete words;
                                
                                char **wordArray = finalWords.getElementArray();
                                
                                char *newDes = join( wordArray,
                                                     finalWords.size(),
                                                     " " );
                                finalWords.deallocateStringElements();
                                delete [] wordArray;
                                
                                
                                char *finalDes;
                                
                                if( worldY == 1 && numbered ) {
                                    finalDes = autoSprintf( 
                                        "%d. %s", 
                                        worldX - 10,
                                        newDes );
                                    }
                                else {
                                    finalDes = stringDuplicate( newDes );
                                    }
                                
                                delete [] newDes;
                                
                                double w = 
                                    mainFontReview->measureString( finalDes );
                                
                                doublePair rectPos = labelPos;
                                rectPos.y += 3;
                                
                                
                                

                                if( blackBorder ) {
                                    
                                    // block hole in border
                                    startAddingToStencil( false, true );
                                    setDrawColor( 1, 1, 1, 1 );
                                    drawRect( 
                                        rectPos, 
                                        w/2 + 
                                        mainFontReview->getFontHeight() / 2, 
                                        24 );
                                    
                                    startDrawingThroughStencil( true );
                                    
                                    setDrawColor( 0, 0, 0, fade );
                                    drawRect( 
                                        rectPos, 
                                        w/2 + 
                                        mainFontReview->getFontHeight() / 2 +
                                        2, 
                                        24 + 2 );
                                    
                                    stopStencil();
                                    }
                                
                                double rectW = 
                                    w/2 + mainFontReview->getFontHeight() / 2;
                                
                                double rectH = 24;
                                
                                setDrawColor( 1, 1, 1, fade );
                                drawRect( rectPos, rectW, rectH );

                                
                                if( whiteBorder ) {

                                    FloatColor lineColor = 
                                        { 1, 1, 1, (float)fade };
                                
                                    doublePair corners[4];
                                    
                                    double rOut = 20;
                                    double rIn = 4;
                                    
                                    

                                    corners[0].x = rectPos.x - rectW - 
                                        getBoundedRandom( worldX, 34,
                                                          rOut, rIn );
                                    corners[0].y = rectPos.y - rectH - 
                                        getBoundedRandom( worldX, 87,
                                                          rOut, rIn );
                                    
                                    
                                    corners[1].x = rectPos.x + rectW + 
                                        getBoundedRandom( worldX, 94,
                                                          rOut, rIn );
                                    corners[1].y = rectPos.y - rectH - 
                                        getBoundedRandom( worldX, 103,
                                                          rOut, rIn );
                                    
                                    
                                    corners[2].x = rectPos.x + rectW + 
                                        getBoundedRandom( worldX, 99,
                                                          rOut, rIn );
                                    corners[2].y = rectPos.y + rectH + 
                                        getBoundedRandom( worldX, 113,
                                                          rOut, rIn );
                                    
                                    
                                    corners[3].x = rectPos.x - rectW - 
                                        getBoundedRandom( worldX, 123,
                                                          rOut, rIn );
                                    corners[3].y = rectPos.y + rectH + 
                                        getBoundedRandom( worldX, 135,
                                                          rOut, rIn );
                                    
                                    
                                    for( int i=0; i<4; i++ ) {
                                        
                                        drawLine( mLineSegmentSprite,
                                                  corners[i], 
                                                  corners[ ( i + 1 ) % 4 ], 
                                                  lineColor );
                                        }
                                    }
                                

                                setDrawColor( 0, 0, 0, fade );
                                mainFontReview->drawString( 
                                    finalDes, 
                                    labelPos,
                                    alignCenter );
                                
                                delete [] finalDes;
                                }
                            }
                        }
                    }
                }
            }
        }
    

    if( apocalypseInProgress ) {
        toggleAdditiveBlend( true );
        
        setDrawColor( 1, 1, 1, apocalypseDisplayProgress );
        
        drawRect( lastScreenViewCenter, 640, 360 );
        
        toggleAdditiveBlend( false );
        }
    
    
    if( takingPhoto ) {

        if( photoSequenceNumber == -1 ) {
            photoSequenceNumber = getNextPhotoSequenceNumber();
            }
        else if( photoSig == NULL && ! waitingForPhotoSig ) {            
            char *message = 
                autoSprintf( "PHOTO %d %d %d#",
                             takingPhotoGlobalPos.x, takingPhotoGlobalPos.y,
                             photoSequenceNumber );
            sendToServerSocket( message );
            waitingForPhotoSig = true;
            delete [] message;
            }
        else if( photoSig != NULL ) {
            doublePair pos;
            
            pos.x = takingPhotoGlobalPos.x;
            pos.y = takingPhotoGlobalPos.y;
            
            
            pos = mult( pos, CELL_D );
            pos = sub( pos, lastScreenViewCenter );
            
            int screenWidth, screenHeight;
            getScreenDimensions( &screenWidth, &screenHeight );
            
            pos.x += screenWidth / 2;
            pos.y += screenHeight / 2;
        
            char *ourName;
            
            if( ourLiveObject->name != NULL ) {
                ourName = ourLiveObject->name;
                }
            else {
                ourName = (char*)translate( "namelessPerson" );
                }
            
            SimpleVector<int> subjectIDs;
            SimpleVector<char*> subjectNames;

            int xStart = takingPhotoGlobalPos.x + 1;
            
            int xEnd;

            if( takingPhotoFlip ) {
                xStart = takingPhotoGlobalPos.x - 3;
                xEnd = takingPhotoGlobalPos.x - 1;
                }
            else {
                xEnd = xStart + 3;
                }
            
            int yStart = takingPhotoGlobalPos.y - 1;
            int yEnd = yStart + 2;


            for( int i=0; i<gameObjects.size(); i++ ) {
                
                LiveObject *o = gameObjects.getElement( i );
                
                if( o != ourLiveObject ) {
                    doublePair p = o->currentPos;
                    
                    if( p.x >= xStart && p.x <= xEnd &&
                        p.y >= yStart && p.y <= yEnd ) {
                        subjectIDs.push_back( o->id );
                        
                        if( o->name != NULL ) {
                            subjectNames.push_back( o->name );
                            }
                        }
                    }
                }
            

            takePhoto( pos, takingPhotoFlip ? -1 : 1,
                       photoSequenceNumber,
                       photoSig,
                       ourID,
                       ourName,
                       &subjectIDs,
                       &subjectNames );
            
            takingPhoto = false;
            delete [] photoSig;
            photoSig = NULL;
            photoSequenceNumber = -1;
            waitingForPhotoSig = false;
            }
        }
    

    
    if( hideGuiPanel ) {
        // skip gui
        return;
        }    
        
    if( showFPS ) {
            
        doublePair pos = lastScreenViewCenter;
        pos.x -= 600;
        pos.y += 300;
        
        if( mTutorialNumber > 0 || mGlobalMessageShowing ) {
            pos.y -= 50;
            }

        if( frameBatchMeasureStartTime == -1 ) {
            frameBatchMeasureStartTime = game_getCurrentTime();
            }
        else {
            framesInBatch ++;
            
            
            if( framesInBatch == 30 ) {
                double delta = game_getCurrentTime() - 
                    frameBatchMeasureStartTime;
            
                fpsToDraw = framesInBatch / delta;
                
                addToGraph( &fpsHistoryGraph, fpsToDraw );
                
                // new batch
                frameBatchMeasureStartTime = game_getCurrentTime();
                framesInBatch = 0;
                
                // average time measures
                for( int i=0; i<MEASURE_TIME_NUM_CATEGORIES; i++ ) {
                    timeMeasures[i] /= 30;
                    timeMeasureToDraw[i] = timeMeasures[i];
                    }
                
                
                addToGraph( &timeMeasureHistoryGraph, timeMeasures );
                
                // start measuring again
                for( int i=0; i<MEASURE_TIME_NUM_CATEGORIES; i++ ) {
                    timeMeasures[i] = 0;
                    }

                double uniqueDrawn = endCountingUniqueSpriteDraws();
                double spritesDrawn = endCountingSpritesDrawn();
                
                spriteCountToDraw = spritesDrawn / 30;
                // this is uniq count drawn in whole 30-frame batch
                // not an average
                uniqueSpriteCountToDraw = uniqueDrawn;
                
                pixelCountToDraw = runningPixelCount / 30;

                addToGraph( &spriteCountHistoryGraph, spriteCountToDraw );
                addToGraph( &uniqueSpriteHistoryGraph, 
                            uniqueSpriteCountToDraw );
                
                addToGraph( &pixelCountHistoryGraph, pixelCountToDraw );
                
                startCountingSpritesDrawn();
                startCountingUniqueSpriteDraws();
                runningPixelCount = 0;
                }
            }
        if( fpsToDraw != -1 ) {
        
            char *fpsString = 
                autoSprintf( "%5.1f %s", fpsToDraw, translate( "fps" ) );
            
            drawFixedShadowStringWhite( fpsString, pos );
            
            pos.x += 20 + numbersFontFixed->measureString( fpsString );
            pos.y -= 20;
            
            FloatColor yellow = { 1, 1, 0, 1 };
            drawGraph( &fpsHistoryGraph, pos, yellow );

            delete [] fpsString;

            pos.x += 120;
            pos.y += 60;
            
            double maxStringW = 0;

            
            for( int i=MEASURE_TIME_NUM_CATEGORIES - 1; i>=0; i-- ) {
                
                char *timeString = 
                    autoSprintf( "%4.1f %s %s", 
                                 timeMeasureToDraw[i] * 1000,
                                 timeMeasureNames[i],
                                 translate( "ms/f" ) );
                pos.y -= 20;
                
                setDrawColor( timeMeasureGraphColors[i] );
                drawFixedShadowString( timeString, pos );

                double w = numbersFontFixed->measureString( timeString );
                if( w > maxStringW ) {
                    maxStringW = w;
                    }
                
                delete [] timeString;
                }

            pos.y += 0;
            
            pos.x += 20 + maxStringW;

            drawGraph( &timeMeasureHistoryGraph, pos, timeMeasureGraphColors );

            
            pos.x += 120;

            drawGraph( &spriteCountHistoryGraph, pos, yellow );


            pos.x -= 60;
            pos.y += 60;
            char *spriteString = 
                autoSprintf( "%6.0f %s", spriteCountToDraw, 
                             translate( "spritesDrawn" ) );
            
            drawFixedShadowStringWhite( spriteString, pos );

            delete [] spriteString;

            pos.x += 60;
            pos.y -= 60;


            pos.x += 120;
            drawGraph( &uniqueSpriteHistoryGraph, pos, yellow );

            pos.x -= 60;
            pos.y -= 20;

            char *unqString = 
                autoSprintf( "%6.0f %s", uniqueSpriteCountToDraw, 
                             translate( "uniqueSprites" ) );
            
            drawFixedShadowStringWhite( unqString, pos );

            delete [] unqString;


            pos.y -= 80;
            drawGraph( &pixelCountHistoryGraph, pos, yellow );

            pos.x -= 60;
            pos.y -= 20;

            char pixBuffer[16];
            
            stringFormatIntGrouped( pixBuffer, (int)pixelCountToDraw ); 

            
            char *pixString = 
                autoSprintf( "%9s %s", pixBuffer, 
                             translate( "pixelsDrawn" ) );
            
            drawFixedShadowStringWhite( pixString, pos );

            delete [] pixString;
            }
        else {
            drawFixedShadowStringWhite( translate( "fpsPending" ), pos );
            }
        }
    
    if( showNet ) {
        doublePair pos = lastScreenViewCenter;
        pos.x -= 600;
        pos.y += 300;
        
        if( showFPS ) {
            pos.y -= 50;
            }
        // covered by tutorial sheets
        if( mTutorialNumber > 0 || mGlobalMessageShowing ) {
            pos.y -= 50;
            }

        double curTime = game_getCurrentTime();
        
        if( netBatchMeasureStartTime == -1 ) {
            netBatchMeasureStartTime = curTime;
            }
        else {
            
            double batchLength = curTime - netBatchMeasureStartTime;
            
            if( batchLength >= 1.0 ) {
                // full batch

                messagesInPerSec = lrint( messagesInCount / batchLength );
                messagesOutPerSec = lrint( messagesOutCount / batchLength );

                bytesInPerSec = lrint( bytesInCount / batchLength );
                bytesOutPerSec = lrint( bytesOutCount / batchLength );
                
                
                addToGraph( &messagesInHistoryGraph, messagesInPerSec );
                addToGraph( &messagesOutHistoryGraph, messagesOutPerSec );
                addToGraph( &bytesInHistoryGraph, bytesInPerSec );
                addToGraph( &bytesOutHistoryGraph, bytesOutPerSec );


                // new batch
                messagesInCount = 0;
                messagesOutCount = 0;
                bytesInCount = 0;
                bytesOutCount = 0;
                
                netBatchMeasureStartTime = curTime;
                }
            }
        
        if( messagesInPerSec != -1 ) {
            char *netStringA = 
                autoSprintf( translate( "netStringA" ), 
                             messagesOutPerSec, messagesInPerSec );
            char *netStringB = 
                autoSprintf( translate( "netStringB" ), 
                             bytesOutPerSec, bytesInPerSec );
            
            drawFixedShadowStringWhite( netStringA, pos );
            
            doublePair graphPos = pos;
            
            graphPos.x += 20 + numbersFontFixed->measureString( netStringA );
            graphPos.y -= 20;

            FloatColor yellow = { 1, 1, 0, 1 };
            drawGraph( &messagesOutHistoryGraph, graphPos, yellow );

            graphPos.x += historyGraphLength + 10;
            drawGraph( &messagesInHistoryGraph, graphPos, yellow );

            pos.y -= 50;
            
            drawFixedShadowStringWhite( netStringB, pos );
            
            graphPos = pos;
            
            graphPos.x += 20 + numbersFontFixed->measureString( netStringB );
            graphPos.y -= 20;

            drawGraph( &bytesOutHistoryGraph, graphPos, yellow );

            graphPos.x += historyGraphLength + 10;
            drawGraph( &bytesInHistoryGraph, graphPos, yellow );



            delete [] netStringA;
            delete [] netStringB;
            }
        else {
            drawFixedShadowStringWhite( translate( "netPending" ), pos );
            }
        }
    
    if( showPing ) {
        if( pongDeltaTime != -1 &&
            pingDisplayStartTime == -1 ) {
            pingDisplayStartTime = game_getCurrentTime();
            }
        
        doublePair pos = lastScreenViewCenter;
        pos.x += 300;
        pos.y += 300;
        
        if( mTutorialNumber > 0 || mGlobalMessageShowing ) {
            pos.y -= 50;
            }

        char *pingString;
        
        if( pongDeltaTime == -1 ) {
            pingString = 
                autoSprintf( "%s...", translate( "ping" ) );
            }
        else {
            pingString = 
                autoSprintf( "%s %d %s", translate( "ping" ), 
                             lrint( pongDeltaTime * 1000 ), 
                             translate( "ms" ) );
            }

        drawFixedShadowStringWhite( pingString, pos );
            
        delete [] pingString;
    
        if( pingDisplayStartTime != -1 &&
            game_getCurrentTime() - pingDisplayStartTime > 10 ) {
            showPing = false;
            }
        }
    
    


    for( int j=0; j<2; j++ ) {
        doublePair slipPos = add( mHomeSlipPosOffset[j], lastScreenViewCenter );
        
        if( ! equal( mHomeSlipPosOffset[j], mHomeSlipHideOffset[j] ) ) {
            drawHomeSlip( slipPos, j );
            }
        }




    
    
    const char *shiftHintKey = "shiftHint";
    if( mUsingSteam ) {
        shiftHintKey = "zHint";
        }
    
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        if( ! equal( mHintPosOffset[i], mHintHideOffset[i] ) 
            &&
            mHintMessage[i] != NULL ) {
            
            doublePair hintPos  = 
                add( mHintPosOffset[i], lastScreenViewCenter );
            
            hintPos = add( hintPos, mHintExtraOffset[i] );


            char *pageNum = NULL;
            double pageNumExtra = 0;
            
            if( mNumTotalHints[i] > 1 ) {
                
                pageNum = 
                    autoSprintf( "(%d %s %d)",
                                 mHintMessageIndex[i] + 1, 
                                 translate( "ofHint" ),
                                 mNumTotalHints[i] );
            
                double extraA = handwritingFont->measureString( pageNum );
            
                double extraB = 
                    handwritingFont->measureString( translate( "tabHint" ) );

                double extraC = 
                    handwritingFont->measureString( translate( shiftHintKey ) );
                
                if( extraB > extraA ) {
                    extraA = extraB;
                    }
                if( extraC > extraA ) {
                    extraA = extraC;
                    }
                
                pageNumExtra = extraA;
                
                hintPos.x -= extraA;
                hintPos.x -= 10;
                }
            

            setDrawColor( 1, 1, 1, 1 );

            doublePair sheetSpritePos = hintPos;
            sheetSpritePos.x += mHintSheetXTweak[i];
            
            drawSprite( mHintSheetSprites[i], sheetSpritePos );
            

            setDrawColor( 0, 0, 0, 1.0f );
            double lineSpacing = handwritingFont->getFontHeight() / 2 + 5;
            
            int numLines;
            
            char **lines = split( mHintMessage[i], "#", &numLines );
            
            doublePair lineStart = hintPos;
            lineStart.x -= 280;
            lineStart.y += 30;
            for( int l=0; l<numLines; l++ ) {
                
                if( l == 1 ) {
                    doublePair drawPos = lineStart;
                    drawPos.x -= 5;
                    
                    handwritingFont->drawString( "+",
                                                 drawPos, alignRight );
                    }
                
                if( l == 2 ) {
                    doublePair drawPos = lineStart;
                    drawPos.x -= 5;
                    
                    handwritingFont->drawString( "=",
                                                 drawPos, alignRight );
                    }
                
                handwritingFont->drawString( lines[l], 
                                             lineStart, alignLeft );
                    
                delete [] lines[l];
                
                lineStart.y -= lineSpacing;
                }
            delete [] lines;
            

            if( pageNum != NULL ) {
                
                // now draw tab message
                
                lineStart = hintPos;
                lineStart.x -= 280;
                
                lineStart.x -= mHintExtraOffset[i].x;
                lineStart.x += 20;
                
                lineStart.y += 30;
                
                handwritingFont->drawString( pageNum, lineStart, alignLeft );
                
                
                lineStart.y -= lineSpacing;
                
                lineStart.x += pageNumExtra;

                // center shift and tab hints under page number, which is always
                // bigger
                lineStart.x -= 0.5 * handwritingFont->measureString( pageNum );
                
                delete [] pageNum;
                
                handwritingFont->drawString( translate( shiftHintKey ), 
                                             lineStart, alignCenter );
                
                lineStart.y -= lineSpacing;
                
                handwritingFont->drawString( translate( "tabHint" ), 
                                             lineStart, alignCenter );
                }
            }
        }



    // now draw tutorial sheets
    if( mTutorialNumber > 0 || mGlobalMessageShowing )
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        if( ! equal( mTutorialPosOffset[i], mTutorialHideOffset[i] ) ) {
            
            doublePair tutorialPos  = 
                add( mTutorialPosOffset[i], lastScreenViewCenter );
            
            if( i % 2 == 1 ) {
                tutorialPos = sub( tutorialPos, mTutorialExtraOffset[i] );
                }
            else {
                tutorialPos = add( tutorialPos, mTutorialExtraOffset[i] );
                }
            
            setDrawColor( 1, 1, 1, 1 );
            // rotate 180
            drawSprite( mHintSheetSprites[i], tutorialPos, 1.0, 0.5,
                        mTutorialFlips[i] );
            

            setDrawColor( 0, 0, 0, 1.0f );
            double lineSpacing = handwritingFont->getFontHeight() / 2 + 16;
            
            int numLines;
            
            char **lines = split( mTutorialMessage[i], "##", &numLines );
            
            doublePair lineStart = tutorialPos;
            
            if( i % 2 == 1 ) {
                lineStart.x -= 289;
                //lineStart.x += mTutorialExtraOffset[i].x;
                }
            else {
                lineStart.x += 289;
                lineStart.x -= mTutorialExtraOffset[i].x;
                }
            
            lineStart.y += 8;
            for( int l=0; l<numLines; l++ ) {
                
                handwritingFont->drawString( lines[l], 
                                             lineStart, alignLeft );
                    
                delete [] lines[l];
                
                lineStart.y -= lineSpacing;
                }
            delete [] lines;
            }
        }










    double highestCravingYOffset = 0;
    
    if( mLiveCravingSheetIndex != -1 ) {
        // craving showing
        // find highest one
        highestCravingYOffset = 0;
                
        for( int c=0; c<NUM_HINT_SHEETS; c++ ) {
            double offset = mCravingPosOffset[c].y - mCravingHideOffset[c].y;
            if( offset > highestCravingYOffset ) {
                highestCravingYOffset = offset;
                }
            }
        }
    



    setDrawColor( 1, 1, 1, 1 );
    
    for( int i=0; i<3; i++ ) { 
        if( !equal( mHungerSlipPosOffset[i], mHungerSlipHideOffsets[i] ) ) {
            doublePair slipPos = lastScreenViewCenter;
            slipPos = add( slipPos, mHungerSlipPosOffset[i] );
            
            if( mHungerSlipWiggleAmp[i] > 0 ) {
                
                double distFromHidden =
                    mHungerSlipPosOffset[i].y - mHungerSlipHideOffsets[i].y;

                // amplitude grows when we are further from
                // hidden, and shrinks again as we go back down

                double slipHarmonic = 
                    ( 0.5 * ( 1 - cos( mHungerSlipWiggleTime[i] ) ) ) *
                    mHungerSlipWiggleAmp[i] * distFromHidden;
                
                slipPos.y += slipHarmonic;
                

                if( i == 2 ) {
                    
                    if( mStarvingSlipLastPos[0] != 0 &&
                        mStarvingSlipLastPos[1] != 0 ) {
                        double lastDir = mStarvingSlipLastPos[1] - 
                            mStarvingSlipLastPos[0];
                        
                        
                        if( lastDir > 0 ) {
                            
                            double newDir = 
                                slipHarmonic - mStarvingSlipLastPos[1];
                            
                            if( newDir < 0 ) {
                                // peak
                                if( mPulseHungerSound && 
                                    mHungerSound != NULL ) {
                                    // make sure it can be heard, even
                                    // if paused
                                    setSoundLoudness( 1.0 );
                                    playSoundSprite( mHungerSound, 
                                                     getSoundEffectsLoudness(),
                                                     // middle
                                                     0.5 );
                                    }
                                }
                            }
                        
                        }
                    mStarvingSlipLastPos[0] = mStarvingSlipLastPos[1];
                    
                    mStarvingSlipLastPos[1] = slipHarmonic;
                    }
                }
            
            slipPos.y += lrint( highestCravingYOffset / 1.75 );

            drawSprite( mHungerSlipSprites[i], slipPos );
            }
        }



    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {

        if( ! equal( mYumSlipPosOffset[i], mYumSlipHideOffset[i] ) ) {
            doublePair slipPos = 
                add( mYumSlipPosOffset[i], lastScreenViewCenter );
        
            slipPos.y += lrint( highestCravingYOffset / 1.75 );
            
            setDrawColor( 1, 1, 1, 1 );
            drawSprite( mYumSlipSprites[i], slipPos );
            
            doublePair messagePos = slipPos;
            messagePos.y += 11;

            if( mYumSlipNumberToShow[i] != 0 ) {
                char *s = autoSprintf( "%dx", mYumSlipNumberToShow[i] );

                setDrawColor( 0, 0, 0, 1 );
                handwritingFont->drawString( s, messagePos, alignCenter );
                delete [] s;
                }
            if( i == 2 || i == 3 ) {
                setDrawColor( 0, 0, 0, 1 );

                const char *word;
                
                if( i == 3 ) {
                    word = translate( "meh" );
                    }
                else {
                    word = translate( "yum" );
                    }

                handwritingFont->drawString( word, messagePos, alignCenter );
                }
            }
        }

    
    
    // now draw craving sheets
    if( mLiveCravingSheetIndex > -1 )
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        if( ! equal( mCravingPosOffset[i], mCravingHideOffset[i] ) ) {
            
            doublePair cravingPos  = 
                add( mCravingPosOffset[i], lastScreenViewCenter );
            
            cravingPos = add( cravingPos, mCravingExtraOffset[i] );
            
            setDrawColor( 1, 1, 1, 1.0 );
            // flip, don't rotate
            drawSprite( mHintSheetSprites[i], cravingPos, 1.0, 0.0, true );
                
            setDrawColor( 0, 0, 0, 1.0f );
            
            doublePair lineStart = cravingPos;
            
            lineStart.x += 298;
            lineStart.x -= mCravingExtraOffset[i].x;
            
            lineStart.y += 26;
                
            handwritingFont->drawString( mCravingMessage[i],
                                         lineStart, alignLeft );
            
            }
        }



    
    // finally, draw chat note sheet, so that it covers craving sheet
    // whenever it is up.

    int lineSpacing = 20;

    doublePair notePos = add( mNotePaperPosOffset, lastScreenViewCenter );

    if( ! equal( mNotePaperPosOffset, mNotePaperHideOffset ) ) {
        setDrawColor( 1, 1, 1, 1 );
        drawSprite( mNotePaperSprite, notePos );
        

        doublePair drawPos = notePos;

        drawPos.x += 160;
        drawPos.y += 79;
        drawPos.y += 22;
        
        drawPos.x += 27;

        setDrawColor( 0, 0, 0, 1 );
        
        handwritingFont->drawString( translate( "enterHint" ), 
                                     drawPos,
                                     alignRight );
        }
        

    

    doublePair paperPos = add( mNotePaperPosOffset, lastScreenViewCenter );

    if( mSayField.isFocused() ) {
        char *partialSay = mSayField.getText();

        char *strUpper = stringToUpperCase( partialSay );
        
        delete [] partialSay;

        SimpleVector<char*> *lines = splitLines( strUpper, 345 );
        
        mNotePaperPosTargetOffset.y = mNotePaperHideOffset.y + 58;
        
        if( lines->size() > 1 ) {    
            mNotePaperPosTargetOffset.y += 20 * ( lines->size() - 1 );
            }
        
        doublePair drawPos = paperPos;

        drawPos.x -= 160;
        drawPos.y += 79;


        doublePair drawPosTemp = drawPos;
        

        for( int i=0; i<mLastKnownNoteLines.size(); i++ ) {
            char *oldString = mLastKnownNoteLines.getElementDirect( i );
            int oldLen = strlen( oldString );
            
            SimpleVector<doublePair> charPos;        
                    
            pencilFont->getCharPos( &charPos, 
                                    oldString,
                                    drawPosTemp,
                                    alignLeft );
            
            int newLen = 0;
            
            if( i < lines->size() ) {
                // compare lines

                newLen = strlen( lines->getElementDirect( i ) );
                
                }
            

            // any extra chars?
                    
            for( int j=newLen; j<oldLen; j++ ) {
                mErasedNoteChars.push_back( oldString[j] );
                       
                mErasedNoteCharOffsets.push_back(
                    sub( charPos.getElementDirect( j ),
                         paperPos ) );
                
                mErasedNoteCharFades.push_back( 1.0f );
                }
            
            drawPosTemp.y -= lineSpacing;
            }
        mLastKnownNoteLines.deallocateStringElements();
        
        for( int i=0; i<lines->size(); i++ ) {
            mLastKnownNoteLines.push_back( 
                stringDuplicate( lines->getElementDirect(i) ) );
            }
        

    
        delete [] strUpper;

        
        
        setDrawColor( 0, 0, 0, 1 );
        
        mCurrentNoteChars.deleteAll();
        mCurrentNoteCharOffsets.deleteAll();
        
        for( int i=0; i<lines->size(); i++ ) {
            char *line = lines->getElementDirect( i );
            
            pencilFont->drawString( line, drawPos,
                                    alignLeft );

            SimpleVector<doublePair> charPos;        
                    
            pencilFont->getCharPos( &charPos, 
                                    line,
                                    drawPos,
                                    alignLeft );

            int lineSize = strlen( line );
            
            for( int j=0; j<lineSize; j++ ) {
                mCurrentNoteChars.push_back( line[j] );
                mCurrentNoteCharOffsets.push_back( 
                    sub( charPos.getElementDirect( j ), paperPos ) );
                }

            drawPos.y -= lineSpacing;
            }
        lines->deallocateStringElements();
        delete lines;
        }
    else {
        mNotePaperPosTargetOffset = mNotePaperHideOffset;

        doublePair drawPos = paperPos;

        drawPos.x -= 160;
        drawPos.y += 79;

        for( int i=0; i<mLastKnownNoteLines.size(); i++ ) {
            // whole line gone
            
            char *oldString = mLastKnownNoteLines.getElementDirect( i );
            int oldLen = strlen( oldString );
            
            SimpleVector<doublePair> charPos;        
                    
            pencilFont->getCharPos( &charPos, 
                                    oldString,
                                    drawPos,
                                    alignLeft );
                    
            for( int j=0; j<oldLen; j++ ) {
                mErasedNoteChars.push_back( oldString[j] );
                        
                mErasedNoteCharOffsets.push_back(
                    sub( charPos.getElementDirect( j ),
                         paperPos ) );
                
                mErasedNoteCharFades.push_back( 1.0f );
                }
            
            drawPos.y -= lineSpacing;

            }
        mLastKnownNoteLines.deallocateStringElements();
        }
    


    setDrawColor( 0, 0, 0, 1 );
    for( int i=0; i<mErasedNoteChars.size(); i++ ) {
        setDrawFade( mErasedNoteCharFades.getElementDirect( i ) *
                     pencilErasedFontExtraFade );
        
        pencilErasedFont->
            drawCharacterSprite( 
                mErasedNoteChars.getElementDirect( i ), 
                add( paperPos, 
                     mErasedNoteCharOffsets.getElementDirect( i ) ) );
        }






    // info panel at bottom, over top of all the other slips
    setDrawColor( 1, 1, 1, 1 );
    doublePair panelPos = lastScreenViewCenter;
    panelPos.y -= 242 + 32 + 16 + 6;
    drawSprite( mGuiPanelSprite, panelPos );

    if( ourLiveObject != NULL &&
        ourLiveObject->dying  &&
        ! ourLiveObject->sick ) {
        toggleMultiplicativeBlend( true );
        doublePair bloodPos = panelPos;
        bloodPos.y -= 32;
        bloodPos.x -= 32;
        drawSprite( mGuiBloodSprite, bloodPos );
        toggleMultiplicativeBlend( false );
        }
    


    if( ourLiveObject != NULL ) {

        // draw curse token status
        Font *curseTokenFont;
        if( ourLiveObject->curseTokenCount > 0 ) {
            setDrawColor( 0, 0, 0, 1.0 );
            curseTokenFont = pencilFont;
            }
        else {
            setDrawColor( 0, 0, 0, pencilErasedFontExtraFade );
            curseTokenFont = pencilErasedFont;
            }

        // show as a sigil to right of temp meter
        doublePair curseTokenPos = { lastScreenViewCenter.x + 621, 
                                     lastScreenViewCenter.y - 316 };
        curseTokenFont->drawString( "C", curseTokenPos, alignCenter );
        curseTokenFont->drawString( "+", curseTokenPos, alignCenter );
        curseTokenPos.x += 6;
        curseTokenFont->drawString( "X", curseTokenPos, alignCenter );
        
        
        // for now, we receive at most one update per life, so
        // don't need to worry about showing erased version of this
        if( ourLiveObject->excessCursePoints > 0 ) {
            setDrawColor( 0, 0, 0, 1.0 );
            doublePair pointsPos = curseTokenPos;
            pointsPos.y -= 22;
            pointsPos.x -= 3;
            
            char *pointString = autoSprintf( "%d", 
                                             ourLiveObject->excessCursePoints );
            pencilFont->drawString( pointString, pointsPos, alignCenter );
            delete [] pointString;
            }
        

        setDrawColor( 1, 1, 1, 1 );
        toggleMultiplicativeBlend( true );

        for( int i=0; i<ourLiveObject->foodCapacity; i++ ) {
            doublePair pos = { lastScreenViewCenter.x - 590, 
                               lastScreenViewCenter.y - 334 };
        
            pos.x += i * 30;
            drawSprite( 
                    mHungerBoxSprites[ i % NUM_HUNGER_BOX_SPRITES ], 
                    pos );
                
            if( i < ourLiveObject->foodStore ) {                
                drawSprite( 
                    mHungerBoxFillSprites[ i % NUM_HUNGER_BOX_SPRITES ], 
                    pos );
                }
            else if( i < ourLiveObject->maxFoodStore ) {
                drawSprite( 
                    mHungerBoxFillErasedSprites[ i % NUM_HUNGER_BOX_SPRITES ], 
                    pos );
                }
            }
        for( int i=ourLiveObject->foodCapacity; 
             i < ourLiveObject->maxFoodCapacity; i++ ) {
            doublePair pos = { lastScreenViewCenter.x - 590, 
                               lastScreenViewCenter.y - 334 };
            
            pos.x += i * 30;
            drawSprite( 
                mHungerBoxErasedSprites[ i % NUM_HUNGER_BOX_SPRITES ], 
                pos );
            
            if( i < ourLiveObject->maxFoodStore ) {
                drawSprite( 
                    mHungerBoxFillErasedSprites[ i % NUM_HUNGER_BOX_SPRITES ], 
                    pos );
                }
            }
        
        
                
        
        doublePair pos = { lastScreenViewCenter.x + 546, 
                           lastScreenViewCenter.y - 319 };

        if( mCurrentArrowHeat != -1 ) {
            
            if( mCurrentArrowHeat != ourLiveObject->heat ) {
                
                for( int i=0; i<mOldArrows.size(); i++ ) {
                    OldArrow *a = mOldArrows.getElement( i );
                    
                    a->fade -= 0.01;
                    if( a->fade < 0 ) {
                        mOldArrows.deleteElement( i );
                        i--;
                        }
                    }
                

                OldArrow a;
                a.i = mCurrentArrowI;
                a.heat = mCurrentArrowHeat;
                a.fade = 1.0;

                mOldArrows.push_back( a );

                mCurrentArrowI++;
                mCurrentArrowI = mCurrentArrowI % NUM_TEMP_ARROWS;
                }
            }

        toggleAdditiveTextureColoring( true );
        
        for( int i=0; i<mOldArrows.size(); i++ ) {
            doublePair pos2 = pos;
            OldArrow *a = mOldArrows.getElement( i );
            
            float v = 1.0 - a->fade;
            setDrawColor( v, v, v, 1 );
            pos2.x += ( a->heat - 0.5 ) * 120;

            // no sub pixel positions
            pos2.x = round( pos2.x );

            drawSprite( mTempArrowErasedSprites[a->i], pos2 );
            }
        toggleAdditiveTextureColoring( false );
        
        setDrawColor( 1, 1, 1, 1 );

        mCurrentArrowHeat = ourLiveObject->heat;
        
        pos.x += ( mCurrentArrowHeat - 0.5 ) * 120;
        
        // no sub pixel positions
        pos.x = round( pos.x );
        
        drawSprite( mTempArrowSprites[mCurrentArrowI], pos );
        
        toggleMultiplicativeBlend( false );
        

        for( int i=0; i<mOldDesStrings.size(); i++ ) {
            doublePair pos = { lastScreenViewCenter.x, 
                               lastScreenViewCenter.y - 313 };
            float fade =
                mOldDesFades.getElementDirect( i );
            
            setDrawColor( 0, 0, 0, fade * pencilErasedFontExtraFade );
            pencilErasedFont->drawString( 
                mOldDesStrings.getElementDirect( i ), pos, alignCenter );
            }

        doublePair yumPos = { lastScreenViewCenter.x - 480, 
                              lastScreenViewCenter.y - 313 };
        
        setDrawColor( 0, 0, 0, 1 );
        if( mYumBonus > 0 ) {    
            char *yumString = autoSprintf( "+%d", mYumBonus );
            
            pencilFont->drawString( yumString, yumPos, alignLeft );
            delete [] yumString;
            }
        
        for( int i=0; i<mOldYumBonus.size(); i++ ) {
            float fade =
                mOldYumBonusFades.getElementDirect( i );
            
            setDrawColor( 0, 0, 0, fade * pencilErasedFontExtraFade );
            char *yumString = autoSprintf( "+%d", 
                                           mOldYumBonus.getElementDirect( i ) );
            pencilErasedFont->drawString( yumString, yumPos, alignLeft );
            delete [] yumString;
            }



        doublePair atePos = { lastScreenViewCenter.x, 
                              lastScreenViewCenter.y - 347 };
        
        int shortestFill = 100;
        
        
        for( int i=0; i<mOldLastAteStrings.size(); i++ ) {
            float fade =
                mOldLastAteFades.getElementDirect( i );
            
            setDrawColor( 0, 0, 0, fade * pencilErasedFontExtraFade );
            
            pencilErasedFont->drawString( 
                mOldLastAteStrings.getElementDirect( i ), atePos, alignLeft );

            toggleMultiplicativeBlend( true );
            toggleAdditiveTextureColoring( true );
            
            float v = 1.0f - mOldLastAteBarFades.getElementDirect( i );
            setDrawColor( v, v, v, 1 );
            
            int fillMax = mOldLastAteFillMax.getElementDirect( i );

            if( fillMax < shortestFill ) {
                shortestFill = fillMax;
                }

            drawHungerMaxFillLine( atePos, 
                                   fillMax,
                                   mHungerBarErasedSprites,
                                   mHungerDashErasedSprites, 
                                   false,
                                   // only draw dashes once, for longest
                                   // one
                                   true );


            toggleAdditiveTextureColoring( false );
            toggleMultiplicativeBlend( false );
            }

        if( shortestFill < 100 ) {
            toggleMultiplicativeBlend( true );
            setDrawColor( 1, 1, 1, 1 );
            
            drawHungerMaxFillLine( atePos, 
                                   shortestFill,
                                   mHungerBarErasedSprites,
                                   mHungerDashErasedSprites, 
                                   true,
                                   // draw longest erased dash line once
                                   false );
            toggleMultiplicativeBlend( false );
            }
        

        if( mCurrentLastAteString != NULL ) {
            setDrawColor( 0, 0, 0, 1 );
        
            pencilFont->drawString( 
                mCurrentLastAteString, atePos, alignLeft );
            
            
            toggleMultiplicativeBlend( true );
            setDrawColor( 1, 1, 1, 1 );
            
            drawHungerMaxFillLine( atePos, 
                                   mCurrentLastAteFillMax,
                                   mHungerBarSprites,
                                   mHungerDashSprites,
                                   false, false );
            
            toggleMultiplicativeBlend( false );
            }


        doublePair tipPos = { lastScreenViewCenter.x, 
                           lastScreenViewCenter.y - 313 };

        char overTempMeter = false;
        
        doublePair mousePos = { lastMouseX, lastMouseY };
        
        if( mousePos.y < tipPos.y + 13 &&
            mousePos.x > tipPos.x + 480 &&
            mousePos.x < tipPos.x + 607 ) {
            overTempMeter = true;
            }
        
        char badBiome = false;
        if( mCurMouseOverBiome != -1 &&
            mBadBiomeIndices.getElementIndex( mCurMouseOverBiome ) 
            != -1 ) {
            badBiome = true;
            mLastMouseOverID = 0;
            mCurMouseOverID = 0;
            }
        
        if( ( overTempMeter && ourLiveObject->foodDrainTime > 0 ) 
            || badBiome
            || mCurMouseOverID != 0 || mLastMouseOverID != 0 ) {
            
            int idToDescribe = mCurMouseOverID;
            
            if( mCurMouseOverID == 0 ) {
                idToDescribe = mLastMouseOverID;
                }

            
            

            char *des = NULL;
            char *desToDelete = NULL;
            

            
            if( overTempMeter && ourLiveObject->foodDrainTime > 0 ) {
                // don't replace old until next mouse over
                // otherwise, it gets redrawn constantly as value
                // changes
                if( mCurrentDes == NULL ||
                    strstr( mCurrentDes,
                            translate( "foodTimeString" ) ) == NULL ) {
                    
                    
                    char *indoorBonusString;
                    double mainSeconds = ourLiveObject->foodDrainTime;
                    
                    if( ourLiveObject->indoorBonusTime > 0 ) {
                        indoorBonusString = 
                            autoSprintf( 
                                translate( "indoorBonusFormatString" ),
                                ourLiveObject->indoorBonusTime );
                        mainSeconds -= ourLiveObject->indoorBonusTime;
                        }
                    else {
                        indoorBonusString = stringDuplicate( "" );
                        }
                    
                    
                    des = autoSprintf( translate( "foodTimeFormatString" ),
                                       translate( "foodTimeString" ),
                                       mainSeconds,
                                       indoorBonusString );
                    delete [] indoorBonusString;
                    
                    desToDelete = des;
                    }
                else {
                    des = stringDuplicate( mCurrentDes );
                    desToDelete = des;
                    }
                }
            else if( idToDescribe == -99 ) {
                if( ourLiveObject->holdingID > 0 &&
                    getObject( ourLiveObject->holdingID )->foodValue > 0 ) {
                    
                    const char *key = "eat";
                    
                    if( strstr( 
                            getObject( ourLiveObject->holdingID )->description,
                            "+drink" ) != NULL ) {
                        key = "drink";
                        }

                    des = autoSprintf( "%s %s",
                                       translate( key ),
                                       getObject( ourLiveObject->holdingID )->
                                       description );
                    desToDelete = des;
                    }
                else if( ( ourLiveObject->dying || isSick( ourLiveObject ) ) 
                         &&
                         ourLiveObject->holdingID > 0 ) {
                    des = autoSprintf( "%s %s",
                                       translate( "youWith" ),
                                       getObject( ourLiveObject->holdingID )->
                                       description );
                    desToDelete = des;
                    }
                else {
                    des = (char*)translate( "you" );
                    
                    if( ourLiveObject->leadershipNameTag != NULL ||
                        ourLiveObject->name != NULL ) {
                        char *workingName;
                        if( ourLiveObject->name != NULL ) {
                            workingName = 
                                autoSprintf( " %s", ourLiveObject->name );
                            }
                        else {
                            workingName = stringDuplicate( "" );
                            }
                        
                        const char *leaderString = "";
                        
                        if( ourLiveObject->leadershipNameTag != NULL ) {
                            leaderString = ourLiveObject->leadershipNameTag;
                            }
                        
                        
                        des = autoSprintf( "%s - %s%s", des, 
                                           leaderString, workingName );
                        desToDelete = des;

                        delete [] workingName;
                        }
                    }
                }
            else if( idToDescribe < 0 ) {
                LiveObject *otherObj = getLiveObject( -idToDescribe );
                
                if( otherObj != NULL ) {
                    des = otherObj->relationName;
                    }
                if( des == NULL ) {
                    des = (char*)translate( "unrelated" );

                    if( otherObj != NULL && otherObj->warPeaceStatus != 0 ) {
                        
                        const char *key = "atWar";
                        if( otherObj->warPeaceStatus > 0 ) {
                            key = "atPeace";
                            }
                        
                        des = autoSprintf( "%s - %s", des, translate( key ) );
                        
                        if( desToDelete != NULL ) {
                            delete [] desToDelete;
                            }
                        
                        desToDelete = des;
                        }
                    }
                if( otherObj != NULL && otherObj->name != NULL ) {
                    des = autoSprintf( "%s - %s",
                                       otherObj->name, des );
                    
                    if( desToDelete != NULL ) {
                        delete [] desToDelete;
                        }
                    
                    desToDelete = des;
                    }
                
                if( otherObj != NULL && otherObj->leadershipNameTag != NULL ) {
                    if( otherObj->name == NULL ) {
                        des = autoSprintf( "%s - %s",
                                           otherObj->leadershipNameTag, des );
                        }
                    else {
                        des = autoSprintf( "%s %s",
                                           otherObj->leadershipNameTag, des );
                        }
                    
                    if( desToDelete != NULL ) {
                        delete [] desToDelete;
                        }
                    
                    desToDelete = des;
                    }
                

                if( otherObj != NULL && 
                    ( otherObj->dying || isSick( otherObj ) )
                    && otherObj->holdingID > 0 ) {
                    des = autoSprintf( "%s - %s %s",
                                       des,
                                       translate( "with" ),
                                       getObject( otherObj->holdingID )->
                                       description );
                    if( desToDelete != NULL ) {
                        delete [] desToDelete;
                        }
                    
                    desToDelete = des;
                    }
                }
            else if( badBiome ) {
                // we're over a bad biome
                int bInd =
                    mBadBiomeIndices.getElementIndex( mCurMouseOverBiome );
                des = mBadBiomeNames.getElementDirect( bInd );
                }
            else {
                ObjectRecord *o = getObject( idToDescribe );
                
                des = o->description;

                if( strstr( des, "origGrave" ) != NULL ) {
                    char found = false;
                    
                    for( int g=0; g<mGraveInfo.size(); g++ ) {
                        GraveInfo *gI = mGraveInfo.getElement( g );
                        
                        if( gI->worldPos.x == mCurMouseOverWorld.x &&
                            gI->worldPos.y == mCurMouseOverWorld.y ) {
                            
                            char *desNoComment = stringDuplicate( des );
                            stripDescriptionComment( desNoComment );

                            // a grave we know about
                            int years = 
                                lrint( 
                                    ( game_getCurrentTime() - 
                                      gI->creationTime ) *
                                    ourLiveObject->ageRate );

                            if( gI->creationTimeUnknown ) {
                                years = 0;
                                }

                            double currentTime = game_getCurrentTime();
                            
                            if( gI->lastMouseOverYears != -1 &&
                                currentTime - gI->lastMouseOverTime < 2 ) {
                                // continuous mouse-over
                                // don't let years tick up
                                years = gI->lastMouseOverYears;
                                gI->lastMouseOverTime = currentTime;
                                }
                            else {
                                // save it for next time
                                gI->lastMouseOverYears = years;
                                gI->lastMouseOverTime = currentTime;
                                }
                            
                            const char *yearWord;
                            if( years == 1 ) {
                                yearWord = translate( "yearAgo" );
                                }
                            else {
                                yearWord = translate( "yearsAgo" );
                                }
                            
                            char *yearsString;
                            
                            if( years >= NUM_NUMBER_KEYS ) {
                                if( years > 1000000 ) {
                                    int mil = years / 1000000;
                                    int remain = years % 1000000;
                                    int thou = remain / 1000;
                                    int extra = remain % 1000;
                                    yearsString = 
                                        autoSprintf( "%d,%d,%d", 
                                                     mil, thou, extra );
                                    }
                                else if( years > 1000 ) {
                                    yearsString = 
                                        autoSprintf( "%d,%d", 
                                                     years / 1000,
                                                     years % 1000 );
                                    }
                                else {
                                    yearsString = autoSprintf( "%d", years );
                                    }
                                }
                            else {
                                yearsString = stringDuplicate( 
                                    translate( numberKeys[ years ] ) );
                                }
                            


                            char *deathPhrase;
                            
                            if( years == 0 ) {
                                deathPhrase = stringDuplicate( "" );
                                }
                            else {
                                deathPhrase = 
                                    autoSprintf( " - %s %s %s",
                                                 translate( "died" ),
                                                 yearsString, yearWord );
                                }

                            delete [] yearsString;
                            
                            des = autoSprintf( "%s %s %s%s",
                                               desNoComment, translate( "of" ),
                                               gI->relationName,
                                               deathPhrase );
                            delete [] desNoComment;
                            delete [] deathPhrase;
                            
                            desToDelete = des;
                            found = true;
                            break;
                            }    
                        }
                    
                        

                    if( !found ) {

                        char alreadySent = false;
                        for( int i=0; i<graveRequestPos.size(); i++ ) {
                            if( equal( graveRequestPos.getElementDirect( i ),
                                       mCurMouseOverWorld ) ) {
                                alreadySent = true;
                                break;
                                }
                            }

                        if( !alreadySent ) {                            
                            char *graveMessage = 
                                autoSprintf( "GRAVE %d %d#",
                                             mCurMouseOverWorld.x,
                                             mCurMouseOverWorld.y );
                            
                            sendToServerSocket( graveMessage );
                            delete [] graveMessage;
                            graveRequestPos.push_back( mCurMouseOverWorld );
                            }
                        
                        // blank des for now
                        // avoid flicker when response arrives
                        des = stringDuplicate( "" );
                        desToDelete = des;
                        }
                    
                    }
                else if( o->isOwned ) {
                    char found = false;
                    
                    for( int g=0; g<mOwnerInfo.size(); g++ ) {
                        OwnerInfo *gI = mOwnerInfo.getElement( g );
                        
                        if( gI->worldPos.x == mCurMouseOverWorld.x &&
                            gI->worldPos.y == mCurMouseOverWorld.y ) {
                            
                            char *desNoComment = stringDuplicate( des );
                            stripDescriptionComment( desNoComment );


                            const char *personName = 
                                translate( "unknownPerson" );
                            
                            double minDist = DBL_MAX;
                            LiveObject *ourLiveObject = getOurLiveObject();
                            
                            for( int p=0; p< gI->ownerList->size(); p++ ) {
                                int pID = gI->ownerList->getElementDirect( p );
                                
                                if( pID == ourID ) {
                                    personName = translate( "YOU" );
                                    break;
                                    }
                                LiveObject *pO = getLiveObject( pID );
                                if( pO != NULL ) {
                                    double thisDist =
                                        distance( pO->currentPos,
                                                  ourLiveObject->currentPos );
                                    if( thisDist < minDist ) {
                                        minDist = thisDist;
                                        if( pO->name != NULL ) {
                                            personName = pO->name;
                                            }
                                        else {
                                            personName = 
                                                translate( "namelessPerson" );
                                            }
                                        }
                                    }
                                }
                            

                            // an owned object we know about
                            des = autoSprintf( "%s %s %s",
                                               desNoComment, 
                                               translate( "ownedBy" ),
                                               personName );
                            delete [] desNoComment;
                            
                            desToDelete = des;
                            found = true;
                            break;
                            }    
                        }

                    if( !found ) {

                        char alreadySent = false;
                        for( int i=0; i<ownerRequestPos.size(); i++ ) {
                            if( equal( ownerRequestPos.getElementDirect( i ),
                                       mCurMouseOverWorld ) ) {
                                alreadySent = true;
                                break;
                                }
                            }

                        if( !alreadySent ) {                            
                            char *ownerMessage = 
                                autoSprintf( "OWNER %d %d#",
                                             mCurMouseOverWorld.x,
                                             mCurMouseOverWorld.y );
                            
                            sendToServerSocket( ownerMessage );
                            delete [] ownerMessage;
                            ownerRequestPos.push_back( mCurMouseOverWorld );
                            }
                        
                        // blank des for now
                        // avoid flicker when response arrives
                        des = stringDuplicate( "" );
                        desToDelete = des;
                        }
                    }

                

                Homeland *h = getHomeland( mCurMouseOverWorld.x,
                                           mCurMouseOverWorld.y );
                if( h != NULL ) {
                    char *newDes = NULL;
                    
                    if( h->familyName != NULL ) {
                        newDes =
                            autoSprintf( "%s %s %s",
                                         h->familyName,
                                         translate( "family" ),
                                         des );
                        }
                    else {
                        newDes =
                            autoSprintf( "%s %s",
                                         translate( "abandoned" ),
                                         des );
                        }
                    if( newDes != NULL ) {
                        if( desToDelete != NULL ) {
                            delete [] desToDelete;
                            }
                        des = newDes;
                        desToDelete = des;
                        }
                    }
                
                if( o->toolSetIndex != -1 ) {                
                    const char *status = translate( "toolInfo" );
                    
                    char *newDes = NULL;
                    

                    if( ! o->toolLearned ) {
                        status = translate( "unlearnedToolInfo" );

                        if( totalToolSlots > 0 ) {
                            newDes = 
                                autoSprintf( "%s%d/%d %s - %s", 
                                             status, 
                                             totalToolSlots - usedToolSlots, 
                                             totalToolSlots,
                                             translate( "slotsLeft" ),
                                             des );
                            }
                        }

                    if( newDes == NULL ) {
                        newDes = autoSprintf( "%s%s", status, des );
                        }
                    
                    if( desToDelete != NULL ) {
                        delete [] desToDelete;
                        }
                    des = newDes;
                    desToDelete = des;
                    }

                }
            
            char *stringUpper = stringToUpperCase( des );

            if( desToDelete != NULL ) {
                delete [] desToDelete;
                }

            stripDescriptionComment( stringUpper );
            

            if( mCurrentDes == NULL ) {
                mCurrentDes = stringDuplicate( stringUpper );
                }
            else {
                if( strcmp( mCurrentDes, stringUpper ) != 0 ) {
                    // adding a new one to stack, fade out old
                    
                    for( int i=0; i<mOldDesStrings.size(); i++ ) {
                        float fade =
                            mOldDesFades.getElementDirect( i );
                        
                        if( fade > 0.5 ) {
                            fade -= 0.20;
                            }
                        else {
                            fade -= 0.1;
                            }
                        
                        *( mOldDesFades.getElement( i ) ) = fade;
                        if( fade <= 0 ) {
                            mOldDesStrings.deallocateStringElement( i );
                            mOldDesFades.deleteElement( i );
                            i--;
                            }
                        else if( strcmp( mCurrentDes, 
                                         mOldDesStrings.getElementDirect(i) )
                                 == 0 ) {
                            // already in stack, move to top
                            mOldDesStrings.deallocateStringElement( i );
                            mOldDesFades.deleteElement( i );
                            i--;
                            }
                        }
                    
                    mOldDesStrings.push_back( mCurrentDes );
                    mOldDesFades.push_back( 1.0f );
                    mCurrentDes = stringDuplicate( stringUpper );
                    }
                }
            
            
            setDrawColor( 0, 0, 0, 1 );
            pencilFont->drawString( stringUpper, tipPos, alignCenter );
            
            delete [] stringUpper;
            }
        else {
            if( mCurrentDes != NULL ) {
                // done with this des

                // move to top of stack
                for( int i=0; i<mOldDesStrings.size(); i++ ) {
                    if( strcmp( mCurrentDes, 
                                mOldDesStrings.getElementDirect(i) )
                        == 0 ) {
                        // already in stack, move to top
                        mOldDesStrings.deallocateStringElement( i );
                        mOldDesFades.deleteElement( i );
                        i--;
                        }
                    }
                
                mOldDesStrings.push_back( mCurrentDes );
                mOldDesFades.push_back( 1.0f );
                mCurrentDes = NULL;
                }
            }
            

        
        if( showBugMessage ) {
            
            setDrawColor( 1, 1, 1, 0.5 );
            
            drawRect( lastScreenViewCenter, 612, 252 );


            setDrawColor( 1, 1, 1, 0.5 );
            
            
            setDrawColor( 0.2, 0.2, 0.2, 0.85  );
            
            drawRect( lastScreenViewCenter, 600, 240 );
            
            setDrawColor( 1, 1, 1, 1 );

            doublePair messagePos = lastScreenViewCenter;

            messagePos.y += 200;

            switch( showBugMessage ) {
                case 1:
                    drawMessage( "bugMessage1a", messagePos );
                    break;
                case 2:
                    drawMessage( "bugMessage1b", messagePos );
                    break;
                case 3:
                    drawMessage( "bugMessage1c", messagePos );
                    break;
                }
            
            messagePos = lastScreenViewCenter;


            drawMessage( bugEmail, messagePos );
            
            messagePos.y -= 200;
            drawMessage( "bugMessage2", messagePos );
            }
        }

    
    if( vogMode ) {
        // draw again, so we can see picker
        PageComponent::base_draw( inViewCenter, inViewSize );
        }

    if( showFPS ) {
        timeMeasures[2] += game_getCurrentTime() - drawStartTime;
        }
    
    }




char nearEndOfMovement( LiveObject *inPlayer ) {
    if( inPlayer->currentSpeed == 0  ) {
        return true;
        }
    else if( inPlayer->currentPathStep >= inPlayer->pathLength - 2 ) {
        return true;
        }
    return false;
    }



void playPendingReceivedMessages( LiveObject *inPlayer ) {
    printf( "Playing %d pending received messages for %d\n", 
            inPlayer->pendingReceivedMessages.size(), inPlayer->id );
    
    for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
        readyPendingReceivedMessages.push_back( 
            inPlayer->pendingReceivedMessages.getElementDirect( i ) );
        }
    inPlayer->pendingReceivedMessages.deleteAll();
    }



void playPendingReceivedMessagesRegardingOthers( LiveObject *inPlayer ) {
    printf( "Playing %d pending received messages for %d "
            "    (skipping those that don't affect other players or map)\n", 
            inPlayer->pendingReceivedMessages.size(), inPlayer->id );

    for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
        char *message = inPlayer->pendingReceivedMessages.getElementDirect( i );
        
        if( strstr( message, "PU" ) == message ) {
            // only keep PU's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PU\n%d", &messageID );
            
            if( messageID != inPlayer->id ) {
                readyPendingReceivedMessages.push_back( message );
                }
            else {
                delete [] message;
                }
            }
        else if( strstr( message, "PM" ) == message ) {
            // only keep PM's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PM\n%d", &messageID );
            
            if( messageID != inPlayer->id ) {
                readyPendingReceivedMessages.push_back( message );
                }
            else {
                delete [] message;
                }
            }
        else {
            // not a PU, keep it no matter what (map change, etc.
            readyPendingReceivedMessages.push_back( message );
            }
        }
    inPlayer->pendingReceivedMessages.deleteAll();
    }



void dropPendingReceivedMessagesRegardingID( LiveObject *inPlayer,
                                             int inIDToDrop ) {
    for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
        char *message = inPlayer->pendingReceivedMessages.getElementDirect( i );
        char match = false;
        
        if( strstr( message, "PU" ) == message ) {
            // only keep PU's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PU\n%d", &messageID );
            
            if( messageID == inIDToDrop ) {
                match = true;
                }
            }
        else if( strstr( message, "PM" ) == message ) {
            // only keep PM's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PM\n%d", &messageID );
            
            if( messageID == inIDToDrop ) {
                match = true;
                }
            }
        
        if( match ) {
            delete [] message;
            inPlayer->pendingReceivedMessages.deleteElement( i );
            i--;
            }
        }
    }




void LivingLifePage::applyReceiveOffset( int *inX, int *inY ) {
    if( mMapGlobalOffsetSet ) {
        *inX -= mMapGlobalOffset.x;
        *inY -= mMapGlobalOffset.y;
        }
    }



int LivingLifePage::sendX( int inX ) {
    if( mMapGlobalOffsetSet ) {
        return inX + mMapGlobalOffset.x;
        }
    return inX;
    }



int LivingLifePage::sendY( int inY ) {
    if( mMapGlobalOffsetSet ) {
        return inY + mMapGlobalOffset.y;
        }
    return inY;
    }



char *LivingLifePage::getDeathReason() {
    if( mDeathReason != NULL ) {
        return stringDuplicate( mDeathReason );
        }
    else {
        return NULL;
        }
    }



void LivingLifePage::handleOurDeath( char inDisconnect ) {
    
    if( mDeathReason == NULL ) {
        mDeathReason = stringDuplicate( "" );
        }
    
    int years = (int)floor( computeCurrentAge( getOurLiveObject() ) );

    char *ageString;
    
    if( years == 1 ) {
        ageString = stringDuplicate( translate( "deathAgeOne" ) );
        }
    else {
        ageString = autoSprintf( translate( "deathAge" ), years );
        }
    
    char *partialReason = stringDuplicate( mDeathReason );
    delete [] mDeathReason;
    
    mDeathReason = autoSprintf( "%s####%s", ageString, partialReason );
    
    delete [] ageString;
    delete [] partialReason;
    

    setWaiting( false );

    if( inDisconnect ) {
        setSignal( "disconnect" );
        }
    else {
        setSignal( "died" );
        }
    
    instantStopMusic();
    // so sound tails are not still playing when we we get reborn
    fadeSoundSprites( 0.1 );
    setSoundLoudness( 0 );
    }



static char isCategory( int inID ) {
    if( inID <= 0 ) {
        return false;
        }
    
    CategoryRecord *c = getCategory( inID );
    
    if( c == NULL ) {
        return false;
        }
    if( ! c->isPattern && c->objectIDSet.size() > 0 ) {
        return true;
        }
    return false;
    }


static char isFood( int inID ) {
    if( inID <= 0 ) {
        return false;
        }
    
    ObjectRecord *o = getObject( inID );
    
    if( o->foodValue > 0 ) {
        return true;
        }
    else {
        return false;
        }
    }




static char getTransHintable( TransRecord *inTrans ) {

    if( inTrans->lastUseActor ) {
        return false;
        }
    if( inTrans->lastUseTarget ) {
        return false;
        }
    
    if( inTrans->actor >= 0 && 
        ( inTrans->target > 0 || inTrans->target == -1 ) &&
        ( inTrans->newActor > 0 || inTrans->newTarget > 0 ) ) {


        if( isCategory( inTrans->actor ) ) {
            return false;
            }
        if( isCategory( inTrans->target ) ) {
            return false;
            }
        
        if( inTrans->target == -1 && inTrans->newTarget == 0 &&
            ! isFood( inTrans->actor ) ) {
            // generic one-time-use transition
            return false;
            }

        return true;
        }
    else {
        return false;
        }
    }




static int findMainObjectID( int inObjectID ) {
    if( inObjectID <= 0 ) {
        return inObjectID;
        }
    
    ObjectRecord *o = getObject( inObjectID );
    
    if( o == NULL ) {
        return inObjectID;
        }
    
    if( o->isUseDummy ) {
        return o->useDummyParent;
        }
    else {
        return inObjectID;
        }
    }



static int getTransMostImportantResult( TransRecord *inTrans ) {
    int actor = findMainObjectID( inTrans->actor );
    int target = findMainObjectID( inTrans->target );
    int newActor = findMainObjectID( inTrans->newActor );
    int newTarget = findMainObjectID( inTrans->newTarget );

    int result = 0;
        

    if( target != newTarget &&
        newTarget > 0 &&
        actor != newActor &&
        newActor > 0 ) {
        // both actor and target change
        // we need to decide which is the most important result
        // to hint
            
        if( actor == 0 && newActor > 0 ) {
            // something new ends in your hand, that's more important
            result = newActor;
            }
        else {
            // if the trans takes one of the elements to a deeper
            // state, that's more important, starting with actor
            if( actor > 0 && 
                getObjectDepth( newActor ) > getObjectDepth( actor ) ) {
                result = newActor;
                }
            else if( target > 0 && 
                     getObjectDepth( newTarget ) > 
                     getObjectDepth( target ) ) {
                result = newTarget;
                }
            // else neither actor or target becomes deeper
            // which result is deeper?
            else if( getObjectDepth( newActor ) > 
                     getObjectDepth( newTarget ) ) {
                result = newActor;
                }
            else {
                result = newTarget;
                }
            }
        }
    else if( target != newTarget && 
             newTarget > 0 ) {
            
        result = newTarget;
        }
    else if( actor != newActor && 
             newActor > 0 ) {
            
        result = newActor;
        }
    else if( newTarget > 0 ) {
        // don't show NOTHING as a result
        result = newTarget;
        }

    return result;
    }



int LivingLifePage::getNumHints( int inObjectID ) {
    

    char sameFilter = false;
    
    if( mLastHintFilterString == NULL &&
        mHintFilterString == NULL ) {
        sameFilter = true;
        }
    else if( mLastHintFilterString != NULL &&
             mHintFilterString != NULL &&
             strcmp( mLastHintFilterString, mHintFilterString ) == 0 ) {
        sameFilter = true;
        }
    


    if( mLastHintSortedSourceID == inObjectID && sameFilter ) {
        return mLastHintSortedList.size();
        }
    
    
    mCurrentHintTargetObject[0] = 0;
    mCurrentHintTargetObject[1] = 0;


    // else need to regenerated sorted list

    mLastHintSortedSourceID = inObjectID;
    mLastHintSortedList.deleteAll();

    if( mLastHintFilterString != NULL ) {
        delete [] mLastHintFilterString;
        mLastHintFilterString = NULL;
        }

    if( mHintFilterString != NULL ) {
        mLastHintFilterString = stringDuplicate( mHintFilterString );
        }
    
    if( ! sameFilter ) {
        // new filter, clear all bookmarks
        int maxObjectID = getMaxObjectID();
        for( int i=0; i<=maxObjectID; i++ ) {
            mHintBookmarks[i] = 0;
            }
        }
    

    // heap sort
    MinPriorityQueue<TransRecord*> queue;
    
    SimpleVector<TransRecord*> *trans = getAllUses( inObjectID );
    
    
    SimpleVector<TransRecord *> unfilteredTrans;
    SimpleVector<TransRecord *> filteredTrans;
    
    if( trans != NULL )
    for( int i = 0; i<trans->size(); i++ ) {
        TransRecord *t = trans->getElementDirect( i );
        
        if( getTransHintable( t ) ) {
            unfilteredTrans.push_back( t );
            filteredTrans.push_back( t );
            }
        }


    int numFilterHits = 0;



    // old logic:
    // filter hints for held object based on steps along way to filter
    // target
    //
    // see new logic below
    if( false )
    if( mLastHintFilterString != NULL && filteredTrans.size() > 0 ) {
        
        SimpleVector<int> hitIDs = findObjectsMatchingWords(
            mLastHintFilterString,
            inObjectID,
            200,
            &numFilterHits );
        

        int numHits = hitIDs.size();

        // list of IDs that are used to make hit objects
        SimpleVector<int> precursorIDs;
        
        if( numHits > 0 ) {
            
            if( numHits < 10 ) {
                
                for( int i=0; i<numHits; i++ ) {
                    precursorIDs.push_back( hitIDs.getElementDirect( i ) );
                    }
                // go limited number of steps back
                
                SimpleVector<int> lastStep = precursorIDs;

                for( int s=0; s<10; s++ ) {
                    SimpleVector<int> oldLastStep = lastStep;
                    
                    lastStep.deleteAll();
                    
                    for( int i=0; i<oldLastStep.size(); i++ ) {
                        int oldStepID = oldLastStep.getElementDirect( i );
                        if( oldStepID == inObjectID ) {
                            // don't follow precursor chains back through
                            // our object
                            // don't care about things BEFORE out object
                            // that lead to filter target
                            continue;
                            }
                        
                        int oldStepDepth = getObjectDepth( oldStepID );


                        int numResults = 0;
                        int numRemain = 0;
                        TransRecord **prodTrans =
                            searchProduces( oldStepID, 
                                            0,
                                            200,
                                            &numResults, &numRemain );
                        
                        if( prodTrans != NULL ) {
                            
                            int shallowestTransDepth = UNREACHABLE;
                            int shallowestTransIndex = -1;

                            for( int t=0; t<numResults; t++ ) {
                                
                                int actor = prodTrans[t]->actor;
                                int target = prodTrans[t]->target;
                                
                                int transDepth = UNREACHABLE;
                                
                                if( actor > 0 ) {
                                    transDepth = getObjectDepth( actor );
                                    if( transDepth == UNREACHABLE ) {
                                        // is this a category?
                                        CategoryRecord *cat = 
                                            getCategory( actor );
                                        
                                        if( cat != NULL &&
                                            cat->objectIDSet.size() > 0 &&
                                            ! cat->isPattern ) {
                                            continue;
                                            }
                                        }
                                    }
                                if( target > 0 ) {
                                    int targetDepth = getObjectDepth( target );
                                    if( targetDepth == UNREACHABLE ) {
                                        // must be category
                                        // is this a category?
                                        CategoryRecord *cat = 
                                            getCategory( target );
                                        
                                        if( cat != NULL && 
                                            cat->objectIDSet.size() > 0 &&
                                            ! cat->isPattern ) {
                                            continue;
                                            }
                                        }
                                    if( targetDepth > transDepth ||
                                        transDepth == UNREACHABLE ) {
                                        transDepth = targetDepth;
                                        }
                                    }
                                
                                if( transDepth < shallowestTransDepth ) {
                                    shallowestTransDepth = transDepth;
                                    shallowestTransIndex = t;
                                    }
                                }
                            
                            
                            if( shallowestTransIndex != -1 &&
                                shallowestTransDepth < oldStepDepth ) {
                                int actor = 
                                    prodTrans[shallowestTransIndex]->actor;
                                int target = 
                                    prodTrans[shallowestTransIndex]->target;
                                
                                if( actor > 0 &&
                                    precursorIDs.
                                    getElementIndex( actor ) == -1 ) {

                                    precursorIDs.push_back( actor );
                                    lastStep.push_back( actor );
                                    }

                                if( target > 0 && 
                                    precursorIDs.
                                    getElementIndex( target ) == -1 ) {

                                    precursorIDs.push_back( target );
                                    lastStep.push_back( target );
                                    }
                                }
                            
                            delete [] prodTrans;
                            }
                        }
                    }
                }
            
            int numPrecursors = precursorIDs.size();

            for( int i = 0; i<filteredTrans.size(); i++ ) {
                char matchesFilter = false;
                TransRecord *t = filteredTrans.getElementDirect( i );
                
                // don't show trans that result in a hit or a precursor of a
                // hit if the trans doesn't display that hit or precursor
                // as a result when shown to user (will be confusing if
                // it only produces the precursor as a "side-effect"
                // example:  bone needle produced from stitching shoes
                //           and bone needle is a precursor of bellows
                //           but it's very odd to show the shoe-producing
                //           transition when filtering by bellows and
                //           holding two rabbit furs.
                int resultOfTrans = getTransMostImportantResult( t );
                
                for( int h=0; h<numHits; h++ ) {
                    int id = hitIDs.getElementDirect( h );    
                    
                    if( t->actor != id && t->target != id 
                        &&
                        ( resultOfTrans == id ) ) {
                        matchesFilter = true;
                        break;
                        }    
                    }
                if( matchesFilter == false ) {
                    for( int p=0; p<numPrecursors; p++ ) {
                        int id = precursorIDs.getElementDirect( p );
                        
                        if( t->actor != id && t->target != id 
                            &&
                            ( resultOfTrans == id ) ) {
                            // precursors only count if they actually
                            // make id, not just if they use it

                            // but make sure it doesn't use
                            // one of our main hits as an ingredient
                            char hitIsIngredient = false;
                            
                            int actor = t->actor;
                            int target = t->target;
                            
                            if( actor > 0 ) {
                                ObjectRecord *actorO = 
                                    getObject( actor );
                                if( actorO->isUseDummy ) {
                                    actor = actorO->useDummyParent;
                                    }
                                }
                            if( target > 0 ) {
                                ObjectRecord *targetO = 
                                    getObject( target );
                                if( targetO->isUseDummy ) {
                                    target = targetO->useDummyParent;
                                    }
                                }
                            
                            for( int h=0; h<numHits; h++ ) {
                                int hitID = hitIDs.getElementDirect( h ); 
                                
                                if( actor == hitID || 
                                    target == hitID ) {
                                    hitIsIngredient = true;
                                    break;
                                    }
                                }
                            if( ! hitIsIngredient ) {
                                matchesFilter = true;
                                break;
                                }
                            }
                        }
                    }
                
                if( ! matchesFilter ) {
                    filteredTrans.deleteElement( i );
                    i--;
                    }
                }
            }

        }
    



    // new logic:
    // show all trans leading to this target object
    if( mLastHintFilterString != NULL ) {
        
        SimpleVector<int> hitIDs = findObjectsMatchingWords(
            mLastHintFilterString,
            0, // don't filter out what we're holding
            200,
            &numFilterHits );

        if( hitIDs.size() > 0 && hitIDs.size() < 10 ) {
            filteredTrans.deleteAll();
            unfilteredTrans.deleteAll();
            
            SimpleVector<int> frontier;
            
            SimpleVector<int> alreadySeenObjects;

            frontier.push_back_other( &hitIDs );
            
            while( frontier.size() > 0 ) {
                printf( "Frontier = " );
                for( int i=0; i<frontier.size(); i++ ) {
                    printf( " [ %s ]  ",
                            getObject( frontier.getElementDirect( i ) )->
                            description );
                    }
                printf( "\n" );

                SimpleVector<int> newFrontier;
                
                for( int i=0; i<frontier.size(); i++ ) {
                    int oID = frontier.getElementDirect( i );
                    int oD = getObjectDepth( oID );
                    
                    int numResults = 0;
                    int numRemain = 0;
                    TransRecord **prodTrans =
                        searchProduces( oID, 
                                        0,
                                        200,
                                        &numResults, &numRemain );
                    
                    if( prodTrans != NULL ) {
                        TransRecord *minDepthTrans = NULL;
                        int minDepth = UNREACHABLE;
                        
                        for( int r=0; r<numResults; r++ ) {
                            
                            int actor = prodTrans[r]->actor;
                            int target = prodTrans[r]->target;

                            int actorD = 0;
                            int targetD = 0;
                            
                            if( actor > 0 ) {
                                actorD = getObjectDepth( actor );
                                }
                            if( target > 0 ) {
                                targetD = getObjectDepth( target );
                                }

                            if( actor >= -1 && 
                                ( actor <= 0 || actorD < oD ) 
                                &&
                                target != 0 && targetD < oD 
                                &&
                                filteredTrans.getElementIndex( prodTrans[r] ) 
                                == -1 ) {
                                
                                if( prodTrans[r]->lastUseTarget ||
                                    prodTrans[r]->lastUseActor ) {
                                    // skip last use transitions
                                    continue;
                                    }

                                // skip dummy versions of transitions, too
                                if( actor > 0 &&
                                    ( getObject( actor )->isUseDummy ||
                                      getObject( actor )->isVariableDummy ) ) {
                                    continue;
                                    }
                                if( target > 0 &&
                                    ( getObject( target )->isUseDummy ||
                                      getObject( target )->isVariableDummy ) ) {
                                    continue;
                                    }                                
                             
                                
                                int maxDepth = actorD;
                                if( targetD > maxDepth ) {
                                    maxDepth = targetD;
                                    }
                                
                                if( maxDepth < minDepth ) {
                                    minDepth = maxDepth;
                                    minDepthTrans = prodTrans[r];
                                    }
                                }
                            }
                        if( minDepthTrans != NULL ) {
                            int actor = minDepthTrans->actor;
                            int target = minDepthTrans->target;

                            if( actor > 0 ) {
                                if( alreadySeenObjects.getElementIndex( actor )
                                    == -1 ) {
                                    newFrontier.push_back( actor );
                                    alreadySeenObjects.push_back( actor );
                                    }
                                }
                            
                            if( target > 0 &&
                                alreadySeenObjects.getElementIndex( target )
                                == -1 ) {
                                newFrontier.push_back( target );
                                alreadySeenObjects.push_back( target );
                                }
                            
                            filteredTrans.push_back( minDepthTrans );
                            }
                        
                        delete [] prodTrans;
                        }
                    }
                frontier.deleteAll();
                frontier.push_back_other( &newFrontier );
                }
            unfilteredTrans.push_back_other( &filteredTrans );
            }
        else {
            // matches too many things, ignore
            numFilterHits = 0;
            }
        }
    


    int numTrans = filteredTrans.size();

    int numRelevant = numTrans;

      // for now, just leave it empty
    if( false &&
        numTrans == 0 && unfilteredTrans.size() > 0 ) {
        
        // after filtering, no transititions are left
        // show all trans instead
        for( int i = 0; i<unfilteredTrans.size(); i++ ) {
            filteredTrans.push_back( unfilteredTrans.getElementDirect( i ) );
            }
        numTrans = filteredTrans.size();
        }
    
    

    int lastSheet = NUM_HINT_SHEETS - 1;
    
    if( mPendingFilterString != NULL ) {
        delete [] mPendingFilterString;
        mPendingFilterString = NULL;
        }


    mHintFilterStringNoMatch = false;

    if( mLastHintFilterString != NULL ) {

        if( mPendingFilterString != NULL ) {
            delete [] mPendingFilterString;
            mPendingFilterString = NULL;
            }
        
        if( numRelevant == 0 || numFilterHits == 0 ) {
            const char *key = "notRelevant";
            char *reasonString = NULL;
            if( numFilterHits == 0 && unfilteredTrans.size() > 0 ) {
                // no match because object named in filter does not
                // exist
                key = "noMatch";
                reasonString = stringDuplicate( translate( key ) );
                mHintFilterStringNoMatch = true;
                }
            else {
                const char *formatString = translate( key );
                
                
                char *objString = getDisplayObjectDescription( inObjectID );
                reasonString = autoSprintf( formatString, objString );
                
                delete [] objString;
                }
            
            

            mPendingFilterString = autoSprintf( "%s %s %s",
                                                translate( "making" ),
                                                mLastHintFilterString,
                                                reasonString );
            delete [] reasonString;
            }
        else {    
            mPendingFilterString = autoSprintf( "%s %s",
                                                translate( "making" ),
                                                mLastHintFilterString );
            }
        
        }
    else {
        mHintTargetOffset[ lastSheet ] = mHintHideOffset[ lastSheet ];
        }




    // skip any that repeat exactly the same string
    // (example:  goose pond in different states)
    SimpleVector<char *> otherActorStrings;
    SimpleVector<char *> otherTargetStrings;
    

    for( int i=0; i<numTrans; i++ ) {
        TransRecord *tr = filteredTrans.getElementDirect( i );
        
        int depth = 0;
            
        if( tr->actor > 0 && tr->actor != inObjectID ) {
            depth = getObjectDepth( tr->actor );
            }
        else if( tr->target > 0 && tr->target != inObjectID ) {
            depth = getObjectDepth( tr->target );
            }


        if( mLastHintFilterString != NULL ) {
            // new logic:
            // show all trans leading to this target object
            
            // sort by largest depth

            if( tr->actor > 0 ) {
                depth = getObjectDepth( tr->actor );
                }

            if( tr->target > 0 ) {
                int depthT = getObjectDepth( tr->target );
                if( depthT > depth ) {
                    depth = depthT;
                    }
                }
            }
        

            
            
        char stringAlreadyPresent = false;
        
        // new logic:
        // show all that we found if filtering
        // but still remove duplicates if we're not filtering
        if( mLastHintFilterString == NULL )
        if( tr->actor > 0 && tr->actor != inObjectID ) {
            ObjectRecord *otherObj = getObject( tr->actor );
                
            char *trimmedDesc = stringDuplicate( otherObj->description );
            stripDescriptionComment( trimmedDesc );

            for( int s=0; s<otherActorStrings.size(); s++ ) {
                    
                if( strcmp( trimmedDesc, 
                            otherActorStrings.getElementDirect( s ) )
                    == 0 ) {
                        
                    stringAlreadyPresent = true;
                    break;
                    }    
                }

            if( !stringAlreadyPresent ) {
                otherActorStrings.push_back( trimmedDesc );
                }
            else {
                delete [] trimmedDesc;
                }
            }
            
        // new logic:
        // show all that we found if filtering
        // but still remove duplicates if we're not filtering
        if( mLastHintFilterString == NULL )
        if( tr->target > 0 && tr->target != inObjectID ) {
            ObjectRecord *otherObj = getObject( tr->target );
                
            char *trimmedDesc = stringDuplicate( otherObj->description );
            stripDescriptionComment( trimmedDesc );

            for( int s=0; s<otherTargetStrings.size(); s++ ) {
                    
                if( strcmp( trimmedDesc, 
                            otherTargetStrings.getElementDirect( s ) )
                    == 0 ) {
                        
                    stringAlreadyPresent = true;
                    break;
                    }    
                }

            if( !stringAlreadyPresent ) {
                otherTargetStrings.push_back( trimmedDesc );
                }
            else {
                delete [] trimmedDesc;
                }
            }

            
        if( !stringAlreadyPresent ) {
            queue.insert( tr, depth );
            }
        }
    
    otherActorStrings.deallocateStringElements();
    otherTargetStrings.deallocateStringElements();

    int numInQueue = queue.size();
    
    for( int i=0; i<numInQueue; i++ ) {
        mLastHintSortedList.push_back( queue.removeMin() );
        }
    
    if( mLastHintFilterString != NULL && numFilterHits > 0 ) {
        // reverse the order
        SimpleVector< TransRecord *> revList( mLastHintSortedList.size() );
        
        for( int i=mLastHintSortedList.size()-1; i >= 0; i-- ) {
            revList.push_back( mLastHintSortedList.getElementDirect( i ) );
            }
        mLastHintSortedList.deleteAll();
        mLastHintSortedList.push_back_other( &revList );
        }
    
    
    return mLastHintSortedList.size();
    }



static double getLongestLine( char *inMessage ) {
    
    double longestLine = 0;
    
    int numLines;
    char **lines = split( inMessage, 
                          "#", &numLines );
    
    for( int l=0; l<numLines; l++ ) {
        double len = handwritingFont->measureString( lines[l] );
        
        if( len > longestLine ) {
            longestLine = len;
                    }
        delete [] lines[l];
        }
    delete [] lines;
    
    return longestLine;
    }




char *LivingLifePage::getHintMessage( int inObjectID, int inIndex,
                                      int inDoNotPointAtThis ) {

    if( inObjectID != mLastHintSortedSourceID ) {
        getNumHints( inObjectID );
        }

    TransRecord *found = NULL;

    if( inIndex < mLastHintSortedList.size() ) {    
        found = mLastHintSortedList.getElementDirect( inIndex );
        }
    

    
    if( found != NULL ) {
        int actor = findMainObjectID( found->actor );
        int target = findMainObjectID( found->target );
        int newActor = findMainObjectID( found->newActor );

        int result = getTransMostImportantResult( found );
        
        
        char *actorString;
        
        if( actor > 0 ) {
            actorString = stringToUpperCase( getObject( actor )->description );
            stripDescriptionComment( actorString );
            }
        else if( actor == 0 || actor == -2 ) {
            // show bare hand for default actions too
            actorString = stringDuplicate( translate( "bareHandHint" ) );
            }
        else if( actor == -1 ) {
            actorString = stringDuplicate( translate( "timeHint" ) );
            }
        else {
            actorString = stringDuplicate( "" );
            }

        
        char eventually = false;
        
        char *targetString;
        
        if( target > 0 ) {
            targetString = 
                stringToUpperCase( getObject( target )->description );
            stripDescriptionComment( targetString );
            }
        else if( target == -1 && actor > 0 ) {
            ObjectRecord *actorObj = getObject( actor );
            
            if( actorObj->foodValue ) {
                targetString = stringDuplicate( translate( "eatHint" ) );
                
                if( result == 0 && 
                    actor == newActor &&
                    actorObj->numUses > 1 ) {
                    // see if there's a last-use transition and give hint
                    // about what it produces, in the end

                    int lastDummy = 
                        actorObj->useDummyIDs[ 0 ];
                    
                    TransRecord *lastUseTrans = getTrans( lastDummy, -1 );
                    
                    if( lastUseTrans != NULL ) {
                        if( lastUseTrans->newActor > 0 ) {
                            result = findMainObjectID( lastUseTrans->newActor );
                            eventually = true;
                            }
                        else if( lastUseTrans->newTarget > 0 ) {
                            result = 
                                findMainObjectID( lastUseTrans->newTarget );
                            eventually = true;
                            }
                        }    
                    }
                }
            else {
                targetString = stringDuplicate( translate( "bareGroundHint" ) );
                }
            }
        else {
            targetString = stringDuplicate( "" );
            }


        mCurrentHintTargetObject[0] = 0;
        mCurrentHintTargetObject[1] = 0;

        // never show visual pointer toward what we're holding
        // we handle this elsewhere too, so just obey inDoNotPoint here
        if( target > 0 && 
            target != inDoNotPointAtThis ) {
            mCurrentHintTargetObject[1] = target;
            }
        if( actor > 0 &&
                 actor != inDoNotPointAtThis ) {
            mCurrentHintTargetObject[0] = actor;
            }
        
        if( actor > 0 && target > 0 &&
            actor == target ) {
            // both actor and target are same
            // show pointer to ones that are on the ground
            mCurrentHintTargetObject[0] = actor;
            mCurrentHintTargetObject[1] = 0;
            }
        else if( actor == 0 && target > 0 && 
                 target != inDoNotPointAtThis ) {
            // bare hand action
            // show target even if it matches what we're giving hints about
            mCurrentHintTargetObject[0] = target;
            mCurrentHintTargetObject[1] = 0;
            }
        
        

        char *resultString;
        
        if( result > 0 ) {
            resultString = 
                stringToUpperCase( getObject( result )->description );
            }
        else {
            resultString = stringDuplicate( translate( "nothingHint" ) );
            }
        
        stripDescriptionComment( resultString );
        
        if( eventually ) {
            char *old = resultString;
            resultString = autoSprintf( "%s %s", old, 
                                        translate( "eventuallyHint" ) );
            delete [] old;
            }

        
        char *fullString =
            autoSprintf( "%s#%s#%s", actorString,
                         targetString, 
                         resultString );
        
        delete [] actorString;
        delete [] targetString;
        delete [] resultString;
        
        return fullString;
        }
    else {
        return stringDuplicate( translate( "noHint" ) );
        }
    }



// inNewID > 0
static char shouldCreationSoundPlay( int inOldID, int inNewID ) {
    if( inOldID == inNewID ) {
        // no change
        return false;
        }
    

    // make sure this is really a fresh creation
    // of newID, and not a cycling back around
    // for a reusable object
    
    // also not useDummies that have the same
    // parent
    char sameParent = false;
    
    ObjectRecord *obj = getObject( inNewID );

    if( obj->creationSound.numSubSounds == 0 ) {
        return false;
        }

    if( inOldID > 0 && inNewID > 0 ) {
        ObjectRecord *oldObj = getObject( inOldID );
        
        if( obj->isUseDummy &&
            oldObj->isUseDummy &&
            obj->useDummyParent ==
            oldObj->useDummyParent ) {
            sameParent = true;
            }
        else if( obj->numUses > 1 
                 &&
                 oldObj->isUseDummy 
                 &&
                 oldObj->useDummyParent
                 == inNewID ) {
            sameParent = true;
            }
        else if( oldObj->numUses > 1 
                 &&
                 obj->isUseDummy 
                 &&
                 obj->useDummyParent
                 == inOldID ) {
            sameParent = true;
            }
        }
    
    if( ! sameParent 
        &&
        ( ! obj->creationSoundInitialOnly
          ||
          inOldID <= 0
          ||
          ( ! isSpriteSubset( inOldID, inNewID ) ) ) ) {
        return true;
        }

    return false;
    }


static char checkIfHeldContChanged( LiveObject *inOld, LiveObject *inNew ) {    
                    
    if( inOld->numContained != inNew->numContained ) {
        return true;
        }
    else {
        for( int c=0; c<inOld->numContained; c++ ) {
            if( inOld->containedIDs[c] != 
                inNew->containedIDs[c] ) {
                return true;
                }
            if( inOld->subContainedIDs[c].size() != 
                inNew->subContainedIDs[c].size() ) {
                return true;
                }
            for( int s=0; s<inOld->subContainedIDs[c].size();
                 s++ ) {
                if( inOld->subContainedIDs[c].
                    getElementDirect( s ) !=
                    inNew->subContainedIDs[c].
                    getElementDirect( s ) ) {
                    return true;
                    }
                }
            }
        }
    return false;
    }




void LivingLifePage::sendBugReport( int inBugNumber ) {
    char *bugString = stringDuplicate( "" );

    if( lastMessageSentToServer != NULL ) {
        char *temp = bugString;
        bugString = autoSprintf( "%s   Just sent: [%s]",
                                 temp, lastMessageSentToServer );
        delete [] temp;
        }
    if( nextActionMessageToSend != NULL ) {
        char *temp = bugString;
        bugString = autoSprintf( "%s   Waiting to send: [%s]",
                                 temp, nextActionMessageToSend );
        delete [] temp;
        }
    
    // clear # terminators from message
    char *spot = strstr( bugString, "#" );
    
    while( spot != NULL ) {
        spot[0] = ' ';
        spot = strstr( bugString, "#" );
        }
    
    
    char *bugMessage = autoSprintf( "BUG %d %s#", inBugNumber, bugString );
    
    delete [] bugString;

    sendToServerSocket( bugMessage );
    delete [] bugMessage;
    
    if( ! SettingsManager::getIntSetting( "reportWildBugToUser", 1 ) ) {
        return;
        }

    FILE *f = fopen( "stdout.txt", "r" );

    int recordGame = SettingsManager::getIntSetting( "recordGame", 0 );
    
    if( f != NULL ) {
        // stdout.txt exists
        
        printf( "Bug report sent, telling user to email files to us.\n" );
        
        fclose( f );

        showBugMessage = 3;
        
        if( recordGame ) {
            showBugMessage = 2;
            }
        }
    else if( recordGame ) {
        printf( "Bug report sent, telling user to email files to us.\n" );
        showBugMessage = 1;
        }
    
    }



void LivingLifePage::endExtraObjectMove( int inExtraIndex ) {
    int i = inExtraIndex;

    ExtraMapObject *o = mMapExtraMovingObjects.getElement( i );
    o->moveOffset.x = 0;
    o->moveOffset.y = 0;
    o->moveSpeed = 0;
                
    if( o->curAnimType != ground ) {
                        
        o->lastAnimType = o->curAnimType;
        o->curAnimType = ground;
        o->lastAnimFade = 1;
                    
        o->animationLastFrameCount = o->animationFrameCount;
                    
        o->animationFrameCount = 0;
        }

    GridPos worldPos = 
        mMapExtraMovingObjectsDestWorldPos.getElementDirect( i );
            
    int mapI = getMapIndex( worldPos.x, worldPos.y );
            
    if( mapI != -1 ) {
        // put it in dest
        putInMap( mapI, o );
        mMap[ mapI ] = 
            mMapExtraMovingObjectsDestObjectIDs.getElementDirect( i );
        }
            
    mMapExtraMovingObjects.deleteElement( i );
    mMapExtraMovingObjectsDestWorldPos.deleteElement( i );
    mMapExtraMovingObjectsDestObjectIDs.deleteElement( i );
    }



doublePair LivingLifePage::getPlayerPos( LiveObject *inPlayer ) {
    if( inPlayer->heldByAdultID != -1 ) {
        LiveObject *adult = getLiveObject( inPlayer->heldByAdultID );
        
        if( adult != NULL ) {
            return adult->currentPos;
            }
        }
    return inPlayer->currentPos;
    }




void LivingLifePage::displayGlobalMessage( char *inMessage ) {
    
    char *upper = stringToUpperCase( inMessage );
                
    char found;
    
    char *lines = replaceAll( upper, "**", "##", &found );
    delete [] upper;
    
    char *spaces = replaceAll( lines, "_", " ", &found );
    
    delete [] lines;
    
    
    mGlobalMessageShowing = true;
    mGlobalMessageStartTime = game_getCurrentTime();
    
    if( mLiveTutorialSheetIndex >= 0 ) {
        mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
            mTutorialHideOffset[ mLiveTutorialSheetIndex ];
        }
    mLiveTutorialSheetIndex ++;
    
    if( mLiveTutorialSheetIndex >= NUM_HINT_SHEETS ) {
        mLiveTutorialSheetIndex -= NUM_HINT_SHEETS;
        }
    mTutorialMessage[ mLiveTutorialSheetIndex ] = 
        stringDuplicate( spaces );
    
    // other tutorial messages don't need to be destroyed
    mGlobalMessagesToDestroy.push_back( 
        (char*)( mTutorialMessage[ mLiveTutorialSheetIndex ] ) );
    
    mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
        mTutorialHideOffset[ mLiveTutorialSheetIndex ];
    
    mTutorialTargetOffset[ mLiveTutorialSheetIndex ].y -= 100;
    
    double longestLine = getLongestLine( 
        (char*)( mTutorialMessage[ mLiveTutorialSheetIndex ] ) );
    
    mTutorialExtraOffset[ mLiveTutorialSheetIndex ].x = longestLine;
    
    
    delete [] spaces;
    }




void LivingLifePage::setNewCraving( int inFoodID, int inYumBonus ) {
    char *foodDescription = 
        stringToUpperCase( getObject( inFoodID )->description );
                
    stripDescriptionComment( foodDescription );

    char *message = 
        autoSprintf( "%s: %s (+%d)", translate( "craving"), 
                     foodDescription, inYumBonus );
    
    delete [] foodDescription;
    
    
    if( mLiveCravingSheetIndex > -1 ) {
        // hide old craving sheet
        mCravingTargetOffset[ mLiveCravingSheetIndex ] =
            mCravingHideOffset[ mLiveCravingSheetIndex ];
        }
    mLiveCravingSheetIndex ++;
    
    if( mLiveCravingSheetIndex >= NUM_HINT_SHEETS ) {
        mLiveCravingSheetIndex -= NUM_HINT_SHEETS;
        }
    
    if( mCravingMessage[ mLiveCravingSheetIndex ] != NULL ) {
        delete [] mCravingMessage[ mLiveCravingSheetIndex ];
        mCravingMessage[ mLiveCravingSheetIndex ] = NULL;
        }

    mCravingMessage[ mLiveCravingSheetIndex ] = message;
    
    mCravingTargetOffset[ mLiveCravingSheetIndex ] =
        mCravingHideOffset[ mLiveCravingSheetIndex ];
    
    mCravingTargetOffset[ mLiveCravingSheetIndex ].y += 64;
    
    double longestLine = getLongestLine( 
        (char*)( mCravingMessage[ mLiveCravingSheetIndex ] ) );
    
    mCravingExtraOffset[ mLiveCravingSheetIndex ].x = longestLine;
    }



// color list from here:
// https://sashat.me/2017/01/11/list-of-20-simple-distinct-colors/

#define NUM_BADGE_COLORS 17
static const char *badgeColors[NUM_BADGE_COLORS] = { "#e6194B", 
                                                     "#3cb44b", 
                                                     "#ffe119", 
                                                     "#4363d8", 
                                                     "#f58231",
                                                     
                                                     "#42d4f4", 
                                                     "#f032e6", 
                                                     "#fabebe", 
                                                     "#469990",
                                                     "#e6beff", 
                                                     
                                                     "#9A6324", 
                                                     "#fffac8", 
                                                     "#800000", 
                                                     "#aaffc3", 
                                                     "#000075", 
                                                     
                                                     "#a9a9a9", 
                                                     "#ffffff" };



static char justHitTab = false;

        
void LivingLifePage::step() {
    
    if( isAnySignalSet() ) {
        return;
        }
    

    if( apocalypseInProgress && apocalypseDisplayProgress < 1.0 ) {
        double stepSize = frameRateFactor / ( apocalypseDisplaySeconds * 60.0 );
        apocalypseDisplayProgress += stepSize;
        
        if( apocalypseDisplayProgress >= 1.0 ) {
            apocalypseDisplayProgress = 1.0;
            }
        }
    
    if( mRemapPeak > 0 ) {
        if( mRemapDelay < 1 ) {
            double stepSize = 
                frameRateFactor / ( remapDelaySeconds * 60.0 );
            mRemapDelay += stepSize;
            }
        else {

            double stepSize = 
                mRemapDirection * frameRateFactor / ( remapPeakSeconds * 60.0 );
        
            mCurrentRemapFraction += stepSize;
            
            if( stepSize > 0 && mCurrentRemapFraction >= mRemapPeak ) {
                mCurrentRemapFraction = mRemapPeak;
                mRemapDirection *= -1;
                }
            if( stepSize < 0 && mCurrentRemapFraction <= 0 ) {
                mCurrentRemapFraction = 0;
                mRemapPeak = 0;
                }
            if( takingPhoto ) {
                // stop remapping briefly during photo
                setRemapFraction( 0 );
                }
            else {
                setRemapFraction( mCurrentRemapFraction );
                }
            }
        }
    

    if( mouseDown ) {
        mouseDownFrames++;
        }
    
    double pageLifeTime = game_getCurrentTime() - mPageStartTime;

    if( mServerSocket == -1 ) {
        serverSocketConnected = false;
        connectionMessageFade = 1.0f;
        
        // don't keep looping to reconnect on instant hard-fail
        if( ! serverSocketHardFail ) {    
            mServerSocket = openSocketConnection( serverIP, serverPort );
            timeLastMessageSent = game_getCurrentTime();
            
            if( mServerSocket == -1 ) {
                // instant hard-fail, probably from bad hostname that
                // gets looked up right away.
                serverSocketHardFail = true;
                }
            }
        
        
        if( mServerSocket == -1 ) {
            // hard fail condition
            
            if( pageLifeTime >= 1 ) {
                // they have seen "CONNECTING" long enough
                
                setWaiting( false );
                setSignal( "connectionFailed" );
                }
            }
        
        
        return;
        }
    

    
    
    if( pageLifeTime < 1 ) {
        // let them see CONNECTING message for a bit
        return;
        }
    

    if( serverSocketConnected ) {
        // we've heard from server, not waiting to connect anymore
        setWaiting( false );
        }
    else {
        
        if( pageLifeTime > 10 ) {
            // having trouble connecting.
            closeSocket( mServerSocket );
            mServerSocket = -1;

            setWaiting( false );
            setSignal( "connectionFailed" );
            
            return;
            }
        }
    
    
    double messageProcessStartTime = showFPS ? game_getCurrentTime() : 0;
    
    // first, read all available data from server
    char readSuccess = readServerSocketFull( mServerSocket );
    
    if( showFPS ) {
        timeMeasures[0] += game_getCurrentTime() - messageProcessStartTime;
        }
    


    if( ! readSuccess ) {
        
        if( serverSocketConnected ) {    
            double connLifeTime = game_getCurrentTime() - connectedTime;
            
            if( connLifeTime < 1 ) {
                // let player at least see waiting page
                // avoid flicker
                return;
                }
            }
        

        closeSocket( mServerSocket );
        mServerSocket = -1;

        if( mFirstServerMessagesReceived  ) {
            
            if( mDeathReason != NULL ) {
                delete [] mDeathReason;
                }
            mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );
            
            handleOurDeath( true );
            }
        else {
            setWaiting( false );
            
            if( userReconnect ) {
                setSignal( "reconnectFailed" );
                }
            else {
                setSignal( "loginFailed" );
                }
            }
        return;
        }



    double updateStartTime = showFPS ? game_getCurrentTime() : 0;

    
    if( mLastMouseOverID != 0 ) {
        mLastMouseOverFade -= 0.01 * frameRateFactor;
        
        if( mLastMouseOverFade < 0 ) {
            mLastMouseOverID = 0;
            }
        }
    

    if( mGlobalMessageShowing ) {
        
        if( game_getCurrentTime() - mGlobalMessageStartTime > 10 ) {
            mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
                mTutorialHideOffset[ mLiveTutorialSheetIndex ];

            if( mTutorialPosOffset[ mLiveTutorialSheetIndex ].y ==
                mTutorialHideOffset[ mLiveTutorialSheetIndex ].y ) {
                // done hiding
                mGlobalMessageShowing = false;
                mGlobalMessagesToDestroy.deallocateStringElements();
                }
            }
        }
    

    // move moving objects
    int numCells = mMapD * mMapD;
    
    for( int i=0; i<numCells; i++ ) {
                
        if( mMapMoveSpeeds[i] > 0 &&
            ( mMapMoveOffsets[ i ].x != 0 ||
              mMapMoveOffsets[ i ].y != 0  ) ) {

            
            doublePair nullOffset = { 0, 0 };
                    

            doublePair delta = sub( nullOffset, 
                                    mMapMoveOffsets[ i ] );
                    
            double step = frameRateFactor * mMapMoveSpeeds[ i ] / 60.0;
                    
            if( length( delta ) < step ) {
                        
                mMapMoveOffsets[ i ].x = 0;
                mMapMoveOffsets[ i ].y = 0;
                mMapMoveSpeeds[ i ] = 0;
                
                if( mMapCurAnimType[ i ] != ground ) {
                        
                    mMapLastAnimType[ i ] = mMapCurAnimType[ i ];
                    mMapCurAnimType[ i ] = ground;
                    mMapLastAnimFade[ i ] = 1;
                    
                    mMapAnimationLastFrameCount[ i ] =
                        mMapAnimationFrameCount[ i ];
                    
                    mMapAnimationFrozenRotFrameCount[ i ] = 
                        mMapAnimationLastFrameCount[ i ];
                    
                    }
                }
            else {
                mMapMoveOffsets[ i ] =
                    add( mMapMoveOffsets[ i ],
                         mult( normalize( delta ), step ) );
                }
            }
        }
    
    
    // step extra moving objects
    for( int i=0; i<mMapExtraMovingObjects.size(); i++ ) {
        
        ExtraMapObject *o = mMapExtraMovingObjects.getElement( i );
        
        doublePair nullOffset = { 0, 0 };
                    

        doublePair delta = sub( nullOffset, o->moveOffset );
        
        double step = frameRateFactor * o->moveSpeed / 60.0;
                    
        if( length( delta ) < step ) {
            // reached dest
            
            endExtraObjectMove( i );
            i--;
            }
        else {
            o->moveOffset =
                add( o->moveOffset,
                     mult( normalize( delta ), step ) );
            }
        }
    



    if( mCurMouseOverID > 0 && ! mCurMouseOverSelf ) {
        mCurMouseOverFade += 0.2 * frameRateFactor;
        if( mCurMouseOverFade >= 1 ) {
            mCurMouseOverFade = 1.0;
            }
        }
    
    for( int i=0; i<mPrevMouseOverSpotFades.size(); i++ ) {
        float f = mPrevMouseOverSpotFades.getElementDirect( i );
        
        f -= 0.1 * frameRateFactor;
        
        
        if( f <= 0 ) {
            mPrevMouseOverSpotFades.deleteElement( i );
            mPrevMouseOverSpots.deleteElement( i );
            mPrevMouseOverSpotsBehind.deleteElement( i );
            i--;
            }
        else {
            *( mPrevMouseOverSpotFades.getElement( i ) ) = f;
            }
        }
    
    
    
    if( mCurMouseOverCell.x != -1 ) {
        
        LiveObject *ourObject = getOurLiveObject();

        // we're not mousing over any object or person
        if( mCurMouseOverID == 0 &&
            ! mCurMouseOverPerson &&
            // AND we're not moving AND we're holding something
            ourObject != NULL &&
            ! ourObject->inMotion &&
            ourObject->holdingID > 0 ) {
            // fade cell in
            mCurMouseOverCellFade += 
                mCurMouseOverCellFadeRate * frameRateFactor;
            
            if( mCurMouseOverCellFade >= 1 ) {
                mCurMouseOverCellFade = 1.0;
                }
            }
        else {
            // fade cell out
            mCurMouseOverCellFade -= 0.1 * frameRateFactor;
            if( mCurMouseOverCellFade < 0 ) {
                mCurMouseOverCellFade = 0;
                }
            }
        }
    
    for( int i=0; i<mPrevMouseOverCellFades.size(); i++ ) {
        float f = mPrevMouseOverCellFades.getElementDirect( i );
        
        f -= 0.1 * frameRateFactor;
        
        
        if( f <= 0 ) {
            mPrevMouseOverCellFades.deleteElement( i );
            mPrevMouseOverCells.deleteElement( i );
            i--;
            }
        else {
            *( mPrevMouseOverCellFades.getElement( i ) ) = f;
            }
        }


    for( int i=0; i<mPrevMouseClickCellFades.size(); i++ ) {
        float f = mPrevMouseClickCellFades.getElementDirect( i );
        
        f -= 0.02 * frameRateFactor;
        
        
        if( f <= 0 ) {
            mPrevMouseClickCellFades.deleteElement( i );
            mPrevMouseClickCells.deleteElement( i );
            i--;
            }
        else {
            *( mPrevMouseClickCellFades.getElement( i ) ) = f;
            }
        }
    
    
    if( ! equal( mNotePaperPosOffset, mNotePaperPosTargetOffset ) ) {
        doublePair delta = 
            sub( mNotePaperPosTargetOffset, mNotePaperPosOffset );
        
        double d = distance( mNotePaperPosTargetOffset, mNotePaperPosOffset );
        
        
        if( d <= 1 ) {
            mNotePaperPosOffset = mNotePaperPosTargetOffset;
            }
        else {
            int speed = frameRateFactor * 4;

            if( d < 8 ) {
                speed = lrint( frameRateFactor * d / 2 );
                }

            if( speed > d ) {
                speed = floor( d );
                }

            if( speed < 1 ) {
                speed = 1;
                }
            
            doublePair dir = normalize( delta );
            
            mNotePaperPosOffset = 
                add( mNotePaperPosOffset,
                     mult( dir, speed ) );
            }
        
        if( equal( mNotePaperPosTargetOffset, mNotePaperHideOffset ) ) {
            // fully hidden, clear erased stuff
            mLastKnownNoteLines.deallocateStringElements();
            mErasedNoteChars.deleteAll();
            mErasedNoteCharOffsets.deleteAll();
            mErasedNoteCharFades.deleteAll();
            }
        }
    
    
    
    LiveObject *ourObject = getOurLiveObject();

    if( ourObject != NULL ) {    
        
        for( int j=0; j<2; j++ ) {
            char tooClose = false;
            double homeDist = 0;
            char temporary = false;
            char tempPerson = false;
            const char *tempPersonKey = NULL;
            
            int homeArrow = getHomeDir( ourObject->currentPos, &homeDist,
                                        &tooClose, &temporary, &tempPerson, 
                                        &tempPersonKey, j );
            
            if( ! apocalypseInProgress && homeArrow != -1 && ! tooClose ) {
                mHomeSlipPosTargetOffset[j].y = 
                    mHomeSlipHideOffset[j].y + mHomeSlipShowDelta[j];
                
                char longDistance = homeDist > 1000;
                
                if( longDistance ) {
                    if( j == 0 ) {
                        mHomeSlipPosTargetOffset[j].y += 20;
                        }
                    else {
                        mHomeSlipPosTargetOffset[j].y += 20;
                        }
                    }
                if( temporary || 
                    ( ( personHintEverDrawn[j] || mapHintEverDrawn[j] ) 
                      && longDistance ) ) {
                    if( j == 0 ) {
                        mHomeSlipPosTargetOffset[j].y += 20;
                        }
                    else {
                        mHomeSlipPosTargetOffset[j].y += 20;
                        }
                    }
                }
            else {
                mHomeSlipPosTargetOffset[j].y = mHomeSlipHideOffset[j].y;
                }
            }
        
        int cm = ourObject->currentMouseOverClothingIndex;
        if( cm != -1 ) {
            ourObject->clothingHighlightFades[ cm ] 
                += 0.2 * frameRateFactor;
            if( ourObject->clothingHighlightFades[ cm ] >= 1 ) {
                ourObject->clothingHighlightFades[ cm ] = 1.0;
                }
            }
        for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
            if( c != cm ) {
                ourObject->clothingHighlightFades[ c ]
                    -= 0.1 * frameRateFactor;
                if( ourObject->clothingHighlightFades[ c ] < 0 ) {
                    ourObject->clothingHighlightFades[ c ] = 0;
                    }
                }
            }
        }



    // update yum slip positions
    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {
        
        if( ! equal( mYumSlipPosOffset[i], mYumSlipPosTargetOffset[i] ) ) {
            doublePair delta = 
                sub( mYumSlipPosTargetOffset[i], mYumSlipPosOffset[i] );
            
            double d = 
                distance( mYumSlipPosTargetOffset[i], mYumSlipPosOffset[i] );
            
            
            if( d <= 1 ) {
                mYumSlipPosOffset[i] = mYumSlipPosTargetOffset[i];
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }
                
                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mYumSlipPosOffset[i] = 
                    add( mYumSlipPosOffset[i],
                         mult( dir, speed ) );
                }        
            }
        }
    

    
    // update home slip positions
    for( int j=0; j<2; j++ )
    if( ! equal( mHomeSlipPosOffset[j], mHomeSlipPosTargetOffset[j] ) ) {
        doublePair delta = 
            sub( mHomeSlipPosTargetOffset[j], mHomeSlipPosOffset[j] );
        
        double d = distance( mHomeSlipPosTargetOffset[j], 
                             mHomeSlipPosOffset[j] );
        
        
        if( d <= 1 ) {
            mHomeSlipPosOffset[j] = mHomeSlipPosTargetOffset[j];
            if( equal( mHomeSlipPosTargetOffset[j], mHomeSlipHideOffset[j] ) ) {
                // fully hidden
                // clear all arrow states
                for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
                    mHomeArrowStates[j][i].solid = false;
                    mHomeArrowStates[j][i].fade = 0;
                    }
                mapHintEverDrawn[j] = false;
                personHintEverDrawn[j] = false;
                
                // clear old dist strings too
                mPreviousHomeDistStrings[j].deallocateStringElements();
                mPreviousHomeDistFades[j].deleteAll();
                }
            }
        else {
            int speed = frameRateFactor * 4;

            if( d < 8 ) {
                speed = lrint( frameRateFactor * d / 2 );
                }

            if( speed > d ) {
                speed = floor( d );
                }

            if( speed < 1 ) {
                speed = 1;
                }
            
            doublePair dir = normalize( delta );
            
            mHomeSlipPosOffset[j] = 
                add( mHomeSlipPosOffset[j],
                     mult( dir, speed ) );
            }        
        }
    
    
    

    if( ourObject != NULL ) {
        char newTrigger = false;
        
        AnimType anim = stepLiveTriggers( &newTrigger );
        
        if( anim != endAnimType && newTrigger ) {
            addNewAnim( ourObject, anim );
            }
        else if( anim == endAnimType && ourObject->curAnim > endAnimType ) {
            // trigger is over, back to ground
            addNewAnim( ourObject, ground );
            }
        }
    
    
    if( ourObject != NULL && mNextHintObjectID != 0 &&
        getNumHints( mNextHintObjectID ) > 0 ) {


        
        if( ! isHintFilterStringInvalid() &&
            mCurrentHintObjectID != mNextHintObjectID ) {
            // check if we should jump the hint index to hint about this
            // thing we just picked up relative to our goal object
            
            int matchID = mNextHintObjectID;
            
            ObjectRecord *o = getObject( matchID );
            
            if( o->isUseDummy ) {
                matchID = o->useDummyParent;
                }
            else if( o->isVariableDummy ) {
                matchID = o->variableDummyParent;
                }

            if( mLastHintSortedList.size() > mCurrentHintIndex ) {
                // don't switch if we're already matched
                TransRecord *currHintTrans = 
                    mLastHintSortedList.getElementDirect( mCurrentHintIndex );
                
                if( currHintTrans->actor != matchID && 
                    currHintTrans->target != matchID ) {
                    
                    for( int i=0; i<mLastHintSortedList.size(); i++ ) {
                        TransRecord *t = 
                            mLastHintSortedList.getElementDirect( i );
                        
                        if( t->actor == matchID ||
                            t->target == matchID ) {
                            
                            mNextHintIndex = i;
                            break;
                            }
                        }
                    }
                }
            }
        


        if( ( isHintFilterStringInvalid() &&
              mCurrentHintObjectID != mNextHintObjectID ) ||
            mCurrentHintIndex != mNextHintIndex ||
            mForceHintRefresh ||
            justHitTab ) {
            
            char autoHint = false;
            
            // hide target object indicator unless player picked this hint
            if( mCurrentHintObjectID != mNextHintObjectID &&
                isHintFilterStringInvalid() ) {
                // they changed what they are holding
                
                // and they don't have a filter applied currently
                autoHint = true;
                }
            // or they just hit TAB 
            if( justHitTab ) {
                autoHint = false;
                }
            justHitTab = false;
            
            mForceHintRefresh = false;

            int newLiveSheetIndex = 0;

            if( mLiveHintSheetIndex != -1 ) {
                mHintTargetOffset[mLiveHintSheetIndex] = mHintHideOffset[0];
                
                // save last hint sheet for filter
                newLiveSheetIndex = 
                    (mLiveHintSheetIndex + 1 ) % ( NUM_HINT_SHEETS - 1 );
                
                }
            
            mLiveHintSheetIndex = newLiveSheetIndex;
            
            int i = mLiveHintSheetIndex;

            mHintTargetOffset[i] = mHintHideOffset[0];
            mHintTargetOffset[i].y += 100;
            
            if( mLastHintFilterString != NULL ) {
                mHintTargetOffset[i].y += 30;
                }
            

            mHintMessageIndex[ i ] = mNextHintIndex;
            
            mCurrentHintObjectID = mNextHintObjectID;
            mCurrentHintIndex = mNextHintIndex;
            
            if( isHintFilterStringInvalid() ) {
                mHintBookmarks[ mCurrentHintObjectID ] = mCurrentHintIndex;
                }
            
            mNumTotalHints[ i ] = 
                getNumHints( mCurrentHintObjectID );

            if( mHintMessage[ i ] != NULL ) {
                delete [] mHintMessage[ i ];
                }
            
            mHintMessage[ i ] = getHintMessage( mCurrentHintObjectID, 
                                                mHintMessageIndex[i],
                                                // don't set pointer
                                                // to what we're holding
                                                ourObject->holdingID );

            if( autoHint ) {
                // hide pointer until they start tabbing again
                mCurrentHintTargetObject[0] = 0;
                mCurrentHintTargetObject[1] = 0;
                }
            

            mHintExtraOffset[ i ].x = - getLongestLine( mHintMessage[i] );
            }
        else {
            // even in case where filter set, update this
            mCurrentHintObjectID = mNextHintObjectID;
            }



        if( ! isHintFilterStringInvalid() &&
            mCurrentHintIndex >= 0 ) {
            // special case
            // always show pointers to objects for current hint, unless
            // we're holding one
            
            if( mLastHintSortedList.size() > mCurrentHintIndex ) {
                TransRecord *t = 
                    mLastHintSortedList.getElementDirect( mCurrentHintIndex );
                int heldID = getObjectParent( ourObject->holdingID );
                
                
                if( t->actor != heldID ) {
                    mCurrentHintTargetObject[0] = t->actor;
                    }
                if( t->target != heldID ) {
                    mCurrentHintTargetObject[1] = t->target;
                    }
                }
            }


        }
    else if( ourObject != NULL && mNextHintObjectID != 0 &&
             getNumHints( mNextHintObjectID ) == 0 ) {
        // holding something with no hints, hide last sheet
        int newLiveSheetIndex = 0;
        
        if( mLiveHintSheetIndex != -1 ) {
            mHintTargetOffset[mLiveHintSheetIndex] = mHintHideOffset[0];
            
            // save last hint sheet for filter
            newLiveSheetIndex = 
                (mLiveHintSheetIndex + 1 ) % ( NUM_HINT_SHEETS - 1 );
            
            }
        
        mLiveHintSheetIndex = newLiveSheetIndex;
        
        mCurrentHintObjectID = mNextHintObjectID;
        }
    
    
        
    int lastSheet = NUM_HINT_SHEETS - 1;
    if( mPendingFilterString != NULL &&
        ( mHintMessage[ lastSheet ] == NULL ||
          strcmp( mHintMessage[ lastSheet ], mPendingFilterString ) != 0 ) ) {
        
        mHintTargetOffset[ lastSheet ] = mHintHideOffset[ lastSheet ];
        
        if( equal( mHintPosOffset[ lastSheet ], 
                   mHintHideOffset[ lastSheet ] ) ) {
            
            mNumTotalHints[ lastSheet ] = 1;
            mHintMessageIndex[ lastSheet ] = 0;
            
            mHintTargetOffset[ lastSheet ].y += 53;

            if( mHintMessage[ lastSheet ] != NULL ) {
                delete [] mHintMessage[ lastSheet ];
                }
            
            mHintMessage[ lastSheet ] = 
                stringDuplicate( mPendingFilterString );

            double len = 
                handwritingFont->measureString( mHintMessage[ lastSheet ] );
            
            mHintExtraOffset[ lastSheet ].x = - len;
            }
        
        }
    else if( mPendingFilterString == NULL &&
             mHintMessage[ lastSheet ] != NULL ) {
        mHintTargetOffset[ lastSheet ] = mHintHideOffset[ lastSheet ];
        
        if( equal( mHintPosOffset[ lastSheet ], 
                   mHintHideOffset[ lastSheet ] ) ) {
            
            // done hiding
            delete [] mHintMessage[ lastSheet ];
            mHintMessage[ lastSheet ] = NULL;
            }
        }
    

    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        
        if( ! equal( mHintPosOffset[i], mHintTargetOffset[i] ) ) {
            doublePair delta = 
                sub( mHintTargetOffset[i], mHintPosOffset[i] );
            
            double d = distance( mHintTargetOffset[i], mHintPosOffset[i] );
            
            
            if( d <= 1 ) {
                mHintPosOffset[i] = mHintTargetOffset[i];                
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }
                
                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mHintPosOffset[i] = 
                    add( mHintPosOffset[i],
                         mult( dir, speed ) );
                }
            
            }
        }


    // should new tutorial sheet be shown?
    if( ( mTutorialNumber > 0 || mGlobalMessageShowing ) 
        && ourObject != NULL ) {
        
        // search map for closest tutorial trigger

        double closeDist = 999999;
        int closestNumber = -1;

        char closestIsFinal = false;
        

        for( int y=0; y<mMapD; y++ ) {
        
            int worldY = y + mMapOffsetY - mMapD / 2;
            
            for( int x=0; x<mMapD; x++ ) {
                
                int worldX = x + mMapOffsetX - mMapD / 2;
                
                doublePair worldPos  = { (double)worldX, (double)worldY };
                
                double dist = distance( worldPos, ourObject->currentPos );
                
                if( dist < closeDist ) {
                    
                    int mapI = y * mMapD + x;
                    
                    int mapID = mMap[ mapI ];
                    
                    if( mapID > 0 ) {
                        
                        ObjectRecord *mapO = getObject( mapID );
                        
                        char *tutLoc = strstr( mapO->description, "tutorial" );
                        if( tutLoc != NULL ) {
                            
                            int tutPage = -1;
                            
                            sscanf( tutLoc, "tutorial %d", &tutPage );
                            

                            if( tutPage != -1 ) {
                                
                                closeDist = dist;
                                closestNumber = tutPage;
                                
                                if( strstr( mapO->description, "done" ) 
                                    != NULL ) {
                                    closestIsFinal = true;
                                    }
                                else {
                                    closestIsFinal = false;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        

        if( closeDist > 4 &&
            mLiveTutorialTriggerNumber != -1 ) {
            // don't set a new one unless we get close enough to it
            // OR we haven't even set the first one yet.
            closestNumber = -1;
            }
        

        
        if( closestNumber > -1 && 
            closestNumber != mLiveTutorialTriggerNumber ) {
            
            // different tutorial stone that what is showing
            
            if( closestIsFinal ) {
                // done with tutorial for good, unless they request it
                SettingsManager::setSetting( "tutorialDone", mTutorialNumber );
                }
            

            if( mLiveTutorialSheetIndex >= 0 ) {
                mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
                    mTutorialHideOffset[ mLiveTutorialSheetIndex ];
                }
            mLiveTutorialSheetIndex ++;
            
            if( mLiveTutorialSheetIndex >= NUM_HINT_SHEETS ) {
                mLiveTutorialSheetIndex -= NUM_HINT_SHEETS;
                }

            mLiveTutorialTriggerNumber = closestNumber;
            

            char *transString;

            if( mUsingSteam && mLiveTutorialTriggerNumber == 8 ) {
                transString = autoSprintf( "tutorial_%d_steam", 
                                           mLiveTutorialTriggerNumber );
                }
            else {
                if( mTutorialNumber == 1 ) {
                    transString = autoSprintf( "tutorial_%d", 
                                               mLiveTutorialTriggerNumber );
                    }
                else {
                    transString = autoSprintf( "tutorial_%d_%d",
                                               mTutorialNumber,
                                               mLiveTutorialTriggerNumber );
                    }
                }
            
            mTutorialMessage[ mLiveTutorialSheetIndex ] = 
                translate( transString );


            mTutorialTargetOffset[ mLiveTutorialSheetIndex ] =
                mTutorialHideOffset[ mLiveTutorialSheetIndex ];
            
            mTutorialTargetOffset[ mLiveTutorialSheetIndex ].y -= 100;
            
            delete [] transString;


            double longestLine = getLongestLine( 
                (char*)( mTutorialMessage[ mLiveTutorialSheetIndex ] ) );
            
            mTutorialExtraOffset[ mLiveTutorialSheetIndex ].x = longestLine;
            }
        }
    



    // pos for tutorial sheets
    // don't start sliding first sheet until map loaded
    if( ( mTutorialNumber > 0 || mGlobalMessageShowing )
        && mDoneLoadingFirstObjectSet )
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        
        if( ! equal( mTutorialPosOffset[i], mTutorialTargetOffset[i] ) ) {
            doublePair delta = 
                sub( mTutorialTargetOffset[i], mTutorialPosOffset[i] );
            
            double d = distance( mTutorialTargetOffset[i], 
                                 mTutorialPosOffset[i] );
            
            
            if( d <= 1 ) {
                mTutorialPosOffset[i] = mTutorialTargetOffset[i];
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }
                
                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mTutorialPosOffset[i] = 
                    add( mTutorialPosOffset[i],
                         mult( dir, speed ) );
                }
            
            if( equal( mTutorialTargetOffset[i], 
                       mTutorialHideOffset[i] ) ) {
                // fully hidden
                }
            else if( equal( mTutorialPosOffset[i],
                            mTutorialTargetOffset[i] ) ) {
                // fully visible, play chime
                double stereoPos = 0.25;
                
                if( i % 2 != 0 ) {
                    stereoPos = 0.75;
                    }
                
                if( mTutorialSound != NULL ) {
                        playSoundSprite( mTutorialSound, 
                                         0.18 * getSoundEffectsLoudness(), 
                                         stereoPos );
                    }
                }
            }
        }




    // pos for craving sheets
    // don't start sliding first sheet until map loaded
    if( mLiveCravingSheetIndex >= 0 && mDoneLoadingFirstObjectSet )
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        
        if( ! equal( mCravingPosOffset[i], mCravingTargetOffset[i] ) ) {
            doublePair delta = 
                sub( mCravingTargetOffset[i], mCravingPosOffset[i] );
            
            double d = distance( mCravingTargetOffset[i], 
                                 mCravingPosOffset[i] );
            
            
            if( d <= 1 ) {
                mCravingPosOffset[i] = mCravingTargetOffset[i];
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }
                
                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mCravingPosOffset[i] = 
                    add( mCravingPosOffset[i],
                         mult( dir, speed ) );
                }
            }
        }
    



    
    char anySlipsMovingDown = false;
    for( int i=0; i<3; i++ ) {
        if( i != mHungerSlipVisible &&
            ! equal( mHungerSlipPosOffset[i], mHungerSlipHideOffsets[i] ) ) {
            // visible when it shouldn't be
            mHungerSlipPosTargetOffset[i] = mHungerSlipHideOffsets[i];
            anySlipsMovingDown = true;
            }
        }
    
    if( !anySlipsMovingDown ) {
        if( mHungerSlipVisible != -1 ) {
            // send one up
            mHungerSlipPosTargetOffset[ mHungerSlipVisible ] =
                mHungerSlipShowOffsets[ mHungerSlipVisible ];
            }
        }


    // move all toward their targets
    for( int i=0; i<3; i++ ) {
        if( ! equal( mHungerSlipPosOffset[i], 
                     mHungerSlipPosTargetOffset[i] ) ) {
            
            doublePair delta = 
                sub( mHungerSlipPosTargetOffset[i], mHungerSlipPosOffset[i] );
        
            double d = distance( mHungerSlipPosTargetOffset[i], 
                                 mHungerSlipPosOffset[i] );
        
        
            if( d <= 1 ) {
                mHungerSlipPosOffset[i] = mHungerSlipPosTargetOffset[i];
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }

                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mHungerSlipPosOffset[i] = 
                    add( mHungerSlipPosOffset[i],
                         mult( dir, speed ) );
                }
            
            if( equal( mHungerSlipPosTargetOffset[i],
                       mHungerSlipHideOffsets[i] ) ) {
                // fully hidden    
                // reset wiggle time
                mHungerSlipWiggleTime[i] = 0;
                }
                
            }
        
        if( ! equal( mHungerSlipPosOffset[i],
                     mHungerSlipHideOffsets[i] ) ) {
            
            // advance wiggle time
            mHungerSlipWiggleTime[i] += 
                frameRateFactor * mHungerSlipWiggleSpeed[i];
            }
        }


    double curTime = game_getCurrentTime();


    // after 5 seconds of waiting, send PING
    if( playerActionPending && 
        ourObject != NULL && 
        curTime - lastServerMessageReceiveTime < 1 &&
        curTime - ourObject->pendingActionAnimationStartTime > 
        5 + largestPendingMessageTimeGap &&
        ! waitingForPong ) {

        printf( "Been waiting for response to our action request "
                "from server for %.2f seconds, and last server message "
                "received %.2f sec ago, saw a message time gap along the way "
                "of %.2f, sending PING request to server as sanity check.\n",
                curTime - ourObject->pendingActionAnimationStartTime,
                curTime - lastServerMessageReceiveTime,
                largestPendingMessageTimeGap );

        waitingForPong = true;
        lastPingSent ++;
        char *pingMessage = autoSprintf( "PING 0 0 %d#", lastPingSent );
        
        sendToServerSocket( pingMessage );
        delete [] pingMessage;
        }
    else if( playerActionPending &&
             ourObject != NULL &&
             curTime - ourObject->pendingActionAnimationStartTime > 5 &&
             curTime - lastServerMessageReceiveTime > 10 &&
             ! waitingForPong ) {
        // we're waiting for a response from the server, and
        // we haven't heard ANYTHING from the server in a long time
        // a full, two-way network connection break
        printf( "Been waiting for response to our action request "
                "from server for %.2f seconds, and last server message "
                "received %.2f sec ago.  Declaring connection broken.\n",
                curTime - ourObject->pendingActionAnimationStartTime,
                curTime - lastServerMessageReceiveTime );

        closeSocket( mServerSocket );
        mServerSocket = -1;
        
        if( mDeathReason != NULL ) {
            delete [] mDeathReason;
            }
        mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );
            
        handleOurDeath( true );
        }
    

    // after 10 seconds of waiting, if we HAVE received our PONG back
    if( playerActionPending && 
        ourObject != NULL && 
        curTime - lastServerMessageReceiveTime < 1 &&
        curTime - ourObject->pendingActionAnimationStartTime > 
        10 + largestPendingMessageTimeGap ) {

        // been bouncing for five seconds with no answer from server
        // in the mean time, we have seen other messages arrive from server
        // (so any network outage is over)

        if( waitingForPong &&
            lastPingSent == lastPongReceived ) {
            
            // and got PONG response, so server is hearing us
            // this is a real bug


            printf( 
                "Been waiting for response to our action request "
                "from server for %.2f seconds, and last server message "
                "received %.2f sec ago, saw a message time gap along the way "
                "of %.2f, AND got PONG response to our PING, giving up\n",
                curTime - ourObject->pendingActionAnimationStartTime,
                curTime - lastServerMessageReceiveTime,
                largestPendingMessageTimeGap );

            sendBugReport( 1 );

            // end it
            ourObject->pendingActionAnimationProgress = 0;
            ourObject->pendingAction = false;
            
            playerActionPending = false;
            waitingForPong = false;
            playerActionTargetNotAdjacent = false;
            
            if( nextActionMessageToSend != NULL ) {
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }
            
            int goodX = ourObject->xServer;
            int goodY = ourObject->yServer;
            
            printf( "   Jumping back to last-known server position of %d,%d\n",
                    goodX, goodY );
            
            // jump to wherever server said we were before
            ourObject->inMotion = false;
            
            ourObject->moveTotalTime = 0;
            ourObject->currentSpeed = 0;
            ourObject->currentGridSpeed = 0;
            
            
            ourObject->currentPos.x = goodX;
            ourObject->currentPos.y = goodY;
            
            ourObject->xd = goodX;
            ourObject->yd = goodY;
            ourObject->destTruncated = false;
            }
        else {
            printf( 
                "Been waiting for response to our action request "
                "from server for %.2f seconds, and no response received "
                "for our PING.  Declaring connection broken.\n",
                curTime - ourObject->pendingActionAnimationStartTime );
            
            closeSocket( mServerSocket );
            mServerSocket = -1;

            if( mDeathReason != NULL ) {
                delete [] mDeathReason;
                }
            mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );
            
            handleOurDeath( true );
            }
        }
    
    
    if( serverSocketConnected && 
        game_getCurrentTime() - timeLastMessageSent > 15 ) {
        // more than 15 seconds without client making a move
        // send KA to keep connection open
        sendToServerSocket( (char*)"KA 0 0#" );
        }
    

    if( showFPS ) {
        timeMeasures[1] += game_getCurrentTime() - updateStartTime;
        }
    


    messageProcessStartTime = showFPS ? game_getCurrentTime() : 0;
    

    char *message = getNextServerMessage();


    while( message != NULL ) {
        overheadServerBytesRead += 52;
        
        printf( "Got length %d message\n%s\n", 
                (int)strlen( message ), message );

        messageType type = getMessageType( message );
        
        if( mapPullMode && type != MAP_CHUNK ) {
            // ignore it---map is a frozen snapshot in time
            // or as close as we can get to it
            type = UNKNOWN;
            }
        
        if( type == SHUTDOWN  || type == FORCED_SHUTDOWN ) {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            
            setWaiting( false );
            setSignal( "serverShutdown" );
            
            delete [] message;
            return;
            }
        else if( type == SERVER_FULL ) {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            
            setWaiting( false );
            setSignal( "serverFull" );
            
            delete [] message;
            return;
            }
        else if( type == GLOBAL_MESSAGE ) {
            if( mTutorialNumber <= 0 ) {
                // not in tutorial
                // display this message
                
                char messageFromServer[200];
                sscanf( message, "MS\n%199s", messageFromServer );            
                
                displayGlobalMessage( messageFromServer );
                }
            }
        else if( type == WAR_REPORT ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );

            // clear war status of all
            for( int i=0; i<gameObjects.size(); i++ ) {
                gameObjects.getElement( i )->warPeaceStatus = 0;
                }

            int ourLineage = getOurLiveObject()->lineageEveID;
            

            if( numLines > 1 ) {
                for( int i=1; i<numLines; i++ ) {
                    int a = 0;
                    int b = 0;
                    char status[20] = "neutral";
                    sscanf( lines[i], "%d %d %19s", &a, &b, status );
                    
                    int stat = 0;
                    
                    if( strcmp( status, "war" ) == 0 ) {
                        stat = -1;
                        }
                    else if( strcmp( status, "peace" ) == 0 ) {
                        stat = 1;
                        }
                    

                    if( stat != 0 && a > 0 && b > 0 ) {
                        int otherID = 0;
                        if( a == ourLineage ) {
                            otherID = b;
                            }
                        else if( b == ourLineage ) {
                            otherID = a;
                            }
                        
                        if( otherID != 0 ) {
                            // mark players in this other line with this
                            // status
                            for( int i=0; i<gameObjects.size(); i++ ) {
                                
                                LiveObject *o = gameObjects.getElement( i );
                                
                                if( o->lineageEveID == otherID ) {
                                    o->warPeaceStatus = stat;
                                    }
                                }
                            }
                        }
                    }
                }

            for( int i=0; i<numLines; i++ ) {
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == LEARNED_TOOL_REPORT ) {
            SimpleVector<char *> *tokens = 
                tokenizeString( message );
            
            // first token is LR
            // rest are learned IDs
            for( int i=1; i < tokens->size(); i++ ) {
                char *tok = tokens->getElementDirect( i );
                
                int id = 0;
                sscanf( tok, "%d", &id );
                if( id > 0 ) {
                    ObjectRecord *o = getObject( id );
                    
                    if( o != NULL ) {
                        o->toolLearned = true;
                        }
                    }
                }
            tokens->deallocateStringElements();
            delete tokens;
            }
        else if( type == TOOL_EXPERTS ) {
            SimpleVector<char *> *tokens = tokenizeString( message );
            
            double curTime = game_getCurrentTime();
            
            // first token is LR
            // rest are learned IDs
            for( int i=1; i < tokens->size(); i++ ) {
                char *tok = tokens->getElementDirect( i );
                
                int id = 0;
                sscanf( tok, "%d", &id );
                if( id > 0 ) {
                    LiveObject *o = getLiveObject( id );
                   
                    if( o != NULL ) {

                        if( o->currentSpeech != NULL ) {
                            delete [] o->currentSpeech;
                            o->currentSpeech = NULL;
                            }
                        
                        o->currentSpeech = stringDuplicate( "+" );
                        o->speechFade = 1.0;
                                
                        o->speechIsSuccessfulCurse = false;

                        o->speechFadeETATime = curTime + 3;
                        o->speechIsCurseTag = false;
                        o->speechIsOverheadLabel = false;
                        }
                    }
                }
            tokens->deallocateStringElements();
            delete tokens;
            }
        else if( type == TOOL_SLOTS ) {
            int numSlotsUsed = 0;
            int numTotalSlots = 0;
            
            int numRead = 
                sscanf( message, "TS\n%d %d", &numSlotsUsed, &numTotalSlots );
            
            if( numRead == 2 ) {
                usedToolSlots = numSlotsUsed;
                totalToolSlots = numTotalSlots;
                }
            }
        else if( type == HOMELAND ) {
            int x = 0;
            int y = 0;
            
            char famName[40];

            int numRead = 
                sscanf( message, "HL\n%d %d %39s", &x, &y, famName );
            
            if( numRead == 3 ) {
                Homeland *h = getHomeland( x, y );
                if( h != NULL ) {
                    if( h->familyName != NULL ) {
                        delete [] h->familyName;
                        h->familyName = NULL;
                        }
                    
                    if( strcmp( famName, "0" ) != 0 ) {
                        h->familyName = stringDuplicate( famName );
                        }
                    }
                else {
                    char *newFamName = NULL;
                    if( strcmp( famName, "0" ) != 0 ) {
                        newFamName = stringDuplicate( famName );
                        }
                    Homeland h = { x, y, newFamName };
                    homelands.push_back( h );
                    }
                }
            }
        else if( type == FLIP ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );

            if( numLines > 0 ) {
                delete [] lines[0];
                }
            
            for( int i=1; i<numLines; i++ ) {
                int id = 0;
                int facingLeft = 0;
                
                int numRead = 
                    sscanf( lines[i], "%d %d", &id, &facingLeft );
            
                if( numRead == 2 ) {
                    LiveObject *o = getLiveObject( id );
                    
                    if( o != NULL && ! o->inMotion ) {
                        char flip = false;
                        
                        if( facingLeft && ! o->holdingFlip ) {
                            o->holdingFlip = true;
                            flip = true;
                            }
                        else if( ! facingLeft && o->holdingFlip ) {
                            o->holdingFlip = false;
                            flip = true;
                            }
                        if( flip ) {
                            o->lastAnim = moving;
                            o->curAnim = ground2;
                            o->lastAnimFade = 1;

                            o->lastHeldAnim = moving;
                            o->curHeldAnim = held;
                            o->lastHeldAnimFade = 1;
                            }
                        }
                    }
                delete [] lines[i];
                }
            
            delete [] lines;
            }
        else if( type == CRAVING ) {
            int foodID = -1;
            int bonus = 0;
            
            int numRead = 
                sscanf( message, "CR\n%d %d", &foodID, &bonus );
            
            if( numRead == 2 ) {
                setNewCraving( foodID, bonus );
                }
            }
        else if( type == SEQUENCE_NUMBER ) {
            // need to respond with LOGIN message
            
            char challengeString[200];

            // we don't use these for anything in client
            int currentPlayers = 0;
            int maxPlayers = 0;
            mRequiredVersion = versionNumber;
            
            sscanf( message, 
                    "SN\n"
                    "%d/%d\n"
                    "%199s\n"
                    "%d\n", &currentPlayers, &maxPlayers, challengeString, 
                    &mRequiredVersion );
            

            if( mRequiredVersion > versionNumber ||
                ( mRequiredVersion < versionNumber &&
                  mRequiredVersion < dataVersionNumber ) ) {
                
                // if server is using a newer version than us, we must upgrade
                // our client
                
                // if server is using an older version, check that
                // their version is not behind our data version at least

                closeSocket( mServerSocket );
                mServerSocket = -1;

                setWaiting( false );

                if( ! usingCustomServer && 
                    mRequiredVersion < dataVersionNumber ) {
                    // we have a newer data version than the server
                    // the servers must be in the process of updating, and
                    // we connected at just the wrong time
                    // Don't display a confusing version mismatch message here.
                    setSignal( "serverUpdate" );
                    }
                else {
                    setSignal( "versionMismatch" );
                    }

                delete [] message;
                return;
                }

            char *pureKey = getPureAccountKey();
            
            char *password = 
                SettingsManager::getStringSetting( "serverPassword" );
            
            if( password == NULL ) {
                password = stringDuplicate( "x" );
                }
            

            char *pwHash = hmac_sha1( password, challengeString );

            char *keyHash = hmac_sha1( pureKey, challengeString );
            
            delete [] pureKey;
            delete [] password;
            
            
            // we record the number of characters sent for playback
            // if playback is using a different email.ini setting, this
            // will fail.
            // So pad the email with up to 80 space characters
            // Thus, the login message is always this same length
            
            char *twinExtra;
            
            if( userTwinCode != NULL ) {
                char *hash = computeSHA1Digest( userTwinCode );
                twinExtra = autoSprintf( " %s %d", hash, userTwinCount );
                delete [] hash;
                }
            else {
                twinExtra = stringDuplicate( "" );
                }
                                         

            char *outMessage;

            char *tempEmail;
            
            if( strlen( userEmail ) > 0 ) {
                tempEmail = stringDuplicate( userEmail );
                }
            else {
                // a blank email
                // this will cause LOGIN message to have one less token
                
                // stick a place-holder in there instead
                tempEmail = stringDuplicate( "blank_email" );
                }
            
            const char *loginWord = "LOGIN";
            
            if( userReconnect ) {
                loginWord = "RLOGIN";
                }


            if( strlen( userEmail ) <= 80 ) {    
                outMessage = autoSprintf( "%s %s %-80s %s %s %d%s#",
                                          loginWord,
                                          clientTag, tempEmail, pwHash, keyHash,
                                          mTutorialNumber, twinExtra );
                }
            else {
                // their email is too long for this trick
                // don't cut it off.
                // but note that the playback will fail if email.ini
                // doesn't match on the playback machine
                outMessage = autoSprintf( "%s %s %s %s %s %d%s#",
                                          loginWord,
                                          clientTag, tempEmail, pwHash, keyHash,
                                          mTutorialNumber, twinExtra );
                }
            
            delete [] tempEmail;
            delete [] twinExtra;
            delete [] pwHash;
            delete [] keyHash;

            sendToServerSocket( outMessage );
            
            delete [] outMessage;
            
            delete [] message;
            return;
            }
        else if( type == ACCEPTED ) {
            // logged in successfully, wait for next message
            
            // subsequent messages should all be part of FRAME batches
            waitForFrameMessages = true;

            SettingsManager::setSetting( "loginSuccess", 1 );

            delete [] message;
            return;
            }
        else if( type == REJECTED ) {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            
            setWaiting( false );
            
            if( userReconnect ) {
                setSignal( "reconnectFailed" );
                }
            else {
                setSignal( "loginFailed" );
                }
            
            delete [] message;
            return;
            }
        else if( type == NO_LIFE_TOKENS ) {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            
            setWaiting( false );
            setSignal( "noLifeTokens" );
            
            delete [] message;
            return;
            }
        else if( type == APOCALYPSE ) {
            apocalypseDisplayProgress = 0;
            apocalypseInProgress = true;
            }
        else if( type == APOCALYPSE_DONE ) {
            apocalypseDisplayProgress = 0;
            apocalypseInProgress = false;
            homePosStack.deleteAll();

            clearToolLearnedStatus();

            // cancel all emots
            for( int i=0; i<gameObjects.size(); i++ ) {
                LiveObject *p = gameObjects.getElement( i );
                p->currentEmot = NULL;
                }
            }
        else if( type == MONUMENT_CALL ) {
            int posX, posY, monumentID;
            
            int numRead = sscanf( message, "MN\n%d %d %d",
                                  &posX, &posY, &monumentID );
            if( numRead == 3 ) {
                applyReceiveOffset( &posX, &posY );

                doublePair pos;
                pos.x = posX;
                pos.y = posY;
                
                LiveObject *ourLiveObject = getOurLiveObject();

                if( ourLiveObject != NULL ) {
                    double d = distance( pos, ourLiveObject->currentPos );
                    
                    if( d > 32 ) {
                        addAncientHomeLocation( posX, posY );
                        isAncientHomePosHell = false;
                        
                        // play sound in distance
                        ObjectRecord *monObj = getObject( monumentID );
                        
                        if( monObj != NULL && 
                            monObj->creationSound.numSubSounds > 0 ) {    
                            
                            if( strstr( monObj->description, "+hellArrow" ) ) {
                                isAncientHomePosHell = true;
                                }

                            doublePair realVector = 
                                getVectorFromCamera( lrint( posX ),
                                                     lrint( posY ) );
                            // position off camera in that direction
                            // but fake distance
                            realVector = mult( normalize( realVector ), 4 );
                            
                            playSound( monObj->creationSound, realVector );
                            }
                        }
                    }
                }            
            }
        else if( type == GRAVE ) {
            int posX, posY, playerID;
            
            int numRead = sscanf( message, "GV\n%d %d %d",
                                  &posX, &posY, &playerID );
            if( numRead == 3 ) {
                applyReceiveOffset( &posX, &posY );
                
                LiveObject *gravePerson = getLiveObject( playerID );
                
                if( gravePerson != NULL && 
                    ( gravePerson->relationName || 
                      gravePerson->name != NULL ) ) {
                    
                    GraveInfo g;
                    g.worldPos.x = posX;
                    g.worldPos.y = posY;
                    g.creationTime = game_getCurrentTime();
                    g.creationTimeUnknown = false;
                    g.lastMouseOverYears = -1;
                    g.lastMouseOverTime = g.creationTime;
                    
                    char *des = gravePerson->relationName;
                    char *desToDelete = NULL;
                    
                    if( des == NULL ) {
                        des = (char*)translate( "unrelated" );

                        if( gravePerson->name == NULL ) {
                            // call them nameless instead
                            des = (char*)translate( "namelessPerson" );
                            }
                        }
                    if( gravePerson->name != NULL ) {
                        des = autoSprintf( "%s - %s",
                                           gravePerson->name, des );
                        desToDelete = des;
                        }

                    g.relationName = stringDuplicate( des );
                    
                    if( desToDelete != NULL ) {
                        delete [] desToDelete;
                        }
                    
                    // this grave replaces any in same location
                    for( int i=0; i< mGraveInfo.size(); i++ ) {
                        GraveInfo *otherG = mGraveInfo.getElement( i );
                        
                        if( otherG->worldPos.x == posX &&
                            otherG->worldPos.y == posY ) {
                            
                            delete [] otherG->relationName;
                            mGraveInfo.deleteElement( i );
                            i--;
                            }
                        }
                    

                    mGraveInfo.push_back( g );
                    }
                }            
            }
        else if( type == GRAVE_MOVE ) {
            int posX, posY, posXNew, posYNew;

            int swapDest = 0;
            
            int numRead = sscanf( message, "GM\n%d %d %d %d %d",
                                  &posX, &posY, &posXNew, &posYNew,
                                  &swapDest );
            if( numRead == 4 || numRead == 5 ) {
                applyReceiveOffset( &posX, &posY );
                applyReceiveOffset( &posXNew, &posYNew );


                // handle case where two graves are "in the air"
                // at the same time, and one gets put down where the
                // other was picked up from before the other gets put down.
                // (graves swapping position)

                // When showing label to player, these are walked through
                // in order until first match found

                // So, we should walk through in reverse order until
                // LAST match found, and then move that one to the top
                // of the list.

                // it will "cover up" the label of the still-matching
                // grave further down on the list, which we will find
                // and fix later when it fininall finishes moving.
                char found = false;
                for( int i=mGraveInfo.size() - 1; i >= 0; i-- ) {
                    GraveInfo *g = mGraveInfo.getElement( i );
                    
                    if( g->worldPos.x == posX &&
                        g->worldPos.y == posY ) {
                        
                        g->worldPos.x = posXNew;
                        g->worldPos.y = posYNew;
                        
                        GraveInfo gStruct = *g;
                        mGraveInfo.deleteElement( i );
                        mGraveInfo.push_front( gStruct );
                        found = true;
                        break;
                        }    
                    }
                
                if( found && ! swapDest ) {
                    // do NOT need to keep any extra ones around
                    // this fixes cases where old grave info is left
                    // behind, due to decay
                    for( int i=1; i < mGraveInfo.size(); i++ ) {
                        GraveInfo *g = mGraveInfo.getElement( i );
                        
                        if( g->worldPos.x == posXNew &&
                            g->worldPos.y == posYNew ) {
                            
                            // a stale match
                            mGraveInfo.deleteElement( i );
                            i--;
                            }
                        }
                    }
                }            
            }
        else if( type == GRAVE_OLD ) {
            int posX, posY, playerID, displayID;
            double age;
            
            int eveID = -1;
            

            char nameBuffer[200];
            
            nameBuffer[0] = '\0';
            
            int numRead = sscanf( message, "GO\n%d %d %d %d %lf %199s",
                                  &posX, &posY, &playerID, &displayID,
                                  &age, nameBuffer );
            if( numRead == 6 ) {
                applyReceiveOffset( &posX, &posY );

                GridPos thisPos = { posX, posY };
                
                for( int i=0; i<graveRequestPos.size(); i++ ) {
                    if( equal( graveRequestPos.getElementDirect( i ),
                               thisPos ) ) {
                        graveRequestPos.deleteElement( i );
                        break;
                        }
                    }
                
                int nameLen = strlen( nameBuffer );
                for( int i=0; i<nameLen; i++ ) {
                    if( nameBuffer[i] == '_' ) {
                        nameBuffer[i] = ' ';
                        }
                    }
                
                
                SimpleVector<int> otherLin;

                int numLines;
                
                char **lines = split( message, "\n", &numLines );

                if( numLines > 1 ) {
                    SimpleVector<char *> *tokens = 
                        tokenizeString( lines[1] );
                    
                    int numNormalTokens = tokens->size();
                                
                    if( tokens->size() > 6 ) {
                        char *lastToken =
                            tokens->getElementDirect( 
                                tokens->size() - 1 );
                                    
                        if( strstr( lastToken, "eve=" ) ) {   
                            // eve tag at end
                            numNormalTokens--;
                            
                            sscanf( lastToken, "eve=%d", &( eveID ) );
                            }
                        }

                    for( int t=6; t<numNormalTokens; t++ ) {
                        char *tok = tokens->getElementDirect( t );
                                    
                        int mID = 0;
                        sscanf( tok, "%d", &mID );
                        
                        if( mID != 0 ) {
                            otherLin.push_back( mID );
                            }
                        }
                    tokens->deallocateStringElements();
                    delete tokens;
                    }


                for( int i=0; i<numLines; i++ ) {
                    delete [] lines[i];
                    }
                delete [] lines;
                
                LiveObject *ourLiveObject = getOurLiveObject();

                char *relationName = getRelationName( 
                    &( ourLiveObject->lineage ),
                    &otherLin,
                    ourID,
                    playerID,
                    ourLiveObject->displayID,
                    displayID,
                    ourLiveObject->age,
                    age,
                    ourLiveObject->lineageEveID,
                    eveID );

                GraveInfo g;
                g.worldPos.x = posX;
                g.worldPos.y = posY;
                
                char *des = relationName;
                char *desToDelete = NULL;
                    
                if( des == NULL ) {
                    des = (char*)translate( "unrelated" );
                    
                    if( strcmp( nameBuffer, "" ) == 0 ||
                        strcmp( nameBuffer, "~" ) == 0 ) {
                        // call them nameless instead
                        des = (char*)translate( "namelessPerson" );

                        if( playerID == 0 ) {
                            // call them forgotten instead
                            des = (char*)translate( "forgottenPerson" );
                            }
                        }
                    }
                if( strcmp( nameBuffer, "" ) != 0 &&
                    strcmp( nameBuffer, "~" ) != 0 ) {
                    des = autoSprintf( "%s - %s",
                                       nameBuffer, des );
                    desToDelete = des;
                    }

                g.relationName = stringDuplicate( des );
                
                if( desToDelete != NULL ) {
                    delete [] desToDelete;
                    }
                
                if( relationName != NULL ) {
                    delete [] relationName;
                    }
                
                g.creationTime = 
                    game_getCurrentTime() - age / ourLiveObject->ageRate;
                
                if( age == -1 ) {
                    g.creationTime = 0;
                    g.creationTimeUnknown = true;
                    }
                else {
                    g.creationTimeUnknown = false;
                    }
                
                g.lastMouseOverYears = -1;
                g.lastMouseOverTime = g.creationTime;
                
                mGraveInfo.push_back( g );
                }
            }
        else if( type == OWNER ) {
            SimpleVector<char*> *tokens = tokenizeString( message );
            
            if( tokens->size() >= 3 ) {
                int x = 0;
                int y = 0;
                
                sscanf( tokens->getElementDirect( 1 ), "%d", &x );
                sscanf( tokens->getElementDirect( 2 ), "%d", &y );
                
                GridPos thisPos = { x, y };
                
                for( int i=0; i<ownerRequestPos.size(); i++ ) {
                    if( equal( ownerRequestPos.getElementDirect( i ),
                               thisPos ) ) {
                        ownerRequestPos.deleteElement( i );
                        break;
                        }
                    }

                OwnerInfo *o = NULL;
                
                for( int i=0; i<mOwnerInfo.size(); i++ ) {
                    OwnerInfo *thisInfo = mOwnerInfo.getElement( i );
                    if( thisInfo->worldPos.x == x &&
                        thisInfo->worldPos.y == y ) {
                        
                        o = thisInfo;
                        break;
                        }    
                    }
                if( o == NULL ) {
                    // not found, create new
                    GridPos p = { x, y };
                    
                    OwnerInfo newO = { p, new SimpleVector<int>() };
                    
                    mOwnerInfo.push_back( newO );
                    o = mOwnerInfo.getElement( mOwnerInfo.size() - 1 );
                    }

                o->ownerList->deleteAll();
                for( int t=3; t < tokens->size(); t++ ) {
                    int ownerID = 0;
                    sscanf( tokens->getElementDirect( t ), "%d", &ownerID );
                    if( ownerID > 0 ) {
                        o->ownerList->push_back( ownerID );
                        }
                    }
                }
            tokens->deallocateStringElements();
            delete tokens;
            }
        else if( type == FOLLOWING ) {
            SimpleVector<char*> *tokens = tokenizeString( message );
            
            if( tokens->size() >= 4 ) {
             
                for( int i=1; i< tokens->size() - 1; i += 3 ){
                    
                    int f = 0;
                    int l = 0;
                    int c = -1;
                    
                    sscanf( tokens->getElementDirect( i ), "%d", &f );
                    sscanf( tokens->getElementDirect( i + 1 ), "%d", &l );
                    sscanf( tokens->getElementDirect( i + 2 ), "%d", &c );
                    
                    LiveObject *fo = getLiveObject( f );
                    
                    if( fo != NULL ) {
                        if( l != 0 ) {
                            fo->followingID = l;
                            }
                        else {
                            fo->followingID = -1;
                            }
                        }
                    
                    if( l > 0 && c != -1 ) {
                        LiveObject *lo = getLiveObject( l );
                        if( lo != NULL ) {
                            while( c >= NUM_BADGE_COLORS ) {
                                // wrap around
                                c -= NUM_BADGE_COLORS;
                                }
                            lo->personalLeadershipColor = getFloatColor(
                                badgeColors[c] );
                            }
                        }
                    }
                }
            tokens->deallocateStringElements();
            delete tokens;
            
            updateLeadership();
            }
        else if( type == EXILED ) {
            SimpleVector<char*> *tokens = tokenizeString( message );
            
            if( tokens->size() >= 3 ) {
             
                for( int i=1; i< tokens->size() - 1; i += 2 ){
                    
                    // target and perp
                    int t = 0;
                    int p = 0;
                
                    sscanf( tokens->getElementDirect( i ), "%d", &t );
                    sscanf( tokens->getElementDirect( i + 1 ), "%d", &p );
                    
                    LiveObject *to = getLiveObject( t );
                    
                    if( to != NULL ) {
                        if( p == -1 ) {
                            // their exile list has been cleared
                            to->exiledByIDs.deleteAll();
                            }
                        else if( p > 0 ) {
                            if( to->exiledByIDs.getElementIndex( p ) == -1 ) {
                                to->exiledByIDs.push_back( p );
                                }
                            }
                        }
                    }
                }
            tokens->deallocateStringElements();
            delete tokens;
            
            updateLeadership();
            }
        else if( type == VALLEY_SPACING ) {
            sscanf( message, "VS\n%d %d",
                    &valleySpacing, &valleyOffset );
            }
        else if( type == FLIGHT_DEST ) {
            int posX, posY, playerID;
            
            int numRead = sscanf( message, "FD\n%d %d %d",
                                  &playerID, &posX, &posY );
            if( numRead == 3 ) {
                applyReceiveOffset( &posX, &posY );
                
                LiveObject *flyingPerson = getLiveObject( playerID );
                
                if( flyingPerson != NULL ) {
                    // move them there instantly
                    flyingPerson->xd = posX;
                    flyingPerson->yd = posY;
                    
                    flyingPerson->xServer = posX;
                    flyingPerson->yServer = posY;
                    
                    flyingPerson->currentPos.x = posX;
                    flyingPerson->currentPos.y = posY;
                    
                    flyingPerson->currentSpeed = 0;
                    flyingPerson->currentGridSpeed = 0;
                    flyingPerson->destTruncated = false;
                    
                    flyingPerson->currentMoveDirection.x = 0;
                    flyingPerson->currentMoveDirection.y = 0;
                    
                    if( flyingPerson->pathToDest != NULL ) {
                        delete [] flyingPerson->pathToDest;
                        flyingPerson->pathToDest = NULL;
                        }

                    flyingPerson->inMotion = false;
                        

                    if( flyingPerson->id == ourID ) {
                        // special case for self
                        
                        // jump camera there instantly
                        lastScreenViewCenter.x = posX * CELL_D;
                        lastScreenViewCenter.y = posY * CELL_D;
                        setViewCenterPosition( lastScreenViewCenter.x,
                                               lastScreenViewCenter.y );
                        
                        // show loading screen again
                        mFirstServerMessagesReceived = 2;
                        mStartedLoadingFirstObjectSet = false;
                        mDoneLoadingFirstObjectSet = false;
                        mFirstObjectSetLoadingProgress = 0;
                        mPlayerInFlight = true;

                        // home markers invalid now
                        homePosStack.deleteAll();
                        }
                    }
                }            
            }
        else if( type == BAD_BIOMES ) {
            mBadBiomeIndices.deleteAll();
            mBadBiomeNames.deallocateStringElements();
            
            int numLines;
                
            char **lines = split( message, "\n", &numLines );
            
            for( int i=1; i<numLines; i++ ) {
                int index = 0;
                char name[100];
                
                int numRead = sscanf( lines[i], "%d %99s", &index, name );
                
                if( numRead == 2 ) {
                    int nameLen = strlen( name );
                    for( int c=0; c<nameLen; c++ ) {
                        if( name[c] == '_' ) {
                            name[c] = ' ';
                            }
                        }
                    
                    mBadBiomeIndices.push_back( index );
                    mBadBiomeNames.push_back( stringDuplicate( name ) );
                    }
                }
            
            for( int i=0; i<numLines; i++ ) {
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == VOG_UPDATE ) {
            int posX, posY;
            
            int numRead = sscanf( message, "VU\n%d %d",
                                  &posX, &posY );
            if( numRead == 2 ) {
                vogModeActuallyOn = true;
                
                vogPos.x = posX;
                vogPos.y = posY;

                mObjectPicker.setPosition( vogPos.x * CELL_D + 510,
                                           vogPos.y * CELL_D + 90 );

                // jump camp instantly
                lastScreenViewCenter.x = posX * CELL_D;
                lastScreenViewCenter.y = posY * CELL_D;
                setViewCenterPosition( lastScreenViewCenter.x,
                                       lastScreenViewCenter.y );

                mCurMouseOverCellFade = 1.0;
                
                mCurMouseOverCell.x = vogPos.x - mMapOffsetX + mMapD / 2;
                mCurMouseOverCell.y = vogPos.y - mMapOffsetY + mMapD / 2;
                }
            }
        else if( type == PHOTO_SIGNATURE ) {
            int posX, posY;

            char sig[100];
            
            int numRead = sscanf( message, "PH\n%d %d %99s",
                                  &posX, &posY, sig );
            if( numRead == 3 ) {
                photoSig = stringDuplicate( sig );
                }
            else {
                photoSig = stringDuplicate( "NO_SIG" );
                }
            }
        else if( type == MAP_CHUNK ) {
            
            int sizeX = 0;
            int sizeY = 0;
            int x = 0;
            int y = 0;
            
            int binarySize = 0;
            int compressedSize = 0;
            
            sscanf( message, "MC\n%d %d %d %d\n%d %d\n", 
                    &sizeX, &sizeY, &x, &y, &binarySize, &compressedSize );
            
            printf( "Got map chunk with bin size %d, compressed size %d\n", 
                    binarySize, compressedSize );
            
            if( ! mMapGlobalOffsetSet ) {
                
                // we need 7 fraction bits to represent 128 pixels per tile
                // 32-bit float has 23 significant bits, not counting sign
                // that leaves 16 bits for tile coordinate, or 65,536
                // Give two extra bits of wiggle room
                int maxOK = 16384;
                
                if( x < maxOK &&
                    x > -maxOK &&
                    y < maxOK &&
                    y > -maxOK ) {
                    printf( "First chunk isn't too far from center, using "
                            "0,0 as our global offset\n" );
                    
                    mMapGlobalOffset.x = 0;
                    mMapGlobalOffset.y = 0;
                    mMapGlobalOffsetSet = true;
                    }
                else {
                    printf( 
                        "Using this first chunk center as our global offset:  "
                        "%d, %d\n", x, y );
                    mMapGlobalOffset.x = x;
                    mMapGlobalOffset.y = y;
                    mMapGlobalOffsetSet = true;
                    }
                }
            
            applyReceiveOffset( &x, &y );
            
            // recenter our in-ram sub-map around this new chunk
            int newMapOffsetX = x + sizeX/2;
            int newMapOffsetY = y + sizeY/2;
            
            // move old cached map cells over to line up with new center

            int xMove = mMapOffsetX - newMapOffsetX;
            int yMove = mMapOffsetY - newMapOffsetY;
            
            int *newMap = new int[ mMapD * mMapD ];
            int *newMapBiomes = new int[ mMapD * mMapD ];
            int *newMapFloors = new int[ mMapD * mMapD ];
            

            double *newMapAnimationFrameCount = new double[ mMapD * mMapD ];
            double *newMapAnimationLastFrameCount = new double[ mMapD * mMapD ];

            double *newMapAnimationFrozenRotFameCount = 
                new double[ mMapD * mMapD ];

            char *newMapAnimationFrozenRotFameCountUsed = 
                new char[ mMapD * mMapD ];

            int *newMapFloorAnimationFrameCount = new int[ mMapD * mMapD ];
        
            AnimType *newMapCurAnimType = new AnimType[ mMapD * mMapD ];
            AnimType *newMapLastAnimType = new AnimType[ mMapD * mMapD ];
            double *newMapLastAnimFade = new double[ mMapD * mMapD ];
            
            doublePair *newMapDropOffsets = new doublePair[ mMapD * mMapD ];
            double *newMapDropRot = new double[ mMapD * mMapD ];
            SoundUsage *newMapDropSounds = new SoundUsage[ mMapD * mMapD ];

            doublePair *newMapMoveOffsets = new doublePair[ mMapD * mMapD ];
            double *newMapMoveSpeeds = new double[ mMapD * mMapD ];


            char *newMapTileFlips= new char[ mMapD * mMapD ];
            
            SimpleVector<int> *newMapContainedStacks = 
                new SimpleVector<int>[ mMapD * mMapD ];
            
            SimpleVector< SimpleVector<int> > *newMapSubContainedStacks = 
                new SimpleVector< SimpleVector<int> >[ mMapD * mMapD ];

            char *newMapPlayerPlacedFlags = new char[ mMapD * mMapD ];

            
            for( int i=0; i<mMapD *mMapD; i++ ) {
                // starts uknown, not empty
                newMap[i] = -1;
                newMapBiomes[i] = -1;
                newMapFloors[i] = -1;

                int newX = i % mMapD;
                int newY = i / mMapD;

                int worldX = newX + mMapOffsetX - mMapD / 2;
                int worldY = newY + mMapOffsetY - mMapD / 2;
                
                // each cell is different, but always the same
                newMapAnimationFrameCount[i] =
                    lrint( getXYRandom( worldX, worldY ) * 10000 );
                newMapAnimationLastFrameCount[i] =
                    newMapAnimationFrameCount[i];
                
                newMapAnimationFrozenRotFameCount[i] = 0;
                newMapAnimationFrozenRotFameCountUsed[i] = false;
                
                newMapFloorAnimationFrameCount[i] =
                    lrint( getXYRandom( worldX, worldY ) * 13853 );
                
                

                newMapCurAnimType[i] = ground;
                newMapLastAnimType[i] = ground;
                newMapLastAnimFade[i] = 0;
                newMapDropOffsets[i].x = 0;
                newMapDropOffsets[i].y = 0;
                newMapDropRot[i] = 0;
                
                newMapDropSounds[i] = blankSoundUsage;
                
                newMapMoveOffsets[i].x = 0;
                newMapMoveOffsets[i].y = 0;
                newMapMoveSpeeds[i] = 0;

                newMapTileFlips[i] = false;
                newMapPlayerPlacedFlags[i] = false;
                

                
                int oldX = newX - xMove;
                int oldY = newY - yMove;
                
                if( oldX >= 0 && oldX < mMapD
                    &&
                    oldY >= 0 && oldY < mMapD ) {
                    
                    int oI = oldY * mMapD + oldX;

                    newMap[i] = mMap[oI];
                    newMapBiomes[i] = mMapBiomes[oI];
                    newMapFloors[i] = mMapFloors[oI];

                    newMapAnimationFrameCount[i] = mMapAnimationFrameCount[oI];
                    newMapAnimationLastFrameCount[i] = 
                        mMapAnimationLastFrameCount[oI];

                    newMapAnimationFrozenRotFameCount[i] = 
                        mMapAnimationFrozenRotFrameCount[oI];

                    newMapAnimationFrozenRotFameCountUsed[i] = 
                        mMapAnimationFrozenRotFrameCountUsed[oI];

                    newMapFloorAnimationFrameCount[i] = 
                        mMapFloorAnimationFrameCount[oI];
                    
                    newMapCurAnimType[i] = mMapCurAnimType[oI];
                    newMapLastAnimType[i] = mMapLastAnimType[oI];
                    newMapLastAnimFade[i] = mMapLastAnimFade[oI];
                    newMapDropOffsets[i] = mMapDropOffsets[oI];
                    newMapDropRot[i] = mMapDropRot[oI];
                    newMapDropSounds[i] = mMapDropSounds[oI];

                    newMapMoveOffsets[i] = mMapMoveOffsets[oI];
                    newMapMoveSpeeds[i] = mMapMoveSpeeds[oI];

                    newMapTileFlips[i] = mMapTileFlips[oI];
                    

                    newMapContainedStacks[i] = mMapContainedStacks[oI];
                    newMapSubContainedStacks[i] = mMapSubContainedStacks[oI];

                    newMapPlayerPlacedFlags[i] = 
                        mMapPlayerPlacedFlags[oI];
                    }
                }
            
            memcpy( mMap, newMap, mMapD * mMapD * sizeof( int ) );
            memcpy( mMapBiomes, newMapBiomes, mMapD * mMapD * sizeof( int ) );
            memcpy( mMapFloors, newMapFloors, mMapD * mMapD * sizeof( int ) );

            memcpy( mMapAnimationFrameCount, newMapAnimationFrameCount, 
                    mMapD * mMapD * sizeof( double ) );
            memcpy( mMapAnimationLastFrameCount, 
                    newMapAnimationLastFrameCount, 
                    mMapD * mMapD * sizeof( double ) );

            memcpy( mMapAnimationFrozenRotFrameCount, 
                    newMapAnimationFrozenRotFameCount, 
                    mMapD * mMapD * sizeof( double ) );

            memcpy( mMapAnimationFrozenRotFrameCountUsed, 
                    newMapAnimationFrozenRotFameCountUsed, 
                    mMapD * mMapD * sizeof( char ) );

            
            memcpy( mMapFloorAnimationFrameCount, 
                    newMapFloorAnimationFrameCount, 
                    mMapD * mMapD * sizeof( int ) );

            
            memcpy( mMapCurAnimType, newMapCurAnimType, 
                    mMapD * mMapD * sizeof( AnimType ) );
            memcpy( mMapLastAnimType, newMapLastAnimType,
                    mMapD * mMapD * sizeof( AnimType ) );
            memcpy( mMapLastAnimFade, newMapLastAnimFade,
                    mMapD * mMapD * sizeof( double ) );
            memcpy( mMapDropOffsets, newMapDropOffsets,
                    mMapD * mMapD * sizeof( doublePair ) );
            memcpy( mMapDropRot, newMapDropRot,
                    mMapD * mMapD * sizeof( double ) );
            memcpy( mMapDropSounds, newMapDropSounds,
                    mMapD * mMapD * sizeof( SoundUsage ) );

            memcpy( mMapMoveOffsets, newMapMoveOffsets,
                    mMapD * mMapD * sizeof( doublePair ) );
            memcpy( mMapMoveSpeeds, newMapMoveSpeeds,
                    mMapD * mMapD * sizeof( double ) );

            
            memcpy( mMapTileFlips, newMapTileFlips,
                    mMapD * mMapD * sizeof( char ) );
            
            // can't memcpy vectors
            // need to assign them so copy constructors are invoked
            for( int i=0; i<mMapD *mMapD; i++ ) {
                mMapContainedStacks[i] = newMapContainedStacks[i];
                mMapSubContainedStacks[i] = newMapSubContainedStacks[i];
                }
            

            memcpy( mMapPlayerPlacedFlags, newMapPlayerPlacedFlags,
                    mMapD * mMapD * sizeof( char ) );
            
            delete [] newMap;
            delete [] newMapBiomes;
            delete [] newMapFloors;
            delete [] newMapAnimationFrameCount;
            delete [] newMapAnimationLastFrameCount;
            delete [] newMapAnimationFrozenRotFameCount;
            delete [] newMapAnimationFrozenRotFameCountUsed;
            delete [] newMapFloorAnimationFrameCount;
            
            delete [] newMapCurAnimType;
            delete [] newMapLastAnimType;
            delete [] newMapLastAnimFade;
            delete [] newMapDropOffsets;
            delete [] newMapDropRot;
            delete [] newMapDropSounds;

            delete [] newMapMoveOffsets;
            delete [] newMapMoveSpeeds;
            
            delete [] newMapTileFlips;
            delete [] newMapContainedStacks;
            delete [] newMapSubContainedStacks;
            
            delete [] newMapPlayerPlacedFlags;
            
            

            mMapOffsetX = newMapOffsetX;
            mMapOffsetY = newMapOffsetY;
            
            
            unsigned char *compressedChunk = 
                new unsigned char[ compressedSize ];
    
            
            for( int i=0; i<compressedSize; i++ ) {
                compressedChunk[i] = serverSocketBuffer.getElementDirect( i );
                }
            serverSocketBuffer.deleteStartElements( compressedSize );

            
            unsigned char *decompressedChunk =
                zipDecompress( compressedChunk, 
                               compressedSize,
                               binarySize );
            
            delete [] compressedChunk;
            
            if( decompressedChunk == NULL ) {
                printf( "Decompressing chunk failed\n" );
                }
            else {
                
                unsigned char *binaryChunk = 
                    new unsigned char[ binarySize + 1 ];
            
                memcpy( binaryChunk, decompressedChunk, binarySize );
            
                delete [] decompressedChunk;
 
            
                // for now, binary chunk is actually just ASCII
                binaryChunk[ binarySize ] = '\0';
            
                
                SimpleVector<char *> *tokens = 
                    tokenizeString( (char*)binaryChunk );
            
                delete [] binaryChunk;


                int numCells = sizeX * sizeY;
                
                if( tokens->size() == numCells ) {
                    
                    for( int i=0; i<tokens->size(); i++ ) {
                        int cX = i % sizeX;
                        int cY = i / sizeX;
                        
                        int mapX = cX + x - mMapOffsetX + mMapD / 2;
                        int mapY = cY + y - mMapOffsetY + mMapD / 2;
                        
                        if( mapX >= 0 && mapX < mMapD
                            &&
                            mapY >= 0 && mapY < mMapD ) {
                            
                            
                            int mapI = mapY * mMapD + mapX;
                            int oldMapID = mMap[mapI];
                            
                            sscanf( tokens->getElementDirect(i),
                                    "%d:%d:%d", 
                                    &( mMapBiomes[mapI] ),
                                    &( mMapFloors[mapI] ),
                                    &( mMap[mapI] ) );
                            
                            if( mMap[mapI] != oldMapID ) {
                                // our placement status cleared
                                mMapPlayerPlacedFlags[mapI] = false;
                                }

                            mMapContainedStacks[mapI].deleteAll();
                            mMapSubContainedStacks[mapI].deleteAll();
                            
                            if( strstr( tokens->getElementDirect(i), "," ) 
                                != NULL ) {
                                
                                int numInts;
                                char **ints = 
                                    split( tokens->getElementDirect(i), 
                                           ",", &numInts );
                                
                                delete [] ints[0];
                                
                                int numContained = numInts - 1;
                                
                                for( int c=0; c<numContained; c++ ) {
                                    SimpleVector<int> newSubStack;
                                    
                                    mMapSubContainedStacks[mapI].push_back(
                                        newSubStack );
                                    
                                    int contained = atoi( ints[ c + 1 ] );
                                    mMapContainedStacks[mapI].push_back( 
                                        contained );
                                    
                                    if( strstr( ints[c + 1], ":" ) != NULL ) {
                                        // sub-container items
                                
                                        int numSubInts;
                                        char **subInts = 
                                            split( ints[c + 1], 
                                                   ":", &numSubInts );
                                
                                        delete [] subInts[0];
                                        int numSubCont = numSubInts - 1;

                                        SimpleVector<int> *subStack =
                                            mMapSubContainedStacks[mapI].
                                            getElement(c);

                                        for( int s=0; s<numSubCont; s++ ) {
                                            subStack->push_back(
                                                atoi( subInts[ s + 1 ] ) );
                                            delete [] subInts[ s + 1 ];
                                            }

                                        delete [] subInts;
                                        }

                                    delete [] ints[ c + 1 ];
                                    }
                                delete [] ints;
                                }
                            }
                        }
                    }   
                
                tokens->deallocateStringElements();
                delete tokens;
                
                if( !( mFirstServerMessagesReceived & 1 ) ) {
                    // first map chunk just recieved
                    
                    char found = false;
                    int closestX = 0;
                    int closestY = 0;
                    
                    // only if marker starts on birth map chunk
                    
                    // use distance squared here, no need for sqrt

                    // rough estimate of radius of birth map chunk
                    // this allows markers way off the screen, but so what?
                    double closestDist = 16 * 16;

                    int mapCenterY = y + sizeY / 2;
                    int mapCenterX = x + sizeX / 2;
                    printf( "Map center = %d,%d\n", mapCenterX, mapCenterY );
                    
                    for( int mapY=0; mapY < mMapD; mapY++ ) {
                        for( int mapX=0; mapX < mMapD; mapX++ ) {
                        
                            int i = mMapD * mapY + mapX;
                            
                            int id = mMap[ i ];
                            
                            if( id > 0 ) {
                            
                                // check for home marker
                                if( getObject( id )->homeMarker ) {
                                    int worldY = mapY + mMapOffsetY - mMapD / 2;
                                    
                                    int worldX = mapX + mMapOffsetX - mMapD / 2;

                                    double dist = 
                                        pow( worldY - mapCenterY, 2 )
                                        +
                                        pow( worldX - mapCenterX, 2 );
                                    
                                    if( dist < closestDist ) {
                                        closestDist = dist;
                                        closestX = worldX;
                                        closestY = worldY;
                                        found = true;
                                        }
                                    }
                                }
                            }
                        }
                    
                    if( found ) {
                        printf( "Found starting home marker at %d,%d\n",
                                closestX, closestY );
                        addHomeLocation( closestX, closestY );
                        }
                    }
                

                mFirstServerMessagesReceived |= 1;
                }

            if( mapPullMode ) {
                
                if( x == mapPullCurrentX - sizeX/2 && 
                    y == mapPullCurrentY - sizeY/2 ) {
                    
                    lastScreenViewCenter.x = mapPullCurrentX * CELL_D;
                    lastScreenViewCenter.y = mapPullCurrentY * CELL_D;
                    setViewCenterPosition( lastScreenViewCenter.x,
                                           lastScreenViewCenter.y );
                    
                    mapPullCurrentX += 10;
                    
                    if( mapPullCurrentX > mapPullEndX ) {
                        mapPullCurrentX = mapPullStartX;
                        mapPullCurrentY += 5;
                        
                        if( mapPullCurrentY > mapPullEndY ) {
                            mapPullModeFinalImage = true;
                            }
                        }
                    mapPullCurrentSaved = false;
                    mapPullCurrentSent = false;
                    }                
                }
            }
        else if( type == MAP_CHANGE ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip fist
                delete [] lines[0];
                }
            
            char idBuffer[500];
            
            for( int i=1; i<numLines; i++ ) {
                
                int x, y, floorID, responsiblePlayerID;
                int oldX, oldY;
                float speed = 0;
                                
                
                int numRead = sscanf( lines[i], "%d %d %d %499s %d %d %d %f",
                                      &x, &y, &floorID, 
                                      idBuffer, &responsiblePlayerID,
                                      &oldX, &oldY, &speed );
                if( numRead == 5 || numRead == 8) {

                    applyReceiveOffset( &x, &y );

                    int mapX = x - mMapOffsetX + mMapD / 2;
                    int mapY = y - mMapOffsetY + mMapD / 2;
                    
                    if( mapX >= 0 && mapX < mMapD
                        &&
                        mapY >= 0 && mapY < mMapD ) {
                        
                        int mapI = mapY * mMapD + mapX;
                        
                        int oldFloor = mMapFloors[ mapI ];

                        mMapFloors[ mapI ] = floorID;
                        

                        int old = mMap[mapI];

                        int newID = -1;
                        
                        int oldContainedCount = 
                            mMapContainedStacks[mapI].size();
                        
                        if( responsiblePlayerID != -1 ) {
                            int rID = responsiblePlayerID;
                            if( rID < -1 ) {
                                rID = -rID;
                                }
                            LiveObject *rObj = getLiveObject( rID );
                            
                            if( rObj != NULL && 
                                rObj->pendingReceivedMessages.size() > 0 ) {
                                

                                printf( "Holding MX message caused by "
                                    "%d until later, "
                                    "%d other messages pending for them\n",
                                    rID,
                                    rObj->pendingReceivedMessages.size() );
                                
                                rObj->pendingReceivedMessages.push_back(
                                    autoSprintf( "MX\n%s\n#",
                                                 lines[i] ) );
                                
                                delete [] lines[i];
                                continue;
                                }
                            }
                        

                        if( strstr( idBuffer, "," ) != NULL ) {
                            int numInts;
                            char **ints = 
                                split( idBuffer, ",", &numInts );
                        
                            
                            newID = atoi( ints[0] );

                            mMap[mapI] = newID;
                            
                            delete [] ints[0];

                            SimpleVector<int> oldContained;
                            // player triggered
                            // with no changed to container
                            // look for contained change
                            if( speed == 0 &&
                                old == newID && 
                                responsiblePlayerID < 0 ) {
                            
                                oldContained.push_back_other( 
                                    &( mMapContainedStacks[mapI] ) );
                                }
                            
                            mMapContainedStacks[mapI].deleteAll();
                            mMapSubContainedStacks[mapI].deleteAll();
                            
                            int numContained = numInts - 1;
                            
                            for( int c=0; c<numContained; c++ ) {
                                
                                SimpleVector<int> newSubStack;
                                    
                                mMapSubContainedStacks[mapI].push_back(
                                    newSubStack );
                                
                                mMapContainedStacks[mapI].push_back(
                                    atoi( ints[ c + 1 ] ) );
                                
                                if( strstr( ints[c + 1], ":" ) != NULL ) {
                                    // sub-container items
                                
                                    int numSubInts;
                                    char **subInts = 
                                        split( ints[c + 1], ":", &numSubInts );
                                
                                    delete [] subInts[0];
                                    int numSubCont = numSubInts - 1;
                                    
                                    SimpleVector<int> *subStack =
                                        mMapSubContainedStacks[mapI].
                                        getElement(c);

                                    for( int s=0; s<numSubCont; s++ ) {
                                        subStack->push_back(
                                            atoi( subInts[ s + 1 ] ) );
                                        delete [] subInts[ s + 1 ];
                                        }

                                    delete [] subInts;
                                    }

                                delete [] ints[ c + 1 ];
                                }
                            delete [] ints;

                            if( speed == 0 &&
                                old == newID && 
                                responsiblePlayerID < 0
                                &&
                                oldContained.size() ==
                                mMapContainedStacks[mapI].size() ) {
                                // no change in number of items
                                // count number that change
                                int changeCount = 0;
                                int changeIndex = -1;
                                for( int i=0; i<oldContained.size(); i++ ) {
                                    if( oldContained.
                                        getElementDirect( i ) 
                                        !=
                                        mMapContainedStacks[mapI].
                                        getElementDirect( i ) ) {
                                        changeCount++;
                                        changeIndex = i;
                                        }
                                    }
                                if( changeCount == 1 ) {
                                    // single item changed
                                    // play sound for it?

                                    int oldContID =
                                        oldContained.
                                        getElementDirect( changeIndex );
                                    int newContID =
                                        mMapContainedStacks[mapI].
                                        getElementDirect( changeIndex );
                                    
                                    
                                    // watch out for swap case, with single
                                    // item
                                    // don't play sound then
                                    LiveObject *causingPlayer =
                                        getLiveObject( - responsiblePlayerID );

                                    if( causingPlayer != NULL &&
                                        causingPlayer->holdingID 
                                        != oldContID ) {
                                        

                                        ObjectRecord *newObj = 
                                            getObject( newContID );
                                        
                                        if( shouldCreationSoundPlay(
                                                oldContID, newContID ) ) {
                                            if( newObj->
                                                creationSound.numSubSounds 
                                                > 0 ) {
                                                
                                                playSound( 
                                                    newObj->creationSound,
                                                    getVectorFromCamera( 
                                                        x, y ) );
                                                }
                                            }
                                        else if(
                                            causingPlayer != NULL &&
                                            causingPlayer->holdingID == 0 &&
                                            bothSameUseParent( newContID,
                                                               oldContID ) &&
                                            newObj->
                                            usingSound.numSubSounds > 0 ) {
                                        
                                            ObjectRecord *oldObj = 
                                                getObject( oldContID );
                                        
                                            // only play sound if new is
                                            // less used than old (filling back
                                            // up sound)
                                            if( getObjectParent( oldContID ) ==
                                                newContID ||
                                                oldObj->thisUseDummyIndex <
                                                newObj->thisUseDummyIndex ) {
                                        
                                                playSound( 
                                                    newObj->usingSound,
                                                    getVectorFromCamera( 
                                                        x, y ) );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        else {
                            // a single int
                            newID = atoi( idBuffer );
                            mMap[mapI] = newID;
                            mMapContainedStacks[mapI].deleteAll();
                            mMapSubContainedStacks[mapI].deleteAll();
                            }
                        
                        if( speed > 0 ) {
                            // this cell moved from somewhere
                            
                            applyReceiveOffset( &oldX, &oldY );
                            

                            GridPos oldPos = { oldX, oldY };             

                            // check if we have a move-to object "in the air"
                            // that is supposed to end up at this location
                            // if so, make it snap there
                            for( int i=0; 
                                 i<mMapExtraMovingObjects.size(); i++ ) {
                                
                                if( equal( 
                                        mMapExtraMovingObjectsDestWorldPos.
                                        getElementDirect( i ),
                                        oldPos ) ) {
                                    endExtraObjectMove( i );
                                    break;
                                    }
                                }
                            


                            int oldMapI = getMapIndex( oldX, oldY );
                            
                            int sourceObjID = 0;
                            if( oldMapI != -1 ) {
                                sourceObjID = mMap[ oldMapI ];
                            
                                // check what move-trans for sourceID
                                // produces.  If it produces something
                                // show that moving instead
                                
                                TransRecord *moveTrans = 
                                    getTrans( -1, sourceObjID );
                                
                                if( moveTrans != NULL &&
                                    moveTrans->move > 0 ) {
                                    sourceObjID = moveTrans->newTarget;
                                    }
                                }
                            
                            ExtraMapObject oldObj;
                            
                            if( old > 0 && sourceObjID != 0 &&
                                getTrans( sourceObjID, old ) != NULL ) {
                                
                                // save old object while we
                                // set up new object
                                oldObj = copyFromMap( mapI );
                                oldObj.objectID = old;
                                if( old == -1 ) {
                                    oldObj.objectID = 0;
                                    }
                                }

                            mMapMoveSpeeds[mapI] = speed;
                            
                            

                            doublePair oldOffset = { 0, 0 };
                            
                            char moveEastOrWest = false;
                            
                            if( oldMapI != -1 ) {
                                // location where we came from
                                oldOffset = mMapMoveOffsets[ oldMapI ];

                                mMapAnimationFrameCount[ mapI ] =
                                    mMapAnimationFrameCount[ oldMapI ];
                                
                                mMapAnimationLastFrameCount[ mapI ] =
                                    mMapAnimationLastFrameCount[ oldMapI ];
                                
        
                                mMapAnimationFrozenRotFrameCount[ mapI ] =
                                    mMapAnimationFrozenRotFrameCount[ 
                                        oldMapI ];
                                
                                mMapCurAnimType[ mapI ] = 
                                    mMapCurAnimType[ oldMapI ];
                                mMapLastAnimType[ mapI ] = 
                                    mMapLastAnimType[ oldMapI ];
                                
                                mMapLastAnimFade[ mapI ] =
                                    mMapLastAnimFade[ oldMapI ];
                                
                                if( mMapLastAnimFade[ mapI ] == 0 &&
                                    mMapCurAnimType[ mapI ] != moving ) {
                                    // not in the middle of a fade
                                    
                                    // fade to moving
                                    mMapLastAnimType[ mapI ] =
                                        mMapCurAnimType[ mapI ];
                                    mMapCurAnimType[ mapI ] = moving;
                                    mMapLastAnimFade[ mapI ] = 1;

                                    if( mMapAnimationFrozenRotFrameCountUsed[
                                            mapI ] ) {
                                        mMapAnimationFrameCount[ mapI ] =
                                            mMapAnimationFrozenRotFrameCount[ 
                                                oldMapI ];
                                        }
                                    }
                                
                                int oldLocID = mMap[ oldMapI ];
                                
                                if( oldLocID > 0 ) {
                                    TransRecord *decayTrans = 
                                        getTrans( -1, oldLocID );
                        
                                    if( decayTrans != NULL ) {
                                        
                                        if( decayTrans->move == 6 ||
                                            decayTrans->move == 7 ) {
                                            moveEastOrWest = true;
                                            }
                                        }
                                    }
                                }

                            double oldTrueX = oldX + oldOffset.x;
                            double oldTrueY = oldY + oldOffset.y;
                            
                            mMapMoveOffsets[mapI].x = oldTrueX - x;
                            mMapMoveOffsets[mapI].y = oldTrueY - y;

                            if( moveEastOrWest || x > oldTrueX ) {
                                mMapTileFlips[mapI] = false;
                                }
                            else if( x < oldTrueX ) {
                                mMapTileFlips[mapI] = true;
                                }

                                                        
                            if( old > 0 && sourceObjID != 0 &&
                                getTrans( sourceObjID, old ) != NULL ) {
                                
                                // now that we've set up new object in dest
                                // copy it
                                ExtraMapObject newObj = copyFromMap( mapI );
                                // leave source in place during move
                                newObj.objectID = sourceObjID;
                                
                                // save it as an extra obj
                                mMapExtraMovingObjects.push_back( newObj );
                                GridPos worldDestPos = { x, y };
                                mMapExtraMovingObjectsDestWorldPos.push_back(
                                    worldDestPos );
                                
                                mMapExtraMovingObjectsDestObjectIDs.push_back(
                                    newID );

                                // put old object back in place for now
                                putInMap( mapI, &oldObj );
                                }


                            if( old == 0 && sourceObjID != 0 ) {
                                // object moving into empty spot
                                // track where it came from as old
                                // so that we don't play initial
                                // creation sound by accident
                                old = sourceObjID;
                                }
                            }
                        else {
                            // if these are still set, even though
                            // this map change is NOT for a moving object
                            // it means that the old object is still busy
                            // arriving at the destination, while a new
                            // change to it has happened.
                            // Try NOT snapping it to its destination when
                            // this happens, and letting it finish its
                            // move in the new state
                            
                            //mMapMoveSpeeds[mapI] = 0;
                            //mMapMoveOffsets[mapI].x = 0;
                            //mMapMoveOffsets[mapI].y = 0;
                            }
                        
                        
                        TransRecord *nextDecayTrans = getTrans( -1, newID );
                        
                        if( nextDecayTrans != NULL ) {
                            
                            if( nextDecayTrans->move == 6 ||
                                nextDecayTrans->move == 7 ) {
                                // this object will move to left/right in future
                                // force no flip now
                                mMapTileFlips[mapI] = false;
                                }
                            }
                        

                        if( oldFloor != floorID && floorID > 0 ) {
                            // floor changed
                            
                            ObjectRecord *obj = getObject( floorID );
                            if( obj->creationSound.numSubSounds > 0 ) {    
                                    
                                playSound( obj->creationSound,
                                           getVectorFromCamera( x, y ) );
                                }
                            }
                        

                        LiveObject *responsiblePlayerObject = NULL;
                        
                        if( responsiblePlayerID > 0 ) {
                            responsiblePlayerObject = 
                                getGameObject( responsiblePlayerID );
                            }
                        
                        if( old > 0 &&
                            newID > 0 &&
                            old != newID &&
                            responsiblePlayerID == - ourID ) {
                            
                            // check for photo triggered
                            if( strstr( getObject( newID )->description,
                                        "+photo" ) != NULL ) {
                                
                                takingPhotoGlobalPos.x = x;
                                takingPhotoGlobalPos.y = y;
                                takingPhotoFlip = mMapTileFlips[ mapI ];
                                takingPhoto = true;
                                }
                            
                            }
                        

                        if( old > 0 &&
                            old == newID &&
                            mMapContainedStacks[mapI].size() > 
                            oldContainedCount &&
                            responsiblePlayerObject != NULL &&
                            responsiblePlayerObject->holdingID == 0 ) {
                            
                            // target is changed container and
                            // responsible player's hands now empty
                            
                            
                            // first, try and play the "using"
                            // sound for the container
                            char soundPlayed = false;
                            
                            SoundUsage contSound = 
                                getObject( mMap[mapI] )->usingSound;
                            
                            if( contSound.numSubSounds > 0 ) {
                                playSound( contSound, 
                                           getVectorFromCamera( x, y ) );
                                soundPlayed = true;
                                }

                            if( ! soundPlayed ) {
                                // no container using sound defined
                                
                                
                                // play player's using sound
                            
                                SoundUsage s = getObject( 
                                    responsiblePlayerObject->displayID )->
                                    usingSound;

                                if( s.numSubSounds > 0 ) {
                                    playSound( s, getVectorFromCamera( x, y ) );
                                    }
                                }
                            }
                        


                        if( responsiblePlayerID == -1 ) {
                            // no one dropped this
                            
                            // our placement status cleared
                            mMapPlayerPlacedFlags[mapI] = false;
                            }
                        

                        // Check if a home marker has been set or removed
                        if( responsiblePlayerID != -1 &&
                            ( old != 0 ||
                              newID != 0 ) ) {
                            // player-triggered change
                            
                            int rID = responsiblePlayerID;
                            if( rID < -1 ) {
                                rID = -rID;
                                }
                            
                            if( rID == ourID ) {
                                // local player triggered
                                
                                char addedOrRemoved = false;
                                
                                if( newID > 0 &&
                                    getObject( newID )->homeMarker ) {
                                    
                                    addHomeLocation( x, y );
                                    addedOrRemoved = true;
                                    }
                                else if( old > 0 &&
                                         getObject( old )->homeMarker ) {
                                    removeHomeLocation( x, y );
                                    addedOrRemoved = true;
                                    }
                                
                                if( addedOrRemoved ) {
                                    // look in region for our home locations
                                    // that may have been removed by others
                                    // (other people pulling up our stake)
                                    // and clear them.  Our player can
                                    // no longer clear them, because they
                                    // don't exist on the map anymore
                                    
                                    for( int ry=y-7; ry<=y+7; ry++ ) {
                                        for( int rx=x-7; rx<=x+7; rx++ ) {
                                    
                                            int mapRX = 
                                                rx - mMapOffsetX + mMapD / 2;
                                            int mapRY = 
                                                ry - mMapOffsetY + mMapD / 2;
                    
                                            if( mapRX >= 0 && mapRX < mMapD
                                                &&
                                                mapRY >= 0 && mapRY < mMapD ) {
                        
                                                int mapRI = 
                                                    mapRY * mMapD + mapRX;
                        
                                                int cellID = mMap[ mapRI ];
                                                
                                                if( cellID == 0 ||
                                                    ( cellID > 0 &&
                                                      ! getObject( cellID )->
                                                        homeMarker ) ) {
                                                    
                                                    removeHomeLocation(
                                                        rx, ry );
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        


                        if( old != newID && 
                            newID != 0 && 
                            responsiblePlayerID <= -1 ) {
                            // ID change, and not just a player setting
                            // an object down
                            
                            ObjectRecord *newObj = getObject( newID );


                            if( old > 0 ) {
                                
                                if( responsiblePlayerID == -1 ) {
                                    // object auto-decayed from some other
                                    // object
                                    
                                    // play decay sound
                                    ObjectRecord *obj = getObject( old );
                                    if( obj->decaySound.numSubSounds > 0 ) {    
                                    
                                        playSound( 
                                            obj->decaySound,
                                            getVectorFromCamera( x, y ) );
                                        }
                                    }
                                else if( responsiblePlayerID < -1 ) {
                                    // player caused this object to change
                                    
                                    // sound will be played elsewhere
                                    }
                                }
                            

                            
                            if( newObj->creationSound.numSubSounds > 0 ) {
                                
                                if( old == 0 && responsiblePlayerID < -1 ) {
                                    // new placement, but not set-down
                                    // we don't have information
                                    // from on-ground objects to 
                                    // check for ancestor relationships

                                    // check what the player is left
                                    // holding instead
                                    
                                    LiveObject *responsiblePlayerObject = 
                                        getGameObject( -responsiblePlayerID );
                                    if( responsiblePlayerObject != NULL ) {
                                        
                                        old = 
                                            responsiblePlayerObject->holdingID;
                                        if( old < 0 ) {
                                            old = 0;
                                            }
                                        
                                        if( old > 0 ) {
                                            TransRecord *p = 
                                                getTransProducing( old,
                                                                   newID );
                                            
                                            if( p != NULL && 
                                                p->actor > 0 ) {
                                                old = p->actor;
                                                }
                                            }
                                        }
                                    }
                                

                                if( shouldCreationSoundPlay( old, newID ) ) {
                                    playSound( newObj->creationSound,
                                               getVectorFromCamera( x, y ) );
                                    }
                                }
                            }
                            


                        if( mMap[mapI] != 0 ) {
                            ObjectRecord *newObj = getObject( mMap[mapI] );
                            
                            if( newObj->permanent && newObj->blocksWalking ) {
                                // clear the locally-stored flip for this
                                // tile
                                mMapTileFlips[mapI] = false;
                                }    
                            }
                        
                        if( old != mMap[mapI] && mMap[mapI] != 0 ) {
                            // new placement
                            
                            printf( "New placement, responsible=%d\n",
                                    responsiblePlayerID );
                            
                            if( mMapMoveSpeeds[mapI] == 0 ) {
                                
                                if( old == 0 ) {
                                    // set down into an empty spot
                                    // reset frame count
                                    mMapAnimationFrameCount[mapI] = 0;
                                    mMapAnimationLastFrameCount[mapI] = 0;
                                    }
                                else {
                                    // else, leave existing frame count alone,
                                    // since object has simply gone through a
                                    // transition
                                    
                                    // UNLESS it is a force-zero-start 
                                    // animation
                                    AnimationRecord *newAnim =
                                        getAnimation( mMap[mapI], ground );
                                    
                                    if( newAnim != NULL  && 
                                        newAnim->forceZeroStart ) {
                                        
                                        mMapAnimationFrameCount[mapI] = 0;
                                        mMapAnimationLastFrameCount[mapI] = 0;
                                        }
                                    }
                                    
                                
                                mMapCurAnimType[mapI] = ground;
                                mMapLastAnimType[mapI] = ground;
                                mMapLastAnimFade[mapI] = 0;
                                mMapAnimationFrozenRotFrameCount[mapI] = 0;
                                }
                            else {
                                mMapLastAnimType[mapI] = mMapCurAnimType[mapI];
                                mMapCurAnimType[mapI] = moving;
                                if( mMapLastAnimType[mapI] != moving ) {
                                    mMapLastAnimFade[mapI] = 1;
                                    }
                                }
                            
                            
                            LiveObject *responsiblePlayerObject = NULL;
                            
                            
                            if( responsiblePlayerID > 0 ) {
                                responsiblePlayerObject = 
                                    getGameObject( responsiblePlayerID );

                                }
                            if( responsiblePlayerID != -1 &&
                                responsiblePlayerID != 0 &&
                                getObjectHeight( mMap[mapI] ) < CELL_D ) {
                                
                                // try flagging objects as through-mousable
                                // for any change caused by a player,
                                // as long as object is small enough
                                mMapPlayerPlacedFlags[mapI] = true;
                                }
                            else if( getObjectHeight( mMap[mapI] ) >= CELL_D ) {
                                // object that has become tall enough
                                // that through-highlights feel strange
                                mMapPlayerPlacedFlags[mapI] = false;
                                }
                            // don't forget placement flags for objects
                            // that someone placed but have naturally changed,
                            // but remain short
                            
                            
                            
                            if( responsiblePlayerObject == NULL ||
                                !responsiblePlayerObject->onScreen ) {
                                
                                // set it down instantly, no drop animation
                                // (player's held offset isn't valid)
                                AnimationRecord *animR = 
                                    getAnimation( mMap[mapI], ground );

                                if( animR != NULL && 
                                    animR->randomStartPhase ) {
                                    mMapAnimationFrameCount[mapI] = 
                                        randSource.getRandomBoundedInt( 
                                            0, 10000 );
                                    mMapAnimationLastFrameCount[i] = 
                                        mMapAnimationFrameCount[mapI];
                                    }
                                mMapDropOffsets[mapI].x = 0;
                                mMapDropOffsets[mapI].y = 0;
                                mMapDropRot[mapI] = 0;
                                mMapDropSounds[mapI] = blankSoundUsage;

                                if( responsiblePlayerObject != NULL ) {
                                    // copy their flip, even if off-screen
                                    mMapTileFlips[mapI] =
                                        responsiblePlayerObject->holdingFlip;
                                    }
                                else if( responsiblePlayerID < -1 &&
                                         old == 0 && 
                                         mMap[ mapI ] > 0 &&
                                         ! getObject( mMap[ mapI ] )->
                                         permanent ) {
                                    // use-on-bare-ground
                                    // with non-permanent result
                                    // honor flip direction of player
                                    mMapTileFlips[mapI] =
                                        getLiveObject( -responsiblePlayerID )->
                                        holdingFlip;
                                    }
                                }
                            else {
                                // copy last frame count from last holder
                                // of this object (server tracks
                                // who was holding it and tell us about it)
                                
                                mMapLastAnimType[mapI] = held;
                                mMapLastAnimFade[mapI] = 1;
                                        
                                mMapAnimationFrozenRotFrameCount[mapI] =
                                    responsiblePlayerObject->
                                    heldFrozenRotFrameCount;
                                        
                                        
                                if( responsiblePlayerObject->
                                    lastHeldAnimFade == 0 ) {
                                            
                                    mMapAnimationLastFrameCount[mapI] =
                                        responsiblePlayerObject->
                                        heldAnimationFrameCount;
                                            
                                    mMapLastAnimType[mapI] = 
                                        responsiblePlayerObject->curHeldAnim;
                                    }
                                else {
                                    // dropped object was already
                                    // in the middle of a fade
                                    mMapCurAnimType[mapI] =
                                        responsiblePlayerObject->curHeldAnim;
                                            
                                    mMapAnimationFrameCount[mapI] =
                                        responsiblePlayerObject->
                                        heldAnimationFrameCount;
                                    
                                    mMapLastAnimType[mapI] =
                                        responsiblePlayerObject->lastHeldAnim;
                                    
                                    mMapAnimationLastFrameCount[mapI] =
                                        responsiblePlayerObject->
                                        lastHeldAnimationFrameCount;
                                    
                                    
                                    mMapLastAnimFade[mapI] =
                                        responsiblePlayerObject->
                                        lastHeldAnimFade;
                                    }
                                        
                                            
                                
                                mMapDropOffsets[mapI].x = 
                                    responsiblePlayerObject->
                                    heldObjectPos.x - x;
                                        
                                mMapDropOffsets[mapI].y = 
                                    responsiblePlayerObject->
                                    heldObjectPos.y - y;
                                        
                                mMapDropRot[mapI] = 
                                    responsiblePlayerObject->heldObjectRot;

                                mMapDropSounds[mapI] =
                                    getObject( 
                                        responsiblePlayerObject->displayID )->
                                    usingSound;

                                mMapTileFlips[mapI] =
                                    responsiblePlayerObject->holdingFlip;
                                        
                                if( responsiblePlayerObject->holdingID > 0 &&
                                    old == 0 ) {
                                    // use on bare ground transition
                                    
                                    // don't use drop offset
                                    mMapDropOffsets[mapI].x = 0;
                                    mMapDropOffsets[mapI].y = 0;
                                    
                                    mMapDropRot[mapI] = 0;
                                    mMapDropSounds[mapI] = 
                                        blankSoundUsage;
                                    
                                    if( getObject( mMap[ mapI ] )->
                                        permanent ) {
                                        // resulting in something 
                                        // permanent
                                        // on ground.  Never flip it
                                        mMapTileFlips[mapI] = false;
                                        }
                                    }
                                
                                
                                
                                if( x > 
                                    responsiblePlayerObject->xServer ) {
                                    responsiblePlayerObject->holdingFlip = 
                                        false;
                                    }
                                else if( x < 
                                         responsiblePlayerObject->xServer ) {
                                    responsiblePlayerObject->holdingFlip = 
                                        true;
                                    }
                                }
                            }
                        }
                    }
                
                delete [] lines[i];
                }
            
            delete [] lines;
            }
        else if( type == PLAYER_UPDATE ) {
            
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip fist
                delete [] lines[0];
                }
            
            // for babies that are held, but don't exist yet in 
            // client because PU creating them hasn't been received yet
            // assume these always arrive in the same PU message
            SimpleVector<int> unusedHolderID;
            SimpleVector<int> unusedHeldID;
            
            for( int i=1; i<numLines; i++ ) {

                LiveObject o;

                o.onScreen = false;
                
                o.allSpritesLoaded = false;
                
                o.holdingID = 0;
                o.heldLearned = 0;
                
                o.useWaypoint = false;

                o.pathToDest = NULL;
                o.containedIDs = NULL;
                o.subContainedIDs = NULL;
                
                o.onFinalPathStep = false;
                
                o.age = 0;
                o.finalAgeSet = false;
                
                o.outOfRange = false;
                o.dying = false;
                o.sick = false;
                
                o.lineageEveID = -1;

                o.name = NULL;
                o.relationName = NULL;
                o.warPeaceStatus = 0;
                
                o.curseLevel = 0;
                o.curseName = NULL;
                
                o.excessCursePoints = 0;
                o.curseTokenCount = 0;

                o.tempAgeOverrideSet = false;
                o.tempAgeOverride = 0;

                // don't track these for other players
                o.foodStore = 0;
                o.foodCapacity = 0;                

                o.maxFoodStore = 0;
                o.maxFoodCapacity = 0;

                o.currentSpeech = NULL;
                o.speechFade = 1.0;
                o.speechIsSuccessfulCurse = false;
                o.speechIsCurseTag = false;
                o.lastCurseTagDisplayTime = 0;
                o.speechIsOverheadLabel = false;
                
                o.heldByAdultID = -1;
                o.heldByAdultPendingID = -1;
                
                o.heldByDropOffset.x = 0;
                o.heldByDropOffset.y = 0;
                
                o.babyWiggle = false;

                o.ridingOffset.x = 0;
                o.ridingOffset.y = 0;

                o.animationFrameCount = 0;
                o.heldAnimationFrameCount = 0;

                o.lastAnimationFrameCount = 0;
                o.lastHeldAnimationFrameCount = 0;
                
                
                o.frozenRotFrameCount = 0;
                o.heldFrozenRotFrameCount = 0;
                
                o.frozenRotFrameCountUsed = false;
                o.heldFrozenRotFrameCountUsed = false;
                o.clothing = getEmptyClothingSet();
                
                o.currentMouseOverClothingIndex = -1;
                
                for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                    o.clothingHighlightFades[c] = 0;
                    }
                

                o.somePendingMessageIsMoreMovement = false;

                
                o.actionTargetTweakX = 0;
                o.actionTargetTweakY = 0;
                
                o.currentEmot = NULL;
                o.emotClearETATime = 0;
                
                o.killMode = false;
                o.killWithID = -1;
                o.chasingUs = false;

                o.followingID = -1;
                o.highestLeaderID = -1;
                o.leadershipLevel = 0;
                o.personalLeadershipColor.r = 1;
                o.personalLeadershipColor.g = 1;
                o.personalLeadershipColor.b = 1;
                o.personalLeadershipColor.a = 1;
                o.hasBadge = false;
                o.isExiled = false;
                o.isDubious = false;
                o.followingUs = false;
                o.leadershipNameTag = NULL;
                
                o.isGeneticFamily = false;
                

                int forced = 0;
                int done_moving = 0;
                
                char *holdingIDBuffer = new char[500];

                int heldOriginValid, heldOriginX, heldOriginY,
                    heldTransitionSourceID;
                
                char *clothingBuffer = new char[500];
                
                int justAte = 0;
                int justAteID = 0;
                
                int facingOverride = 0;
                int actionAttempt = 0;
                int actionTargetX = 0;
                int actionTargetY = 0;
                
                double invAgeRate = 60.0;
                
                int responsiblePlayerID = -1;
                
                int heldYum = 0;
                int heldLearned = 1;
                
                int numRead = sscanf( lines[i], 
                                      "%d %d "
                                      "%d "
                                      "%d "
                                      "%d %d "
                                      "%499s %d %d %d %d %f %d %d %d %d "
                                      "%lf %lf %lf %499s %d %d %d "
                                      "%d %d",
                                      &( o.id ),
                                      &( o.displayID ),
                                      &facingOverride,
                                      &actionAttempt,
                                      &actionTargetX,
                                      &actionTargetY,
                                      holdingIDBuffer,
                                      &heldOriginValid,
                                      &heldOriginX,
                                      &heldOriginY,
                                      &heldTransitionSourceID,
                                      &( o.heat ),
                                      &done_moving,
                                      &forced,
                                      &( o.xd ),
                                      &( o.yd ),
                                      &( o.age ),
                                      &invAgeRate,
                                      &( o.lastSpeed ),
                                      clothingBuffer,
                                      &justAte,
                                      &justAteID,
                                      &responsiblePlayerID,
                                      &heldYum,
                                      &heldLearned );
                
                
                // heldYum is 24th value, optional
                // heldLearned is 26th value, optional
                if( numRead >= 23 ) {

                    applyReceiveOffset( &actionTargetX, &actionTargetY );
                    applyReceiveOffset( &heldOriginX, &heldOriginY );
                    applyReceiveOffset( &( o.xd ), &( o.yd ) );
                    
                    printf( "PLAYER_UPDATE with heldOrVal=%d, "
                            "heldOrx=%d, heldOry=%d, "
                            "pX=%d, pY=%d, heldTransSrcID=%d, "
                            "holdingString=%s\n",
                            heldOriginValid, heldOriginX, heldOriginY,
                            o.xd, o.yd, heldTransitionSourceID, 
                            holdingIDBuffer );
                    if( forced ) {
                        printf( "  POSITION FORCED\n" );
                        }

                    o.lastAgeSetTime = game_getCurrentTime();

                    o.ageRate = 1.0 / invAgeRate;

                    int numClothes;
                    char **clothes = split( clothingBuffer, ";", &numClothes );
                        
                    if( numClothes == NUM_CLOTHING_PIECES ) {
                        
                        for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                            
                            int numParts;
                            char **parts = split( clothes[c], ",", &numParts );
                            
                            if( numParts > 0 ) {
                                int id = 0;
                                sscanf( parts[0], "%d", &id );
                                
                                if( id != 0 ) {
                                    setClothingByIndex( &( o.clothing ), 
                                                        c, 
                                                        getObject( id ) );
                                    
                                    if( numParts > 1 ) {
                                        for( int p=1; p<numParts; p++ ) {
                                            
                                            int cID = 0;
                                            sscanf( parts[p], "%d", &cID );
                                            
                                            if( cID != 0 ) {
                                                
                                                o.clothingContained[c].
                                                    push_back( cID );
                                                }
                                            }
                                        }
                                    }
                                }
                            for( int p=0; p<numParts; p++ ) {
                                delete [] parts[p];
                                }
                            delete [] parts;
                            }

                        }
                    
                    for( int c=0; c<numClothes; c++ ) {
                        delete [] clothes[c];
                        }
                    delete [] clothes;

                    
                    if( strstr( holdingIDBuffer, "," ) != NULL ) {
                        int numInts;
                        char **ints = split( holdingIDBuffer, ",", &numInts );
                        
                        o.holdingID = atoi( ints[0] );
                        
                        delete [] ints[0];

                        o.numContained = numInts - 1;
                        o.containedIDs = new int[ o.numContained ];
                        o.subContainedIDs = 
                            new SimpleVector<int>[ o.numContained ];
                        
                        for( int c=0; c<o.numContained; c++ ) {
                            o.containedIDs[c] = atoi( ints[ c + 1 ] );
                            
                            if( strstr( ints[c + 1], ":" ) != NULL ) {
                                // sub-container items
                                
                                int numSubInts;
                                char **subInts = 
                                    split( ints[c + 1], ":", &numSubInts );
                                
                                delete [] subInts[0];
                                int numSubCont = numSubInts - 1;
                                for( int s=0; s<numSubCont; s++ ) {
                                    o.subContainedIDs[c].push_back(
                                        atoi( subInts[ s + 1 ] ) );
                                    delete [] subInts[ s + 1 ];
                                    }
                                
                                delete [] subInts;
                                }

                            delete [] ints[ c + 1 ];
                            }
                        delete [] ints;
                        }
                    else {
                        // a single int
                        o.holdingID = atoi( holdingIDBuffer );
                        o.numContained = 0;
                        }
                    
                    
                    o.xServer = o.xd;
                    o.yServer = o.yd;

                    LiveObject *existing = NULL;

                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == o.id ) {
                            existing = gameObjects.getElement(j);
                            break;
                            }
                        }

                    
                    if( existing != NULL ) {
                        existing->heldLearned = heldLearned;
                        }
                    

                    if( existing != NULL &&
                        existing->id == ourID ) {
                        // got a PU for self

                        if( existing->holdingID != o.holdingID ) {
                            // holding change
                            // if we have a temp home arrow
                            // we've now dropped the map
                            // start a fade-out timer
                            HomePos *r = getHomePosRecord();
                            
                            if( r != NULL && r->temporary &&
                                r->temporaryExpireETA == 0 ) {
                                r->temporaryExpireETA = 
                                    game_getCurrentTime() + 60;
                                }
                            }
                        
                        
                        mYumSlipPosTargetOffset[2] = mYumSlipHideOffset[2];
                        mYumSlipPosTargetOffset[3] = mYumSlipHideOffset[3];
                        
                        int slipIndexToShow = -1;
                        if( heldYum ) {
                            // YUM
                            slipIndexToShow = 2;
                            }
                        else {
                            if( o.holdingID > 0 &&
                                getObject( o.holdingID )->foodValue > 0 ) {
                                // MEH
                                slipIndexToShow = 3;
                                }
                            }
                        
                        if( slipIndexToShow >= 0 ) {
                            mYumSlipPosTargetOffset[slipIndexToShow].y += 36;
                            }

                        if( existing->lastActionSendStartTime != 0 ) {
                            
                            // PU for an action that we sent
                            // measure round-trip time
                            existing->lastResponseTimeDelta = 
                                game_getCurrentTime() - 
                                existing->lastActionSendStartTime;
                            
                            existing->lastActionSendStartTime = 0;
                            }
                        }
                    
                    
                    if( existing != NULL &&
                        existing->destTruncated &&
                        ( existing->xd != o.xd ||
                          existing->yd != o.yd ) ) {
                        // edge case:
                        // our move was truncated due to an obstacle
                        // but then after that we tried to move to what
                        // the server saw as our current location
                        // the server will send a PU for that last move
                        // to end it immediately, but our xd,yd will still
                        // be set from the truncated move
                        // treat this PU as a force
                        printf( "Artificially forcing PU position %d,%d "
                                "because it mismatches our last truncated "
                                "move destination %d,%d\n",
                                o.xd, o.yd, existing->xd, existing->yd );
                        forced = true;
                        }
                    

                    char alreadyHeldAsPending = false;
                    
                    if( existing != NULL ) {

                        if( o.holdingID < 0 ) {
                            // this held PU talks about held baby

                            int heldBabyID = - o.holdingID;
                            
                            LiveObject *heldBaby = getLiveObject( heldBabyID );
                            
                            if( heldBaby != NULL ) {

                                // clear up held status on baby if old
                                // holding adult is out of range
                                // (we're not getting updates about them
                                //  anymore).
                               
                                if( heldBaby->heldByAdultID > 0 && 
                                    heldBaby->heldByAdultID != existing->id ) {
                                    
                                    LiveObject *oldHolder =
                                        getLiveObject( 
                                            heldBaby->heldByAdultID );
                                    
                                    if( oldHolder != NULL &&
                                        oldHolder->outOfRange ) {
                                        heldBaby->heldByAdultID = -1;
                                        }
                                    }

                                if( heldBaby->heldByAdultPendingID > 0 && 
                                    heldBaby->heldByAdultPendingID != 
                                    existing->id ) {
                                    
                                    LiveObject *oldPendingHolder =
                                        getLiveObject( 
                                            heldBaby->heldByAdultPendingID );
                                    
                                    if( oldPendingHolder != NULL &&
                                        oldPendingHolder->outOfRange ) {
                                        heldBaby->heldByAdultPendingID = -1;
                                        }
                                    }

                                
                                // now handle changes to held status
                                if( heldBaby->heldByAdultID == existing->id ) {
                                    // baby already knows it's held
                                    }
                                else if( heldBaby->heldByAdultID != -1 ) {
                                    // baby thinks it's held by another adult

                                    // stick this pending message on that
                                    // adult's queue instead
                                    LiveObject *holdingAdult =
                                        getLiveObject( 
                                            heldBaby->heldByAdultID );
                                    
                                    if( holdingAdult != NULL ) {

                                        char *pendingMessage = 
                                            autoSprintf( "PU\n%s\n#",
                                                         lines[i] );
                                        
                                        holdingAdult->pendingReceivedMessages.
                                            push_back( pendingMessage );
                                        
                                        alreadyHeldAsPending = true;
                                        }
                                    }
                                else if( heldBaby->heldByAdultPendingID == 
                                         -1 ){
                                    // mark held pending
                                    heldBaby->heldByAdultPendingID =
                                        existing->id;
                                    }
                                else if( heldBaby->heldByAdultPendingID !=
                                         existing->id ) {
                                    // already pending held by some other adult
                                    
                                    // stick this pending message on that
                                    // adult's queue instead
                                    LiveObject *holdingAdult =
                                        getLiveObject( 
                                            heldBaby->heldByAdultPendingID );
                                    
                                    if( holdingAdult != NULL ) {
                                        char *pendingMessage = 
                                            autoSprintf( "PU\n%s\n#",
                                                         lines[i] );
                                        holdingAdult->pendingReceivedMessages.
                                            push_back( pendingMessage );
                                        
                                        alreadyHeldAsPending = true;
                                        }
                                    }
                                }
                            }
                        }
                    
                    if( alreadyHeldAsPending ) {
                        }
                    else if( existing != NULL &&
                        existing->id != ourID &&
                        existing->currentSpeed != 0 &&
                        ! forced &&
                        ( done_moving > 0 ||
                          existing->pendingReceivedMessages.size() > 0 ) ) {

                        // non-forced update about other player 
                        // while we're still playing their last movement
                        
                        // sometimes this can mean a move truncation
                        if( existing->pathToDest != NULL &&
                            existing->pathLength > 0 ) {

                            GridPos pathEnd = 
                                existing->pathToDest[ 
                                    existing->pathLength - 1 ];

                            if( o.xd == pathEnd.x &&
                                o.yd == pathEnd.y ) {
                                // PU destination matches our current path dest
                                // no move truncation
                                }
                            else if( done_moving > 0 ) {
                                // PU should be somewhere along our path
                                // a truncated move
                                
                                // truncate our path there
                                for( int p=existing->pathLength - 1;
                                     p >= 0; p-- ) {
                                    
                                    GridPos thisStep = 
                                        existing->pathToDest[ p ];
                                    
                                    if( thisStep.x == o.xd &&
                                        thisStep.y == o.yd ) {
                                        // found

                                        // cut off
                                        existing->pathLength = p + 1;
                                        
                                        // navigate there
                                        if( p > 0 ) {
                                            existing->currentPathStep = p - 1;
                                            }
                                        else {
                                            existing->currentPathStep = p;
                                            }
                                        
                                        // set new truncated dest
                                        existing->xd = o.xd;
                                        existing->yd = o.yd;
                                        break;
                                        }
                                    }

                                if( existing->pathLength == 1 ) {
                                    fixSingleStepPath( existing );
                                    }
                                }
                            }
                        
                        if( done_moving > 0  ||
                            existing->pendingReceivedMessages.size() > 0 ) {
                            
                            // this PU happens after they are done moving
                            // or it happens mid-move, but we already
                            // have messages held, so it may be meant
                            // to happen in the middle of their next move
                            
                            // defer it until they're done moving
                            printf( "Holding PU message for "
                                    "%d until later, "
                                    "%d other messages pending for them\n",
                                    existing->id,
                                    existing->pendingReceivedMessages.size() );
                            
                            existing->pendingReceivedMessages.push_back(
                                autoSprintf( "PU\n%s\n#",
                                             lines[i] ) );
                            }
                        }
                    else if( existing != NULL &&
                             existing->heldByAdultID != -1 &&
                             getLiveObject( existing->heldByAdultID ) != NULL &&
                             getLiveObject( existing->heldByAdultID )->
                                 pendingReceivedMessages.size() > 0 ) {
                        // we're held by an adult who has pending
                        // messages to play at the end of their movement
                        // (server sees that their movement has already ended)
                        // Thus, an update about us should be played later also
                        // whenever the adult's messages are played back
                        
                        LiveObject *holdingPlayer = 
                            getLiveObject( existing->heldByAdultID );

                        printf( "Holding PU message for baby %d held by %d "
                                "until later, "
                                "%d other messages pending for them\n",
                                existing->id,
                                existing->heldByAdultID,
                                holdingPlayer->pendingReceivedMessages.size() );

                        holdingPlayer->
                            pendingReceivedMessages.push_back(
                                autoSprintf( "PU\n%s\n#",
                                             lines[i] ) );
                        }
                    else if( existing != NULL &&
                             existing->heldByAdultPendingID != -1 &&
                             getLiveObject( existing->heldByAdultPendingID ) 
                             != NULL &&
                             getLiveObject( existing->heldByAdultPendingID )->
                                 pendingReceivedMessages.size() > 0 ) {
                        // we're pending to be held by an adult who has pending
                        // messages to play at the end of their movement
                        // (server sees that their movement has already ended)
                        // Thus, an update about us should be played later also
                        // whenever the adult's messages are played back
                        
                        LiveObject *holdingPlayer = 
                            getLiveObject( existing->heldByAdultPendingID );

                        printf( "Holding PU message for baby %d with "
                                "pending held by %d "
                                "until later, "
                                "%d other messages pending for them\n",
                                existing->id,
                                existing->heldByAdultPendingID,
                                holdingPlayer->pendingReceivedMessages.size() );

                        holdingPlayer->
                            pendingReceivedMessages.push_back(
                                autoSprintf( "PU\n%s\n#",
                                             lines[i] ) );
                        }
                    else if( existing != NULL &&
                             responsiblePlayerID != -1 &&
                             getLiveObject( responsiblePlayerID ) != NULL &&
                             getLiveObject( responsiblePlayerID )->
                                 pendingReceivedMessages.size() > 0 ) {
                        // someone else is responsible for this change
                        // to us (we're likely a baby) and that person
                        // is still in the middle of a local walk
                        // with pending messages that will play
                        // after the walk.  Defer this message too

                        LiveObject *rO = 
                            getLiveObject( responsiblePlayerID );
                        
                        printf( "Holding PU message for %d caused by %d "
                                "until later, "
                                "%d other messages pending for them\n",
                                existing->id,
                                responsiblePlayerID,
                                rO->pendingReceivedMessages.size() );

                        rO->pendingReceivedMessages.push_back(
                            autoSprintf( "PU\n%s\n#",
                                         lines[i] ) );
                        }         
                    else if( existing != NULL ) {
                        int oldHeld = existing->holdingID;
                        int heldContChanged = false;
                        
                        if( oldHeld > 0 &&
                            oldHeld == existing->holdingID ) {
                            
                            heldContChanged =
                                checkIfHeldContChanged( &o, existing );
                            }
                        
                        
                        // receiving a PU means they aren't out of
                        // range anymore
                        if( existing->outOfRange ) {
                            // was out of range before
                            // this update is forced
                            existing->currentPos.x = o.xd;
                            existing->currentPos.y = o.yd;
                            
                            existing->currentSpeed = 0;
                            existing->currentGridSpeed = 0;
                            
                            existing->xd = o.xd;
                            existing->yd = o.yd;
                            existing->destTruncated = false;

                            // clear an existing path, since they may no
                            // longer be on it
                            if( existing->pathToDest != NULL ) {
                                delete [] existing->pathToDest;
                                existing->pathToDest = NULL;
                                }

                            }
                        existing->outOfRange = false;

                        
                        existing->lastHoldingID = oldHeld;
                        existing->holdingID = o.holdingID;
                        
                        if( o.id == ourID &&
                            existing->holdingID > 0 &&
                            existing->holdingID != oldHeld ) {
                            // holding something new
                            // hint about it
                            
                            mNextHintObjectID = existing->holdingID;
                            
                            if( isHintFilterStringInvalid() ) {
                                mNextHintIndex = 
                                    mHintBookmarks[ mNextHintObjectID ];
                                }
                            }
                        


                        ObjectRecord *newClothing = 
                            getClothingAdded( &( existing->clothing ), 
                                              &( o.clothing ) );
                        

                        if( newClothing == NULL ) {
                            // has something been removed instead?
                            newClothing = 
                                getClothingAdded( &( o.clothing ),
                                                  &( existing->clothing ) );
                            }
                        
                        char clothingSoundPlayed = false;

                        char clothingChanged = false;
                        
                        if( newClothing != NULL ) {
                            clothingChanged = true;
                            
                            SoundUsage clothingSound = 
                                newClothing->usingSound;
                            
                            if( clothingSound.numSubSounds > 0 ) {
                                playSound( clothingSound, 
                                           getVectorFromCamera( 
                                               o.xd,
                                               o.yd ) );
                                
                                clothingSoundPlayed = true;
                                }
                            else {
                                // play generic pickup sound
                                ObjectRecord *existingObj = 
                                    getObject( existing->displayID );
                                
                                if( existingObj->usingSound.numSubSounds > 0 ) {
                                    
                                    playSound( existingObj->usingSound,
                                               getVectorFromCamera(
                                                   o.xd,
                                                   o.yd ) );
                                    clothingSoundPlayed = true;
                                    }
                                }
                            }

                        existing->clothing = o.clothing;


                        // what we're holding hasn't changed
                        // maybe action failed
                        if( o.id == ourID && existing->holdingID == oldHeld ) {
                            
                            LiveObject *ourObj = getOurLiveObject();
                            
                            if( ourObj->pendingActionAnimationProgress != 0 &&
                                ! ourObj->inMotion ) {
                                
                                addNewAnimPlayerOnly( existing, ground2 );
                                }
                            }
                        
                        if( o.id != ourID ) {
                            
                            if( actionAttempt && ! justAte &&
                                nearEndOfMovement( existing ) ) {
                                existing->actionTargetX = actionTargetX;
                                existing->actionTargetY = actionTargetY;
                                existing->pendingActionAnimationProgress = 
                                    0.025 * frameRateFactor;
                                }

                            if( heldOriginValid || 
                                facingOverride != 0 ) {
                                        
                                if( ( heldOriginValid && 
                                      heldOriginX > existing->xd )
                                    ||
                                    facingOverride == 1 ) {
                                    
                                    existing->holdingFlip = false;
                                    }
                                else if( ( heldOriginValid && 
                                           heldOriginX < existing->xd ) 
                                         ||
                                         facingOverride == -1 ) {
                                    
                                    existing->holdingFlip = true;
                                    }
                                }
                            }
                        else if( o.id == ourID &&
                                 actionAttempt &&
                                 ! justAte &&
                                 existing->killMode &&
                                 // they were still holding the same weapon
                                 // before this update
                                 existing->killWithID == oldHeld &&
                                 // their weapon changed as a result of this
                                 // update
                                 existing->holdingID != oldHeld ) {
                            
                            // show kill "doing" animation and bounce
                            
                            playerActionTargetX = actionTargetX;
                            playerActionTargetY = actionTargetY;
                            
                            addNewAnimPlayerOnly( existing, doing );

                            if( existing->pendingActionAnimationProgress 
                                == 0 ) {    
                                existing->pendingActionAnimationProgress = 
                                    0.025 * frameRateFactor;
                                }
                            
                            if( facingOverride == 1 ) {
                                existing->holdingFlip = false;
                                }
                            else if( facingOverride == -1 ) {
                                existing->holdingFlip = true;
                                }
                            }
                        
                        
                        char creationSoundPlayed = false;
                        char otherSoundPlayed = false;
                        char groundSoundPlayed = false;
                        

                        if( justAte && 
                            o.id != ourID && 
                            existing->holdingID == oldHeld ) {
                            // seems like this PU is about player being
                            // fed by someone else

                            // don't interrupt walking
                            // but still play sound
                            if( nearEndOfMovement( existing ) ) {
                                addNewAnimPlayerOnly( 
                                    existing, eating );
                                }

                            ObjectRecord *ateObj =
                                getObject( justAteID );
                                        
                            if( ateObj->eatingSound.numSubSounds > 0 ) {
                                playSound( 
                                    ateObj->eatingSound,
                                    getVectorFromCamera( 
                                        existing->currentPos.x, 
                                        existing->currentPos.y ) );
                                otherSoundPlayed = true;
                                }
                            }

                        
                        if( justAte && o.id == ourID ) {
                            // we just heard from server that we
                            // finished eating
                            // play sound now
                            ObjectRecord *ateObj =
                                getObject( justAteID );
                            if( ateObj->eatingSound.numSubSounds > 0 ) {
                                playSound( 
                                    ateObj->eatingSound,
                                    getVectorFromCamera( 
                                        existing->currentPos.x, 
                                        existing->currentPos.y ) );
                                otherSoundPlayed = true;
                                }
                            if( strstr( ateObj->description, "remapStart" )
                                != NULL ) {
                                
                                if( mRemapPeak == 0 ) {
                                    // reseed
                                    setRemapSeed( 
                                        remapRandSource.getRandomBoundedInt( 
                                            0,
                                            10000000 ) );
                                    mRemapDelay = 0;
                                    }
                                        
                                // closer to max peak
                                // zeno's paradox style
                                mRemapPeak += 0.5 * ( 1 - mRemapPeak );
                                
                                // back toward peak
                                mRemapDirection = 1.0;
                                }
                            }
                        
                        
                        if( existing->holdingID == 0 ) {
                                                        
                            // don't reset these when dropping something
                            // leave them in place so that dropped object
                            // can use them for smooth fade
                            // existing->animationFrameCount = 0;
                            // existing->curAnim = ground;
                            
                            //existing->lastAnim = ground;
                            //existing->lastAnimFade = 0;
                            if( oldHeld != 0 ) {
                                if( o.id == ourID ) {
                                    if( existing->curAnim == doing ||
                                        existing->curAnim == eating ) {
                                        addNewAnimPlayerOnly( 
                                            existing, ground );
                                        }
                                    }
                                else {
                                    if( justAte ) {
                                        // don't interrupt walking
                                        // but still play sound
                                        if( nearEndOfMovement( existing ) ) {
                                            addNewAnimPlayerOnly( 
                                                existing, eating );
                                            }

                                        if( oldHeld > 0 ) {
                                            ObjectRecord *oldHeldObj =
                                                getObject( oldHeld );
                                        
                                            if( oldHeldObj->
                                                eatingSound.numSubSounds > 0 ) {
                                            
                                                playSound( 
                                                    oldHeldObj->eatingSound,
                                                    getVectorFromCamera( 
                                                     existing->currentPos.x, 
                                                     existing->currentPos.y ) );
                                                otherSoundPlayed = true;
                                                }
                                            }
                                        }
                                    else {
                                        // don't interrupt walking
                                        if( nearEndOfMovement( existing ) ) {
                                            addNewAnimPlayerOnly( 
                                                existing, doing );
                                            }
                                        }
                                    
                                    // don't interrupt walking
                                    if( nearEndOfMovement( existing ) ) {
                                        addNewAnimPlayerOnly( existing, 
                                                              ground );
                                        }
                                    }
                                }
                            }
                        else if( oldHeld != existing->holdingID ||
                                 heldContChanged ) {
                            // holding something new
                            
                            // what we're holding has gone through
                            // transition.  Keep old animation going
                            // for what's held
                            
                            if( o.id == ourID ) {
                                addNewAnimPlayerOnly( existing, ground2 );
                                }
                            else {
                                if( justAte ) {
                                    // don't interrupt walking
                                    // but still play sound
                                    if( nearEndOfMovement( existing ) ) {    
                                        addNewAnimPlayerOnly( 
                                            existing, eating );
                                        }
                                    if( oldHeld > 0 ) {
                                        
                                        ObjectRecord *oldHeldObj =
                                            getObject( oldHeld );
                                        
                                        if( oldHeldObj->eatingSound.numSubSounds
                                            > 0 ) {
                                        
                                            playSound( 
                                                oldHeldObj->eatingSound,
                                                getVectorFromCamera( 
                                                    existing->currentPos.x, 
                                                    existing->currentPos.y ) );
                                            otherSoundPlayed = true;
                                            }
                                        }
                                    }
                                else {
                                    // don't interrupt walking
                                    if( actionAttempt && 
                                        nearEndOfMovement( existing ) ) {
                                        addNewAnimPlayerOnly( 
                                            existing, doing );
                                        }
                                    }
                                // don't interrupt walking
                                if( nearEndOfMovement( existing ) ) {
                                    addNewAnimPlayerOnly( existing, ground2 );
                                    }
                                }
                            
                            if( heldOriginValid ) {
                                
                                // use player's using sound for pickup
                                ObjectRecord *existingObj = 
                                    getObject( existing->displayID );
                                
                                if( existingObj->usingSound.numSubSounds > 0 ) {
                                    
                                    playSound( existingObj->usingSound,
                                               getVectorFromCamera(
                                                   heldOriginX,
                                                   heldOriginY ) );
                                    otherSoundPlayed = true;
                                    }


                                // transition from last ground animation
                                // of object, keeping that frame count
                                // for smooth transition
                                
                                if( ! existing->onScreen ) {
                                    // off-screen, instant pickup,
                                    // no transition animation
                                    existing->heldPosOverride = false;
                                    existing->heldObjectPos = 
                                        existing->currentPos;
                                    existing->heldObjectRot = 0;
                                    
                                    existing->lastHeldAnimFade = 0;
                                    existing->curHeldAnim = held;
                                    }
                                else {
                                    // on-screen, slide into position
                                    // smooth animation transition
                                    existing->heldPosSlideStepCount = 0;
                                    existing->heldPosOverride = true;
                                    existing->heldPosOverrideAlmostOver = false;
                                    existing->heldObjectPos.x = heldOriginX;
                                    existing->heldObjectPos.y = heldOriginY;


                                    // check if held origin needs
                                    // tweaking because what we picked up
                                    // had a moving offset
                                    int mapHeldOriginX = 
                                        heldOriginX - mMapOffsetX + mMapD / 2;
                                    int mapHeldOriginY = 
                                        heldOriginY- mMapOffsetY + mMapD / 2;
                    
                                    if( mapHeldOriginX >= 0 && 
                                        mapHeldOriginX < mMapD
                                        &&
                                        mapHeldOriginY >= 0 && 
                                        mapHeldOriginY < mMapD ) {
                                        
                                        int mapHeldOriginI = 
                                            mapHeldOriginY * mMapD + 
                                            mapHeldOriginX;
                                        
                                        if( mMapMoveSpeeds[ mapHeldOriginI ]
                                            > 0 &&
                                            ( mMapMoveOffsets[ 
                                                  mapHeldOriginI ].x
                                              != 0 
                                              ||
                                              mMapMoveOffsets[ 
                                                  mapHeldOriginI ].y
                                              != 0 ) ) {

                                            // only do this if tweak not already
                                            // set.  Don't want to interrupt
                                            // an existing tweak
                                            if( existing->actionTargetTweakX ==
                                                0 &&
                                                existing->actionTargetTweakY ==
                                                0 ) {
                                                
                                                existing->actionTargetTweakX =
                                                    lrint( 
                                                        mMapMoveOffsets[
                                                        mapHeldOriginI ].x );
                                            
                                                existing->actionTargetTweakY =
                                                    lrint( mMapMoveOffsets[
                                                           mapHeldOriginI ].y );
                                                }
                                            
                                            existing->heldObjectPos =
                                                add( existing->heldObjectPos,
                                                     mMapMoveOffsets[ 
                                                         mapHeldOriginI ] );
                                            }    
                                        }
                                    

                                    existing->heldObjectRot = 0;
                                
                                    
                                    int mapX = 
                                        heldOriginX - mMapOffsetX + mMapD / 2;
                                    int mapY = 
                                        heldOriginY - mMapOffsetY + mMapD / 2;
                                    
                                    if( mapX >= 0 && mapX < mMapD
                                        &&
                                        mapY >= 0 && mapY < mMapD ) {
                                        
                                        int mapI = mapY * mMapD + mapX;
                                        
                                        existing->heldFrozenRotFrameCount =
                                            mMapAnimationFrozenRotFrameCount
                                            [ mapI ];
                                        existing->heldFrozenRotFrameCountUsed =
                                            false;
                                        
                                        if( mMapLastAnimFade[ mapI ] == 0 ) {
                                            existing->lastHeldAnim = 
                                                mMapCurAnimType[ mapI ];
                                            existing->lastHeldAnimFade = 1;
                                            existing->curHeldAnim = held;
                                            
                                            existing->
                                                lastHeldAnimationFrameCount =
                                                mMapAnimationFrameCount[ mapI ];
                                        
                                        existing->heldAnimationFrameCount =
                                            mMapAnimationFrameCount[ mapI ];
                                            }
                                        else {
                                            // map spot is in the middle of
                                            // an animation fade
                                            existing->lastHeldAnim = 
                                                mMapLastAnimType[ mapI ];
                                            existing->
                                                lastHeldAnimationFrameCount =
                                                mMapAnimationLastFrameCount[ 
                                                    mapI ];
                                        
                                            existing->curHeldAnim = 
                                                mMapCurAnimType[ mapI ];
                                            
                                            existing->heldAnimationFrameCount =
                                                mMapAnimationFrameCount[ mapI ];
                                            

                                            existing->lastHeldAnimFade =
                                                mMapLastAnimFade[ mapI ];
                                            
                                            existing->futureHeldAnimStack->
                                                push_back( held );
                                            }
                                        
                                        }
                                    }
                                }
                            else {
                                existing->heldObjectPos = existing->currentPos;
                                existing->heldObjectRot = 0;

                                if( existing->holdingID > 0 ) {
                                    // what player is holding changed
                                    // but they didn't pick anything up
                                    // play change sound
                                    
                                    ObjectRecord *heldObj =
                                        getObject( existing->holdingID );
                                    

                                    if( oldHeld > 0 &&
                                        oldHeld != existing->holdingID &&
                                        heldTransitionSourceID == -1 ) {
                                        // held object auto-decayed from 
                                        // some other object
                                
                                        // play decay sound
                                        ObjectRecord *obj = 
                                            getObject( oldHeld );
                                        if( obj->decaySound.numSubSounds 
                                            > 0 ) {    
                                            
                                            playSound( 
                                                obj->decaySound,
                                                getVectorFromCamera(
                                                    existing->currentPos.x, 
                                                    existing->currentPos.y ) );
                                            otherSoundPlayed = true;
                                            }
                                        }
                                    else if( oldHeld > 0 &&
                                             heldTransitionSourceID > 0 ) {
                                        
                                        TransRecord *t = 
                                            getTrans( oldHeld,
                                                      heldTransitionSourceID );
                                        
                                        if( t == NULL &&
                                            oldHeld == 
                                            heldTransitionSourceID ) {
                                            // see if use-on-bare-ground
                                            // transition exists
                                            t = getTrans( oldHeld, -1 );
                                            }

                                        if( t != NULL &&
                                            t->target != t->newTarget &&
                                            t->newTarget > 0 ) {
                                            
                                            // something produced on
                                            // ground by this transition
                                            
                                            // does it make a sound on
                                            // creation?
                                            
                                            if( getObject( t->newTarget )->
                                                creationSound.numSubSounds 
                                                > 0 ) {
                                                
                                                int sourceID = t->target;
                                                if( sourceID == -1 ) {
                                                    // use on bare ground
                                                    sourceID = oldHeld;
                                                    }

                                                if( shouldCreationSoundPlay(
                                                        sourceID,
                                                        t->newTarget ) ) {
                                                    
                                                    // only make one sound
                                                    otherSoundPlayed = true;
                                                    }
                                                }
                                            }
                                        }
                                    
                                    
                                    if( oldHeld != existing->holdingID &&
                                        ( ! otherSoundPlayed ||
                                          heldObj->creationSoundForce )
                                          && 
                                        ! clothingChanged &&
                                        heldTransitionSourceID >= 0 &&
                                        heldObj->creationSound.numSubSounds 
                                        > 0 ) {
                                        
                                        int testAncestor = oldHeld;
                                        
                                        if( oldHeld <= 0 &&
                                            heldTransitionSourceID > 0 ) {
                                            
                                            testAncestor = 
                                                heldTransitionSourceID;
                                            }
                                        
                                        if( heldTransitionSourceID > 0 ) {
                                            TransRecord *groundTrans =
                                                getTrans( 
                                                    oldHeld, 
                                                    heldTransitionSourceID );
                                            if( groundTrans != NULL &&
                                                groundTrans->newTarget > 0 &&
                                                groundTrans->newActor ==
                                                existing->holdingID ) {
                                                if( shouldCreationSoundPlay(
                                                    groundTrans->target,
                                                    groundTrans->newTarget ) ) {
                                                    groundSoundPlayed = true;
                                                    }
                                                }
                                            }
                                        

                                        if( ( ! groundSoundPlayed ||
                                              heldObj->creationSoundForce )
                                            &&
                                            testAncestor > 0 ) {
                                            // new held object is result
                                            // of a transtion
                                            // (otherwise, it has been
                                            //  removed from a container

                                            // only play creation sound
                                            // if this object is truly new
                                            // (if object is flag for initial
                                            //  creation sounds only)
                                            // Check ancestor chains for
                                            // objects that loop back to their
                                            // initial state.
                                            

                                            // also not useDummies that have 
                                            // the same parent
                                            char sameParent = false;
                                
                                            if( testAncestor > 0 && 
                                                existing->holdingID > 0 ) {
                                                
                                                int holdID =
                                                    existing->holdingID;
                                                
                                                ObjectRecord *ancObj =
                                                    getObject( testAncestor );
                                                

                                                if( heldObj->isUseDummy &&
                                                    ancObj->isUseDummy &&
                                                    heldObj->useDummyParent ==
                                                    ancObj->useDummyParent ) {
                                                    sameParent = true;
                                                    }
                                                else if( heldObj->numUses > 1 
                                                         &&
                                                         ancObj->isUseDummy 
                                                         &&
                                                         ancObj->useDummyParent
                                                         == holdID ) {
                                                    sameParent = true;
                                                    }
                                                else if( ancObj->numUses > 1 
                                                         &&
                                                         heldObj->isUseDummy 
                                                         &&
                                                         heldObj->useDummyParent
                                                         == testAncestor ) {
                                                    sameParent = true;
                                                    }
                                                }
                                                

                                            if( ! sameParent 
                                                &&
                                                (! heldObj->
                                                 creationSoundInitialOnly
                                                 ||
                                                 ( ! isSpriteSubset( 
                                                     testAncestor, 
                                                     existing->holdingID ) )
                                                 ) ) {

                                                playSound( 
                                                heldObj->creationSound,
                                                getVectorFromCamera( 
                                                    existing->currentPos.x, 
                                                    existing->currentPos.y ) );
                                                creationSoundPlayed = true;
                                                
                                                if( existing->id != ourID &&
                                                    strstr( 
                                                        heldObj->description,
                                                        "offScreenSound" )
                                                    != NULL ) {
                                                    
                                                    addOffScreenSound(
                                                      existing->id,
                                                      existing->currentPos.x *
                                                      CELL_D, 
                                                      existing->currentPos.y *
                                                      CELL_D,
                                                      heldObj->description );
                                                    }
                                                }
                                            }
                                        }
                                    
                                    char autoDecay = false;
                                    
                                    if( oldHeld > 0 && 
                                        existing->holdingID > 0 ) {
                                        TransRecord *dR =
                                            getTrans( -1, oldHeld );
                                        
                                        if( dR != NULL &&
                                            dR->newActor == 0 &&
                                            dR->newTarget == 
                                            existing->holdingID ) {
                                            
                                            autoDecay = true;
                                            }
                                        }
                                    
                                    
                                    if( oldHeld == 0 ||
                                        heldContChanged || 
                                        ( heldTransitionSourceID == -1 &&
                                          !autoDecay && 
                                          ! creationSoundPlayed &&
                                          ! clothingSoundPlayed ) ) {
                                        // we're holding something new

                                        existing->lastHeldAnim = held;
                                        existing->
                                            lastHeldAnimationFrameCount = 0;
                                        existing->curHeldAnim = held;
                                        existing->heldAnimationFrameCount = 0;
                                        existing->lastHeldAnimFade = 0;
                                            
                                        
                                        if( ! otherSoundPlayed && 
                                            ! creationSoundPlayed &&
                                            ! groundSoundPlayed &&
                                            ! clothingSoundPlayed ) {
                                            // play generic pickup sound

                                            ObjectRecord *existingObj = 
                                                getObject( 
                                                    existing->displayID );
                                
                                            // skip this if they are dying
                                            // because they may have picked
                                            // up a wound

                                            // also not if they are holding a 
                                            // sickness
                                            
                                            if( ! existing->dying &&
                                                ! isSick( existing ) &&
                                                existingObj->
                                                usingSound.numSubSounds > 0 ) {
                                    
                                                playSound( 
                                                    existingObj->usingSound,
                                                    getVectorFromCamera(
                                                     existing->currentPos.x, 
                                                     existing->currentPos.y ) );
                                                otherSoundPlayed = true;
                                                }
                                            }
                                        }
                                    }
                                }
                            
                            // otherwise, don't touch frame count
                        

                            if( existing->holdingID < 0 ) {
                                // picked up a baby
                                int babyID = - existing->holdingID;
                                
                                // save ALL of these for later, after
                                // we've processed all PU lines, 
                                // because a held baby may not exist
                                // locally yet.
                                unusedHolderID.push_back( existing->id );
                                unusedHeldID.push_back( babyID );
                                }
                            }
                        

                        
                        if( existing->holdingID >= 0 &&
                            oldHeld >= 0 &&
                            oldHeld != existing->holdingID &&
                            ! heldOriginValid &&
                            heldTransitionSourceID > 0 &&
                            ! creationSoundPlayed &&
                            ! clothingSoundPlayed &&
                            ! groundSoundPlayed &&
                            ! otherSoundPlayed ) {
                            
                            
                            // what we're holding changed
                            // but no sound has been played for it yet

                            // check for special case where target
                            // of transition didn't change
                            // or is a use dummy
                            
                            TransRecord *tr = 
                                getTrans( oldHeld, 
                                          heldTransitionSourceID );
                            
                            if( tr != NULL &&
                                ( tr->newActor == existing->holdingID
                                  ||
                                  bothSameUseParent( tr->newActor,
                                                     existing->holdingID ) )
                                &&
                                tr->target == heldTransitionSourceID &&
                                ( tr->newTarget == tr->target 
                                  ||
                                  ( getObject( tr->target ) != NULL
                                    && 
                                    ( getObject( tr->target )->isUseDummy 
                                      ||
                                      getObject( tr->target )->numUses > 1 )
                                    ) 
                                  ||
                                  ( getObject( tr->newTarget ) != NULL
                                    && 
                                    ( getObject( tr->newTarget )->isUseDummy
                                      ||
                                      getObject( tr->newTarget )->numUses > 1 )
                                    )
                                  ) ) {
                                
                                // what about "using" sound
                                // of the target of our transition?
                                
                                char creationWillPlay = false;
                                
                                if( tr->newTarget > 0 && tr->target > 0 &&
                                    shouldCreationSoundPlay( tr->target,
                                                             tr->newTarget ) ) {
                                    creationWillPlay = true;
                                    }
                                

                                if( !creationWillPlay ) {
                                    
                                    ObjectRecord *targetObject = 
                                        getObject( heldTransitionSourceID );
                                    
                                    if( targetObject->usingSound.numSubSounds 
                                        > 0 ) {
                                        
                                        playSound( 
                                            targetObject->usingSound,
                                            getVectorFromCamera(
                                                existing->currentPos.x, 
                                                existing->currentPos.y ) );
                                        }
                                    }
                                }
                            }
                        
                        

                        
                        existing->displayID = o.displayID;
                        existing->age = o.age;
                        existing->lastAgeSetTime = o.lastAgeSetTime;
                        
                        existing->heat = o.heat;

                        if( existing->containedIDs != NULL ) {
                            delete [] existing->containedIDs;
                            }
                        if( existing->subContainedIDs != NULL ) {
                            delete [] existing->subContainedIDs;
                            }

                        existing->containedIDs = o.containedIDs;
                        existing->numContained = o.numContained;
                        existing->subContainedIDs = o.subContainedIDs;
                        
                        existing->xServer = o.xServer;
                        existing->yServer = o.yServer;
                        
                        
                        if( existing->heldByAdultID == -1 ) {
                            
                            updatePersonHomeLocation( 
                                existing->id,
                                lrint( existing->currentPos.x ),
                                lrint( existing->currentPos.y ) );
                            }
                        if( existing->holdingID < 0 ) {
                            int babyID = - existing->holdingID;
                            
                            updatePersonHomeLocation( 
                                babyID,
                                lrint( existing->currentPos.x ),
                                lrint( existing->currentPos.y ) );
                            }
                        

                        existing->lastSpeed = o.lastSpeed;
                        
                        char babyDropped = false;
                        
                        if( done_moving > 0 && existing->heldByAdultID != -1 ) {
                            babyDropped = true;
                            }
                        
                        if( babyDropped ) {
                            // got an update for a player that's being held
                            // this means they've been dropped
                            printf( "Baby dropped\n" );
                            
                            existing->currentPos.x = o.xd;
                            existing->currentPos.y = o.yd;
                            
                            existing->currentSpeed = 0;
                            existing->currentGridSpeed = 0;
                            playPendingReceivedMessages( existing );
                            
                            existing->xd = o.xd;
                            existing->yd = o.yd;
                            existing->destTruncated = false;

                            // clear an existing path, since they may no
                            // longer be on it
                            if( existing->pathToDest != NULL ) {
                                delete [] existing->pathToDest;
                                existing->pathToDest = NULL;
                                }

                            if( existing->lastHeldByRawPosSet ) {    
                                existing->heldByDropOffset =
                                    sub( existing->lastHeldByRawPos,
                                         existing->currentPos );
                                if( length( existing->heldByDropOffset ) > 3 ) {
                                    // too far to fly during drop
                                    // snap instead
                                    existing->heldByDropOffset.x = 0;
                                    existing->heldByDropOffset.y = 0;
                                    }
                                }
                            else {
                                // held pos not known
                                // maybe this holding parent has never
                                // been drawn on the screen
                                // (can happen if they drop us during map load)
                                existing->heldByDropOffset.x = 0;
                                existing->heldByDropOffset.y = 0;
                                }
                            
                            existing->lastHeldByRawPosSet = false;
                            
                            
                            LiveObject *adultO = 
                                getGameObject( existing->heldByAdultID );
                            
                            if( adultO != NULL ) {
                                // fade from held animation
                                existing->lastAnim = 
                                    adultO->curHeldAnim;
                                        
                                existing->animationFrameCount =
                                    adultO->heldAnimationFrameCount;

                                existing->lastAnimFade = 1;
                                existing->curAnim = ground;
                                
                                adultO->holdingID = 0;
                                }

                            existing->heldByAdultID = -1;
                            }
                        else if( done_moving > 0 && forced ) {
                            
                            // don't ever force-update these for
                            // our locally-controlled object
                            // give illusion of it being totally responsive
                            // to move commands

                            // nor do we force-update remote players
                            // don't want glitches at the end of their moves

                            // UNLESS server tells us to force update
                            existing->currentPos.x = o.xd;
                            existing->currentPos.y = o.yd;
                        
                            existing->currentSpeed = 0;
                            existing->currentGridSpeed = 0;

                            // clear an existing path, since they may no
                            // longer be on it
                            if( existing->pathToDest != NULL ) {
                                delete [] existing->pathToDest;
                                existing->pathToDest = NULL;
                                }

                            if( ! existing->somePendingMessageIsMoreMovement ) {
                                addNewAnim( existing, ground );
                                }
                            playPendingReceivedMessages( existing );

                            existing->xd = o.xd;
                            existing->yd = o.yd;
                            existing->destTruncated = false;
                            }
                        
                        if( existing->id == ourID ) {
                            // update for us
                            
                            if( !existing->inMotion ) {
                                // this is an update post-action, not post-move
                                
                                // ready to execute next action
                                playerActionPending = false;
                                waitingForPong = false;
                                playerActionTargetNotAdjacent = false;

                                existing->pendingAction = false;
                                }

                            if( forced ) {
                                existing->pendingActionAnimationProgress = 0;
                                existing->pendingAction = false;
                                
                                playerActionPending = false;
                                waitingForPong = false;
                                playerActionTargetNotAdjacent = false;

                                if( nextActionMessageToSend != NULL ) {
                                    delete [] nextActionMessageToSend;
                                    nextActionMessageToSend = NULL;
                                    }

                                // immediately send ack message
                                char *forceMessage = 
                                    autoSprintf( "FORCE %d %d#",
                                                 existing->xd,
                                                 existing->yd );
                                sendToServerSocket( forceMessage );
                                delete [] forceMessage;
                                }
                            }
                        
                        // in motion until update received, now done
                        if( existing->id != ourID ) {
                            existing->inMotion = false;
                            }
                        else {
                            // only do this if our last requested move
                            // really ended server-side
                            if( done_moving == 
                                existing->lastMoveSequenceNumber ) {
                                
                                existing->inMotion = false;
                                }
                            
                            }
                        
                        
                        existing->moveTotalTime = 0;

                        for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                            
                            int oldNumCont = 
                                existing->clothingContained[c].size();

                            existing->clothingContained[c].deleteAll();

                            int newNumClothingCont = 
                                o.clothingContained[c].size();
                            int *newClothingCont = 
                                o.clothingContained[c].getElementArray();
                        
                            existing->clothingContained[c].appendArray(
                                newClothingCont, newNumClothingCont );
                            delete [] newClothingCont;
                            
                            if( ! clothingSoundPlayed && 
                                newNumClothingCont > oldNumCont ) {
                                
                                // insertion sound
                                
                                char soundPlayed = false;
                                
                                SoundUsage contSound = 
                                    clothingByIndex( 
                                        existing->clothing, c )->usingSound;
                            
                                if( contSound.numSubSounds > 0 ) {
                                    playSound( contSound, 
                                               getVectorFromCamera( 
                                                   existing->xd,
                                                   existing->yd ) );
                                    soundPlayed = true;
                                    }

                                if( ! soundPlayed ) {
                                    // no container using sound defined
                                    // play player's using sound
                            
                                    SoundUsage s = getObject( 
                                        existing->displayID )->
                                        usingSound;

                                    if( s.numSubSounds > 0 ) {
                                        playSound( 
                                            s, 
                                            getVectorFromCamera(
                                                existing->xd,
                                                existing->yd ) );
                                        }
                                    }
                                }
                            }
                        }
                    else {    
                        o.displayChar = lastCharUsed + 1;
                    
                        lastCharUsed = o.displayChar;
                        
                        o.lastHoldingID = o.holdingID;
                        
                        if( o.holdingID < 0 ) {
                            // picked up a baby
                            int babyID = - o.holdingID;
                            
                            // save ALL of these for later, after
                            // we've processed all PU lines, 
                            // because a held baby may not exist
                            // locally yet.
                            unusedHolderID.push_back( o.id );
                            unusedHeldID.push_back( babyID );
                            }

                        o.curAnim = ground;
                        o.lastAnim = ground;
                        o.lastAnimFade = 0;

                        o.curHeldAnim = held;
                        o.lastHeldAnim = held;
                        o.lastHeldAnimFade = 0;
                        
                        o.hide = false;
                        
                        o.inMotion = false;
                        o.lastMoveSequenceNumber = 1;
                        
                        o.holdingFlip = false;
                        
                        o.lastFlipSendTime = game_getCurrentTime();
                        o.lastFlipSent = false;
                        

                        o.lastHeldByRawPosSet = false;

                        o.pendingAction = false;
                        o.pendingActionAnimationProgress = 0;
                        
                        o.currentPos.x = o.xd;
                        o.currentPos.y = o.yd;

                        o.destTruncated = false;

                        o.heldPosOverride = false;
                        o.heldPosOverrideAlmostOver = false;
                        o.heldObjectPos = o.currentPos;
                        o.heldObjectRot = 0;

                        o.currentSpeed = 0;
                        o.numFramesOnCurrentStep = 0;
                        o.currentGridSpeed = 0;
                        
                        o.moveTotalTime = 0;
                        
                        o.futureAnimStack = 
                            new SimpleVector<AnimType>();
                        o.futureHeldAnimStack = 
                            new SimpleVector<AnimType>();
                        
                        o.foodDrainTime = -1;
                        o.indoorBonusTime = 0;

                        
                        int unmarkedIndex =
                            ourUnmarkedOffspring.getElementIndex( o.id );
                        if( unmarkedIndex != -1 ) {
                            // this new baby was pointed to as our offspring
                            // before, as they were born
                            ourUnmarkedOffspring.deleteElement( unmarkedIndex );
                            
                            o.isGeneticFamily = true;
                            }
                        

                        
                        ObjectRecord *obj = getObject( o.displayID );
                        
                        if( obj->creationSound.numSubSounds > 0 ) {
                                
                            playSound( obj->creationSound,
                                       getVectorFromCamera( 
                                           o.currentPos.x,
                                           o.currentPos.y ) );
                            }
                        
                        // insert in age order, youngest last
                        double newAge = computeCurrentAge( &o );
                        char inserted = false;
                        for( int e=0; e<gameObjects.size(); e++ ) {
                            if( computeCurrentAge( gameObjects.getElement( e ) )
                                < newAge ) {
                                // found first younger, insert in front of it
                                gameObjects.push_middle( o, e );
                                recentInsertedGameObjectIndex = e;
                                inserted = true;
                                break;
                                }
                            }
                        if( ! inserted ) {
                            // they're all older than us
                            gameObjects.push_back( o );
                            recentInsertedGameObjectIndex = 
                                gameObjects.size() - 1;
                            }
                        }
                    }
                else if( o.id == ourID && 
                         strstr( lines[i], "X X" ) != NULL  ) {
                    // we died

                    printf( "Got X X death message for our ID %d\n",
                            ourID );

                    // get age after X X
                    char *xxPos = strstr( lines[i], "X X" );
                    
                    LiveObject *ourLiveObject = getOurLiveObject();
                    
                    if( xxPos != NULL ) {
                        sscanf( xxPos, "X X %lf", &( ourLiveObject->age ) );
                        }
                    ourLiveObject->finalAgeSet = true;
                    

                    if( mDeathReason != NULL ) {
                        delete [] mDeathReason;
                        }
                    char *reasonPos = strstr( lines[i], "reason" );
                    
                    if( reasonPos == NULL ) {
                        mDeathReason = stringDuplicate( 
                            translate( "reasonUnknown" ) );
                        }
                    else {
                        char reasonString[100];
                        reasonString[0] = '\0';
                        
                        sscanf( reasonPos, "reason_%99s", reasonString );
                        
                        if( apocalypseInProgress ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonApocalypse" ) );
                            }
                        else if( strcmp( reasonString, "nursing_hunger" ) == 
                                 0 ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonNursingHunger" ) );
                            }
                        else if( strcmp( reasonString, "hunger" ) == 0 ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonHunger" ) );
                            }
                        else if( strcmp( reasonString, "SID" ) == 0 ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonSID" ) );
                            }
                        else if( strcmp( reasonString, "age" ) == 0 ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonOldAge" ) );

                            if( mTutorialNumber == 2 ) {
                                // old age ends tutorial 2
                                SettingsManager::setSetting( 
                                    "tutorialDone", mTutorialNumber );
                                }
                            }
                        else if( strcmp( reasonString, "disconnected" ) == 0 ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonDisconnected" ) );
                            }
                        else if( strstr( reasonString, "killed" ) != NULL ) {
                            
                            int weaponID = 0;
                            
                            sscanf( reasonString, "killed_%d", &weaponID );
                            
                            ObjectRecord *weaponO = NULL;
                            
                            if( weaponID > 0 ) {
                                weaponO = getObject( weaponID );
                                }
                            

                            if( weaponO == NULL ) {
                                mDeathReason = stringDuplicate( 
                                    translate( "reasonKilledUnknown" ) );
                                }
                            else {

                                char *stringUpper = stringToUpperCase( 
                                    weaponO->description );

                                stripDescriptionComment( stringUpper );


                                mDeathReason = autoSprintf( 
                                    "%s%s",
                                    translate( "reasonKilled" ),
                                    stringUpper );
                                
                                delete [] stringUpper;
                                }
                            }
                        else if( strstr( reasonString, "succumbed" ) != NULL ) {
                            
                            int sicknessID = 0;
                            
                            sscanf( reasonString, "succumbed_%d", &sicknessID );
                            
                            ObjectRecord *sicknessO = NULL;
                            
                            if( sicknessID > 0 ) {
                                sicknessO = getObject( sicknessID );
                                }
                            

                            if( sicknessO == NULL ) {
                                mDeathReason = stringDuplicate( 
                                    translate( "reasonSuccumbedUnknown" ) );
                                }
                            else {

                                char *stringUpper = stringToUpperCase( 
                                    sicknessO->description );

                                stripDescriptionComment( stringUpper );


                                mDeathReason = autoSprintf( 
                                    "%s%s",
                                    translate( "reasonSuccumbed" ),
                                    stringUpper );
                                
                                delete [] stringUpper;
                                }
                            }
                        else {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonUnknown" ) );
                            }
                        }
                    
                    if( ! anyLiveTriggersLeft() ) {
                        // if we're in live trigger mode, leave 
                        // server connection open so we can see what happens
                        // next
                        closeSocket( mServerSocket );
                        mServerSocket = -1;
                        handleOurDeath();
                        }
                    else {
                        // just hide our object, but leave it around
                        getOurLiveObject()->hide = true;
                        }
                    }
                else if( strstr( lines[i], "X X" ) != NULL  ) {
                    // object deleted
                        
                    numRead = sscanf( lines[i], "%d %d",
                                      &( o.id ),
                                      &( o.holdingID ) );
                    
                    
                    for( int i=0; i<gameObjects.size(); i++ ) {
                        
                        LiveObject *nextObject =
                            gameObjects.getElement( i );
                        
                        if( nextObject->id == o.id ) {
                            
                            if( nextObject->heldByAdultID > 0 ) {
                                // baby died while held, tell
                                // parent to drop them
                                LiveObject *parent = getGameObject(
                                    nextObject->heldByAdultID );
                                
                                if( parent != NULL &&
                                    parent->holdingID == - o.id ) {
                                    parent->holdingID = 0;
                                    }
                                }
                            
                            // drop any pending messages about them
                            // we don't want these to play back later
                            for( int j=0; j<gameObjects.size(); j++ ) {
                                LiveObject *otherObject = 
                                    gameObjects.getElement( j );
                                
                                dropPendingReceivedMessagesRegardingID(
                                    otherObject, o.id );
                                }
                            
                            // play any pending messages that are
                            // waiting for them to finish their move
                            playPendingReceivedMessagesRegardingOthers( 
                                nextObject );

                            if( nextObject->containedIDs != NULL ) {
                                delete [] nextObject->containedIDs;
                                }
                            if( nextObject->subContainedIDs != NULL ) {
                                delete [] nextObject->subContainedIDs;
                                }
                            
                            if( nextObject->pathToDest != NULL ) {
                                delete [] nextObject->pathToDest;
                                }
                            
                            if( nextObject->currentSpeech != NULL ) {
                                delete [] nextObject->currentSpeech;
                                }
                            
                            if( nextObject->relationName != NULL ) {
                                delete [] nextObject->relationName;
                                }

                            if( nextObject->curseName != NULL ) {
                                delete [] nextObject->curseName;
                                }
                            
                            if( nextObject->name != NULL ) {
                                delete [] nextObject->name;
                                }

                            if( nextObject->leadershipNameTag != NULL ) {
                                delete [] nextObject->leadershipNameTag;
                                }

                            delete nextObject->futureAnimStack;
                            delete nextObject->futureHeldAnimStack;

                            gameObjects.deleteElement( i );

                            updateLeadership();
                            break;
                            }
                        }
                    }
                
                delete [] holdingIDBuffer;
                delete [] clothingBuffer;
                
                delete [] lines[i];
                }
            
            for( int i=0; i<unusedHolderID.size(); i++ ) {
                LiveObject *existing = 
                    getGameObject( unusedHolderID.getElementDirect( i ) );
                
                LiveObject *babyO = 
                    getGameObject( unusedHeldID.getElementDirect( i ) );
                
                if( babyO != NULL && existing != NULL ) {
                    babyO->heldByAdultID = existing->id;
                    
                    if( babyO->heldByAdultPendingID == existing->id ) {
                        // pending held finally happened
                        babyO->heldByAdultPendingID = -1;
                        }
                    
                    // stop crying when held
                    babyO->tempAgeOverrideSet = false;
                    
                    if( babyO->pathToDest != NULL ) {
                        // forget baby's old path
                        // they are going to be set down elsewhere
                        // far away from path
                        delete [] babyO->pathToDest;
                        babyO->pathToDest = NULL;
                        }
                    babyO->currentSpeed = 0;
                    babyO->inMotion = false;
                    
                    if( babyO->id == ourID ) {
                        if( nextActionMessageToSend != NULL ) {
                            // forget pending action, we've been interrupted
                            delete [] nextActionMessageToSend;
                            nextActionMessageToSend = NULL;
                            }
                        playerActionPending = false;
                        waitingForPong = false;
                        }
                    

                    existing->heldFrozenRotFrameCount =
                        babyO->frozenRotFrameCount;
                                    
                    existing->heldFrozenRotFrameCountUsed =
                        false;
                                    

                    if( babyO->lastAnimFade == 0 ) {
                        
                        existing->lastHeldAnim = 
                            babyO->curAnim;
                        
                        existing->heldAnimationFrameCount =
                            babyO->animationFrameCount;
                        
                        existing->lastHeldAnimationFrameCount =
                            babyO->lastAnimationFrameCount;

                        existing->lastHeldAnimFade = 1;
                        existing->curHeldAnim = held;
                        }
                    else {
                        // baby that we're picking up
                        // in the middle of an existing fade
                        
                        existing->lastHeldAnim = 
                            babyO->lastAnim;
                        existing->lastHeldAnimationFrameCount =
                            babyO->lastAnimationFrameCount;
                        
                        existing->curHeldAnim = 
                            babyO->curAnim;
                        
                        existing->heldAnimationFrameCount =
                            babyO->animationFrameCount;
                        
                        
                        existing->lastHeldAnimFade =
                            babyO->lastAnimFade;
                        
                        existing->futureHeldAnimStack->
                            push_back( held );
                        }
                    }
                if( existing != NULL && babyO == NULL ) {
                    // existing is holding a baby that does not exist
                    // this can happen when playing back pending PU messages
                    // when we're holding a baby that dies when we're walking
                    
                    // holding nothing now instead
                    existing->holdingID = 0;
                    }
                
                }
            


            delete [] lines;


            if( ( mFirstServerMessagesReceived & 2 ) == 0 ) {
            
                LiveObject *ourObject = 
                    gameObjects.getElement( recentInsertedGameObjectIndex );
                
                ourID = ourObject->id;

                if( ourID != lastPlayerID ) {
                    // different ID than last time, delete old home markers
                    oldHomePosStack.deleteAll();
                    }
                homePosStack.push_back_other( &oldHomePosStack );

                lastPlayerID = ourID;

                // we have no measurement yet
                ourObject->lastActionSendStartTime = 0;
                ourObject->lastResponseTimeDelta = 0;
                

                remapRandSource.reseed( ourID );

                printf( "Got first PLAYER_UPDATE message, our ID = %d\n",
                        ourID );

                ourObject->displayChar = 'A';
                }
            
            mFirstServerMessagesReceived |= 2;
            }
        else if( type == PLAYER_MOVES_START ) {
            
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip fist
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                LiveObject o;

                double etaSec;
                
                int startX, startY;
                
                int truncated;
                
                int numRead = sscanf( lines[i], "%d %d %d %lf %lf %d",
                                      &( o.id ),
                                      &( startX ),
                                      &( startY ),
                                      &( o.moveTotalTime ),
                                      &etaSec,
                                      &truncated );

                SimpleVector<char *> *tokens =
                    tokenizeString( lines[i] );
                

                applyReceiveOffset( &startX, &startY );
                

                o.pathLength = 0;
                o.pathToDest = NULL;
                
                // require an even number at least 8
                if( tokens->size() < 8 || tokens->size() % 2 != 0 ) {
                    }
                else {                    
                    int numTokens = tokens->size();
        
                    o.pathLength = (numTokens - 6) / 2 + 1;
        
                    o.pathToDest = new GridPos[ o.pathLength ];

                    o.pathToDest[0].x = startX;
                    o.pathToDest[0].y = startY;

                    for( int e=1; e<o.pathLength; e++ ) {
            
                        char *xToken = 
                            tokens->getElementDirect( 6 + (e-1) * 2 );
                        char *yToken = 
                            tokens->getElementDirect( 6 + (e-1) * 2 + 1 );
                        
        
                        sscanf( xToken, "%d", &( o.pathToDest[e].x ) );
                        sscanf( yToken, "%d", &( o.pathToDest[e].y ) );
                        
                        // make them absolute
                        o.pathToDest[e].x += startX;
                        o.pathToDest[e].y += startY;
                        }
        
                    }

                tokens->deallocateStringElements();
                delete tokens;
                    
                
                
                
                o.moveEtaTime = etaSec + game_getCurrentTime();
                

                if( numRead == 6 && o.pathLength > 0 ) {
                
                    o.xd = o.pathToDest[ o.pathLength -1 ].x;
                    o.yd = o.pathToDest[ o.pathLength -1 ].y;
                
    
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == o.id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            if( existing->
                                pendingReceivedMessages.size() > 0 ) {
                                
                                // we've got older messages pending
                                // make this pending too
                                
                                printf( 
                                    "Holding PM message for %d "
                                    "until later, "
                                    "%d other messages pending for them\n",
                                    existing->id,
                                    existing->pendingReceivedMessages.size() );

                                existing->pendingReceivedMessages.push_back(
                                    autoSprintf( "PM\n%s\n#",
                                                 lines[i] ) );
                                existing->somePendingMessageIsMoreMovement =
                                    true;
                                
                                break;
                                }
                            
                            // actually playing the PM message
                            // that means nothing else is pending yet
                            existing->somePendingMessageIsMoreMovement = false;
                                
                            // receiving a PM means they aren't out of
                            // range anymore
                            existing->outOfRange = false;
                            
                            
                            if( existing->heldByAdultID == -1 ) {
                                
                                updatePersonHomeLocation( 
                                    existing->id,
                                    lrint( existing->currentPos.x ),
                                    lrint( existing->currentPos.y ) );
                                }
                            if( existing->holdingID < 0 ) {
                                int babyID = - existing->holdingID;
                                
                                updatePersonHomeLocation( 
                                    babyID,
                                    lrint( existing->currentPos.x ),
                                    lrint( existing->currentPos.y ) );
                                }


                            double timePassed = 
                                o.moveTotalTime - etaSec;
                            
                            double fractionPassed = 
                                timePassed / o.moveTotalTime;
                            
                            if( existing->id != ourID ) {
                                // stays in motion until we receive final
                                // PLAYER_UPDATE from server telling us
                                // that move is over
                                
                                // don't do this for our local object
                                // we track our local inMotion status
                                existing->inMotion = true;
                                }
                            
                            int oldPathLength = 0;
                            GridPos oldCurrentPathPos;
                            int oldCurrentPathIndex = -1;
                            SimpleVector<GridPos> oldPath;
                            
                            if( existing->currentSpeed != 0
                                &&
                                existing->pathToDest != NULL ) {
                                
                                // an interrupted move
                                oldPathLength = existing->pathLength;
                                oldCurrentPathPos = 
                                    existing->pathToDest[
                                        existing->currentPathStep ];
                                
                                oldPath.appendArray( existing->pathToDest,
                                                     existing->pathLength );
                                oldCurrentPathIndex = existing->currentPathStep;
                                }
                            

                            if( existing->id != ourID ) {
                                // remove any double-backs from path
                                // because they confuse smooth path following
                                
                                removeDoubleBacksFromPath( &( o.pathToDest ),
                                                           &( o.pathLength ) );
                                
                                }
                            



                            if( existing->id != ourID ||
                                truncated ) {
                                // always replace path for other players
                                // with latest from server

                                // only replace OUR path if we
                                // learn that a path we submitted
                                // was truncated

                                existing->pathLength = o.pathLength;

                                if( existing->pathToDest != NULL ) {
                                    delete [] existing->pathToDest;
                                    }
                                existing->pathToDest = 
                                    new GridPos[ o.pathLength ];
                                
                                memcpy( existing->pathToDest,
                                        o.pathToDest,
                                        sizeof( GridPos ) * o.pathLength );

                                existing->xd = o.xd;
                                existing->yd = o.yd;
                                
                                existing->destTruncated = truncated;

                                if( existing->id != ourID ) {
                                    // look at how far we think object is
                                    // from current fractional position
                                    // along path

                                    int b = 
                                        (int)floor( 
                                            fractionPassed * 
                                            ( existing->pathLength - 1 ) );
                                    
                                    if( b >= existing->pathLength ) {
                                        b = existing->pathLength - 1;
                                        }

                                    // we may be getting a move for
                                    // an object that has been off-chunk
                                    // for a while and is now moving on-chunk

                                    doublePair curr;
                                    curr.x = existing->pathToDest[ b ].x;
                                    curr.y = existing->pathToDest[ b ].y;
                                    
                                    if( distance( curr, 
                                                  existing->currentPos ) >
                                        5 ) {
                                        
                                        // 5 is too far

                                        // jump right to current loc
                                        // on new path
                                        existing->currentPos = curr;
                                        }
                                    }
                                
                                }
                            


                            if( existing->id != ourID ) {
                                // don't force-update these
                                // for our object
                                // we control it locally, to keep
                                // illusion of full move interactivity
                            
                                char usingOldPathStep = false;
                                char appendingLeadPath = false;
                                
                                if( oldPathLength != 0 ) {
                                    // this move interrupts or truncates
                                    // the move we were already on

                                    // look to see if the old path step
                                    // is on our new path
                                    char found = false;
                                    int foundStep = -1;

                                    for( int p=0; 
                                         p<existing->pathLength - 1;
                                         p++ ) {
                                        
                                        if( equal( existing->pathToDest[p],
                                                   oldCurrentPathPos ) ) {
                                            
                                            found = true;
                                            foundStep = p;
                                            }
                                        }
                                    
                                    if( found ) {
                                        usingOldPathStep = true;
                                        
                                        existing->currentPathStep =
                                            foundStep;
                                        
                                        doublePair foundWorld = 
                                            gridToDouble(
                                                existing->
                                                pathToDest[ foundStep ] );

                                        // where we should move toward
                                        doublePair nextWorld;
                                        
                                        if( foundStep < 
                                            existing->pathLength - 1 ) {
                                            
                                            // point from here to our
                                            // next step
                                            nextWorld = 
                                                gridToDouble(
                                                    existing->
                                                    pathToDest[ 
                                                        foundStep + 1 ] );    
                                            }
                                        else {
                                            // at end of path, point right
                                            // to it
                                            nextWorld = foundWorld;
                                            }

                                        existing->currentMoveDirection =
                                            normalize( 
                                                sub( nextWorld, 
                                                     existing->currentPos ) );
                                        }
                                    else {
                                        // other case
                                        // check if new start on old path

                                        // maybe this new path branches
                                        // off old path before or after 
                                        // where we are
                                        
                                        printf( "    CUR PATH:  " );
                                        GridPos *oldPathArray = 
                                            oldPath.getElementArray();
                                        printPath( oldPathArray,
                                                   oldPathLength );
                                        delete [] oldPathArray;
                                        printf( "    WE AT:  %d (%d,%d)  \n",
                                                oldCurrentPathIndex,
                                                oldCurrentPathPos.x,
                                                oldCurrentPathPos.y );

                                        int foundStartIndex = -1;
                                        
                                        for( int i=0; i<oldPathLength; i++ ) {
                                            GridPos p = 
                                                oldPath.getElementDirect( i );
                                            
                                            if( p.x == startX && 
                                                p.y == startY ) {
                                                
                                                foundStartIndex = i;
                                                break;
                                                }
                                            }
                                        
                                        if( foundStartIndex != -1 ) {
                                            
                                            int step = 1;
                                            
                                            if( foundStartIndex > 
                                                oldCurrentPathIndex ) {
                                                step = 1;
                                                }
                                            else if( foundStartIndex < 
                                                oldCurrentPathIndex ) {
                                                step = -1;
                                                }
                                            appendingLeadPath = true;
                                            
                                            SimpleVector<GridPos> newPath;
                                            
                                            for( int i=oldCurrentPathIndex;
                                                 i != foundStartIndex;
                                                 i += step ) {
                                                
                                                newPath.push_back(
                                                    oldPath.
                                                    getElementDirect( i ) );
                                                }
                                            
                                            for( int i=0; 
                                                 i<existing->pathLength;
                                                 i++ ) {
                                                // now add rest of new path
                                                newPath.push_back(
                                                    existing->pathToDest[i] );
                                                }
                                            
                                            printf( "    OLD PATH:  " );
                                            printPath( existing->pathToDest,
                                                       existing->pathLength );


                                            // now replace path
                                            // with new, lead-appended path
                                            existing->pathLength = 
                                                newPath.size();
                                            
                                            delete [] existing->pathToDest;
                                            
                                            existing->pathToDest =
                                                newPath.getElementArray();
                                            existing->currentPathStep = 0;
                                            existing->numFramesOnCurrentStep 
                                                = 0;
                                            

                                            printf( "    NEW PATH:  " );
                                            printPath( existing->pathToDest,
                                                       existing->pathLength );

                                            removeDoubleBacksFromPath( 
                                                &( existing->pathToDest ),
                                                &( existing->pathLength ) );
                                
                                            printf( 
                                                "    NEW PATH (DB free):  " );
                                            printPath( existing->pathToDest,
                                                       existing->pathLength );

                                            if( existing->pathLength == 1 ) {
                                                fixSingleStepPath( existing );
                                                }
                                            
                                            
                                            doublePair nextWorld =
                                                gridToDouble( 
                                                    existing->pathToDest[1] );
                                            
                                            // point toward next path pos
                                            existing->currentMoveDirection =
                                                normalize( 
                                                    sub( nextWorld, 
                                                         existing->
                                                         currentPos ) );
                                            }    
                                        }
                                    
                                    }
                                
                                
                                if( ! usingOldPathStep && 
                                    ! appendingLeadPath ) {

                                    // we don't have enough info
                                    // to patch path
                                    
                                    // change to walking toward next
                                    // path step from wherever we are
                                    // but DON'T jump existing obj's
                                    // possition suddenly
                                    
                                    printf( "Manually forced\n" );
                                    
                                    // find closest spot along path
                                    // to our current pos
                                    double minDist = DBL_MAX;
                                    
                                    // prev step
                                    int b = -1;
                                    
                                    for( int testB=0; 
                                         testB < existing->pathLength - 1; 
                                         testB ++ ) {
                                        
                                        doublePair worldPos = gridToDouble( 
                                            existing->pathToDest[testB] );
                                        
                                        double thisDist = 
                                            distance( worldPos,
                                                      existing->currentPos );
                                        if( thisDist < minDist ) {
                                            b = testB;
                                            minDist = thisDist;
                                            }
                                        }
                                    

                                    // next step
                                    int n = b + 1;
                                    
                                    existing->currentPathStep = b;
                                    
                                    doublePair nWorld =
                                        gridToDouble( 
                                            existing->pathToDest[n] );
                                    
                                    // point toward next path pos
                                    existing->currentMoveDirection =
                                        normalize( 
                                            sub( nWorld,
                                                 existing->currentPos ) );
                                    
                                    }
                                
                                
                                existing->moveTotalTime = o.moveTotalTime;
                                existing->moveEtaTime = o.moveEtaTime;

                                if( usingOldPathStep ) {
                                    // we're ignoring where server
                                    // says we should be

                                    // BUT, we should change speed to
                                    // compensate for the difference

                                    double oldFractionPassed =
                                        measurePathLength( 
                                            existing,
                                            existing->currentPathStep + 1 )
                                        /
                                        measurePathLength( 
                                            existing, 
                                            existing->pathLength );
                                    
                                    
                                    // if this is positive, we are
                                    // farther along than we should be
                                    // we need to slow down (moveEtaTime
                                    // should get bigger)

                                    // if negative, we are behind
                                    // we should speed up (moveEtaTime
                                    // should get smaller)
                                    double fractionDiff =
                                        oldFractionPassed - fractionPassed;
                                    
                                    double timeAdjust =
                                        existing->moveTotalTime * fractionDiff;
                                    
                                    if( fractionDiff < 0 ) {
                                        // only speed up...
                                        // never slow down, because
                                        // it's always okay if we show
                                        // player arriving early

                                        existing->moveEtaTime += timeAdjust;
                                        }
                                    }
                                

                                updateMoveSpeed( existing );
                                }
                            else if( truncated ) {
                                // adjustment to our own movement
                                
                                // cancel pending action upon arrival
                                // (no longer possible, since truncated)

                                existing->pendingActionAnimationProgress = 0;
                                existing->pendingAction = false;
                                
                                playerActionPending = false;
                                waitingForPong = false;
                                playerActionTargetNotAdjacent = false;

                                if( nextActionMessageToSend != NULL ) {
                                    delete [] nextActionMessageToSend;
                                    nextActionMessageToSend = NULL;
                                    }

                                // this path may be different
                                // from what we actually requested
                                // from sever

                                char stillOnPath = false;
                                    
                                if( oldPathLength > 0 ) {
                                    // on a path, perhaps some other one
                                    
                                    // check if our current pos
                                    // is on this new, truncated path
                                    
                                    char found = false;
                                    int foundStep = -1;
                                    for( int p=0; 
                                         p<existing->pathLength - 1;
                                         p++ ) {
                                        
                                        if( equal( existing->pathToDest[p],
                                                   oldCurrentPathPos ) ) {
                                            
                                            found = true;
                                            foundStep = p;
                                            }
                                        }
                                    
                                        if( found ) {
                                            stillOnPath = true;
                                            
                                            existing->currentPathStep =
                                                foundStep;
                                            }
                                        
                                    }
                                    


                                // only jump around if we must

                                if( ! stillOnPath ) {
                                    // path truncation from what we last knew
                                    // for ourselves, and we're off the end
                                    // of the new path
                                    
                                    // hard jump back
                                    existing->currentSpeed = 0;
                                    existing->currentGridSpeed = 0;
                                    
                                    playPendingReceivedMessages( existing );

                                    existing->currentPos.x =
                                        existing->xd;
                                    existing->currentPos.y =
                                        existing->yd;
                                    }
                                }
                            
                            

                            

                            break;
                            }
                        }
                    }
                
                if( o.pathToDest != NULL ) {
                    delete [] o.pathToDest;
                    }

                delete [] lines[i];
                }
            

            delete [] lines;
            }
        else if( type == PLAYER_SAYS ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id = -1;
                int curseFlag = 0;

                int numRead = 0;
                
                if( strstr( lines[i], "/" ) != NULL ) {
                    // new id/curse_flag format
                    numRead = sscanf( lines[i], "%d/%d ", &id, &curseFlag );
                    }
                else {
                    // old straight ID format
                    numRead = sscanf( lines[i], "%d ", &id );
                    }
                
                if( numRead >= 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            char *firstSpace = strstr( lines[i], " " );
                            
                            char famSpeech = false;
                            if( firstSpace != NULL ) {
                                // check for +FAMILY+
                                // only show it if the person is NOT
                                // currently talking, but remember it
                                // (Don't interrupt speech with spurious
                                //  +FAMILY+ indicators)
                                
                                if( strcmp( &( firstSpace[1] ), 
                                            "+FAMILY+" ) == 0 ) {
                                    existing->isGeneticFamily = true;
                                    famSpeech = true;

                                    if( existing->currentSpeech != NULL ) {
                                        // we learned their family status
                                        // but don't make them say +FAMILY+
                                        // because they have a current speech
                                        // bubble
                                        firstSpace = NULL;
                                        }
                                    }
                                }
                            


                            if( firstSpace != NULL ) {
                                
                                if( existing->currentSpeech != NULL ) {
                                    delete [] existing->currentSpeech;
                                    existing->currentSpeech = NULL;
                                    }
                                
                                existing->currentSpeech = 
                                    stringDuplicate( &( firstSpace[1] ) );
                                

                                double curTime = game_getCurrentTime();
                                
                                existing->speechFade = 1.0;
                                
                                existing->speechIsSuccessfulCurse = curseFlag;

                                if( ! existing->speechIsSuccessfulCurse &&
                                    ! famSpeech &&
                                    curTime - existing->lastCurseTagDisplayTime
                                    > maxCurseTagDisplayGap &&
                                    existing->curseName != NULL ) {
                                    // last speech was NOT curse tag
                                    // and it has been too long
                                    // that means they are babbling
                                    // and hiding their own curse tag
                                    // force it into their speech
                                    
                                    char *taggedSpeech = 
                                        autoSprintf( "X %s X - %s",
                                                     existing->curseName,
                                                     existing->currentSpeech );
                                    delete [] existing->currentSpeech;
                                    existing->currentSpeech = taggedSpeech;
                                    
                                    existing->speechIsCurseTag = true;
                                    existing->lastCurseTagDisplayTime = curTime;
                                    }
                                else {
                                    existing->speechIsCurseTag = false;
                                    }

                                existing->speechIsOverheadLabel = false;


                                // longer time for longer speech
                                existing->speechFadeETATime = 
                                    curTime + 3 +
                                    strlen( existing->currentSpeech ) / 5;

                                if( existing->age < 1 && 
                                    existing->heldByAdultID == -1 ) {
                                    // make 0-y-old unheld baby revert to 
                                    // crying age every time they speak
                                    existing->tempAgeOverrideSet = true;
                                    existing->tempAgeOverride = 0;
                                    existing->tempAgeOverrideSetTime = 
                                        game_getCurrentTime();
                                    }
                                
                                if( curseFlag && mCurseSound != NULL ) {
                                    playSound( 
                                        mCurseSound,
                                        0.5, // a little loud, tweak it
                                        getVectorFromCamera( 
                                            existing->currentPos.x, 
                                            existing->currentPos.y ) );
                                    }

                                if( existing->id == ourID ) {
                                    // look for map metadata
                                    char *starPos =
                                        strstr( existing->currentSpeech,
                                                " *map" );
                                    
                                    if( starPos != NULL ) {
                                        
                                        int mapX, mapY;
                                        
                                        int mapAge = 0;
                                        
                                        int numRead = sscanf( starPos,
                                                              " *map %d %d %d",
                                                              &mapX, &mapY,
                                                              &mapAge );

                                        int mapYears = 
                                            floor( 
                                                mapAge * 
                                                getOurLiveObject()->ageRate );
                                        
                                        // trim it off
                                        starPos[0] ='\0';

                                        char person = false;
                                        char baby = false;
                                        
                                        char *babyPos = 
                                            strstr( existing->currentSpeech, 
                                                    " *baby" );
                                        
                                        int personID = -1;

                                        const char *personKey = NULL;
                                        
                                        if( babyPos != NULL ) {
                                            person = true;
                                            baby = true;
                                            
                                            sscanf( babyPos, 
                                                    " *baby %d", &personID );

                                            babyPos[0] = '\0';
                                            personKey = "baby";
                                            }


                                        if( ! person ) {
                                            char *leaderPos = 
                                                strstr( 
                                                    existing->currentSpeech, 
                                                    " *leader" );
                                            
                                            if( leaderPos != NULL ) {
                                                person = true;
                                                sscanf( leaderPos, 
                                                    " *leader %d", &personID );

                                                leaderPos[0] = '\0';
                                                personKey = "lead";
                                                }
                                            }
                                        
                                        char follower = false;
                                        
                                        if( ! person ) {
                                            char *follPos = 
                                                strstr( 
                                                    existing->currentSpeech, 
                                                    " *follower" );
                                            
                                            if( follPos != NULL ) {
                                                person = true;
                                                follower = true;
                                                sscanf( follPos, 
                                                        " *follower %d", 
                                                        &personID );

                                                follPos[0] = '\0';
                                                personKey = "supp";
                                                }
                                            }
                                        
                                        
                                        
                                        if( ! person ) {
                                            char *expertPos = 
                                                strstr( 
                                                    existing->currentSpeech, 
                                                    " *expert" );
                                            
                                            if( expertPos != NULL ) {
                                                person = true;
                                                sscanf( expertPos, 
                                                        " *expert %d", 
                                                        &personID );

                                                expertPos[0] = '\0';
                                                personKey = "expt";
                                                }
                                            }
                                        
                                        if( ! person ) {
                                            char *ownerPos = 
                                                strstr( 
                                                    existing->currentSpeech, 
                                                    " *owner" );
                                            
                                            if( ownerPos != NULL ) {
                                                person = true;
                                                sscanf( ownerPos, 
                                                        " *owner %d", 
                                                        &personID );

                                                ownerPos[0] = '\0';
                                                personKey = "owner";
                                                }
                                            }


                                        if( ! person ) {
                                            char *visitorPos = 
                                                strstr( 
                                                    existing->currentSpeech, 
                                                    " *visitor" );
                                            
                                            if( visitorPos != NULL ) {
                                                person = true;
                                                sscanf( visitorPos, 
                                                        " *visitor %d", 
                                                        &personID );

                                                visitorPos[0] = '\0';
                                                personKey = "visitor";
                                                }
                                            }
                                        

                                        if( ! person ) {
                                            char *propPos = 
                                                strstr( 
                                                    existing->currentSpeech, 
                                                    " *prop" );
                                            
                                            if( propPos != NULL ) {
                                                person = true;

                                                // prop person id is always 0
                                                personID = 0;

                                                propPos[0] = '\0';
                                                personKey = "property";
                                                }
                                            }

                                        
                                        LiveObject *personO = NULL;
                                        if( personID > 0 ) {
                                            personO = getLiveObject( personID );
                                            }
                                        

                                        if( numRead == 2 || numRead == 3 ) {
                                            addTempHomeLocation( mapX, mapY,
                                                                 person,
                                                                 personID,
                                                                 personO,
                                                                 personKey );
                                            }

                                        if( personID != -1 && baby) {
                                            LiveObject *babyO =
                                                getLiveObject( personID );
                                            if( babyO != NULL ) {
                                                babyO->isGeneticFamily = true;
                                                }
                                            else {
                                                // baby creation message
                                                // not received yet
                                                ourUnmarkedOffspring.push_back(
                                                    personID );
                                                }
                                            }
                                        
                                        doublePair dest = { (double)mapX, 
                                                            (double)mapY };
                                        double d = 
                                            distance( dest,
                                                      existing->currentPos );
                                        
                                        if( d >= 5 ) {
                                            char *dString = 
                                                getSpokenNumber( d );

                                            const char *des = "";
                                            const char *desSpace = "";
                                            
                                            if( follower ) {
                                                des = translate( 
                                                    "closestFollower" );
                                                desSpace = " ";
                                                }
                                            

                                            char *newSpeech =
                                                autoSprintf( 
                                                    "%s - %s%s%s %s",
                                                    existing->currentSpeech,
                                                    des,
                                                    desSpace,
                                                    dString,
                                                    translate( "metersAway" ) );
                                            delete [] dString;
                                            delete [] existing->currentSpeech;

                                            if( mapYears > 0 ) {
                                                const char *yearKey = 
                                                    "yearsAgo";
                                                if( mapYears == 1 ) {
                                                    yearKey = "yearAgo";
                                                    }
                                                
                                                if( mapYears >= 2000 ) {
                                                    mapYears /= 1000;
                                                    yearKey = "millenniaAgo";
                                                    }
                                                else if( mapYears >= 200 ) {
                                                    mapYears /= 100;
                                                    yearKey = "centuriesAgo";
                                                    }
                                                else if( mapYears >= 20 ) {
                                                    mapYears /= 10;
                                                    yearKey = "decadesAgo";
                                                    }

                                                char *ageString =
                                                    getSpokenNumber( mapYears );
                                                char *newSpeechB =
                                                    autoSprintf( 
                                                        "%s - %s %s %s",
                                                        newSpeech,
                                                        translate( "made" ),
                                                        ageString,
                                                        translate( yearKey ) );
                                                delete [] ageString;
                                                delete [] newSpeech;
                                                newSpeech = newSpeechB;
                                                }
                                            
                                            existing->currentSpeech =
                                                newSpeech;
                                            }
                                        }
                                    }
                                }
                            
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == LOCATION_SAYS ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            for( int i=1; i<numLines; i++ ) {
                int x = 0;
                int y = 0;
                
                int numRead = sscanf( lines[i], "%d %d", &x, &y );
                
                
                if( numRead == 2 ) {
                    
                    char *firstSpace = strstr( lines[i], " " );

                    if( firstSpace != NULL ) {
                        char *secondSpace = strstr( &( firstSpace[1] ), " " );
                        

                        if( secondSpace != NULL ) {
                            
                            char *speech = &( secondSpace[1] );
                            
                            LocationSpeech ls;
                            
                            ls.pos.x = x * CELL_D;
                            ls.pos.y = y * CELL_D;
                            
                            ls.speech = stringDuplicate( speech );
                            
                            ls.fade = 1.0;
                            
                            // longer time for longer speech
                            ls.fadeETATime = 
                                game_getCurrentTime() + 3 +
                                strlen( ls.speech ) / 5;

                            locationSpeech.push_back( ls );

                            // remove old location speech at same pos
                            for( int i=0; i<locationSpeech.size() - 1; i++ ) {
                                LocationSpeech other = 
                                    locationSpeech.getElementDirect( i );
                                if( other.pos.x == ls.pos.x &&
                                    other.pos.y == ls.pos.y ) {
                                    locationSpeech.deleteElement( i );
                                    i--;
                                    }
                                }
                            }
                        }
                    }
                
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == PLAYER_EMOT ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }            
            
            for( int i=1; i<numLines; i++ ) {
                int pid, emotIndex;
                int ttlSec = 0;
                
                int numRead = sscanf( lines[i], "%d %d %d",
                                      &pid, &emotIndex, &ttlSec );

                if( numRead >= 2 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == pid ) {
                                
                            LiveObject *existing = gameObjects.getElement(j);
                            Emotion *newEmotPlaySound = NULL;
                            
                            if( ttlSec < 0 ) {
                                // new permanent emot layer
                                newEmotPlaySound = getEmotion( emotIndex );

                                if( newEmotPlaySound != NULL ) {
                                    if( existing->permanentEmots.
                                        getElementIndex( 
                                            newEmotPlaySound ) == -1 ) {
                                    
                                        existing->permanentEmots.push_back(
                                            newEmotPlaySound );
                                        }
                                    }
                                if( ttlSec == -2 ) {
                                    // old emot that we're just learning about
                                    // skip sound
                                    newEmotPlaySound = NULL;
                                    }
                                }
                            else {
                                
                                Emotion *oldEmot = existing->currentEmot;
                            
                                existing->currentEmot = getEmotion( emotIndex );
                            
                                if( numRead == 3 && ttlSec > 0 ) {
                                    existing->emotClearETATime = 
                                        game_getCurrentTime() + ttlSec;
                                    }
                                else {
                                    // no ttl provided by server, use default
                                    existing->emotClearETATime = 
                                        game_getCurrentTime() + emotDuration;
                                    }
                                
                                if( oldEmot != existing->currentEmot &&
                                    existing->currentEmot != NULL ) {
                                    newEmotPlaySound = existing->currentEmot;
                                    }
                                }
                            
                            if( newEmotPlaySound != NULL ) {
                                doublePair playerPos = existing->currentPos;
                                
                                // play sounds for this emotion, but only
                                // if in range
                                if( !existing->outOfRange ) 
                                for( int i=0;
                                     i<getEmotionNumObjectSlots(); i++ ) {
                                    
                                    int id =
                                        getEmotionObjectByIndex(
                                            newEmotPlaySound, i );
                                    
                                    if( id > 0 ) {
                                        ObjectRecord *obj = getObject( id );
                                        
                                        if( obj->creationSound.numSubSounds 
                                            > 0 ) {    
                                    
                                            playSound( 
                                                obj->creationSound,
                                                getVectorFromCamera( 
                                                    playerPos.x,
                                                    playerPos.y ) );
                                            
                                            if( existing->id != ourID &&
                                                strstr( 
                                                    obj->description,
                                                    "offScreenSound" )
                                                != NULL ) {
                                                    
                                                addOffScreenSound(
                                                    existing->id,
                                                    playerPos.x *
                                                    CELL_D, 
                                                    playerPos.y *
                                                    CELL_D,
                                                    obj->description );
                                                }

                                            // stop after first sound played
                                            break;
                                            }
                                        }
                                    }
                                }
                            // found matching player, done
                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == LINEAGE ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->lineage.deleteAll();

                            char *firstSpace = strstr( lines[i], " " );
        
                            if( firstSpace != NULL ) {

                                char *linStart = &( firstSpace[1] );
                                
                                SimpleVector<char *> *tokens = 
                                    tokenizeString( linStart );

                                int numNormalTokens = tokens->size();
                                
                                if( tokens->size() > 0 ) {
                                    char *lastToken =
                                        tokens->getElementDirect( 
                                            tokens->size() - 1 );
                                    
                                    if( strstr( lastToken, "eve=" ) ) {   
                                        // eve tag at end
                                        numNormalTokens--;

                                        sscanf( lastToken, "eve=%d",
                                                &( existing->lineageEveID ) );

                                        if( existing->lineageEveID > 0 ) {
                                            // copy war status from someone
                                            // else in this lineage
                                            for( int i=0; i<gameObjects.size();
                                                 i++ ) {
                                                LiveObject *other =
                                                    gameObjects.getElement( i );
                                                
                                                if( other->id != existing->id &&
                                                    other->lineageEveID ==
                                                    existing->lineageEveID ) {
                                                    existing->warPeaceStatus =
                                                        other->warPeaceStatus;
                                                    break;
                                                    }
                                                }
                                            }
                                        }
                                    }

                                for( int t=0; t<numNormalTokens; t++ ) {
                                    char *tok = tokens->getElementDirect( t );
                                    
                                    int mID = 0;
                                    sscanf( tok, "%d", &mID );
                                    
                                    if( mID != 0 ) {
                                        existing->lineage.push_back( mID );
                                        }
                                    }
                                
                                if( id == ourID ) {
                                    // we just got our own lineage
                                    for( int t=0; 
                                         t < existing->lineage.size();
                                         t++ ) {
                                        LiveObject *ancestor =
                                            getLiveObject( 
                                                existing->lineage.
                                                getElementDirect( t ) );

                                        if( ancestor != NULL ) {
                                            ancestor->isGeneticFamily = true;
                                            }
                                        }
                                    }
                                else {
                                    // are we an ancestor of this person?
                                    for( int t=0; 
                                         t < existing->lineage.size();
                                         t++ ) {
                                        if( ourID == 
                                            existing->
                                            lineage.getElementDirect( t ) ) {
                                            existing->isGeneticFamily = true;
                                            break;
                                            }
                                        }
                                    }
                                
                                tokens->deallocateStringElements();
                                delete tokens;
                                }
                            
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;

            // after a LINEAGE message, we should have lineage for all
            // players
            
            // now process each one and generate relation string
            LiveObject *ourObject = getOurLiveObject();
            
            for( int j=0; j<gameObjects.size(); j++ ) {
                if( gameObjects.getElement(j)->id != ourID ) {
                            
                    LiveObject *other = gameObjects.getElement(j);
                 
                    if( other->relationName == NULL ) {
                        
                        /*
                        // test
                        ourObject->lineage.deleteAll();
                        other->lineage.deleteAll();
                        
                        int cousinNum = 25;
                        int removeNum = 5;
                        
                        for( int i=0; i<=cousinNum; i++ ) {    
                            ourObject->lineage.push_back( i );
                            }

                        for( int i=0; i<=cousinNum - removeNum; i++ ) {    
                            other->lineage.push_back( 100 + i );
                            }
                        other->lineage.push_back( cousinNum );
                        */
                        other->relationName = getRelationName( ourObject,
                                                               other );
                        }
                    }
                }
            }
        else if( type == CURSED ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id, level;
                char buffer[30];
                buffer[0] = '\0';
                
                int numRead = sscanf( lines[i], "%d %d %29s",
                                      &id, &level, buffer );

                if( numRead == 2 || numRead == 3 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->curseLevel = level;
                            
                            if( numRead == 3 ) {
                                if( existing->curseName != NULL ) {
                                    delete [] existing->curseName;
                                    existing->curseName = NULL;
                                    }
                                if( level > 0 ) {
                                    existing->curseName = 
                                        stringDuplicate( buffer );
                                    char *barPos = strstr( existing->curseName,
                                                           "_" );
                                    if( barPos != NULL ) {
                                        barPos[0] = ' ';
                                        }
                                    
                                    // display their cursed tag now
                                    if( existing->currentSpeech != NULL ) {
                                        delete [] existing->currentSpeech;
                                        }
                                    existing->currentSpeech = 
                                        autoSprintf( "X %s X",
                                                     existing->curseName );
                                    existing->speechFadeETATime =
                                        curTime + 3 +
                                        strlen( existing->currentSpeech ) / 5;
                                    existing->speechIsSuccessfulCurse = false;
                                    existing->speechIsCurseTag = true;
                                    existing->lastCurseTagDisplayTime = curTime;
                                    existing->speechIsOverheadLabel = false;
                                    }
                                }
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == CURSE_TOKEN_CHANGE ) {
            LiveObject *ourLiveObject = getOurLiveObject();
            
            if( ourLiveObject != NULL ) {
                
                sscanf( message, "CX\n%d", 
                        &( ourLiveObject->curseTokenCount ) );
                }
            }
        else if( type == CURSE_SCORE ) {
            LiveObject *ourLiveObject = getOurLiveObject();
            
            if( ourLiveObject != NULL ) {
                
                sscanf( message, "CS\n%d", 
                        &( ourLiveObject->excessCursePoints ) );
                }
            }
        else if( type == PONG ) {
            sscanf( message, "PONG\n%d", 
                    &( lastPongReceived ) );
            if( lastPongReceived == lastPingSent ) {
                pongDeltaTime = game_getCurrentTime() - pingSentTime;
                }
            }
        else if( type == NAMES ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            if( existing->name != NULL ) {
                                delete [] existing->name;
                                }
                            
                            char *firstSpace = strstr( lines[i], " " );
        
                            if( firstSpace != NULL ) {

                                char *nameStart = &( firstSpace[1] );
                                
                                existing->name = stringDuplicate( nameStart );
                                }
                            
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == DYING ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int sickFlag = 0;
                
                int numRead = sscanf( lines[i], "%d %d",
                                      &( id ), &sickFlag );

                if( numRead >= 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->dying = true;
                            if( sickFlag ) {
                                existing->sick = true;
                                }
                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == HEALED ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->dying = false;
                            existing->sick = false;
                            
                            // their wound will be gone after this
                            // play decay sound, if any, for their final
                            // wound state
                            if( existing->holdingID > 0 ) {
                                ObjectRecord *held = 
                                    getObject( existing->holdingID );
                                
                                if( held->decaySound.numSubSounds > 0 ) {    
                                    
                                    playSound( 
                                        held->decaySound,
                                        getVectorFromCamera( 
                                            existing->currentPos.x, 
                                            existing->currentPos.y ) );
                                    }
                                }
                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == POSSE_JOIN ) {
            int killer = 0;
            int target = 0;
            sscanf( message, "PJ\n%d %d", &killer, &target );
            
            if( killer > 0 ) {
                LiveObject *k = getGameObject( killer );
                
                if( target == ourID ) {
                    // they are chasing us
                    k->chasingUs = true;
                    }
                else {
                    // no longer chasing us
                    k->chasingUs = false;
                    }
                }
            }
        else if( type == PLAYER_OUT_OF_RANGE ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->outOfRange = true;
                            
                            if( existing->pendingReceivedMessages.size() > 0 ) {
                                // don't let pending messages for out-of-range
                                // players linger
                                playPendingReceivedMessages( existing );
                                }

                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == BABY_WIGGLE ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    if( id != ourID ) {
                        LiveObject *existing = getLiveObject( id );
                        
                        if( existing != NULL ) {
                            if( existing->heldByAdultID != -1 ) {
                                // wiggle in arms
                                existing->babyWiggle = true;
                                existing->babyWiggleProgress = 0;
                                }
                            else {
                                // wiggle on ground
                                existing->holdingFlip = ! existing->holdingFlip;
                                
                                addNewAnimPlayerOnly( existing, moving );
                                addNewAnimPlayerOnly( existing, ground );
                                }
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == FOOD_CHANGE ) {
            
            LiveObject *ourLiveObject = getOurLiveObject();
            
            if( ourLiveObject != NULL ) {
                
                
                int lastAteID, lastAteFillMax;
                
                int responsiblePlayerID = -1;
                
                int foodStore;
                int foodCapacity;
                double lastSpeed;

                int oldYumBonus = mYumBonus;
                mYumBonus = 0;
                
                int oldYumMultiplier = mYumMultiplier;
                mYumMultiplier = 0;
                

                sscanf( message, "FX\n%d %d %d %d %lf %d %d %d", 
                        &( foodStore ),
                        &( foodCapacity ),
                        &( lastAteID ),
                        &( lastAteFillMax ),
                        &( lastSpeed ),
                        &responsiblePlayerID,
                        &mYumBonus, &mYumMultiplier );


                
                if( oldYumBonus != mYumBonus ) {
                    // pull out of old stack, if present
                    for( int i=0; i<mOldYumBonus.size(); i++ ) {
                        if( mOldYumBonus.getElementDirect( i ) == mYumBonus ) {
                            mOldYumBonus.deleteElement( i );
                            i--;
                            }
                        }
                    
                    // fade existing
                    for( int i=0; i<mOldYumBonus.size(); i++ ) {
                        float fade =
                            mOldYumBonusFades.getElementDirect( i );
                        
                        if( fade > 0.5 ) {
                            fade -= 0.20;
                            }
                        else {
                            fade -= 0.1;
                            }
                        
                        *( mOldYumBonusFades.getElement( i ) ) = fade;
                        if( fade <= 0 ) {
                            mOldYumBonus.deleteElement( i );
                            mOldYumBonusFades.deleteElement( i );
                            i--;
                            }
                        }                    

                    if( oldYumBonus != 0 ) {
                        // push on top of stack
                        mOldYumBonus.push_back( oldYumBonus );
                        mOldYumBonusFades.push_back( 1.0f );
                        }
                    }
                
                if( mYumMultiplier != oldYumMultiplier ) {
                    int oldSlipIndex = -1;
                    int newSlipIndex = 0;
                    
                    for( int i=0; i<2; i++ ) {
                        if( mYumSlipNumberToShow[i] == oldYumMultiplier ) {
                            oldSlipIndex = i;
                            newSlipIndex = ( i + 1 ) % 2;
                            }
                        }
                    if( oldSlipIndex != -1 ) {
                        mYumSlipPosTargetOffset[ oldSlipIndex ] =
                            mYumSlipHideOffset[ oldSlipIndex ];
                        }

                    mYumSlipPosTargetOffset[ newSlipIndex ] =
                        mYumSlipHideOffset[ newSlipIndex ];
                    
                    if( mYumMultiplier > 0 ) {
                        mYumSlipPosTargetOffset[ newSlipIndex ].y += 36;
                        }
                    mYumSlipNumberToShow[ newSlipIndex ] = mYumMultiplier;
                    }
                
                
                

                if( responsiblePlayerID != -1 &&
                    getLiveObject( responsiblePlayerID ) != NULL &&
                    getLiveObject( responsiblePlayerID )->
                        pendingReceivedMessages.size() > 0 ) {
                    // someone else fed us, and they're still in the
                    // middle of a local walk with updates pending after
                    // they finish
                    
                    // defer this food update too
                    
                    LiveObject *rO = getLiveObject( responsiblePlayerID );
                    
                    printf( "Holding FX message caused by %d until later, "
                            "%d other messages pending for them\n",
                            responsiblePlayerID,
                            rO->pendingReceivedMessages.size() );
                    rO->pendingReceivedMessages.push_back(
                        stringDuplicate( message ) );
                    }
                else {
                    char foodIncreased = false;
                    int oldFoodStore = ourLiveObject->foodStore;
                    
                    if( foodCapacity == ourLiveObject->foodCapacity &&
                        foodStore > ourLiveObject->foodStore ) {
                        foodIncreased = true;
                        }
                    
                    ourLiveObject->foodStore = foodStore;
                    ourLiveObject->foodCapacity = foodCapacity;
                    ourLiveObject->lastSpeed = lastSpeed;
                    

                    if( mCurrentLastAteString != NULL ) {                    
                    

                        // one to add to erased list
                        // fade older ones first

                        for( int i=0; i<mOldLastAteStrings.size(); i++ ) {
                            float fade =
                                mOldLastAteFades.getElementDirect( i );
                        
                            if( fade > 0.5 ) {
                                fade -= 0.20;
                                }
                            else {
                                fade -= 0.1;
                                }
                        
                            *( mOldLastAteFades.getElement( i ) ) = fade;
                        

                            // bar must fade slower (different blending mode)
                            float barFade =
                                mOldLastAteBarFades.getElementDirect( i );
                        
                            barFade -= 0.01;
                        
                            *( mOldLastAteBarFades.getElement( i ) ) = barFade;
                        

                            if( fade <= 0 ) {
                                mOldLastAteStrings.deallocateStringElement( i );
                                mOldLastAteFillMax.deleteElement( i );
                                mOldLastAteFades.deleteElement( i );
                                mOldLastAteBarFades.deleteElement( i );
                                i--;
                                }

                            else if( 
                                strcmp( 
                                    mCurrentLastAteString, 
                                    mOldLastAteStrings.getElementDirect(i) )
                                == 0 ) {
                                // already in stack, move to top
                                mOldLastAteStrings.deallocateStringElement( i );
                                mOldLastAteFillMax.deleteElement( i );
                                mOldLastAteFades.deleteElement( i );
                                mOldLastAteBarFades.deleteElement( i );
                                i--;
                                }
                            }
                    
                        mOldLastAteStrings.push_back( mCurrentLastAteString );
                        mOldLastAteFillMax.push_back( mCurrentLastAteFillMax );
                        mOldLastAteFades.push_back( 1.0f );
                        mOldLastAteBarFades.push_back( 1.0f );

                        mCurrentLastAteString = NULL;
                        mCurrentLastAteFillMax = 0;
                        }
                
                    if( lastAteID != 0 ) {
                        ObjectRecord *lastAteObj = getObject( lastAteID );
                        
                        char *strUpper = stringToUpperCase(
                            lastAteObj->description );

                        stripDescriptionComment( strUpper );

                        const char *key = "lastAte";
                        
                        if( strstr( lastAteObj->description, 
                                    "+drink" ) != NULL ) {
                            key = "lastDrank";
                            }
                        else if( lastAteObj->permanent ) {
                            key = "lastAtePermanent";
                            }
                        
                        mCurrentLastAteString = 
                            autoSprintf( "%s %s",
                                         translate( key ),
                                         strUpper );
                        delete [] strUpper;
                    
                        mCurrentLastAteFillMax = lastAteFillMax;
                        }
                    else if( foodIncreased ) {
                        // we were fed, but we didn't eat anything
                        // must have been breast milk
                        mCurrentLastAteString =
                            stringDuplicate( translate( "breastMilk" ) );
                    
                        mCurrentLastAteFillMax = oldFoodStore;
                        }
                    

                    printf( "Our food = %d/%d\n", 
                            ourLiveObject->foodStore,
                            ourLiveObject->foodCapacity );
                

                    if( ourLiveObject->foodStore > 
                        ourLiveObject->maxFoodStore ) {
                        
                        ourLiveObject->maxFoodStore = ourLiveObject->foodStore;
                        }
                    if( ourLiveObject->foodCapacity > 
                        ourLiveObject->maxFoodCapacity ) {
                    
                        ourLiveObject->maxFoodCapacity = 
                            ourLiveObject->foodCapacity;
                        }


                    double curAge = computeCurrentAge( ourLiveObject );
                    
                    if( curAge < 3 ) {
                        mHungerSlipVisible = 0;
                        // special case for babies
                        // show either full or starving
                        // only show starving at 2 food or lower
                        // starving means you can nurse/eat
                        if( ourLiveObject->foodStore + mYumBonus <= 2 ) {
                             setMusicLoudness( 0 );
                             mHungerSlipVisible = 2;
                             mPulseHungerSound = true;
                            }
                        }
                    else if( ourLiveObject->foodStore == 
                        ourLiveObject->foodCapacity ) {
                        
                        mPulseHungerSound = false;

                        mHungerSlipVisible = 0;
                        }
                    else if( ourLiveObject->foodStore + mYumBonus <= 4 &&
                             curAge >= 57.33 ) {
                        mHungerSlipVisible = 2;
                        mPulseHungerSound = false;
                        }
                    else if( ourLiveObject->foodStore + mYumBonus <= 4 &&
                             curAge < 57.33 ) {
                        
                        // don't play hunger sounds at end of life
                        // because it interrupts our end-of-life song
                        // currently it's 2:37 long


                        // quiet music so hunger sound can be heard
                        setMusicLoudness( 0 );
                        mHungerSlipVisible = 2;
                    
                        if( ourLiveObject->foodStore > 0 ) {
                        
                            if( ourLiveObject->foodStore > 1 ) {
                                if( mHungerSound != NULL ) {
                                    // make sure it can be heard
                                    // even if paused
                                    setSoundLoudness( 1.0 );
                                    playSoundSprite( mHungerSound, 
                                                     getSoundEffectsLoudness(),
                                                     // middle
                                                     0.5 );
                                    }
                                mPulseHungerSound = false;
                                }
                            else {
                                mPulseHungerSound = true;
                                }
                            }
                        }
                    else if( ourLiveObject->foodStore + mYumBonus <= 8 ) {
                        mHungerSlipVisible = 1;
                        mPulseHungerSound = false;
                        }
                    else {
                        mHungerSlipVisible = -1;
                        }

                    if( ourLiveObject->foodStore + mYumBonus > 4 ||
                        computeCurrentAge( ourLiveObject ) >= 57 ) {
                        // restore music
                        setMusicLoudness( musicLoudness );
                        
                        mPulseHungerSound = false;
                        }
                    
                    }
                }
            }
        else if( type == HEAT_CHANGE ) {
            
            LiveObject *ourLiveObject = getOurLiveObject();
            
            if( ourLiveObject != NULL ) {
                sscanf( message, "HX\n%f %f %f", 
                        &( ourLiveObject->heat ),
                        &( ourLiveObject->foodDrainTime ),
                        &( ourLiveObject->indoorBonusTime ) );
                
                }
            }
        
        

        delete [] message;

        // process next message if there is one
        message = getNextServerMessage();
        }
    
    
    if( showFPS ) {
        timeMeasures[0] += game_getCurrentTime() - messageProcessStartTime;
        }
    



    updateStartTime = showFPS ? game_getCurrentTime() : 0;;


    if( mapPullMode && mapPullCurrentSaved && ! mapPullCurrentSent ) {
        char *message = autoSprintf( "MAP %d %d#",
                                     sendX( mapPullCurrentX ),
                                     sendY( mapPullCurrentY ) );
        
        sendToServerSocket( message );
        
        
        delete [] message;
        mapPullCurrentSent = true;
        }

        
    // check if we're about to move off the screen
    LiveObject *ourLiveObject = getOurLiveObject();


    
    if( !mapPullMode && 
        mDoneLoadingFirstObjectSet && ourLiveObject != NULL ) {
        

        // current age
        double age = computeCurrentAge( ourLiveObject );

        int sayCap = getSayLimit( age );

        if( vogMode ) {
            sayCap = 200;
            }
        
        char *currentText = mSayField.getText();
        
        if( strlen( currentText ) > 0 && currentText[0] == '/' ) {
            // typing a filter
            // hard cap at 25, regardless of age
            // don't want them typing long filters that overflow the display
            sayCap = 23;
            }
        delete [] currentText;

        mSayField.setMaxLength( sayCap );



        LiveObject *cameraFollowsObject = ourLiveObject;
        
        if( ourLiveObject->heldByAdultID != -1 ) {
            cameraFollowsObject = 
                getGameObject( ourLiveObject->heldByAdultID );
            
            if( cameraFollowsObject == NULL ) {
                ourLiveObject->heldByAdultID = -1;
                cameraFollowsObject = ourLiveObject;
                }
            }
        
        doublePair targetObjectPos = cameraFollowsObject->currentPos;
        
        if( vogMode ) {
            targetObjectPos = vogPos;
            }
        


        doublePair screenTargetPos = 
            mult( targetObjectPos, CELL_D );
        
        if( vogMode ) {
            // don't adjust camera
            }
        else if( 
            cameraFollowsObject->currentPos.x != cameraFollowsObject->xd
            ||
            cameraFollowsObject->currentPos.y != cameraFollowsObject->yd ) {
            
            // moving

            // push camera out in front
            

            double moveScale = 40 * cameraFollowsObject->currentSpeed;
            if( ( screenCenterPlayerOffsetX < 0 &&
                  cameraFollowsObject->currentMoveDirection.x < 0 )
                ||
                ( screenCenterPlayerOffsetX > 0 &&
                  cameraFollowsObject->currentMoveDirection.x > 0 ) ) {
                
                moveScale += fabs( screenCenterPlayerOffsetX );
                }
            
            screenCenterPlayerOffsetX -= 
                lrint( moveScale * 
                       cameraFollowsObject->currentMoveDirection.x );
            
            moveScale = 40 * cameraFollowsObject->currentSpeed;
            if( ( screenCenterPlayerOffsetY < 0 &&
                  cameraFollowsObject->currentMoveDirection.y < 0 )
                ||
                ( screenCenterPlayerOffsetY > 0 &&
                  cameraFollowsObject->currentMoveDirection.y > 0 ) ) {
                
                moveScale += fabs( screenCenterPlayerOffsetY );
                }
            

            screenCenterPlayerOffsetY -= 
                lrint( moveScale * 
                       cameraFollowsObject->currentMoveDirection.y );
 
            if( screenCenterPlayerOffsetX < -viewWidth / 3 ) {
                screenCenterPlayerOffsetX =  -viewWidth / 3;
                }
            if( screenCenterPlayerOffsetX >  viewWidth / 3 ) {
                screenCenterPlayerOffsetX =  viewWidth / 3;
                }
            if( screenCenterPlayerOffsetY < -viewHeight / 5 ) {
                screenCenterPlayerOffsetY =  -viewHeight / 5;
                }
            if( screenCenterPlayerOffsetY >  viewHeight / 6 ) {
                screenCenterPlayerOffsetY =  viewHeight / 6;
                }
            }
        else if( false ) { // skip for now
            // stopped moving
            
            if( screenCenterPlayerOffsetX > 0 ) {
                int speed = screenCenterPlayerOffsetX / 10;
                
                if( speed == 0 || speed > 2 ) {
                    speed = 1;
                    }
                screenCenterPlayerOffsetX -= speed;
                }
            if( screenCenterPlayerOffsetX < 0 ) {
                int speed = screenCenterPlayerOffsetX / 10;
                
                if( speed == 0 || speed < -2 ) {
                    speed = -1;
                    }
                screenCenterPlayerOffsetX -= speed;
                }
            
            if( screenCenterPlayerOffsetY > 0 ) {
                int speed = screenCenterPlayerOffsetY / 10;
                
                if( speed == 0 || speed > 2 ) {
                    speed = 1;
                    }
                screenCenterPlayerOffsetY -= speed;
                }
            if( screenCenterPlayerOffsetY < 0 ) {
                int speed = screenCenterPlayerOffsetY / 10;
                
                if( speed == 0 || speed < -2 ) {
                    speed = -1;
                    }
                screenCenterPlayerOffsetY -= speed;
                }
            
            }

        if( ! vogMode ) {
            
            screenTargetPos.x = 
                CELL_D * targetObjectPos.x - 
                screenCenterPlayerOffsetX;
            
            screenTargetPos.y = 
                CELL_D * targetObjectPos.y - 
                screenCenterPlayerOffsetY;
            }
        

        // whole pixels
        screenTargetPos.x = round( screenTargetPos.x );
        screenTargetPos.y = round( screenTargetPos.y );

        if( !shouldMoveCamera ) {
            screenTargetPos = lastScreenViewCenter;
            }
        

        doublePair dir = sub( screenTargetPos, lastScreenViewCenter );
        
        char viewChange = false;
        
        int maxRX = viewWidth / 15;
        int maxRY = viewHeight / 15;
        int maxR = 0;
        double moveSpeedFactor = 20 * cameraFollowsObject->currentSpeed;
        
        if( moveSpeedFactor < 1 ) {
            moveSpeedFactor = 1 * frameRateFactor;
            }

        if( fabs( dir.x ) > maxRX ) {
            double moveScale = moveSpeedFactor * sqrt( fabs(dir.x) - maxRX );

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.x = lrint( moveStep.x );
                        
            if( fabs( moveStep.x ) > 0 ) {
                lastScreenViewCenter.x += moveStep.x;
                viewChange = true;
                }
            }
        if( fabs( dir.y ) > maxRY ) {
            double moveScale = moveSpeedFactor * sqrt( fabs(dir.y) - maxRY );

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.y = lrint( moveStep.y );
                        
            if( fabs( moveStep.y ) > 0 ) {
                lastScreenViewCenter.y += moveStep.y;
                viewChange = true;
                }
            }
        

        if( false && length( dir ) > maxR ) {
            double moveScale = moveSpeedFactor * sqrt( length( dir ) - maxR );

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.x = lrint( moveStep.x );
            moveStep.y = lrint( moveStep.y );
                        
            if( length( moveStep ) > 0 ) {
                lastScreenViewCenter = add( lastScreenViewCenter, 
                                            moveStep );
                viewChange = true;
                }
            }
        

        if( viewChange ) {
            //lastScreenViewCenter.x = screenTargetPos.x;
            //lastScreenViewCenter.y = screenTargetPos.y;
            
            setViewCenterPosition( lastScreenViewCenter.x, 
                                   lastScreenViewCenter.y );
            
            }

        }
    

    HomePos *curHomePosRecord = getHomePosRecord();

    doublePair ourPos = { 0, 0 };

    if( ourLiveObject != NULL ) {
        ourPos = getPlayerPos( ourLiveObject );
        }
    
            
    // update all positions for moving objects
    if( !mapPullMode )
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );

        double curTime = game_getCurrentTime();

        if( o->currentSpeech != NULL ) {
            
            if( curTime > o->speechFadeETATime ) {
                
                o->speechFade -= 0.05 * frameRateFactor;

                if( o->speechFade <= 0 ) {
                    delete [] o->currentSpeech;
                    o->currentSpeech = NULL;
                    o->speechFade = 1.0;
                    o->speechIsSuccessfulCurse = false;
                    
                    
                    // display their curse tag after everything they say
                    if( ! o->speechIsCurseTag && 
                        o->curseName != NULL ) {
                        o->currentSpeech = autoSprintf( "X %s X",
                                                        o->curseName );
                        o->speechFadeETATime =
                            curTime + 3 +
                            strlen( o->currentSpeech ) / 5;
                        o->speechIsCurseTag = true;
                        o->speechIsOverheadLabel = false;
                        o->lastCurseTagDisplayTime = curTime;
                        }
                    }
                }
            }
        else {
            // show curse tags every 15 seconds
            // like a nervous tic
            if( o->curseName != NULL &&
                curTime - o->lastCurseTagDisplayTime > 15 ) {
                o->currentSpeech = autoSprintf( "X %s X",
                                                o->curseName );
                o->speechFadeETATime =
                    curTime + 3 +
                    strlen( o->currentSpeech ) / 5;
                o->speechIsCurseTag = true;
                o->speechIsOverheadLabel = false;
                o->lastCurseTagDisplayTime = curTime;
                }
            }

        if( o->currentSpeech == NULL ) {
            // check if we have an arrow to them
            
            if( curHomePosRecord != NULL &&
                curHomePosRecord->temporary && 
                curHomePosRecord->tempPerson && 
                curHomePosRecord->personID == o->id &&
                curHomePosRecord->tempPersonKey != NULL ) {
                
                char *transKey = autoSprintf( "%sLabel", 
                                              curHomePosRecord->tempPersonKey );
                
                const char *label = translate( transKey );
                
                delete [] transKey;
                
                o->currentSpeech = stringDuplicate( label );
                
                // expire along with arrow
                // unless interrupted by other speech
                o->speechFadeETATime = 
                    curHomePosRecord->temporaryExpireETA;
                o->speechIsCurseTag = false;
                o->speechIsOverheadLabel = true;
                }
            }
        
        
        
        if( o->currentEmot != NULL ) {
            if( game_getCurrentTime() > o->emotClearETATime ) {
                
                // play decay sounds for this emot

                if( !o->outOfRange ) {
                    for( int s=0; s<getEmotionNumObjectSlots(); s++ ) {
                                    
                        int id = getEmotionObjectByIndex( o->currentEmot, s );
                                    
                        if( id > 0 ) {
                            ObjectRecord *obj = getObject( id );
                                        
                            if( obj->decaySound.numSubSounds > 0 ) {    
                                    
                                playSound( 
                                    obj->decaySound,
                                    getVectorFromCamera( 
                                        o->currentPos.x,
                                        o->currentPos.y ) );
                                }
                            }
                        }
                    }
                
                o->currentEmot = NULL;
                }
            }

        
        double animSpeed = o->lastSpeed;
        

        if( o->holdingID > 0 ) {
            ObjectRecord *heldObj = getObject( o->holdingID );
            
            if( heldObj->speedMult > 1.0 ) {
                // don't speed up animations just because movement
                // speed has increased
                // but DO slow animations down
                animSpeed /= heldObj->speedMult;
                }
            }

        double oldFrameCount = o->animationFrameCount;
        o->animationFrameCount += animSpeed / BASE_SPEED;
        o->lastAnimationFrameCount += animSpeed / BASE_SPEED;
        

        if( o->curAnim == moving ) {
            o->frozenRotFrameCount += animSpeed / BASE_SPEED;
            
            }
            
        
        char holdingRideable = false;
            
        if( o->holdingID > 0 &&
            getObject( o->holdingID )->rideable ) {
            holdingRideable = true;
            }
        

        if( ! vogModeActuallyOn &&
            ! o->outOfRange &&
            distance( getPlayerPos( o ), ourPos ) > 
            maxChunkDimension ) {
            // mark as out of range, even if we've never heard an official
            // PO message about them
            // Maybe they weren't moving when we walked out of range for them
            // We don't want spurious animation and emote sounds to be played
            // for them in this case.
            o->outOfRange = true;
            }
        
                    
        // no anim sounds if out of range
        if( ! o->outOfRange ) {
            AnimType t = o->curAnim;
            doublePair pos = o->currentPos;

            if( o->curAnim != moving || !holdingRideable ) {
                // don't play player moving sound if riding something
                
                
                if( o->heldByAdultID != -1 ) {
                    t = held;
                    
                    
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        
                        LiveObject *parent = gameObjects.getElement( j );
                        
                        if( parent->id == o->heldByAdultID ) {
                            
                            pos = parent->currentPos;
                            }
                        }
                    }
                
                handleAnimSound( o->id, 
                                 o->displayID,
                                 computeCurrentAge( o ),
                                 t,
                                 oldFrameCount, o->animationFrameCount,
                                 pos.x,
                                 pos.y );    
                }
            
            if( o->currentEmot != NULL ) {
                int numSlots = getEmotionNumObjectSlots();
                
                for( int e=0; e<numSlots; e++ ) {
                    int oID =
                        getEmotionObjectByIndex( o->currentEmot, e );
                    
                    if( oID != 0 ) {
                        
                        handleAnimSound( o->id,
                                         oID,
                                         0,
                                         t,
                                         oldFrameCount, o->animationFrameCount,
                                         pos.x,
                                         pos.y ); 
                        }
                    }
                }
            }
            
        
        
        
        if( o->lastAnimFade > 0 ) {
            
            if( o->lastAnimFade == 1 ) {
                // fade just started
                // check if it's necessary
                
                if( isAnimFadeNeeded( o->displayID, 
                                      o->lastAnim, o->curAnim ) ) {
                    // fade needed, do nothing
                    }
                else {
                    // fade not needed
                    // jump to end of it
                    o->lastAnimFade = 0;
                    }
                }
            

            o->lastAnimFade -= 0.05 * frameRateFactor;
            if( o->lastAnimFade < 0 ) {
                o->lastAnimFade = 0;

                if( o->futureAnimStack->size() > 0 ) {
                    // move on to next in stack
                    
                    addNewAnimDirect( 
                        o, 
                        o->futureAnimStack->getElementDirect( 0 ) );

                    // pop from stack                    
                    o->futureAnimStack->deleteElement( 0 );
                    }
                
                }
            }

        oldFrameCount = o->heldAnimationFrameCount;
        o->heldAnimationFrameCount += animSpeed / BASE_SPEED;
        o->lastHeldAnimationFrameCount += animSpeed / BASE_SPEED;
        
        if( o->curHeldAnim == moving ) {
            o->heldFrozenRotFrameCount += animSpeed / BASE_SPEED;
            }
        
        if( o->holdingID > 0 && ! o->outOfRange ) {
            handleAnimSound( o->id,
                             o->holdingID, 0, o->curHeldAnim,
                             oldFrameCount, o->heldAnimationFrameCount,
                             o->currentPos.x,
                             o->currentPos.y );
            }
        

        if( o->lastHeldAnimFade > 0 ) {
            
            if( o->lastHeldAnimFade == 1 ) {
                // fade just started
                // check if it's necessary
                
                int heldDisplayID = o->holdingID;
                

                if( o->holdingID < 0 ) {
                    LiveObject *babyO = getGameObject( - o->holdingID );
            
                    if( babyO != NULL ) {    
                        heldDisplayID = babyO->displayID;
                        }
                    else {
                        heldDisplayID = 0;
                        }
                    }
                

                if( heldDisplayID > 0 &&
                    isAnimFadeNeeded( heldDisplayID, 
                                      o->lastHeldAnim, o->curHeldAnim ) ) {
                    // fade needed, do nothing
                    }
                else {
                    // fade not needed
                    // jump to end of it
                    o->lastHeldAnimFade = 0;
                    }
                }
            

            o->lastHeldAnimFade -= 0.05 * frameRateFactor;
            if( o->lastHeldAnimFade < 0 ) {
                o->lastHeldAnimFade = 0;
                
                if( o->futureHeldAnimStack->size() > 0 ) {
                    // move on to next in stack
                    
                    addNewHeldAnimDirect( 
                        o,
                        o->futureHeldAnimStack->getElementDirect( 0 ) );
                    
                    // pop from stack
                    o->futureHeldAnimStack->deleteElement( 0 );
                    }
                
                }
            }


        if( o->currentSpeed != 0 && o->pathToDest != NULL ) {

            GridPos curStepDest = o->pathToDest[ o->currentPathStep ];
            GridPos nextStepDest = o->pathToDest[ o->currentPathStep + 1 ];
            
            doublePair startPos = { (double)curStepDest.x, 
                                    (double)curStepDest.y };

            doublePair endPos = { (double)nextStepDest.x, 
                                  (double)nextStepDest.y };
            
            while( distance( o->currentPos, endPos ) <= o->currentSpeed &&
                   o->currentPathStep < o->pathLength - 2 ) {
                
                // speed too great, overshooting next step
                
                o->currentPathStep ++;
                o->numFramesOnCurrentStep = 0;
                
                nextStepDest = o->pathToDest[ o->currentPathStep + 1 ];
            
                endPos.x = nextStepDest.x;
                endPos.y = nextStepDest.y;
                }


            doublePair dir = normalize( sub( endPos, o->currentPos ) );
            

            double turnFactor = 0.35;
            
            if( o->currentPathStep == o->pathLength - 2 ) {
                // last segment
                // speed up turn toward final spot so that we
                // don't miss it and circle around it forever
                turnFactor = 0.5;
                }

            if( o->currentGridSpeed > 4 ) {
                // tighten turns as speed increases to avoid
                // circling around forever
                turnFactor *= o->currentGridSpeed / 4;
                }
            
            
            if( dot( dir, o->currentMoveDirection ) >= 0 ) {
                // a right angle turn or smaller

            
                o->currentMoveDirection = 
                    normalize( add( o->currentMoveDirection, 
                                    mult( dir, 
                                          turnFactor * frameRateFactor ) ) );
                }
            else {
                // a double-back in the path
                // don't tot smooth turn through this, because
                // it doesn't resolve
                // instead, just turn sharply
                o->currentMoveDirection = dir;
                }
            
            if( o->numFramesOnCurrentStep * o->currentSpeed  * frameRateFactor
                > 2 ) {
                // spent twice as much time reaching this tile as we should
                // may be circling
                // go directly there instead
                o->currentMoveDirection = dir;
                }

            if( o->currentGridSpeed * frameRateFactor > 12 ) {
                // at high speed, can't round turns at all without circling
                o->currentMoveDirection = dir;
                }
            
            
            

            // don't change flip unless moving substantially in x
            if( fabs( o->currentMoveDirection.x ) > 0.5 ) {
                if( o->currentMoveDirection.x > 0 ) {
                    o->holdingFlip = false;
                    }
                else {
                    o->holdingFlip = true;
                    }         
                }
            

            if( o->currentPathStep < o->pathLength - 2 ) {

                o->currentPos = add( o->currentPos,
                                     mult( o->currentMoveDirection,
                                           o->currentSpeed ) );
                
                addNewAnim( o, moving );
                
                
                if( pathStepDistFactor * distance( o->currentPos,
                                    startPos )
                    >
                    distance( o->currentPos,
                              endPos ) ) {
                    
                    o->currentPathStep ++;
                    o->numFramesOnCurrentStep = 0;
                    }
                }
            else {

                if( o->id == ourID && mouseDown && shouldMoveCamera &&
                    o->pathLength > 2 ) {
                    
                    float worldMouseX, worldMouseY;
                    
                    screenToWorld( lastScreenMouseX,
                                   lastScreenMouseY,
                                   &worldMouseX,
                                   &worldMouseY );

                    doublePair worldMouse = { worldMouseX, worldMouseY };
                    
                    doublePair worldCurrent = mult( o->currentPos,
                                                    CELL_D );
                    doublePair delta = sub( worldMouse, worldCurrent );

                    // if player started by clicking on nothing
                    // allow continued movement right away
                    // however, if they started by clicking on something
                    // make sure they are really holding the mouse down
                    // (give them time to unpress the mouse)
                    if( nextActionMessageToSend == NULL ||
                        mouseDownFrames >  
                        minMouseDownFrames / frameRateFactor ) {
                        
                        double absX = fabs( delta.x );
                        double absY = fabs( delta.y );
                        

                        if( absX > CELL_D * 1 
                            ||
                            absY > CELL_D * 1 ) {
                            
                            if( absX < CELL_D * 4 
                                &&
                                absY < CELL_D * 4 
                                &&
                                mouseDownFrames >  
                                minMouseDownFrames / frameRateFactor ) {
                                
                                // they're holding mouse down very close
                                // to to their character
                                
                                // throw mouse way out, further in the same
                                // direction
                                
                                // we don't want to repeatedly find a bunch
                                // of short-path moves when mouse
                                // is held down
                            
                                doublePair mouseVector =
                                    mult( 
                                        normalize( 
                                            sub( worldMouse, worldCurrent ) ),
                                        CELL_D * 4 );
                                
                                doublePair fakeClick = add( worldCurrent,
                                                            mouseVector );
                                
                                o->useWaypoint = true;
                                // leave some wiggle room here
                                // path through waypoint might get extended
                                // if it involves obstacles
                                o->maxWaypointPathLength = 10;
                                
                                o->waypointX = lrint( worldMouseX / CELL_D );
                                o->waypointY = lrint( worldMouseY / CELL_D );

                                isAutoClick = true;
                                pointerDown( fakeClick.x, fakeClick.y );
                                isAutoClick = false;

                                o->useWaypoint = false;
                                }
                            else {
                                isAutoClick = true;
                                pointerDown( worldMouseX, worldMouseY );
                                isAutoClick = false;
                                }
                            }
                        }
                    }
                else if( o->id == ourID && o->pathLength >= 2 &&
                         nextActionMessageToSend == NULL &&
                         distance( endPos, o->currentPos )
                         < o->currentSpeed ) {

                    // reached destination of bare-ground click

                    // check for auto-walk on road

                    GridPos prevStep = o->pathToDest[ o->pathLength - 2 ];
                    GridPos finalStep = o->pathToDest[ o->pathLength - 1 ];
                    
                    int mapIP = getMapIndex( prevStep.x, prevStep.y );
                    int mapIF = getMapIndex( finalStep.x, finalStep.y );
                    
                    if( mapIF != -1 && mapIP != -1 ) {
                        int floor = mMapFloors[ mapIF ];
                        
                        if( floor > 0 && 
                            sameRoadClass( mMapFloors[ mapIP ], floor ) && 
                            getObject( floor )->rideable ) {
                            
                            // rideable floor is a road!
                            
                            int xDir = finalStep.x - prevStep.x;
                            int yDir = finalStep.y - prevStep.y;
                            
                            GridPos nextStep = finalStep;
                            nextStep.x += xDir;
                            nextStep.y += yDir;
                            
                            int len = 0;

                            if( isSameRoad( floor, finalStep, xDir, yDir ) ) {
                                // floor continues in same direction
                                // go as far as possible in that direction
                                // with next click
                                while( len < 5 && isSameRoad( floor, nextStep,
                                                              xDir, yDir ) ) {
                                    nextStep.x += xDir;
                                    nextStep.y += yDir;
                                    len ++;
                                    }
                                }
                            else {
                                nextStep = finalStep;
                                char foundBranch = false;
                                

                                // continuing in same direction goes off road
                                // try branching off in another
                                // direction instead

                                int nX[8] = { 1, 1,  1, -1, -1, -1, 0,  0 };
                                int nY[8] = { 1, 0, -1,  1,  0, -1, 1, -1 };
                                
                                SimpleVector<GridPos> allDirs;
                                // dist of each dir from xDir,yDir
                                SimpleVector<double> allDist;
                                
                                GridPos lastDir = { xDir, yDir };
                                
                                SimpleVector<GridPos> sortedDirs;
                                SimpleVector<double> sortedDist;

                                for( int i=0; i<8; i++ ) {
                                    GridPos p = { nX[i], nY[i] };
                                    
                                    // do not include lastDir
                                    // or completely opposite dir
                                    if( equal( p, lastDir ) ) {
                                        continue;
                                        }
                                    if( p.x == lastDir.x * -1 &&
                                        p.y == lastDir.y * -1 ) {
                                        continue;
                                        }

                                    allDirs.push_back( p );
                                    allDist.push_back( 
                                        distance2( lastDir, p ) );
                                    }
                                
                                while( allDirs.size() > 0 ) {
                                    double minDist = 99999;
                                    int minInd = -1;
                                    for( int i=0; i<allDirs.size(); i++ ) {
                                        double d = 
                                            allDist.getElementDirect( i );
                                        if( d < minDist ) {
                                            minDist = d;
                                            minInd = i;
                                            }
                                        }
                                    sortedDirs.push_back( 
                                        allDirs.getElementDirect( minInd ) );
                                    sortedDist.push_back( 
                                        allDist.getElementDirect( minInd ) );
                                    allDirs.deleteElement( minInd );
                                    allDist.deleteElement( minInd );
                                    }
                                
                                printf( "Last dir = %d,%d\n", xDir, yDir );
                                for( int i=0; i<8; i++ ) {
                                    printf( 
                                        "  %d (dist %f):  %d,%d\n",
                                        i, 
                                        sortedDist.getElementDirect( i ),
                                        sortedDirs.getElementDirect( i ).x,
                                        sortedDirs.getElementDirect( i ).y );
                                    }
                                
                                
                                // now we have 6 dirs, sorted by 
                                // how far off they are from lastDir 
                                // and NOT including lastDir or its
                                // complete opposite.

                                // find the first one that continues
                                // on the same road surface
                                for( int i=0; i<6 && !foundBranch; i++ ) {
                                    
                                    GridPos d = 
                                        sortedDirs.getElementDirect( i );
                                    xDir = d.x;
                                    yDir = d.y;
                                    
                                    if( isSameRoad( floor, finalStep, xDir,
                                                     yDir ) ) {
                                        foundBranch = true;
                                        }
                                    }
                                
                                if( foundBranch ) {
                                    nextStep.x += xDir;
                                    nextStep.y += yDir;
                                    
                                    while( len < 5 &&
                                           isSameRoad( floor, nextStep,
                                                        xDir, yDir ) ) {
                                        nextStep.x += xDir;
                                        nextStep.y += yDir;
                                        len++;
                                        }
                                    }
                                }

                            if( ! equal( nextStep, finalStep ) ) {
                                // found straight-line continue of road
                                // auto-click there (but don't hold
                                
                                // avoid clicks on self and objects
                                // when walking on road
                                mForceGroundClick = true;
                                pointerDown( nextStep.x * CELL_D, 
                                             nextStep.y * CELL_D );
                                
                                pointerUp( nextStep.x * CELL_D, 
                                           nextStep.y * CELL_D );
                                
                                mForceGroundClick = false;
                                
                                endPos.x = (double)( nextStep.x );
                                endPos.y = (double)( nextStep.y );
                                }
                            }
                        }
                    }
                

                if( distance( endPos, o->currentPos )
                    < o->currentSpeed ) {

                    // reached destination
                    o->currentPos = endPos;
                    o->currentSpeed = 0;
                    o->currentGridSpeed = 0;
                    
                    playPendingReceivedMessages( o );
                    
                    //trailColor.r = randSource.getRandomBoundedDouble( 0, .5 );
                    //trailColor.g = randSource.getRandomBoundedDouble( 0, .5 );
                    //trailColor.b = randSource.getRandomBoundedDouble( 0, .5 );
                    

                    if( ( o->id != ourID && 
                          ! o->somePendingMessageIsMoreMovement ) 
                        ||
                        ( o->id == ourID && 
                          nextActionMessageToSend == NULL ) ) {
                        
                        // simply stop walking
                        if( o->holdingID != 0 ) {
                            addNewAnim( o, ground2 );
                            }
                        else {
                            addNewAnim( o, ground );
                            }
                        }

                    printf( "Reached dest (%.0f,%.0f) %f seconds early\n",
                            endPos.x, endPos.y,
                            o->moveEtaTime - game_getCurrentTime() );
                    }
                else {

                    addNewAnim( o, moving );
                    
                    o->currentPos = add( o->currentPos,
                                         mult( o->currentMoveDirection,
                                               o->currentSpeed ) );

                    if( 1.5 * distance( o->currentPos,
                                        startPos )
                        >
                        distance( o->currentPos,
                                  endPos ) ) {
                        
                        o->onFinalPathStep = true;
                        }
                    }
                }
            
            // correct move speed based on how far we have left to go
            // and eta wall-clock time
            
            // make this correction once per second
            if( game_getCurrentTime() - o->timeOfLastSpeedUpdate
                > .25 ) {
    
                //updateMoveSpeed( o );
                }
            
            o->numFramesOnCurrentStep++;
            }
        
        
        
        double progressInc = 0.025 * frameRateFactor;

        if( o->id == ourID &&
            ( o->pendingAction || o->pendingActionAnimationProgress != 0 ) ) {
            
            o->pendingActionAnimationProgress += progressInc;
            
            if( o->pendingActionAnimationProgress > 1 ) {
                if( o->pendingAction ) {
                    // still pending, wrap around smoothly
                    o->pendingActionAnimationProgress -= 1;
                    }
                else {
                    // no longer pending, finish last cycle by snapping
                    // back to 0
                    o->pendingActionAnimationProgress = 0;
                    o->actionTargetTweakX = 0;
                    o->actionTargetTweakY = 0;
                    }
                }
            }
        else if( o->id != ourID && o->pendingActionAnimationProgress != 0 ) {
            
            o->pendingActionAnimationProgress += progressInc;
            
            if( o->pendingActionAnimationProgress > 1 ) {
                // no longer pending, finish last cycle by snapping
                // back to 0
                o->pendingActionAnimationProgress = 0;
                o->actionTargetTweakX = 0;
                o->actionTargetTweakY = 0;
                }
            }
        }
    
    
    // step fades on location-based speech
    if( !mapPullMode )
    for( int i=0; i<locationSpeech.size(); i++ ) {
        LocationSpeech *ls = locationSpeech.getElement( i );
        if( game_getCurrentTime() > ls->fadeETATime ) {
            ls->fade -= 0.05 * frameRateFactor;
            
            if( ls->fade <= 0 ) {
                delete [] ls->speech;
                locationSpeech.deleteElement( i );
                i --;
                }
            }
        }
    

    double currentTime = game_getCurrentTime();
    

    if( nextActionMessageToSend != NULL
        && ourLiveObject != NULL
        && ourLiveObject->currentSpeed == 0 
        && (
            isGridAdjacent( ourLiveObject->xd, ourLiveObject->yd,
                            playerActionTargetX, playerActionTargetY )
            ||
            ( ourLiveObject->xd == playerActionTargetX &&
              ourLiveObject->yd == playerActionTargetY ) 
            ||
            playerActionTargetNotAdjacent ) ) {
        
        // done moving on client end
        // can start showing pending action animation, even if 
        // end of motion not received from server yet

        if( !playerActionPending ) {
            playerActionPending = true;
            ourLiveObject->pendingAction = true;
            
            // start measuring again to detect network outages 
            // during this pending action
            largestPendingMessageTimeGap = 0;
            

            // start on first frame to force at least one cycle no
            // matter how fast the server responds
            ourLiveObject->pendingActionAnimationProgress = 
                0.025 * frameRateFactor;
            
            ourLiveObject->pendingActionAnimationStartTime = 
                currentTime;
            
            if( nextActionEating ) {
                addNewAnim( ourLiveObject, eating );
                }
            else {
                addNewAnim( ourLiveObject, doing );
                }
            }
        
        
        // wait until 
        // we've stopped moving locally
        // AND animation has played for a bit
        // (or we know that recent ping has been long enough so that
        //  animation will play long enough without waiting ahead of time)
        // AND server agrees with our position
        if( ! ourLiveObject->inMotion && 
            currentTime - ourLiveObject->pendingActionAnimationStartTime > 
            0.166 - ourLiveObject->lastResponseTimeDelta &&
            ourLiveObject->xd == ourLiveObject->xServer &&
            ourLiveObject->yd == ourLiveObject->yServer ) {
            
 
            // move end acked by server AND action animation in progress

            // queued action waiting for our move to end
            
            ourLiveObject->lastActionSendStartTime = currentTime;
            sendToServerSocket( nextActionMessageToSend );
            
            // reset the timer, because we've gotten some information
            // back from the server about our action
            // so it doesn't seem like we're experiencing a dropped-message
            // bug just yet.
            ourLiveObject->pendingActionAnimationStartTime = 
                game_getCurrentTime();


            if( nextActionEating ) {
                // don't play eating sound here until 
                // we hear from server that we actually ate    
                if( false && ourLiveObject->holdingID > 0 ) {
                    ObjectRecord *held = getObject( ourLiveObject->holdingID );
                    
                    if( held->eatingSound.numSubSounds > 0 ) {
                        playSound( held->eatingSound,
                                   getVectorFromCamera( 
                                       ourLiveObject->currentPos.x, 
                                       ourLiveObject->currentPos.y ) );
                        }       
                    }
                }
            

            delete [] nextActionMessageToSend;
            nextActionMessageToSend = NULL;
            }
        }



    if( ourLiveObject != NULL ) {
        if( ourLiveObject->holdingFlip != ourLiveObject->lastFlipSent &&
            currentTime - ourLiveObject->lastFlipSendTime > 2 ) {
            
            // been 2 seconds since last sent FLIP to server
            // avoid spamming
            int offset = 1;
            
            if( ourLiveObject->holdingFlip ) {
                offset = -1;
                }
            
            char *message = autoSprintf( 
                "FLIP %d %d#",
                ourLiveObject->xd + offset,
                ourLiveObject->yd );
            
            sendToServerSocket( message );
            
            delete [] message;
            ourLiveObject->lastFlipSendTime = currentTime;
            ourLiveObject->lastFlipSent = ourLiveObject->holdingFlip;
            }
        }
    
    

    if( mFirstServerMessagesReceived == 3 ) {

        if( mStartedLoadingFirstObjectSet && ! mDoneLoadingFirstObjectSet ) {
            mDoneLoadingFirstObjectSet = 
                isLiveObjectSetFullyLoaded( &mFirstObjectSetLoadingProgress );
            
            if( mDoneLoadingFirstObjectSet &&
                game_getCurrentTime() - mStartedLoadingFirstObjectSetStartTime
                < 1 ) {
                // always show loading progress for at least 1 second
                //mDoneLoadingFirstObjectSet = false;
                }
            

            if( mDoneLoadingFirstObjectSet ) {
                mPlayerInFlight = false;
                
                printf( "First map load done\n" );
                
                int loaded, total;
                countLoadedSprites( &loaded, &total );
                
                printf( "%d/%d sprites loaded\n", loaded, total );
                

                restartMusic( computeCurrentAge( ourLiveObject ),
                              ourLiveObject->ageRate );
                setSoundLoudness( 1.0 );
                resumePlayingSoundSprites();
                setMusicLoudness( musicLoudness );

                // center view on player's starting position
                lastScreenViewCenter.x = CELL_D * ourLiveObject->xd;
                lastScreenViewCenter.y = CELL_D * ourLiveObject->yd;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );

                mapPullMode = 
                    SettingsManager::getIntSetting( "mapPullMode", 0 );
                mapPullStartX = 
                    SettingsManager::getIntSetting( "mapPullStartX", -10 );
                mapPullStartY = 
                    SettingsManager::getIntSetting( "mapPullStartY", -10 );
                mapPullEndX = 
                    SettingsManager::getIntSetting( "mapPullEndX", 10 );
                mapPullEndY = 
                    SettingsManager::getIntSetting( "mapPullEndY", 10 );
                
                mapPullCurrentX = mapPullStartX;
                mapPullCurrentY = mapPullStartY;

                if( mapPullMode ) {
                    mMapGlobalOffset.x = mapPullCurrentX;
                    mMapGlobalOffset.y = mapPullCurrentY;
                    mMapGlobalOffsetSet = true;
                    
                    applyReceiveOffset( &mapPullCurrentX, &mapPullCurrentY );
                    applyReceiveOffset( &mapPullStartX, &mapPullStartY );
                    applyReceiveOffset( &mapPullEndX, &mapPullEndY );
                

                    mapPullCurrentSaved = true;
                    mapPullModeFinalImage = false;
                    
                    char *message = autoSprintf( "MAP %d %d#",
                                                 sendX( mapPullCurrentX ),
                                                 sendY( mapPullCurrentY ) );

                    sendToServerSocket( message );
               
                    mapPullCurrentSent = true;

                    delete [] message;

                    int screenWidth, screenHeight;
                    getScreenDimensions( &screenWidth, &screenHeight );

                    double scale = screenWidth / (double)screenW;

                    mapPullTotalImage = 
                        new Image( lrint(
                                       ( 10 + mapPullEndX - mapPullStartX ) 
                                       * CELL_D * scale ),
                                   lrint( ( 6 + mapPullEndY - mapPullStartY ) 
                                          * CELL_D * scale ),
                                   3, false );
                    numScreensWritten = 0;
                    }
                }
            }
        else {
            
            clearLiveObjectSet();
            
            
            // push all objects from grid, live players, what they're holding
            // and wearing into live set

            // any direct-from-death graves
            // we want these to pop in instantly whenever someone dies
            SimpleVector<int> *allPossibleDeathMarkerIDs = 
                getAllPossibleDeathIDs();
            
            for( int i=0; i<allPossibleDeathMarkerIDs->size(); i++ ) {
                addBaseObjectToLiveObjectSet( 
                    allPossibleDeathMarkerIDs->getElementDirect( i ) );
                }
            
            
            
            // next, players
            for( int i=0; i<gameObjects.size(); i++ ) {
                LiveObject *o = gameObjects.getElement( i );
                
                addBaseObjectToLiveObjectSet( o->displayID );
                
                
                if( ! o->allSpritesLoaded ) {
                    // check if they're loaded yet
                    
                    int numLoaded = 0;

                    ObjectRecord *displayObj = getObject( o->displayID );
                    for( int s=0; s<displayObj->numSprites; s++ ) {
                        
                        if( markSpriteLive( displayObj->sprites[s] ) ) {
                            numLoaded ++;
                            }
                        else if( getSpriteRecord( displayObj->sprites[s] )
                                 == NULL ) {
                            // object references sprite that doesn't exist
                            // count as loaded
                            numLoaded ++;
                            }
                        }
                    
                    if( numLoaded == displayObj->numSprites ) {
                        o->allSpritesLoaded = true;
                        }
                    }
                

                // and what they're holding
                if( o->holdingID > 0 ) {
                    addBaseObjectToLiveObjectSet( o->holdingID );

                    // and what it contains
                    for( int j=0; j<o->numContained; j++ ) {
                        addBaseObjectToLiveObjectSet(
                            o->containedIDs[j] );
                        
                        for( int s=0; s<o->subContainedIDs[j].size(); s++ ) {
                            addBaseObjectToLiveObjectSet(
                                o->subContainedIDs[j].getElementDirect( s ) );
                            }
                        }
                    }
                
                // and their clothing
                for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                    ObjectRecord *cObj = clothingByIndex( o->clothing, c );
                    
                    if( cObj != NULL ) {
                        addBaseObjectToLiveObjectSet( cObj->id );

                        // and what it containes
                        for( int cc=0; 
                             cc< o->clothingContained[c].size(); cc++ ) {
                            int ccID = 
                                o->clothingContained[c].getElementDirect( cc );
                            addBaseObjectToLiveObjectSet( ccID );
                            }
                        }
                    }
                }
            
            
            // next all objects in grid
            int numMapCells = mMapD * mMapD;
            
            for( int i=0; i<numMapCells; i++ ) {
                if( mMapFloors[i] > 0 ) {
                    addBaseObjectToLiveObjectSet( mMapFloors[i] );
                    }
                
                if( mMap[i] > 0 ) {
                    
                    addBaseObjectToLiveObjectSet( mMap[i] );
            
                    // and what is contained in each object
                    int numCont = mMapContainedStacks[i].size();
                    
                    for( int j=0; j<numCont; j++ ) {
                        addBaseObjectToLiveObjectSet(
                            mMapContainedStacks[i].getElementDirect( j ) );
                        
                        SimpleVector<int> *subVec =
                            mMapSubContainedStacks[i].getElement( j );
                        
                        int numSub = subVec->size();
                        for( int s=0; s<numSub; s++ ) {
                            addBaseObjectToLiveObjectSet( 
                                subVec->getElementDirect( s ) );
                            }
                        }
                    }
                }


            markEmotionsLive();
            
            finalizeLiveObjectSet();
            
            mStartedLoadingFirstObjectSet = true;
            mStartedLoadingFirstObjectSetStartTime = game_getCurrentTime();
            }
        }


    if( showFPS ) {
        timeMeasures[1] += game_getCurrentTime() - updateStartTime;
        }
    
    }




// previous function (step) is so long that it's slowin down Emacs
// on the following function
// put a dummy function here to help emacs.
static void dummyFunctionA() {
    // call self to avoid compiler warning for unused function
    if( false ) {
        dummyFunctionA();
        }
    }






char LivingLifePage::isSameRoad( int inFloor, GridPos inFloorPos, 
                                 int inDX, int inDY ) {    
    GridPos nextStep = inFloorPos;
    nextStep.x += inDX;
    nextStep.y += inDY;
                            
    int nextMapI = getMapIndex( nextStep.x, nextStep.y );
                          
    GridPos nextMap = getMapPos( nextStep.x, nextStep.y );
  
    if( nextMapI != -1 
        &&
        sameRoadClass( mMapFloors[ nextMapI ], inFloor ) 
        && 
        ! getCellBlocksWalking( nextMap.x, nextMap.y ) ) {
        return true;
        }
    return false;
    }



  
void LivingLifePage::makeActive( char inFresh ) {
    // unhold E key
    mEKeyDown = false;
    mZKeyDown = false;
    mXKeyDown = false;
    
    mouseDown = false;
    shouldMoveCamera = true;
    
    screenCenterPlayerOffsetX = 0;
    screenCenterPlayerOffsetY = 0;
    

    mLastMouseOverID = 0;
    mCurMouseOverID = 0;
    mCurMouseOverBiome = -1;
    mCurMouseOverFade = 0;
    mCurMouseOverBehind = false;
    mCurMouseOverPerson = false;
    mCurMouseOverSelf = false;

    mPrevMouseOverSpots.deleteAll();
    mPrevMouseOverSpotFades.deleteAll();
    mPrevMouseOverSpotsBehind.deleteAll();
    
    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;
    mCurMouseOverCellFade = 0;
    mLastClickCell.x = -1;
    mLastClickCell.y = -1;
    
    
    mPrevMouseOverCells.deleteAll();
    mPrevMouseOverCellFades.deleteAll();

    mPrevMouseClickCells.deleteAll();
    mPrevMouseClickCellFades.deleteAll();

    

    if( !inFresh ) {
        return;
        }

    for( int i=0; i<homelands.size(); i++ ) {
        delete [] homelands.getElementDirect( i ).familyName;
        }
    homelands.deleteAll();
    

    usedToolSlots = 0;
    totalToolSlots = 0;
    

    ourUnmarkedOffspring.deleteAll();
    
    clearToolLearnedStatus();

    for( int j=0; j<2; j++ ) {
        mapHintEverDrawn[j] = false;
        personHintEverDrawn[j] = false;
        }

    mOldHintArrows.deleteAll();

    mGlobalMessageShowing = false;
    mGlobalMessageStartTime = 0;
    mGlobalMessagesToDestroy.deallocateStringElements();
    

    mCurrentHintTargetObject[0] = 0;
    mCurrentHintTargetObject[1] = 0;
    
    
    offScreenSounds.deleteAll();
    
    oldHomePosStack.deleteAll();
    
    oldHomePosStack.push_back_other( &homePosStack );
    
    homePosStack.deleteAll();
    

    takingPhoto = false;
    photoSequenceNumber = -1;
    waitingForPhotoSig = false;
    if( photoSig != NULL ) {
        delete [] photoSig;
        photoSig = NULL;
        }


    graveRequestPos.deleteAll();
    ownerRequestPos.deleteAll();
    
    clearOwnerInfo();

    clearLocationSpeech();

    mPlayerInFlight = false;
    
    vogMode = false;
    
    showFPS = false;
    showNet = false;
    showPing = false;
    
    waitForFrameMessages = false;

    serverSocketConnected = false;
    serverSocketHardFail = false;
    connectionMessageFade = 1.0f;
    connectedTime = 0;

    
    for( int j=0; j<2; j++ ) {
        mPreviousHomeDistStrings[j].deallocateStringElements();
        mPreviousHomeDistFades[j].deleteAll();
        }
    
    mForceHintRefresh = false;
    mLastHintSortedSourceID = 0;
    mLastHintSortedList.deleteAll();

    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        mHintTargetOffset[i] = mHintHideOffset[i];
        mHintPosOffset[i] = mHintHideOffset[i];
        }

    mCurrentHintObjectID = 0;
    mCurrentHintIndex = 0;
    
    mNextHintObjectID = 0;
    mNextHintIndex = 0;
    
    
    if( mHintFilterString != NULL ) {
        delete [] mHintFilterString;
        mHintFilterString = NULL;
        }

    if( mLastHintFilterString != NULL ) {
        delete [] mLastHintFilterString;
        mLastHintFilterString = NULL;
        }

    if( mPendingFilterString != NULL ) {
        delete [] mPendingFilterString;
        mPendingFilterString = NULL;
        }


    int tutorialDone = SettingsManager::getIntSetting( "tutorialDone", 0 );
    
    if( tutorialDone == 0 ) {
        mTutorialNumber = 1;
        }
    else if( tutorialDone == 1 ) {
        mTutorialNumber = 2;
        }
    else {
        mTutorialNumber = 0;
        }
    
    if( mForceRunTutorial != 0 ) {
        mTutorialNumber = mForceRunTutorial;
        mForceRunTutorial = 0;
        }

    mLiveTutorialSheetIndex = -1;
    mLiveCravingSheetIndex = -1;
    
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {    
        mTutorialTargetOffset[i] = mTutorialHideOffset[i];
        mTutorialPosOffset[i] = mTutorialHideOffset[i];
        mTutorialMessage[i] = "";

        mCravingTargetOffset[i] = mCravingHideOffset[i];
        mCravingPosOffset[i] = mCravingHideOffset[i];
        if( mCravingMessage[i] != NULL ) {
            delete [] mCravingMessage[i];
            mCravingMessage[i] = NULL;
            }
        }
    
    

    savingSpeechEnabled = SettingsManager::getIntSetting( "allowSavingSpeech",
                                                          0 );


    for( int i=0; i<mGraveInfo.size(); i++ ) {
        delete [] mGraveInfo.getElement(i)->relationName;
        }
    mGraveInfo.deleteAll();

    clearOwnerInfo();
    

    mRemapDelay = 0;
    mRemapPeak = 0;
    mRemapDirection = 1.0;
    mCurrentRemapFraction = 0;
    
    apocalypseInProgress = false;

    clearMap();
    
    mMapExtraMovingObjects.deleteAll();
    mMapExtraMovingObjectsDestWorldPos.deleteAll();
    mMapExtraMovingObjectsDestObjectIDs.deleteAll();
            

    mMapGlobalOffsetSet = false;
    mMapGlobalOffset.x = 0;
    mMapGlobalOffset.y = 0;
    
    
    mNotePaperPosOffset = mNotePaperHideOffset;

    mNotePaperPosTargetOffset = mNotePaperPosOffset;

    
    for( int j=0; j<2; j++ ) {
        mHomeSlipPosOffset[j] = mHomeSlipHideOffset[j];
        mHomeSlipPosTargetOffset[j] = mHomeSlipPosOffset[j];
        }
    

    mLastKnownNoteLines.deallocateStringElements();
    mErasedNoteChars.deleteAll();
    mErasedNoteCharOffsets.deleteAll();
    mErasedNoteCharFades.deleteAll();
    
    mSentChatPhrases.deallocateStringElements();

    for( int i=0; i<3; i++ ) {    
        mHungerSlipPosOffset[i] = mHungerSlipHideOffsets[i];
        mHungerSlipPosTargetOffset[i] = mHungerSlipPosOffset[i];
        mHungerSlipWiggleTime[i] = 0;
        }
    mHungerSlipVisible = -1;

    mSayField.setText( "" );
    mSayField.unfocus();
    TextField::unfocusAll();
    
    mOldArrows.deleteAll();
    mOldDesStrings.deallocateStringElements();
    mOldDesFades.deleteAll();

    mOldLastAteStrings.deallocateStringElements();
    mOldLastAteFillMax.deleteAll();
    mOldLastAteFades.deleteAll();
    mOldLastAteBarFades.deleteAll();
    
    mYumBonus = 0;
    mOldYumBonus.deleteAll();
    mOldYumBonusFades.deleteAll();
    
    mYumMultiplier = 0;
    
    
    for( int i=0; i<NUM_YUM_SLIPS; i++ ) {
        mYumSlipPosOffset[i] = mYumSlipHideOffset[i];
        mYumSlipPosTargetOffset[i] = mYumSlipHideOffset[i];
        mYumSlipNumberToShow[i] = 0;
        }
    

    mCurrentArrowI = 0;
    mCurrentArrowHeat = -1;
    if( mCurrentDes != NULL ) {
        delete [] mCurrentDes;
        }
    mCurrentDes = NULL;

    if( mCurrentLastAteString != NULL ) {
        delete [] mCurrentLastAteString;
        }
    mCurrentLastAteString = NULL;


    lastScreenViewCenter.x = 0;
    lastScreenViewCenter.y = 0;
    
    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );
    
    mPageStartTime = game_getCurrentTime();
    
    setWaiting( true, false );

    readyPendingReceivedMessages.deallocateStringElements();
    serverFrameMessages.deallocateStringElements();

    if( pendingMapChunkMessage != NULL ) {
        delete [] pendingMapChunkMessage;
        pendingMapChunkMessage = NULL;
        }
    pendingCMData = false;
    

    clearLiveObjects();
    mFirstServerMessagesReceived = 0;
    mStartedLoadingFirstObjectSet = false;
    mDoneLoadingFirstObjectSet = false;
    mFirstObjectSetLoadingProgress = 0;

    playerActionPending = false;
    
    waitingForPong = false;
    lastPingSent = 0;
    lastPongReceived = 0;
    

    serverSocketBuffer.deleteAll();

    if( nextActionMessageToSend != NULL ) {    
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }
    

    for( int j=0; j<2; j++ ) {
        for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
            mHomeArrowStates[j][i].solid = false;
            mHomeArrowStates[j][i].fade = 0;
            }
        }
    }







void LivingLifePage::checkForPointerHit( PointerHitRecord *inRecord,
                                         float inX, float inY ) {
    
    LiveObject *ourLiveObject = getOurLiveObject();

    double minDistThatHits = 2.0;

    int clickDestX = lrintf( ( inX ) / CELL_D );
    
    int clickDestY = lrintf( ( inY ) / CELL_D );
    
    float clickExtraX = inX - clickDestX * CELL_D;
    float clickExtraY = inY - clickDestY * CELL_D;

    PointerHitRecord *p = inRecord;
    
    p->closestCellX = clickDestX;
    p->closestCellY = clickDestY;

    if( mForceGroundClick ) {
        return;
        }
    
    int clickDestMapX = clickDestX - mMapOffsetX + mMapD / 2;
    int clickDestMapY = clickDestY - mMapOffsetY + mMapD / 2;
    
    int clickDestMapI = clickDestMapY * mMapD + clickDestMapX;
    
    if( clickDestMapY >= 0 && clickDestMapY < mMapD &&
        clickDestMapX >= 0 && clickDestMapX < mMapD ) {
                
        int oID = mMap[ clickDestMapI ];

        if( ! ourLiveObject->inMotion 
            &&
            // don't allow them to mouse over clicked cell after click
            // until they mouse out and back in again
            ( clickDestMapX != mLastClickCell.x ||
              clickDestMapY != mLastClickCell.y )  
            &&
            ( clickDestMapX != mCurMouseOverCell.x ||
              clickDestMapY != mCurMouseOverCell.y ) ) {

            GridPos newPos = { clickDestMapX, clickDestMapY };

            float oldFade = -0.8f;
            
            for( int i=0; i<mPrevMouseOverCells.size(); i++ ) {
                GridPos old = mPrevMouseOverCells.getElementDirect( i );
            
                if( equal( old, mCurMouseOverCell ) ) {
                    
                    mPrevMouseOverCells.deleteElement( i );
                    mPrevMouseOverCellFades.deleteElement( i );
                    }
                else if( equal( old, newPos ) ) {
                    oldFade = mPrevMouseOverCellFades.getElementDirect( i );

                    mPrevMouseOverCells.deleteElement( i );
                    mPrevMouseOverCellFades.deleteElement( i );
                    }
                }

            mPrevMouseOverCells.push_back( mCurMouseOverCell );
            mPrevMouseOverCellFades.push_back( mCurMouseOverCellFade );

            mCurMouseOverCell.x = clickDestMapX;
            mCurMouseOverCell.y = clickDestMapY;
            
            // moused somewhere new, clear last click
            mLastClickCell.x = -1;
            mLastClickCell.y = -1;
            

            mCurMouseOverCellFadeRate = 0.04;
            
            if( oldFade < 0 && oID > 0 ) {
                // show cell instantly when mousing over and occupied space
                oldFade = 0;
                mCurMouseOverCellFadeRate = 0.2;
                }
            
            mCurMouseOverCellFade = oldFade;
            }
        
        
        // check this cell first

        // all short objects are mouse-through-able
        
        if( oID > 0 && 
            getObjectHeight( oID ) < CELL_D ) {
            ObjectRecord *obj = getObject( oID );
            

            float thisObjClickExtraX = clickExtraX;
            float thisObjClickExtraY = clickExtraY;
            
            if( mMapMoveSpeeds[ clickDestMapI ] > 0 ) {
                thisObjClickExtraX -= 
                    mMapMoveOffsets[ clickDestMapI ].x * CELL_D;
                thisObjClickExtraY -= 
                    mMapMoveOffsets[ clickDestMapI ].y * CELL_D;
                }
            
            int sp, cl, sl;
                
            double dist = getClosestObjectPart( 
                obj,
                NULL,
                &( mMapContainedStacks[clickDestMapI] ),
                NULL,
                false,
                -1,
                -1,
                mMapTileFlips[ clickDestMapI ],
                thisObjClickExtraX,
                thisObjClickExtraY,
                &sp, &cl, &sl,
                // ignore transparent parts
                // allow objects behind smoke to be picked up
                false );
            
            if( dist < minDistThatHits ) {
                p->hitOurPlacement = true;
                p->closestCellX = clickDestX;
                p->closestCellY = clickDestY;
                
                p->hitSlotIndex = sl;
                
                p->hitAnObject = true;
                p->hitObjectID = oID;
                }
            }
        }
    

    // need to check 3 around cell in all directions because
    // of moving object offsets

    // start in front row
    // right to left
    // (check things that are in front first
    for( int y=clickDestY-3; y<=clickDestY+3 && ! p->hit; y++ ) {
        float clickOffsetY = ( clickDestY  - y ) * CELL_D + clickExtraY;

        // first all non drawn-behind map objects in this row
        // (in front of people)
        for( int x=clickDestX+3; x>=clickDestX-3  && 
                 ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;

            int mapX = x - mMapOffsetX + mMapD / 2;
            int mapY = y - mMapOffsetY + mMapD / 2;
            
            if( mapY < 0 || mapY >= mMapD || 
                mapX < 0 || mapX >= mMapD ) { 
                // skip any that are out of map bounds
                continue;
                }
            

            int mapI = mapY * mMapD + mapX;

            int oID = mMap[ mapI ];
            
            

            if( oID > 0 &&
                ! getObject( oID )->drawBehindPlayer ) {
                ObjectRecord *obj = getObject( oID );
                
                float thisObjClickOffsetX = clickOffsetX;
                float thisObjClickOffsetY = clickOffsetY;
            
                if( mMapMoveSpeeds[ mapI ] > 0 ) {
                    thisObjClickOffsetX -= 
                        mMapMoveOffsets[ mapI ].x * CELL_D;
                    thisObjClickOffsetY -= 
                        mMapMoveOffsets[ mapI ].y * CELL_D;
                    }


                int sp, cl, sl;
                
                double dist = getClosestObjectPart( 
                    obj,
                    NULL,
                    &( mMapContainedStacks[mapI] ),
                    NULL,
                    false,
                    -1,
                    -1,
                    mMapTileFlips[ mapI ],
                    thisObjClickOffsetX,
                    thisObjClickOffsetY,
                    &sp, &cl, &sl,
                    // ignore transparent parts
                    // allow objects behind smoke to be picked up
                    false );
                
                if( dist < minDistThatHits ) {
                    p->hit = true;
                    
                    // already hit a short object
                    // AND this object is tall
                    // (don't click through short behind short)
                    if( p->hitOurPlacement &&
                        getObjectHeight( oID ) > .75 * CELL_D ) {
                        
                        if( p->closestCellY > y ) {
                            p->hitOurPlacementBehind = true;
                            }
                        }
                    else {
                        p->closestCellX = x;
                        p->closestCellY = y;
                        
                        p->hitSlotIndex = sl;

                        p->hitAnObject = true;
                        p->hitObjectID = oID;
                        }
                    }
                }
            }

        // don't worry about p->hitOurPlacement when checking them
        // next, people in this row
        // recently dropped babies are in front and tested first
        if( ! mXKeyDown )
        for( int d=0; d<2 && ! p->hit; d++ )
        for( int x=clickDestX+1; x>=clickDestX-1 && ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;
            
            for( int i=gameObjects.size()-1; i>=0 && ! p->hit; i-- ) {
        
                LiveObject *o = gameObjects.getElement( i );

                if( o->outOfRange ) {
                    // out of range, but this was their last known position
                    // don't draw now
                    continue;
                    }
                
                if( o->heldByAdultID != -1 ) {
                    // held by someone else, don't draw now
                    continue;
                    }

                if( d == 1 &&
                    ( o->heldByDropOffset.x != 0 ||
                      o->heldByDropOffset.y != 0 ) ) {
                    // recently dropped baby, skip
                    continue;
                    }
                else if( d == 0 &&
                         o->heldByDropOffset.x == 0 &&
                         o->heldByDropOffset.y == 0 ) {
                    // not a recently-dropped baby, skip
                    continue;
                    }
                
                
                double oX = o->xd;
                double oY = o->yd;
                
                if( o->currentSpeed != 0 && o->pathToDest != NULL ) {
                    oX = o->currentPos.x;
                    oY = o->currentPos.y;
                    }
                
                
                if( round( oX ) == x &&
                    round( oY ) == y ) {
                                        
                    // here!

                    double personClickOffsetX = ( oX - x ) * CELL_D;
                    double personClickOffsetY = ( oY - y ) * CELL_D;
                    
                    personClickOffsetX = clickOffsetX - personClickOffsetX;
                    personClickOffsetY = clickOffsetY - personClickOffsetY;

                    if( computeCurrentAgeNoOverride( o ) < noMoveAge ) {
                        // laying on ground
                        
                        // rotate
                        doublePair c = { personClickOffsetX, 
                                         personClickOffsetY };
                        c = rotate( c, 0.25 * 2 * M_PI );
                        
                        personClickOffsetX = c.x;
                        personClickOffsetY = c.y;
                        
                        // offset
                        personClickOffsetY += 32;
                        }


                    ObjectRecord *obj = getObject( o->displayID );
                    
                    int sp, cl, sl;
                    
                    double dist = getClosestObjectPart( 
                        obj,
                        &( o->clothing ),
                        NULL,
                        o->clothingContained,
                        false,
                        computeCurrentAge( o ),
                        -1,
                        o->holdingFlip,
                        personClickOffsetX,
                        personClickOffsetY,
                        &sp, &cl, &sl );
                    
                    if( dist < minDistThatHits ) {
                        p->hit = true;
                        p->closestCellX = x;
                        p->closestCellY = y;
                        if( o == ourLiveObject ) {
                            p->hitSelf = true;

                            }
                        else {
                            p->hitOtherPerson = true;
                            p->hitOtherPersonID = o->id;
                            }
                        if( cl != -1 ) {
                            p->hitClothingIndex = cl;
                            p->hitSlotIndex = sl;
                            }
                        }
                    }
                }
            
            }
        
        // now drawn-behind objects in this row

        for( int x=clickDestX+1; x>=clickDestX-1  
                 && ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;

            int mapX = x - mMapOffsetX + mMapD / 2;
            int mapY = y - mMapOffsetY + mMapD / 2;

            if( mapY < 0 || mapY >= mMapD || 
                mapX < 0 || mapX >= mMapD ) { 
                // skip any that are out of map bounds
                continue;
                }

            int mapI = mapY * mMapD + mapX;

            int oID = mMap[ mapI ];
            
            

            if( oID > 0 &&
                getObject( oID )->drawBehindPlayer ) {
                ObjectRecord *obj = getObject( oID );
                

                float thisObjClickOffsetX = clickOffsetX;
                float thisObjClickOffsetY = clickOffsetY;
            
                if( mMapMoveSpeeds[ mapI ] > 0 ) {
                    thisObjClickOffsetX -= 
                        mMapMoveOffsets[ mapI ].x * CELL_D;
                    thisObjClickOffsetY -= 
                        mMapMoveOffsets[ mapI ].y * CELL_D;
                    }


                int sp, cl, sl;
                
                double dist = getClosestObjectPart( 
                    obj,
                    NULL,
                    &( mMapContainedStacks[mapI] ),
                    NULL,
                    false,
                    -1,
                    -1,
                    mMapTileFlips[ mapI ],
                    thisObjClickOffsetX,
                    thisObjClickOffsetY,
                    &sp, &cl, &sl,
                    // ignore transparent parts
                    // allow objects behind smoke to be picked up
                    false );
                
                if( dist < minDistThatHits ) {
                    p->hit = true;
                    
                                        
                    // already hit a short object
                    // AND this object is tall
                    // (don't click through short behind short)
                    if( p->hitOurPlacement &&
                        getObjectHeight( oID ) > .75 * CELL_D ) {
                        
                        if( p->closestCellY > y ) {
                            p->hitOurPlacementBehind = true;
                            }
                        }
                    else {
                        p->closestCellX = x;
                        p->closestCellY = y;
                        
                        p->hitSlotIndex = sl;
                        
                        p->hitAnObject = true;
                        p->hitObjectID = oID;                
                        }
                    }
                }
            }
        }

    if( p->hitOurPlacement &&
        ( p->hitOtherPerson || p->hitSelf ) ) {
        p->hitAnObject = false;
        }
    else {
        // count it now, if we didn't hit a person that blocks it
        p->hit = true;
        }




    if( !mXKeyDown )
    if( p->hit && p->hitAnObject && ! p->hitOtherPerson && ! p->hitSelf ) {
        // hit an object
        
        // what if someone is standing behind it?
        
        // look at two cells above
        for( int y= p->closestCellY + 1; 
             y < p->closestCellY + 3 && ! p->hitOtherPerson; y++ ) {
            
            float clickOffsetY = ( clickDestY  - y ) * CELL_D + clickExtraY;
        
            // look one cell to left/right
            for( int x= p->closestCellX - 1; 
                 x < p->closestCellX + 2 && ! p->hitOtherPerson; x++ ) {

                float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;

                for( int i=gameObjects.size()-1; 
                     i>=0 && ! p->hitOtherPerson; i-- ) {
        
                    LiveObject *o = gameObjects.getElement( i );
                    
                    if( o->outOfRange ) {
                        // out of range, but this was their last known position
                        // don't draw now
                        continue;
                        }
                    
                    if( o->heldByAdultID != -1 ) {
                        // held by someone else, don't draw now
                        continue;
                        }
                    
                    if( o->heldByDropOffset.x != 0 ||
                        o->heldByDropOffset.y != 0 ) {
                        // recently dropped baby, skip
                        continue;
                        }
                
                    if( o == ourLiveObject ) {
                        // ignore clicks on self behind tree
                        continue;
                        }
                    
                    
                    int oX = o->xd;
                    int oY = o->yd;
                
                    if( o->currentSpeed != 0 && o->pathToDest != NULL ) {
                        if( o->onFinalPathStep ) {
                            oX = o->pathToDest[ o->pathLength - 1 ].x;
                            oY = o->pathToDest[ o->pathLength - 1 ].y;
                            }
                        else {
                            oX = o->pathToDest[ o->currentPathStep ].x;
                            oY = o->pathToDest[ o->currentPathStep ].y;
                            }
                        }
            
                    
                    if( oY == y && oX == x ) {
                        // here!
                        ObjectRecord *obj = getObject( o->displayID );
                    
                        int sp, cl, sl;
                        
                        double dist = getClosestObjectPart( 
                            obj,
                            &( o->clothing ),
                            NULL,
                            o->clothingContained,
                            false,
                            computeCurrentAge( o ),
                            -1,
                            o->holdingFlip,
                            clickOffsetX,
                            clickOffsetY,
                            &sp, &cl, &sl );
                    
                        if( dist < minDistThatHits ) {
                            p->hit = true;
                            p->closestCellX = x;
                            p->closestCellY = y;
                            
                            p->hitAnObject = false;
                            p->hitOtherPerson = true;
                            p->hitOtherPersonID = o->id;
                            
                            if( cl != -1 ) {
                                p->hitClothingIndex = cl;
                                p->hitSlotIndex = sl;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    


        





void LivingLifePage::pointerMove( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
        
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );

    if( showBugMessage ) {
        return;
        }

    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    PointerHitRecord p;
    
    p.hitSlotIndex = -1;
    

    p.hit = false;
    p.hitSelf = false;
    p.hitOurPlacement = false;
    p.hitOurPlacementBehind = false;
    
    p.hitClothingIndex = -1;
    

    // when we click in a square, only count as hitting something
    // if we actually clicked the object there.  Else, we can walk
    // there if unblocked.
    p.hitAnObject = false;

    p.hitOtherPerson = false;

    checkForPointerHit( &p, inX, inY );

    mCurMouseOverPerson = p.hitOtherPerson || p.hitSelf;
    
    
    int clickDestX = p.closestCellX;
    int clickDestY = p.closestCellY;
    

    int destID = 0;
    int destBiome = -1;
    int destFloor = 0;
    
    int mapX = clickDestX - mMapOffsetX + mMapD / 2;
    int mapY = clickDestY - mMapOffsetY + mMapD / 2;
    if( mapY >= 0 && mapY < mMapD &&
        mapX >= 0 && mapX < mMapD ) {
        
        if( p.hitAnObject ) {
            destID = mMap[ mapY * mMapD + mapX ];
            }
        
        destBiome = mMapBiomes[ mapY * mMapD + mapX ];
        destFloor = mMapFloors[ mapY * mMapD + mapX ];
        }



    if( mCurMouseOverID > 0 
        && 
        ! mCurMouseOverSelf
        &&
        ( mCurMouseOverID != destID
          ||
          mCurMouseOverSpot.x != mapX ||
          mCurMouseOverSpot.y != mapY ) ) {
        
        GridPos prev = { mCurMouseOverSpot.x, mCurMouseOverSpot.y };
        
        for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
            GridPos old = mPrevMouseOverSpots.getElementDirect( i );
            
            if( equal( old, prev ) ) {
                mPrevMouseOverSpots.deleteElement( i );
                mPrevMouseOverSpotFades.deleteElement( i );
                mPrevMouseOverSpotsBehind.deleteElement( i );
                break;
                }
            }
        
        mPrevMouseOverSpots.push_back( prev );
        mPrevMouseOverSpotFades.push_back( mCurMouseOverFade );
        mPrevMouseOverSpotsBehind.push_back( mCurMouseOverBehind );
        }

    char overNothing = true;

    LiveObject *ourLiveObject = getOurLiveObject();

    ourLiveObject->currentMouseOverClothingIndex = -1;
    
    if( destID == 0 ) {
        if( p.hitSelf ) {
            mCurMouseOverSelf = true;
            
            // clear when mousing over bare parts of body
            // show YOU
            mCurMouseOverID = -99;
            mCurMouseOverBiome = -1;
            
            overNothing = false;
            
            
            if( p.hitClothingIndex != -1 ) {
                if( p.hitSlotIndex != -1 ) {
                    mCurMouseOverID = 
                        ourLiveObject->clothingContained[ p.hitClothingIndex ].
                        getElementDirect( p.hitSlotIndex );
                    }
                else {
                    ObjectRecord *c = 
                        clothingByIndex( ourLiveObject->clothing, 
                                         p.hitClothingIndex );
                    mCurMouseOverID = c->id;
                    }
                
                ourLiveObject->currentMouseOverClothingIndex =
                    p.hitClothingIndex;
                }
            }
        if( p.hitOtherPerson ) {
            // store negative in place so that we can show their relation
            // string
            mCurMouseOverID = - p.hitOtherPersonID;
            mCurMouseOverBiome = -1;
            }
        }
    
    if( destID == 0 && mCurMouseOverID == 0 ) {
        // show biome anyway
        mCurMouseOverBiome = destBiome;
        }

    if( destFloor != 0 ) {
        mCurMouseOverBiome = -1;
        }


    if( destID > 0 ) {
        mCurMouseOverSelf = false;
        
        if( ( destID != mCurMouseOverID ||
              mCurMouseOverSpot.x != mapX ||
              mCurMouseOverSpot.y != mapY  ) ) {
            
            // start new fade-in
            mCurMouseOverFade = 0;
            }

        mCurMouseOverID = destID;
        if( destFloor == 0 ) {
            mCurMouseOverBiome = destBiome;
            }
        
        overNothing = false;
        
        if( p.hitSlotIndex != -1 ) {
            mCurMouseOverID = 
                mMapContainedStacks[ mapY * mMapD + mapX ].
                getElementDirect( p.hitSlotIndex );
            }
        
        
        mCurMouseOverSpot.x = mapX;
        mCurMouseOverSpot.y = mapY;

        mCurMouseOverWorld.x = clickDestX;
        mCurMouseOverWorld.y = clickDestY;
        
        mCurMouseOverBehind = p.hitOurPlacementBehind;
        

        // check if we already have a partial-fade-out for this cell
        // from previous mouse-overs
        for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
            GridPos old = mPrevMouseOverSpots.getElementDirect( i );
            
            if( equal( old, mCurMouseOverSpot ) ) {
                mCurMouseOverFade = 
                    mPrevMouseOverSpotFades.getElementDirect( i );
                
                mPrevMouseOverSpots.deleteElement( i );
                mPrevMouseOverSpotFades.deleteElement( i );
                mPrevMouseOverSpotsBehind.deleteElement( i );
                break;
                }
            }
        }
    
    
    if( overNothing && mCurMouseOverID != 0 ) {
        mLastMouseOverID = mCurMouseOverID;
        
        if( mCurMouseOverSelf ) {
            // don't keep YOU or EAT or clothing tips around after we mouse 
            // outside of them
            mLastMouseOverID = 0;
            }
        mCurMouseOverSelf = false;
        
        mCurMouseOverID = 0;
        mCurMouseOverBehind = false;
        mLastMouseOverFade = 1.0f;
        }
    
    
    double worldX = inX / (double)CELL_D;
    

    if( ! ourLiveObject->inMotion &&
        // watch for being stuck with no-move object in hand, like fishing
        // pole
        ( ourLiveObject->holdingID <= 0 ||
          getObject( ourLiveObject->holdingID )->speedMult > 0 ) ) {
        char flip = false;
        
        if( ourLiveObject->holdingFlip &&
            worldX > ourLiveObject->currentPos.x + 0.5 ) {
            ourLiveObject->holdingFlip = false;
            flip = true;
            }
        else if( ! ourLiveObject->holdingFlip &&
                 worldX < ourLiveObject->currentPos.x - 0.5 ) {
            ourLiveObject->holdingFlip = true;
            flip = true;
            }

        if( flip ) {
            ourLiveObject->lastAnim = moving;
            ourLiveObject->curAnim = ground2;
            ourLiveObject->lastAnimFade = 1;
            
            ourLiveObject->lastHeldAnim = moving;
            ourLiveObject->curHeldAnim = held;
            ourLiveObject->lastHeldAnimFade = 1;
            }
        }
    
    
    }




char LivingLifePage::getCellBlocksWalking( int inMapX, int inMapY ) {
    
    if( inMapY >= 0 && inMapY < mMapD &&
        inMapX >= 0 && inMapX < mMapD ) {
        
        int destID = mMap[ inMapY * mMapD + inMapX ];
        
        
        if( destID > 0 && getObject( destID )->blocksWalking ) {
            return true;
            }
        else {
            // check for wide neighbors
            
            int r = getMaxWideRadius();
            
            int startX = inMapX - r;
            int endX = inMapX + r;
            
            if( startX < 0 ) {
                startX = 0;
                }
            if( endX >= mMapD ) {
                endX = mMapD - 1;
                }
            for( int x=startX; x<=endX; x++ ) {
                int nID = mMap[ inMapY * mMapD + x ];

                if( nID > 0 ) {
                    ObjectRecord *nO = getObject( nID );
                    
                    if( nO->blocksWalking && nO->wide &&
                        ( x - inMapX <= nO->rightBlockingRadius || 
                          inMapX - x >= nO->leftBlockingRadius ) ) {
                        return true;
                        }
                    }
                }
            }
        
        return false;
        }
    else {
        // off map blocks
        return true;
        }
    }





static int savedXD = 0;
static int savedYD = 0;
static char savedInMotion = false;
static GridPos *savedPathToDest = NULL;
static int savedPathLength = 0;
static int savedCurrentPathStep = 0;
static char savedOnFinalPathStep = false;
static int savedNumFramesOnCurrentStep = 0;
static char savedDestTruncated = false;
static doublePair savedCurrentMoveDirection = { 0, 0 };


// saves path
// restore or free calls must be used to free memory
static void savePlayerPath( LiveObject *inObject ) {
    
    if( inObject->pathToDest != NULL ) {
        savedXD = inObject->xd;
        savedYD = inObject->yd;
        savedInMotion = inObject->inMotion;
        
        savedPathLength = inObject->pathLength;
        savedPathToDest = new GridPos[ savedPathLength ];

        memcpy( savedPathToDest, inObject->pathToDest,
                sizeof( GridPos ) * inObject->pathLength );
        savedCurrentPathStep = inObject->currentPathStep;
        savedOnFinalPathStep = inObject->onFinalPathStep;
        savedNumFramesOnCurrentStep = inObject->numFramesOnCurrentStep;
        savedDestTruncated = inObject->destTruncated;
        savedCurrentMoveDirection = inObject->currentMoveDirection;
        }
    }


static void restoreSavedPath( LiveObject *inObject ) {

    if( inObject->pathToDest != NULL ) {
        delete [] inObject->pathToDest;
        }
    
    inObject->xd = savedXD;
    inObject->yd = savedYD;
    inObject->inMotion = savedInMotion;
    inObject->pathToDest = savedPathToDest;
    inObject->pathLength = savedPathLength;
    inObject->currentPathStep = savedCurrentPathStep;
    inObject->onFinalPathStep = savedOnFinalPathStep;
    inObject->numFramesOnCurrentStep = savedNumFramesOnCurrentStep;
    inObject->destTruncated = savedDestTruncated;
    inObject->currentMoveDirection = savedCurrentMoveDirection;
    }


static void freeSavedPath() {
    if( savedPathToDest != NULL ) {
        delete [] savedPathToDest;
        savedPathToDest = NULL;
        }
    }







void LivingLifePage::pointerDown( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;

    if( showBugMessage ) {
        return;
        }
    
    if( mServerSocket == -1 ) {
        // dead
        return;
        }
    
    if( vogMode ) {
        return;
        }

    char modClick = false;
    
    if( ( mEKeyDown && mEKeyEnabled ) || isLastMouseButtonRight() ) {
        modClick = true;
        }
    
    mLastMouseOverID = 0;
    
    // detect cases where mouse is held down already
    // this is for long-distance player motion, and we don't want
    // this to result in an action by accident if they mouse over
    // something actionable along the way
    char mouseAlreadyDown = mouseDown;
    
    mouseDown = true;
    if( !mouseAlreadyDown ) {
        mouseDownFrames = 0;
        }
    
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );
    
    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    if( playerActionPending ) {
        printf( "Skipping click, action pending\n" );
        
        // block further actions until update received to confirm last
        // action
        return;
        }
    

    LiveObject *ourLiveObject = getOurLiveObject();
    
    if( ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->speedMult == 0 ) {
        // holding something that stops movement entirely, ignore click
        
        TransRecord *groundTrans = getTrans( ourLiveObject->holdingID, -1 );
        
        if( groundTrans == NULL ) {

            printf( "Skipping click, holding 0-speed object "
                    "that can't be used on ground\n" );
            return;
            }
        }
    

    if( ourLiveObject->heldByAdultID != -1 ) {
        // click from a held baby

        // no longer limiting frequency of JUMP messages
        // limit them by previous wiggle animation ending
        // this makes the visuals on the parent and child client as close
        // as possible
        if( ! ourLiveObject->babyWiggle ) {
            // send new JUMP message instead of ambigous MOVE message
            sendToServerSocket( (char*)"JUMP 0 0#" );
            
            // start new wiggle
            ourLiveObject->babyWiggle = true;
            ourLiveObject->babyWiggleProgress = 0;
            }
        
        
        return;
        }
    else if( computeCurrentAgeNoOverride( ourLiveObject ) < noMoveAge ) {
        // to young to even move
        // ignore click

        printf( "Skipping click, too young to move\n" );
        
        // flip and animate to at least register click
        ourLiveObject->holdingFlip = ! ourLiveObject->holdingFlip;
        
        addNewAnimPlayerOnly( ourLiveObject, moving );
        addNewAnimPlayerOnly( ourLiveObject, ground );
        
        // JUMP message also tells server we're wiggling on the ground
        sendToServerSocket( (char*)"JUMP 0 0#" );

        return;
        }
    

    // prepare for various calls to computePathToDest below
    findClosestPathSpot( ourLiveObject );
    
    

    // consider 3x4 area around click and test true object pixel
    // collisions in that area
    PointerHitRecord p;
    
    p.hitSlotIndex = -1;
    
    p.hit = false;
    p.hitSelf = false;
    p.hitOurPlacement = false;
    p.hitOurPlacementBehind = false;
    
    p.hitClothingIndex = -1;
    

    // when we click in a square, only count as hitting something
    // if we actually clicked the object there.  Else, we can walk
    // there if unblocked.
    p.hitAnObject = false;
    
    p.hitOtherPerson = false;
    

    checkForPointerHit( &p, inX, inY );



    // new semantics
    // as soon as we trigger a kill attempt, we go into kill mode
    // by sending server a KILL message right away
    char killMode = false;
 

    // don't allow weapon-drop on kill-click unless there's really
    // no one around
    if( ! mouseAlreadyDown &&
        modClick && ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->deadlyDistance > 0 &&
        isShiftKeyDown() &&
        ! p.hitOtherPerson ) {
        
        // everything good to go for a kill-click, but they missed
        // hitting someone (and maybe they clicked on an object instead)


        // make sure they didn't target some animal that their weapon
        // can work on
        if( p.hitAnObject && p.hitObjectID > 0 &&
            getTrans( ourLiveObject->holdingID, p.hitObjectID ) != NULL ) {
            
            // a transition exists for this weapon on the destination
            // object
            }
        else {
            // clicked on nothing relevant

            // find closest person for them to hit
        
            doublePair clickPos = { inX, inY };
        
        
            int closePersonID = -1;
            double closeDistance = DBL_MAX;
        
            for( int i=gameObjects.size()-1; i>=0; i-- ) {
        
                LiveObject *o = gameObjects.getElement( i );

                if( o->id == ourID ) {
                    // don't consider ourself as a kill target
                    continue;
                    }
            
                if( o->outOfRange ) {
                    // out of range, but this was their last known position
                    // don't draw now
                    continue;
                    }
            
                if( o->heldByAdultID != -1 ) {
                    // held by someone else, can't click on them
                    continue;
                    }
            
                if( o->heldByDropOffset.x != 0 ||
                    o->heldByDropOffset.y != 0 ) {
                    // recently dropped baby, skip
                    continue;
                    }
                
                
                double oX = o->xd;
                double oY = o->yd;
                
                if( o->currentSpeed != 0 && o->pathToDest != NULL ) {
                    oX = o->currentPos.x;
                    oY = o->currentPos.y;
                    }

                oY *= CELL_D;
                oX *= CELL_D;
            

                // center of body up from feet position in tile
                oY += CELL_D / 2;
            
                doublePair oPos = { oX, oY };
            

                double thisDistance = distance( clickPos, oPos );
            
                if( thisDistance < closeDistance ) {
                    closeDistance = thisDistance;
                    closePersonID = o->id;
                    }
                }

            if( closePersonID != -1 && closeDistance < 4 * CELL_D ) {
                // somewhat close to clicking on someone
                p.hitOtherPerson = true;
                p.hitOtherPersonID = closePersonID;
                p.hitAnObject = false;
                p.hit = true;
                killMode = true;
                }
            }
        }






    
    mCurMouseOverPerson = p.hitOtherPerson || p.hitSelf;

    // don't allow clicking on object during continued motion
    if( mouseAlreadyDown ) {
        p.hit = false;
        p.hitSelf = false;
        p.hitOurPlacement = false;
        p.hitAnObject = false;
        p.hitOtherPerson = false;
        
        
        // also, use direct tile clicking
        // (don't remap clicks on the top of tall objects down to the base tile)
        p.closestCellX = lrintf( ( inX ) / CELL_D );
    
        p.closestCellY = lrintf( ( inY ) / CELL_D );
        }
    
    // clear mouse over cell
    mPrevMouseOverCells.push_back( mCurMouseOverCell );
    mPrevMouseOverCellFades.push_back( mCurMouseOverCellFade );
    
    if( mCurMouseOverCell.x != -1 ) {    
        mLastClickCell = mCurMouseOverCell;
        }

    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;

    
    int clickDestX = p.closestCellX;
    int clickDestY = p.closestCellY;

    int destID = 0;
    int floorDestID = 0;
    
    int destObjInClickedTile = 0;
    char destObjInClickedTilePermanent = false;
    
    int destNumContained = 0;
    
    int mapX = clickDestX - mMapOffsetX + mMapD / 2;
    int mapY = clickDestY - mMapOffsetY + mMapD / 2;    
    
    
    if( mouseAlreadyDown && 
        mouseDownFrames >  
        minMouseDownFrames / frameRateFactor ) {
        
        // continuous movement mode

        // watch out for case where they mouse over a blocked spot by accident
        
        if( getCellBlocksWalking( mapX, mapY ) ) {
            printf( "Blocked at cont move click dest %d,%d\n",
                    clickDestX, clickDestY );
            
            double xDelta = clickDestX - ourLiveObject->currentPos.x;
            double yDelta = clickDestY - ourLiveObject->currentPos.y;

            int step = 1;
            int limit = 10;
                
            if( fabs( xDelta ) > fabs( yDelta ) ) {
                // push out further in x direction
                if( xDelta < 0 ) {
                    step = -step;
                    limit = -limit;
                    }

                for( int xd=step; xd != limit; xd += step ) {
                    if( ! getCellBlocksWalking( mapX + xd, mapY ) ) {
                        // found
                        clickDestX += xd;
                        break;
                        }
                    }
                }
            else {
                // push out further in y direction
                if( yDelta < 0 ) {
                    step = -step;
                    limit = -limit;
                    }

                for( int yd=step; yd != limit; yd += step ) {
                    if( ! getCellBlocksWalking( mapX, mapY + yd ) ) {
                        // found
                        clickDestY += yd;
                        break;
                        }
                    }
                }
            
            printf( "Pushing out to click dest %d,%d\n",
                    clickDestX, clickDestY );
            
            // recompute
            mapX = clickDestX - mMapOffsetX + mMapD / 2;
            mapY = clickDestY - mMapOffsetY + mMapD / 2;
            }
        }
    

    printf( "clickDestX,Y = %d, %d,  mapX,Y = %d, %d, curX,Y = %d, %d\n", 
            clickDestX, clickDestY, 
            mapX, mapY,
            ourLiveObject->xd, ourLiveObject->yd );
    if( ! killMode && 
        mapY >= 0 && mapY < mMapD &&
        mapX >= 0 && mapX < mMapD ) {
        
        destID = mMap[ mapY * mMapD + mapX ];
        floorDestID = mMapFloors[ mapY * mMapD + mapX ];
        
        destObjInClickedTile = destID;

        if( destObjInClickedTile > 0 ) {
            destObjInClickedTilePermanent =
                getObject( destObjInClickedTile )->permanent;
            }
    
        destNumContained = mMapContainedStacks[ mapY * mMapD + mapX ].size();
        

        // if holding something, and this is a set-down action
        // show click reaction
        if( modClick &&
            ourLiveObject->holdingID != 0 ) {
        
            int mapI = mapY * mMapD + mapX;
            
            int id = mMap[mapI];
            
            if( id == 0 || ! getObject( id )->permanent ) {
                
                // empty cell, or something we can swap held with
                
                GridPos clickPos = { mapX, mapY };
                
                mPrevMouseClickCells.push_back( clickPos );
                mPrevMouseClickCellFades.push_back( 1 );
                
                // instantly fade current cell to get it out of the way
                // of our click indicator
                mCurMouseOverCellFade = 0;
                }
            
            }
        }



    nextActionEating = false;
    nextActionDropping = false;
    


    if( p.hitSelf ) {
        // click on self

        // ignore unless it's a use-on-self action and standing still

        if( ! ourLiveObject->inMotion ) {
            
            if( nextActionMessageToSend != NULL ) {
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }

            if( !modClick || p.hitClothingIndex == -1 ) {
                
                if( ourLiveObject->holdingID > 0 &&
                    getObject( ourLiveObject->holdingID )->foodValue > 0 ) {
                    nextActionEating = true;

                    if( p.hitClothingIndex >= 0 &&
                        clothingByIndex( ourLiveObject->clothing,
                                         p.hitClothingIndex )->numSlots > 0 ) {
                        // clicked on container clothing
                        // server interprets this as an insert request, not
                        // an eat request
                        nextActionEating = false;
                        }
                    }
    
                nextActionMessageToSend = 
                    autoSprintf( "SELF %d %d %d#",
                                 sendX( clickDestX ), sendY( clickDestY ), 
                                 p.hitClothingIndex );
                printf( "Use on self\n" );
                }
            else {
                // modclick on hit clothing

                if( ourLiveObject->holdingID > 0 ) {
                    nextActionMessageToSend = 
                        autoSprintf( "DROP %d %d %d#",
                                     sendX( clickDestX ), sendY( clickDestY ), 
                                     p.hitClothingIndex  );
                    nextActionDropping = true;
                    printf( "Add to own clothing container\n" );
                    }
                else {
                    nextActionMessageToSend = 
                        autoSprintf( "SREMV %d %d %d %d#",
                                     sendX( clickDestX ), sendY( clickDestY ), 
                                     p.hitClothingIndex,
                                     p.hitSlotIndex );
                    printf( "Remove from own clothing container\n" );
                    }
                }

            playerActionTargetX = clickDestX;
            playerActionTargetY = clickDestY;
            
            }
        

        return;
        }
    
    
    // may change to empty adjacent spot to click
    int moveDestX = clickDestX;
    int moveDestY = clickDestY;
    
    char mustMove = false;


    


    if( destID > 0 && p.hitAnObject ) {
        
        if( ourLiveObject->holdingID > 0 ) {
            TransRecord *tr = getTrans( ourLiveObject->holdingID, destID );
            
            if( tr == NULL ) {
                // try defaul transition
                // tr = getTrans( -2, destID );
                
                // for now, DO NOT consider default transition
                // no main transition for this held object applies
                // so we should probably give a hint about what CAN apply
                // to the target object.

                // Default transitions are currently just used to make
                // something react to the player (usually for animals getting
                // startled), not to actually accomplish
                // something

                // so we should show hints about the target object BEFORE it
                // went into its (temporary) reaction state. 
                }
            
            if( tr == NULL || tr->newTarget == destID ) {
                // give hint about dest object which will be unchanged 
                mNextHintObjectID = destID;
                if( isHintFilterStringInvalid() ) {
                    mNextHintIndex = mHintBookmarks[ destID ];
                    }
                }
            else if( tr->newActor > 0 && 
                     ourLiveObject->holdingID != tr->newActor ) {
                // give hint about how what we're holding will change
                mNextHintObjectID = tr->newActor;
                if( isHintFilterStringInvalid() ) {
                    mNextHintIndex = mHintBookmarks[ tr->newTarget ];
                    }
                }
            else if( tr->newTarget > 0 ) {
                // give hint about changed target after we act on it
                mNextHintObjectID = tr->newTarget;
                if( isHintFilterStringInvalid() ) {
                    mNextHintIndex = mHintBookmarks[ tr->newTarget ];
                    }
                }
            }
        else {
            // bare hand
            // only give hint about hit if no bare-hand action applies

            if( getTrans( 0, destID ) == NULL ) {
                mNextHintObjectID = destID;
                if( isHintFilterStringInvalid() ) {
                    mNextHintIndex = mHintBookmarks[ destID ];
                    }
                }
            }
        }

    
    if( destID > 0 && ! p.hitAnObject ) {
        
        // clicked on empty space near an object
        
        if( ! getObject( destID )->blocksWalking ) {
            
            // object in this space not blocking
            
            // count as an attempt to walk to the spot where the object is
            destID = 0;
            }
        else {
            // clicked on grid cell around blocking object
            destID = 0;
            
            // search for closest non-blocking and walk there
            
            GridPos closestP = { -1, -1 };
            
            double startDist = 99999;
            
            double closestDist = startDist;
            GridPos clickP = { clickDestX, clickDestY };
            GridPos ourP = { ourLiveObject->xd, ourLiveObject->yd };
            
            for( int y=-5; y<5; y++ ) {
                for( int x=-5; x<5; x++ ) {
                    
                    GridPos p = { clickDestX + x, clickDestY + y };
                    
                    int mapPX = p.x - mMapOffsetX + mMapD / 2;
                    int mapPY = p.y - mMapOffsetY + mMapD / 2;

                    if( mapPY >= 0 && mapPY < mMapD &&
                        mapPX >= 0 && mapPX < mMapD ) {
                        
                        int oID = mMap[ mapPY * mMapD + mapPX ];

                        if( oID == 0 
                            ||
                            ( oID > 0 && ! getObject( oID )->blocksWalking ) ) {


                            double d2 = distance2( p, clickP );
                            
                            if( d2 < closestDist ) {
                                closestDist = d2;
                                closestP = p;
                                }
                            else if( d2 == closestDist ) {
                                // break tie by whats closest to player
                                
                                double dPlayerOld = distance2( closestP,
                                                               ourP );
                                double dPlayerNew = distance2( p, ourP );
                                
                                if( dPlayerNew < dPlayerOld ) {
                                    closestDist = d2;
                                    closestP = p;
                                    }
                                }
                            
                            }
                        }
                    
                    }
                }
            
            if( closestDist < startDist ) {
                // found one
                // walk there instead
                moveDestX = closestP.x;
                moveDestY = closestP.y;
                
                clickDestX = moveDestX;
                clickDestY = moveDestY;
                
                mapX = clickDestX - mMapOffsetX + mMapD / 2;
                mapY = clickDestY - mMapOffsetY + mMapD / 2;
                }
            }
        }

    printf( "DestID = %d\n", destID );
    
    if( destID < 0 ) {
        return;
        }

    if( nextActionMessageToSend != NULL ) {
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }
    


   
    
    


    if( destID == 0 &&
        p.hitOtherPerson &&
        modClick && ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->deadlyDistance > 0 &&
        isShiftKeyDown() ) {
        
        // special case

        // check for possible kill attempt at a distance

        LiveObject *o = getLiveObject( p.hitOtherPersonID );
        
        if( o->id != ourID &&            
            o->heldByAdultID == -1 ) {
                
            // can't kill by clicking on ghost-location of held baby
            
            // clicked on someone
                    
            // new semantics:
            // send KILL to server right away to
            // tell server of our intentions
            // (whether or not we are close enough)
                
            // then walk there
                
            if( nextActionMessageToSend != NULL ) {
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }
                        
                        
            char *killMessage = 
                autoSprintf( "KILL %d %d %d#",
                             sendX( clickDestX ), 
                             sendY( clickDestY ),
                             p.hitOtherPersonID );
            printf( "KILL with direct-click target player "
                    "id=%d\n", p.hitOtherPersonID );
                    
            sendToServerSocket( killMessage );

            delete [] killMessage;
            

            // given that there's a cool-down now before killing, don't
            // auto walk there.
            // Player will enter kill state, and we'll let them navivate there
            // after that.
            ourLiveObject->killMode = true;
            ourLiveObject->killWithID = ourLiveObject->holdingID;
            
            return;
            }
        }
    

    if( destID != 0 &&
        ! modClick &&
        ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->useDistance > 1 &&
        getTrans( ourLiveObject->holdingID, destID ) != NULL ) {
        // check if we're close enough to use this from here 
        
        double d = sqrt( ( clickDestX - ourLiveObject->xd ) * 
                         ( clickDestX - ourLiveObject->xd )
                         +
                         ( clickDestY - ourLiveObject->yd ) * 
                         ( clickDestY - ourLiveObject->yd ) );

        if( getObject( ourLiveObject->holdingID )->useDistance >= d ) {
            // close enough to use object right now

                        
            if( nextActionMessageToSend != NULL ) {
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }
            
            nextActionMessageToSend = 
                autoSprintf( "USE %d %d#",
                             sendX( clickDestX ), 
                             sendY( clickDestY ) );
            
                        
            playerActionTargetX = clickDestX;
            playerActionTargetY = clickDestY;
            
            playerActionTargetNotAdjacent = true;
                        
            printf( "USE from a distance, src=(%d,%d), dest=(%d,%d)\n",
                    ourLiveObject->xd, ourLiveObject->yd,
                    clickDestX, clickDestY );
            
            return;
            }
        }
    

    char tryingToHeal = false;
    

    if( ourLiveObject->holdingID >= 0 &&
        p.hitOtherPerson &&
        getLiveObject( p.hitOtherPersonID )->dying ) {
        
        LiveObject *targetPlayer = getLiveObject( p.hitOtherPersonID );
        
        if( targetPlayer->holdingID > 0 ) {
            
            TransRecord *r = getTrans( ourLiveObject->holdingID,
                                       targetPlayer->holdingID );
            
            if( r != NULL ) {
                // a transition applies between what we're holding and their
                // wound
                tryingToHeal = true;
                }
            }
        }
    
    
    char canClickOnOtherForNonKill = false;
    
    if( tryingToHeal ) {
        canClickOnOtherForNonKill = true;
        }
    
    if( ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->deadlyDistance == 0 &&
        ( getObject( ourLiveObject->holdingID )->clothing != 'n' ||
          getObject( ourLiveObject->holdingID )->foodValue > 0 ) ) {
        canClickOnOtherForNonKill = true;
        }
    

    
    // true if we're too far away to use on baby BUT we should execute
    // UBABY once we get to destination

    // if we're close enough to use on baby, we'll use on baby from where 
    // we're standing
    // and return
    char useOnBabyLater = false;
    
    if( p.hitOtherPerson &&
        ! modClick && 
        destID == 0 &&
        canClickOnOtherForNonKill ) {


        doublePair targetPos = { (double)clickDestX, (double)clickDestY };

        for( int i=0; i<gameObjects.size(); i++ ) {
        
            LiveObject *o = gameObjects.getElement( i );
            
            if( o->id != ourID &&
                o->heldByAdultID == -1 ) {

                if( distance( targetPos, o->currentPos ) < 1 ) {
                    // clicked on someone
                    
                    if( ! ourLiveObject->inMotion && 
                        ( isGridAdjacent( clickDestX, clickDestY,
                                        ourLiveObject->xd, 
                                        ourLiveObject->yd ) ||
                          ( clickDestX == ourLiveObject->xd && 
                            clickDestY == ourLiveObject->yd ) ) ) {

                        
                        if( nextActionMessageToSend != NULL ) {
                            delete [] nextActionMessageToSend;
                            nextActionMessageToSend = NULL;
                            }
            
                        nextActionMessageToSend = 
                            autoSprintf( "UBABY %d %d %d %d#",
                                         sendX( clickDestX ), 
                                         sendY( clickDestY ), 
                                         p.hitClothingIndex, 
                                         p.hitOtherPersonID );
                        
                        
                        playerActionTargetX = clickDestX;
                        playerActionTargetY = clickDestY;
                        
                        playerActionTargetNotAdjacent = true;
                        
                        printf( "UBABY with target player %d\n", 
                                p.hitOtherPersonID );

                        return;
                        }
                    else {
                        // too far away, but try to use on baby later,
                        // once we walk there, using standard path-to-adjacent
                        // code below
                        useOnBabyLater = true;
                        
                        break;
                        }
                    }
                }
            }
    
        }




    char tryingToPickUpBaby = false;
    
    double ourAge = computeCurrentAge( ourLiveObject );

    if( destID == 0 &&
        p.hit &&
        p.hitOtherPerson &&
        ! p.hitAnObject &&
        ! modClick && ourLiveObject->holdingID == 0 &&
        // only adults can pick up babies
        ourAge > 13 ) {
        

        doublePair targetPos = { (double)clickDestX, (double)clickDestY };

        for( int i=0; i<gameObjects.size(); i++ ) {
        
            LiveObject *o = gameObjects.getElement( i );
            
            if( o->id != ourID &&
                o->heldByAdultID == -1 ) {

                if( distance( targetPos, o->currentPos ) < 1 ) {
                    // clicked on someone

                    if( computeCurrentAge( o ) < 5 ) {

                        // they're a baby
                        
                        tryingToPickUpBaby = true;
                        break;
                        }
                    }
                }
            }
        }

    
    // for USE actions that specify a slot number
    int useExtraIParam = -1;
    

    if( !killMode && 
        destID == 0 && !modClick && !tryingToPickUpBaby && !useOnBabyLater && 
        ! ( clickDestX == ourLiveObject->xd && 
            clickDestY == ourLiveObject->yd ) ) {
        // a move to an empty spot where we're not already standing
        // can interrupt current move
        
        mustMove = true;
        }
    else if( ( modClick && 
               // we can right click on an empty tile or full tile if
               // we're holding something
               // we can also right click with empty hand to pick something
               // up
               ( ourLiveObject->holdingID != 0 || 
                 ( destObjInClickedTile > 0 && 
                   ! destObjInClickedTilePermanent ) ) )
             || killMode
             || tryingToPickUpBaby
             || useOnBabyLater
             || destID != 0
             || ( modClick && ourLiveObject->holdingID == 0 &&
                  destNumContained > 0 ) ) {
        // use/drop modifier
        // OR pick up action
            
        
        char canExecute = false;
        char sideAccess = false;
        char noBackAccess = false;
        
        if( destID > 0 && getObject( destID )->sideAccess ) {
            sideAccess = true;
            }
        
        if( destID > 0 && getObject( destID )->noBackAccess ) {
            noBackAccess = true;
            }
        

        // direct click on adjacent cells or self cell?
        if( isGridAdjacent( clickDestX, clickDestY,
                            ourLiveObject->xd, ourLiveObject->yd )
            || 
            ( clickDestX == ourLiveObject->xd && 
              clickDestY == ourLiveObject->yd ) ) {
            
            canExecute = true;
            
            if( sideAccess &&
                ( clickDestY > ourLiveObject->yd ||
                  clickDestY < ourLiveObject->yd ) ) {
                // trying to access side-access object from N or S
                canExecute = false;
                }
            if( noBackAccess &&
                ( clickDestY < ourLiveObject->yd ) ) {
                // trying to access noBackAccess object from N
                canExecute = false;
                }

            if( canExecute && 
                clickDestX == ourLiveObject->xd && 
                clickDestY == ourLiveObject->yd ) {
                // access from where we're standing
                
                // make sure result is non-blocking
                // (else walk to an empty spot before executing action)
                if( destID > 0 ) {
                    
                    int newDestID = 0;
                    
                    TransRecord *useTrans = 
                        getTrans( ourLiveObject->holdingID, destID );
                    
                    if( useTrans != NULL ) {
                        newDestID = useTrans->newTarget;
                        }
                    
                    if( newDestID > 0 && 
                        getObject( newDestID )->blocksWalking ) {
                        canExecute = false;
                        }
                    }
                }
            }


        if( ! canExecute ) {
            // need to move to empty adjacent first, if it exists
            
            // also consider spot itself in some cases
            
            int nDX[5] = { 0, -1, +1, 0, 0 };
            int nDY[5] = { 0, 0, 0, -1, +1 };
            
            char foundEmpty = false;
            
            int closestDist = 9999999;

            char oldPathExists = ( ourLiveObject->pathToDest != NULL );

            if( oldPathExists ) {
                savePlayerPath( ourLiveObject );
                }
            
            // don't consider dest spot itself generally
            int nStart = 1;
            
            int nLimit = 5;
            
            if( sideAccess ) {
                // don't consider N or S neighbors
                nLimit = 3;
                }
            else if( noBackAccess ) {
                // don't consider N neighbor
                nLimit = 4;
                }
            else if( destID > 0 &&
                     ( ourLiveObject->holdingID <= 0 ||
                       ( getObject( ourLiveObject->holdingID )->permanent &&
                         strstr( getObject( ourLiveObject->holdingID )->
                                 description, "sick" ) != NULL ) )
                     &&
                     getObject( destID )->permanent &&
                     ! getObject( destID )->blocksWalking ) {
                
                TransRecord *handTrans = NULL;
                if( ourLiveObject->holdingID == 0 ) {
                    handTrans = getTrans( 0, destID );
                    }
                
                if( handTrans == NULL ||
                    ( handTrans->newActor != 0 &&
                      getObject( handTrans->newActor )->foodValue > 0 &&
                        handTrans->newTarget != 0 &&
                      ! getObject( handTrans->newTarget )->blocksWalking ) ) {
                    // walk to tile itself if target is permanent
                    // and not blocking, and hand is empty
                    // AND this will result in something still
                    // on the ground (so it's not a transforming pick-up,
                    // like pulling an onion).
                    // and the new thing on the ground is not blocking
                    // (so we're not closing a door)
                    // and what you get in the hand is edible
                    // (example:  picking berries from behind the bush)
                    //
                    // this is the main situation where you'd want to
                    // click the same target and yourself
                    // multiple times in a row, so having yourself
                    // as close as possible to the target matters
                    nStart = 0;
                    }
                }



            for( int n=nStart; n<nLimit; n++ ) {
                int x = mapX + nDX[n];
                int y = mapY + nDY[n];

                if( y >= 0 && y < mMapD &&
                    x >= 0 && x < mMapD ) {
                 
                    
                    int mapI = y * mMapD + x;
                    
                    if( mMap[ mapI ] == 0
                        ||
                        ( mMap[ mapI ] != -1 && 
                          ! getObject( mMap[ mapI ] )->blocksWalking ) ) {
                        
                        int emptyX = clickDestX + nDX[n];
                        int emptyY = clickDestY + nDY[n];

                        // check if there's a path there
                        int oldXD = ourLiveObject->xd;
                        int oldYD = ourLiveObject->yd;
                        
                        // set this temporarily for pathfinding
                        ourLiveObject->xd = emptyX;
                        ourLiveObject->yd = emptyY;
                        
                        computePathToDest( ourLiveObject );
                        
                        if( ourLiveObject->pathToDest != NULL &&
                            ourLiveObject->pathLength < closestDist ) {
                            
                            // can get there
                            
                            moveDestX = emptyX;
                            moveDestY = emptyY;
                            
                            closestDist = ourLiveObject->pathLength;
                            
                            foundEmpty = true;
                            }
                        
                        // restore our old dest
                        ourLiveObject->xd = oldXD;
                        ourLiveObject->yd = oldYD;    

                        if( n == 0 && foundEmpty ) {
                            // always prefer tile itself, if that's an option
                            // based on logic above, even if further
                            break;
                            }
                        }
                    
                    }
                }


            if( !foundEmpty && 
                ! sideAccess &&
                nStart > 0 &&
                ( destID == 0 ||
                  ! getObject( destID )->blocksWalking ) ) {
                
                // all neighbors blocked
                // we didn't consider tile itself before
                // but now we will, as last resort.

                // consider tile itself as dest
                int oldXD = ourLiveObject->xd;
                int oldYD = ourLiveObject->yd;
                        
                // set this temporarily for pathfinding
                ourLiveObject->xd = clickDestX;
                ourLiveObject->yd = clickDestY;
                        
                computePathToDest( ourLiveObject );
                        
                if( ourLiveObject->pathToDest != NULL  ) {
                            
                    // can get there
                    
                    moveDestX = clickDestX;
                    moveDestY = clickDestY;
                            
                    foundEmpty = true;
                    }
                        
                // restore our old dest
                ourLiveObject->xd = oldXD;
                ourLiveObject->yd = oldYD; 
                }
            
            
            if( oldPathExists ) {
                restoreSavedPath( ourLiveObject );
                }

            if( foundEmpty ) {
                canExecute = true;
                }
            // always try to move as close as possible, even
            // if we can't actually get close enough to execute action
            mustMove = true;
            }
        

        if( !canExecute && getObject( destID )->wide ) {
            // can't reach main tile on wide object
            // can we reach a side-wing?
            // only check where we're actually standing
            // this is a rare case where we're trapped by a wide object
            // no problem in making user walk as close as possible manually
            // to try to escape (so we don't need to overhaul all movement
            // code to deal with wide objects)
            ObjectRecord *destO = getObject( destID );
            
            for( int r=0; r<destO->leftBlockingRadius; r++ ) {
                int testX = clickDestX - r - 1;
                if( isGridAdjacent( testX, clickDestY,
                                    ourLiveObject->xd, ourLiveObject->yd ) ) {
                    canExecute = true;
                    break;
                    }
                }
            
            if( ! canExecute )
            for( int r=0; r<destO->rightBlockingRadius; r++ ) {
                int testX = clickDestX + r + 1;
                if( isGridAdjacent( testX, clickDestY,
                                    ourLiveObject->xd, ourLiveObject->yd ) ) {
                    canExecute = true;
                    break;
                    }
                }
            if( canExecute ) {
                mustMove = false;
                playerActionTargetNotAdjacent = true;
                }
            }
        
        

        if( canExecute && ! killMode ) {
            
            const char *action = "";
            char *extra = stringDuplicate( "" );
            
            char send = false;
            
            if( tryingToPickUpBaby ) {
                action = "BABY";
                if( p.hitOtherPerson ) {
                    delete [] extra;
                    extra = autoSprintf( " %d", p.hitOtherPersonID );
                    }
                send = true;
                }
            else if( useOnBabyLater ) {
                action = "UBABY";
                delete [] extra;
                extra = autoSprintf( " %d %d", p.hitClothingIndex,
                                     p.hitOtherPersonID );
                send = true;
                }
            else if( modClick && destID == 0 ) {
                
                if( ourLiveObject->holdingID != 0 ) {
                    action = "DROP";
                    nextActionDropping = true;
                    }
                else {
                    action = "USE";
                    nextActionDropping = false;
                    }
                
                send = true;

                // check for other special case
                // a use-on-ground transition or use-on-floor transition

                if( ourLiveObject->holdingID > 0 ) {
                        
                    ObjectRecord *held = 
                        getObject( ourLiveObject->holdingID );
                        
                    char foundAlt = false;
                        
                    if( held->foodValue == 0 &&
                        destObjInClickedTile == 0 ) {
                        // a truly empty spot where use-on-bare-ground
                        // can happen

                        TransRecord *r = 
                            getTrans( ourLiveObject->holdingID,
                                      -1 );
                            
                        if( r != NULL &&
                            r->newTarget != 0 ) {
                                
                            // a use-on-ground transition exists!
                                
                            // override the drop action
                            action = "USE";
                            foundAlt = true;
                            }
                        }

                    if( !foundAlt && floorDestID > 0 ) {
                        // check if use on floor exists
                        TransRecord *r = 
                            getTrans( ourLiveObject->holdingID, 
                                      floorDestID );
                                
                        if( r != NULL ) {
                            // a use-on-floor transition exists!
                                
                            // override the drop action
                            action = "USE";
                                
                            }
                        }
                    }
                }
            else if( destID != 0 &&
                     ( modClick || 
                       ( getObject( destID )->permanent &&
                         getTrans( 0, destID ) == NULL
                         ) 
                       )
                     && ourLiveObject->holdingID == 0 &&
                     getNumContainerSlots( destID ) > 0 ) {
                
                // for permanent container objects that have no bare-hand
                // transition , we shouldn't make
                // distinction between left and right click

                action = "REMV";

                if( ! modClick ) {
                    // no bare-hand action
                    // but check if this object decays in 1 second
                    // and if so, a bare-hand action applies after that
                    TransRecord *decayTrans = getTrans( -1, destID );
                    if( decayTrans != NULL &&
                        decayTrans->newTarget > 0 &&
                        decayTrans->autoDecaySeconds == 1 ) {
                        
                        if( getTrans( 0, decayTrans->newTarget ) != NULL ) {
                            // switch to USE in this case
                            // because server will force object to decay
                            // so a transition can go through
                            action = "USE";
                            }
                        }
                    }
                else {
                    // in case of mod-click, if we clicked a contained item
                    // directly, and it has a bare hand transition,
                    // consider doing that as a USE
                    ObjectRecord *destObj = getObject( destID );
                    
                    if( destObj->numSlots > p.hitSlotIndex &&
                        strstr( destObj->description, "+useOnContained" )
                        != NULL ) {
                        action = "USE";
                        useExtraIParam = p.hitSlotIndex;
                        }
                    }
                

                send = true;
                delete [] extra;
                extra = autoSprintf( " %d", p.hitSlotIndex );
                }
            else if( modClick && ourLiveObject->holdingID != 0 &&
                     destID != 0 &&
                     getNumContainerSlots( destID ) > 0 &&
                     destNumContained <= getNumContainerSlots( destID ) ) {
                action = "DROP";
                nextActionDropping = true;
                send = true;
                }
            else if( modClick && ourLiveObject->holdingID != 0 &&
                     destID != 0 &&
                     getNumContainerSlots( destID ) == 0 &&
                     ! getObject( destID )->permanent ) {
                action = "DROP";
                nextActionDropping = true;
                send = true;
                }
            else if( destID != 0 ) {
                // allow right click and default to USE if nothing
                // else applies
                action = "USE";

                // check for case where both bare-hand transition
                // AND pickup applies
                // use mod-click to differentiate between two possibilities
                if( ourLiveObject->holdingID == 0 &&
                    getNumContainerSlots( destID ) == 0 &&
                    ! getObject( destID )->permanent ) {
                    
                    TransRecord *bareHandTrans = getTrans( 0, destID );
                    
                    if( bareHandTrans != NULL &&
                        bareHandTrans->newTarget != 0 ) {
                        
                        // bare hand trans exists, and it's NOT just a
                        // direct "pickup" trans that should always be applied
                        // (from target to new actor)
                        // The bare hand trans leaves something on the ground
                        // meaning that it is transformational (removing
                        // a plate from a stack, tweaking something, etc.)

                        if( modClick ) {
                            action = "USE";
                            }
                        else {
                            action = "REMV";

                             delete [] extra;
                             extra = stringDuplicate( " 0" );
                            }
                        }
                    }
                else if( ourLiveObject->holdingID > 0 &&
                         p.hitSlotIndex != -1 &&
                         getNumContainerSlots( destID ) > p.hitSlotIndex ) {
                    
                    // USE on a slot?  Only if allowed by container

                    ObjectRecord *destObj = getObject( destID );
                    
                    if( strstr( destObj->description, "+useOnContained" )
                        != NULL ) {
                        useExtraIParam = p.hitSlotIndex;
                        }
                    }
                
                send = true;
                }
            
            if( strcmp( action, "DROP" ) == 0 ) {
                delete [] extra;
                nextActionDropping = true;
                extra = stringDuplicate( " -1" );
                }

            if( strcmp( action, "USE" ) == 0 &&
                destID > 0 ) {
                // optional ID param for USE, specifying that we clicked
                // on something
                delete [] extra;
                
                if( useExtraIParam != -1 ) {
                    extra = autoSprintf( " %d %d", destID, useExtraIParam );
                    }
                else {
                    extra = autoSprintf( " %d", destID );
                    }
                }
            
            
            if( send ) {
                // queue this until after we are done moving, if we are
                nextActionMessageToSend = 
                    autoSprintf( "%s %d %d%s#", action,
                                 sendX( clickDestX ), 
                                 sendY( clickDestY ), extra );
                
                int trueClickDestX = clickDestX;
                int trueClickDestY = clickDestY;
                
                if( mapY >= 0 && mapY < mMapD &&
                    mapX >= 0 && mapX < mMapD ) {
                    
                    int mapI = mapY * mMapD + mapX;
                    
                    if( mMapMoveSpeeds[ mapI ] > 0 ) {        
                        
                        trueClickDestX = 
                            lrint( clickDestX + mMapMoveOffsets[mapI].x );
                        trueClickDestY = 
                            lrint( clickDestY + mMapMoveOffsets[mapI].y );
                        }
                    }

                ourLiveObject->actionTargetX = clickDestX;
                ourLiveObject->actionTargetY = clickDestY;

                ourLiveObject->actionTargetTweakX = trueClickDestX - clickDestX;
                ourLiveObject->actionTargetTweakY = trueClickDestY - clickDestY;
                

                playerActionTargetX = clickDestX;
                playerActionTargetY = clickDestY;
                }

            delete [] extra;
            }
        }
    


    
    if( mustMove ) {

        char oldPathExists = ( ourLiveObject->pathToDest != NULL );
        
        if( oldPathExists ) {
            savePlayerPath( ourLiveObject );
            }



        int oldXD = ourLiveObject->xd;
        int oldYD = ourLiveObject->yd;
        
        ourLiveObject->xd = moveDestX;
        ourLiveObject->yd = moveDestY;
        ourLiveObject->destTruncated = false;



        
        

        computePathToDest( ourLiveObject );

        
        if( ourLiveObject->pathToDest == NULL ) {
            // adjust move to closest possible
            ourLiveObject->xd = ourLiveObject->closestDestIfPathFailedX;
            ourLiveObject->yd = ourLiveObject->closestDestIfPathFailedY;
            ourLiveObject->destTruncated = false;
            
            if( ourLiveObject->xd == oldXD && ourLiveObject->yd == oldYD ) {
                // completely blocked in, no path at all toward dest
                
                if( oldPathExists ) {
                    restoreSavedPath( ourLiveObject );
                    }
                    

                // ignore click
                
                if( nextActionMessageToSend != NULL ) {
                    delete [] nextActionMessageToSend;
                    nextActionMessageToSend = NULL;
                    }
                
                return;
                }

            if( oldPathExists ) {
                freeSavedPath();
                oldPathExists = false;
                }
            

            computePathToDest( ourLiveObject );
            
            if( ourLiveObject->pathToDest == NULL &&
                ourLiveObject->useWaypoint ) {
                // waypoint itself may be blocked
                // try again with no waypoint at all
                ourLiveObject->useWaypoint = false;
                computePathToDest( ourLiveObject );
                }

            if( ourLiveObject->pathToDest == NULL ) {
                // this happens when our curPos is slightly off of xd,yd
                // but not a full cell away

                // make a fake path
                doublePair dest = { (double) ourLiveObject->xd,
                                    (double) ourLiveObject->yd };
                
                doublePair dir = normalize( sub( dest, 
                                                 ourLiveObject->currentPos ) );

                // fake start, one grid step away
                doublePair fakeStart = dest;

                if( fabs( dir.x ) > fabs( dir.y ) ) {
                    
                    if( dir.x < 0 ) {
                        fakeStart.x += 1;
                        }
                    else {
                        fakeStart.x -= 1;
                        }
                    }
                else {
                    if( dir.y < 0 ) {
                        fakeStart.y += 1;
                        }
                    else {
                        fakeStart.y -= 1;
                        }
                    
                    }
                
                
                ourLiveObject->pathToDest = new GridPos[2];
                
                ourLiveObject->pathToDest[0].x = (int)fakeStart.x;
                ourLiveObject->pathToDest[0].y = (int)fakeStart.y;
                
                ourLiveObject->pathToDest[1].x = ourLiveObject->xd;
                ourLiveObject->pathToDest[1].y = ourLiveObject->yd;
                
                ourLiveObject->pathLength = 2;
                ourLiveObject->currentPathStep = 0;
                ourLiveObject->numFramesOnCurrentStep = 0;
                ourLiveObject->onFinalPathStep = false;
                
                ourLiveObject->currentMoveDirection =
                    normalize( 
                        sub( gridToDouble( ourLiveObject->pathToDest[1] ), 
                             gridToDouble( ourLiveObject->pathToDest[0] ) ) );
                }

            if( ourLiveObject->xd == oldXD 
                &&
                ourLiveObject->yd == oldYD ) {
                
                // truncated path is where we're already going
                // don't send new move message
                return;
                }


            moveDestX = ourLiveObject->xd;
            moveDestY = ourLiveObject->yd;
            
            if( nextActionMessageToSend != NULL ) {
                // abort the action, because we can't reach the spot we
                // want to reach
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }
            
            }
        
        
        if( oldPathExists ) {
            freeSavedPath();
            }
            


        // send move right away
        //Thread::staticSleep( 2000 );
        SimpleVector<char> moveMessageBuffer;
        
        moveMessageBuffer.appendElementString( "MOVE" );
        ourLiveObject->lastMoveSequenceNumber ++;
        
        // start is absolute
        char *startString = 
            autoSprintf( " %d %d @%d", 
                         sendX( ourLiveObject->pathToDest[0].x ),
                         sendY( ourLiveObject->pathToDest[0].y ),
                         ourLiveObject->lastMoveSequenceNumber );
        moveMessageBuffer.appendElementString( startString );
        delete [] startString;
        
        for( int i=1; i<ourLiveObject->pathLength; i++ ) {
            // rest are relative to start
            char *stepString = autoSprintf( " %d %d", 
                                         ourLiveObject->pathToDest[i].x
                                            - ourLiveObject->pathToDest[0].x,
                                         ourLiveObject->pathToDest[i].y
                                            - ourLiveObject->pathToDest[0].y );
            
            moveMessageBuffer.appendElementString( stepString );
            delete [] stepString;
            }
        moveMessageBuffer.appendElementString( "#" );
        

        char *message = moveMessageBuffer.getElementString();

        sendToServerSocket( message );


        delete [] message;

        // start moving before we hear back from server

        
        ourLiveObject->inMotion = true;




        double floorSpeedMod = computePathSpeedMod( ourLiveObject,
                                                    ourLiveObject->pathLength );
        

        ourLiveObject->moveTotalTime = 
            measurePathLength( ourLiveObject, ourLiveObject->pathLength ) / 
            ( ourLiveObject->lastSpeed * floorSpeedMod );

        ourLiveObject->moveEtaTime = game_getCurrentTime() +
            ourLiveObject->moveTotalTime;

            
        updateMoveSpeed( ourLiveObject );
        }    
    }




void LivingLifePage::pointerDrag( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );
    
    if( showBugMessage ) {
        return;
        }
    }


void LivingLifePage::pointerUp( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;

    if( showBugMessage ) {
        return;
        }
    

    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    if( playerActionPending ) {
        // block further actions until update received to confirm last
        // action
        mouseDown = false;
        return;
        }

    if( mouseDown && 
        getOurLiveObject()->inMotion 
        &&
        mouseDownFrames >  
        minMouseDownFrames / frameRateFactor ) {
        
        // treat the up as one final click (at closest next path position
        // to where they currently are)
        
        LiveObject *o = getOurLiveObject();

        if( o->pathToDest != NULL &&
            o->currentPathStep < o->pathLength - 2 ) {
            GridPos p = o->pathToDest[ o->currentPathStep + 1 ];
        
            isAutoClick = true;
            pointerDown( p.x * CELL_D, p.y * CELL_D );
            isAutoClick = false;
            }
        

        // don't do this for now, because it's confusing
        // pointerDown( inX, inY );
        }

    mouseDown = false;


    // clear mouse over cell
    mPrevMouseOverCells.push_back( mCurMouseOverCell );
    mPrevMouseOverCellFades.push_back( mCurMouseOverCellFade );
    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;
    }




SimpleVector<int> LivingLifePage::getOurLeadershipChain() {
    
    SimpleVector<int> ourLeadershipChain;

    LiveObject *ourLiveObject = getOurLiveObject();
    
    int nextID = ourLiveObject->followingID;
    
    while( nextID != -1 ) {
        ourLeadershipChain.push_back( nextID );
        
        LiveObject *l = getGameObject( nextID );

        if( l != NULL ) {
            nextID = l->followingID;
            }
        else {
            nextID = -1;
            }
        }
    return ourLeadershipChain;
    }



char LivingLifePage::isFollower( LiveObject *inLeader, 
                                 LiveObject *inFollower ) {
    int nextID = inFollower->followingID;

    while( nextID != -1 ) {
        if( nextID == inLeader->id ) {
            return true;
            }
        LiveObject *l = getGameObject( nextID );

        if( l != NULL ) {
            nextID = l->followingID;
            }
        else {
            nextID = -1;
            }
        }
    return false;
    }



int LivingLifePage::getTopLeader( LiveObject *inPlayer ) {
    int nextID = inPlayer->followingID;

    while( nextID != -1 ) {
        LiveObject *l = getGameObject( nextID );

        if( l != NULL ) {
            nextID = l->followingID;
            
            if( nextID == -1 ) {
                return l->id;
                }
            }
        else {
            nextID = -1;
            }
        }
    
    // top leader is self
    return inPlayer->id;
    }




char LivingLifePage::isExiled( LiveObject *inViewer, LiveObject *inPlayer ) {
    int viewerID = inViewer->id;
    
    for( int e=0; e < inPlayer->exiledByIDs.size(); e++ ) {
        int eID = inPlayer->exiledByIDs.getElementDirect( e );
        
        // we have them exiled
        if( eID == viewerID ) {
            return true;
            }
        
        LiveObject *eO = getLiveObject( eID );
        
        if( isFollower( eO, inViewer ) ) {
            // one of our leaders has them exiled
            return true;
            }
        }
    
    return false;
    }




static void showPlayerLabel( LiveObject *inPlayer, const char *inLabel, 
                             double inETA ) {
    
    if( inPlayer->currentSpeech != NULL ) {
        delete [] inPlayer->currentSpeech;
        inPlayer->currentSpeech = NULL;
        }
    
    inPlayer->currentSpeech = stringDuplicate( inLabel );
    inPlayer->speechFade = 1.0;
    
    inPlayer->speechIsSuccessfulCurse = false;
    
    inPlayer->speechFadeETATime = inETA;
    inPlayer->speechIsCurseTag = false;
    inPlayer->speechIsOverheadLabel = false;
    }



static char commandTyped( char *inTyped, const char *inCommandTransKey ) {
    const char *command = translate( inCommandTransKey );
    
    if( strstr( inTyped, command ) == inTyped ) {
        
        char *trimmedCommand = trimWhitespace( inTyped );
        
        unsigned int lengthTrim = strlen( trimmedCommand );
        
        delete [] trimmedCommand;
        
        if( lengthTrim == strlen( command ) ) {
            return true;
            }
        }
    return false;
    }




void LivingLifePage::keyDown( unsigned char inASCII ) {
    
    registerTriggerKeyCommand( inASCII, this );


    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    if( showBugMessage ) {
        if( inASCII == '%' ) {
            showBugMessage = false;
            }
        return;
        }

    
    switch( inASCII ) {
        /*
        // useful for testing
        case '+':
            getOurLiveObject()->displayID = getRandomPersonObject();
            break;
        case '_':
            getOurLiveObject()->age += 10;
            break;
        case '-':
            getOurLiveObject()->age -= 5;
            break;
        case '~':
            getOurLiveObject()->age -= 1;
            break;
        */
        case 'V':
            if( ! mSayField.isFocused() &&
                serverSocketConnected &&
                SettingsManager::getIntSetting( "vogModeOn", 0 ) ) {
                
                if( ! vogMode ) {
                    sendToServerSocket( (char*)"VOGS 0 0#" );
                    vogMode = true;
                    vogModeActuallyOn = false;

                    vogPos = getOurLiveObject()->currentPos;
                    vogPickerOn = false;
                    mObjectPicker.setPosition( vogPos.x * CELL_D + 510,
                                               vogPos.y * CELL_D + 90 );
                    
                    // jump camp instantly
                    lastScreenViewCenter.x = vogPos.x * CELL_D;
                    lastScreenViewCenter.y = vogPos.y * CELL_D;
                    setViewCenterPosition( lastScreenViewCenter.x,
                                           lastScreenViewCenter.y );
                    }
                else {
                    sendToServerSocket( (char*)"VOGX 0 0#" );
                    vogMode = false;
                    if( vogPickerOn ) {
                        removeComponent( &mObjectPicker );
                        mObjectPicker.removeActionListener( this );
                        }
                    vogPickerOn = false;
                    }
                }
            break;
        case 'I':
            if( ! mSayField.isFocused() &&
                vogMode ) {
                
                if( ! vogPickerOn ) {
                    addComponent( &mObjectPicker );
                    mObjectPicker.addActionListener( this );
                    }
                else {
                    removeComponent( &mObjectPicker );
                    mObjectPicker.removeActionListener( this );    
                    }
                vogPickerOn = ! vogPickerOn;
                }
            break;
        case 'N':
            if( ! mSayField.isFocused() &&
                serverSocketConnected &&
                vogMode ) {
                sendToServerSocket( (char*)"VOGP 0 0#" );
                }
            break;
        case 'M':
            if( ! mSayField.isFocused() &&
                serverSocketConnected &&
                vogMode ) {
                sendToServerSocket( (char*)"VOGN 0 0#" );
                }
            break;
        case 'S':
            if( savingSpeechEnabled && 
                ! mSayField.isFocused() ) {
                savingSpeechColor = true;
                savingSpeechMask = false;
                savingSpeech = true;
                }
            break;
        case 'x':
        case 'X':
            if( userTwinCode != NULL &&
                ! mStartedLoadingFirstObjectSet ) {
                
                closeSocket( mServerSocket );
                mServerSocket = -1;
                
                setWaiting( false );
                setSignal( "twinCancel" );
                }
            else if( ! mSayField.isFocused() ) {
                mXKeyDown = true;
                }
            break;
        /*
        case 'b':
            blackBorder = true;
            whiteBorder = false;
            break;
        case 'B':
            blackBorder = false;
            whiteBorder = true;
            break;
        */
        /*
        case 'a':
            drawAdd = ! drawAdd;
            break;
        case 'm':
            drawMult = ! drawMult;
            break;
        case 'n':
            multAmount += 0.05;
            printf( "Mult amount = %f\n", multAmount );
            break;
        case 'b':
            multAmount -= 0.05;
            printf( "Mult amount = %f\n", multAmount );
            break;
        case 's':
            addAmount += 0.05;
            printf( "Add amount = %f\n", addAmount );
            break;
        case 'd':
            addAmount -= 0.05;
            printf( "Add amount = %f\n", addAmount );
            break;
        */
        /*
        case 'd':
            pathStepDistFactor += .1;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 'D':
            pathStepDistFactor += .5;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 's':
            pathStepDistFactor -= .1;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 'S':
            pathStepDistFactor -= .5;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 'f':
            trail.deleteAll();
            trailColors.deleteAll();
            break;
        */
        case 'e':
        case 'E':
            if( ! mSayField.isFocused() ) {
                mEKeyDown = true;
                }
            break;
        case 'z':
        case 'Z':
            if( mUsingSteam && ! mSayField.isFocused() ) {
                mZKeyDown = true;
                }
            break;
        case ' ':
            if( ! mSayField.isFocused() ) {
                shouldMoveCamera = false;
                }
            break;
        case 9: // tab
            if( mCurrentHintObjectID != 0 ) {
                
                int num = getNumHints( mCurrentHintObjectID );
                
                int skip = 1;
                
                if( !mUsingSteam && isShiftKeyDown() ) {
                    skip = -1;
                    }
                else if( mUsingSteam && mZKeyDown ) {
                    skip = -1;
                    }
                if( isCommandKeyDown() ) {
                    if( num > 5 ) {
                        skip *= 5;
                        }
                    }

                if( num > 0 ) {
                    mNextHintIndex += skip;
                    if( mNextHintIndex >= num ) {
                        mNextHintIndex -= num;
                        }
                    if( mNextHintIndex < 0 ) {
                        mNextHintIndex += num;
                        }
                    justHitTab = true;
                    }
                }
            break;
        case '/':
            if( ! mSayField.isFocused() ) {
                mEKeyDown = false;
                mZKeyDown = false;
                mXKeyDown = false;

                // start typing a filter
                mSayField.setText( "/" );
                mSayField.focus();
                }
            break;
        case 13:  // enter
            // speak
            if( ! TextField::isAnyFocused() ) {
                mEKeyDown = false;
                mZKeyDown = false;
                mXKeyDown = false;

                mSayField.setText( "" );
                mSayField.focus();
                }
            else if( mSayField.isFocused() ) {
                char *typedText = mSayField.getText();
                
                
                // tokenize and then recombine with single space
                // between each.  This will get rid of any hidden
                // lead/trailing spaces, which may confuse server
                // for baby naming, etc.
                // also eliminates all-space strings, which 
                // end up in speech history, but are invisible
                SimpleVector<char *> *tokens = 
                    tokenizeString( typedText );
                
                if( tokens->size() > 0 ) {
                    char *oldTypedText = typedText;
                    
                    char **strings = tokens->getElementArray();
                    
                    typedText = join( strings, tokens->size(), " " );
                    
                    delete [] strings;
                    
                    delete [] oldTypedText;
                    }
                else {
                    // string with nothing but spaces, or empty
                    // force it empty
                    delete [] typedText;
                    typedText = stringDuplicate( "" );
                    }
                
                tokens->deallocateStringElements();
                delete tokens;
                    
                

                if( strcmp( typedText, "" ) == 0 ) {
                    mSayField.unfocus();
                    }
                else {
                    
                    if( strlen( typedText ) > 0 &&
                         typedText[0] == '/' ) {
                    
                        // a command, don't send to server
                        
                        const char *filterCommand = "/";
                        
                        if( strstr( typedText, filterCommand ) == typedText ) {
                            // starts with filter command
                            
                            LiveObject *ourLiveObject = getOurLiveObject();
                            
                            int emotIndex = getEmotionIndex( typedText );
                            
                            if( emotIndex != -1 ) {
                                char *message = 
                                    autoSprintf( "EMOT 0 0 %d#", emotIndex );
                                
                                sendToServerSocket( message );
                                delete [] message;
                                }
                            else if( commandTyped( typedText, "dieCommand" ) 
                                     &&
                                     computeCurrentAge( ourLiveObject ) < 2 ) {
                                // die command issued from baby
                                char *message = 
                                    autoSprintf( "DIE 0 0#" );
                                
                                sendToServerSocket( message );
                                delete [] message;
                                }
                            else if( commandTyped( typedText, "fpsCommand" ) ) {
                                showFPS = !showFPS;
                                frameBatchMeasureStartTime = -1;
                                framesInBatch = 0;
                                fpsToDraw = -1;
                                if( showFPS ) {
                                    startCountingUniqueSpriteDraws();
                                    startCountingSpritesDrawn();
                                    }
                                else {
                                    endCountingUniqueSpriteDraws();
                                    endCountingSpritesDrawn();
                                    }
                                }
                            else if( commandTyped( typedText, "netCommand" ) ) {
                                showNet = !showNet;
                                netBatchMeasureStartTime = -1;
                                messagesInPerSec = -1;
                                messagesOutPerSec = -1;
                                bytesInPerSec = -1;
                                bytesOutPerSec = -1;
                                messagesInCount = 0;
                                messagesOutCount = 0;
                                bytesInCount = 0;
                                bytesOutCount = 0;
                                }
                            else if( commandTyped( typedText, 
                                                   "pingCommand" ) ) {

                                waitingForPong = true;
                                lastPingSent ++;
                                char *pingMessage = 
                                    autoSprintf( "PING 0 0 %d#", lastPingSent );
                                
                                sendToServerSocket( pingMessage );
                                delete [] pingMessage;

                                showPing = true;
                                pingSentTime = game_getCurrentTime();
                                pongDeltaTime = -1;
                                pingDisplayStartTime = -1;
                                }
                            else if( commandTyped( typedText, 
                                                   "disconnectCommand" ) ) {
                                forceDisconnect = true;
                                }
                            else if( commandTyped( typedText, 
                                                   "familyCommand" ) ) {
                                
                                const char *famLabel = 
                                    translate( "familyLabel" );
                                
                                double eta = game_getCurrentTime() + 3 +
                                    strlen( famLabel ) / 5;

                                for( int f=0; f<gameObjects.size(); f++ ) {
                                    
                                    LiveObject *famO = 
                                        gameObjects.getElement( f );
                                    if( famO->isGeneticFamily ) {
                                        
                                        showPlayerLabel( famO, famLabel,
                                                         eta );
                                        }
                                    }
                                }
                            else if( commandTyped( typedText, 
                                                   "leaderCommand" ) ) {
                                
                                leaderCommandTyped = true;
                                
                                const char *leaderLabel = 
                                    translate( "leaderLabel" );
                                
                                double eta = game_getCurrentTime() + 3 +
                                    strlen( leaderLabel ) / 5;

                                SimpleVector<int> ourLeadershipChain =
                                    getOurLeadershipChain();

                                for( int f=0; 
                                     f<ourLeadershipChain.size(); f++ ) {
                                    
                                    LiveObject *leadO = 
                                        getLiveObject( 
                                            ourLeadershipChain.
                                            getElementDirect( f ) );
                                    
                                    showPlayerLabel( leadO, leaderLabel, eta );
                                    }
                                sendToServerSocket( (char*)"LEAD 0 0#" );
                                }
                            else if( commandTyped( typedText, 
                                                   "followerCommand" ) ) {
                                
                                const char *followerLabel = 
                                    translate( "followerLabel" );
                                
                                double eta = game_getCurrentTime() + 3 +
                                    strlen( followerLabel ) / 5;
                                
                                int count = 0;
                                
                                for( int f=0; f<gameObjects.size(); f++ ) {
                                    
                                    LiveObject *testO =
                                        gameObjects.getElement( f );
                                    
                                    if( isFollower( ourLiveObject, testO ) ) {
                                        
                                        showPlayerLabel( testO, 
                                                         followerLabel, eta );
                                        count ++;
                                        }
                                    }
                                
                                char *message;
                                if( count == 0 ) {
                                    message = stringDuplicate( 
                                        translate( "noFollowerCountMessage" ) );
                                    }
                                else if( count == 1 ) {
                                    message = stringDuplicate( 
                                        translate( 
                                            "oneFollowerCountMessage" ) );
                                    }
                                else {
                                    message =
                                        autoSprintf( 
                                            translate( "followerCountMessage" ),
                                            count );
                                    }
                                
                                displayGlobalMessage( message );
                                delete [] message;
                                }
                            else if( commandTyped( typedText, 
                                                   "allyCommand" ) ) {
                                
                                const char *allyLabel = 
                                    translate( "allyLabel" );
                                
                                double eta = game_getCurrentTime() + 3 +
                                    strlen( allyLabel ) / 5;

                                int ourTopLeader = 
                                    getTopLeader( ourLiveObject );
                                
                                int count = 0;
                                
                                for( int f=0; f<gameObjects.size(); f++ ) {
                                    
                                    LiveObject *testO =
                                        gameObjects.getElement( f );
                                    
                                    if( testO != ourLiveObject 
                                        &&
                                        getTopLeader( testO ) == 
                                        ourTopLeader 
                                        &&
                                        // we don't see them as exiled
                                        ! isExiled( ourLiveObject, testO )
                                        &&
                                        // they don't see us as exiled
                                        ! isExiled( testO, ourLiveObject ) ) {
                                        
                                        showPlayerLabel( testO, 
                                                         allyLabel, eta );
                                        count ++;
                                        }
                                    }
                                
                                char *message;
                                if( count == 0 ) {
                                    message = stringDuplicate( 
                                        translate( "noAllyCountMessage" ) );
                                    }
                                else if( count == 1 ) {
                                    message = stringDuplicate( 
                                        translate( 
                                            "oneAllyCountMessage" ) );
                                    }
                                else {
                                    message =
                                        autoSprintf( 
                                            translate( "allyCountMessage" ),
                                            count );
                                    }
                                
                                displayGlobalMessage( message );
                                delete [] message;
                                }
                            else if( commandTyped( typedText, 
                                                   "unfollowCommand" ) ) {
                                sendToServerSocket( (char*)"UNFOL 0 0#" );
                                }
                            else {
                                // filter hints
                                char *filterString = 
                                    &( typedText[ strlen( filterCommand ) ] );
                                
                                
                                if( mHintFilterString != NULL ) {
                                    delete [] mHintFilterString;
                                    mHintFilterString = NULL;
                                    }
                                
                                char *trimmedFilterString = 
                                    trimWhitespace( filterString );
                                
                                int filterStringLen = 
                                    strlen( trimmedFilterString );
                            
                                if( filterStringLen > 0 ) {
                                    // not blank
                                    mHintFilterString = 
                                        stringDuplicate( trimmedFilterString );
                                    }
                            
                                delete [] trimmedFilterString;
                            
                                mForceHintRefresh = true;
                                mNextHintIndex = 0;
                                }
                            }
                        }
                    else {
                        // actual, spoken text, not a /command
                        
                        if( strstr( typedText,
                                    translate( "orderCommand" ) ) 
                            == typedText ) {
                            
                            // when issuing an order, place +FOLLOWER+
                            // above the heads of followers
                            
                            const char *tag = translate( "followerLabel" );

                            double curTime = game_getCurrentTime();
                            for( int f=0; f<gameObjects.size(); f++ ) {
                                    
                                LiveObject *follO = 
                                    gameObjects.getElement( f );
                                if( follO->followingUs ) {
                                    if( follO->currentSpeech != NULL ) {
                                        delete [] follO->currentSpeech;
                                        follO->currentSpeech = NULL;
                                        }
                                    
                                    follO->currentSpeech = 
                                        stringDuplicate( tag );
                                    follO->speechFade = 1.0;
                                    
                                    follO->speechIsSuccessfulCurse = false;
                                    
                                    follO->speechFadeETATime =
                                        curTime + 3 +
                                        strlen( follO->currentSpeech ) / 5;
                                    follO->speechIsCurseTag = false;
                                    follO->speechIsOverheadLabel = false;
                                    }
                                }
                            }
                        
                        // send text to server

                        const char *sayCommand = "SAY";
                        
                        if( vogMode ) {
                            sayCommand = "VOGT";
                            }
                        
                        char *message = 
                            autoSprintf( "%s 0 0 %s#",
                                         sayCommand, typedText );
                        sendToServerSocket( message );
                        delete [] message;
                        }
                    
                    for( int i=0; i<mSentChatPhrases.size(); i++ ) {
                        if( strcmp( 
                                typedText, 
                                mSentChatPhrases.getElementDirect(i) ) 
                            == 0 ) {
                            
                            delete [] mSentChatPhrases.getElementDirect(i);
                            mSentChatPhrases.deleteElement(i);
                            i--;
                            }
                        }

                    mSentChatPhrases.push_back( stringDuplicate( typedText ) );

                    mSayField.setText( "" );
                    mSayField.unfocus();
                    }
                
                delete [] typedText;
                }
            else if( vogMode ) {
                // return to cursor control
                TextField::unfocusAll();
                }
            break;
        }
    }



void LivingLifePage::specialKeyDown( int inKeyCode ) {
    if( showBugMessage ) {
        return;
        }
    
    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    if( vogMode && ! TextField::isAnyFocused() ) {
        GridPos posOffset = { 0, 0 };
        
        int jump = 1;
        
        if( isCommandKeyDown() ) {
            jump *= 2;
            }
        if( isShiftKeyDown() ) {
            jump *= 5;
            }

        if( inKeyCode == MG_KEY_UP ) {
            posOffset.y = jump;
            }
        else if( inKeyCode == MG_KEY_DOWN ) {
            posOffset.y = -jump;
            }
        else if( inKeyCode == MG_KEY_LEFT ) {
            posOffset.x = -jump;
            }
        else if( inKeyCode == MG_KEY_RIGHT ) {
            posOffset.x = jump;
            }

        if( posOffset.x != 0 || posOffset.y != 0 ) {
            
            GridPos newPos;
            newPos.x = vogPos.x;
            newPos.y = vogPos.y;
            
            newPos.x += posOffset.x;
            newPos.y += posOffset.y;
            
            char *message = autoSprintf( "VOGM %d %d#",
                                         newPos.x, newPos.y );
            sendToServerSocket( message );
            delete [] message;
            }
        }
    else if( inKeyCode == MG_KEY_UP ||
        inKeyCode == MG_KEY_DOWN ) {
        if( ! mSayField.isFocused() && inKeyCode == MG_KEY_UP ) {
            if( mSentChatPhrases.size() > 0 ) {
                mSayField.setText( 
                    mSentChatPhrases.getElementDirect( 
                        mSentChatPhrases.size() - 1 ) );
                }
            else {
                mSayField.setText( "" );
                }
            mSayField.focus();
            mEKeyDown = false;
            mZKeyDown = false;
            mXKeyDown = false;
            }
        else {
            char *curText = mSayField.getText();
                
            int curBufferIndex = -1;
            
            if( strcmp( curText, "" ) != 0 ) {
                for( int i=mSentChatPhrases.size()-1; i>=0; i-- ) {
                    if( strcmp( curText, mSentChatPhrases.getElementDirect(i) )
                        == 0 ) {
                        curBufferIndex = i;
                        break;
                        }
                    }
                }
            delete [] curText;
            
            int newIndex = mSentChatPhrases.size();
            
            if( curBufferIndex != -1 ) {
                newIndex = curBufferIndex;
                
                }

            if( inKeyCode == MG_KEY_UP ) {
                newIndex --;
                }
            else {
                newIndex ++;
                }

            
            if( newIndex >= 0 ) {
                
                if( mCurrentNoteChars.size() > 0 ) {
                    // fade older erased chars 

                    for( int i=0; i<mErasedNoteCharFades.size(); i++ ) {
                        if( mErasedNoteCharFades.getElementDirect( i ) > 0.5 ) {
                            *( mErasedNoteCharFades.getElement( i ) ) -= 0.2;
                            }
                        else {
                            *( mErasedNoteCharFades.getElement( i ) ) -= 0.1;
                            }
                        
                        if( mErasedNoteCharFades.getElementDirect( i ) <= 0 ) {
                            mErasedNoteCharFades.deleteElement( i );
                            mErasedNoteChars.deleteElement( i );
                            mErasedNoteCharOffsets.deleteElement( i );
                            i--;
                            }
                        }
                    }
                
                // first, remove exact duplicates from erased
                for( int i=0; i<mCurrentNoteChars.size(); i++ ) {
                    char c = mCurrentNoteChars.getElementDirect( i );
                    doublePair pos = 
                        mCurrentNoteCharOffsets.getElementDirect( i );
                    
                    for( int j=0; j<mErasedNoteChars.size(); j++ ) {
                        if( mErasedNoteChars.getElementDirect(j) == c 
                            &&
                            equal( mErasedNoteCharOffsets.getElementDirect(j),
                                   pos ) ) {
                            
                            mErasedNoteChars.deleteElement( j );
                            mErasedNoteCharOffsets.deleteElement( j );
                            mErasedNoteCharFades.deleteElement( j );
                            j--;
                            }
                        }
                    }
                

                for( int i=0; i<mCurrentNoteChars.size(); i++ ) {
                    mErasedNoteChars.push_back( 
                        mCurrentNoteChars.getElementDirect( i ) );
                    
                    mErasedNoteCharOffsets.push_back(
                        mCurrentNoteCharOffsets.getElementDirect( i ) );
                    
                    mErasedNoteCharFades.push_back( 1.0f );
                    }
                
                if( newIndex >= mSentChatPhrases.size() ) {
                    mSayField.setText( "" );
                    }
                else {
                    mSayField.setText( 
                        mSentChatPhrases.getElementDirect( newIndex ) );
                    }
                }
            }
        }
    }


        
void LivingLifePage::keyUp( unsigned char inASCII ) {

    switch( inASCII ) {
        case 'e':
        case 'E':
            mEKeyDown = false;
            break;
        case 'z':
        case 'Z':
            mZKeyDown = false;
            break;
        case 'x':
        case 'X':
            mXKeyDown = false;
            break;
        case ' ':
            shouldMoveCamera = true;
            break;
        }

    }




void LivingLifePage::actionPerformed( GUIComponent *inTarget ) {
    if( vogMode && inTarget == &mObjectPicker ) {

        char rightClick;
        int objectID = mObjectPicker.getSelectedObject( &rightClick );
        
        if( objectID != -1 ) {
            char *message = autoSprintf( "VOGI %d %d %d#",
                                         lrint( vogPos.x ), 
                                         lrint( vogPos.y ), objectID );
            
            sendToServerSocket( message );
            
            delete [] message;
            }
        }
    }





ExtraMapObject LivingLifePage::copyFromMap( int inMapI ) {
    ExtraMapObject o = {
        mMap[ inMapI ],
        mMapMoveSpeeds[ inMapI ],
        mMapMoveOffsets[ inMapI ],
        mMapAnimationFrameCount[ inMapI ],
        mMapAnimationLastFrameCount[ inMapI ],
        mMapAnimationFrozenRotFrameCount[ inMapI ],
        mMapCurAnimType[ inMapI ],
        mMapLastAnimType[ inMapI ],
        mMapLastAnimFade[ inMapI ],
        mMapTileFlips[ inMapI ],
        mMapContainedStacks[ inMapI ],
        mMapSubContainedStacks[ inMapI ] };
    
    return o;
    }

        
void LivingLifePage::putInMap( int inMapI, ExtraMapObject *inObj ) {
    mMap[ inMapI ] = inObj->objectID;
    
    mMapMoveSpeeds[ inMapI ] = inObj->moveSpeed;
    mMapMoveOffsets[ inMapI ] = inObj->moveOffset;
    mMapAnimationFrameCount[ inMapI ] = inObj->animationFrameCount;
    mMapAnimationLastFrameCount[ inMapI ] = inObj->animationLastFrameCount;
    mMapAnimationFrozenRotFrameCount[ inMapI ] = 
        inObj->animationFrozenRotFrameCount;
    
    mMapCurAnimType[ inMapI ] = inObj->curAnimType;
    mMapLastAnimType[ inMapI ] = inObj->lastAnimType;
    mMapLastAnimFade[ inMapI ] = inObj->lastAnimFade;
    mMapTileFlips[ inMapI ] = inObj->flip;
    
    mMapContainedStacks[ inMapI ] = inObj->containedStack;
    mMapSubContainedStacks[ inMapI ] = inObj->subContainedStack;
    }



void LivingLifePage::pushOldHintArrow( int inIndex ) {
    int i = inIndex;
    if( mCurrentHintTargetPointerFade[i] > 0 ) {
        OldHintArrow h = { mLastHintTargetPos[i],
                           mCurrentHintTargetPointerBounce[i],
                           mCurrentHintTargetPointerFade[i] };
        mOldHintArrows.push_back( h );
        }
    }



char LivingLifePage::isHintFilterStringInvalid() {
    return mHintFilterString == NULL || mHintFilterStringNoMatch;
    }




static void prependLeadershipTag( LiveObject *inPlayer, const char *inPrefix ) {
    LiveObject *o = inPlayer;
    
    char *newTag;
    
    if( o->leadershipNameTag != NULL ) {
        
        newTag = autoSprintf( "%s %s", 
                              inPrefix,
                              o->leadershipNameTag );
        
        delete [] o->leadershipNameTag;
        }
    else {
        newTag = autoSprintf( "%s", inPrefix );
        }
    
    o->leadershipNameTag = newTag;
    }








#define NUM_LEADERSHIP_NAMES 8
static const char *
leadershipNameKeys[NUM_LEADERSHIP_NAMES][2] = { { "lord",
                                                  "lady" },
                                                { "baron",
                                                  "baroness" },
                                                { "count",
                                                  "countess" },
                                                { "duke",
                                                  "duchess" },
                                                { "king",
                                                  "queen" },
                                                { "emperor",
                                                  "empress" },
                                                { "highEmperor",
                                                  "highEmpress" },
                                                { "supremeEmperor",
                                                  "supremeEmpress" } };



void LivingLifePage::updateLeadership() {
    for( int i=0; i<gameObjects.size(); i++ ) {
        LiveObject *o = gameObjects.getElement( i );
        
        // reset for now
        // we will rebuild these
        o->leadershipLevel = 0;
        o->highestLeaderID = -1;
        o->hasBadge = false;
        o->isExiled = false;
        o->isDubious = false;
        o->followingUs = false;
        }


    // compute leadership levels
    char change = true;
    
    while( change ) {
        change = false;
        for( int i=0; i<gameObjects.size(); i++ ) {
            LiveObject *o = gameObjects.getElement( i );
            
            if( o->followingID != -1 ) {
                
                LiveObject *l = getGameObject( o->followingID );
                
                if( l != NULL ) {
                    if( l->leadershipLevel <= o->leadershipLevel ) {
                        l->leadershipLevel = o->leadershipLevel + 1;
                        change = true;
                        }
                    }
                }
            }
        }
    
    // now form names based on leadership levels
    for( int i=0; i<gameObjects.size(); i++ ) {
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->leadershipNameTag != NULL ) {
            delete [] o->leadershipNameTag;
            o->leadershipNameTag = NULL;
            }
        
        if( o->leadershipLevel > 0 ) {
            
            int lv = o->leadershipLevel;
            
            if( lv > NUM_LEADERSHIP_NAMES ) {
                lv = NUM_LEADERSHIP_NAMES;
                }
            
            lv -= 1;

            int gIndex = 0;
            if( ! getObject( o->displayID )->male ) {
                gIndex = 1;
                }
            o->leadershipNameTag = 
                stringDuplicate( translate( leadershipNameKeys[lv][gIndex] ) );
            }
        }

    for( int i=0; i<gameObjects.size(); i++ ) {
        LiveObject *o = gameObjects.getElement( i );

        int nextID = o->followingID;

        while( nextID != -1 ) {
            if( nextID == ourID ) {
                o->followingUs = true;
                }
            
            LiveObject *l = getGameObject( nextID );
            if( l != NULL ) {
                o->highestLeaderID = nextID;
                
                if( l->highestLeaderID != -1 ) {
                    o->highestLeaderID = l->highestLeaderID;
                    if( l->followingUs ) {
                        o->followingUs = true;
                        }
                    nextID = -1;
                    }
                else {
                    nextID = l->followingID;
                    }
                }
            else {
                nextID = -1;
                }
            }
        if( o->highestLeaderID != -1 ) {
            LiveObject *l = getGameObject( o->highestLeaderID );
            if( l != NULL ) {
                o->hasBadge = true;
                o->badgeColor = l->personalLeadershipColor;
                }
            }
        else if( o->leadershipLevel > 0 ) {
            // a leader with no other leaders above
            o->hasBadge = true;
            o->badgeColor = o->personalLeadershipColor;
            }
        }

    
    SimpleVector<int> ourLeadershipChain = getOurLeadershipChain();
    
    LiveObject *ourLiveObject = getOurLiveObject();


    // find our followers
    for( int i=0; i<gameObjects.size(); i++ ) {
        LiveObject *o = gameObjects.getElement( i );
        if( o->followingUs ) {
            
            prependLeadershipTag( o, translate( "follower" ) );
            }
        }


    
    // find exiled people.  We might see ourselves as exiled.
    for( int i=0; i<gameObjects.size(); i++ ) {
        LiveObject *o = gameObjects.getElement( i );
        
        // see if any of our leaders (or us) have this person exiled
        for( int e=0; e < o->exiledByIDs.size(); e++ ) {
            int eID = o->exiledByIDs.getElementDirect( e );
            
            if( eID == ourID ||
                ourLeadershipChain.getElementIndex( eID ) != -1 ) {
                
                o->isExiled = true;
                
                prependLeadershipTag( o, translate( "exiled" ) );
                break;
                }
            }
        }
    // now find dubious people who are following those we see as exiled
    // we can be dubious too
    for( int i=0; i<gameObjects.size(); i++ ) {
        LiveObject *o = gameObjects.getElement( i );

        if( o->isExiled ) {
            continue;
            }
        
        // not seen as exiled by us
        
        // follow their leadership chain up
        // look for exiled leaders
        int nextID = o->followingID;

        while( nextID != -1 ) {
            
            LiveObject *l = getGameObject( nextID );
            if( l != NULL ) {
                if( l->isExiled ) {
                    
                    o->isDubious = true;
                    
                    prependLeadershipTag( o, translate( "dubious" ) );
                    
                    break;
                    }
                nextID = l->followingID;
                }
            else {
                nextID = -1;
                }
            }
        }


    // add YOUR in front of our leaders and followers, even if exiled
    for( int i=0; i<ourLeadershipChain.size(); i++ ) {
        
        LiveObject *l = getGameObject( 
            ourLeadershipChain.getElementDirect( i ) );

        if( l != NULL ) {
            if( l->leadershipNameTag != NULL ) {
                
                prependLeadershipTag( l, translate( "your" ) );
                }
            }
        }
    for( int i=0; i<gameObjects.size(); i++ ) {
        LiveObject *o = gameObjects.getElement( i );
        if( o->followingUs ) {
                            
            if( o->leadershipNameTag != NULL ) {
                
                prependLeadershipTag( o, translate( "your" ) );
                }            
            }
        }


    

    // find our allies
    if( ourLiveObject->highestLeaderID != -1 ) {
        for( int i=0; i<gameObjects.size(); i++ ) {
            LiveObject *o = gameObjects.getElement( i );
            if( o != ourLiveObject && 
                o->highestLeaderID == ourLiveObject->highestLeaderID &&
                ! o->isExiled &&
                ! o->followingUs &&
                o->leadershipNameTag == NULL ) {
                
                o->leadershipNameTag = autoSprintf( "%s %s",
                                                    translate( "your" ),
                                                    translate( "ally" ) );
                }
            }
        }

    
    
    }

