
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <assert.h>
#include <float.h>


#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketServer.h"
#include "minorGems/network/SocketPoll.h"
#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"

#include "minorGems/system/Thread.h"
#include "minorGems/system/Time.h"

#include "minorGems/game/doublePair.h"

#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/log/FileLog.h"

#include "minorGems/formats/encodingUtils.h"

#include "minorGems/io/file/File.h"


#include "map.h"
#include "../gameSource/transitionBank.h"
#include "../gameSource/objectBank.h"
#include "../gameSource/animationBank.h"
#include "../gameSource/categoryBank.h"

#include "lifeLog.h"
#include "foodLog.h"
#include "backup.h"
#include "triggers.h"
#include "playerStats.h"
#include "serverCalls.h"
#include "failureLog.h"
#include "names.h"


#include "minorGems/util/random/JenkinsRandomSource.h"


static JenkinsRandomSource randSource;


#include "../gameSource/GridPos.h"


#define HEAT_MAP_D 10

float targetHeat = 10;


#define PERSON_OBJ_ID 12


int minPickupBabyAge = 10;

int babyAge = 5;

double forceDeathAge = 60;


double minSayGapInSeconds = 1.0;

int maxLineageTracked = 20;

int apocalypsePossible = 0;
char apocalypseTriggered = false;
char apocalypseRemote = false;
GridPos apocalypseLocation = { 0, 0 };
int lastApocalypseNumber = 0;
double apocalypseStartTime = 0;
char apocalypseStarted = false;

double remoteApocalypseCheckInterval = 30;
double lastRemoteApocalypseCheckTime = 0;
WebRequest *apocalypseRequest = NULL;



char monumentCallPending = false;
int monumentCallX = 0;
int monumentCallY = 0;
int monumentCallID = 0;




static double minFoodDecrementSeconds = 5.0;
static double maxFoodDecrementSeconds = 20;
static int babyBirthFoodDecrement = 10;

// bonus applied to all foods
// makes whole server a bit easier (or harder, if negative)
static int eatBonus = 0;


// keep a running sequence number to challenge each connecting client
// to produce new login hashes, avoiding replay attacks.
static unsigned int nextSequenceNumber = 1;


static int requireClientPassword = 1;
static int requireTicketServerCheck = 1;
static char *clientPassword = NULL;
static char *ticketServerURL = NULL;
static char *reflectorURL = NULL;

// larger of dataVersionNumber.txt or serverCodeVersionNumber.txt
static int versionNumber = 1;


static double childSameRaceLikelihood = 0.9;
static int familySpan = 2;


// phrases that trigger baby and family naming
static SimpleVector<char*> nameGivingPhrases;
static SimpleVector<char*> familyNameGivingPhrases;

static char *eveName = NULL;



// for incoming socket connections that are still in the login process
typedef struct FreshConnection {
        Socket *sock;
        SimpleVector<char> *sockBuffer;

        unsigned int sequenceNumber;

        WebRequest *ticketServerRequest;

        char error;
        const char *errorCauseString;
        
        char shutdownMode;

        // for tracking connections that have failed to LOGIN 
        // in a timely manner
        double connectionStartTimeSeconds;

        char *email;
    } FreshConnection;


SimpleVector<FreshConnection> newConnections;



typedef struct LiveObject {
        char *email;
        
        int id;
        
        // object ID used to visually represent this player
        int displayID;
        
        char *name;

        char isEve;        

        int parentID;

        // 0 for Eve
        int parentChainLength;

        SimpleVector<int> *lineage;
        

        // time that this life started (for computing age)
        // not actual creation time (can be adjusted to tweak starting age,
        // for example, in case of Eve who starts older).
        double lifeStartTimeSeconds;
        
        // the wall clock time when this life started
        // used for computing playtime, not age
        double trueStartTimeSeconds;
        

        double lastSayTimeSeconds;

        // held by other player?
        char heldByOther;
        int heldByOtherID;

        // player that's responsible for updates that happen to this
        // player during current step
        int responsiblePlayerID;

        // start and dest for a move
        // same if reached destination
        int xs;
        int ys;
        
        int xd;
        int yd;
        
        // next player update should be flagged
        // as a forced position change
        char posForced;
        

        int pathLength;
        GridPos *pathToDest;
        
        char pathTruncated;

        char firstMapSent;
        int lastSentMapX;
        int lastSentMapY;
        
        double moveTotalSeconds;
        double moveStartTime;
        
        int facingOverride;
        int actionAttempt;
        GridPos actionTarget;
        
        int holdingID;

        // absolute time in seconds that what we're holding should decay
        // or 0 if it never decays
        timeSec_t holdingEtaDecay;


        // where on map held object was picked up from
        char heldOriginValid;
        int heldOriginX;
        int heldOriginY;

        // if held object was created by a transition on a target, what is the
        // object ID of the target from the transition?
        int heldTransitionSourceID;
        

        int numContained;
        int *containedIDs;
        timeSec_t *containedEtaDecays;

        // vector of sub-contained for each contained item
        SimpleVector<int> *subContainedIDs;
        SimpleVector<timeSec_t> *subContainedEtaDecays;
        

        // if they've been killed and part of a weapon (bullet?) has hit them
        // this will be included in their grave
        int embeddedWeaponID;
        timeSec_t embeddedWeaponEtaDecay;
        
        // and what original weapon killed them?
        int murderSourceID;
        char holdingWound;
        

        Socket *sock;
        SimpleVector<char> *sockBuffer;

        char isNew;
        char firstMessageSent;
        
        char dying;
        // wall clock time when they will be dead
        double dyingETA;

        char error;
        const char *errorCauseString;
        
        

        int customGraveID;
        
        char *deathReason;

        char deleteSent;
        // wall clock time when we consider the delete good and sent
        // and can close their connection
        double deleteSentDoneETA;

        char deathLogged;

        char newMove;
        
        // heat map that player carries around with them
        // every time they stop moving, it is updated to compute
        // their local temp
        float heatMap[ HEAT_MAP_D * HEAT_MAP_D ];

        // heat of local object
        // map is tracked in heat units (each object produces an 
        // integer amount of heat)
        // it is mapped into 0..1 based on targetHeat to set this value here
        float heat;
        
        int foodStore;
        
        // wall clock time when we should decrement the food store
        double foodDecrementETASeconds;
        
        // should we send player a food status message
        char foodUpdate;
        
        // info about the last thing we ate, for FX food messages sent
        // just to player
        int lastAteID;
        int lastAteFillMax;
        
        // this is for PU messages sent to everyone
        char justAte;
        int justAteID;
        
        ClothingSet clothing;
        
        timeSec_t clothingEtaDecay[NUM_CLOTHING_PIECES];

        SimpleVector<int> clothingContained[NUM_CLOTHING_PIECES];
        
        SimpleVector<timeSec_t> 
            clothingContainedEtaDecays[NUM_CLOTHING_PIECES];

        char needsUpdate;
        char updateSent;

        // babies born to this player
        SimpleVector<timeSec_t> *babyBirthTimes;
        SimpleVector<int> *babyIDs;
        
        // wall clock time after which they can have another baby
        // starts at 0 (start of time epoch) for non-mothers, as
        // they can have their first baby right away.
        timeSec_t birthCoolDown;
        

        timeSec_t lastRegionLookTime;
        
    } LiveObject;



SimpleVector<LiveObject> players;



static LiveObject *getLiveObject( int inID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->id == inID ) {
            return o;
            }
        }
    
    return NULL;
    }


char *getPlayerName( int inID ) {
    LiveObject *o = getLiveObject( inID );
    if( o != NULL ) {
        return o->name;
        }
    return NULL;
    }




static double pickBirthCooldownSeconds() {
    // Kumaraswamy distribution
    // PDF:
    // k(x,a,b) = a * b * x**( a - 1 ) * (1-x**a)**(b-1)
    // CDF:
    // kCDF(x,a,b) = 1 - (1-x**a)**b
    // Invers CDF:
    // kCDFInv(y,a,b) = ( 1 - (1-y)**(1.0/b) )**(1.0/a)

    // For b=1, PDF curve starts at 0 and curves upward, for all a > 2
    // good values seem to be a=1.5, b=1

    // actually, make it more bell-curve like, with a=2, b=3

    double a = 2;
    double b = 3;
    
    // mean is around 2 minutes
    

    // uniform
    double u = randSource.getRandomDouble();
    
    // feed into inverted CDF to sample a value from the distribution
    double v = pow( 1 - pow( 1-u, (1/b) ), 1/a );
    
    // v is in [0..1], the value range of Kumaraswamy

    // put max at 5 minutes
    return v * 5 * 60;
    }




typedef struct FullMapContained{ 
        int numContained;
        int *containedIDs;
        timeSec_t *containedEtaDecays;
        SimpleVector<int> *subContainedIDs;
        SimpleVector<timeSec_t> *subContainedEtaDecays;
    } FullMapContained;



// including contained and sub contained in one call
FullMapContained getFullMapContained( int inX, int inY ) {
    FullMapContained r;
    
    r.containedIDs = getContained( inX, inY, &( r.numContained ) );
    r.containedEtaDecays = 
        getContainedEtaDecay( inX, inY, &( r.numContained ) );
    
    if( r.numContained == 0 ) {
        r.subContainedIDs = NULL;
        r.subContainedEtaDecays = NULL;
        }
    else {
        r.subContainedIDs = new SimpleVector<int>[ r.numContained ];
        r.subContainedEtaDecays = new SimpleVector<timeSec_t>[ r.numContained ];
        }
    
    for( int c=0; c< r.numContained; c++ ) {
        if( r.containedIDs[c] < 0 ) {
            
            int numSub;
            int *subContainedIDs = getContained( inX, inY, &numSub,
                                                 c + 1 );
            
            if( subContainedIDs != NULL ) {
                
                r.subContainedIDs[c].appendArray( subContainedIDs, numSub );
                delete [] subContainedIDs;
                }
            
            timeSec_t *subContainedEtaDecays = 
                getContainedEtaDecay( inX, inY, &numSub,
                                      c + 1 );

            if( subContainedEtaDecays != NULL ) {
                
                r.subContainedEtaDecays[c].appendArray( subContainedEtaDecays, 
                                                        numSub );
                delete [] subContainedEtaDecays;
                }
            }
        }
    
    return r;
    }


void setContained( LiveObject *inPlayer, FullMapContained inContained ) {
    
    inPlayer->numContained = inContained.numContained;
                                    
    inPlayer->containedIDs = inContained.containedIDs;
    
    inPlayer->containedEtaDecays =
        inContained.containedEtaDecays;
    
    inPlayer->subContainedIDs =
        inContained.subContainedIDs;
    inPlayer->subContainedEtaDecays =
        inContained.subContainedEtaDecays;
    }
    
    
    
    
void clearPlayerHeldContained( LiveObject *inPlayer ) {
    inPlayer->numContained = 0;
    
    delete [] inPlayer->containedIDs;
    delete [] inPlayer->containedEtaDecays;
    delete [] inPlayer->subContainedIDs;
    delete [] inPlayer->subContainedEtaDecays;
    
    inPlayer->containedIDs = NULL;
    inPlayer->containedEtaDecays = NULL;
    inPlayer->subContainedIDs = NULL;
    inPlayer->subContainedEtaDecays = NULL;
    }
    



void transferHeldContainedToMap( LiveObject *inPlayer, int inX, int inY ) {
    if( inPlayer->numContained != 0 ) {
        timeSec_t curTime = Time::timeSec();
        float stretch = 
            getObject( inPlayer->holdingID )->slotTimeStretch;
        
        for( int c=0;
             c < inPlayer->numContained;
             c++ ) {
            
            // undo decay stretch before adding
            // (stretch applied by adding)
            if( stretch != 1.0 &&
                inPlayer->containedEtaDecays[c] != 0 ) {
                
                timeSec_t offset = 
                    inPlayer->containedEtaDecays[c] - curTime;
                
                offset = offset * stretch;
                
                inPlayer->containedEtaDecays[c] =
                    curTime + offset;
                }

            addContained( 
                inX, inY,
                inPlayer->containedIDs[c],
                inPlayer->containedEtaDecays[c] );

            int numSub = inPlayer->subContainedIDs[c].size();
            if( numSub > 0 ) {

                int container = inPlayer->containedIDs[c];
                
                if( container < 0 ) {
                    container *= -1;
                    }
                
                float subStretch = getObject( container )->slotTimeStretch;
                    
                
                int *subIDs = 
                    inPlayer->subContainedIDs[c].getElementArray();
                timeSec_t *subDecays = 
                    inPlayer->subContainedEtaDecays[c].
                    getElementArray();
                
                for( int s=0; s < numSub; s++ ) {
                    
                    // undo decay stretch before adding
                    // (stretch applied by adding)
                    if( subStretch != 1.0 &&
                        subDecays[s] != 0 ) {
                
                        timeSec_t offset = subDecays[s] - curTime;
                        
                        offset = offset * subStretch;
                        
                        subDecays[s] = curTime + offset;
                        }

                    addContained( inX, inY,
                                  subIDs[s], subDecays[s],
                                  c + 1 );
                    }
                delete [] subIDs;
                delete [] subDecays;
                }
            }

        clearPlayerHeldContained( inPlayer );
        }
    }




// diagonal steps are longer
static double measurePathLength( int inXS, int inYS, 
                                 GridPos *inPathPos, int inPathLength ) {
    
    // diags are square root of 2 in length
    double diagLength = 1.4142356237;
    

    double totalLength = 0;
    
    GridPos lastPos = { inXS, inYS };
    
    for( int i=0; i<inPathLength; i++ ) {
        
        GridPos thisPos = inPathPos[i];
        
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



static int getLiveObjectIndex( int inID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->id == inID ) {
            return i;
            }
        }

    return -1;
    }





int nextID = 2;



void quitCleanup() {
    AppLog::info( "Cleaning up on quit..." );
    

    for( int i=0; i<newConnections.size(); i++ ) {
        FreshConnection *nextConnection = newConnections.getElement( i );
        
        delete nextConnection->sock;
        delete nextConnection->sockBuffer;
        

        if( nextConnection->ticketServerRequest != NULL ) {
            delete nextConnection->ticketServerRequest;
            }

        if( nextConnection->email != NULL ) {
            delete [] nextConnection->email;
            }
        }
    newConnections.deleteAll();
    


    for( int i=0; i<players.size(); i++ ) {
        LiveObject *nextPlayer = players.getElement(i);
        delete nextPlayer->sock;
        delete nextPlayer->sockBuffer;
        delete nextPlayer->lineage;

        if( nextPlayer->name != NULL ) {
            delete [] nextPlayer->name;
            }
        
        if( nextPlayer->email != NULL  ) {
            delete [] nextPlayer->email;
            }

        if( nextPlayer->containedIDs != NULL ) {
            delete [] nextPlayer->containedIDs;
            }

        if( nextPlayer->containedEtaDecays != NULL ) {
            delete [] nextPlayer->containedEtaDecays;
            }
        
        if( nextPlayer->subContainedIDs != NULL ) {
            delete [] nextPlayer->subContainedIDs;
            }
        
        if( nextPlayer->subContainedEtaDecays != NULL ) {
            delete [] nextPlayer->subContainedEtaDecays;
            }
        
        
        if( nextPlayer->pathToDest != NULL ) {
            delete [] nextPlayer->pathToDest;
            }
        
        if( nextPlayer->deathReason != NULL ) {
            delete [] nextPlayer->deathReason;
            }


        delete nextPlayer->babyBirthTimes;
        delete nextPlayer->babyIDs;
        }
    players.deleteAll();

    freePlayerStats();

    freeNames();
    
    freeLifeLog();
    
    freeFoodLog();
    freeFailureLog();
    
    freeTriggers();

    freeMap();

    freeTransBank();
    freeCategoryBank();
    freeObjectBank();
    freeAnimationBank();
    
    if( clientPassword != NULL ) {
        delete [] clientPassword;
        clientPassword = NULL;
        }
    

    if( ticketServerURL != NULL ) {
        delete [] ticketServerURL;
        ticketServerURL = NULL;
        }

    if( reflectorURL != NULL ) {
        delete [] reflectorURL;
        reflectorURL = NULL;
        }

    nameGivingPhrases.deallocateStringElements();
    familyNameGivingPhrases.deallocateStringElements();
    
    if( eveName != NULL ) {
        delete [] eveName;
        eveName = NULL;
        }

    if( apocalypseRequest != NULL ) {
        delete apocalypseRequest;
        apocalypseRequest = NULL;
        }
    }






volatile char quit = false;

void intHandler( int inUnused ) {
    AppLog::info( "Quit received for unix" );
    
    // since we handled this singal, we will return to normal execution
    quit = true;
    }


#ifdef WIN_32
#include <windows.h>
BOOL WINAPI ctrlHandler( DWORD dwCtrlType ) {
    if( CTRL_C_EVENT == dwCtrlType ) {
        AppLog::info( "Quit received for windows" );
        
        // will auto-quit as soon as we return from this handler
        // so cleanup now
        //quitCleanup();
        
        // seems to handle CTRL-C properly if launched by double-click
        // or batch file
        // (just not if launched directly from command line)
        quit = true;
        }
    return true;
    }
#endif


int numConnections = 0;







// reads all waiting data from socket and stores it in buffer
// returns true if socket still good, false on error
char readSocketFull( Socket *inSock, SimpleVector<char> *inBuffer ) {

    char buffer[512];
    
    int numRead = inSock->receive( (unsigned char*)buffer, 512, 0 );
    
    if( numRead == -1 ) {
        return false;
        }
    
    while( numRead > 0 ) {
        inBuffer->appendArray( buffer, numRead );

        numRead = inSock->receive( (unsigned char*)buffer, 512, 0 );
        }

    return true;
    }



// NULL if there's no full message available
char *getNextClientMessage( SimpleVector<char> *inBuffer ) {
    // find first terminal character #

    int index = inBuffer->getElementIndex( '#' );
        
    if( index == -1 ) {
        return NULL;
        }
    
    if( index > 1 && 
        inBuffer->getElementDirect( 0 ) == 'K' &&
        inBuffer->getElementDirect( 1 ) == 'A' ) {
        
        // a KA (keep alive) message
        // short-cicuit the processing here
        
        inBuffer->deleteStartElements( index + 1 );
        return NULL;
        }
    
        

    char *message = new char[ index + 1 ];
    
    // all but terminal character
    for( int i=0; i<index; i++ ) {
        message[i] = inBuffer->getElementDirect( i );
        }
    
    // delete from buffer, including terminal character
    inBuffer->deleteStartElements( index + 1 );
    
    message[ index ] = '\0';
    
    return message;
    }





typedef enum messageType {
	MOVE,
    USE,
    SELF,
    BABY,
    UBABY,
    REMV,
    SREMV,
    DROP,
    KILL,
    SAY,
    MAP,
    TRIGGER,
    BUG,
    UNKNOWN
    } messageType;




typedef struct ClientMessage {
        messageType type;
        int x, y, c, i;
        
        int trigger;
        int bug;

        // some messages have extra positions attached
        int numExtraPos;

        // NULL if there are no extra
        GridPos *extraPos;

        // null if type not SAY
        char *saidText;
        
        // null if type not BUG
        char *bugText;

    } ClientMessage;


static int pathDeltaMax = 16;


// if extraPos present in result, destroyed by caller
ClientMessage parseMessage( char *inMessage ) {
    
    char nameBuffer[100];
    
    ClientMessage m;
    
    m.i = -1;
    m.c = -1;
    m.trigger = -1;
    m.numExtraPos = 0;
    m.extraPos = NULL;
    m.saidText = NULL;
    m.bugText = NULL;
    // don't require # terminator here

    int numRead = sscanf( inMessage, 
                          "%99s %d %d", nameBuffer, &( m.x ), &( m.y ) );

    
    if( numRead >= 2 &&
        strcmp( nameBuffer, "BUG" ) == 0 ) {
        m.type = BUG;
        m.bug = m.x;
        m.bugText = stringDuplicate( inMessage );
        return m;
        }


    if( numRead != 3 ) {
        
        if( numRead == 2 &&
            strcmp( nameBuffer, "TRIGGER" ) == 0 ) {
            m.type = TRIGGER;
            m.trigger = m.x;
            }
        else {
            m.type = UNKNOWN;
            }
        
        return m;
        }
    

    if( strcmp( nameBuffer, "MOVE" ) == 0) {
        m.type = MOVE;

        SimpleVector<char *> *tokens =
            tokenizeString( inMessage );
        
        // require an odd number at least 5
        if( tokens->size() < 5 || tokens->size() % 2 != 1 ) {
            tokens->deallocateStringElements();
            delete tokens;
            
            m.type = UNKNOWN;
            return m;
            }
        
        int numTokens = tokens->size();
        
        m.numExtraPos = (numTokens - 3) / 2;
        
        m.extraPos = new GridPos[ m.numExtraPos ];

        for( int e=0; e<m.numExtraPos; e++ ) {
            
            char *xToken = tokens->getElementDirect( 3 + e * 2 );
            char *yToken = tokens->getElementDirect( 3 + e * 2 + 1 );
            
            
            sscanf( xToken, "%d", &( m.extraPos[e].x ) );
            sscanf( yToken, "%d", &( m.extraPos[e].y ) );
            
            if( abs( m.extraPos[e].x ) > pathDeltaMax ||
                abs( m.extraPos[e].y ) > pathDeltaMax ) {
                // path goes too far afield
                
                // terminate it here
                m.numExtraPos = e;
                
                if( e == 0 ) {
                    delete [] m.extraPos;
                    m.extraPos = NULL;
                    }
                break;
                }
                

            // make them absolute
            m.extraPos[e].x += m.x;
            m.extraPos[e].y += m.y;
            }
        
        tokens->deallocateStringElements();
        delete tokens;
        }
    else if( strcmp( nameBuffer, "USE" ) == 0 ) {
        m.type = USE;
        }
    else if( strcmp( nameBuffer, "SELF" ) == 0 ) {
        m.type = SELF;

        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "UBABY" ) == 0 ) {
        m.type = UBABY;

        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "BABY" ) == 0 ) {
        m.type = BABY;
        }
    else if( strcmp( nameBuffer, "SREMV" ) == 0 ) {
        m.type = SREMV;
        
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.c ),
                          &( m.i ) );
        
        if( numRead != 5 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "REMV" ) == 0 ) {
        m.type = REMV;
        
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "DROP" ) == 0 ) {
        m.type = DROP;
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.c ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else if( strcmp( nameBuffer, "KILL" ) == 0 ) {
        m.type = KILL;
        }
    else if( strcmp( nameBuffer, "MAP" ) == 0 ) {
        m.type = MAP;
        }
    else if( strcmp( nameBuffer, "SAY" ) == 0 ) {
        m.type = SAY;

        // look after second space
        char *firstSpace = strstr( inMessage, " " );
        
        if( firstSpace != NULL ) {
            
            char *secondSpace = strstr( &( firstSpace[1] ), " " );
            
            if( secondSpace != NULL ) {

                char *thirdSpace = strstr( &( secondSpace[1] ), " " );
                
                if( thirdSpace != NULL ) {
                    m.saidText = stringDuplicate( &( thirdSpace[1] ) );
                    }
                }
            }
        }
    else {
        m.type = UNKNOWN;
        }
    
    return m;
    }



// compute closest starting position part way along
// path
// (-1 if closest spot is starting spot not included in path steps)
int computePartialMovePathStep( LiveObject *inPlayer ) {
    
    double fractionDone = 
        ( Time::getCurrentTime() - 
          inPlayer->moveStartTime )
        / inPlayer->moveTotalSeconds;
    
    if( fractionDone > 1 ) {
        fractionDone = 1;
        }
    
    int c = 
        lrint( ( inPlayer->pathLength ) *
               fractionDone );
    return c - 1;
    }



GridPos computePartialMoveSpot( LiveObject *inPlayer ) {

    int c = computePartialMovePathStep( inPlayer );

    if( c >= 0 ) {
        
        GridPos cPos = inPlayer->pathToDest[c];
        
        return cPos;
        }
    else {
        GridPos cPos = { inPlayer->xs, inPlayer->ys };
        
        return cPos;
        }
    }



GridPos killPlayer( const char *inEmail ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( strcmp( o->email, inEmail ) == 0 ) {
            o->error = true;
            
            return computePartialMoveSpot( o );
            }
        }
    
    GridPos noPos = { 0, 0 };
    return noPos;
    }



void forcePlayerAge( const char *inEmail, double inAge ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( strcmp( o->email, inEmail ) == 0 ) {
            double ageSec = inAge / getAgeRate();
            
            o->lifeStartTimeSeconds = Time::getCurrentTime() - ageSec;
            o->needsUpdate = true;
            }
        }
    }





double computeFoodDecrementTimeSeconds( LiveObject *inPlayer ) {
    double value = maxFoodDecrementSeconds * 2 * inPlayer->heat;
    
    if( value > maxFoodDecrementSeconds ) {
        // also reduce if too hot (above 0.5 heat)
        
        double extra = value - maxFoodDecrementSeconds;

        value = maxFoodDecrementSeconds - extra;
        }
    
    // all player temp effects push us up above min
    value += minFoodDecrementSeconds;

    return value;
    }


double getAgeRate() {
    return 1.0 / 60.0;
    }


static void setDeathReason( LiveObject *inPlayer, const char *inTag,
                            int inOptionalID = 0 ) {
    
    if( inPlayer->deathReason != NULL ) {
        delete [] inPlayer->deathReason;
        }
    
    // leave space in front so it works at end of PU line
    if( strcmp( inTag, "killed" ) == 0 ) {
        
        inPlayer->deathReason = autoSprintf( " reason_%s_%d", 
                                             inTag, inOptionalID );
        }
    else {
        // ignore ID
        inPlayer->deathReason = autoSprintf( " reason_%s", inTag );
        }
    }



int longestShutdownLine = -1;

void handleShutdownDeath( LiveObject *inPlayer,
                          int inX, int inY ) {
    if( inPlayer->parentChainLength > longestShutdownLine ) {
        longestShutdownLine = inPlayer->parentChainLength;
        
        FILE *f = fopen( "shutdownLongLineagePos.txt", "w" );
        if( f != NULL ) {
            fprintf( f, "%d,%d", inX, inY );
            fclose( f );
            }
        }
    }



double computeAge( LiveObject *inPlayer ) {
    double deltaSeconds = 
        Time::getCurrentTime() - inPlayer->lifeStartTimeSeconds;
    
    double age = deltaSeconds * getAgeRate();
    
    if( age >= forceDeathAge ) {
        setDeathReason( inPlayer, "age" );
        
        inPlayer->error = true;
        }
    return age;
    }



int getSayLimit( LiveObject *inPlayer ) {
    int limit = (unsigned int)( floor( computeAge( inPlayer ) ) + 1 );

    if( inPlayer->isEve && limit < 30 ) {
        // give Eve room to name her family line
        limit = 30;
        }
    return limit;
    }




int getSecondsPlayed( LiveObject *inPlayer ) {
    double deltaSeconds = 
        Time::getCurrentTime() - inPlayer->trueStartTimeSeconds;

    return lrint( deltaSeconds );
    }


// false for male, true for female
char getFemale( LiveObject *inPlayer ) {
    ObjectRecord *r = getObject( inPlayer->displayID );
    
    return ! r->male;
    }



char isFertileAge( LiveObject *inPlayer ) {
    double age = computeAge( inPlayer );
                    
    char f = getFemale( inPlayer );
                    
    if( age >= 14 && age <= 40 && f ) {
        return true;
        }
    else {
        return false;
        }
    }




int computeFoodCapacity( LiveObject *inPlayer ) {
    int ageInYears = lrint( computeAge( inPlayer ) );
    
    if( ageInYears < 44 ) {
        
        if( ageInYears > 16 ) {
            ageInYears = 16;
            }
        
        return ageInYears + 4;
        }
    else {
        // food capacity decreases as we near 60
        int cap = 60 - ageInYears + 4;
        
        if( cap < 4 ) {
            cap = 4;
            }
        
        return cap;
        }
    }


double computeMoveSpeed( LiveObject *inPlayer ) {
    double age = computeAge( inPlayer );
    

    // with 128-wide tiles, character moves at 480 pixels per second
    // at 60 fps, this is 8 pixels per frame
    // important that it's a whole number of pixels for smooth camera movement
    double speed = 3.75;
    
    // baby moves at 360 pixels per second, or 6 pixels per frame
    double babySpeedFactor = 0.75;

    double fullSpeedAge = 10.0;
    

    if( age < fullSpeedAge ) {
        
        double speedFactor = babySpeedFactor + 
            ( 1.0 - babySpeedFactor ) * age / fullSpeedAge;
        
        speed *= speedFactor;
        }


    // for now, try no age-based speed decrease
    /*
    if( age < 20 ) {
        speed *= age / 20;
        }
    if( age > 40 ) {
        // half speed by 60, then keep slowing down after that
        speed -= (age - 40 ) * 2.0 / 20.0;
        
        }
    */
    // no longer slow down with hunger
    /*
    int foodCap = computeFoodCapacity( inPlayer );
    
    
    if( inPlayer->foodStore <= foodCap / 2 ) {
        // jumps instantly to 1/2 speed at half food, then decays after that
        speed *= inPlayer->foodStore / (double) foodCap;
        }
    */


    // never move at 0 speed, divide by 0 errors for eta times
    if( speed < 0.1 ) {
        speed = 0.1;
        }

    // apply character's speed mult
    speed *= getObject( inPlayer->displayID )->speedMult;
    

    char riding = false;
    
    if( inPlayer->holdingID > 0 ) {
        ObjectRecord *r = getObject( inPlayer->holdingID );

        if( r->clothing == 'n' ) {
            // clothing only changes your speed when it's worn
            speed *= r->speedMult;
            }
        
        if( r->rideable ) {
            riding = true;
            }
        }
    

    if( !riding ) {
        // clothing can affect speed

        for( int i=0; i<NUM_CLOTHING_PIECES; i++ ) {
            ObjectRecord *c = clothingByIndex( inPlayer->clothing, i );
            
            if( c != NULL ) {
                
                speed *= c->speedMult;
                }
            }
        }
    
    // after all multipliers, make sure it's a whole number of pixels per frame

    double pixelsPerFrame = speed * 128.0 / 60.0;
    
    
    if( pixelsPerFrame > 0.5 ) {
        // can round to at least one pixel per frame
        pixelsPerFrame = lrint( pixelsPerFrame );
        }
    else {
        // fractional pixels per frame
        
        // ensure a whole number of frames per pixel
        double framesPerPixel = 1.0 / pixelsPerFrame;
        
        framesPerPixel = lrint( framesPerPixel );
        
        pixelsPerFrame = 1.0 / framesPerPixel;
        }
    
    speed = pixelsPerFrame * 60 / 128.0;
        
    return speed;
    }



    
    
// returns NULL if there are no matching moves
char *getMovesMessage( char inNewMovesOnly, 
                       SimpleVector<ChangePosition> *inChangeVector = NULL,
                       int inOneIDOnly = -1 ) {
    
    SimpleVector<char> messageBuffer;

    messageBuffer.appendElementString( "PM\n" );

    int numPlayers = players.size();
                
    
    int numLines = 0;

    for( int i=0; i<numPlayers; i++ ) {
                
        LiveObject *o = players.getElement( i );                
        
        if( o->error ) {
            continue;
            }

        if( ( o->xd != o->xs || o->yd != o->ys )
            &&
            ( o->newMove || !inNewMovesOnly ) 
            && ( inOneIDOnly == -1 || inOneIDOnly == o->id ) ) {

 
            // p_id xs ys xd yd fraction_done eta_sec
            
            double deltaSec = Time::getCurrentTime() - o->moveStartTime;
            
            double etaSec = o->moveTotalSeconds - deltaSec;
            
            if( etaSec < 0 ) {
                etaSec = 0;
                }

            if( inNewMovesOnly ) {
                o->newMove = false;
                }
            
            
            
            SimpleVector<char> messageLineBuffer;
        
            // start is absolute
            char *startString = autoSprintf( "%d %d %d %.3f %.3f %d", 
                                             o->id, 
                                             o->xs, o->ys, 
                                             o->moveTotalSeconds, etaSec,
                                             o->pathTruncated );
            // mark that this has been sent
            o->pathTruncated = false;
            
            messageLineBuffer.appendElementString( startString );
            delete [] startString;
            
            for( int p=0; p<o->pathLength; p++ ) {
                // rest are relative to start
                char *stepString = autoSprintf( " %d %d", 
                                                o->pathToDest[p].x
                                                - o->xs,
                                                o->pathToDest[p].y
                                                - o->ys );
                
                messageLineBuffer.appendElementString( stepString );
                delete [] stepString;
                }
        
            messageLineBuffer.appendElementString( "\n" );

            char *messageLine = messageLineBuffer.getElementString();
                                    
            messageBuffer.appendElementString( messageLine );
            delete [] messageLine;
            
            if( inChangeVector != NULL ) {
                ChangePosition p = { o->xd, o->yd, false };
                inChangeVector->push_back( p );
                }

            numLines ++;
            
            }
        }
    
        
    if( numLines > 0 ) {
        
        messageBuffer.push_back( '#' );
                
        char *message = messageBuffer.getElementString();
        
        return message;
        }
    
    return NULL;
    
    }



static char isGridAdjacent( int inXA, int inYA, int inXB, int inYB ) {
    if( ( abs( inXA - inXB ) == 1 && inYA == inYB ) 
        ||
        ( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {
        
        return true;
        }

    return false;
    }


//static char isGridAdjacent( GridPos inA, GridPos inB ) {
//    return isGridAdjacent( inA.x, inA.y, inB.x, inB.y );
//    }


static char isGridAdjacentDiag( int inXA, int inYA, int inXB, int inYB ) {
    if( isGridAdjacent( inXA, inYA, inXB, inYB ) ) {
        return true;
        }
    
    if( abs( inXA - inXB ) == 1 && abs( inYA - inYB ) ) {
        return true;
        }
    
    return false;
    }


static char isGridAdjacentDiag( GridPos inA, GridPos inB ) {
    return isGridAdjacentDiag( inA.x, inA.y, inB.x, inB.y );
    }



static char equal( GridPos inA, GridPos inB ) {
    if( inA.x == inB.x && inA.y == inB.y ) {
        return true;
        }
    return false;
    }


static double distance( GridPos inA, GridPos inB ) {
    return sqrt( ( inA.x - inB.x ) * ( inA.x - inB.x )
                 +
                 ( inA.y - inB.y ) * ( inA.y - inB.y ) );
    }



// returns (0,0) if no player found
GridPos getClosestPlayerPos( int inX, int inY ) {
    GridPos c = { inX, inY };
    
    double closeDist = DBL_MAX;
    GridPos closeP = { 0, 0 };
    
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        if( o->error ) {
            continue;
            }
        
        GridPos p;

        if( o->xs == o->xd && o->ys == o->yd ) {
            p.x = o->xd;
            p.y = o->yd;
            }
        else {
            p = computePartialMoveSpot( o );
            }
        
        double d = distance( p, c );
        
        if( d < closeDist ) {
            closeDist = d;
            closeP = p;
            }
        }
    return closeP;
    }




static int chunkDimensionX = 32;
static int chunkDimensionY = 30;


static int getMaxChunkDimension() {
    return chunkDimensionX;
    }




// sets lastSentMap in inO if chunk goes through
// returns result of send, auto-marks error in inO
int sendMapChunkMessage( LiveObject *inO, 
                         char inDestOverride = false,
                         int inDestOverrideX = 0, 
                         int inDestOverrideY = 0 ) {
    
    
    int messageLength = 0;

    int xd = inO->xd;
    int yd = inO->yd;
    
    if( inDestOverride ) {
        xd = inDestOverrideX;
        yd = inDestOverrideY;
        }
    
    
    int halfW = chunkDimensionX / 2;
    int halfH = chunkDimensionY / 2;
    
    int fullStartX = xd - halfW;
    int fullStartY = yd - halfH;
    
    int numSent = 0;

    

    if( ! inO->firstMapSent ) {
        // send full rect centered on x,y
        
        inO->firstMapSent = true;
        
        unsigned char *mapChunkMessage = getChunkMessage( fullStartX,
                                                          fullStartY,
                                                          chunkDimensionX,
                                                          chunkDimensionY,
                                                          &messageLength );
                
        numSent += 
            inO->sock->send( mapChunkMessage, 
                             messageLength, 
                             false, false );
                
        delete [] mapChunkMessage;
        }
    else {
        
        // our closest previous chunk center
        int lastX = inO->lastSentMapX;
        int lastY = inO->lastSentMapY;


        // split next chunk into two bars by subtracting last chunk
        
        int horBarStartX = fullStartX;
        int horBarStartY = fullStartY;
        int horBarW = chunkDimensionX;
        int horBarH = chunkDimensionY;
        
        if( yd > lastY ) {
            // remove bottom of bar
            horBarStartY = lastY + halfH;
            horBarH = yd - lastY;
            }
        else {
            // remove top of bar
            horBarH = lastY - yd;
            }
        

        int vertBarStartX = fullStartX;
        int vertBarStartY = fullStartY;
        int vertBarW = chunkDimensionX;
        int vertBarH = chunkDimensionY;
        
        if( xd > lastX ) {
            // remove left part of bar
            vertBarStartX = lastX + halfW;
            vertBarW = xd - lastX;
            }
        else {
            // remove right part of bar
            vertBarW = lastX - xd;
            }
        
        // now trim vert bar where it intersects with hor bar
        if( yd > lastY ) {
            // remove top of vert bar
            vertBarH -= horBarH;
            }
        else {
            // remove bottom of vert bar
            vertBarStartY = horBarStartY + horBarH;
            vertBarH -= horBarH;
            }
        
        
        // only send if non-zero width and height
        if( horBarW > 0 && horBarH > 0 ) {
            int len;
            unsigned char *mapChunkMessage = getChunkMessage( horBarStartX,
                                                              horBarStartY,
                                                              horBarW,
                                                              horBarH,
                                                              &len );
            messageLength += len;
            
            numSent += 
                inO->sock->send( mapChunkMessage, 
                                 len, 
                                 false, false );
            
            delete [] mapChunkMessage;
            }
        if( vertBarW > 0 && vertBarH > 0 ) {
            int len;
            unsigned char *mapChunkMessage = getChunkMessage( vertBarStartX,
                                                              vertBarStartY,
                                                              vertBarW,
                                                              vertBarH,
                                                              &len );
            messageLength += len;
            
            numSent += 
                inO->sock->send( mapChunkMessage, 
                                 len, 
                                 false, false );
            
            delete [] mapChunkMessage;
            }
        }
    
    
                

    if( numSent == messageLength ) {
        // sent correctly
        inO->lastSentMapX = xd;
        inO->lastSentMapY = yd;
        }
    else {
        setDeathReason( inO, "disconnected" );
        
        inO->error = true;
        inO->errorCauseString = "Socket write failed";
        }
    return numSent;
    }



double intDist( int inXA, int inYA, int inXB, int inYB ) {
    int dx = inXA - inXB;
    int dy = inYA - inYB;

    return sqrt(  dx * dx + dy * dy );
    }




char *getHoldingString( LiveObject *inObject ) {
    if( inObject->numContained == 0 ) {
        return autoSprintf( "%d", inObject->holdingID );
        }

    
    SimpleVector<char> buffer;
    

    char *idString = autoSprintf( "%d", inObject->holdingID );
    
    buffer.appendElementString( idString );
    
    delete [] idString;
    
    
    if( inObject->numContained > 0 ) {
        for( int i=0; i<inObject->numContained; i++ ) {
            
            char *idString = autoSprintf( ",%d", 
                                          abs( inObject->containedIDs[i] ) );
    
            buffer.appendElementString( idString );
    
            delete [] idString;

            if( inObject->subContainedIDs[i].size() > 0 ) {
                for( int s=0; s<inObject->subContainedIDs[i].size(); s++ ) {
                    
                    idString = autoSprintf( 
                        ":%d", 
                        inObject->subContainedIDs[i].getElementDirect( s ) );
    
                    buffer.appendElementString( idString );
                
                    delete [] idString;
                    }
                }
            }
        }
    
    return buffer.getElementString();
    }



// only consider living, non-moving players
char isMapSpotEmptyOfPlayers( int inX, int inY ) {

    int numLive = players.size();
    
    for( int i=0; i<numLive; i++ ) {
        LiveObject *nextPlayer = players.getElement( i );
        
        if( // not about to be deleted
            ! nextPlayer->error &&
            // held players aren't on map (their coordinates are stale)
            ! nextPlayer->heldByOther &&
            // stationary
            nextPlayer->xs == nextPlayer->xd &&
            nextPlayer->ys == nextPlayer->yd &&
            // in this spot
            inX == nextPlayer->xd &&
            inY == nextPlayer->yd ) {
            return false;            
            } 
        }
    
    return true;
    }




// checks both grid of objects and live, non-moving player positions
char isMapSpotEmpty( int inX, int inY, char inConsiderPlayers = true ) {
    int target = getMapObject( inX, inY );
    
    if( target != 0 ) {
        return false;
        }
    
    if( !inConsiderPlayers ) {
        return true;
        }
    
    return isMapSpotEmptyOfPlayers( inX, inY );
    }



static void setFreshEtaDecayForHeld( LiveObject *inPlayer ) {
    
    if( inPlayer->holdingID == 0 ) {
        inPlayer->holdingEtaDecay = 0;
        }
    
    // does newly-held object have a decay defined?
    TransRecord *newDecayT = getTrans( -1, inPlayer->holdingID );
                    
    if( newDecayT != NULL ) {
        inPlayer->holdingEtaDecay = 
            Time::timeSec() + newDecayT->autoDecaySeconds;
        }
    else {
        // no further decay
        inPlayer->holdingEtaDecay = 0;
        }
    }



void handleMapChangeToPaths( 
    int inX, int inY, ObjectRecord *inNewObject,
    SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    if( inNewObject->blocksWalking ) {
    
        GridPos dropSpot = { inX, inY };
          
        int numLive = players.size();
                      
        for( int j=0; j<numLive; j++ ) {
            LiveObject *otherPlayer = 
                players.getElement( j );
            
            if( otherPlayer->error ) {
                continue;
                }

            if( otherPlayer->xd != otherPlayer->xs ||
                otherPlayer->yd != otherPlayer->ys ) {
                
                GridPos cPos = 
                    computePartialMoveSpot( otherPlayer );
                                        
                if( distance( cPos, dropSpot ) 
                    <= 2 * pathDeltaMax ) {
                                            
                    // this is close enough
                    // to this path that it might
                    // block it
                
                    int c = computePartialMovePathStep( otherPlayer );

                    // -1 means starting, pre-path pos is closest
                    // push it up to first path step
                    if( c < 0 ) {
                        c = 0;
                        }

                    char blocked = false;
                    int blockedStep = -1;
                                            
                    for( int p=c; 
                         p<otherPlayer->pathLength;
                         p++ ) {
                                                
                        if( equal( 
                                otherPlayer->
                                pathToDest[p],
                                dropSpot ) ) {
                                                    
                            blocked = true;
                            blockedStep = p;
                            break;
                            }
                        }
                                            
                    if( blocked ) {
                        printf( 
                            "  Blocked by drop\n" );
                        }
                                            

                    if( blocked &&
                        blockedStep > 0 ) {
                                                
                        otherPlayer->pathLength
                            = blockedStep;
                        otherPlayer->pathTruncated
                            = true;

                        // update timing
                        double dist = 
                            measurePathLength( otherPlayer->xs,
                                               otherPlayer->ys,
                                               otherPlayer->pathToDest,
                                               otherPlayer->pathLength );    
                                                
                        double distAlreadyDone =
                            measurePathLength( otherPlayer->xs,
                                               otherPlayer->ys,
                                               otherPlayer->pathToDest,
                                               c );
                            
                        double moveSpeed = computeMoveSpeed( otherPlayer );

                        otherPlayer->moveTotalSeconds 
                            = 
                            dist / 
                            moveSpeed;
                            
                        double secondsAlreadyDone = 
                            distAlreadyDone / 
                            moveSpeed;
                                
                        otherPlayer->moveStartTime = 
                            Time::getCurrentTime() - 
                            secondsAlreadyDone;
                            
                        otherPlayer->newMove = true;
                                                
                        otherPlayer->xd 
                            = otherPlayer->pathToDest[
                                blockedStep - 1].x;
                        otherPlayer->yd 
                            = otherPlayer->pathToDest[
                                blockedStep - 1].y;
                                                
                        }
                    else if( blocked ) {
                        // cutting off path
                        // right at the beginning
                        // nothing left

                        // end move now
                        otherPlayer->xd = 
                            otherPlayer->xs;
                                                
                        otherPlayer->yd = 
                            otherPlayer->ys;
                             
                        otherPlayer->posForced = true;
                    
                        inPlayerIndicesToSendUpdatesAbout->push_back( j );
                        }
                    } 
                                        
                }                                    
            }
        }
    
    }



// returns true if found
char findDropSpot( int inX, int inY, int inSourceX, int inSourceY, 
                   GridPos *outSpot ) {
    char found = false;
    int foundX = inX;
    int foundY = inY;
    
    // change direction of throw
    // to match direction of 
    // drop action
    int xDir = inX - inSourceX;
    int yDir = inY - inSourceY;
                                    
        
    if( xDir == 0 && yDir == 0 ) {
        xDir = 1;
        }
    
    // cap to magnitude
    // death drops can be non-adjacent
    if( xDir > 1 ) {
        xDir = 1;
        }
    if( xDir < -1 ) {
        xDir = -1;
        }
    
    if( yDir > 1 ) {
        yDir = 1;
        }
    if( yDir < -1 ) {
        yDir = -1;
        }
        

    // check in y dir first at each
    // expanded radius?
    char yFirst = false;
        
    if( yDir != 0 ) {
        yFirst = true;
        }
        
    for( int d=1; d<10 && !found; d++ ) {
            
        char doneY0 = false;
            
        for( int yD = -d; yD<=d && !found; 
             yD++ ) {
                
            if( ! doneY0 ) {
                yD = 0;
                }
                
            if( yDir != 0 ) {
                yD *= yDir;
                }
                
            char doneX0 = false;
                
            for( int xD = -d; 
                 xD<=d && !found; 
                 xD++ ) {
                    
                if( ! doneX0 ) {
                    xD = 0;
                    }
                    
                if( xDir != 0 ) {
                    xD *= xDir;
                    }
                    
                    
                if( yD == 0 && xD == 0 ) {
                    if( ! doneX0 ) {
                        doneX0 = true;
                            
                        // back up in loop
                        xD = -d - 1;
                        }
                    continue;
                    }
                                                
                int x = 
                    inSourceX + xD;
                int y = 
                    inSourceY + yD;
                                                
                if( yFirst ) {
                    // swap them
                    // to reverse order
                    // of expansion
                    x = 
                        inSourceX + yD;
                    y =
                        inSourceY + xD;
                    }
                                                


                if( 
                    isMapSpotEmpty( x, y ) ) {
                                                    
                    found = true;
                    foundX = x;
                    foundY = y;
                    }
                                                    
                if( ! doneX0 ) {
                    doneX0 = true;
                                                        
                    // back up in loop
                    xD = -d - 1;
                    }
                }
                                                
            if( ! doneY0 ) {
                doneY0 = true;
                                                
                // back up in loop
                yD = -d - 1;
                }
            }
        }

    outSpot->x = foundX;
    outSpot->y = foundY;
    return found;
    }



// drops an object held by a player at target x,y location
// doesn't check for adjacency (so works for thrown drops too)
// if target spot blocked, will search for empty spot to throw object into
void handleDrop( int inX, int inY, LiveObject *inDroppingPlayer,
                 SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    int oldHoldingID = inDroppingPlayer->holdingID;
    

    if( oldHoldingID > 0 &&
        getObject( oldHoldingID )->permanent ) {
        // what they are holding is stuck in their
        // hand

        // see if a use-on-bare-ground drop 
        // action applies (example:  dismounting
        // a horse)
                            
        // note that if use on bare ground
        // also has a new actor, that will be lost
        // in this process.
        // (example:  holding a roped lamb when dying,
        //            lamb is dropped, rope is lost)

        TransRecord *bareTrans =
            getTrans( oldHoldingID, -1 );
                            
        if( bareTrans != NULL &&
            bareTrans->newTarget > 0 ) {
                            
            oldHoldingID = bareTrans->newTarget;
            
            inDroppingPlayer->holdingID = 
                bareTrans->newTarget;

            setFreshEtaDecayForHeld( inDroppingPlayer );
            }
        }

    int targetX = inX;
    int targetY = inY;

    int mapID = getMapObject( inX, inY );
    char mapSpotBlocking = false;
    if( mapID > 0 ) {
        mapSpotBlocking = getObject( mapID )->blocksWalking;
        }
    

    if( ( inDroppingPlayer->holdingID < 0 && mapSpotBlocking )
        ||
        ( inDroppingPlayer->holdingID > 0 && mapID != 0 ) ) {
        
        // drop spot blocked
        // search for another
        // throw held into nearest empty spot
                                    
        
        GridPos spot;
        
        char found = findDropSpot( inX, inY, 
                                   inDroppingPlayer->xd, inDroppingPlayer->yd,
                                   &spot );
        
        int foundX = spot.x;
        int foundY = spot.y;



        if( found ) {
            targetX = foundX;
            targetY = foundY;
            }
        else {
            // no place to drop it, it disappears

            // UNLESS we're holding a baby,
            // then just put the baby where we are
            if( inDroppingPlayer->holdingID < 0 ) {
                int babyID = - inDroppingPlayer->holdingID;
                
                LiveObject *babyO = getLiveObject( babyID );
                
                if( babyO != NULL ) {
                    babyO->xd = inDroppingPlayer->xd;
                    babyO->xs = inDroppingPlayer->xd;
                    
                    babyO->yd = inDroppingPlayer->yd;
                    babyO->ys = inDroppingPlayer->yd;

                    babyO->heldByOther = false;

                    if( isFertileAge( inDroppingPlayer ) ) {    
                        // reset food decrement time
                        babyO->foodDecrementETASeconds =
                            Time::getCurrentTime() +
                            computeFoodDecrementTimeSeconds( babyO );
                        }
                    
                    inPlayerIndicesToSendUpdatesAbout->push_back( 
                        getLiveObjectIndex( babyID ) );
                    }
                
                }
            
            inDroppingPlayer->holdingID = 0;
            inDroppingPlayer->holdingEtaDecay = 0;
            inDroppingPlayer->heldOriginValid = 0;
            inDroppingPlayer->heldOriginX = 0;
            inDroppingPlayer->heldOriginY = 0;
            inDroppingPlayer->heldTransitionSourceID = -1;
            
            if( inDroppingPlayer->numContained != 0 ) {
                clearPlayerHeldContained( inDroppingPlayer );
                }
            return;
            }            
        }
    
    
    if( inDroppingPlayer->holdingID < 0 ) {
        // dropping a baby
        
        int babyID = - inDroppingPlayer->holdingID;
                
        LiveObject *babyO = getLiveObject( babyID );
        
        if( babyO != NULL ) {
            babyO->xd = targetX;
            babyO->xs = targetX;
                    
            babyO->yd = targetY;
            babyO->ys = targetY;
            
            babyO->heldByOther = false;

            if( isFertileAge( inDroppingPlayer ) ) {    
                // reset food decrement time
                babyO->foodDecrementETASeconds =
                    Time::getCurrentTime() +
                    computeFoodDecrementTimeSeconds( babyO );
                }

            inPlayerIndicesToSendUpdatesAbout->push_back( 
                getLiveObjectIndex( babyID ) );
            }
        
        inDroppingPlayer->holdingID = 0;
        inDroppingPlayer->holdingEtaDecay = 0;
        inDroppingPlayer->heldOriginValid = 0;
        inDroppingPlayer->heldOriginX = 0;
        inDroppingPlayer->heldOriginY = 0;
        inDroppingPlayer->heldTransitionSourceID = -1;
        
        return;
        }
    
    setResponsiblePlayer( inDroppingPlayer->id );
    
    setMapObject( targetX, targetY, inDroppingPlayer->holdingID );
    setEtaDecay( targetX, targetY, inDroppingPlayer->holdingEtaDecay );

    transferHeldContainedToMap( inDroppingPlayer, targetX, targetY );
    
                                

    setResponsiblePlayer( -1 );
                                
    inDroppingPlayer->holdingID = 0;
    inDroppingPlayer->holdingEtaDecay = 0;
    inDroppingPlayer->heldOriginValid = 0;
    inDroppingPlayer->heldOriginX = 0;
    inDroppingPlayer->heldOriginY = 0;
    inDroppingPlayer->heldTransitionSourceID = -1;
                                
    // watch out for truncations of in-progress
    // moves of other players
            
    ObjectRecord *droppedObject = getObject( oldHoldingID );
   
    handleMapChangeToPaths( targetX, targetY, droppedObject,
                            inPlayerIndicesToSendUpdatesAbout );
    
    }



LiveObject *getAdultHolding( LiveObject *inBabyObject ) {
    int numLive = players.size();
    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *adultO = players.getElement( j );

        if( - adultO->holdingID == inBabyObject->id ) {
            return adultO;
            }
        }
    return NULL;
    }



void handleForcedBabyDrop( 
    LiveObject *inBabyObject,
    SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    int numLive = players.size();
    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *adultO = players.getElement( j );

        if( - adultO->holdingID == inBabyObject->id ) {

            // don't need to send update about this adult's
            // holding status.
            // the update sent about the baby will inform clients
            // that the baby is no longer held by this adult
            //inPlayerIndicesToSendUpdatesAbout->push_back( j );
            
            GridPos dropPos;
            
            if( adultO->xd == 
                adultO->xs &&
                adultO->yd ==
                adultO->ys ) {
                
                dropPos.x = adultO->xd;
                dropPos.y = adultO->yd;
                }
            else {
                dropPos = 
                    computePartialMoveSpot( adultO );
                }
            
            
            handleDrop( 
                dropPos.x, dropPos.y, 
                adultO,
                inPlayerIndicesToSendUpdatesAbout );

            
            break;
            }
        }
    }




char isMapSpotBlocking( int inX, int inY ) {
    
    int target = getMapObject( inX, inY );

    if( target != 0 ) {
        ObjectRecord *obj = getObject( target );
    
        if( obj->blocksWalking ) {
            return true;
            }
        }
    
    // not directly blocked
    // need to check for wide objects to left and right
    int maxR = getMaxWideRadius();
    
    for( int dx = -maxR; dx <= maxR; dx++ ) {
        
        if( dx != 0 ) {
            
            int nX = inX + dx;
        
            int nID = getMapObject( nX, inY );
            
            if( nID != 0 ) {
                ObjectRecord *nO = getObject( nID );
                
                if( nO->wide ) {
                    
                    int dist;
                    int minDist;
                    
                    if( dx < 0 ) {
                        dist = -dx;
                        minDist = nO->rightBlockingRadius;
                        }
                    else {
                        dist = dx;
                        minDist = nO->leftBlockingRadius;
                        }
                    
                    if( dist <= minDist ) {
                        return true;
                        }
                    }
                }
            }
        }
    
    return false;
    }






// returns 0 for NULL
static int objectRecordToID( ObjectRecord *inRecord ) {
    if( inRecord == NULL ) {
        return 0;
        }
    else {
        return inRecord->id;
        }
    }


// inDelete true to send X X for position
// inPartial gets update line for player's current possition mid-path
static char *getUpdateLine( LiveObject *inPlayer, char inDelete,
                            char inPartial = false ) {

    char *holdingString = getHoldingString( inPlayer );
    
    int doneMoving = 0;
    
    if( inPlayer->xs == inPlayer->xd &&
        inPlayer->ys == inPlayer->yd &&
        ! inPlayer->heldByOther ) {
        doneMoving = 1;
        }
    
        

    char *posString;
    if( inDelete ) {
        posString = stringDuplicate( "0 0 X X" );
        }
    else {
        int x, y;

        if( doneMoving || ! inPartial ) {
            x = inPlayer->xs;
            y = inPlayer->ys;
            }
        else {
            // mid-move, and partial position requested
            GridPos p = computePartialMoveSpot( inPlayer );
            
            x = p.x;
            y = p.y;
            }
        
        posString = autoSprintf( "%d %d %d %d",          
                                 doneMoving,
                                 inPlayer->posForced,
                                 x, 
                                 y );
        }
    
    SimpleVector<char> clothingListBuffer;
    
    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        ObjectRecord *cObj = clothingByIndex( inPlayer->clothing, c );
        int id = 0;
        
        if( cObj != NULL ) {
            id = objectRecordToID( cObj );
            }
        
        char *idString = autoSprintf( "%d", id );
        
        clothingListBuffer.appendElementString( idString );
        delete [] idString;
        
        if( cObj != NULL && cObj->numSlots > 0 ) {
            
            for( int cc=0; cc<inPlayer->clothingContained[c].size(); cc++ ) {
                char *contString = 
                    autoSprintf( 
                        ",%d", 
                        inPlayer->clothingContained[c].getElementDirect( cc ) );
                clothingListBuffer.appendElementString( contString );
                delete [] contString;
                }
            }

        if( c < NUM_CLOTHING_PIECES - 1 ) {
            clothingListBuffer.push_back( ';' );
            }
        }
    
    char *clothingList = clothingListBuffer.getElementString();


    const char *deathReason = "";
    
    if( inDelete && inPlayer->deathReason != NULL ) {
        deathReason = inPlayer->deathReason;
        }
    

    char *updateLine = autoSprintf( 
        "%d %d %d %d %d %d %s %d %d %d %d "
        "%.2f %s %.2f %.2f %.2f %s %d %d %d%s\n",
        inPlayer->id,
        inPlayer->displayID,
        inPlayer->facingOverride,
        inPlayer->actionAttempt,
        inPlayer->actionTarget.x,
        inPlayer->actionTarget.y,
        holdingString,
        inPlayer->heldOriginValid,
        inPlayer->heldOriginX,
        inPlayer->heldOriginY,
        inPlayer->heldTransitionSourceID,
        inPlayer->heat,
        posString,
        computeAge( inPlayer ),
        1.0 / getAgeRate(),
        computeMoveSpeed( inPlayer ),
        clothingList,
        inPlayer->justAte,
        inPlayer->justAteID,
        inPlayer->responsiblePlayerID,
        deathReason );
    
    inPlayer->justAte = false;
    inPlayer->justAteID = 0;
    
    // held origin only valid once
    inPlayer->heldOriginValid = 0;
    
    inPlayer->facingOverride = 0;
    inPlayer->actionAttempt = 0;

    delete [] holdingString;
    delete [] posString;
    delete [] clothingList;
    
    return updateLine;
    }



static LiveObject *getHitPlayer( int inX, int inY,
                                 char inCountMidPath = false,
                                 int inMaxAge = -1,
                                 int inMinAge = -1,
                                 int *outHitIndex = NULL ) {
    GridPos targetPos = { inX, inY };

    int numLive = players.size();
                                    
    LiveObject *hitPlayer = NULL;
                                    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *otherPlayer = 
            players.getElement( j );
        
        if( otherPlayer->error ) {
            continue;
            }

        if( inMaxAge != -1 &&
            computeAge( otherPlayer ) > inMaxAge ) {
            continue;
            }

        if( inMinAge != -1 &&
            computeAge( otherPlayer ) < inMinAge ) {
            continue;
            }

        if( otherPlayer->xd == 
            otherPlayer->xs &&
            otherPlayer->yd ==
            otherPlayer->ys ) {
            // other player standing still
                                            
            if( otherPlayer->xd ==
                inX &&
                otherPlayer->yd ==
                inY ) {
                                                
                // hit
                hitPlayer = otherPlayer;
                if( outHitIndex != NULL ) {
                    *outHitIndex = j;
                    }
                break;
                }
            }
        else {
            // other player moving
                
            GridPos cPos = 
                computePartialMoveSpot( 
                    otherPlayer );
                                        
            if( equal( cPos, targetPos ) ) {
                // hit
                hitPlayer = otherPlayer;
                if( outHitIndex != NULL ) {
                    *outHitIndex = j;
                    }
                break;
                }
            else if( inCountMidPath ) {
                
                int c = computePartialMovePathStep( otherPlayer );

                // consider path step before and after current location
                for( int i=-1; i<=1; i++ ) {
                    int testC = c + i;
                    
                    if( testC >= 0 && testC < otherPlayer->pathLength ) {
                        cPos = otherPlayer->pathToDest[testC];
                 
                        if( equal( cPos, targetPos ) ) {
                            // hit
                            hitPlayer = otherPlayer;
                            if( outHitIndex != NULL ) {
                                *outHitIndex = j;
                                }
                            break;
                            }
                        }
                    }
                if( hitPlayer != NULL ) {
                    break;
                    }
                }
            }
        }

    return hitPlayer;
    }

    


        


void processLoggedInPlayer( Socket *inSock,
                            SimpleVector<char> *inSockBuffer,
                            char *inEmail ) {
    
    // reload these settings every time someone new connects
    // thus, they can be changed without restarting the server
    minFoodDecrementSeconds = 
        SettingsManager::getFloatSetting( "minFoodDecrementSeconds", 5.0f );
    
    maxFoodDecrementSeconds = 
        SettingsManager::getFloatSetting( "maxFoodDecrementSeconds", 20 );

    babyBirthFoodDecrement = 
        SettingsManager::getIntSetting( "babyBirthFoodDecrement", 10 );


    eatBonus = 
        SettingsManager::getIntSetting( "eatBonus", 0 );



    numConnections ++;
                
    LiveObject newObject;

    newObject.email = inEmail;
    
    newObject.id = nextID;
    nextID++;

    SettingsManager::setSetting( "nextPlayerID",
                                 (int)nextID );


    newObject.responsiblePlayerID = -1;
    
    newObject.displayID = getRandomPersonObject();
    
    newObject.isEve = false;


    newObject.trueStartTimeSeconds = Time::getCurrentTime();
    newObject.lifeStartTimeSeconds = newObject.trueStartTimeSeconds;
                            

    newObject.lastSayTimeSeconds = Time::getCurrentTime();
    

    newObject.heldByOther = false;
                            
    int numOfAge = 0;
                            
    int numPlayers = players.size();
                            
    SimpleVector<LiveObject*> parentChoices;
    

    // lower the bad mother limit in low-population situations
    // so that babies aren't stuck with the same low-skill mother over and
    // over
    int badMotherLimit = 2 + numPlayers / 3;

    if( badMotherLimit > 10 ) {
        badMotherLimit = 10;
        }
    
    // with new birth cooldowns, we don't need bad mother limit anymore
    // try making it a non-factor
    badMotherLimit = 9999999;
    
    

    for( int i=0; i<numPlayers; i++ ) {
        LiveObject *player = players.getElement( i );
        
        if( player->error ) {
            continue;
            }

        if( isFertileAge( player ) ) {
            numOfAge ++;
            
            // make sure this woman isn't on cooldown
            // and that she's not a bad mother
            char canHaveBaby = true;

            
            if( Time::timeSec() < player->birthCoolDown ) {    
                canHaveBaby = false;
                }

            int numPastBabies = player->babyIDs->size();
            
            if( canHaveBaby && numPastBabies >= badMotherLimit ) {
                int numDead = 0;
                
                for( int b=0; b < numPastBabies; b++ ) {
                    
                    int bID = 
                        player->babyIDs->getElementDirect( b );

                    char bAlive = false;
                    
                    for( int j=0; j<numPlayers; j++ ) {
                        LiveObject *otherObj = players.getElement( j );
                    
                        if( otherObj->error ) {
                            continue;
                            }

                        int id = otherObj->id;
                    
                        if( id == bID ) {
                            bAlive = true;
                            break;
                            }
                        }
                    if( ! bAlive ) {
                        numDead ++;
                        }
                    }
                
                if( numDead >= badMotherLimit ) {
                    // this is a bad mother who lets all babies die
                    // don't give them more babies
                    canHaveBaby = false;
                    }
                }
            
            if( canHaveBaby ) {
                parentChoices.push_back( player );
                }
            }
        }

    newObject.parentChainLength = 1;

    if( parentChoices.size() == 0 || numOfAge == 0 ) {
        // new Eve
        // she starts almost full grown

        newObject.isEve = true;
        
        newObject.lifeStartTimeSeconds -= 14 * ( 1.0 / getAgeRate() );

        
        int femaleID = getRandomFemalePersonObject();
        
        if( femaleID != -1 ) {
            newObject.displayID = femaleID;
            }
        }
    

    if( numOfAge == 0 ) {
        // all existing babies are good spawn spot for Eve
                    
        for( int i=0; i<numPlayers; i++ ) {
            LiveObject *player = players.getElement( i );
            
            if( player->error ) {
                continue;
                }

            if( computeAge( player ) < babyAge ) {
                parentChoices.push_back( player );
                }
            }
        }
    else {
        // testing
        //newObject.lifeStartTimeSeconds -= 14 * ( 1.0 / getAgeRate() );
        }
    
                
    // else player starts as newborn
                

    // start full up to capacity with food
    newObject.foodStore = computeFoodCapacity( &newObject );

    if( ! newObject.isEve ) {
        // babies start out almost starving
        newObject.foodStore = 2;
        }

    newObject.heat = 0.5;

    newObject.foodDecrementETASeconds =
        Time::getCurrentTime() + 
        computeFoodDecrementTimeSeconds( &newObject );
                
    newObject.foodUpdate = true;
    newObject.lastAteID = 0;
    newObject.lastAteFillMax = 0;
    newObject.justAte = false;
    newObject.justAteID = 0;
    
    newObject.clothing = getEmptyClothingSet();

    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        newObject.clothingEtaDecay[c] = 0;
        }
    
    newObject.xs = 0;
    newObject.ys = 0;
    newObject.xd = 0;
    newObject.yd = 0;
    
    newObject.lastRegionLookTime = 0;
    
    
    LiveObject *parent = NULL;
                
    if( parentChoices.size() > 0 ) {

        if( newObject.isEve ) {
            // spawned next to random existing player
            int parentIndex = 
                randSource.getRandomBoundedInt( 0,
                                                parentChoices.size() - 1 );
            
            parent = parentChoices.getElementDirect( parentIndex );
            }
        else {
            // baby
            
            // pick random mother from a weighted distribution based on 
            // each mother's temperature
            
            
            // 0.5 temp is worth .5 weight
            // 1.0 temp and 0 are worth 0 weight
            
            double totalTemp = 0;
            
            for( int i=0; i<parentChoices.size(); i++ ) {
                LiveObject *p = parentChoices.getElementDirect( i );

                totalTemp += 0.5 - abs( p->heat - 0.5 );
                }

            double choice = 
                randSource.getRandomBoundedDouble( 0, totalTemp );
            
            
            totalTemp = 0;
            
            for( int i=0; i<parentChoices.size(); i++ ) {
                LiveObject *p = parentChoices.getElementDirect( i );

                totalTemp += 0.5 - abs( p->heat - 0.5 );
                
                if( totalTemp >= choice ) {
                    parent = p;
                    break;
                    }                
                }
            }
        

        
        if( ! newObject.isEve ) {
            // mother giving birth to baby
            // take a ton out of her food store

            int min = 4;
            if( parent->foodStore < min ) {
                min = parent->foodStore;
                }
            parent->foodStore -= babyBirthFoodDecrement;
            if( parent->foodStore < min ) {
                parent->foodStore = min;
                }

            parent->foodDecrementETASeconds +=
                computeFoodDecrementTimeSeconds( parent );
            
            parent->foodUpdate = true;
            

            // only set race if the spawn-near player is our mother
            // otherwise, we are a new Eve spawning next to a baby
            
            timeSec_t curTime = Time::timeSec();
            
            parent->babyBirthTimes->push_back( curTime );
            parent->babyIDs->push_back( newObject.id );
            
            // set cool-down time before this worman can have another baby
            parent->birthCoolDown = pickBirthCooldownSeconds() + curTime;

            ObjectRecord *parentObject = getObject( parent->displayID );

            // pick race of child
            int numRaces;
            int *races = getRaces( &numRaces );
        
            int parentRaceIndex = -1;
            
            for( int i=0; i<numRaces; i++ ) {
                if( parentObject->race == races[i] ) {
                    parentRaceIndex = i;
                    break;
                    }
                }
            

            if( parentRaceIndex != -1 ) {
                
                int childRace = parentObject->race;
                
                if( randSource.getRandomDouble() > childSameRaceLikelihood ) {
                    // different race than parent
                    
                    int offset = 1;
                    
                    if( randSource.getRandomBoolean() ) {
                        offset = -1;
                        }
                    int childRaceIndex = parentRaceIndex + offset;
                    
                    // don't wrap around
                    // but push in other direction instead
                    if( childRaceIndex >= numRaces ) {
                        childRaceIndex = numRaces - 2;
                        }
                    if( childRaceIndex < 0 ) {
                        childRaceIndex = 1;
                        }
                    
                    // stay in bounds
                    if( childRaceIndex >= numRaces ) {
                        childRaceIndex = numRaces - 1;
                        }
                    

                    childRace = races[ childRaceIndex ];
                    }
                
                if( childRace == parentObject->race ) {
                    newObject.displayID = getRandomFamilyMember( 
                        parentObject->race, parent->displayID, familySpan );
                    }
                else {
                    newObject.displayID = 
                        getRandomPersonObjectOfRace( childRace );
                    }
            
                }
        
            delete [] races;
            }
        
        if( parent->xs == parent->xd && 
            parent->ys == parent->yd ) {
                        
            // stationary parent
            newObject.xs = parent->xs;
            newObject.ys = parent->ys;
                        
            newObject.xd = parent->xs;
            newObject.yd = parent->ys;
            }
        else {
            // find where parent is along path
            GridPos cPos = computePartialMoveSpot( parent );
                        
            newObject.xs = cPos.x;
            newObject.ys = cPos.y;
                        
            newObject.xd = cPos.x;
            newObject.yd = cPos.y;
            }
        }                    
    else {
        // else starts at civ outskirts (lone Eve)
        int startX, startY;
        getEvePosition( newObject.email, &startX, &startY );

        if( SettingsManager::getIntSetting( "forceEveLocation", 0 ) ) {

            startX = 
                SettingsManager::getIntSetting( "forceEveLocationX", 0 );
            startY = 
                SettingsManager::getIntSetting( "forceEveLocationY", 0 );
            }
        
        
        newObject.xs = startX;
        newObject.ys = startY;
        
        newObject.xd = startX;
        newObject.yd = startY;    
        }
    

    int forceID = SettingsManager::getIntSetting( "forceEveObject", 0 );
    
    if( forceID > 0 ) {
        newObject.displayID = forceID;
        }
    
    
    float forceAge = SettingsManager::getFloatSetting( "forceEveAge", 0.0 );
    
    if( forceAge > 0 ) {
        newObject.lifeStartTimeSeconds = 
            Time::getCurrentTime() - forceAge * ( 1.0 / getAgeRate() );
        }
    

    newObject.holdingID = 0;


    if( areTriggersEnabled() ) {
        int id = getTriggerPlayerDisplayID( inEmail );
        
        if( id != -1 ) {
            newObject.displayID = id;
            
            newObject.lifeStartTimeSeconds = 
                Time::getCurrentTime() - 
                getTriggerPlayerAge( inEmail ) * ( 1.0 / getAgeRate() );
        
            GridPos pos = getTriggerPlayerPos( inEmail );
            
            newObject.xd = pos.x;
            newObject.yd = pos.y;
            newObject.xs = pos.x;
            newObject.ys = pos.y;
            newObject.xd = pos.x;
            
            newObject.holdingID = getTriggerPlayerHolding( inEmail );
            newObject.clothing = getTriggerPlayerClothing( inEmail );
            }
        }
    
    
    newObject.lineage = new SimpleVector<int>();
    
    newObject.name = NULL;
    
    newObject.pathLength = 0;
    newObject.pathToDest = NULL;
    newObject.pathTruncated = 0;
    newObject.firstMapSent = false;
    newObject.lastSentMapX = 0;
    newObject.lastSentMapY = 0;
    newObject.moveStartTime = Time::getCurrentTime();
    newObject.moveTotalSeconds = 0;
    newObject.facingOverride = 0;
    newObject.actionAttempt = 0;
    newObject.actionTarget.x = 0;
    newObject.actionTarget.y = 0;
    newObject.holdingEtaDecay = 0;
    newObject.heldOriginValid = 0;
    newObject.heldOriginX = 0;
    newObject.heldOriginY = 0;
    newObject.heldTransitionSourceID = -1;
    newObject.numContained = 0;
    newObject.containedIDs = NULL;
    newObject.containedEtaDecays = NULL;
    newObject.subContainedIDs = NULL;
    newObject.subContainedEtaDecays = NULL;
    newObject.embeddedWeaponID = 0;
    newObject.embeddedWeaponEtaDecay = 0;
    newObject.murderSourceID = 0;
    newObject.holdingWound = false;
    
    newObject.sock = inSock;
    newObject.sockBuffer = inSockBuffer;
    newObject.isNew = true;
    newObject.firstMessageSent = false;
    
    newObject.dying = false;
    newObject.dyingETA = 0;
    
    newObject.error = false;
    newObject.errorCauseString = "";
    
    newObject.customGraveID = -1;
    newObject.deathReason = NULL;
    
    newObject.deleteSent = false;
    newObject.deathLogged = false;
    newObject.newMove = false;
    
    newObject.posForced = false;

    newObject.needsUpdate = false;
    newObject.updateSent = false;
    
    newObject.babyBirthTimes = new SimpleVector<timeSec_t>();
    newObject.babyIDs = new SimpleVector<int>();
    
    newObject.birthCoolDown = 0;
                
                
                
    for( int i=0; i<HEAT_MAP_D * HEAT_MAP_D; i++ ) {
        newObject.heatMap[i] = 0;
        }

    
    newObject.parentID = -1;
    char *parentEmail = NULL;

    if( parent != NULL && isFertileAge( parent ) ) {
        // do not log babies that new Eve spawns next to as parents
        newObject.parentID = parent->id;
        parentEmail = parent->email;

        newObject.parentChainLength = parent->parentChainLength + 1;

        // mother
        newObject.lineage->push_back( newObject.parentID );
        
        for( int i=0; 
             i < parent->lineage->size() && 
                 i < maxLineageTracked - 1;
             i++ ) {
            
            newObject.lineage->push_back( 
                parent->lineage->getElementDirect( i ) );
            }
        }
    
    // parent pointer possibly no longer valid after push_back, which
    // can resize the vector
    parent = NULL;
    players.push_back( newObject );            


    logBirth( newObject.id,
              newObject.email,
              newObject.parentID,
              parentEmail,
              ! getFemale( &newObject ),
              newObject.xd,
              newObject.yd,
              players.size(),
              newObject.parentChainLength );
    
    AppLog::infoF( "New player %s connected as player %d",
                   newObject.email, newObject.id );
    }




// doesn't check whether dest itself is blocked
static char directLineBlocked( GridPos inSource, GridPos inDest ) {
    // line algorithm from here
    // https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
    
    double deltaX = inDest.x - inSource.x;
    
    double deltaY = inDest.y - inSource.y;
    

    int xStep = 1;
    if( deltaX < 0 ) {
        xStep = -1;
        }
    
    int yStep = 1;
    if( deltaY < 0 ) {
        yStep = -1;
        }
    

    if( deltaX == 0 ) {
        // vertical line
        
        // just walk through y
        for( int y=inSource.y; y != inDest.y; y += yStep ) {
            if( isMapSpotBlocking( inSource.x, y ) ) {
                return true;
                }
            }
        }
    else {
        double deltaErr = fabs( deltaY / (double)deltaX );
        
        double error = deltaErr - 0.5;
        
        int y = inSource.y;
        for( int x=inSource.x; x != inDest.x; x += xStep ) {
            if( isMapSpotBlocking( x, y ) ) {
                return true;
                }
            error += deltaErr;
            
            if( error >= 0.5 ) {
                y += yStep;
                error -= 1.0;
                }
            }
        }

    return false;
    }



char removeFromContainerToHold( LiveObject *inPlayer, 
                                int inContX, int inContY,
                                int inSlotNumber );



// swap indicates that we want to put the held item at the bottom
// of the container and take the top one
// returns true if added
static char addHeldToContainer( LiveObject *inPlayer,
                                int inTargetID,
                                int inContX, int inContY,
                                char inSwap = false ) {
    
    int target = inTargetID;
        
    int targetSlots = 
        getNumContainerSlots( target );
                                        
    ObjectRecord *targetObj =
        getObject( target );
    
    if( isGrave( target ) ) {
        return false;
        }
    

    int slotSize =
        targetObj->slotSize;
    
    int containSize =
        getObject( 
            inPlayer->holdingID )->
        containSize;

    int numIn = 
        getNumContained( inContX, inContY );
    
    if( numIn < targetSlots &&
        isContainable( 
            inPlayer->holdingID ) &&
        containSize <= slotSize ) {
        
        // add to container
        
        setResponsiblePlayer( 
            inPlayer->id );
        
        int idToAdd = inPlayer->holdingID;


        float stretch = getObject( idToAdd )->slotTimeStretch;
                    
                    

        if( inPlayer->numContained > 0 ) {
            // negative to indicate sub-container
            idToAdd *= -1;
            }

        

        addContained( 
            inContX, inContY,
            idToAdd,
            inPlayer->holdingEtaDecay );

        if( inPlayer->numContained > 0 ) {
            timeSec_t curTime = Time::timeSec();
            
            for( int c=0; c<inPlayer->numContained; c++ ) {
                
                // undo decay stretch before adding
                // (stretch applied by adding)
                if( stretch != 1.0 &&
                    inPlayer->containedEtaDecays[c] != 0 ) {
                
                    timeSec_t offset = 
                        inPlayer->containedEtaDecays[c] - curTime;
                    
                    offset = offset * stretch;
                    
                    inPlayer->containedEtaDecays[c] = curTime + offset;
                    }


                addContained( inContX, inContY, inPlayer->containedIDs[c],
                              inPlayer->containedEtaDecays[c],
                              numIn + 1 );
                }
            
            clearPlayerHeldContained( inPlayer );
            }
        

        
        setResponsiblePlayer( -1 );
        
        inPlayer->holdingID = 0;
        inPlayer->holdingEtaDecay = 0;
        inPlayer->heldOriginValid = 0;
        inPlayer->heldOriginX = 0;
        inPlayer->heldOriginY = 0;
        inPlayer->heldTransitionSourceID = -1;

        int numInNow = getNumContained( inContX, inContY );
        
        if( inSwap &&  numInNow > 1 ) {
            // take what's on bottom of container, but only if it's different
            // from what's in our hand

            // if we find a same object on bottom, keep going up until
            // we find a non-same one to swap
            for( int botInd = 0; botInd < numInNow - 1; botInd ++ ) {
                
                char same = false;

                int bottomItem = 
                    getContained( inContX, inContY, botInd, 0 );
            
                if( bottomItem == idToAdd ) {
                    if( bottomItem > 0 ) {
                        // not sub conts
                        same = true;
                        }
                    else {
                        // they must contain same stuff to be same
                        int bottomNum = getNumContained( inContX, inContY,
                                                         botInd + 1 );
                        int topNum =  getNumContained( inContX, inContY,
                                                       numInNow );
                    
                        if( bottomNum != topNum ) {
                            same = true;
                            }
                        else {
                            for( int b=0; b<bottomNum; b++ ) {
                                int subB = getContained( inContX, inContY,
                                                         b, botInd );
                                int subT = getContained( inContX, inContY,
                                                         b, numInNow );
                                
                                if( subB != subT ) {
                                    same = false;
                                    break;
                                    }
                                }
                            same = true;
                            }
                        }
                    }
            

                if( ! same ) {
                    // found one to swap
                    removeFromContainerToHold( inPlayer, inContX, inContY, 
                                               botInd );
                    break;
                    }
                }
            // if we didn't remove one, it means whole container is full
            // of identical items.
            // the swap action doesn't work, so we just let it
            // behave like an add action instead.
            }

        return true;
        }

    return false;
    }



// returns true if succeeded
char removeFromContainerToHold( LiveObject *inPlayer, 
                                int inContX, int inContY,
                                int inSlotNumber ) {
    inPlayer->heldOriginValid = 0;
    inPlayer->heldOriginX = 0;
    inPlayer->heldOriginY = 0;                        
    inPlayer->heldTransitionSourceID = -1;
    

    if( isGridAdjacent( inContX, inContY,
                        inPlayer->xd, 
                        inPlayer->yd ) 
        ||
        ( inContX == inPlayer->xd &&
          inContY == inPlayer->yd ) ) {
                            
        inPlayer->actionAttempt = 1;
        inPlayer->actionTarget.x = inContX;
        inPlayer->actionTarget.y = inContY;
                            
        if( inContX > inPlayer->xd ) {
            inPlayer->facingOverride = 1;
            }
        else if( inContX < inPlayer->xd ) {
            inPlayer->facingOverride = -1;
            }

        // can only use on targets next to us for now,
        // no diags
                            
        int target = getMapObject( inContX, inContY );
                            
        if( target != 0 ) {
                            
            int numIn = 
                getNumContained( inContX, inContY );
                                
            int toRemoveID = 0;
                                
            if( numIn > 0 ) {
                toRemoveID = getContained( inContX, inContY, inSlotNumber );
                }
            
            char subContain = false;
            
            if( toRemoveID < 0 ) {
                toRemoveID *= -1;
                subContain = true;
                }

            if( inPlayer->holdingID == 0 && 
                numIn > 0 &&
                // old enough to handle it
                getObject( toRemoveID )->minPickupAge <= 
                computeAge( inPlayer ) ) {
                // get from container


                if( subContain ) {
                    int subSlotNumber = inSlotNumber;
                    
                    if( subSlotNumber == -1 ) {
                        subSlotNumber = numIn - 1;
                        }

                    inPlayer->containedIDs =
                        getContained( inContX, inContY, 
                                      &( inPlayer->numContained ), 
                                      subSlotNumber + 1 );
                    inPlayer->containedEtaDecays =
                        getContainedEtaDecay( inContX, inContY, 
                                              &( inPlayer->numContained ), 
                                              subSlotNumber + 1 );

                    // these will be cleared when removeContained is called
                    // for this slot below, so just get them now without clearing

                    // empty vectors... there are no sub-sub containers
                    inPlayer->subContainedIDs = 
                        new SimpleVector<int>[ inPlayer->numContained ];
                    inPlayer->subContainedEtaDecays = 
                        new SimpleVector<timeSec_t>[ inPlayer->numContained ];
                
                    }
                
                
                setResponsiblePlayer( - inPlayer->id );
                
                inPlayer->holdingID =
                    removeContained( 
                        inContX, inContY, inSlotNumber,
                        &( inPlayer->holdingEtaDecay ) );
                        
                setResponsiblePlayer( -1 );

                if( inPlayer->holdingID < 0 ) {
                    // sub-contained
                    
                    inPlayer->holdingID *= -1;    
                    }
                
                // contained objects aren't animating
                // in a way that needs to be smooth
                // transitioned on client
                inPlayer->heldOriginValid = 0;
                inPlayer->heldOriginX = 0;
                inPlayer->heldOriginY = 0;

                return true;
                }
            }
        }        
    
    return false;
    }



static char addHeldToClothingContainer( LiveObject *inPlayer, 
                                        int inC ) {    
    // drop into own clothing
    ObjectRecord *cObj = 
        clothingByIndex( 
            inPlayer->clothing,
            inC );
                                    
    if( cObj != NULL &&
        isContainable( 
            inPlayer->holdingID ) ) {
                                        
        int oldNum =
            inPlayer->
            clothingContained[inC].size();
                                        
        int slotSize =
            cObj->slotSize;
                                        
        int containSize =
            getObject( inPlayer->holdingID )->
            containSize;
    
        if( oldNum < cObj->numSlots &&
            containSize <= slotSize ) {
            // room
            inPlayer->clothingContained[inC].
                push_back( 
                    inPlayer->holdingID );

            if( inPlayer->
                holdingEtaDecay != 0 ) {
                                                
                timeSec_t curTime = 
                    Time::timeSec();
                                            
                timeSec_t offset = 
                    inPlayer->
                    holdingEtaDecay - 
                    curTime;
                                            
                offset = 
                    offset / 
                    cObj->
                    slotTimeStretch;
                                                
                inPlayer->holdingEtaDecay =
                    curTime + offset;
                }
                                            
            inPlayer->
                clothingContainedEtaDecays[inC].
                push_back( inPlayer->
                           holdingEtaDecay );
                                        
            inPlayer->holdingID = 0;
            inPlayer->holdingEtaDecay = 0;
            inPlayer->heldOriginValid = 0;
            inPlayer->heldOriginX = 0;
            inPlayer->heldOriginY = 0;
            inPlayer->heldTransitionSourceID =
                -1;

            return true;
            }
        }

    return false;
    }



// change held as the result of a transition
static void handleHoldingChange( LiveObject *inPlayer, int inNewHeldID ) {
    
    LiveObject *nextPlayer = inPlayer;

    int oldHolding = nextPlayer->holdingID;
    
    int oldContained = 
        nextPlayer->numContained;

    nextPlayer->holdingID = inNewHeldID;
    
    if( oldHolding != nextPlayer->holdingID ) {
        setFreshEtaDecayForHeld( nextPlayer );
        }
    
    nextPlayer->heldOriginValid = 0;
    nextPlayer->heldOriginX = 0;
    nextPlayer->heldOriginY = 0;
    
    // can newly changed container hold
    // less than what it could contain
    // before?
    
    int newHeldSlots = getNumContainerSlots( 
        nextPlayer->holdingID );
    
    if( newHeldSlots < oldContained ) {
        // truncate
        nextPlayer->numContained
            = newHeldSlots;
        }
    
    if( newHeldSlots > 0 && 
        oldHolding != 0 ) {
                                        
        restretchDecays( 
            newHeldSlots,
            nextPlayer->containedEtaDecays,
            oldHolding,
            nextPlayer->holdingID );
        }
                                    
    }



static unsigned char *makeCompressedMessage( char *inMessage, int inLength,
                                             int *outLength ) {
    
    int compressedSize;
    unsigned char *compressedData =
        zipCompress( (unsigned char*)inMessage, inLength, &compressedSize );



    char *header = autoSprintf( "CM\n%d %d\n#", 
                                inLength,
                                compressedSize );
    int headerLength = strlen( header );
    int fullLength = headerLength + compressedSize;
    
    unsigned char *fullMessage = new unsigned char[ fullLength ];
    
    memcpy( fullMessage, (unsigned char*)header, headerLength );
    
    memcpy( &( fullMessage[ headerLength ] ), compressedData, compressedSize );

    delete [] compressedData;
    
    *outLength = fullLength;
    
    delete [] header;
    
    return fullMessage;
    }



static int maxUncompressedSize = 256;


static void sendMessageToPlayer( LiveObject *inPlayer, 
                                 char *inMessage, int inLength ) {
    
    unsigned char *message = (unsigned char*)inMessage;
    int len = inLength;
    
    char deleteMessage = false;

    if( inLength > maxUncompressedSize ) {
        message = makeCompressedMessage( inMessage, inLength, &len );
        deleteMessage = true;
        }

    int numSent = 
        inPlayer->sock->send( message, 
                              len, 
                              false, false );
        
    if( numSent != len ) {
        setDeathReason( inPlayer, "disconnected" );
        
        inPlayer->error = true;
        inPlayer->errorCauseString = "Socket write failed";
        }

    if( deleteMessage ) {
        delete [] message;
        }
    }
    


void readNameGivingPhrases( const char *inSettingsName, 
                            SimpleVector<char*> *inList ) {
    char *cont = SettingsManager::getSettingContents( inSettingsName, "" );
    
    if( strcmp( cont, "" ) == 0 ) {
        delete [] cont;
        return;    
        }
    
    int numParts;
    char **parts = split( cont, "\n", &numParts );
    delete [] cont;
    
    for( int i=0; i<numParts; i++ ) {
        if( strcmp( parts[i], "" ) != 0 ) {
            inList->push_back( stringToUpperCase( parts[i] ) );
            }
        delete [] parts[i];
        }
    delete [] parts;
    }



// returns pointer to name in string
char *isNamingSay( char *inSaidString, SimpleVector<char*> *inPhraseList ) {
    for( int i=0; i<inPhraseList->size(); i++ ) {
        char *testString = inPhraseList->getElementDirect( i );
        
        if( strstr( inSaidString, testString ) == inSaidString ) {
            // hit
            int phraseLen = strlen( testString );
            // skip spaces after
            while( inSaidString[ phraseLen ] == ' ' ) {
                phraseLen++;
                }
            return &( inSaidString[ phraseLen ] );
            }
        }
    return NULL;
    }



char *isBabyNamingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &nameGivingPhrases );
    }

char *isFamilyNamingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &familyNameGivingPhrases );
    }




int readIntFromFile( const char *inFileName, int inDefaultValue ) {
    FILE *f = fopen( inFileName, "r" );
    
    if( f == NULL ) {
        return inDefaultValue;
        }
    
    int val = inDefaultValue;
    
    fscanf( f, "%d", &val );

    fclose( f );

    return val;
    }



void apocalypseStep() {
    
    double curTime = Time::getCurrentTime();

    if( !apocalypseTriggered ) {
        
        if( apocalypseRequest == NULL &&
            curTime - lastRemoteApocalypseCheckTime > 
            remoteApocalypseCheckInterval ) {
            printf( "Checking for remote apocalypse\n" );
            
            lastRemoteApocalypseCheckTime = curTime;
            
            char *url = autoSprintf( "%s?action=check_apocalypse", 
                                     reflectorURL );
        
            apocalypseRequest =
                new WebRequest( "GET", url, NULL );
            
            delete [] url;
            }
        else if( apocalypseRequest != NULL ) {
            int result = apocalypseRequest->step();

            if( result == -1 ) {
                AppLog::info( 
                    "Apocalypse check:  Request to reflector failed." );
                }
            else if( result == 1 ) {
                // done, have result

                char *webResult = 
                    apocalypseRequest->getResult();
                
                if( strstr( webResult, "OK" ) == NULL ) {
                    AppLog::infoF( 
                        "Apocalypse check:  Bad response from reflector:  %s.",
                        webResult );
                    }
                else {
                    int newApocalypseNumber = lastApocalypseNumber;
                    
                    sscanf( webResult, "%d\n", &newApocalypseNumber );
                
                    if( newApocalypseNumber > lastApocalypseNumber ) {
                        lastApocalypseNumber = newApocalypseNumber;
                        apocalypseTriggered = true;
                        apocalypseRemote = true;
                        AppLog::infoF( 
                            "Apocalypse check:  New remote apocalypse:  %d.",
                            lastApocalypseNumber );
                        SettingsManager::setSetting( "lastApocalypseNumber",
                                                     lastApocalypseNumber );
                        }
                    }
                    
                delete [] webResult;
                }
            
            if( result != 0 ) {
                delete apocalypseRequest;
                apocalypseRequest = NULL;
                }
            }
        }
        


    if( apocalypseTriggered ) {

        if( !apocalypseStarted ) {
            apocalypsePossible = 
                SettingsManager::getIntSetting( "apocalypsePossible", 0 );

            if( !apocalypsePossible ) {
                // settings change since we last looked at it
                apocalypseTriggered = false;
                return;
                }

            if( !apocalypseRemote && 
                apocalypseRequest == NULL && reflectorURL != NULL ) {
                
                
                char *reflectorSharedSecret = 
                    SettingsManager::
                    getStringSetting( "reflectorSharedSecret" );
                
                if( reflectorSharedSecret != NULL ) {
                    lastApocalypseNumber++;

                    AppLog::infoF( 
                        "Apocalypse trigger:  New local apocalypse:  %d.",
                        lastApocalypseNumber );

                    SettingsManager::setSetting( "lastApocalypseNumber",
                                                 lastApocalypseNumber );

                    int closestPlayerIndex = -1;
                    double closestDist = 999999999;
                    
                    for( int i=0; i<players.size(); i++ ) {
                        LiveObject *nextPlayer = players.getElement( i );
                        if( !nextPlayer->error ) {
                            
                            double dist = 
                                abs( nextPlayer->xd - apocalypseLocation.x ) +
                                abs( nextPlayer->yd - apocalypseLocation.y );
                            if( dist < closestDist ) {
                                closestPlayerIndex = i;
                                closestDist = dist;
                                }
                            }
                        
                        }
                    char *name = NULL;
                    if( closestPlayerIndex != -1 ) {
                        name = 
                            players.getElement( closestPlayerIndex )->
                            name;
                        }
                    
                    if( name == NULL ) {
                        name = stringDuplicate( "UNKNOWN" );
                        }
                    
                    char *idString = autoSprintf( "%d", lastApocalypseNumber );
                    
                    char *hash = hmac_sha1( reflectorSharedSecret, idString );

                    delete [] idString;

                    char *url = autoSprintf( 
                        "%s?action=trigger_apocalypse"
                        "&id=%d&id_hash=%s&name=%s",
                        reflectorURL, lastApocalypseNumber, hash, name );

                    delete [] hash;
                    delete [] name;
                    
                    printf( "Starting new web request for %s\n", url );
                    
                    apocalypseRequest =
                        new WebRequest( "GET", url, NULL );
                                
                    delete [] url;
                    delete [] reflectorSharedSecret;
                    }
                }


            // send all players the AP message
            const char *message = "AP\n#";
            int messageLength = strlen( message );
            
            for( int i=0; i<players.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( i );
                if( !nextPlayer->error ) {
                    
                    int numSent = 
                        nextPlayer->sock->send( 
                            (unsigned char*)message, 
                            messageLength,
                            false, false );
                    
                    if( numSent != messageLength ) {
                        setDeathReason( nextPlayer, "disconnected" );
                        
                        nextPlayer->error = true;
                        nextPlayer->errorCauseString =
                            "Socket write failed";
                        }
                    }
                }
            
            apocalypseStartTime = Time::getCurrentTime();
            apocalypseStarted = true;
            }

        if( apocalypseRequest != NULL ) {
            
            int result = apocalypseRequest->step();
                

            if( result == -1 ) {
                AppLog::info( 
                    "Apocalypse trigger:  Request to reflector failed." );
                }
            else if( result == 1 ) {
                // done, have result

                char *webResult = 
                    apocalypseRequest->getResult();
                printf( "Apocalypse trigger:  "
                        "Got web result:  '%s'\n", webResult );
                
                if( strstr( webResult, "OK" ) == NULL ) {
                    AppLog::infoF( 
                        "Apocalypse trigger:  "
                        "Bad response from reflector:  %s.",
                        webResult );
                    }
                delete [] webResult;
                }
            
            if( result != 0 ) {
                delete apocalypseRequest;
                apocalypseRequest = NULL;
                }
            }

        if( apocalypseRequest == NULL &&
            Time::getCurrentTime() - apocalypseStartTime >= 7 ) {
            
            for( int i=0; i<players.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( i );
                
                if( ! nextPlayer->error ) {
                    nextPlayer->error = true;
                    setDeathReason( nextPlayer, "apocalypse" );
                    }
                }

            if( players.size() == 0 ) {
                // apocalypse over

                // clear map
                freeMap();
                
                wipeMapFiles();
                
                initMap();


                lastRemoteApocalypseCheckTime = curTime;
                apocalypseStarted = false;
                apocalypseTriggered = false;
                apocalypseRemote = false;
                }
            }
        }
    }




void monumentStep() {
    if( monumentCallPending ) {
        // send to all players
        char *message = autoSprintf( "MN\n%d %d %d\n#", 
                                     monumentCallX, monumentCallY,
                                     monumentCallID );
        int messageLength = strlen( message );
            
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            if( !nextPlayer->error ) {
                
                int numSent = 
                    nextPlayer->sock->send( 
                        (unsigned char*)message, 
                        messageLength,
                        false, false );
                    
                if( numSent != messageLength ) {
                    setDeathReason( nextPlayer, "disconnected" );
                    
                    nextPlayer->error = true;
                    nextPlayer->errorCauseString =
                        "Socket write failed";
                    }
                }
            }

        delete [] message;

        monumentCallPending = false;
        }
    }




int main() {

    nextID = 
        SettingsManager::getIntSetting( "nextPlayerID", 2 );


    // make backup and delete old backup every three days
    AppLog::setLog( new FileLog( "log.txt", 259200 ) );

    AppLog::setLoggingLevel( Log::DETAIL_LEVEL );
    AppLog::printAllMessages( true );

    printf( "\n" );
    AppLog::info( "Server starting up" );

    printf( "\n" );
    
    
    

    nextSequenceNumber = 
        SettingsManager::getIntSetting( "sequenceNumber", 1 );

    requireClientPassword =
        SettingsManager::getIntSetting( "requireClientPassword", 1 );
    
    requireTicketServerCheck =
        SettingsManager::getIntSetting( "requireTicketServerCheck", 1 );
    
    clientPassword = 
        SettingsManager::getStringSetting( "clientPassword" );


    int dataVer = readIntFromFile( "dataVersionNumber.txt", 1 );
    int codVer = readIntFromFile( "serverCodeVersionNumber.txt", 1 );
    
    versionNumber = dataVer;
    if( codVer > versionNumber ) {
        versionNumber = codVer;
        }
    
    printf( "\n" );
    AppLog::infoF( "Server using version number %d", versionNumber );

    printf( "\n" );
    



    minFoodDecrementSeconds = 
        SettingsManager::getFloatSetting( "minFoodDecrementSeconds", 5.0f );

    maxFoodDecrementSeconds = 
        SettingsManager::getFloatSetting( "maxFoodDecrementSeconds", 20 );

    babyBirthFoodDecrement = 
        SettingsManager::getIntSetting( "babyBirthFoodDecrement", 10 );


    eatBonus = 
        SettingsManager::getIntSetting( "eatBonus", 0 );
    

    if( clientPassword == NULL ) {
        requireClientPassword = 0;
        }


    ticketServerURL = 
        SettingsManager::getStringSetting( "ticketServerURL" );
    

    if( ticketServerURL == NULL ) {
        requireTicketServerCheck = 0;
        }

    
    reflectorURL = SettingsManager::getStringSetting( "reflectorURL" );

    apocalypsePossible = 
        SettingsManager::getIntSetting( "apocalypsePossible", 0 );

    lastApocalypseNumber = 
        SettingsManager::getIntSetting( "lastApocalypseNumber", 0 );


    childSameRaceLikelihood =
        (double)SettingsManager::getFloatSetting( "childSameRaceLikelihood",
                                                  0.90 );
    
    familySpan =
        SettingsManager::getIntSetting( "familySpan", 2 );
    
    
    readNameGivingPhrases( "babyNamingPhrases", &nameGivingPhrases );
    readNameGivingPhrases( "familyNamingPhrases", &familyNameGivingPhrases );
    
    eveName = 
        SettingsManager::getStringSetting( "eveName", "EVE" );


#ifdef WIN_32
    printf( "\n\nPress CTRL-C to shut down server gracefully\n\n" );

    SetConsoleCtrlHandler( ctrlHandler, TRUE );
#else
    printf( "\n\nPress CTRL-Z to shut down server gracefully\n\n" );

    signal( SIGTSTP, intHandler );
#endif

    initNames();

    initLifeLog();
    initBackup();
    
    initPlayerStats();


    char rebuilding;

    initAnimationBankStart( &rebuilding );
    while( initAnimationBankStep() < 1.0 );
    initAnimationBankFinish();


    initObjectBankStart( &rebuilding, true, true );
    while( initObjectBankStep() < 1.0 );
    initObjectBankFinish();

    
    initCategoryBankStart( &rebuilding );
    while( initCategoryBankStep() < 1.0 );
    initCategoryBankFinish();


    // auto-generate category-based transitions
    initTransBankStart( &rebuilding, true, true, true, true );
    while( initTransBankStep() < 1.0 );
    initTransBankFinish();
    

    // defaults to one hour
    int epochSeconds = 
        SettingsManager::getIntSetting( "epochSeconds", 3600 );
    
    setTransitionEpoch( epochSeconds );


    initFoodLog();
    initFailureLog();
    
    initTriggers();


    initMap();
    
    
    int port = 
        SettingsManager::getIntSetting( "port", 5077 );
    
    
    SocketPoll sockPoll;
    
    
    
    SocketServer server( port, 256 );
    
    sockPoll.addSocketServer( &server );
    
    AppLog::infoF( "Listening for connection on port %d", port );

    // if we received one the last time we looped, don't sleep when
    // polling for socket being ready, because there could be more data
    // waiting in the buffer for a given socket
    char someClientMessageReceived = false;
    

    while( !quit ) {
        
        int shutdownMode = SettingsManager::getIntSetting( "shutdownMode", 0 );
        
        
        apocalypseStep();
        monumentStep();
        
        checkBackup();

        stepFoodLog();
        stepFailureLog();
        
        stepPlayerStats();
        
        
        int numLive = players.size();
        
        double secPerYear = 1.0 / getAgeRate();
        

        // check for timeout for shortest player move or food decrement
        // so that we wake up from listening to socket to handle it
        double minMoveTime = 999999;
        
        double curTime = Time::getCurrentTime();

        for( int i=0; i<numLive; i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            
            // clear at the start of each step
            nextPlayer->responsiblePlayerID = -1;

            if( nextPlayer->error ) {
                continue;
                }

            if( nextPlayer->xd != nextPlayer->xs ||
                nextPlayer->yd != nextPlayer->ys ) {
                
                double moveTimeLeft =
                    nextPlayer->moveTotalSeconds -
                    ( curTime - nextPlayer->moveStartTime );
                
                if( moveTimeLeft < 0 ) {
                    moveTimeLeft = 0;
                    }
                
                if( moveTimeLeft < minMoveTime ) {
                    minMoveTime = moveTimeLeft;
                    }
                }
            
            // look at food decrement time too
                
            double timeLeft =
                nextPlayer->foodDecrementETASeconds - curTime;
                        
            if( timeLeft < 0 ) {
                timeLeft = 0;
                }
            if( timeLeft < minMoveTime ) {
                minMoveTime = timeLeft;
                }           

            // look at held decay too
            if( nextPlayer->holdingEtaDecay != 0 ) {
                
                timeLeft = nextPlayer->holdingEtaDecay - curTime;
                
                if( timeLeft < 0 ) {
                    timeLeft = 0;
                    }
                if( timeLeft < minMoveTime ) {
                    minMoveTime = timeLeft;
                    }
                }
            
            for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                if( nextPlayer->clothingEtaDecay[c] != 0 ) {
                    timeLeft = nextPlayer->clothingEtaDecay[c] - curTime;
                    
                    if( timeLeft < 0 ) {
                        timeLeft = 0;
                        }
                    if( timeLeft < minMoveTime ) {
                        minMoveTime = timeLeft;
                        }
                    }
                for( int cc=0; cc<nextPlayer->clothingContained[c].size();
                     cc++ ) {
                    timeSec_t decay =
                        nextPlayer->clothingContainedEtaDecays[c].
                        getElementDirect( cc );
                    
                    if( decay != 0 ) {
                        timeLeft = decay - curTime;
                        
                        if( timeLeft < 0 ) {
                            timeLeft = 0;
                            }
                        if( timeLeft < minMoveTime ) {
                            minMoveTime = timeLeft;
                            }
                        }
                    }
                }
            
            // look at old age death to
            double ageLeft = forceDeathAge - computeAge( nextPlayer );
            
            double ageSecondsLeft = ageLeft * secPerYear;
            
            if( ageSecondsLeft < minMoveTime ) {
                minMoveTime = ageSecondsLeft;

                if( minMoveTime < 0 ) {
                    minMoveTime = 0;
                    }
                }
            

            // as low as it can get, no need to check other players
            if( minMoveTime == 0 ) {
                break;
                }
            }
        
        
        SocketOrServer *readySock =  NULL;

        double pollTimeout = 2;
        
        if( minMoveTime < pollTimeout ) {
            // shorter timeout if we have to wake up for a move
            
            // HOWEVER, always keep max timout at 2 sec
            // so we always wake up periodically to catch quit signals, etc

            pollTimeout = minMoveTime;
            }
        
        if( pollTimeout > 0 ) {
            int shortestDecay = getNextDecayDelta();
            
            if( shortestDecay != -1 ) {
                
                if( shortestDecay < pollTimeout ) {
                    pollTimeout = shortestDecay;
                    }
                }
            }

        
        char anyTicketServerRequestsOut = false;

        for( int i=0; i<newConnections.size(); i++ ) {
            
            FreshConnection *nextConnection = newConnections.getElement( i );

            if( nextConnection->ticketServerRequest != NULL ) {
                anyTicketServerRequestsOut = true;
                break;
                }
            }
        
        if( anyTicketServerRequestsOut ) {
            // need to step outstanding ticket server web requests
            // sleep a tiny amount of time to avoid cpu spin
            pollTimeout = 0.01;
            }


        if( areTriggersEnabled() ) {
            // need to handle trigger timing
            pollTimeout = 0.01;
            }

        if( someClientMessageReceived ) {
            // don't wait at all
            // we need to check for next message right away
            pollTimeout = 0;
            }

        // we thus use zero CPU as long as no messages or new connections
        // come in, and only wake up when some timed action needs to be
        // handled
        
        readySock = sockPoll.wait( (int)( pollTimeout * 1000 ) );
        
        
        
        
        if( readySock != NULL && !readySock->isSocket ) {
            // server ready
            Socket *sock = server.acceptConnection( 0 );

            if( sock != NULL ) {
                AppLog::info( "Got connection" );                

                FreshConnection newConnection;
                
                newConnection.connectionStartTimeSeconds = 
                    Time::getCurrentTime();

                newConnection.email = NULL;

                newConnection.sock = sock;

                newConnection.sequenceNumber = nextSequenceNumber;
                
                nextSequenceNumber ++;
                
                SettingsManager::setSetting( "sequenceNumber",
                                             (int)nextSequenceNumber );
                
                char *message;
                
                int maxPlayers = 
                    SettingsManager::getIntSetting( "maxPlayers", 200 );
                
                int currentPlayers = players.size() + newConnections.size();
                    

                if( apocalypseTriggered || shutdownMode ) {
                        
                    AppLog::info( "We are in shutdown mode, "
                                  "deflecting new connection" );         
                    
                    AppLog::infoF( "%d player(s) still alive on server.",
                                   players.size() );

                    message = autoSprintf( "SHUTDOWN\n"
                                           "%d/%d\n"
                                           "#",
                                           currentPlayers, maxPlayers );
                    newConnection.shutdownMode = true;
                    }
                else if( currentPlayers >= maxPlayers ) {
                    AppLog::infoF( "%d of %d permitted players connected, "
                                   "deflecting new connection",
                                   currentPlayers, maxPlayers );
                    
                    message = autoSprintf( "SERVER_FULL\n"
                                           "%d/%d\n"
                                           "#",
                                           currentPlayers, maxPlayers );
                    
                    newConnection.shutdownMode = true;
                    }         
                else {
                    message = autoSprintf( "SN\n"
                                           "%d/%d\n"
                                           "%lu\n"
                                           "%lu\n#",
                                           currentPlayers, maxPlayers,
                                           newConnection.sequenceNumber,
                                           versionNumber );
                    newConnection.shutdownMode = false;
                    }


                // wait for email and hashes to come from client
                // (and maybe ticket server check isn't required by settings)
                newConnection.ticketServerRequest = NULL;
                newConnection.error = false;
                newConnection.errorCauseString = "";
                
                int messageLength = strlen( message );
                
                int numSent = 
                    sock->send( (unsigned char*)message, 
                                messageLength, 
                                false, false );
                    
                delete [] message;
                    

                if( numSent != messageLength ) {
                    // failed or blocked on our first send attempt

                    // reject it right away

                    delete sock;
                    sock = NULL;
                    }
                else {
                    // first message sent okay
                    newConnection.sockBuffer = new SimpleVector<char>();
                    

                    sockPoll.addSocket( sock );

                    newConnections.push_back( newConnection );
                    }

                AppLog::infoF( "Listening for another connection on port %d", 
                               port );
    
                }
            }
        

        stepTriggers();
        
        
        // listen for messages from new connections

        for( int i=0; i<newConnections.size(); i++ ) {
            
            FreshConnection *nextConnection = newConnections.getElement( i );
            

            if( nextConnection->ticketServerRequest != NULL ) {
                
                int result = nextConnection->ticketServerRequest->step();
                

                if( result == -1 ) {
                    AppLog::info( "Request to ticket server failed, "
                                  "client rejected." );
                    nextConnection->error = true;
                    nextConnection->errorCauseString =
                        "Ticket server failed";
                    }
                else if( result == 1 ) {
                    // done, have result

                    char *webResult = 
                        nextConnection->ticketServerRequest->getResult();
                    
                    if( strstr( webResult, "INVALID" ) != NULL ) {
                        AppLog::info( 
                            "Client key hmac rejected by ticket server, "
                            "client rejected." );
                        nextConnection->error = true;
                        nextConnection->errorCauseString =
                            "Client key check failed";
                        }
                    else if( strstr( webResult, "VALID" ) != NULL ) {
                        // correct!


                        const char *message = "ACCEPTED\n#";
                        int messageLength = strlen( message );
                
                        int numSent = 
                            nextConnection->sock->send( 
                                (unsigned char*)message, 
                                messageLength, 
                                false, false );
                        

                        if( numSent != messageLength ) {
                            AppLog::info( "Failed to write to client socket, "
                                          "client rejected." );
                            nextConnection->error = true;
                            nextConnection->errorCauseString =
                                "Socket write failed";

                            }
                        else {
                            // ready to start normal message exchange
                            // with client
                            
                            AppLog::info( "Got new player logged in" );
                            
                            processLoggedInPlayer( 
                                nextConnection->sock,
                                nextConnection->sockBuffer,
                                nextConnection->email );
                            
                            delete nextConnection->ticketServerRequest;
                            newConnections.deleteElement( i );
                            i--;
                            }
                        }
                    else {
                        AppLog::errorF( 
                            "Unexpected result from ticket server, "
                            "client rejected:  %s", webResult );
                        nextConnection->error = true;
                        nextConnection->errorCauseString =
                            "Client key check failed "
                            "(bad ticketServer response)";
                        }
                    delete [] webResult;
                    }
                }
            else {

                double timeDelta = Time::getCurrentTime() -
                    nextConnection->connectionStartTimeSeconds;
                

                

                char result = 
                    readSocketFull( nextConnection->sock,
                                    nextConnection->sockBuffer );
                
                if( ! result ) {
                    AppLog::info( "Failed to read from client socket, "
                                  "client rejected." );
                    nextConnection->error = true;
                    nextConnection->errorCauseString =
                        "Socket read failed";
                    }
                
                char *message = NULL;
                int timeLimit = 10;
                
                if( ! nextConnection->shutdownMode ) {
                    message = 
                        getNextClientMessage( nextConnection->sockBuffer );
                    }
                else {
                    timeLimit = 5;
                    }
                
                if( message != NULL ) {
                    
                    
                    if( strstr( message, "LOGIN" ) != NULL ) {
                        
                        SimpleVector<char *> *tokens =
                            tokenizeString( message );
                        
                        if( tokens->size() == 4 ) {
                            
                            nextConnection->email = 
                                stringDuplicate( 
                                    tokens->getElementDirect( 1 ) );
                            char *pwHash = tokens->getElementDirect( 2 );
                            char *keyHash = tokens->getElementDirect( 3 );
                            

                            char emailAlreadyLoggedIn = false;
                            

                            for( int p=0; p<players.size(); p++ ) {
                                LiveObject *o = players.getElement( p );
                                

                                if( strcmp( o->email, 
                                            nextConnection->email ) == 0 ) {
                                    emailAlreadyLoggedIn = true;
                                    break;
                                    }
                                }

                            if( emailAlreadyLoggedIn ) {
                                AppLog::infoF( 
                                    "Another client already "
                                    "connected as %s, "
                                    "client rejected.",
                                    nextConnection->email );
                                
                                nextConnection->error = true;
                                nextConnection->errorCauseString =
                                    "Duplicate email";
                                }

                            if( requireClientPassword &&
                                ! nextConnection->error  ) {

                                char *value = 
                                    autoSprintf( 
                                        "%d",
                                        nextConnection->sequenceNumber );
                                
                                char *trueHash = hmac_sha1( clientPassword, 
                                                            value );

                                delete [] value;
                                

                                if( strcmp( trueHash, pwHash ) != 0 ) {
                                    AppLog::info( "Client password hmac bad, "
                                                  "client rejected." );
                                    nextConnection->error = true;
                                    nextConnection->errorCauseString =
                                        "Password check failed";
                                    }

                                delete [] trueHash;
                                }
                            
                            if( requireTicketServerCheck &&
                                ! nextConnection->error ) {
                                
                                char *encodedEmail =
                                    URLUtils::urlEncode( 
                                        nextConnection->email );

                                char *url = autoSprintf( 
                                    "%s?action=check_ticket_hash"
                                    "&email=%s"
                                    "&hash_value=%s"
                                    "&string_to_hash=%lu",
                                    ticketServerURL,
                                    encodedEmail,
                                    keyHash,
                                    nextConnection->sequenceNumber );

                                delete [] encodedEmail;

                                nextConnection->ticketServerRequest =
                                    new WebRequest( "GET", url, NULL );
                                
                                delete [] url;
                                }
                            else if( !requireTicketServerCheck &&
                                     !nextConnection->error ) {
                                
                                // let them in without checking
                                
                                const char *message = "ACCEPTED\n#";
                                int messageLength = strlen( message );
                
                                int numSent = 
                                    nextConnection->sock->send( 
                                        (unsigned char*)message, 
                                        messageLength, 
                                        false, false );
                        

                                if( numSent != messageLength ) {
                                    AppLog::info( 
                                        "Failed to send on client socket, "
                                        "client rejected." );
                                    nextConnection->error = true;
                                    nextConnection->errorCauseString =
                                        "Socket write failed";
                                    }
                                else {
                                    // ready to start normal message exchange
                                    // with client
                            
                                    AppLog::info( "Got new player logged in" );
                                    
                                    processLoggedInPlayer( 
                                        nextConnection->sock,
                                        nextConnection->sockBuffer,
                                        nextConnection->email );
                                    
                                    delete nextConnection->ticketServerRequest;
                                    newConnections.deleteElement( i );
                                    i--;
                                    }
                                }
                            }
                        else {
                            AppLog::info( "LOGIN message has wrong format, "
                                          "client rejected." );
                            nextConnection->error = true;
                            nextConnection->errorCauseString =
                                "Bad login message";
                            }


                        tokens->deallocateStringElements();
                        delete tokens;
                        }
                    else {
                        AppLog::info( "Client's first message not LOGIN, "
                                      "client rejected." );
                        nextConnection->error = true;
                        nextConnection->errorCauseString =
                            "Unexpected first message";
                        }
                    
                    delete [] message;
                    }
                else if( timeDelta > timeLimit ) {
                    if( nextConnection->shutdownMode ) {
                        AppLog::info( "5 second grace period for new "
                                      "connection in shutdown mode, closing." );
                        }
                    else {
                        AppLog::info( 
                            "Client failed to LOGIN after 10 seconds, "
                            "client rejected." );
                        }
                    nextConnection->error = true;
                    nextConnection->errorCauseString =
                        "Login timeout";
                    }
                }
            }
            
            
        

        // now clean up any new connections that have errors
        
        for( int i=0; i<newConnections.size(); i++ ) {
            
            FreshConnection *nextConnection = newConnections.getElement( i );
            
            if( nextConnection->error ) {
                
                // try sending REJECTED message at end

                const char *message = "REJECTED\n#";
                nextConnection->sock->send( (unsigned char*)message,
                                            strlen( message ), false, false );

                AppLog::infoF( "Closing new connection on error "
                               "(cause: %s)",
                               nextConnection->errorCauseString );

                delete nextConnection->sock;
                delete nextConnection->sockBuffer;
                
                if( nextConnection->ticketServerRequest != NULL ) {
                    delete nextConnection->ticketServerRequest;
                    }
                
                if( nextConnection->email != NULL ) {
                    delete [] nextConnection->email;
                    }

                newConnections.deleteElement( i );
                i--;
                }
            }
        
    
        
    
        someClientMessageReceived = false;

        numLive = players.size();
        

        // listen for any messages from clients 

        // track index of each player that needs an update sent about it
        // we compose the full update message below
        SimpleVector<int> playerIndicesToSendUpdatesAbout;
        
        SimpleVector<int> playerIndicesToSendLineageAbout;

        SimpleVector<int> playerIndicesToSendNamesAbout;

        SimpleVector<int> playerIndicesToSendDyingAbout;

        // accumulated text of update lines
        SimpleVector<char> newUpdates;
        SimpleVector<ChangePosition> newUpdatesPos;
        SimpleVector<int> newUpdatePlayerIDs;


        // separate accumulated text of updates for player deletes
        // these are global, so they're not tagged with positions for
        // spatial filtering
        SimpleVector<char> newDeleteUpdates;
        

        SimpleVector<char> mapChanges;
        SimpleVector<ChangePosition> mapChangesPos;
        
        SimpleVector<char> newSpeech;
        SimpleVector<ChangePosition> newSpeechPos;

        
        timeSec_t curLookTime = Time::timeSec();
        
        for( int i=0; i<numLive; i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            
            nextPlayer->updateSent = false;

            if( nextPlayer->error ) {
                continue;
                }
            
            GridPos curPos = { nextPlayer->xd, nextPlayer->yd };
            
            if( nextPlayer->xd != nextPlayer->xs ||
                nextPlayer->yd != nextPlayer->ys ) {
                
                curPos = computePartialMoveSpot( nextPlayer );
                }
            
            int curOverID = getMapObject( curPos.x, curPos.y );
            

            if( ! nextPlayer->heldByOther &&
                curOverID != 0 && 
                ! isMapObjectInTransit( curPos.x, curPos.y ) ) {
                
                ObjectRecord *curOverObj = getObject( curOverID );
                
                if( curOverObj->permanent && curOverObj->deadlyDistance > 0 ) {
                    
                    setDeathReason( nextPlayer, 
                                    "killed",
                                    curOverID );

                    nextPlayer->error = true;
                    nextPlayer->errorCauseString =
                        "Player killed by permanent object";

                    // generic on-person
                    TransRecord *r = 
                        getTrans( curOverID, 0 );

                    if( r != NULL ) {
                        setMapObject( curPos.x, curPos.y, r->newActor );
                        }
                    continue;
                    }
                }
            
            
            if( curLookTime - nextPlayer->lastRegionLookTime > 5 ) {
                lookAtRegion( nextPlayer->xd - 8, nextPlayer->yd - 7,
                              nextPlayer->xd + 8, nextPlayer->yd + 7 );
                nextPlayer->lastRegionLookTime = curLookTime;
                }

            
            char result = 
                readSocketFull( nextPlayer->sock, nextPlayer->sockBuffer );
            
            if( ! result ) {
                setDeathReason( nextPlayer, "disconnected" );
                
                nextPlayer->error = true;
                nextPlayer->errorCauseString =
                    "Socket read failed";
                }

            char *message = getNextClientMessage( nextPlayer->sockBuffer );
            
            if( message != NULL ) {
                someClientMessageReceived = true;
                
                AppLog::infoF( "Got client message from %d: %s",
                               nextPlayer->id, message );
                
                ClientMessage m = parseMessage( message );
                
                delete [] message;
                
                if( m.type == UNKNOWN ) {
                    AppLog::info( "Client error, unknown message type." );
                    
                    setDeathReason( nextPlayer, "disconnected" );

                    nextPlayer->error = true;
                    nextPlayer->errorCauseString =
                        "Unknown message type";
                    }

                //Thread::staticSleep( 
                //    testRandSource.getRandomBoundedInt( 0, 450 ) );
                
                if( m.type == BUG ) {
                    int allow = 
                        SettingsManager::getIntSetting( "allowBugReports", 0 );

                    if( allow ) {
                        char *bugName = 
                            autoSprintf( "bug_%d_%d_%f",
                                         m.bug,
                                         nextPlayer->id,
                                         Time::getCurrentTime() );
                        char *bugInfoName = autoSprintf( "%s_info.txt",
                                                         bugName );
                        char *bugOutName = autoSprintf( "%s_out.txt",
                                                        bugName );
                        FILE *bugInfo = fopen( bugInfoName, "w" );
                        if( bugInfo != NULL ) {
                            fprintf( bugInfo, 
                                     "Bug report from player %d\n"
                                     "Bug text:  %s\n", 
                                     nextPlayer->id,
                                     m.bugText );
                            fclose( bugInfo );
                            
                            File outFile( NULL, "serverOut.txt" );
                            if( outFile.exists() ) {
                                fflush( stdout );
                                File outCopyFile( NULL, bugOutName );
                                
                                outFile.copy( &outCopyFile );
                                }
                            }
                        delete [] bugName;
                        delete [] bugInfoName;
                        delete [] bugOutName;
                        }
                    }
                else if( m.type == MAP ) {
                    
                    int allow = 
                        SettingsManager::getIntSetting( "allowMapRequests", 0 );
                    

                    if( allow ) {
                        
                        SimpleVector<char *> *list = 
                            SettingsManager::getSetting( 
                                "mapRequestAllowAccounts" );
                        
                        allow = false;
                        
                        for( int i=0; i<list->size(); i++ ) {
                            if( strcmp( nextPlayer->email,
                                        list->getElementDirect( i ) ) == 0 ) {
                                
                                allow = true;
                                break;
                                }
                            }
                        
                        list->deallocateStringElements();
                        delete list;
                        }
                    

                    if( allow ) {
                        int length;
                        unsigned char *mapChunkMessage = 
                            getChunkMessage( m.x - chunkDimensionX / 2, 
                                             m.y - chunkDimensionY / 2,
                                             chunkDimensionX,
                                             chunkDimensionY, 
                                             &length );
                        
                        int numSent = 
                            nextPlayer->sock->send( mapChunkMessage, 
                                                    length, 
                                                    false, false );
                
                        delete [] mapChunkMessage;

                        if( numSent != length ) {
                            setDeathReason( nextPlayer, "disconnected" );

                            nextPlayer->error = true;
                            nextPlayer->errorCauseString =
                                "Socket write failed";
                            }
                        }
                    else {
                        AppLog::infoF( "Map pull request rejected for %s", 
                                       nextPlayer->email );
                        }
                    }
                else if( m.type == TRIGGER ) {
                    if( areTriggersEnabled() ) {
                        trigger( m.trigger );
                        }
                    }
                // if player is still moving, ignore all actions
                // except for move interrupts
                else if( ( nextPlayer->xs == nextPlayer->xd &&
                      nextPlayer->ys == nextPlayer->yd ) 
                    ||
                    m.type == MOVE ||
                    m.type == SAY ) {
                    

                    if( m.type == MOVE && nextPlayer->heldByOther ) {
                        // baby wiggling out of parent's arms
                        handleForcedBabyDrop( 
                            nextPlayer,
                            &playerIndicesToSendUpdatesAbout );
                        
                        // drop them and ignore rest of their move
                        // request, until they click again
                        }
                    else if( m.type == MOVE ) {
                        //Thread::staticSleep( 1000 );


                        printf( "  Processing move, "
                                "we think player at old start pos %d,%d\n",
                                nextPlayer->xs,
                                nextPlayer->ys );
                        printf( "  Player's last path = " );
                        for( int p=0; p<nextPlayer->pathLength; p++ ) {
                            printf( "(%d, %d) ",
                                    nextPlayer->pathToDest[p].x, 
                                    nextPlayer->pathToDest[p].y );
                            }
                        printf( "\n" );

                        char interrupt = false;
                        char pathPrefixAdded = false;
                        
                        // first, construct a path from any existing
                        // path PLUS path that player is suggesting
                        SimpleVector<GridPos> unfilteredPath;

                        if( nextPlayer->xs != m.x ||
                            nextPlayer->ys != m.y ) {
                            
                            // start pos of their submitted path
                            // donesn't match where we think they are

                            // it could be an interrupt to past move
                            // OR, if our server sees move as done but client 
                            // doesn't yet, they may be sending a move
                            // from the middle of their last path.

                            // treat this like an interrupt to last move
                            // in both cases.

                            // a new move interrupting a non-stationary object
                            interrupt = true;

                            // where we think they are along last move path
                            GridPos cPos;
                            
                            if( nextPlayer->xs != nextPlayer->xd 
                                ||
                                nextPlayer->ys != nextPlayer->yd ) {
                                
                                // a real interrupt to a move that is
                                // still in-progress on server
                                cPos = computePartialMoveSpot( nextPlayer );
                                }
                            else {
                                // we think their last path is done
                                cPos.x = nextPlayer->xs;
                                cPos.y = nextPlayer->ys;
                                }
                            
                            
                            printf( "   we think player in motion or "
                                    "done moving at %d,%d\n",
                                    cPos.x,
                                    cPos.y );
       
                            nextPlayer->xs = cPos.x;
                            nextPlayer->ys = cPos.y;
                            
                            
                            char cOnTheirNewPath = false;
                            

                            for( int p=0; p<m.numExtraPos; p++ ) {
                                if( equal( cPos, m.extraPos[p] ) ) {
                                    cOnTheirNewPath = true;
                                    break;
                                    }
                                }
                            
                            if( cPos.x == m.x && cPos.y == m.y ) {
                                // also if equal to their start pos
                                cOnTheirNewPath = true;
                                }
                            


                            if( !cOnTheirNewPath &&
                                nextPlayer->pathLength > 0 ) {

                                // add prefix to their path from
                                // c to the start of their path
                                
                                // index where they think they are

                                // could be ahead or behind where we think
                                // they are
                                
                                int theirPathIndex = -1;
                            
                                for( int p=0; p<nextPlayer->pathLength; p++ ) {
                                    GridPos pos = nextPlayer->pathToDest[p];
                                    
                                    if( m.x == pos.x && m.y == pos.y ) {
                                        // reached point along old path
                                        // where player thinks they 
                                        // actually are
                                        theirPathIndex = p;
                                        break;
                                        }
                                    }
                                
                                char theirIndexNotFound = false;
                                
                                if( theirPathIndex == -1 ) {
                                    // if not found, assume they think they
                                    // are at start of their old path
                                    
                                    theirIndexNotFound = true;
                                    theirPathIndex = 0;
                                    }
                                

                                printf( "They are on our path at index %d\n",
                                        theirPathIndex );
                                
                                // okay, they think they are on last path
                                // that we had for them

                                // step through path from where WE
                                // think they should be to where they
                                // think they are and add this as a prefix
                                // to the path they submitted
                                // (we may walk backward along the old
                                //  path to do this)
                                
                                int c = computePartialMovePathStep( 
                                    nextPlayer );
                                    
                                // -1 means starting, pre-path 
                                // pos is closest
                                // but okay to leave c at -1, because
                                // we will add pathStep=1 to it

                                int pathStep = 0;
                                    
                                if( theirPathIndex < c ) {
                                    pathStep = -1;
                                    }
                                else if( theirPathIndex > c ) {
                                    pathStep = 1;
                                    }
                                    
                                if( pathStep != 0 ) {
                                    for( int p = c + pathStep; 
                                         p != theirPathIndex + pathStep; 
                                         p += pathStep ) {
                                        GridPos pos = 
                                            nextPlayer->pathToDest[p];
                                            
                                        unfilteredPath.push_back( pos );
                                        }
                                    }

                                if( theirIndexNotFound ) {
                                    // add their path's starting pos
                                    // at the end of the prefix
                                    GridPos pos = { m.x, m.y };
                                    
                                    unfilteredPath.push_back( pos );
                                    }
                                
                                // otherwise, they are where we think
                                // they are, and we don't need to prefix
                                // their path

                                printf( "Prefixing their path "
                                        "with %d steps\n",
                                        unfilteredPath.size() );
                                }
                            }
                        
                        if( unfilteredPath.size() > 0 ) {
                            pathPrefixAdded = true;
                            }

                        // now add path player says they want to go down

                        for( int p=0; p < m.numExtraPos; p++ ) {
                            unfilteredPath.push_back( m.extraPos[p] );
                            }
                        

                        printf( "Unfiltered path = " );
                        for( int p=0; p<unfilteredPath.size(); p++ ) {
                            printf( "(%d, %d) ",
                                    unfilteredPath.getElementDirect(p).x, 
                                    unfilteredPath.getElementDirect(p).y );
                            }
                        printf( "\n" );

                        // remove any duplicate spots due to doubling back

                        for( int p=1; p<unfilteredPath.size(); p++ ) {
                            
                            if( equal( unfilteredPath.getElementDirect(p-1),
                                       unfilteredPath.getElementDirect(p) ) ) {
                                unfilteredPath.deleteElement( p );
                                p--;
                                printf( "FOUND duplicate path element\n" );
                                }
                            }
                            
                                
                                       
                        
                        nextPlayer->xd = m.extraPos[ m.numExtraPos - 1].x;
                        nextPlayer->yd = m.extraPos[ m.numExtraPos - 1].y;
                        
                        
                        if( nextPlayer->xd == nextPlayer->xs &&
                            nextPlayer->yd == nextPlayer->ys ) {
                            // this move request truncates to where
                            // we think player actually is

                            // send update to terminate move right now
                            playerIndicesToSendUpdatesAbout.push_back( i );
                            printf( "A move that takes player "
                                    "where they already are, "
                                    "ending move now\n" );
                            }
                        else {
                            // an actual move away from current xs,ys

                            if( interrupt ) {
                                printf( "Got valid move interrupt\n" );
                                }
                                

                            // check path for obstacles
                            // and make sure it contains the location
                            // where we think they are
                            
                            char truncated = 0;
                            
                            SimpleVector<GridPos> validPath;

                            char startFound = false;
                            
                            
                            int startIndex = 0;
                            // search from end first to find last occurrence
                            // of start pos
                            for( int p=unfilteredPath.size() - 1; p>=0; p-- ) {
                                
                                if( unfilteredPath.getElementDirect(p).x 
                                      == nextPlayer->xs
                                    &&
                                    unfilteredPath.getElementDirect(p).y 
                                      == nextPlayer->ys ) {
                                    
                                    startFound = true;
                                    startIndex = p;
                                    break;
                                    }
                                }
                            
                            printf( "Start index = %d (startFound = %d)\n", 
                                    startIndex, startFound );
                            
                            if( ! startFound &&
                                ! isGridAdjacentDiag( 
                                    unfilteredPath.
                                      getElementDirect(startIndex).x,
                                    unfilteredPath.
                                      getElementDirect(startIndex).y,
                                    nextPlayer->xs,
                                    nextPlayer->ys ) ) {
                                // path start jumps away from current player 
                                // start
                                // ignore it
                                }
                            else {
                                
                                GridPos lastValidPathStep =
                                    { m.x, m.y };
                                
                                if( pathPrefixAdded ) {
                                    lastValidPathStep.x = nextPlayer->xs;
                                    lastValidPathStep.y = nextPlayer->ys;
                                    }
                                
                                // we know where we think start
                                // of this path should be,
                                // but player may be behind this point
                                // on path (if we get their message late)
                                // So, it's not safe to pre-truncate
                                // the path

                                // However, we will adjust timing, below,
                                // to match where we think they should be
                                
                                // enforce client behavior of not walking
                                // down through objects in our cell that are
                                // blocking us
                                char currentBlocked = false;
                                
                                if( isMapSpotBlocking( lastValidPathStep.x,
                                                       lastValidPathStep.y ) ) {
                                    currentBlocked = true;
                                    }
                                

                                for( int p=0; 
                                     p<unfilteredPath.size(); p++ ) {
                                
                                    GridPos pos = 
                                        unfilteredPath.getElementDirect(p);

                                    if( isMapSpotBlocking( pos.x, pos.y ) ) {
                                        // blockage in middle of path
                                        // terminate path here
                                        truncated = 1;
                                        break;
                                        }
                                    
                                    if( currentBlocked && p == 0 &&
                                        pos.y == lastValidPathStep.y - 1 ) {
                                        // attempt to walk down through
                                        // blocking object at starting location
                                        truncated = 1;
                                        break;
                                        }
                                    

                                    // make sure it's not more
                                    // than one step beyond
                                    // last step

                                    if( ! isGridAdjacentDiag( 
                                            pos, lastValidPathStep ) ) {
                                        // a path with a break in it
                                        // terminate it here
                                        truncated = 1;
                                        break;
                                        }
                                    
                                    // no blockage, no gaps, add this step
                                    validPath.push_back( pos );
                                    lastValidPathStep = pos;
                                    }
                                }
                            
                            if( validPath.size() == 0 ) {
                                // path not permitted
                                AppLog::info( "Path submitted by player "
                                              "not valid, "
                                              "ending move now" );
                                //assert( false );
                                nextPlayer->xd = nextPlayer->xs;
                                nextPlayer->yd = nextPlayer->ys;
                                
                                nextPlayer->posForced = true;

                                // send update about them to end the move
                                // right now
                                playerIndicesToSendUpdatesAbout.push_back( i );
                                }
                            else {
                                // a good path
                                
                                if( nextPlayer->pathToDest != NULL ) {
                                    delete [] nextPlayer->pathToDest;
                                    nextPlayer->pathToDest = NULL;
                                    }

                                nextPlayer->pathTruncated = truncated;
                                
                                nextPlayer->pathLength = validPath.size();
                                
                                nextPlayer->pathToDest = 
                                    validPath.getElementArray();
                                    
                                // path may be truncated from what was 
                                // requested, so set new d
                                nextPlayer->xd = 
                                    nextPlayer->pathToDest[ 
                                        nextPlayer->pathLength - 1 ].x;
                                nextPlayer->yd = 
                                    nextPlayer->pathToDest[ 
                                        nextPlayer->pathLength - 1 ].y;

                                // distance is number of orthogonal steps
                            
                                double dist = 
                                    measurePathLength( nextPlayer->xs,
                                                       nextPlayer->ys,
                                                       nextPlayer->pathToDest,
                                                       nextPlayer->pathLength );
 
                                double distAlreadyDone =
                                    measurePathLength( nextPlayer->xs,
                                                       nextPlayer->ys,
                                                       nextPlayer->pathToDest,
                                                       startIndex );
                             
                                double moveSpeed = computeMoveSpeed( 
                                    nextPlayer );
                                
                                nextPlayer->moveTotalSeconds = dist / 
                                    moveSpeed;
                           
                                double secondsAlreadyDone = distAlreadyDone / 
                                    moveSpeed;
                                
                                printf( "Skipping %f seconds along new %f-"
                                        "second path\n",
                                        secondsAlreadyDone, 
                                        nextPlayer->moveTotalSeconds );
                                
                                nextPlayer->moveStartTime = 
                                    Time::getCurrentTime() - 
                                    secondsAlreadyDone;
                            
                                nextPlayer->newMove = true;
                                }
                            }
                        }
                    else if( m.type == SAY && m.saidText != NULL &&
                             Time::getCurrentTime() - 
                             nextPlayer->lastSayTimeSeconds > 
                             minSayGapInSeconds ) {
                        
                        nextPlayer->lastSayTimeSeconds = 
                            Time::getCurrentTime();

                        unsigned int sayLimit = getSayLimit( nextPlayer );
                        
                        if( strlen( m.saidText ) > sayLimit ) {
                            // truncate
                            m.saidText[ sayLimit ] = '\0';
                            }
                        
                        if( nextPlayer->isEve && nextPlayer->name == NULL ) {
                            char *name = isFamilyNamingSay( m.saidText );
                            
                            if( name != NULL && strcmp( name, "" ) != 0 ) {
                                const char *close = findCloseLastName( name );
                                nextPlayer->name = autoSprintf( "%s %s",
                                                                eveName, 
                                                                close );
                                logName( nextPlayer->id,
                                         nextPlayer->name );
                                playerIndicesToSendNamesAbout.push_back( i );
                                }
                            }

                        if( nextPlayer->holdingID < 0 &&
                            nextPlayer->babyIDs->size() > 0 &&
                            nextPlayer->babyIDs->getElementIndex(
                                - nextPlayer->holdingID ) != -1 ) {

                            // we're holding one of our babies
                            
                            LiveObject *babyO =
                                getLiveObject( - nextPlayer->holdingID );
                            
                            if( babyO != NULL && babyO->name == NULL ) {
                                char *name = isBabyNamingSay( m.saidText );
                                
                                if( name != NULL && strcmp( name, "" ) != 0 ) {
                                    const char *lastName = "";
                                    if( nextPlayer->name != NULL ) {
                                        lastName = strstr( nextPlayer->name, 
                                                           " " );
                                        
                                        if( lastName == NULL ) {
                                            lastName = "";
                                            }
                                        }

                                    const char *close = 
                                        findCloseFirstName( name );
                                    babyO->name = autoSprintf( "%s%s",
                                                               close, 
                                                               lastName );
                                    logName( babyO->id,
                                             babyO->name );
                                    
                                    playerIndicesToSendNamesAbout.push_back( 
                                        getLiveObjectIndex( babyO->id ) );
                                    }
                                }
                            }
                        
                        

                        
                        char *line = autoSprintf( "%d %s\n", nextPlayer->id,
                                                  m.saidText );
                        
                        newSpeech.appendElementString( line );
                        
                        delete [] line;
                        

                        
                        ChangePosition p = { nextPlayer->xd, nextPlayer->yd, 
                                             false };
                        
                        // if held, speech happens where held
                        if( nextPlayer->heldByOther ) {
                            LiveObject *holdingPlayer = 
                                getLiveObject( nextPlayer->heldByOtherID );
                
                            p.x = holdingPlayer->xd;
                            p.y = holdingPlayer->yd;
                            }


                        newSpeechPos.push_back( p );
                        }
                    else if( m.type == KILL ) {
                        // send update even if action fails (to let them
                        // know that action is over)
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        
                        if( nextPlayer->holdingID > 0 &&
                            ! (m.x == nextPlayer->xd &&
                               m.y == nextPlayer->yd ) ) {
                            
                            nextPlayer->actionAttempt = 1;
                            nextPlayer->actionTarget.x = m.x;
                            nextPlayer->actionTarget.y = m.y;
                            
                            if( m.x > nextPlayer->xd ) {
                                nextPlayer->facingOverride = 1;
                                }
                            else if( m.x < nextPlayer->xd ) {
                                nextPlayer->facingOverride = -1;
                                }

                            // holding something
                            ObjectRecord *heldObj = 
                                getObject( nextPlayer->holdingID );
                            
                            if( heldObj->deadlyDistance > 0 ) {
                                // it's deadly

                                GridPos targetPos = { m.x, m.y };
                                GridPos playerPos = { nextPlayer->xd,
                                                      nextPlayer->yd };
                                
                                double d = distance( targetPos,
                                                     playerPos );
                                
                                if( heldObj->deadlyDistance >= d &&
                                    ! directLineBlocked( playerPos, 
                                                         targetPos ) ) {
                                    // target is close enough
                                    // and no blocking objects along the way
                                    
                                    // is anyone there?
                                    LiveObject *hitPlayer = 
                                        getHitPlayer( m.x, m.y, true );
                                    
                                    char someoneHit = false;

                                    if( hitPlayer != NULL ) {
                                        someoneHit = true;
                                        // break the connection with 
                                        // them, eventually
                                        // let them stagger a bit first

                                        hitPlayer->murderSourceID =
                                            nextPlayer->holdingID;
                                        
                                        setDeathReason( hitPlayer, 
                                                        "killed",
                                                        nextPlayer->holdingID );

                                        // if not already dying
                                        if( ! hitPlayer->dying ) {
                                            int staggerTime = 
                                                SettingsManager::getIntSetting(
                                                    "deathStaggerTime", 20 );

                                            hitPlayer->dying = true;
                                            hitPlayer->dyingETA = 
                                                Time::getCurrentTime() + 
                                                staggerTime;
                                            playerIndicesToSendDyingAbout.
                                                push_back( 
                                                    getLiveObjectIndex( 
                                                        hitPlayer->id ) );
                                        
                                            hitPlayer->errorCauseString =
                                                "Player killed by other player";
                                        
                                            logDeath( hitPlayer->id,
                                                      hitPlayer->email,
                                                      hitPlayer->isEve,
                                                      computeAge( hitPlayer ),
                                                      getSecondsPlayed( 
                                                          hitPlayer ),
                                                      ! getFemale( hitPlayer ),
                                                      m.x, m.y,
                                                      players.size() - 1,
                                                      false,
                                                      nextPlayer->id,
                                                      nextPlayer->email );
                                            
                                            if( shutdownMode ) {
                                                handleShutdownDeath( 
                                                    hitPlayer, m.x, m.y );
                                                }

                                            hitPlayer->deathLogged = true;
                                            }
                                        }
                                    
                                    
                                    // a player either hit or not
                                    // in either case, weapon was used
                                    
                                    // check for a transition for weapon

                                    // 0 is generic "on person" target
                                    TransRecord *r = 
                                        getTrans( nextPlayer->holdingID, 
                                                  0 );

                                    TransRecord *rHit = NULL;
                                    
                                    if( someoneHit ) {
                                        // last use on target specifies
                                        // grave and weapon change on hit
                                        // non-last use (r above) specifies
                                        // what projectile ends up in grave
                                        // or on ground
                                        rHit = 
                                            getTrans( nextPlayer->holdingID, 
                                                      0, false, true );
                                        
                                        if( rHit != NULL &&
                                            rHit->newTarget > 0 ) {
                                            hitPlayer->customGraveID = 
                                                rHit->newTarget;
                                            }

                                        // last use on actor specifies
                                        // what is left in victim's hand
                                        TransRecord *woundHit = 
                                            getTrans( nextPlayer->holdingID, 
                                                      0, true, false );
                                        
                                        if( woundHit != NULL &&
                                            woundHit->newTarget > 0 ) {
                                            
                                            // don't drop their wound
                                            if( hitPlayer->holdingID != 0 &&
                                                ! hitPlayer->holdingWound ) {
                                                handleDrop( 
                                                    m.x, m.y, 
                                                    hitPlayer,
                                             &playerIndicesToSendUpdatesAbout );
                                                }
                                            hitPlayer->holdingID = 
                                                woundHit->newTarget;
                                            hitPlayer->holdingWound = true;
                                            
                                            playerIndicesToSendUpdatesAbout.
                                                push_back( 
                                                    getLiveObjectIndex( 
                                                        hitPlayer->id ) );
                                            }   
                                        }
                                    

                                    int oldHolding = nextPlayer->holdingID;
                                    timeSec_t oldEtaDecay = 
                                        nextPlayer->holdingEtaDecay;

                                    if( rHit != NULL ) {
                                        // if hit trans exist
                                        // leave bloody knife or
                                        // whatever in hand
                                        nextPlayer->holdingID = rHit->newActor;
                                        }
                                    else if( r != NULL ) {
                                        nextPlayer->holdingID = r->newActor;
                                        }


                                    if( r != NULL || rHit != NULL ) {
                                        
                                        nextPlayer->heldTransitionSourceID = 0;
                                        
                                        if( oldHolding != 
                                            nextPlayer->holdingID ) {
                                            
                                            setFreshEtaDecayForHeld( 
                                                nextPlayer );
                                            }
                                        }
                                    

                                    if( r != NULL ) {
                                    
                                        if( hitPlayer != NULL &&
                                            r->newTarget != 0 ) {
                                        
                                            hitPlayer->embeddedWeaponID = 
                                                r->newTarget;
                                        
                                            if( oldHolding == r->newTarget ) {
                                                // what we are holding
                                                // is now embedded in them
                                                // keep old decay
                                                hitPlayer->
                                                    embeddedWeaponEtaDecay =
                                                    oldEtaDecay;
                                                }
                                            else {
                                            
                                                TransRecord *newDecayT = 
                                                    getTrans( -1, 
                                                              r->newTarget );
                    
                                                if( newDecayT != NULL ) {
                                                    hitPlayer->
                                                     embeddedWeaponEtaDecay = 
                                                        Time::timeSec() + 
                                                        newDecayT->
                                                        autoDecaySeconds;
                                                    }
                                                else {
                                                    // no further decay
                                                    hitPlayer->
                                                        embeddedWeaponEtaDecay 
                                                        = 0;
                                                    }
                                                }
                                            }
                                        else if( hitPlayer == NULL &&
                                                 isMapSpotEmpty( m.x, 
                                                                 m.y ) ) {
                                            // no player hit, and target ground
                                            // spot is empty
                                            setMapObject( m.x, m.y, 
                                                          r->newTarget );
                                        
                                            // if we're thowing a weapon
                                            // target is same as what we
                                            // were holding
                                            if( oldHolding == r->newTarget ) {
                                                // preserve old decay time 
                                                // of what we were holding
                                                setEtaDecay( m.x, m.y,
                                                             oldEtaDecay );
                                                }
                                            }
                                        // else new target, post-kill-attempt
                                        // is lost
                                        }
                                    }
                                }
                            }
                        }
                    else if( m.type == USE ) {
                        // send update even if action fails (to let them
                        // know that action is over)
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        
                        char distanceUseAllowed = false;
                        
                        if( nextPlayer->holdingID > 0 ) {
                            
                            // holding something
                            ObjectRecord *heldObj = 
                                getObject( nextPlayer->holdingID );
                            
                            if( heldObj->useDistance > 1 ) {
                                // it's long-distance

                                GridPos targetPos = { m.x, m.y };
                                GridPos playerPos = { nextPlayer->xd,
                                                      nextPlayer->yd };
                                
                                double d = distance( targetPos,
                                                     playerPos );
                                
                                if( heldObj->useDistance >= d &&
                                    ! directLineBlocked( playerPos, 
                                                         targetPos ) ) {
                                    distanceUseAllowed = true;
                                    }
                                }
                            }
                        

                        if( distanceUseAllowed 
                            ||
                            isGridAdjacent( m.x, m.y,
                                            nextPlayer->xd, 
                                            nextPlayer->yd ) 
                            ||
                            ( m.x == nextPlayer->xd &&
                              m.y == nextPlayer->yd ) ) {
                            
                            nextPlayer->actionAttempt = 1;
                            nextPlayer->actionTarget.x = m.x;
                            nextPlayer->actionTarget.y = m.y;
                            
                            if( m.x > nextPlayer->xd ) {
                                nextPlayer->facingOverride = 1;
                                }
                            else if( m.x < nextPlayer->xd ) {
                                nextPlayer->facingOverride = -1;
                                }

                            // can only use on targets next to us for now,
                            // no diags
                            
                            int target = getMapObject( m.x, m.y );
                            
                            int oldHolding = nextPlayer->holdingID;
                            
                            if( target != 0 ) {                                
                                ObjectRecord *targetObj = getObject( target );
                                
                                // try using object on this target 
                                char transApplied = false;
                                
                                TransRecord *r = NULL;
                                char defaultTrans = false;
                                
                                char heldCanBeUsed = false;
                                char containmentTransfer = false;
                                if( // if what we're holding contains
                                    // stuff, block it from being
                                    // used as a tool
                                    nextPlayer->numContained == 0 ) {
                                    heldCanBeUsed = true;
                                    }
                                else if( nextPlayer->holdingID > 0 ) {
                                    // see if result of trans
                                    // would preserve containment

                                    r = getTrans( nextPlayer->holdingID,
                                                  target );


                                    ObjectRecord *heldObj = getObject( 
                                        nextPlayer->holdingID );
                                    
                                    if( r != NULL && r->newActor == 0 &&
                                        r->newTarget > 0 ) {
                                        ObjectRecord *newTargetObj =
                                            getObject( r->newTarget );
                                        
                                        if( targetObj->numSlots == 0
                                            && newTargetObj->numSlots >=
                                            heldObj->numSlots
                                            &&
                                            newTargetObj->slotSize >=
                                            heldObj->slotSize ) {
                                            
                                            containmentTransfer = true;
                                            heldCanBeUsed = true;
                                            }
                                        }
                                    r = NULL;
                                    }
                                

                                if( nextPlayer->holdingID >= 0 &&
                                    heldCanBeUsed ) {
                                    // negative holding is ID of baby
                                    // which can't be used
                                    // (and no bare hand action available)
                                    r = getTrans( nextPlayer->holdingID,
                                                  target );
                                    
                                    transApplied = true;
                                    
                                    if( r == NULL && 
                                        ( nextPlayer->holdingID > 0 || 
                                          targetObj->permanent ) ) {
                                        
                                        // search for default 
                                        r = getTrans( -2, target );
                                        
                                        if( r != NULL ) {
                                            defaultTrans = true;
                                            }
                                        }
                                    
                                    if( r == NULL && 
                                        nextPlayer->holdingID > 0 ) {
                                        
                                        logTransitionFailure( 
                                            nextPlayer->holdingID,
                                            target );
                                        }
                                    }
                                
                                if( r != NULL && containmentTransfer ) {
                                    // special case contained items
                                    // moving from actor into new target
                                    // (and hand left empty)
                                    setResponsiblePlayer( - nextPlayer->id );
                                    setMapObject( m.x, m.y, r->newTarget );
                                    setResponsiblePlayer( -1 );
                                    
                                    transferHeldContainedToMap( nextPlayer,
                                                                m.x, m.y );
                                    handleHoldingChange( nextPlayer,
                                                         r->newActor );
                                    }
                                else if( r != NULL &&
                                    // are we old enough to handle
                                    // what we'd get out of this transition?
                                    ( r->newActor == 0 || 
                                      getObject( r->newActor )->minPickupAge <= 
                                      computeAge( nextPlayer ) ) 
                                    &&
                                    // does this create a blocking object?
                                    // only consider vertical-blocking
                                    // objects (like vertical walls and doors)
                                    // because these look especially weird
                                    // on top of a player
                                    // We can detect these because they 
                                    // also hug the floor
                                    // Horizontal doors look fine when
                                    // closed on player because of their
                                    // vertical offset.
                                    //
                                    // if so, make sure there's not someone
                                    // standing still there
                                    ( r->newTarget == 0 ||
                                      ! 
                                      ( getObject( r->newTarget )->
                                          blocksWalking
                                        &&
                                        getObject( r->newTarget )->
                                          floorHugging )
                                      ||
                                      isMapSpotEmptyOfPlayers( m.x, 
                                                               m.y ) ) ) {
                                    
                                    if( ! defaultTrans ) {    
                                        handleHoldingChange( nextPlayer,
                                                             r->newActor );
                                        
                                        if( r->target > 0 ) {    
                                            nextPlayer->heldTransitionSourceID =
                                                r->target;
                                            }
                                        else {
                                            nextPlayer->heldTransitionSourceID =
                                                -1;
                                            }
                                        }
                                    


                                    // has target shrunken as a container?
                                    int oldSlots = 
                                        getNumContainerSlots( target );
                                    int newSlots = 
                                        getNumContainerSlots( r->newTarget );
                                    
                                    if( oldSlots > 0 &&
                                        newSlots == 0 && 
                                        r->actor == 0 &&
                                        r->newActor > 0
                                        &&
                                        getNumContainerSlots( r->newActor ) ==
                                        oldSlots &&
                                        getObject( r->newActor )->slotSize >=
                                        targetObj->slotSize ) {
                                        
                                        // bare-hand action that results
                                        // in something new in hand
                                        // with same number of slots 
                                        // as target
                                        // keep what was contained

                                        FullMapContained f =
                                            getFullMapContained( m.x, m.y );

                                        setContained( nextPlayer, f );
                                    
                                        clearAllContained( m.x, m.y );
                                        
                                        restretchDecays( 
                                            nextPlayer->numContained,
                                            nextPlayer->containedEtaDecays,
                                            target, r->newActor );
                                        }
                                    else {
                                        // target on ground changed
                                        // and we don't have the same
                                        // number of slots in a new held obj
                                        
                                        shrinkContainer( m.x, m.y, newSlots );
                                    
                                        if( newSlots > 0 ) {    
                                            restretchMapContainedDecays( 
                                                m.x, m.y,
                                                target,
                                                r->newTarget );
                                            }
                                        }
                                    
                                    
                                    timeSec_t oldEtaDecay = 
                                        getEtaDecay( m.x, m.y );
                                    
                                    setResponsiblePlayer( - nextPlayer->id );
                                    
                                    if( r->newTarget > 0 
                                        && getObject( r->newTarget )->floor ) {

                                        // it turns into a floor
                                        setMapObject( m.x, m.y, 0 );
                                        
                                        setMapFloor( m.x, m.y, r->newTarget );
                                        
                                        if( r->newTarget == target ) {
                                            // unchanged
                                            // keep old decay in place
                                            setFloorEtaDecay( m.x, m.y, 
                                                              oldEtaDecay );
                                            }
                                        }
                                    else {    
                                        setMapObject( m.x, m.y, r->newTarget );
                                        }
                                    
                                    
                                    setResponsiblePlayer( -1 );

                                    if( target == r->newTarget ) {
                                        // target not changed
                                        // keep old decay in place
                                        setEtaDecay( m.x, m.y, oldEtaDecay );
                                        }
                                    
                                    if( r->newTarget != 0 ) {
                                        
                                        handleMapChangeToPaths( 
                                            m.x, m.y,
                                            getObject( r->newTarget ),
                                            &playerIndicesToSendUpdatesAbout );
                                        }
                                    }
                                else if( nextPlayer->holdingID == 0 &&
                                         ! targetObj->permanent &&
                                         targetObj->minPickupAge <= 
                                         computeAge( nextPlayer ) ) {
                                    // no bare-hand transition applies to
                                    // this non-permanent target object
                                    
                                    // treat it like pick up
                                    transApplied = true;
                                    
                                    nextPlayer->holdingEtaDecay = 
                                        getEtaDecay( m.x, m.y );
                                    
                                    FullMapContained f =
                                        getFullMapContained( m.x, m.y );

                                    setContained( nextPlayer, f );
                                    
                                    clearAllContained( m.x, m.y );
                                    
                                    setResponsiblePlayer( - nextPlayer->id );
                                    setMapObject( m.x, m.y, 0 );
                                    setResponsiblePlayer( -1 );
                                    
                                    nextPlayer->holdingID = target;
                                    
                                    nextPlayer->heldOriginValid = 1;
                                    nextPlayer->heldOriginX = m.x;
                                    nextPlayer->heldOriginY = m.y;
                                    nextPlayer->heldTransitionSourceID = -1;
                                    }
                                else if( nextPlayer->holdingID == 0 &&
                                         targetObj->permanent ) {
                                    
                                    // try removing from permanent
                                    // container
                                    removeFromContainerToHold( nextPlayer,
                                                               m.x, m.y,
                                                               m.i );
                                    }         
                                else if( nextPlayer->holdingID > 0 ) {
                                    // try adding what we're holding to
                                    // target container
                                    
                                    transApplied = addHeldToContainer(
                                        nextPlayer, target, m.x, m.y );
                                    }
                                

                                if( targetObj->permanent &&
                                    targetObj->foodValue > 0 ) {
                                    
                                    // just touching this object
                                    // causes us to eat from it

                                    nextPlayer->lastAteID = 
                                        targetObj->id;
                                    nextPlayer->lastAteFillMax =
                                        nextPlayer->foodStore;
                                    
                                    nextPlayer->foodStore += 
                                        targetObj->foodValue;
                                    
                                    logEating( targetObj->id,
                                               targetObj->foodValue,
                                               computeAge( nextPlayer ),
                                               m.x, m.y );
                                    
                                    nextPlayer->foodStore += eatBonus;

                                    int cap =
                                        computeFoodCapacity( nextPlayer );
                                    
                                    if( nextPlayer->foodStore > cap ) {
                                        nextPlayer->foodStore = cap;
                                        }

                                    
                                    // we eat everything BUT what
                                    // we picked from it, if anything
                                    if( oldHolding == 0 && 
                                        nextPlayer->holdingID != 0 ) {
                                        
                                        ObjectRecord *newHeld =
                                            getObject( nextPlayer->holdingID );
                                        
                                        if( newHeld->foodValue > 0 ) {
                                            nextPlayer->foodStore -=
                                                newHeld->foodValue;

                                            if( nextPlayer->lastAteFillMax >
                                                nextPlayer->foodStore ) {
                                                
                                                nextPlayer->foodStore =
                                                    nextPlayer->lastAteFillMax;
                                                }
                                            }
                                        
                                        }
                                    


                                    nextPlayer->foodDecrementETASeconds =
                                        Time::getCurrentTime() +
                                        computeFoodDecrementTimeSeconds( 
                                            nextPlayer );
                                    
                                    nextPlayer->foodUpdate = true;
                                    }
                                

                                if( ! transApplied &&
                                    nextPlayer->holdingID != 0 ) {
                                    // no transition for what we're
                                    // holding on target

                                    // check if surrounded

                                    int nA = 
                                        getMapObject( nextPlayer->xd - 1, 
                                                      nextPlayer->yd );
                                    int nB = 
                                        getMapObject( nextPlayer->xd + 1, 
                                                      nextPlayer->yd );
                                    int nC = 
                                        getMapObject( nextPlayer->xd, 
                                                      nextPlayer->yd - 1 );
                                    int nD = 
                                        getMapObject( nextPlayer->xd, 
                                                      nextPlayer->yd + 1 );
                                    
                                    if( nA != 0 && nB != 0 && 
                                        nC != 0 && nD != 0 
                                        &&
                                        getObject( nA )->blocksWalking &&
                                        getObject( nB )->blocksWalking &&
                                        getObject( nC )->blocksWalking &&
                                        getObject( nD )->blocksWalking ) {
                                        

                                        // surrounded with blocking
                                        // objects while holding
                                    
                                        // throw held into nearest empty spot
                                        
                                        handleDrop( 
                                            m.x, m.y, 
                                            nextPlayer,
                                            &playerIndicesToSendUpdatesAbout );
                                        }
                                    
                                    // action doesn't happen, just the drop
                                    }
                                }
                            else if( nextPlayer->holdingID > 0 ) {
                                // target location emtpy
                                // target not where we're standing
                                // we're holding something
                                
                                char usedOnFloor = false;
                                int floorID = getMapFloor( m.x, m.y );
                                
                                if( floorID > 0 ) {
                                    
                                    TransRecord *r = 
                                        getTrans( nextPlayer->holdingID,
                                                  floorID );
                                
                                    if( r == NULL ) {
                                        logTransitionFailure( 
                                            nextPlayer->holdingID,
                                            floorID );
                                        }
                                        

                                    if( r != NULL && 
                                        // make sure we're not too young
                                        // to hold result of on-floor
                                        // transition
                                        ( r->newActor == 0 ||
                                          getObject( r->newActor )->
                                             minPickupAge <= 
                                          computeAge( nextPlayer ) ) ) {

                                        // applies to floor
                                        int resultID = r->newTarget;
                                        
                                        if( getObject( resultID )->floor ) {
                                            // changing floor to floor
                                            // go ahead
                                            usedOnFloor = true;
                                            
                                            if( resultID != floorID ) {
                                                setMapFloor( m.x, m.y,
                                                             resultID );
                                                }
                                            handleHoldingChange( nextPlayer,
                                                                 r->newActor );
                                            }
                                        else {
                                            // changing floor to non-floor
                                            char canPlace = true;
                                            if( getObject( resultID )->
                                                blocksWalking &&
                                                ! isMapSpotEmpty( m.x, m.y ) ) {
                                                canPlace = false;
                                                }
                                            
                                            if( canPlace ) {
                                                setMapFloor( m.x, m.y, 0 );
                                                
                                                setMapObject( m.x, m.y,
                                                              resultID );
                                                
                                                handleHoldingChange( 
                                                    nextPlayer,
                                                    r->newActor );
                                            
                                                usedOnFloor = true;
                                                }
                                            }
                                        }
                                    }
                                


                                // consider a use-on-bare-ground transtion
                                
                                ObjectRecord *obj = 
                                    getObject( nextPlayer->holdingID );
                                
                                if( ! usedOnFloor && obj->foodValue == 0 ) {
                                    
                                    // get no-target transtion
                                    // (not a food transition, since food
                                    //   value is 0)
                                    TransRecord *r = 
                                        getTrans( nextPlayer->holdingID, 
                                                  -1 );


                                    char canPlace = false;
                                    
                                    if( r != NULL &&
                                        r->newTarget != 0 
                                        && 
                                        // make sure we're not too young
                                        // to hold result of bare ground
                                        // transition
                                        ( r->newActor == 0 ||
                                          getObject( r->newActor )->
                                             minPickupAge <= 
                                          computeAge( nextPlayer ) ) ) {
                                        
                                        canPlace = true;

                                        if( getObject( r->newTarget )->
                                            blocksWalking &&
                                            ! isMapSpotEmpty( m.x, m.y ) ) {
                                            
                                            // can't do on-bare ground
                                            // transition where person 
                                            // standing
                                            // if it creates a blocking 
                                            // object
                                            canPlace = false;
                                            }
                                        }
                                    
                                    if( canPlace ) {

                                        if( nextPlayer->numContained > 0 &&
                                            r->newActor == 0 &&
                                            r->newTarget > 0 &&
                                            getObject( r->newTarget )->numSlots 
                                            >= nextPlayer->numContained &&
                                            getObject( r->newTarget )->slotSize
                                            >= obj->slotSize ) {

                                            // use on bare ground with full
                                            // container that leaves
                                            // hand empty
                                            
                                            // and there's room in newTarget

                                            setResponsiblePlayer( 
                                                - nextPlayer->id );
                                            setMapObject( m.x, m.y, 
                                                          r->newTarget );
                                            setResponsiblePlayer( -1 );
                                    
                                            transferHeldContainedToMap( 
                                                nextPlayer, m.x, m.y );
                                            
                                            handleHoldingChange( nextPlayer,
                                                                 r->newActor );
                                            }
                                        else {
                                            handleHoldingChange( nextPlayer,
                                                                 r->newActor );
                                        
                                            setResponsiblePlayer( 
                                                - nextPlayer->id );
                                            
                                            if( r->newTarget > 0 
                                                && getObject( r->newTarget )->
                                                floor ) {
                                                
                                                setMapFloor( m.x, m.y, 
                                                             r->newTarget );
                                                }
                                            else {    
                                                setMapObject( m.x, m.y, 
                                                              r->newTarget );
                                                }
                                            
                                            setResponsiblePlayer( -1 );
                                            
                                            handleMapChangeToPaths( 
                                             m.x, m.y,
                                             getObject( r->newTarget ),
                                             &playerIndicesToSendUpdatesAbout );
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    else if( m.type == BABY ) {
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        
                        if( computeAge( nextPlayer ) >= minPickupBabyAge 
                            &&
                            ( isGridAdjacent( m.x, m.y,
                                              nextPlayer->xd, 
                                              nextPlayer->yd ) 
                              ||
                              ( m.x == nextPlayer->xd &&
                                m.y == nextPlayer->yd ) ) ) {
                            
                            nextPlayer->actionAttempt = 1;
                            nextPlayer->actionTarget.x = m.x;
                            nextPlayer->actionTarget.y = m.y;
                            
                            if( m.x > nextPlayer->xd ) {
                                nextPlayer->facingOverride = 1;
                                }
                            else if( m.x < nextPlayer->xd ) {
                                nextPlayer->facingOverride = -1;
                                }


                            if( nextPlayer->holdingID == 0 ) {
                                // target location empty and 
                                // and our hands are empty
                                
                                // check if there's a baby to pick up there

                                // is anyone there?
                                LiveObject *hitPlayer = 
                                    getHitPlayer( m.x, m.y, false, 5 );
                                
                                if( hitPlayer != NULL &&
                                    !hitPlayer->heldByOther &&
                                    computeAge( hitPlayer ) < babyAge  ) {
                                    
                                    // negative holding IDs to indicate
                                    // holding another player
                                    nextPlayer->holdingID = -hitPlayer->id;
                                    
                                    nextPlayer->holdingEtaDecay = 0;

                                    hitPlayer->heldByOther = true;
                                    hitPlayer->heldByOtherID = nextPlayer->id;
                                    
                                    // force baby to drop what they are
                                    // holding

                                    if( hitPlayer->holdingID != 0 ) {

                                        if( hitPlayer->holdingWound &&
                                            hitPlayer->embeddedWeaponID > 0 ) {
                                            
                                            // switch from wound to
                                            // holding embedded weapon
                                            // have them drop it when
                                            // picked up
                                            hitPlayer->holdingID = 
                                                hitPlayer->
                                                embeddedWeaponID;
                                            hitPlayer->holdingEtaDecay =
                                                hitPlayer->
                                                embeddedWeaponEtaDecay;
                                            
                                            hitPlayer->embeddedWeaponID = 0;
                                            hitPlayer->embeddedWeaponEtaDecay
                                                = 0;
                                            hitPlayer->holdingWound = false;
                                            }
                                        else if( hitPlayer->holdingWound &&
                                                 hitPlayer->embeddedWeaponID 
                                                 == 0 ) {
                                            // holding wound only
                                            // disappears
                                            hitPlayer->holdingID = 0;
                                            hitPlayer->holdingEtaDecay = 0;
                                            }
                                        

                                        if( hitPlayer->holdingID > 0 ) {
                                            handleDrop( 
                                                m.x, m.y, hitPlayer,
                                             &playerIndicesToSendUpdatesAbout );
                                            }
                                        }
                                    
                                    if( hitPlayer->xd != hitPlayer->xs
                                        ||
                                        hitPlayer->yd != hitPlayer->ys ) {
                                        
                                        // force baby to stop moving
                                        hitPlayer->xd = m.x;
                                        hitPlayer->yd = m.y;
                                        hitPlayer->xs = m.x;
                                        hitPlayer->ys = m.y;
                                        
                                        // but don't send an update
                                        // about this
                                        // (everyone will get the pick-up
                                        //  update for the holding adult)
                                        }
                                    
                                    // if adult fertile female, baby auto-fed
                                    if( isFertileAge( nextPlayer ) ) {
                                        
                                        hitPlayer->foodStore = 
                                            computeFoodCapacity( hitPlayer );
                
                                        hitPlayer->foodUpdate = true;
                                        hitPlayer->responsiblePlayerID =
                                            nextPlayer->id;
                                        
                                        // reset their food decrement time
                                        hitPlayer->foodDecrementETASeconds =
                                            Time::getCurrentTime() +
                                            computeFoodDecrementTimeSeconds( 
                                                hitPlayer );

                                        // fixed cost to pick up baby
                                        // this still encourages baby-parent
                                        // communication so as not
                                        // to get the most mileage out of 
                                        // food
                                        int nurseCost = 1;
                                        
                                        nextPlayer->foodStore -= nurseCost;
                                        
                                        if( nextPlayer->foodStore < 0 ) {
                                            // catch mother death later
                                            // at her next food decrement
                                            nextPlayer->foodStore = 0;
                                            }
                                        // leave their food decrement
                                        // time alone
                                        nextPlayer->foodUpdate = true;
                                        }
                                    
                                    nextPlayer->heldOriginValid = 1;
                                    nextPlayer->heldOriginX = m.x;
                                    nextPlayer->heldOriginY = m.y;
                                    nextPlayer->heldTransitionSourceID = -1;
                                    }
                                
                                }
                            }
                        }
                    else if( m.type == SELF || m.type == UBABY ) {
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        
                        char holdingFood = false;
                        char holdingDrugs = false;
                        
                        if( nextPlayer->holdingID > 0 ) {
                            ObjectRecord *obj = 
                                getObject( nextPlayer->holdingID );
                            
                            if( obj->foodValue > 0 ) {
                                holdingFood = true;

                                if( strstr( obj->description, "remapStart" )
                                    != NULL ) {
                                    // don't count drugs as food to 
                                    // feed other people
                                    holdingFood = false;
                                    holdingDrugs = true;
                                    }
                                }
                            }
                        
                        LiveObject *targetPlayer = NULL;
                        
                        if( nextPlayer->holdingID < 0 ) {
                            // holding a baby
                            // don't allow this action through
                            // keep targetPlayer NULL
                            }
                        else if( m.type == SELF ) {
                            if( m.x == nextPlayer->xd &&
                                m.y == nextPlayer->yd ) {
                                
                                // use on self
                                targetPlayer = nextPlayer;
                                }
                            }
                        else if( m.type == UBABY ) {
                            
                            if( isGridAdjacent( m.x, m.y,
                                                nextPlayer->xd, 
                                                nextPlayer->yd ) ||
                                ( m.x == nextPlayer->xd &&
                                  m.y == nextPlayer->yd ) ) {
                                

                                if( m.x > nextPlayer->xd ) {
                                    nextPlayer->facingOverride = 1;
                                    }
                                else if( m.x < nextPlayer->xd ) {
                                    nextPlayer->facingOverride = -1;
                                    }
                                
                                // try click on baby
                                int hitIndex;
                                LiveObject *hitPlayer = 
                                    getHitPlayer( m.x, m.y, 
                                                  false, 5, -1, &hitIndex );
                                
                                if( hitPlayer != NULL && holdingDrugs ) {
                                    // can't even feed baby drugs
                                    // too confusing
                                    hitPlayer = NULL;
                                    }

                                if( hitPlayer == NULL ||
                                    hitPlayer == nextPlayer ) {
                                    // try click on elderly
                                    hitPlayer = 
                                        getHitPlayer( m.x, m.y, false, -1, 
                                                      55, &hitIndex );
                                    }
                                
                                if( ( hitPlayer == NULL ||
                                      hitPlayer == nextPlayer )
                                    &&
                                    holdingFood ) {
                                    
                                    // feeding action 
                                    // try click on everyone
                                    hitPlayer = 
                                        getHitPlayer( m.x, m.y, false, -1, -1, 
                                                      &hitIndex );
                                    }
                                

                                if( hitPlayer != NULL &&
                                    hitPlayer != nextPlayer ) {
                                    
                                    targetPlayer = hitPlayer;
                                    
                                    playerIndicesToSendUpdatesAbout.push_back( 
                                        hitIndex );
                                    
                                    targetPlayer->responsiblePlayerID =
                                            nextPlayer->id;
                                    }
                                }
                            }
                        

                        if( targetPlayer != NULL ) {
                            
                            // use on self/baby
                            nextPlayer->actionAttempt = 1;
                            nextPlayer->actionTarget.x = m.x;
                            nextPlayer->actionTarget.y = m.y;
                            
                            if( nextPlayer->holdingID > 0 ) {
                                ObjectRecord *obj = 
                                    getObject( nextPlayer->holdingID );
                                
                                int cap = computeFoodCapacity( targetPlayer );
                                
                                if( obj->foodValue > 0 && 
                                    targetPlayer->foodStore < cap ) {
                                    
                                    targetPlayer->justAte = true;
                                    targetPlayer->justAteID = 
                                        nextPlayer->holdingID;

                                    targetPlayer->lastAteID = 
                                        nextPlayer->holdingID;
                                    targetPlayer->lastAteFillMax =
                                        targetPlayer->foodStore;
                                    
                                    targetPlayer->foodStore += obj->foodValue;
                                    
                                    logEating( obj->id,
                                               obj->foodValue,
                                               computeAge( targetPlayer ),
                                               m.x, m.y );
                                    
                                    targetPlayer->foodStore += eatBonus;

                                    
                                    if( targetPlayer->foodStore > cap ) {
                                        targetPlayer->foodStore = cap;
                                        }
                                    targetPlayer->foodDecrementETASeconds =
                                        Time::getCurrentTime() +
                                        computeFoodDecrementTimeSeconds( 
                                            targetPlayer );
                                    
                                    // get eat transtion
                                    TransRecord *r = 
                                        getTrans( nextPlayer->holdingID, 
                                                  -1 );

                                    

                                    if( r != NULL ) {
                                        int oldHolding = nextPlayer->holdingID;
                                        nextPlayer->holdingID = r->newActor;
                                        
                                        if( oldHolding !=
                                            nextPlayer->holdingID ) {
                                            
                                            setFreshEtaDecayForHeld( 
                                                nextPlayer );
                                            }
                                        }
                                    else {
                                        // default, holding nothing after eating
                                        nextPlayer->holdingID = 0;
                                        nextPlayer->holdingEtaDecay = 0;
                                        }
                                    
                                    nextPlayer->heldOriginValid = 0;
                                    nextPlayer->heldOriginX = 0;
                                    nextPlayer->heldOriginY = 0;
                                    nextPlayer->heldTransitionSourceID = -1;
                                    
                                    targetPlayer->foodUpdate = true;
                                    }
                                else if( obj->clothing != 'n' ) {
                                    // wearable
                                    
                                    nextPlayer->holdingID = 0;
                                    timeSec_t oldEtaDecay = 
                                        nextPlayer->holdingEtaDecay;
                                    
                                    nextPlayer->holdingEtaDecay = 0;
                                    
                                    nextPlayer->heldOriginValid = 0;
                                    nextPlayer->heldOriginX = 0;
                                    nextPlayer->heldOriginY = 0;
                                    nextPlayer->heldTransitionSourceID = -1;
                                    
                                    ObjectRecord *oldC = NULL;
                                    timeSec_t oldCEtaDecay = 0;
                                    int oldNumContained = 0;
                                    int *oldContainedIDs = NULL;
                                    timeSec_t *oldContainedETADecays = NULL;
                                    

                                    ObjectRecord **clothingSlot = NULL;
                                    int clothingSlotIndex;

                                    switch( obj->clothing ) {
                                        case 'h':
                                            clothingSlot = 
                                                &( targetPlayer->clothing.hat );
                                            clothingSlotIndex = 0;
                                            break;
                                        case 't':
                                            clothingSlot = 
                                              &( targetPlayer->clothing.tunic );
                                            clothingSlotIndex = 1;
                                            break;
                                        case 'b':
                                            clothingSlot = 
                                                &( targetPlayer->
                                                   clothing.bottom );
                                            clothingSlotIndex = 4;
                                            break;
                                        case 'p':
                                            clothingSlot = 
                                                &( targetPlayer->
                                                   clothing.backpack );
                                            clothingSlotIndex = 5;
                                            break;
                                        case 's':
                                            if( targetPlayer->clothing.backShoe
                                                == NULL ) {

                                                clothingSlot = 
                                                    &( targetPlayer->
                                                       clothing.backShoe );
                                                clothingSlotIndex = 3;

                                                }
                                            else if( 
                                                targetPlayer->clothing.frontShoe
                                                == NULL ) {
                                                
                                                clothingSlot = 
                                                    &( targetPlayer->
                                                       clothing.frontShoe );
                                                clothingSlotIndex = 2;
                                                }
                                            else {
                                                // replace front shoe

                                                clothingSlot = 
                                                    &( targetPlayer->
                                                       clothing.frontShoe );
                                                clothingSlotIndex = 2;
                                                }
                                            break;
                                        }
                                    
                                    if( clothingSlot != NULL ) {
                                        
                                        oldC = *clothingSlot;
                                        int ind = clothingSlotIndex;
                                        
                                        oldCEtaDecay = 
                                            targetPlayer->clothingEtaDecay[ind];
                                        
                                        oldNumContained = 
                                            targetPlayer->
                                            clothingContained[ind].size();
                                        
                                        if( oldNumContained > 0 ) {
                                            oldContainedIDs = 
                                                targetPlayer->
                                                clothingContained[ind].
                                                getElementArray();
                                            oldContainedETADecays =
                                                targetPlayer->
                                                clothingContainedEtaDecays[ind].
                                                getElementArray();
                                            }
                                        
                                        *clothingSlot = obj;
                                        targetPlayer->clothingEtaDecay[ind] =
                                            oldEtaDecay;
                                        
                                        targetPlayer->
                                            clothingContained[ind].
                                            deleteAll();
                                        targetPlayer->
                                            clothingContainedEtaDecays[ind].
                                            deleteAll();
                                            
                                        if( nextPlayer->numContained > 0 ) {
                                            
                                            targetPlayer->clothingContained[ind]
                                                .appendArray( 
                                                    nextPlayer->containedIDs,
                                                    nextPlayer->numContained );

                                            targetPlayer->
                                                clothingContainedEtaDecays[ind]
                                                .appendArray( 
                                                    nextPlayer->
                                                    containedEtaDecays,
                                                    nextPlayer->numContained );
                                                

                                            // ignore sub-contained
                                            // because clothing can
                                            // never contain containers
                                            clearPlayerHeldContained( 
                                                nextPlayer );
                                            }
                                            
                                        
                                        if( oldC != NULL ) {
                                            nextPlayer->holdingID =
                                                oldC->id;
                                            nextPlayer->holdingEtaDecay
                                                = oldCEtaDecay;
                                            
                                            nextPlayer->numContained =
                                                oldNumContained;
                                            nextPlayer->containedIDs =
                                                oldContainedIDs;
                                            nextPlayer->containedEtaDecays =
                                                oldContainedETADecays;
                                            
                                            // empty sub-contained vectors
                                            // because clothing never
                                            // never contains containers
                                            nextPlayer->subContainedIDs
                                                = new SimpleVector<int>[
                                                    nextPlayer->numContained ];
                                            nextPlayer->subContainedEtaDecays
                                                = new SimpleVector<timeSec_t>[
                                                    nextPlayer->numContained ];
                                            }
                                        }
                                    }
                                else if( targetPlayer == nextPlayer &&
                                         m.i >= 0 && 
                                         m.i < NUM_CLOTHING_PIECES ) {
                                    // not wearable or food
                                    // try dropping what we're holding
                                    // into clothing
                                    addHeldToClothingContainer( nextPlayer,
                                                                m.i );
                                    }
                                }         
                            else {
                                // empty hand on self/baby, remove clothing

                                ObjectRecord **clothingSlot = NULL;
                                int clothingSlotIndex;
                                

                                if( m.i == 2 &&
                                    targetPlayer->clothing.frontShoe != NULL ) {
                                    clothingSlot = 
                                        &( targetPlayer->clothing.frontShoe );
                                    clothingSlotIndex = 2;
                                    }
                                else if( m.i == 3 &&
                                         targetPlayer->clothing.backShoe 
                                         != NULL ) {
                                    clothingSlot = 
                                        &( targetPlayer->clothing.backShoe );
                                    clothingSlotIndex = 3;
                                    }
                                else if( m.i == 0 && 
                                         targetPlayer->clothing.hat != NULL ) {
                                    clothingSlot = 
                                        &( targetPlayer->clothing.hat );
                                    clothingSlotIndex = 0;
                                    }
                                else if( m.i == 1 &&
                                         targetPlayer->clothing.tunic 
                                         != NULL ) {
                                    clothingSlot = 
                                        &( targetPlayer->clothing.tunic );
                                    clothingSlotIndex = 1;
                                    }
                                else if( m.i == 4 &&
                                         targetPlayer->clothing.bottom 
                                         != NULL ) {
                                    clothingSlot = 
                                        &( targetPlayer->clothing.bottom );
                                    clothingSlotIndex = 4;
                                    }
                                else if( m.i == 5 &&
                                         targetPlayer->
                                         clothing.backpack != NULL ) {
                                    
                                    clothingSlot = 
                                        &( targetPlayer->clothing.backpack );
                                    clothingSlotIndex = 5;
                                    }

                                if( clothingSlot != NULL ) {
                                    
                                    int ind = clothingSlotIndex;
                                    
                                    nextPlayer->holdingID =
                                        ( *clothingSlot )->id;
                                    *clothingSlot = NULL;
                                    nextPlayer->holdingEtaDecay =
                                        targetPlayer->clothingEtaDecay[ind];
                                    targetPlayer->clothingEtaDecay[ind] = 0;
                                    
                                    nextPlayer->numContained =
                                        targetPlayer->
                                        clothingContained[ind].size();
                                    
                                    nextPlayer->containedIDs =
                                        targetPlayer->
                                        clothingContained[ind].
                                        getElementArray();
                                    
                                    targetPlayer->clothingContained[ind].
                                        deleteAll();
                                    
                                    nextPlayer->containedEtaDecays =
                                        targetPlayer->
                                        clothingContainedEtaDecays[ind].
                                        getElementArray();
                                    
                                    targetPlayer->
                                        clothingContainedEtaDecays[ind].
                                        deleteAll();
                                    
                                    // empty sub contained in clothing
                                    nextPlayer->subContainedIDs =
                                        new SimpleVector<int>[
                                            nextPlayer->numContained ];
                                    
                                    nextPlayer->subContainedEtaDecays =
                                        new SimpleVector<timeSec_t>[
                                            nextPlayer->numContained ];
                                    

                                    nextPlayer->heldOriginValid = 0;
                                    nextPlayer->heldOriginX = 0;
                                    nextPlayer->heldOriginY = 0;
                                    }
                                }
                            }
                        }                    
                    else if( m.type == DROP ) {
                        //Thread::staticSleep( 2000 );
                        
                        // send update even if action fails (to let them
                        // know that action is over)
                        playerIndicesToSendUpdatesAbout.push_back( i );

                        char canDrop = true;
                        
                        if( nextPlayer->holdingID > 0 &&
                            getObject( nextPlayer->holdingID )->permanent ) {
                            canDrop = false;
                            }

                        if( canDrop 
                            &&
                            ( isGridAdjacent( m.x, m.y,
                                              nextPlayer->xd, 
                                              nextPlayer->yd ) 
                              ||
                              ( m.x == nextPlayer->xd &&
                                m.y == nextPlayer->yd )  ) ) {
                            
                            nextPlayer->actionAttempt = 1;
                            nextPlayer->actionTarget.x = m.x;
                            nextPlayer->actionTarget.y = m.y;
                            
                            if( m.x > nextPlayer->xd ) {
                                nextPlayer->facingOverride = 1;
                                }
                            else if( m.x < nextPlayer->xd ) {
                                nextPlayer->facingOverride = -1;
                                }

                            if( nextPlayer->holdingID != 0 ) {
                                
                                if( nextPlayer->holdingID < 0 ) {
                                    // baby drop
                                    int target = getMapObject( m.x, m.y );
                                    
                                    if( target == 0 // nothing here
                                        ||
                                        ! getObject( target )->
                                            blocksWalking ) {
                                        handleDrop( 
                                            m.x, m.y, nextPlayer,
                                            &playerIndicesToSendUpdatesAbout );
                                        }    
                                    }
                                else if( isMapSpotEmpty( m.x, m.y ) ) {
                                
                                    // empty spot to drop non-baby into
                                    
                                    handleDrop( 
                                        m.x, m.y, nextPlayer,
                                        &playerIndicesToSendUpdatesAbout );
                                    }
                                else if( m.c >= 0 && 
                                         m.c < NUM_CLOTHING_PIECES &&
                                         m.x == nextPlayer->xd &&
                                         m.y == nextPlayer->yd  &&
                                         nextPlayer->holdingID > 0 ) {
                                    
                                    addHeldToClothingContainer( nextPlayer,
                                                                m.c );
                                    }
                                else if( nextPlayer->holdingID > 0 ) {
                                    // non-baby drop
                                    
                                    ObjectRecord *droppedObj
                                        = getObject( 
                                            nextPlayer->holdingID );
                                    
                                    int target = getMapObject( m.x, m.y );
                            
                                    if( target != 0 ) {
                                        
                                        ObjectRecord *targetObj =
                                            getObject( target );
                                        
                                        int targetSlots =
                                            targetObj->numSlots;
                                        
                                        int targetSlotSize = 0;
                                        
                                        if( targetSlots > 0 ) {
                                            targetSlotSize =
                                                targetObj->slotSize;
                                            }
                                        
                                        char canGoIn = false;
                                        
                                        if( droppedObj->containable &&
                                            targetSlotSize >=
                                            droppedObj->containSize ) {
                                            canGoIn = true;
                                            }
                                        
                                        

                                        // DROP indicates they 
                                        // right-clicked on container
                                        // so use swap mode
                                        if( canGoIn && 
                                            addHeldToContainer( 
                                                nextPlayer,
                                                target,
                                                m.x, m.y, true ) ) {
                                            // handled
                                            }
                                        else if( ! canGoIn &&
                                                 ! targetObj->permanent 
                                                 &&
                                                 targetObj->minPickupAge <=
                                                 computeAge( nextPlayer ) ) {
                                            // drop onto a spot where
                                            // something exists, and it's
                                            // not a container

                                            // swap what we're holding for
                                            // target
                                            
                                            timeSec_t newHoldingEtaDecay = 
                                                getEtaDecay( m.x, m.y );

                                            FullMapContained f = 
                                                getFullMapContained( m.x, m.y );
                                            

                                            clearAllContained( m.x, m.y );
                                            setMapObject( m.x, m.y, 0 );
                                    
                                            handleDrop(
                                             m.x, m.y, nextPlayer,
                                             &playerIndicesToSendUpdatesAbout );
                                    
                                            
                                            nextPlayer->holdingID = target;
                                            nextPlayer->holdingEtaDecay =
                                                newHoldingEtaDecay;

                                            setContained( nextPlayer, f );
                                            
                                    
                                            nextPlayer->heldOriginValid = 1;
                                            nextPlayer->heldOriginX = m.x;
                                            nextPlayer->heldOriginY = m.y;
                                            nextPlayer->heldTransitionSourceID =
                                                -1;
                                            }
                                        }
                                    else {
                                        // no object here
                                        
                                        // maybe there's a person
                                        // standing here

                                        // only allow drop if what we're
                                        // dropping is non-blocking
                                        
                                        
                                        if( ! droppedObj->blocksWalking ) {
                                            
                                             handleDrop( 
                                              m.x, m.y, nextPlayer,
                                              &playerIndicesToSendUpdatesAbout 
                                              );
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    else if( m.type == REMV ) {
                        // send update even if action fails (to let them
                        // know that action is over)
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        
                        removeFromContainerToHold( nextPlayer,
                                                   m.x, m.y, m.i );
                        }                        
                    else if( m.type == SREMV ) {
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        
                        // remove contained object from clothing
                        
                        if( m.x == nextPlayer->xd &&
                            m.y == nextPlayer->yd &&
                            nextPlayer->holdingID == 0 ) {
                            
                            nextPlayer->actionAttempt = 1;
                            nextPlayer->actionTarget.x = m.x;
                            nextPlayer->actionTarget.y = m.y;
                            
                            if( m.c >= 0 && m.c < NUM_CLOTHING_PIECES ) {

                                ObjectRecord *cObj = 
                                    clothingByIndex( nextPlayer->clothing, 
                                                     m.c );
                                
                                float stretch = 1.0f;
                                
                                if( cObj != NULL ) {
                                    stretch = cObj->slotTimeStretch;
                                    }

                                int oldNumContained = 
                                    nextPlayer->clothingContained[m.c].size();

                                int slotToRemove = m.i;
                                
                                if( slotToRemove < 0 ) {
                                    slotToRemove = oldNumContained - 1;
                                    }
                                
                                int toRemoveID = -1;
                                
                                if( oldNumContained > 0 &&
                                    oldNumContained > slotToRemove &&
                                    slotToRemove >= 0 ) {
                                    
                                    toRemoveID = 
                                        nextPlayer->clothingContained[m.c].
                                        getElementDirect( slotToRemove );
                                    }

                                if( oldNumContained > 0 &&
                                    oldNumContained > slotToRemove &&
                                    slotToRemove >= 0 &&
                                    // old enough to handle it
                                    getObject( toRemoveID )->minPickupAge <= 
                                    computeAge( nextPlayer ) ) {
                                    

                                    nextPlayer->holdingID = 
                                        nextPlayer->clothingContained[m.c].
                                        getElementDirect( slotToRemove );

                                    nextPlayer->holdingEtaDecay = 
                                        nextPlayer->
                                        clothingContainedEtaDecays[m.c].
                                        getElementDirect( slotToRemove );
                                    
                                    timeSec_t curTime = Time::timeSec();

                                    if( nextPlayer->holdingEtaDecay != 0 ) {
                                        
                                        timeSec_t offset = 
                                            nextPlayer->holdingEtaDecay
                                            - curTime;
                                        offset = offset * stretch;
                                        nextPlayer->holdingEtaDecay =
                                            curTime + offset;
                                        }

                                    nextPlayer->clothingContained[m.c].
                                        deleteElement( slotToRemove );
                                    nextPlayer->clothingContainedEtaDecays[m.c].
                                        deleteElement( slotToRemove );

                                    nextPlayer->heldOriginValid = 0;
                                    nextPlayer->heldOriginX = 0;
                                    nextPlayer->heldOriginY = 0;
                                    nextPlayer->heldTransitionSourceID = -1;
                                    }
                                }
                            }
                        }
                    
                    if( m.numExtraPos > 0 ) {
                        delete [] m.extraPos;
                        }

                    if( m.saidText != NULL ) {
                        delete [] m.saidText;
                        }
                    if( m.bugText != NULL ) {
                        delete [] m.bugText;
                        }
                    }
                }
            }



        // now that messages have been processed for all
        // loop over and handle all post-message checks

        // for example, if a player later in the list sends a message
        // killing an earlier player, we need to check to see that
        // player deleted afterward here
        for( int i=0; i<numLive; i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            
            double curTime = Time::getCurrentTime();
            
            if( nextPlayer->dying && ! nextPlayer->error &&
                curTime >= nextPlayer->dyingETA ) {
                // finally died
                nextPlayer->error = true;
                }
            

                
            if( nextPlayer->isNew ) {
                // their first position is an update
                

                playerIndicesToSendUpdatesAbout.push_back( i );
                playerIndicesToSendLineageAbout.push_back( i );
                
                nextPlayer->isNew = false;
                
                // force this PU to be sent to everyone
                ChangePosition p = { 0, 0, true };
                newUpdatesPos.push_back( p );
                }
            else if( nextPlayer->error && ! nextPlayer->deleteSent ) {
                
                if( nextPlayer->heldByOther ) {
                    
                    handleForcedBabyDrop( nextPlayer,
                                          &playerIndicesToSendUpdatesAbout );
                    }                
                else if( nextPlayer->holdingID < 0 ) {
                    LiveObject *babyO = 
                        getLiveObject( - nextPlayer->holdingID );
                    
                    handleForcedBabyDrop( babyO,
                                          &playerIndicesToSendUpdatesAbout );
                    }
                
                char *updateLine = getUpdateLine( nextPlayer, true );

                newDeleteUpdates.appendElementString( updateLine );
                
                delete [] updateLine;
                
                nextPlayer->isNew = false;
                
                nextPlayer->deleteSent = true;
                // wait 5 seconds before closing their connection
                // so they can get the message
                nextPlayer->deleteSentDoneETA = Time::getCurrentTime() + 5;
                
                if( areTriggersEnabled() ) {
                    // add extra time so that rest of triggers can be received
                    // and rest of trigger results can be sent
                    // back to this client
                    
                    // another hour...
                    nextPlayer->deleteSentDoneETA += 3600;
                    // and don't set their error flag after all
                    // keep receiving triggers from them

                    nextPlayer->error = false;
                    }
                else {
                    // stop listening for activity on this socket
                    sockPoll.removeSocket( nextPlayer->sock );
                    }
                

                GridPos dropPos;
                
                if( nextPlayer->xd == 
                    nextPlayer->xs &&
                    nextPlayer->yd ==
                    nextPlayer->ys ) {
                    // deleted player standing still
                    
                    dropPos.x = nextPlayer->xd;
                    dropPos.y = nextPlayer->yd;
                    }
                else {
                    // player moving
                    
                    dropPos = 
                        computePartialMoveSpot( nextPlayer );
                    }

                if( ! nextPlayer->deathLogged ) {
                    double age = computeAge( nextPlayer );
                    
                    char disconnect = true;
                    
                    if( age >= forceDeathAge ) {
                        disconnect = false;
                        }
                    
                    logDeath( nextPlayer->id,
                              nextPlayer->email,
                              nextPlayer->isEve,
                              computeAge( nextPlayer ),
                              getSecondsPlayed( nextPlayer ),
                              ! getFemale( nextPlayer ),
                              dropPos.x, dropPos.y,
                              players.size() - 1,
                              disconnect );
                    
                    if( shutdownMode ) {
                        handleShutdownDeath( nextPlayer, dropPos.x, dropPos.y );
                        }
                    
                    nextPlayer->deathLogged = true;
                    }
                
                // now that death has been logged, and delete sent,
                // we can clear their email address so that the 
                // can log in again during the deleteSentDoneETA window
                
                if( nextPlayer->email != NULL ) {
                    delete [] nextPlayer->email;
                    }
                nextPlayer->email = stringDuplicate( "email_cleared" );


                int oldObject = getMapObject( dropPos.x, dropPos.y );
                
                SimpleVector<int> oldContained;
                SimpleVector<timeSec_t> oldContainedETADecay;
                
                int nX[4] = { -1, 1,  0, 0 };
                int nY[4] = {  0, 0, -1, 1 };
                
                int n = 0;
                GridPos centerDropPos = dropPos;
                
                while( oldObject != 0 && n <= 4 ) {
                    
                    if( isGrave( oldObject ) ) {
                        int numContained;
                        
                        int *contained = getContained( dropPos.x, dropPos.y, 
                                                       &numContained );
                        
                        timeSec_t *containedETA =
                            getContainedEtaDecay( dropPos.x, dropPos.y, 
                                                  &numContained );
                        
                        if( numContained > 0 ) {
                            oldContained.appendArray( contained, numContained );
                            
                            oldContainedETADecay.appendArray(
                                containedETA, numContained );

                            delete [] contained;
                            delete [] containedETA;
                            }
                        setMapObject( dropPos.x, dropPos.y,  0 );
                        clearAllContained( dropPos.x, dropPos.y );
                        oldObject = 0;
                        }
                    else {
                        ObjectRecord *r = getObject( oldObject );
                        
                        if( r->numSlots == 0 && ! r->permanent 
                            && ! r->rideable ) {
                            
                            oldContained.push_back( oldObject );
                            oldContainedETADecay.push_back(
                                getEtaDecay( dropPos.x, dropPos.y ) );
                            
                            setMapObject( dropPos.x, dropPos.y, 0 );
                            oldObject = 0;
                            }
                        }

                    oldObject = getMapObject( dropPos.x, dropPos.y );
                    
                    if( oldObject != 0 ) {
                        
                        // try next neighbor
                        dropPos.x = centerDropPos.x + nX[n];
                        dropPos.y = centerDropPos.y + nY[n];
                        
                        n++;
                        oldObject = getMapObject( dropPos.x, dropPos.y );
                        }
                    }


                // assume death markes non-blocking, so it's safe
                // to drop one even if other players standing here
                if( isMapSpotEmpty( dropPos.x, dropPos.y, false ) ) {
                    int deathID = getRandomDeathMarker();
                    
                    if( nextPlayer->customGraveID > 0 ) {
                        deathID = nextPlayer->customGraveID;
                        }

                    if( deathID > 0 ) {
                        
                        setResponsiblePlayer( - nextPlayer->id );
                        setMapObject( dropPos.x, dropPos.y, 
                                      deathID );
                        setResponsiblePlayer( -1 );
                        
                        ObjectRecord *deathObject = getObject( deathID );
                        
                        int roomLeft = deathObject->numSlots;
                        
                        if( roomLeft >= 1 ) {
                            // room for weapon remnant
                            if( nextPlayer->embeddedWeaponID != 0 ) {
                                addContained( 
                                    dropPos.x, dropPos.y,
                                    nextPlayer->embeddedWeaponID,
                                    nextPlayer->embeddedWeaponEtaDecay );
                                roomLeft--;
                                }
                            }
                        
                            
                        if( roomLeft >= 5 ) {
                            // room for clothing
                            
                            if( nextPlayer->clothing.tunic != NULL ) {
                                
                                addContained( 
                                    dropPos.x, dropPos.y,
                                    nextPlayer->clothing.tunic->id,
                                    nextPlayer->clothingEtaDecay[1] );
                                roomLeft--;
                                }
                            if( nextPlayer->clothing.bottom != NULL ) {
                                
                                addContained( 
                                    dropPos.x, dropPos.y,
                                    nextPlayer->clothing.bottom->id,
                                    nextPlayer->clothingEtaDecay[4] );
                                roomLeft--;
                                }
                            if( nextPlayer->clothing.backpack != NULL ) {
                                
                                addContained( 
                                    dropPos.x, dropPos.y,
                                    nextPlayer->clothing.backpack->id,
                                    nextPlayer->clothingEtaDecay[5] );
                                roomLeft--;
                                }
                            if( nextPlayer->clothing.backShoe != NULL ) {
                                
                                addContained( 
                                    dropPos.x, dropPos.y,
                                    nextPlayer->clothing.backShoe->id,
                                    nextPlayer->clothingEtaDecay[3] );
                                roomLeft--;
                                }
                            if( nextPlayer->clothing.frontShoe != NULL ) {
                                
                                addContained( 
                                    dropPos.x, dropPos.y,
                                    nextPlayer->clothing.frontShoe->id,
                                    nextPlayer->clothingEtaDecay[2] );
                                roomLeft--;
                                }
                            if( nextPlayer->clothing.hat != NULL ) {
                                
                                addContained( dropPos.x, dropPos.y,
                                              nextPlayer->clothing.hat->id,
                                              nextPlayer->clothingEtaDecay[0] );
                                roomLeft--;
                                }
                            }
                        
                        // room for what clothing contained
                        timeSec_t curTime = Time::timeSec();
                        
                        for( int c=0; c < NUM_CLOTHING_PIECES && roomLeft > 0; 
                             c++ ) {
                            
                            float oldStretch = 1.0;
                            
                            ObjectRecord *cObj = clothingByIndex( 
                                nextPlayer->clothing, c );
                            
                            if( cObj != NULL ) {
                                oldStretch = cObj->slotTimeStretch;
                                }
                            
                            float newStretch = deathObject->slotTimeStretch;
                            
                            for( int cc=0; 
                                 cc < nextPlayer->clothingContained[c].size() 
                                     &&
                                     roomLeft > 0;
                                 cc++ ) {
                                
                                if( nextPlayer->
                                    clothingContainedEtaDecays[c].
                                    getElementDirect( cc ) != 0 &&
                                    oldStretch != newStretch ) {
                                        
                                    timeSec_t offset = 
                                        nextPlayer->
                                        clothingContainedEtaDecays[c].
                                        getElementDirect( cc ) - 
                                        curTime;
                                        
                                    offset = offset * oldStretch;
                                    offset = offset / newStretch;
                                        
                                    *( nextPlayer->
                                       clothingContainedEtaDecays[c].
                                       getElement( cc ) ) =
                                        curTime + offset;
                                    }

                                addContained( 
                                    dropPos.x, dropPos.y,
                                    nextPlayer->
                                    clothingContained[c].
                                    getElementDirect( cc ),
                                    nextPlayer->
                                    clothingContainedEtaDecays[c].
                                    getElementDirect( cc ) );
                                roomLeft --;
                                }
                            }
                        
                        int oc = 0;
                        
                        while( oc < oldContained.size() && roomLeft > 0 ) {
                            addContained( 
                                dropPos.x, dropPos.y,
                                oldContained.getElementDirect( oc ),
                                oldContainedETADecay.getElementDirect( oc ) );
                            oc++;
                            roomLeft--;                                
                            }
                        }  
                    }
                if( nextPlayer->holdingID != 0 ) {

                    char doNotDrop = false;
                    
                    if( nextPlayer->murderSourceID > 0 ) {
                        
                        TransRecord *woundHit = 
                            getTrans( nextPlayer->murderSourceID, 
                                      0, true, false );
                        
                        if( woundHit != NULL &&
                            woundHit->newTarget > 0 ) {
                            
                            if( nextPlayer->holdingID == woundHit->newTarget ) {
                                // they are simply holding their wound object
                                // don't drop this on the ground
                                doNotDrop = true;
                                }
                            }
                        }
                    
                    if( ! doNotDrop ) {
                        // drop what they were holding

                        // this will almost always involve a throw
                        // (death marker, at least, will be in the way)
                        handleDrop( 
                            dropPos.x, dropPos.y, 
                            nextPlayer,
                            &playerIndicesToSendUpdatesAbout );
                        }
                    else {
                        // just clear what they were holding
                        nextPlayer->holdingID = 0;
                        }
                    }
                }
            else if( ! nextPlayer->error ) {
                // other update checks for living players
                
                if( nextPlayer->holdingEtaDecay != 0 &&
                    nextPlayer->holdingEtaDecay < curTime ) {
                
                    // what they're holding has decayed

                    int oldID = nextPlayer->holdingID;
                
                    TransRecord *t = getTrans( -1, oldID );

                    if( t != NULL ) {

                        int newID = t->newTarget;

                        nextPlayer->holdingID = newID;
                        nextPlayer->heldTransitionSourceID = -1;
                    
                        int oldSlots = 
                            getNumContainerSlots( oldID );

                        int newSlots = getNumContainerSlots( newID );
                    
                        if( newSlots < oldSlots ) {
                            // new container can hold less
                            // truncate
                            
                            GridPos dropPos = 
                                computePartialMoveSpot( nextPlayer );
                            
                            // offset to counter-act offsets built into
                            // drop code
                            dropPos.x += 1;
                            dropPos.y += 1;

                            char found = false;
                            GridPos spot;
                            
                            if( getMapObject( dropPos.x, dropPos.y ) == 0 ) {
                                spot = dropPos;
                                found = true;
                                }
                            else {
                                found = findDropSpot( 
                                    dropPos.x, dropPos.y,
                                    dropPos.x, dropPos.y,
                                    &spot );
                                }
                            
                            
                            if( found ) {
                                
                                // throw it on map temporarily
                                handleDrop( 
                                    spot.x, spot.y, 
                                    nextPlayer,
                                    &playerIndicesToSendUpdatesAbout );
                                
                                
                                // shrink contianer on map
                                shrinkContainer( spot.x, spot.y, 
                                                 newSlots );

                                // pick it back up
                                nextPlayer->holdingEtaDecay = 
                                    getEtaDecay( spot.x, spot.y );
                                    
                                FullMapContained f =
                                    getFullMapContained( spot.x, spot.y );

                                nextPlayer->holdingID = newID;
                                setContained( nextPlayer, f );
                                
                                clearAllContained( spot.x, spot.y );
                                setMapObject( spot.x, spot.y, 0 );
                                }
                            
                            nextPlayer->numContained = newSlots;
                            }
                    
                        setFreshEtaDecayForHeld( nextPlayer );
                    
                        if( nextPlayer->numContained > 0 ) {    
                            restretchDecays( nextPlayer->numContained,
                                             nextPlayer->containedEtaDecays,
                                             oldID, newID );
                            }
                    
                        
                        ObjectRecord *newObj = getObject( newID );
                        ObjectRecord *oldObj = getObject( oldID );
                        
                        
                        if( newObj != NULL && newObj->permanent &&
                            oldObj != NULL && ! oldObj->permanent ) {
                            // object decayed into a permanent
                            // force drop
                             GridPos dropPos = 
                                computePartialMoveSpot( nextPlayer );
                            
                             handleDrop( 
                                    dropPos.x, dropPos.y, 
                                    nextPlayer,
                                    &playerIndicesToSendUpdatesAbout );
                            }
                        

                        playerIndicesToSendUpdatesAbout.push_back( i );
                        }
                    else {
                        // no decay transition exists
                        // clear it
                        setFreshEtaDecayForHeld( nextPlayer );
                        }
                    }

                // check if anything in the container they are holding
                // has decayed
                if( nextPlayer->holdingID > 0 &&
                    nextPlayer->numContained > 0 ) {
                    
                    char change = false;
                    
                    SimpleVector<int> newContained;
                    SimpleVector<timeSec_t> newContainedETA;

                    SimpleVector< SimpleVector<int> > newSubContained;
                    SimpleVector< SimpleVector<timeSec_t> > newSubContainedETA;
                    
                    for( int c=0; c< nextPlayer->numContained; c++ ) {
                        int oldID = abs( nextPlayer->containedIDs[c] );
                        int newID = oldID;

                        timeSec_t newDecay = 
                            nextPlayer->containedEtaDecays[c];

                        SimpleVector<int> subCont = 
                            nextPlayer->subContainedIDs[c];
                        SimpleVector<timeSec_t> subContDecay = 
                            nextPlayer->subContainedEtaDecays[c];

                        if( newDecay != 0 && newDecay < curTime ) {
                            
                            change = true;
                            
                            TransRecord *t = getTrans( -1, oldID );

                            newDecay = 0;

                            if( t != NULL ) {
                                
                                newID = t->newTarget;
                            
                                if( newID != 0 ) {
                                    float stretch = 
                                        getObject( nextPlayer->holdingID )->
                                        slotTimeStretch;
                                    
                                    TransRecord *newDecayT = 
                                        getTrans( -1, newID );
                                
                                    if( newDecayT != NULL ) {
                                        newDecay = 
                                            Time::timeSec() +
                                            newDecayT->autoDecaySeconds /
                                            stretch;
                                        }
                                    else {
                                        // no further decay
                                        newDecay = 0;
                                        }
                                    }
                                }
                            }
                        
                        SimpleVector<int> cVec;
                        SimpleVector<timeSec_t> dVec;

                        if( newID != 0 ) {
                            
                            int newSlots = getObject( newID )->numSlots;
                            
                            if( newID != oldID
                                &&
                                newSlots < getObject( oldID )->numSlots ) {
                                
                                // shrink sub-contained
                                subCont.shrink( newSlots );
                                subContDecay.shrink( newSlots );
                                }
                            }
                        else {
                            subCont.deleteAll();
                            subContDecay.deleteAll();
                            }

                        // handle decay for each sub-contained object
                        for( int s=0; s<subCont.size(); s++ ) {
                            int oldSubID = subCont.getElementDirect( s );
                            int newSubID = oldSubID;
                            timeSec_t newSubDecay = 
                                subContDecay.getElementDirect( s );
                            
                            if( newSubDecay != 0 && newSubDecay < curTime ) {
                            
                                change = true;
                            
                                TransRecord *t = getTrans( -1, oldSubID );

                                newSubDecay = 0;

                                if( t != NULL ) {
                                
                                    newSubID = t->newTarget;
                            
                                    if( newSubID != 0 ) {
                                        float subStretch = 
                                            getObject( newID )->
                                            slotTimeStretch;
                                    
                                        TransRecord *newSubDecayT = 
                                            getTrans( -1, newSubID );
                                
                                        if( newSubDecayT != NULL ) {
                                            newSubDecay = 
                                                Time::timeSec() +
                                                newSubDecayT->autoDecaySeconds /
                                                subStretch;
                                            }
                                        else {
                                            // no further decay
                                            newSubDecay = 0;
                                            }
                                        }
                                    }
                                }
                            
                            if( newSubID != 0 ) {
                                cVec.push_back( newSubID );
                                dVec.push_back( newSubDecay );
                                }
                            }
                        
                        if( newID != 0 ) {    
                            newSubContained.push_back( cVec );
                            newSubContainedETA.push_back( dVec );

                            if( cVec.size() > 0 ) {
                                newID *= -1;
                                }
                            
                            newContained.push_back( newID );
                            newContainedETA.push_back( newDecay );
                            }
                        }
                    
                    

                    if( change ) {
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        
                        delete [] nextPlayer->containedIDs;
                        delete [] nextPlayer->containedEtaDecays;
                        delete [] nextPlayer->subContainedIDs;
                        delete [] nextPlayer->subContainedEtaDecays;
                        
                        nextPlayer->numContained = newContained.size();
                        
                        nextPlayer->containedIDs = 
                            newContained.getElementArray();
                        nextPlayer->containedEtaDecays = 
                            newContainedETA.getElementArray();
                        
                        nextPlayer->subContainedIDs =
                            newSubContained.getElementArray();
                        nextPlayer->subContainedEtaDecays =
                            newSubContainedETA.getElementArray();
                        }
                    }
                
                
                // check if their clothing has decayed
                // or what's in their clothing
                for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                    ObjectRecord *cObj = 
                        clothingByIndex( nextPlayer->clothing, c );
                    
                    if( cObj != NULL &&
                        nextPlayer->clothingEtaDecay[c] != 0 &&
                        nextPlayer->clothingEtaDecay[c] < 
                        curTime ) {
                
                        // what they're wearing has decayed

                        int oldID = cObj->id;
                
                        TransRecord *t = getTrans( -1, oldID );

                        if( t != NULL ) {

                            int newID = t->newTarget;
                            
                            ObjectRecord *newCObj = NULL;
                            if( newID != 0 ) {
                                newCObj = getObject( newID );
                                
                                TransRecord *newDecayT = getTrans( -1, newID );
                                
                                if( newDecayT != NULL ) {
                                    nextPlayer->clothingEtaDecay[c] = 
                                        Time::timeSec() + 
                                        newDecayT->autoDecaySeconds;
                                    }
                                else {
                                    // no further decay
                                    nextPlayer->clothingEtaDecay[c] = 0;
                                    }
                                }
                            else {
                                nextPlayer->clothingEtaDecay[c] = 0;
                                }
                            
                            setClothingByIndex( &( nextPlayer->clothing ),
                                                c, newCObj );
                            
                            int oldSlots = 
                                getNumContainerSlots( oldID );

                            int newSlots = getNumContainerSlots( newID );
                    
                            if( newSlots < oldSlots ) {
                                // new container can hold less
                                // truncate
                                nextPlayer->
                                    clothingContained[c].
                                    shrink( newSlots );
                                
                                nextPlayer->
                                    clothingContainedEtaDecays[c].
                                    shrink( newSlots );
                                }
                            
                            float oldStretch = 
                                cObj->slotTimeStretch;
                            float newStretch = 
                                newCObj->slotTimeStretch;
                            
                            if( oldStretch != newStretch ) {
                                timeSec_t curTime = Time::timeSec();
                                
                                for( int cc=0;
                                     cc < nextPlayer->
                                          clothingContainedEtaDecays[c].size();
                                     cc++ ) {
                                    
                                    if( nextPlayer->
                                        clothingContainedEtaDecays[c].
                                        getElementDirect( cc ) != 0 ) {
                                        
                                        timeSec_t offset = 
                                            nextPlayer->
                                            clothingContainedEtaDecays[c].
                                            getElementDirect( cc ) - 
                                            curTime;
                                        
                                        offset = offset * oldStretch;
                                        offset = offset / newStretch;
                                        
                                        *( nextPlayer->
                                           clothingContainedEtaDecays[c].
                                           getElement( cc ) ) =
                                            curTime + offset;
                                        }
                                    }
                                }

                            playerIndicesToSendUpdatesAbout.push_back( i );
                            }
                        else {
                            // no valid decay transition, end it
                            nextPlayer->clothingEtaDecay[c] = 0;
                            }
                        
                        }
                    
                    // check for decay of what's contained in clothing
                    if( cObj != NULL &&
                        nextPlayer->clothingContainedEtaDecays[c].size() > 0 ) {
                        
                        char change = false;
                        
                        SimpleVector<int> newContained;
                        SimpleVector<timeSec_t> newContainedETA;

                        for( int cc=0; 
                             cc <
                                 nextPlayer->
                                 clothingContainedEtaDecays[c].size();
                             cc++ ) {
                            
                            int oldID = nextPlayer->
                                clothingContained[c].getElementDirect( cc );
                            int newID = oldID;
                        
                            timeSec_t decay = 
                                nextPlayer->clothingContainedEtaDecays[c]
                                .getElementDirect( cc );

                            timeSec_t newDecay = decay;
                            
                            if( decay != 0 && decay < curTime ) {
                                
                                change = true;
                            
                                TransRecord *t = getTrans( -1, oldID );
                                
                                newDecay = 0;

                                if( t != NULL ) {
                                    newID = t->newTarget;
                            
                                    if( newID != 0 ) {
                                        TransRecord *newDecayT = 
                                            getTrans( -1, newID );
                                        
                                        if( newDecayT != NULL ) {
                                            newDecay = 
                                                Time::timeSec() +
                                                newDecayT->
                                                autoDecaySeconds /
                                                cObj->slotTimeStretch;
                                            }
                                        else {
                                            // no further decay
                                            newDecay = 0;
                                            }
                                        }
                                    }
                                }
                        
                            if( newID != 0 ) {
                                newContained.push_back( newID );
                                newContainedETA.push_back( newDecay );
                                } 
                            }
                        
                        if( change ) {
                            playerIndicesToSendUpdatesAbout.push_back( i );
                            
                            // assignment operator for vectors
                            // copies one vector into another
                            // replacing old contents
                            nextPlayer->clothingContained[c] =
                                newContained;
                            nextPlayer->clothingContainedEtaDecays[c] =
                                newContainedETA;
                            }
                        
                        }
                    
                    
                    }
                

                // check if they are done moving
                // if so, send an update
                

                if( nextPlayer->xd != nextPlayer->xs ||
                    nextPlayer->yd != nextPlayer->ys ) {
                
                    
                    if( Time::getCurrentTime() - nextPlayer->moveStartTime
                        >
                        nextPlayer->moveTotalSeconds ) {
                        
                        // done
                        nextPlayer->xs = nextPlayer->xd;
                        nextPlayer->ys = nextPlayer->yd;
                        nextPlayer->newMove = false;
                        

                        printf( "Player %d's move is done at %d,%d\n",
                                nextPlayer->id,
                                nextPlayer->xs,
                                nextPlayer->ys );

                        if( nextPlayer->pathTruncated ) {
                            // truncated, but never told them about it
                            // force update now
                            nextPlayer->posForced = true;
                            }
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        }
                    }
                
                // check if we need to decrement their food
                if( Time::getCurrentTime() > 
                    nextPlayer->foodDecrementETASeconds ) {
                    
                    // only if femail of fertile age
                    char heldByFemale = false;
                    
                    if( nextPlayer->heldByOther ) {
                        LiveObject *adultO = getAdultHolding( nextPlayer );
                        
                        if( adultO != NULL &&
                            isFertileAge( adultO ) ) {
                    
                            heldByFemale = true;
                            }
                        }
                    
                    
                    LiveObject *decrementedPlayer = NULL;

                    if( !heldByFemale ) {
                        nextPlayer->foodStore --;
                        decrementedPlayer = nextPlayer;
                        }
                    // if held by fertile female, food for baby is free for
                    // duration of holding
                    
                    // only update the time of the fed player
                    nextPlayer->foodDecrementETASeconds +=
                        computeFoodDecrementTimeSeconds( nextPlayer );

                    

                    if( decrementedPlayer != NULL &&
                        decrementedPlayer->foodStore <= 0 ) {
                        // player has died
                        
                        // break the connection with them

                        if( heldByFemale ) {
                            setDeathReason( decrementedPlayer, 
                                            "nursing_hunger" );
                            }
                        else {
                            setDeathReason( decrementedPlayer, 
                                            "hunger" );
                            }
                        
                        decrementedPlayer->error = true;
                        decrementedPlayer->errorCauseString = "Player starved";


                        GridPos deathPos;
                                        
                        if( decrementedPlayer->xd == 
                            decrementedPlayer->xs &&
                            decrementedPlayer->yd ==
                            decrementedPlayer->ys ) {
                            // deleted player standing still
                            
                            deathPos.x = decrementedPlayer->xd;
                            deathPos.y = decrementedPlayer->yd;
                            }
                        else {
                            // player moving
                            
                            deathPos = 
                                computePartialMoveSpot( decrementedPlayer );
                            }
                        
                        logDeath( decrementedPlayer->id,
                                  decrementedPlayer->email,
                                  decrementedPlayer->isEve,
                                  computeAge( decrementedPlayer ),
                                  getSecondsPlayed( decrementedPlayer ),
                                  ! getFemale( decrementedPlayer ),
                                  deathPos.x, deathPos.y,
                                  players.size() - 1,
                                  false );
                        
                        if( shutdownMode ) {
                            handleShutdownDeath( decrementedPlayer,
                                                 deathPos.x, deathPos.y );
                            }
                                            
                        decrementedPlayer->deathLogged = true;
                                        

                        // no negative
                        decrementedPlayer->foodStore = 0;
                        }
                    
                    if( decrementedPlayer != NULL ) {
                        decrementedPlayer->foodUpdate = true;
                        }
                    }
                
                }
            
            
            }
        

        
        // check for any that have been individually flagged, but
        // aren't on our list yet (updates caused by external triggers)
        for( int i=0; i<players.size() ; i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            
            if( nextPlayer->needsUpdate ) {
                playerIndicesToSendUpdatesAbout.push_back( i );
            
                nextPlayer->needsUpdate = false;
                }
            }
        

        
        for( int i=0; i<playerIndicesToSendUpdatesAbout.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement( 
                playerIndicesToSendUpdatesAbout.getElementDirect( i ) );

            if( nextPlayer->updateSent ) {
                continue;
                }
            

            // recompute heat map
            
            
            // what if we recompute it from scratch every time?
            for( int i=0; i<HEAT_MAP_D * HEAT_MAP_D; i++ ) {
                nextPlayer->heatMap[i] = 0;
                }

            float heatOutputGrid[ HEAT_MAP_D * HEAT_MAP_D ];
            float rGrid[ HEAT_MAP_D * HEAT_MAP_D ];

            for( int y=0; y<HEAT_MAP_D; y++ ) {
                int mapY = nextPlayer->ys + y - HEAT_MAP_D / 2;
                
                for( int x=0; x<HEAT_MAP_D; x++ ) {
                    
                    int mapX = nextPlayer->xs + x - HEAT_MAP_D / 2;
                    
                    int j = y * HEAT_MAP_D + x;
                    heatOutputGrid[j] = 0;
                    rGrid[j] = 0;
                    
                    heatOutputGrid[j] +=
                        getBiomeHeatValue( getMapBiome( mapX, mapY ) );


                    ObjectRecord *o = getObject( getMapObject( mapX, mapY ) );
                    
                    
                    

                    if( o != NULL ) {
                        heatOutputGrid[j] += o->heatValue;
                        if( o->permanent ) {
                            // loose objects sitting on ground don't
                            // contribute to r-value (like dropped clothing)
                            rGrid[j] = o->rValue;
                            }


                        // skip checking for heat-producing contained items
                        // for now.  Consumes too many server-side resources
                        // can still check for heat produced by stuff in
                        // held container (below).
                        
                        if( false && o->numSlots > 0 ) {
                            // contained can produce heat shielded by container
                            // r value
                            double oRFactor = 1 - o->rValue;
                            
                            int numCont;
                            int *cont = getContained( mapX, mapY, &numCont );
                            
                            if( cont != NULL ) {
                                
                                for( int c=0; c<numCont; c++ ) {
                                    
                                    int cID = cont[c];
                                    char hasSub = false;
                                    if( cID < 0 ) {
                                        hasSub = true;
                                        cID = -cID;
                                        }

                                    ObjectRecord *cO = getObject( cID );
                                    heatOutputGrid[j] += 
                                        cO->heatValue * oRFactor;
                                    
                                    if( hasSub ) {
                                        double cRFactor = 1 - cO->rValue;
                                        
                                        int numSub;
                                        int *sub = getContained( mapX, mapY, 
                                                                 &numSub, 
                                                                 c + 1 );
                                        if( sub != NULL ) {
                                            for( int s=0; s<numSub; s++ ) {
                                                ObjectRecord *sO = 
                                                    getObject( sub[s] );
                                                
                                                heatOutputGrid[j] += 
                                                    sO->heatValue * 
                                                    cRFactor * 
                                                    oRFactor;
                                                }
                                            delete [] sub;
                                            }
                                        }
                                    }
                                delete [] cont;
                                }
                            }
                        }
                    

                    // floor can insulate or produce heat too
                    ObjectRecord *fO = getObject( getMapFloor( mapX, mapY ) );
                    
                    if( fO != NULL ) {
                        heatOutputGrid[j] += fO->heatValue;
                        rGrid[j] += fO->rValue;
                        }
                    }
                }

            // clothing is additive to R value at center spot

            float headWeight = 0.25;
            float chestWeight = 0.35;
            float buttWeight = 0.2;
            float eachFootWeigth = 0.1;
            
            float backWeight = 0.1;


            float clothingR = 0;
            
            if( nextPlayer->clothing.hat != NULL ) {
                clothingR += headWeight *  nextPlayer->clothing.hat->rValue;
                }
            if( nextPlayer->clothing.tunic != NULL ) {
                clothingR += chestWeight * nextPlayer->clothing.tunic->rValue;
                }
            if( nextPlayer->clothing.frontShoe != NULL ) {
                clothingR += 
                    eachFootWeigth * nextPlayer->clothing.frontShoe->rValue;
                }
            if( nextPlayer->clothing.backShoe != NULL ) {
                clothingR += eachFootWeigth * 
                    nextPlayer->clothing.backShoe->rValue;
                }
            if( nextPlayer->clothing.bottom != NULL ) {
                clothingR += buttWeight * nextPlayer->clothing.bottom->rValue;
                }
            if( nextPlayer->clothing.backpack != NULL ) {
                clothingR += backWeight * nextPlayer->clothing.backpack->rValue;
                }

            //printf( "Clothing r = %f\n", clothingR );
            
            
            int playerMapIndex = 
                ( HEAT_MAP_D / 2 ) * HEAT_MAP_D +
                ( HEAT_MAP_D / 2 );
            

            rGrid[ playerMapIndex ] += clothingR;
            
            
            if( rGrid[ playerMapIndex ] > 1 ) {
                
                rGrid[ playerMapIndex ] = 1;
                }
            

            // body itself produces 1 unit of heat
            // (r value of clothing can hold this in
            heatOutputGrid[ playerMapIndex ] += 1;
            

            // what player is holding can contribute heat
            if( nextPlayer->holdingID > 0 ) {
                ObjectRecord *heldO = getObject( nextPlayer->holdingID );
                
                heatOutputGrid[ playerMapIndex ] += heldO->heatValue;
                
                double heldRFactor = 1 - heldO->rValue;
                
                // contained can contribute too, but shielded by r-value
                // of container
                for( int c=0; c<nextPlayer->numContained; c++ ) {
                    
                    int cID = nextPlayer->containedIDs[c];
                    char hasSub = false;
                    
                    if( cID < 0 ) {
                        hasSub = true;
                        cID = -cID;
                        }

                    ObjectRecord *contO = getObject( cID );
                    
                    heatOutputGrid[ playerMapIndex ] += 
                        contO->heatValue * heldRFactor;
                    

                    if( hasSub ) {
                        // sub contained too, but shielded by both r-values
                        double contRFactor = 1 - contO->rValue;

                        for( int s=0; 
                             s<nextPlayer->subContainedIDs[c].size(); s++ ) {
                        
                            ObjectRecord *subO =
                                getObject( nextPlayer->subContainedIDs[c].
                                       getElementDirect( s ) );
                            
                            heatOutputGrid[ playerMapIndex ] += 
                                subO->heatValue * 
                                contRFactor * heldRFactor;
                            }
                        }
                    }
                }
            
            // clothing can contribute heat
            for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                
                ObjectRecord *cO = clothingByIndex( nextPlayer->clothing, c );
            
                if( cO != NULL ) {
                    heatOutputGrid[playerMapIndex ] += cO->heatValue;

                    // contained items in clothing can contribute
                    // heat, shielded by clothing r-values
                    double cRFactor = 1 - cO->rValue;

                    for( int s=0; 
                         s < nextPlayer->clothingContained[c].size(); s++ ) {
                        
                        ObjectRecord *sO = 
                            getObject( nextPlayer->clothingContained[c].
                                       getElementDirect( s ) );
                        
                        heatOutputGrid[ playerMapIndex ] += 
                            sO->heatValue * cRFactor;
                        }
                    }
                }
            

            
            //double startTime = Time::getCurrentTime();
            
            int numCycles = 8;
            
            int numNeighbors = 8;
            int ndx[8] = { 0, 1,  0, -1,  1,  1, -1, -1 };
            int ndy[8] = { 1, 0, -1,  0,  1, -1,  1, -1 };
            
            // found equation here:
            // http://demonstrations.wolfram.com/
            //        ACellularAutomatonBasedHeatEquation/
            // diags have way less contact area
            double nWeights[8] = { 4, 4, 4, 4, 1, 1, 1, 1 };
            
            double totalNWeight = 20;
            
            
            //double startTime = Time::getCurrentTime();

            for( int c=0; c<numCycles; c++ ) {
                
                float tempHeatGrid[ HEAT_MAP_D * HEAT_MAP_D ];
                memcpy( tempHeatGrid, nextPlayer->heatMap, 
                        HEAT_MAP_D * HEAT_MAP_D * sizeof( float ) );
                
                for( int y=1; y<HEAT_MAP_D-1; y++ ) {
                    for( int x=1; x<HEAT_MAP_D-1; x++ ) {
                        int j = y * HEAT_MAP_D + x;
            
                        float heatDelta = 0;
                
                        float centerLeak = 1 - rGrid[j];

                        float centerOldHeat = tempHeatGrid[j];

                        for( int n=0; n<numNeighbors; n++ ) {
                
                            int nx = x + ndx[n];
                            int ny = y + ndy[n];
                        
                            int nj = ny * HEAT_MAP_D + nx;
                        
                            float nLeak = 1 - rGrid[ nj ];
                    
                            heatDelta += nWeights[n] * centerLeak * nLeak *
                                ( tempHeatGrid[ nj ] - centerOldHeat );
                            }
                
                        nextPlayer->heatMap[j] = 
                            tempHeatGrid[j] + heatDelta / totalNWeight;
                
                        nextPlayer->heatMap[j] += heatOutputGrid[j];
                        }
                    }
                } 
            
            //printf( "Computing %d cycles took %f ms\n",
            //        numCycles, 
            //        ( Time::getCurrentTime() - startTime ) * 1000 );
            /*
            printf( "Player heat map:\n" );
            
            for( int y=0; y<HEAT_MAP_D; y++ ) {
                
                for( int x=0; x<HEAT_MAP_D; x++ ) {                    
                    int j = y * HEAT_MAP_D + x;
                    
                    printf( "%04.1f ", 
                            nextPlayer->heatMap[ j ] );
                            //tempHeatGrid[ y * HEAT_MAP_D + x ] );
                    }
                printf( "\n" );

                char anyTags = false;
                for( int x=0; x<HEAT_MAP_D; x++ ) {
                    int j = y * HEAT_MAP_D + x;
                    
                    if( j == playerMapIndex ) {
                        anyTags = true;
                        break;
                        }
                    if( heatOutputGrid[ j ] > 0 ) {
                        anyTags = true;
                        break;
                        }
                    }
                
                if( anyTags ) {
                    for( int x=0; x<HEAT_MAP_D; x++ ) {                    
                        int j = y * HEAT_MAP_D + x;
                        if( j == playerMapIndex ) {
                            printf( "p" );
                            }
                        else printf( " " );
                        
                        if( heatOutputGrid[j] > 0 ) {
                            printf( "H%d   ", 
                                    heatOutputGrid[j] );
                            }
                        else {
                            printf( "    " );
                            }
                        }
                    printf( "\n" );
                    }
                }
            */

            float playerHeat = 
                nextPlayer->heatMap[ playerMapIndex ];
            
            // printf( "Player heat = %f\n", playerHeat );
            
            // convert into 0..1 range, where 0.5 represents targetHeat
            nextPlayer->heat = ( playerHeat / targetHeat ) / 2;
            if( nextPlayer->heat > 1 ) {
                nextPlayer->heat = 1;
                }
            if( nextPlayer->heat < 0 ) {
                nextPlayer->heat = 0;
                }

            
            char *updateLine = getUpdateLine( nextPlayer, false );

            
            nextPlayer->posForced = false;

            newUpdates.appendElementString( updateLine );
            newUpdatePlayerIDs.push_back( nextPlayer->id );
            
            ChangePosition p = { nextPlayer->xs, nextPlayer->ys, false };
            newUpdatesPos.push_back( p );

            delete [] updateLine;

            nextPlayer->updateSent = true;
            }
        

        
        SimpleVector<ChangePosition> movesPos;        

        char *moveMessageText = getMovesMessage( true, &movesPos );
        
        unsigned char *moveMessage = NULL;
        int moveMessageLength = 0;
        
        if( moveMessageText != NULL ) {
            moveMessage = (unsigned char*)moveMessageText;
            moveMessageLength = strlen( moveMessageText );

            if( moveMessageLength > maxUncompressedSize ) {
                moveMessage = makeCompressedMessage( moveMessageText,
                                                     moveMessageLength,
                                                     &moveMessageLength );
                delete [] moveMessageText;
                }    
            }
        
                



        unsigned char *updateMessage = NULL;
        int updateMessageLength = 0;
        
        if( newUpdates.size() > 0 ) {
            newUpdates.push_back( '#' );
            char *temp = newUpdates.getElementString();

            char *updateMessageText = concatonate( "PU\n", temp );
            delete [] temp;

            updateMessageLength = strlen( updateMessageText );

            if( updateMessageLength < maxUncompressedSize ) {
                updateMessage = (unsigned char*)updateMessageText;
                }
            else {
                // compress for all players once here
                updateMessage = makeCompressedMessage( 
                    updateMessageText, 
                    updateMessageLength, &updateMessageLength );
                
                delete [] updateMessageText;
                }
            }




        unsigned char *outOfRangeMessage = NULL;
        int outOfRangeMessageLength = 0;
        
        if( newUpdatePlayerIDs.size() > 0 ) {
            SimpleVector<char> messageChars;
            
            messageChars.appendElementString( "PO\n" );
            
            for( int i=0; i<newUpdatePlayerIDs.size(); i++ ) {
                char buffer[20];
                sprintf( buffer, "%d\n",
                         newUpdatePlayerIDs.getElementDirect( i ) );
                
                messageChars.appendElementString( buffer );
                }
            messageChars.push_back( '#' );

            char *outOfRangeMessageText = messageChars.getElementString();

            outOfRangeMessageLength = strlen( outOfRangeMessageText );

            if( outOfRangeMessageLength < maxUncompressedSize ) {
                outOfRangeMessage = (unsigned char*)outOfRangeMessageText;
                }
            else {
                // compress for all players once here
                outOfRangeMessage = makeCompressedMessage( 
                    outOfRangeMessageText, 
                    outOfRangeMessageLength, &outOfRangeMessageLength );
                
                delete [] outOfRangeMessageText;
                }
            }
        

        

        unsigned char *deleteUpdateMessage = NULL;
        int deleteUpdateMessageLength = 0;
        
        if( newDeleteUpdates.size() > 0 ) {
            newDeleteUpdates.push_back( '#' );
            char *temp = newDeleteUpdates.getElementString();
            
            char *deleteUpdateMessageText = concatonate( "PU\n", temp );
            delete [] temp;

            deleteUpdateMessageLength = strlen( deleteUpdateMessageText );

            if( deleteUpdateMessageLength < maxUncompressedSize ) {
                deleteUpdateMessage = (unsigned char*)deleteUpdateMessageText;
                }
            else {
                // compress for all players once here
                deleteUpdateMessage = makeCompressedMessage( 
                    deleteUpdateMessageText, 
                    deleteUpdateMessageLength, &deleteUpdateMessageLength );
                
                delete [] deleteUpdateMessageText;
                }
            }
        

        
                

        // add changes from auto-decays on map, mixed with player-caused changes
        stepMap( &mapChanges, &mapChangesPos );
        

        
        
        unsigned char *mapChangeMessage = NULL;
        int mapChangeMessageLength = 0;
        
        if( mapChanges.size() > 0 ) {
            mapChanges.push_back( '#' );
            char *temp = mapChanges.getElementString();

            char *mapChangeMessageText = concatonate( "MX\n", temp );
            delete [] temp;

            mapChangeMessageLength = strlen( mapChangeMessageText );
            
            if( mapChangeMessageLength < maxUncompressedSize ) {
                mapChangeMessage = (unsigned char*)mapChangeMessageText;
                }
            else {
                // compress for all players once here
                mapChangeMessage = makeCompressedMessage( 
                    mapChangeMessageText, 
                    mapChangeMessageLength, &mapChangeMessageLength );
                
                delete [] mapChangeMessageText;
                }

            }
        


        unsigned char *speechMessage = NULL;
        int speechMessageLength = 0;
        
        if( newSpeech.size() > 0 ) {
            newSpeech.push_back( '#' );
            char *temp = newSpeech.getElementString();

            char *speechMessageText = concatonate( "PS\n", temp );
            delete [] temp;

            speechMessageLength = strlen( speechMessageText );
            
            if( speechMessageLength < maxUncompressedSize ) {
                speechMessage = (unsigned char*)speechMessageText;
                }
            else {
                // compress for all players once here
                speechMessage = makeCompressedMessage( 
                    speechMessageText, 
                    speechMessageLength, &speechMessageLength );
                
                delete [] speechMessageText;
                }

            }


        unsigned char *lineageMessage = NULL;
        int lineageMessageLength = 0;
        
        if( playerIndicesToSendLineageAbout.size() > 0 ) {
            SimpleVector<char> linWorking;
            linWorking.appendElementString( "LN\n" );
            
            int numAdded = 0;
            for( int i=0; i<playerIndicesToSendLineageAbout.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( 
                    playerIndicesToSendLineageAbout.getElementDirect( i ) );

                if( nextPlayer->error ) {
                    continue;
                    }

                char *pID = autoSprintf( "%d", nextPlayer->id );
                linWorking.appendElementString( pID );
                delete [] pID;
                numAdded++;
                for( int j=0; j<nextPlayer->lineage->size(); j++ ) {
                    char *mID = 
                        autoSprintf( 
                            " %d",
                            nextPlayer->lineage->getElementDirect( j ) );
                    linWorking.appendElementString( mID );
                    delete [] mID;
                    }        
                linWorking.push_back( '\n' );
                }
            
            linWorking.push_back( '#' );
            
            if( numAdded > 0 ) {

                char *lineageMessageText = linWorking.getElementString();
                
                lineageMessageLength = strlen( lineageMessageText );
                
                if( lineageMessageLength < maxUncompressedSize ) {
                    lineageMessage = (unsigned char*)lineageMessageText;
                    }
                else {
                    // compress for all players once here
                    lineageMessage = makeCompressedMessage( 
                        lineageMessageText, 
                        lineageMessageLength, &lineageMessageLength );
                    
                    delete [] lineageMessageText;
                    }
                }
            }




        unsigned char *namesMessage = NULL;
        int namesMessageLength = 0;
        
        if( playerIndicesToSendNamesAbout.size() > 0 ) {
            SimpleVector<char> namesWorking;
            namesWorking.appendElementString( "NM\n" );
            
            int numAdded = 0;
            for( int i=0; i<playerIndicesToSendNamesAbout.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( 
                    playerIndicesToSendNamesAbout.getElementDirect( i ) );

                if( nextPlayer->error ) {
                    continue;
                    }

                char *line = autoSprintf( "%d %s\n", nextPlayer->id,
                                          nextPlayer->name );
                numAdded++;
                namesWorking.appendElementString( line );
                delete [] line;
                }
            
            namesWorking.push_back( '#' );
            
            if( numAdded > 0 ) {

                char *namesMessageText = namesWorking.getElementString();
                
                namesMessageLength = strlen( namesMessageText );
                
                if( namesMessageLength < maxUncompressedSize ) {
                    namesMessage = (unsigned char*)namesMessageText;
                    }
                else {
                    // compress for all players once here
                    namesMessage = makeCompressedMessage( 
                        namesMessageText, 
                        namesMessageLength, &namesMessageLength );
                    
                    delete [] namesMessageText;
                    }
                }
            }



        unsigned char *dyingMessage = NULL;
        int dyingMessageLength = 0;
        
        if( playerIndicesToSendDyingAbout.size() > 0 ) {
            SimpleVector<char> dyingWorking;
            dyingWorking.appendElementString( "DY\n" );
            
            int numAdded = 0;
            for( int i=0; i<playerIndicesToSendDyingAbout.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( 
                    playerIndicesToSendDyingAbout.getElementDirect( i ) );

                if( nextPlayer->error ) {
                    continue;
                    }

                char *line = autoSprintf( "%d\n", nextPlayer->id );

                numAdded++;
                dyingWorking.appendElementString( line );
                delete [] line;
                }
            
            dyingWorking.push_back( '#' );
            
            if( numAdded > 0 ) {

                char *dyingMessageText = dyingWorking.getElementString();
                
                dyingMessageLength = strlen( dyingMessageText );
                
                if( dyingMessageLength < maxUncompressedSize ) {
                    dyingMessage = (unsigned char*)dyingMessageText;
                    }
                else {
                    // compress for all players once here
                    dyingMessage = makeCompressedMessage( 
                        dyingMessageText, 
                        dyingMessageLength, &dyingMessageLength );
                    
                    delete [] dyingMessageText;
                    }
                }
            }
        
        
        // send moves and updates to clients
        
        for( int i=0; i<numLive; i++ ) {
            
            LiveObject *nextPlayer = players.getElement(i);

            
            if( ! nextPlayer->firstMessageSent ) {
                

                // first, send the map chunk around them
                
                int numSent = sendMapChunkMessage( nextPlayer );
                
                if( numSent == -2 ) {
                    // still not sent, try again later
                    continue;
                    }



                // now send starting message
                SimpleVector<char> messageBuffer;

                messageBuffer.appendElementString( "PU\n" );

                int numPlayers = players.size();
            
                // must be last in message
                char *playersLine = NULL;
                
                for( int i=0; i<numPlayers; i++ ) {
                
                    LiveObject *o = players.getElement( i );
                
                    if( o != nextPlayer && o->error ) {
                        continue;
                        }

                    
                    // true mid-move positions for first message
                    char *messageLine = getUpdateLine( o, false, true );
                    
                    // skip sending info about errored players in
                    // first message
                    if( o->id != nextPlayer->id ) {
                        messageBuffer.appendElementString( messageLine );
                        delete [] messageLine;
                        }
                    else {
                        // save until end
                        playersLine = messageLine;
                        }
                    }
                
                if( playersLine != NULL ) {    
                    messageBuffer.appendElementString( playersLine );
                    delete [] playersLine;
                    }
                
                messageBuffer.push_back( '#' );
            
                char *message = messageBuffer.getElementString();


                sendMessageToPlayer( nextPlayer, message, strlen( message ) );
                
                delete [] message;
                

                char *movesMessage = getMovesMessage( false );
                
                if( movesMessage != NULL ) {
                    
                
                    sendMessageToPlayer( nextPlayer, movesMessage, 
                                         strlen( movesMessage ) );
                
                    delete [] movesMessage;
                    }



                // send lineage for everyone alive
                
                
                SimpleVector<char> linWorking;
                linWorking.appendElementString( "LN\n" );

                int numAdded = 0;
                
                for( int i=0; i<numPlayers; i++ ) {
                
                    LiveObject *o = players.getElement( i );
                
                    if( o->error ) {
                        continue;
                        }

                    char *pID = autoSprintf( "%d", o->id );
                    linWorking.appendElementString( pID );
                    delete [] pID;
                    numAdded++;
                    for( int j=0; j<o->lineage->size(); j++ ) {
                        char *mID = 
                            autoSprintf( 
                                " %d",
                                o->lineage->getElementDirect( j ) );
                        linWorking.appendElementString( mID );
                        delete [] mID;
                        }        
                    linWorking.push_back( '\n' );
                    }
                
                linWorking.push_back( '#' );
            
                if( numAdded > 0 ) {
                    char *linMessage = linWorking.getElementString();


                    sendMessageToPlayer( nextPlayer, linMessage, 
                                         strlen( linMessage ) );
                
                    delete [] linMessage;
                    }



                // send names for everyone alive
                
                SimpleVector<char> namesWorking;
                namesWorking.appendElementString( "NM\n" );

                numAdded = 0;
                
                for( int i=0; i<numPlayers; i++ ) {
                
                    LiveObject *o = players.getElement( i );
                
                    if( o->error || o->name == NULL) {
                        continue;
                        }

                    char *line = autoSprintf( "%d %s\n", o->id, o->name );
                    namesWorking.appendElementString( line );
                    delete [] line;
                    
                    numAdded++;
                    }
                
                namesWorking.push_back( '#' );
            
                if( numAdded > 0 ) {
                    char *namesMessage = namesWorking.getElementString();


                    sendMessageToPlayer( nextPlayer, namesMessage, 
                                         strlen( namesMessage ) );
                
                    delete [] namesMessage;
                    }




                // send dying for everyone who is dying
                
                SimpleVector<char> dyingWorking;
                dyingWorking.appendElementString( "DY\n" );

                numAdded = 0;
                
                for( int i=0; i<numPlayers; i++ ) {
                
                    LiveObject *o = players.getElement( i );
                
                    if( o->error || ! o->dying ) {
                        continue;
                        }

                    char *line = autoSprintf( "%d\n", o->id );
                    dyingWorking.appendElementString( line );
                    delete [] line;
                    
                    numAdded++;
                    }
                
                dyingWorking.push_back( '#' );
            
                if( numAdded > 0 ) {
                    char *dyingMessage = dyingWorking.getElementString();


                    sendMessageToPlayer( nextPlayer, dyingMessage, 
                                         strlen( dyingMessage ) );
                
                    delete [] dyingMessage;
                    }

                
                nextPlayer->firstMessageSent = true;
                }
            else {
                // this player has first message, ready for updates/moves
                
                int playerXD = nextPlayer->xd;
                int playerYD = nextPlayer->yd;
                
                if( nextPlayer->heldByOther ) {
                    LiveObject *holdingPlayer = 
                        getLiveObject( nextPlayer->heldByOtherID );
                
                    playerXD = holdingPlayer->xd;
                    playerYD = holdingPlayer->yd;
                    }


                if( abs( playerXD - nextPlayer->lastSentMapX ) > 7
                    ||
                    abs( playerYD - nextPlayer->lastSentMapY ) > 8 ) {
                
                    // moving out of bounds of chunk, send update
                    
                    
                    sendMapChunkMessage( nextPlayer,
                                         // override if held
                                         nextPlayer->heldByOther,
                                         playerXD,
                                         playerYD );


                    // send updates about any non-moving players
                    // that are in this chunk
                    SimpleVector<char> chunkPlayerUpdates;

                    SimpleVector<char> chunkPlayerMoves;
                    

                    // add chunk updates for held babies first
                    for( int j=0; j<numLive; j++ ) {
                        LiveObject *otherPlayer = players.getElement( j );
                        
                        if( otherPlayer->error ) {
                            continue;
                            }


                        if( otherPlayer->heldByOther ) {
                            LiveObject *adultO = 
                                getAdultHolding( otherPlayer );
                            
                            if( adultO != NULL ) {
                                

                                if( adultO->id != nextPlayer->id &&
                                    otherPlayer->id != nextPlayer->id ) {
                                    // parent not us
                                    // baby not us

                                    double d = intDist( playerXD,
                                                        playerYD,
                                                        adultO->xd,
                                                        adultO->yd );
                            
                            
                                    if( d <= getMaxChunkDimension() / 2 ) {
                                        // adult holding this baby
                                        // is close enough
                                        // send update about baby
                                        char *updateLine = 
                                            getUpdateLine( otherPlayer, 
                                                           false ); 
                                    
                                        chunkPlayerUpdates.
                                            appendElementString( updateLine );
                                        delete [] updateLine;
                                        }
                                    }
                                }
                            }
                        }
                    
                    
                    int ourHolderID = -1;
                    
                    if( nextPlayer->heldByOther ) {
                        ourHolderID = getAdultHolding( nextPlayer )->id;
                        }
                    
                    // now send updates about all non-held babies,
                    // including any adults holding on-chunk babies
                    // here, AFTER we update about the babies

                    // (so their held status overrides the baby's stale
                    //  position status).
                    for( int j=0; j<numLive; j++ ) {
                        LiveObject *otherPlayer = 
                            players.getElement( j );
                        
                        if( otherPlayer->error ) {
                            continue;
                            }


                        if( !otherPlayer->heldByOther &&
                            otherPlayer->id != nextPlayer->id &&
                            otherPlayer->id != ourHolderID ) {
                            // not us
                            // not a held baby (covered above)
                            // no the adult holding us

                            double d = intDist( playerXD,
                                                playerYD,
                                                otherPlayer->xd,
                                                otherPlayer->yd );
                            
                            
                            if( d <= getMaxChunkDimension() / 2 ) {
                                
                                // send next player a player update
                                // about this player, telling nextPlayer
                                // where this player was last stationary
                                // and what they're holding

                                char *updateLine = 
                                    getUpdateLine( otherPlayer, false ); 
                                    
                                chunkPlayerUpdates.appendElementString( 
                                    updateLine );
                                delete [] updateLine;
                                

                                // We don't need to tell player about 
                                // moves in progress on this chunk.
                                // We're receiving move messages from 
                                // a radius of 32
                                // but this chunk has a radius of 16
                                // so we're hearing about player moves
                                // before they're on our chunk.
                                // Player moves have limited length,
                                // so there's no chance of a long move
                                // that started outside of our 32-radius
                                // finishinging inside this new chunk.
                                }
                            }
                        }


                    if( chunkPlayerUpdates.size() > 0 ) {
                        chunkPlayerUpdates.push_back( '#' );
                        char *temp = chunkPlayerUpdates.getElementString();

                        char *message = concatonate( "PU\n", temp );
                        delete [] temp;

                        sendMessageToPlayer( nextPlayer, message, 
                                             strlen( message ) );
                        
                        delete [] message;
                        }

                    
                    if( chunkPlayerMoves.size() > 0 ) {
                        char *temp = chunkPlayerMoves.getElementString();

                        sendMessageToPlayer( nextPlayer, temp, strlen( temp ) );

                        delete [] temp;
                        }
                    }
                // done handling sending new map chunk and player updates
                // for players in the new chunk
                
                

                // EVERYONE gets info about dying players

                // do this first, so that PU messages about what they 
                // are holding post-wound come later                
                if( dyingMessage != NULL ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            dyingMessage, 
                            dyingMessageLength, 
                            false, false );
                    
                    if( numSent != dyingMessageLength ) {
                        setDeathReason( nextPlayer, "disconnected" );

                        nextPlayer->error = true;
                        nextPlayer->errorCauseString =
                            "Socket write failed";
                        }
                    }


                

                double maxDist = 32;


                if( updateMessage != NULL ) {

                    double minUpdateDist = 64;
                    
                    for( int u=0; u<newUpdatesPos.size(); u++ ) {
                        ChangePosition *p = newUpdatesPos.getElement( u );
                        
                        // update messages can be global when a new
                        // player joins or an old player is deleted
                        if( p->global ) {
                            minUpdateDist = 0;
                            }
                        else {
                            double d = intDist( p->x, p->y, 
                                                playerXD, 
                                                playerYD );
                    
                            if( d < minUpdateDist ) {
                                minUpdateDist = d;
                                }
                            }
                        }

                    if( minUpdateDist <= maxDist ) {
                        int numSent = 
                            nextPlayer->sock->send( 
                                updateMessage, 
                                updateMessageLength, 
                                false, false );

                        if( numSent != updateMessageLength ) {
                            setDeathReason( nextPlayer, "disconnected" );

                            nextPlayer->error = true;
                            nextPlayer->errorCauseString =
                                "Socket write failed";
                            }
                        }
                    else if( outOfRangeMessage != NULL ) {
                        // everyone in the PU is out of range
                        // send short PO instead
                        int numSent = 
                            nextPlayer->sock->send( 
                                outOfRangeMessage, 
                                outOfRangeMessageLength, 
                                false, false );

                        if( numSent != outOfRangeMessageLength ) {
                            setDeathReason( nextPlayer, "disconnected" );

                            nextPlayer->error = true;
                            nextPlayer->errorCauseString =
                                "Socket write failed";
                            }
                        }
                    }
                if( moveMessage != NULL ) {
                    
                    double minUpdateDist = 64;
                    
                    for( int u=0; u<movesPos.size(); u++ ) {
                        ChangePosition *p = movesPos.getElement( u );
                        
                        // move messages are never global

                        double d = intDist( p->x, p->y, 
                                            playerXD, playerYD );
                    
                        if( d < minUpdateDist ) {
                            minUpdateDist = d;
                            if( minUpdateDist <= maxDist ) {
                                break;
                                }
                            }
                        }

                    if( minUpdateDist <= maxDist ) {
                        
                        int numSent = 
                            nextPlayer->sock->send( 
                                moveMessage, 
                                moveMessageLength, 
                                false, false );

                        if( numSent != moveMessageLength ) {
                            setDeathReason( nextPlayer, "disconnected" );

                            nextPlayer->error = true;
                            nextPlayer->errorCauseString =
                                "Socket write failed";
                            }
                        }
                    }
                if( mapChangeMessage != NULL ) {
                    double minUpdateDist = 64;
                    
                    for( int u=0; u<mapChangesPos.size(); u++ ) {
                        ChangePosition *p = mapChangesPos.getElement( u );
                        
                        // map changes are never global

                        double d = intDist( p->x, p->y, 
                                            playerXD, playerYD );
                        
                        if( d < minUpdateDist ) {
                            minUpdateDist = d;
                            }
                        }

                    if( minUpdateDist <= maxDist ) {
                        int numSent = 
                            nextPlayer->sock->send( 
                                mapChangeMessage, 
                                mapChangeMessageLength, 
                                false, false );
                        
                        if( numSent != mapChangeMessageLength ) {
                            setDeathReason( nextPlayer, "disconnected" );

                            nextPlayer->error = true;
                            nextPlayer->errorCauseString =
                                "Socket write failed";
                            }
                        }
                    }
                if( speechMessage != NULL ) {
                    double minUpdateDist = 64;
                    
                    for( int u=0; u<newSpeechPos.size(); u++ ) {
                        ChangePosition *p = newSpeechPos.getElement( u );
                        
                        // speech never global

                        double d = intDist( p->x, p->y, 
                                            playerXD, playerYD );
                        
                        if( d < minUpdateDist ) {
                            minUpdateDist = d;
                            }
                        }

                    if( minUpdateDist <= maxDist ) {
                        int numSent = 
                            nextPlayer->sock->send( 
                                speechMessage, 
                                speechMessageLength, 
                                false, false );
                        
                        if( numSent != speechMessageLength ) {
                            setDeathReason( nextPlayer, "disconnected" );

                            nextPlayer->error = true;
                            nextPlayer->errorCauseString =
                                "Socket write failed";
                            }
                        }
                    }
                

                // EVERYONE gets updates about deleted players
                if( deleteUpdateMessage != NULL ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            deleteUpdateMessage, 
                            deleteUpdateMessageLength, 
                            false, false );
                    
                    if( numSent != deleteUpdateMessageLength ) {
                        setDeathReason( nextPlayer, "disconnected" );

                        nextPlayer->error = true;
                        nextPlayer->errorCauseString =
                            "Socket write failed";
                        }
                    }

                // EVERYONE gets lineage info for new babies
                if( lineageMessage != NULL ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            lineageMessage, 
                            lineageMessageLength, 
                            false, false );
                    
                    if( numSent != lineageMessageLength ) {
                        setDeathReason( nextPlayer, "disconnected" );

                        nextPlayer->error = true;
                        nextPlayer->errorCauseString =
                            "Socket write failed";
                        }
                    }

                // EVERYONE gets newly-given names
                if( namesMessage != NULL ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            namesMessage, 
                            namesMessageLength, 
                            false, false );
                    
                    if( numSent != namesMessageLength ) {
                        setDeathReason( nextPlayer, "disconnected" );

                        nextPlayer->error = true;
                        nextPlayer->errorCauseString =
                            "Socket write failed";
                        }
                    }

                


                if( nextPlayer->foodUpdate ) {
                    // send this player a food status change
                    
                    int cap = computeFoodCapacity( nextPlayer );
                    
                    if( cap < nextPlayer->foodStore ) {
                        nextPlayer->foodStore = cap;
                        }
                    
                    char *foodMessage = autoSprintf( 
                        "FX\n"
                        "%d %d %d %d %.2f %d\n"
                        "#",
                        nextPlayer->foodStore,
                        cap,
                        nextPlayer->lastAteID,
                        nextPlayer->lastAteFillMax,
                        computeMoveSpeed( nextPlayer ),
                        nextPlayer->responsiblePlayerID );
                     
                    int messageLength = strlen( foodMessage );
                    
                    int numSent = 
                         nextPlayer->sock->send( 
                             (unsigned char*)foodMessage, 
                             messageLength,
                             false, false );

                     if( numSent != messageLength ) {
                         setDeathReason( nextPlayer, "disconnected" );

                         nextPlayer->error = true;
                         nextPlayer->errorCauseString =
                             "Socket write failed";
                         }
                     
                     delete [] foodMessage;
                     
                     nextPlayer->foodUpdate = false;
                     nextPlayer->lastAteID = 0;
                     nextPlayer->lastAteFillMax = 0;
                    }
                }
            }

        if( moveMessage != NULL ) {
            delete [] moveMessage;
            }
        if( updateMessage != NULL ) {
            delete [] updateMessage;
            }
        if( outOfRangeMessage != NULL ) {
            delete [] outOfRangeMessage;
            }
        if( mapChangeMessage != NULL ) {
            delete [] mapChangeMessage;
            }
        if( speechMessage != NULL ) {
            delete [] speechMessage;
            }
        if( deleteUpdateMessage != NULL ) {
            delete [] deleteUpdateMessage;
            }
        if( lineageMessage != NULL ) {
            delete [] lineageMessage;
            }
        if( namesMessage != NULL ) {
            delete [] namesMessage;
            }
        if( dyingMessage != NULL ) {
            delete [] dyingMessage;
            }

        
        // handle closing any that have an error
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement(i);

            if( nextPlayer->error && nextPlayer->deleteSent &&
                nextPlayer->deleteSentDoneETA < Time::getCurrentTime() ) {
                AppLog::infoF( "Closing connection to player %d on error "
                               "(cause: %s)",
                               nextPlayer->id, nextPlayer->errorCauseString );

                AppLog::infoF( "%d remaining player(s) alive on server ",
                               players.size() - 1 );

                
                delete nextPlayer->sock;
                delete nextPlayer->sockBuffer;
                delete nextPlayer->lineage;
                
                if( nextPlayer->name != NULL ) {
                    delete [] nextPlayer->name;
                    }
                
                if( nextPlayer->containedIDs != NULL ) {
                    delete [] nextPlayer->containedIDs;
                    }
                
                if( nextPlayer->containedEtaDecays != NULL ) {
                    delete [] nextPlayer->containedEtaDecays;
                    }

                if( nextPlayer->subContainedIDs != NULL ) {
                    delete [] nextPlayer->subContainedIDs;
                    }
                
                if( nextPlayer->subContainedEtaDecays != NULL ) {
                    delete [] nextPlayer->subContainedEtaDecays;
                    }
                
                if( nextPlayer->pathToDest != NULL ) {
                    delete [] nextPlayer->pathToDest;
                    }

                if( nextPlayer->email != NULL ) {
                    delete [] nextPlayer->email;
                    }

                if( nextPlayer->deathReason != NULL ) {
                    delete [] nextPlayer->deathReason;
                    }
                
                delete nextPlayer->babyBirthTimes;
                delete nextPlayer->babyIDs;
                
                players.deleteElement( i );
                i--;
                }
            }


        if( players.size() == 0 && newConnections.size() == 0 ) {
            if( shutdownMode ) {
                AppLog::info( "No live players or connections in shutdown " 
                              " mode, auto-quitting." );
                quit = true;
                }
            }
        }
    

    quitCleanup();
    
    
    AppLog::info( "Done." );

    return 0;
    }



// implement null versions of these to allow a headless build
// we never call drawObject, but we need to use other objectBank functions


void *getSprite( int ) {
    return NULL;
    }

char markSpriteLive( int ) {
    return false;
    }

void stepSpriteBank() {
    }

void drawSprite( void*, doublePair, double, double, char ) {
    }

void setDrawColor( float inR, float inG, float inB, float inA ) {
    }

void setDrawFade( float ) {
    }

float getTotalGlobalFade() {
    return 1.0f;
    }

void toggleAdditiveTextureColoring( char inAdditive ) {
    }



// dummy implementations of these functions, which are used in editor
// and client, but not server
#include "../gameSource/spriteBank.h"
SpriteRecord *getSpriteRecord( int inSpriteID ) {
    return NULL;
    }

#include "../gameSource/soundBank.h"
void checkIfSoundStillNeeded( int inID ) {
    }



char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset ) {
    return false;
    }


char getUsesMultiplicativeBlending( int inID ) {
    return false;
    }


void toggleMultiplicativeBlend( char inMultiplicative ) {
    }


void countLiveUse( SoundUsage inUsage ) {
    }

void unCountLiveUse( SoundUsage inUsage ) {
    }



// animation bank calls these only if lip sync hack is enabled, which
// it never is for server
void *loadSpriteBase( const char*, char ) {
    return NULL;
    }

void freeSprite( void* ) {
    }

void startOutputAllFrames() {
    }

void stopOutputAllFrames() {
    }

