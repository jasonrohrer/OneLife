
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
#include "../gameSource/objectMetadata.h"
#include "../gameSource/animationBank.h"
#include "../gameSource/categoryBank.h"

#include "lifeLog.h"
#include "foodLog.h"
#include "backup.h"
#include "triggers.h"
#include "playerStats.h"
#include "lineageLog.h"
#include "serverCalls.h"
#include "failureLog.h"
#include "names.h"
#include "curses.h"
#include "lineageLimit.h"


#include "minorGems/util/random/JenkinsRandomSource.h"


//#define IGNORE_PRINTF

#ifdef IGNORE_PRINTF
#define printf(fmt, ...) (0)
#endif


static JenkinsRandomSource randSource;


#include "../gameSource/GridPos.h"


#define HEAT_MAP_D 8

float targetHeat = 10;


double secondsPerYear = 60.0;



#define PERSON_OBJ_ID 12


int minPickupBabyAge = 10;

int babyAge = 5;

// age when bare-hand actions become available to a baby (opening doors, etc.)
int defaultActionAge = 3;


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
char postApocalypseStarted = false;


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
static SimpleVector<char*> cursingPhrases;

static char *eveName = NULL;


// maps extended ascii codes to true/false for characters allowed in SAY
// messages
static char allowedSayCharMap[256];

static const char *allowedSayChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-,'?! ";




// for incoming socket connections that are still in the login process
typedef struct FreshConnection {
        Socket *sock;
        SimpleVector<char> *sockBuffer;

        unsigned int sequenceNumber;
        char *sequenceNumberString;
        
        WebRequest *ticketServerRequest;

        double ticketServerRequestStartTime;
        
        char error;
        const char *errorCauseString;
        
        char shutdownMode;

        // for tracking connections that have failed to LOGIN 
        // in a timely manner
        double connectionStartTimeSeconds;

        char *email;
        
        int tutorialNumber;
        CurseStatus curseStatus;
        
        char *twinCode;
        int twinCount;

    } FreshConnection;


SimpleVector<FreshConnection> newConnections;

SimpleVector<FreshConnection> waitingForTwinConnections;



typedef struct LiveObject {
        char *email;
        
        int id;
        
        // object ID used to visually represent this player
        int displayID;
        
        char *name;
        char nameHasSuffix;
        
        char *lastSay;

        CurseStatus curseStatus;
        
        int curseTokenCount;
        char curseTokenUpdate;


        char isEve;        

        char isTutorial;
        
        // used to track incremental tutorial map loading
        TutorialLoadProgress tutorialLoad;


        GridPos birthPos;

        int parentID;

        // 0 for Eve
        int parentChainLength;

        SimpleVector<int> *lineage;
        
        // id of Eve that started this line
        int lineageEveID;
        


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
        char everHeldByParent;

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
        
        char waitingForForceResponse;
        
        int lastMoveSequenceNumber;
        

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
        

        // track origin of held separate to use when placing a grave
        int heldGraveOriginX;
        int heldGraveOriginY;
        

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

        // who killed them?
        int murderPerpID;
        char *murderPerpEmail;
        
        // or if they were killed by a non-person, what was it?
        int deathSourceID;
        
        // true if this character landed a mortal wound on another player
        char everKilledAnyone;

        // true in case of sudden infant death
        char suicide;
        

        Socket *sock;
        SimpleVector<char> *sockBuffer;
        
        // indicates that some messages were sent to this player this 
        // frame, and they need a FRAME terminator message
        char gotPartOfThisFrame;
        

        char isNew;
        char firstMessageSent;
        
        char inFlight;
        

        char dying;
        // wall clock time when they will be dead
        double dyingETA;

        // in cases where their held wound produces a forced emot
        char emotFrozen;
        
        
        char connected;
        
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

        // net heat of environment around player
        // map is tracked in heat units (each object produces an 
        // integer amount of heat)
        // this is in base heat units, range 0 to infinity
        float envHeat;

        // amount of heat currently in player's body, also in
        // base heat units
        float bodyHeat;
        

        // used track current biome heat for biome-change shock effects
        float biomeHeat;
        float lastBiomeHeat;


        // body heat normalized to [0,1], with targetHeat at 0.5
        float heat;
        
        // flags this player as needing to recieve a heat update
        char heatUpdate;
        
        // wall clock time of last time this player was sent
        // a heat update
        double lastHeatUpdate;


        int foodStore;
        
        double foodCapModifier;

        double fever;
        

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
        
        // chain of non-repeating foods eaten
        SimpleVector<int> yummyFoodChain;
        
        // how many bonus from yummy food is stored
        // these are used first before food is decremented
        int yummyBonusStore;
        

        ClothingSet clothing;
        
        timeSec_t clothingEtaDecay[NUM_CLOTHING_PIECES];

        SimpleVector<int> clothingContained[NUM_CLOTHING_PIECES];
        
        SimpleVector<timeSec_t> 
            clothingContainedEtaDecays[NUM_CLOTHING_PIECES];

        char needsUpdate;
        char updateSent;
        char updateGlobal;
        
        // babies born to this player
        SimpleVector<timeSec_t> *babyBirthTimes;
        SimpleVector<int> *babyIDs;
        
        // wall clock time after which they can have another baby
        // starts at 0 (start of time epoch) for non-mothers, as
        // they can have their first baby right away.
        timeSec_t birthCoolDown;
        

        timeSec_t lastRegionLookTime;
        
        double playerCrossingCheckTime;
        

        char monumentPosSet;
        GridPos lastMonumentPos;
        int lastMonumentID;
        char monumentPosSent;
        

        char holdingFlightObject;

    } LiveObject;



SimpleVector<LiveObject> players;
SimpleVector<LiveObject> tutorialLoadingPlayers;



static char checkReadOnly() {
    const char *testFileName = "testReadOnly.txt";
    
    FILE *testFile = fopen( testFileName, "w" );
    
    if( testFile != NULL ) {
        
        fclose( testFile );
        remove( testFileName );
        return false;
        }
    return true;
    }




// returns a person to their natural state
static void backToBasics( LiveObject *inPlayer ) {
    LiveObject *p = inPlayer;

    // do not heal dying people
    if( ! p->holdingWound && p->holdingID > 0 ) {
        
        p->holdingID = 0;
        
        p->holdingEtaDecay = 0;
        
        p->heldOriginValid = false;
        p->heldTransitionSourceID = -1;
        
        
        p->numContained = 0;
        if( p->containedIDs != NULL ) {
            delete [] p->containedIDs;
            delete [] p->containedEtaDecays;
            p->containedIDs = NULL;
        p->containedEtaDecays = NULL;
            }
        
        if( p->subContainedIDs != NULL ) {
            delete [] p->subContainedIDs;
            delete [] p->subContainedEtaDecays;
            p->subContainedIDs = NULL;
            p->subContainedEtaDecays = NULL;
            }
        }
        
        
    p->clothing = getEmptyClothingSet();
    
    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        p->clothingEtaDecay[c] = 0;
        p->clothingContained[c].deleteAll();
        p->clothingContainedEtaDecays[c].deleteAll();
        }
    }




typedef struct GraveInfo {
        GridPos pos;
        int playerID;
    } GraveInfo;


typedef struct GraveMoveInfo {
        GridPos posStart;
        GridPos posEnd;
    } GraveMoveInfo;




// tracking spots on map that inflicted a mortal wound
// put them on timeout afterward so that they don't attack
// again immediately
typedef struct DeadlyMapSpot {
        GridPos pos;
        double timeOfAttack;
    } DeadlyMapSpot;


static double deadlyMapSpotTimeoutSec = 10;

static SimpleVector<DeadlyMapSpot> deadlyMapSpots;


static char wasRecentlyDeadly( GridPos inPos ) {
    double curTime = Time::getCurrentTime();
    
    for( int i=0; i<deadlyMapSpots.size(); i++ ) {
        
        DeadlyMapSpot *s = deadlyMapSpots.getElement( i );
        
        if( curTime - s->timeOfAttack >= deadlyMapSpotTimeoutSec ) {
            deadlyMapSpots.deleteElement( i );
            i--;
            }
        else if( s->pos.x == inPos.x && s->pos.y == inPos.y ) {
            // note that this is a lazy method that only walks through
            // the whole list and checks for timeouts when
            // inPos isn't found
            return true;
            }
        }
    return false;
    }



static void addDeadlyMapSpot( GridPos inPos ) {
    // don't check for duplicates
    // we're only called to add a new deadly spot when the spot isn't
    // currently on deadly cooldown anyway
    DeadlyMapSpot s = { inPos, Time::getCurrentTime() };
    deadlyMapSpots.push_back( s );
    }




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



void freePlayerContainedArrays( LiveObject *inPlayer ) {
    if( inPlayer->containedIDs != NULL ) {
        delete [] inPlayer->containedIDs;
        }
    if( inPlayer->containedEtaDecays != NULL ) {
        delete [] inPlayer->containedEtaDecays;
        }
    if( inPlayer->subContainedIDs != NULL ) {
        delete [] inPlayer->subContainedIDs;
        }
    if( inPlayer->subContainedEtaDecays != NULL ) {
        delete [] inPlayer->subContainedEtaDecays;
        }

    inPlayer->containedIDs = NULL;
    inPlayer->containedEtaDecays = NULL;
    inPlayer->subContainedIDs = NULL;
    inPlayer->subContainedEtaDecays = NULL;
    }



void setContained( LiveObject *inPlayer, FullMapContained inContained ) {
    
    inPlayer->numContained = inContained.numContained;
     
    freePlayerContainedArrays( inPlayer );
    
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




static double getPathSpeedModifier( GridPos *inPathPos, int inPathLength ) {
    
    if( inPathLength < 1 ) {
        return 1;
        }
    

    int floor = getMapFloor( inPathPos[0].x, inPathPos[0].y );

    if( floor == 0 ) {
        return 1;
        }

    double speedMult = getObject( floor )->speedMult;
    
    if( speedMult == 1 ) {
        return 1;
        }
    

    // else we have a speed mult for at least first step in path
    // see if we have same floor for rest of path

    for( int i=1; i<inPathLength; i++ ) {
        
        int thisFloor = getMapFloor( inPathPos[i].x, inPathPos[i].y );
        
        if( thisFloor != floor ) {
            // not same floor whole way
            return 1;
            }
        }
    // same floor whole way
    printf( "Speed modifier = %f\n", speedMult );
    return speedMult;
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


static void deleteMembers( FreshConnection *inConnection ) {
    delete inConnection->sock;
    delete inConnection->sockBuffer;
    
    if( inConnection->sequenceNumberString != NULL ) {    
        delete [] inConnection->sequenceNumberString;
        }
    
    if( inConnection->ticketServerRequest != NULL ) {
        delete inConnection->ticketServerRequest;
        }
    
    if( inConnection->email != NULL ) {
        delete [] inConnection->email;
        }
    
    if( inConnection->twinCode != NULL ) {
        delete [] inConnection->twinCode;
        }
    }




void quitCleanup() {
    AppLog::info( "Cleaning up on quit..." );

    // FreshConnections are in two different lists
    // free structures from both
    SimpleVector<FreshConnection> *connectionLists[2] =
        { &newConnections, &waitingForTwinConnections };

    for( int c=0; c<2; c++ ) {
        SimpleVector<FreshConnection> *list = connectionLists[c];
        
        for( int i=0; i<list->size(); i++ ) {
            FreshConnection *nextConnection = list->getElement( i );
            deleteMembers( nextConnection );
            }
        list->deleteAll();
        }
    
    // add these to players to clean them up togeter
    for( int i=0; i<tutorialLoadingPlayers.size(); i++ ) {
        LiveObject nextPlayer = tutorialLoadingPlayers.getElementDirect( i );
        players.push_back( nextPlayer );
        }
    tutorialLoadingPlayers.deleteAll();
    


    for( int i=0; i<players.size(); i++ ) {
        LiveObject *nextPlayer = players.getElement(i);

        if( nextPlayer->sock != NULL ) {
            delete nextPlayer->sock;
            nextPlayer->sock = NULL;
            }
        if( nextPlayer->sockBuffer != NULL ) {
            delete nextPlayer->sockBuffer;
            nextPlayer->sockBuffer = NULL;
            }

        delete nextPlayer->lineage;

        if( nextPlayer->name != NULL ) {
            delete [] nextPlayer->name;
            }

        if( nextPlayer->lastSay != NULL ) {
            delete [] nextPlayer->lastSay;
            }
        
        if( nextPlayer->email != NULL  ) {
            delete [] nextPlayer->email;
            }


        freePlayerContainedArrays( nextPlayer );
        
        
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


    freeLineageLimit();
    
    freePlayerStats();
    freeLineageLog();
    
    freeNames();
    
    freeCurses();
    
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
    cursingPhrases.deallocateStringElements();

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

        if( ! inSock->isSocketInFDRange() ) {
            // the internal FD of this socket is out of range
            // probably some kind of heap corruption.

            // save a bug report
            int allow = 
                SettingsManager::getIntSetting( "allowBugReports", 0 );

            if( allow ) {
                char *bugName = 
                    autoSprintf( "bug_socket_%f", Time::getCurrentTime() );
                
                char *bugOutName = autoSprintf( "%s_out.txt", bugName );
                
                File outFile( NULL, "serverOut.txt" );
                if( outFile.exists() ) {
                    fflush( stdout );
                    File outCopyFile( NULL, bugOutName );
                    
                    outFile.copy( &outCopyFile );
                    }
                delete [] bugName;
                delete [] bugOutName;
                }
            }
        
            
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

        if( inBuffer->size() > 200 ) {
            // 200 characters with no message terminator?
            // client is sending us nonsense
            // cut it off here to avoid buffer overflow
            
            AppLog::info( "More than 200 characters in client receive buffer "
                          "with no messsage terminator present, "
                          "generating NONSENSE message." );
            
            return stringDuplicate( "NONSENSE 0 0" );
            }

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
    EMOT,
    JUMP,
    DIE,
    FORCE,
    MAP,
    TRIGGER,
    BUG,
    PING,
    UNKNOWN
    } messageType;




typedef struct ClientMessage {
        messageType type;
        int x, y, c, i, id;
        
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

        // for MOVE messages
        int sequenceNumber;

    } ClientMessage;


static int pathDeltaMax = 16;


// if extraPos present in result, destroyed by caller
// inMessage may be modified by this call
ClientMessage parseMessage( LiveObject *inPlayer, char *inMessage ) {
    
    char nameBuffer[100];
    
    ClientMessage m;
    
    m.i = -1;
    m.c = -1;
    m.id = -1;
    m.trigger = -1;
    m.numExtraPos = 0;
    m.extraPos = NULL;
    m.saidText = NULL;
    m.bugText = NULL;
    m.sequenceNumber = -1;
    
    // don't require # terminator here
    
    
    //int numRead = sscanf( inMessage, 
    //                      "%99s %d %d", nameBuffer, &( m.x ), &( m.y ) );
    

    // profiler finds sscanf as a hotspot
    // try a custom bit of code instead
    
    int numRead = 0;
    
    int parseLen = strlen( inMessage );
    if( parseLen > 99 ) {
        parseLen = 99;
        }
    
    for( int i=0; i<parseLen; i++ ) {
        if( inMessage[i] == ' ' ) {
            switch( numRead ) {
                case 0:
                    if( i != 0 ) {
                        memcpy( nameBuffer, inMessage, i );
                        nameBuffer[i] = '\0';
                        numRead++;
                        // rewind back to read the space again
                        // before the first number
                        i--;
                        }
                    break;
                case 1:
                    m.x = atoi( &( inMessage[i] ) );
                    numRead++;
                    break;
                case 2:
                    m.y = atoi( &( inMessage[i] ) );
                    numRead++;
                    break;
                }
            if( numRead == 3 ) {
                break;
                }
            }
        }
    

    
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
        
        char *atPos = strstr( inMessage, "@" );
        
        int offset = 3;
        
        if( atPos != NULL ) {
            offset = 4;            
            }
        

        // in place, so we don't need to deallocate them
        SimpleVector<char *> *tokens =
            tokenizeStringInPlace( inMessage );
        
        // require an even number of extra coords beyond offset
        if( tokens->size() < offset + 2 || 
            ( tokens->size() - offset ) %2 != 0 ) {
            
            delete tokens;
            
            m.type = UNKNOWN;
            return m;
            }
        
        if( atPos != NULL ) {
            // skip @ symbol in token and parse int
            m.sequenceNumber = atoi( &( tokens->getElementDirect( 3 )[1] ) );
            }

        int numTokens = tokens->size();
        
        m.numExtraPos = (numTokens - offset) / 2;
        
        m.extraPos = new GridPos[ m.numExtraPos ];

        for( int e=0; e<m.numExtraPos; e++ ) {
            
            char *xToken = tokens->getElementDirect( offset + e * 2 );
            char *yToken = tokens->getElementDirect( offset + e * 2 + 1 );
            
            // profiler found sscanf is a bottleneck here
            // try atoi instead
            //sscanf( xToken, "%d", &( m.extraPos[e].x ) );
            //sscanf( yToken, "%d", &( m.extraPos[e].y ) );

            m.extraPos[e].x = atoi( xToken );
            m.extraPos[e].y = atoi( yToken );
            
            
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
        
        delete tokens;
        }
    else if( strcmp( nameBuffer, "JUMP" ) == 0 ) {
        m.type = JUMP;
        }
    else if( strcmp( nameBuffer, "DIE" ) == 0 ) {
        m.type = DIE;
        }
    else if( strcmp( nameBuffer, "FORCE" ) == 0 ) {
        m.type = FORCE;
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

        // id param optional
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ), &( m.id ) );
        
        if( numRead != 4 && numRead != 5 ) {
            m.type = UNKNOWN;
            }
        if( numRead != 5 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "BABY" ) == 0 ) {
        m.type = BABY;
        // read optional id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = -1;
            }
        }
    else if( strcmp( nameBuffer, "PING" ) == 0 ) {
        m.type = PING;
        // read unique id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = 0;
            }
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
        
        // read optional id parameter
        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.id ) );
        
        if( numRead != 4 ) {
            m.id = -1;
            }
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
    else if( strcmp( nameBuffer, "EMOT" ) == 0 ) {
        m.type = EMOT;

        numRead = sscanf( inMessage, 
                          "%99s %d %d %d", 
                          nameBuffer, &( m.x ), &( m.y ), &( m.i ) );
        
        if( numRead != 4 ) {
            m.type = UNKNOWN;
            }
        }
    else {
        m.type = UNKNOWN;
        }
    
    // incoming client messages are relative to birth pos
    m.x += inPlayer->birthPos.x;
    m.y += inPlayer->birthPos.y;

    for( int i=0; i<m.numExtraPos; i++ ) {
        m.extraPos[i].x += inPlayer->birthPos.x;
        m.extraPos[i].y += inPlayer->birthPos.y;
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



GridPos getPlayerPos( LiveObject *inPlayer ) {
    if( inPlayer->xs == inPlayer->xd &&
        inPlayer->ys == inPlayer->yd ) {
        
        GridPos cPos = { inPlayer->xs, inPlayer->ys };
        
        return cPos;
        }
    else {
        return computePartialMoveSpot( inPlayer );
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
    return 1.0 / secondsPerYear;
    }


static void setDeathReason( LiveObject *inPlayer, const char *inTag,
                            int inOptionalID = 0 ) {
    
    if( inPlayer->deathReason != NULL ) {
        delete [] inPlayer->deathReason;
        }
    
    // leave space in front so it works at end of PU line
    if( strcmp( inTag, "killed" ) == 0 ||
        strcmp( inTag, "succumbed" ) == 0 ) {
        
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
    if( inPlayer->curseStatus.curseLevel == 0 &&
        inPlayer->parentChainLength > longestShutdownLine ) {
        
        // never count a cursed player as a long line
        
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
    
    int returnVal = 0;
    
    if( ageInYears < 44 ) {
        
        if( ageInYears > 16 ) {
            ageInYears = 16;
            }
        
        returnVal = ageInYears + 4;
        }
    else {
        // food capacity decreases as we near 60
        int cap = 60 - ageInYears + 4;
        
        if( cap < 4 ) {
            cap = 4;
            }
        
        returnVal = cap;
        }

    return ceil( returnVal * inPlayer->foodCapModifier );
    }



// with 128-wide tiles, character moves at 480 pixels per second
// at 60 fps, this is 8 pixels per frame
// important that it's a whole number of pixels for smooth camera movement
static double baseWalkSpeed = 3.75;

// min speed for takeoff
static double minFlightSpeed = 15;



double computeMoveSpeed( LiveObject *inPlayer ) {
    double age = computeAge( inPlayer );
    

    double speed = baseWalkSpeed;
    
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

    // never move at 0 speed, divide by 0 errors for eta times
    if( speed < 0.01 ) {
        speed = 0.01;
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



static double distance( GridPos inA, GridPos inB ) {
    double dx = (double)inA.x - (double)inB.x;
    double dy = (double)inA.y - (double)inB.y;

    return sqrt(  dx * dx + dy * dy );
    }



static float sign( float inF ) {
    if (inF > 0) return 1;
    if (inF < 0) return -1;
    return 0;
    }


// how often do we check what a player is standing on top of for attack effects?
static double playerCrossingCheckStepTime = 0.25;


// for steps in main loop that shouldn't happen every loop
// (loop runs faster or slower depending on how many messages are incoming)
static double periodicStepTime = 0.25;
static double lastPeriodicStepTime = 0;




// recompute heat for fixed number of players per timestep
static int numPlayersRecomputeHeatPerStep = 8;
static int lastPlayerIndexHeatRecomputed = -1;
static double lastHeatUpdateTime = 0;
static double heatUpdateTimeStep = 0.1;


// how often the player's personal heat advances toward their environmental
// heat value
static double heatUpdateSeconds = 2;


// air itself offers some insulation
// a vacuum panel has R-value that is 25x greater than air
static float rAir = 0.04;



// blend R-values multiplicatively, for layers
// 1 - R( A + B ) = (1 - R(A)) * (1 - R(B))
//
// or
//
//R( A + B ) =  R(A) + R(B) - R(A) * R(B)
static double rCombine( double inRA, double inRB ) {
    return inRA + inRB - inRA * inRB;
    }




static float computeClothingR( LiveObject *inPlayer ) {
    
    float headWeight = 0.25;
    float chestWeight = 0.35;
    float buttWeight = 0.2;
    float eachFootWeigth = 0.1;
            
    float backWeight = 0.1;


    float clothingR = 0;
            
    if( inPlayer->clothing.hat != NULL ) {
        clothingR += headWeight *  inPlayer->clothing.hat->rValue;
        }
    if( inPlayer->clothing.tunic != NULL ) {
        clothingR += chestWeight * inPlayer->clothing.tunic->rValue;
        }
    if( inPlayer->clothing.frontShoe != NULL ) {
        clothingR += 
            eachFootWeigth * inPlayer->clothing.frontShoe->rValue;
        }
    if( inPlayer->clothing.backShoe != NULL ) {
        clothingR += eachFootWeigth * 
            inPlayer->clothing.backShoe->rValue;
        }
    if( inPlayer->clothing.bottom != NULL ) {
        clothingR += buttWeight * inPlayer->clothing.bottom->rValue;
        }
    if( inPlayer->clothing.backpack != NULL ) {
        clothingR += backWeight * inPlayer->clothing.backpack->rValue;
        }
    
    // even if the player is naked, they are insulated from their
    // environment by rAir
    return rCombine( rAir, clothingR );
    }



static float computeClothingHeat( LiveObject *inPlayer ) {
    // clothing can contribute heat
    // apply this separate from heat grid above
    float clothingHeat = 0;
    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                
        ObjectRecord *cO = clothingByIndex( inPlayer->clothing, c );
            
        if( cO != NULL ) {
            clothingHeat += cO->heatValue;

            // contained items in clothing can contribute
            // heat, shielded by clothing r-values
            double cRFactor = 1 - cO->rValue;

            for( int s=0; 
                 s < inPlayer->clothingContained[c].size(); s++ ) {
                        
                ObjectRecord *sO = 
                    getObject( inPlayer->clothingContained[c].
                               getElementDirect( s ) );
                        
                clothingHeat += 
                    sO->heatValue * cRFactor;
                }
            }
        }
    return clothingHeat;
    }




static void recomputeHeatMap( LiveObject *inPlayer ) {
    
    int gridSize = HEAT_MAP_D * HEAT_MAP_D;

    // what if we recompute it from scratch every time?
    for( int i=0; i<gridSize; i++ ) {
        inPlayer->heatMap[i] = 0;
        }

    float heatOutputGrid[ HEAT_MAP_D * HEAT_MAP_D ];
    float rGrid[ HEAT_MAP_D * HEAT_MAP_D ];
    float rFloorGrid[ HEAT_MAP_D * HEAT_MAP_D ];


    GridPos pos = getPlayerPos( inPlayer );


    // held baby's pos matches parent pos
    if( inPlayer->heldByOther ) {
        LiveObject *parentObject = getLiveObject( inPlayer->heldByOtherID );
        
        if( parentObject != NULL ) {
            pos = getPlayerPos( parentObject );
            }
        } 

    
    

    for( int y=0; y<HEAT_MAP_D; y++ ) {
        int mapY = pos.y + y - HEAT_MAP_D / 2;
                
        for( int x=0; x<HEAT_MAP_D; x++ ) {
                    
            int mapX = pos.x + x - HEAT_MAP_D / 2;
                    
            int j = y * HEAT_MAP_D + x;
            heatOutputGrid[j] = 0;
            rGrid[j] = rAir;
            rFloorGrid[j] = rAir;


            // call Raw version for better performance
            // we don't care if object decayed since we last looked at it
            ObjectRecord *o = getObject( getMapObjectRaw( mapX, mapY ) );
                    
                    
                    

            if( o != NULL ) {
                heatOutputGrid[j] += o->heatValue;
                if( o->permanent ) {
                    // loose objects sitting on ground don't
                    // contribute to r-value (like dropped clothing)
                    rGrid[j] = rCombine( rGrid[j], o->rValue );
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
                rFloorGrid[j] = rCombine( rFloorGrid[j], fO->rValue );
                }
            }
        }


    
    int numNeighbors = 8;
    int ndx[8] = { 0, 1,  0, -1,  1,  1, -1, -1 };
    int ndy[8] = { 1, 0, -1,  0,  1, -1,  1, -1 };
    
            
    int playerMapIndex = 
        ( HEAT_MAP_D / 2 ) * HEAT_MAP_D +
        ( HEAT_MAP_D / 2 );
        

    
    // what player is holding can contribute heat
    // add this to the grid, since it's "outside" the player's body
    if( inPlayer->holdingID > 0 ) {
        ObjectRecord *heldO = getObject( inPlayer->holdingID );
                
        heatOutputGrid[ playerMapIndex ] += heldO->heatValue;
                
        double heldRFactor = 1 - heldO->rValue;
                
        // contained can contribute too, but shielded by r-value
        // of container
        for( int c=0; c<inPlayer->numContained; c++ ) {
                    
            int cID = inPlayer->containedIDs[c];
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
                     s<inPlayer->subContainedIDs[c].size(); s++ ) {
                        
                    ObjectRecord *subO =
                        getObject( inPlayer->subContainedIDs[c].
                                   getElementDirect( s ) );
                            
                    heatOutputGrid[ playerMapIndex ] += 
                        subO->heatValue * 
                        contRFactor * heldRFactor;
                    }
                }
            }
        }
            
            

    // grid of flags for points that are in same airspace (surrounded by walls)
    // as player
    // This is the area where heat spreads evenly by convection
    char airSpaceGrid[ HEAT_MAP_D * HEAT_MAP_D ];
    
    memset( airSpaceGrid, false, HEAT_MAP_D * HEAT_MAP_D );
    
    airSpaceGrid[ playerMapIndex ] = true;

    SimpleVector<int> frontierA;
    SimpleVector<int> frontierB;
    frontierA.push_back( playerMapIndex );
    
    SimpleVector<int> *thisFrontier = &frontierA;
    SimpleVector<int> *nextFrontier = &frontierB;

    while( thisFrontier->size() > 0 ) {

        for( int f=0; f<thisFrontier->size(); f++ ) {
            
            int i = thisFrontier->getElementDirect( f );
            
            char negativeYCutoff = false;
            
            if( rGrid[i] > rAir ) {
                // grid cell is insulating, and somehow it's in our
                // frontier.  Player must be standing behind a closed
                // door.  Block neighbors to south
                negativeYCutoff = true;
                }
            

            int x = i % HEAT_MAP_D;
            int y = i / HEAT_MAP_D;
            
            for( int n=0; n<numNeighbors; n++ ) {
                        
                int nx = x + ndx[n];
                int ny = y + ndy[n];
                
                if( negativeYCutoff && ndy[n] < 1 ) {
                    continue;
                    }

                if( nx >= 0 && nx < HEAT_MAP_D &&
                    ny >= 0 && ny < HEAT_MAP_D ) {

                    int nj = ny * HEAT_MAP_D + nx;
                    
                    if( ! airSpaceGrid[ nj ]
                        && rGrid[ nj ] <= rAir ) {

                        airSpaceGrid[ nj ] = true;
                        
                        nextFrontier->push_back( nj );
                        }
                    }
                }
            }
        
        thisFrontier->deleteAll();
        
        SimpleVector<int> *temp = thisFrontier;
        thisFrontier = nextFrontier;
        
        nextFrontier = temp;
        }
    
    if( rGrid[playerMapIndex] > rAir ) {
        // player standing in insulated spot
        // don't count this as part of their air space
        airSpaceGrid[ playerMapIndex ] = false;
        }

    int numInAirspace = 0;
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[ i ] ) {
            numInAirspace++;
            }
        }
    
    
    float rBoundarySum = 0;
    int rBoundarySize = 0;
    
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[i] ) {
            
            int x = i % HEAT_MAP_D;
            int y = i / HEAT_MAP_D;
            
            for( int n=0; n<numNeighbors; n++ ) {
                        
                int nx = x + ndx[n];
                int ny = y + ndy[n];
                
                if( nx >= 0 && nx < HEAT_MAP_D &&
                    ny >= 0 && ny < HEAT_MAP_D ) {
                    
                    int nj = ny * HEAT_MAP_D + nx;
                    
                    if( ! airSpaceGrid[ nj ] ) {
                        
                        // boundary!
                        rBoundarySum += rGrid[ nj ];
                        rBoundarySize ++;
                        }
                    }
                else {
                    // boundary is off edge
                    // assume air R-value
                    rBoundarySum += rAir;
                    rBoundarySize ++;
                    }
                }
            }
        }

    
    // floor counts as boundary too
    // 4x its effect (seems more important than one of 8 walls
    
    // count non-air floor tiles while we're at it
    int numFloorTilesInAirspace = 0;

    if( numInAirspace > 0 ) {
        for( int i=0; i<gridSize; i++ ) {
            if( airSpaceGrid[i] ) {
                rBoundarySum += 4 * rFloorGrid[i];
                rBoundarySize += 4;
                
                if( rFloorGrid[i] > rAir ) {
                    numFloorTilesInAirspace++;
                    }
                }
            }
        }
    


    float rBoundaryAverage = rAir;
    
    if( rBoundarySize > 0 ) {
        rBoundaryAverage = rBoundarySum / rBoundarySize;
        }

    
    



    float airSpaceHeatSum = 0;
    
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[i] ) {
            airSpaceHeatSum += heatOutputGrid[i];
            }
        }


    float airSpaceHeatVal = 0;
    
    if( numInAirspace > 0 ) {
        // spread heat evenly over airspace
        airSpaceHeatVal = airSpaceHeatSum / numInAirspace;
        }

    float containedAirSpaceHeatVal = airSpaceHeatVal * rBoundaryAverage;
    


    float radiantAirSpaceHeatVal = 0;

    GridPos playerHeatMapPos = { playerMapIndex % HEAT_MAP_D, 
                                 playerMapIndex / HEAT_MAP_D };
    

    int numRadiantHeatSources = 0;
    
    for( int i=0; i<gridSize; i++ ) {
        if( airSpaceGrid[i] && heatOutputGrid[i] > 0 ) {
            
            int x = i % HEAT_MAP_D;
            int y = i / HEAT_MAP_D;
            
            GridPos heatPos = { x, y };
            

            double d = distance( playerHeatMapPos, heatPos );
            
            // avoid infinite heat when player standing on source

            radiantAirSpaceHeatVal += heatOutputGrid[ i ] / ( 1.5 * d + 1 );
            numRadiantHeatSources ++;
            }
        }
    

    float biomeHeatWeight = 1;
    float radiantHeatWeight = 1;
    
    float containedHeatWeight = 4;


    // boundary r-value also limits affect of biome heat on player's
    // environment... keeps biome "out"
    float boundaryLeak = 1 - rBoundaryAverage;

    if( numFloorTilesInAirspace != numInAirspace ) {
        // biome heat invades airspace if entire thing isn't covered by
        // a floor (not really indoors)
        boundaryLeak = 1;
        }


    // a hot biome only pulls us up above perfect
    // (hot biome leaking into a building can never make the building
    //  just right).
    // Enclosed walls can make a hot biome not as hot, but never cool
    float biomeHeat = getBiomeHeatValue( getMapBiome( pos.x, pos.y ) );
    
    if( biomeHeat > targetHeat ) {
        biomeHeat = boundaryLeak * (biomeHeat - targetHeat) + targetHeat;
        }
    else if( biomeHeat < 0 ) {
        // a cold biome's coldness is modulated directly by walls, however
        biomeHeat = boundaryLeak * biomeHeat;
        }
    
    // small offset to ensure that naked-on-green biome the same
    // in new heat model as old
    float constHeatValue = 1.1;

    inPlayer->envHeat = 
        radiantHeatWeight * radiantAirSpaceHeatVal + 
        containedHeatWeight * containedAirSpaceHeatVal +
        biomeHeatWeight * biomeHeat +
        constHeatValue;

    inPlayer->biomeHeat = biomeHeat + constHeatValue;
    }




typedef struct MoveRecord {
    char *formatString;
    int absoluteX, absoluteY;
    } MoveRecord;



// formatString in returned record destroyed by caller
MoveRecord getMoveRecord( LiveObject *inPlayer,
                          char inNewMovesOnly,
                          SimpleVector<ChangePosition> *inChangeVector = 
                          NULL ) {

    MoveRecord r;
    
    // p_id xs ys xd yd fraction_done eta_sec
    
    double deltaSec = Time::getCurrentTime() - inPlayer->moveStartTime;
    
    double etaSec = inPlayer->moveTotalSeconds - deltaSec;
    
    if( etaSec < 0 ) {
        etaSec = 0;
        }

    
    r.absoluteX = inPlayer->xs;
    r.absoluteY = inPlayer->ys;
            
            
    SimpleVector<char> messageLineBuffer;
    
    // start is absolute
    char *startString = autoSprintf( "%d %%d %%d %.3f %.3f %d", 
                                     inPlayer->id, 
                                     inPlayer->moveTotalSeconds, etaSec,
                                     inPlayer->pathTruncated );
    // mark that this has been sent
    inPlayer->pathTruncated = false;

    if( inNewMovesOnly ) {
        inPlayer->newMove = false;
        }

            
    messageLineBuffer.appendElementString( startString );
    delete [] startString;
            
    for( int p=0; p<inPlayer->pathLength; p++ ) {
                // rest are relative to start
        char *stepString = autoSprintf( " %d %d", 
                                        inPlayer->pathToDest[p].x
                                        - inPlayer->xs,
                                        inPlayer->pathToDest[p].y
                                        - inPlayer->ys );
        
        messageLineBuffer.appendElementString( stepString );
        delete [] stepString;
        }
    
    messageLineBuffer.appendElementString( "\n" );
    
    r.formatString = messageLineBuffer.getElementString();    
    
    if( inChangeVector != NULL ) {
        ChangePosition p = { inPlayer->xd, inPlayer->yd, false };
        inChangeVector->push_back( p );
        }

    return r;
    }



SimpleVector<MoveRecord> getMoveRecords( 
    char inNewMovesOnly,
    SimpleVector<ChangePosition> *inChangeVector = NULL ) {
    
    SimpleVector<MoveRecord> v;
    
    int numPlayers = players.size();
                
    for( int i=0; i<numPlayers; i++ ) {
                
        LiveObject *o = players.getElement( i );                
        
        if( o->error ) {
            continue;
            }

        if( ( o->xd != o->xs || o->yd != o->ys )
            &&
            ( o->newMove || !inNewMovesOnly ) ) {
            
 
            MoveRecord r = getMoveRecord( o, inNewMovesOnly, inChangeVector );
            
            v.push_back( r );
            }
        }

    return v;
    }



char *getMovesMessageFromList( SimpleVector<MoveRecord> *inMoves,
                               GridPos inRelativeToPos ) {

    int numLines = 0;
    
    SimpleVector<char> messageBuffer;

    messageBuffer.appendElementString( "PM\n" );

    for( int i=0; i<inMoves->size(); i++ ) {
        MoveRecord r = inMoves->getElementDirect(i);
        
        char *line = autoSprintf( r.formatString, 
                                  r.absoluteX - inRelativeToPos.x,
                                  r.absoluteY - inRelativeToPos.y );
        
        messageBuffer.appendElementString( line );
        delete [] line;
        
        numLines ++;
        }
    
    if( numLines > 0 ) {
        
        messageBuffer.push_back( '#' );
                
        char *message = messageBuffer.getElementString();
        
        return message;
        }
    
    return NULL;
    }

    
    
    
// returns NULL if there are no matching moves
// positions in moves relative to inRelativeToPos
char *getMovesMessage( char inNewMovesOnly,
                       GridPos inRelativeToPos,
                       SimpleVector<ChangePosition> *inChangeVector = NULL ) {
    
    
    SimpleVector<MoveRecord> v = getMoveRecords( inNewMovesOnly, 
                                                 inChangeVector );
    
    char *message = getMovesMessageFromList( &v, inRelativeToPos );
    
    for( int i=0; i<v.size(); i++ ) {
        delete [] v.getElement(i)->formatString;
        }
    
    return message;
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


static SocketPoll sockPoll;



static void setPlayerDisconnected( LiveObject *inPlayer, 
                                   const char *inReason ) {    
    /*
    setDeathReason( inPlayer, "disconnected" );
    
    inPlayer->error = true;
    inPlayer->errorCauseString = inReason;
    */
    // don't kill them
    
    // just mark them as not connected

    AppLog::infoF( "Player %d (%s) marked as disconnected (%s).",
                   inPlayer->id, inPlayer->email, inReason );
    inPlayer->connected = false;

    // when player reconnects, they won't get a force PU message
    // so we shouldn't be waiting for them to ack
    inPlayer->waitingForForceResponse = false;

    
    
    if( inPlayer->sock != NULL ) {
        // also, stop polling their socket, which will trigger constant
        // socket events from here on out, and cause us to busy-loop
        sockPoll.removeSocket( inPlayer->sock );

        delete inPlayer->sock;
        inPlayer->sock = NULL;
        }
    if( inPlayer->sockBuffer != NULL ) {
        delete inPlayer->sockBuffer;
        inPlayer->sockBuffer = NULL;
        }
    }



// sets lastSentMap in inO if chunk goes through
// returns result of send, auto-marks error in inO
int sendMapChunkMessage( LiveObject *inO, 
                         char inDestOverride = false,
                         int inDestOverrideX = 0, 
                         int inDestOverrideY = 0 ) {
    
    if( ! inO->connected ) {
        // act like it was a successful send so we can move on until
        // they reconnect later
        return 1;
        }
    
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
                                                          inO->birthPos,
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
                                                              inO->birthPos,
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
                                                              inO->birthPos,
                                                              &len );
            messageLength += len;
            
            numSent += 
                inO->sock->send( mapChunkMessage, 
                                 len, 
                                 false, false );
            
            delete [] mapChunkMessage;
            }
        }
    
    
    inO->gotPartOfThisFrame = true;
                

    if( numSent == messageLength ) {
        // sent correctly
        inO->lastSentMapX = xd;
        inO->lastSentMapY = yd;
        }
    else {
        setPlayerDisconnected( inO, "Socket write failed" );
        }
    return numSent;
    }



double intDist( int inXA, int inYA, int inXB, int inYB ) {
    double dx = (double)inXA - (double)inXB;
    double dy = (double)inYA - (double)inYB;

    return sqrt(  dx * dx + dy * dy );
    }




char *getHoldingString( LiveObject *inObject ) {
    
    int holdingID = hideIDForClient( inObject->holdingID );    


    if( inObject->numContained == 0 ) {
        return autoSprintf( "%d", holdingID );
        }

    
    SimpleVector<char> buffer;
    

    char *idString = autoSprintf( "%d", holdingID );
    
    buffer.appendElementString( idString );
    
    delete [] idString;
    
    
    if( inObject->numContained > 0 ) {
        for( int i=0; i<inObject->numContained; i++ ) {
            
            char *idString = autoSprintf( 
                ",%d", 
                hideIDForClient( abs( inObject->containedIDs[i] ) ) );
    
            buffer.appendElementString( idString );
    
            delete [] idString;

            if( inObject->subContainedIDs[i].size() > 0 ) {
                for( int s=0; s<inObject->subContainedIDs[i].size(); s++ ) {
                    
                    idString = autoSprintf( 
                        ":%d", 
                        hideIDForClient( 
                            inObject->subContainedIDs[i].
                            getElementDirect( s ) ) );
    
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
    TransRecord *newDecayT = getMetaTrans( -1, inPlayer->holdingID );
                    
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
                            
                        double moveSpeed = computeMoveSpeed( otherPlayer ) *
                            getPathSpeedModifier( otherPlayer->pathToDest,
                                                  otherPlayer->pathLength );

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



#include "spiral.h"

GridPos findClosestEmptyMapSpot( int inX, int inY, int inMaxPointsToCheck,
                                 char *outFound ) {

    GridPos center = { inX, inY };

    for( int i=0; i<inMaxPointsToCheck; i++ ) {
        GridPos p = getSpriralPoint( center, i );

        if( isMapSpotEmpty( p.x, p.y, false ) ) {    
            *outFound = true;
            return p;
            }
        }
    
    *outFound = false;
    GridPos p = { inX, inY };
    return p;
    }





SimpleVector<char> newSpeech;
SimpleVector<ChangePosition> newSpeechPos;


SimpleVector<char*> newLocationSpeech;
SimpleVector<ChangePosition> newLocationSpeechPos;




char *isCurseNamingSay( char *inSaidString );


static void makePlayerSay( LiveObject *inPlayer, char *inToSay ) {    
                        
    if( inPlayer->lastSay != NULL ) {
        delete [] inPlayer->lastSay;
        inPlayer->lastSay = NULL;
        }
    inPlayer->lastSay = stringDuplicate( inToSay );
                        

    char isCurse = false;

    char *cursedName = isCurseNamingSay( inToSay );
                        
    if( cursedName != NULL && 
        strcmp( cursedName, "" ) != 0 ) {
        
        isCurse = cursePlayer( inPlayer->id,
                               inPlayer->email,
                               cursedName );
        
        if( isCurse ) {
            
            if( hasCurseToken( inPlayer->email ) ) {
                inPlayer->curseTokenCount = 1;
                }
            else {
                inPlayer->curseTokenCount = 0;
                }
            inPlayer->curseTokenUpdate = true;
            }
        }


    int curseFlag = 0;
    if( isCurse ) {
        curseFlag = 1;
        }

    
    char *line = autoSprintf( "%d/%d %s\n", 
                              inPlayer->id,
                              curseFlag,
                              inToSay );
                        
    newSpeech.appendElementString( line );
                        
    delete [] line;
                        

                        
    ChangePosition p = { inPlayer->xd, inPlayer->yd, false };
                        
    // if held, speech happens where held
    if( inPlayer->heldByOther ) {
        LiveObject *holdingPlayer = 
            getLiveObject( inPlayer->heldByOtherID );
                
        if( holdingPlayer != NULL ) {
            p.x = holdingPlayer->xd;
            p.y = holdingPlayer->yd;
            }
        }

    newSpeechPos.push_back( p );



    SimpleVector<int> pipesIn;
    GridPos playerPos = getPlayerPos( inPlayer );
    
    getSpeechPipesIn( playerPos.x, playerPos.y, &pipesIn );
    
    if( pipesIn.size() > 0 ) {
        for( int p=0; p<pipesIn.size(); p++ ) {
            int pipeIndex = pipesIn.getElementDirect( p );

            SimpleVector<GridPos> *pipesOut = getSpeechPipesOut( pipeIndex );

            for( int i=0; i<pipesOut->size(); i++ ) {
                GridPos outPos = pipesOut->getElementDirect( i );
                
                newLocationSpeech.push_back( stringDuplicate( inToSay ) );
                
                ChangePosition outChangePos = { outPos.x, outPos.y, false };
                newLocationSpeechPos.push_back( outChangePos );
                }
            }
        }
    }



static void holdingSomethingNew( LiveObject *inPlayer, 
                                 int inOldHoldingID = 0 ) {
    if( inPlayer->holdingID > 0 ) {
       
        ObjectRecord *o = getObject( inPlayer->holdingID );
        
        ObjectRecord *oldO = NULL;
        if( inOldHoldingID > 0 ) {
            oldO = getObject( inOldHoldingID );
            }
        
        if( o->written &&
            ( oldO == NULL ||
              ! ( oldO->written || oldO->writable ) ) ) {
            
            char metaData[ MAP_METADATA_LENGTH ];
            char found = getMetadata( inPlayer->holdingID, 
                                      (unsigned char*)metaData );

            if( found ) {
                // read what they picked up, subject to limit
                
                unsigned int sayLimit = getSayLimit( inPlayer );
                        
                if( computeAge( inPlayer ) < 10 &&
                    strlen( metaData ) > sayLimit ) {
                    // truncate with ...
                    metaData[ sayLimit ] = '.';
                    metaData[ sayLimit + 1 ] = '.';
                    metaData[ sayLimit + 2 ] = '.';
                    metaData[ sayLimit + 3 ] = '\0';
                    }
                char *quotedPhrase = autoSprintf( ":%s", metaData );
                makePlayerSay( inPlayer, quotedPhrase );
                delete [] quotedPhrase;
                }
            }

        if( o->isFlying ) {
            inPlayer->holdingFlightObject = true;
            }
        else {
            inPlayer->holdingFlightObject = false;
            }
        }
    else {
        inPlayer->holdingFlightObject = false;
        }
    }




// drops an object held by a player at target x,y location
// doesn't check for adjacency (so works for thrown drops too)
// if target spot blocked, will search for empty spot to throw object into
// if inPlayerIndicesToSendUpdatesAbout is NULL, it is ignored
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
            getPTrans( oldHoldingID, -1 );
                            
        if( bareTrans != NULL &&
            bareTrans->newTarget > 0 ) {
                            
            oldHoldingID = bareTrans->newTarget;
            
            inDroppingPlayer->holdingID = 
                bareTrans->newTarget;
            holdingSomethingNew( inDroppingPlayer, oldHoldingID );

            setFreshEtaDecayForHeld( inDroppingPlayer );
            }
        }
    else if( oldHoldingID > 0 &&
             ! getObject( oldHoldingID )->permanent ) {
        // what they are holding is NOT stuck in their
        // hand

        // see if a use-on-bare-ground drop 
        // action applies (example:  getting wounded while holding a goose)
                            
        // do not consider doing this if use-on-bare-ground leaves something
        // in the hand

        TransRecord *bareTrans =
            getPTrans( oldHoldingID, -1 );
                            
        if( bareTrans != NULL &&
            bareTrans->newTarget > 0 &&
            bareTrans->newActor == 0 ) {
                            
            oldHoldingID = bareTrans->newTarget;
            
            inDroppingPlayer->holdingID = 
                bareTrans->newTarget;
            holdingSomethingNew( inDroppingPlayer, oldHoldingID );

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

        GridPos playerPos = getPlayerPos( inDroppingPlayer );
        
        char found = findDropSpot( inX, inY, 
                                   playerPos.x, playerPos.y,
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
                    
                    if( inPlayerIndicesToSendUpdatesAbout != NULL ) {    
                        inPlayerIndicesToSendUpdatesAbout->push_back( 
                            getLiveObjectIndex( babyID ) );
                        }
                    
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
            
            // force baby pos
            // baby can wriggle out of arms in same server step that it was
            // picked up.  In that case, the clients will never get the
            // message that the baby was picked up.  The baby client could
            // be in the middle of a client-side move, and we need to force
            // them back to their true position.
            babyO->posForced = true;
            
            if( isFertileAge( inDroppingPlayer ) ) {    
                // reset food decrement time
                babyO->foodDecrementETASeconds =
                    Time::getCurrentTime() +
                    computeFoodDecrementTimeSeconds( babyO );
                }

            if( inPlayerIndicesToSendUpdatesAbout != NULL ) {
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
   
    if( inPlayerIndicesToSendUpdatesAbout != NULL ) {    
        handleMapChangeToPaths( targetX, targetY, droppedObject,
                                inPlayerIndicesToSendUpdatesAbout );
        }
    
    
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



static void handleHoldingChange( LiveObject *inPlayer, int inNewHeldID );



static void swapHeldWithGround( 
    LiveObject *inPlayer, int inTargetID, 
    int inMapX, int inMapY,
    SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout) {

    timeSec_t newHoldingEtaDecay = getEtaDecay( inMapX, inMapY );
    
    FullMapContained f = getFullMapContained( inMapX, inMapY );
    
    
    clearAllContained( inMapX, inMapY );
    setMapObject( inMapX, inMapY, 0 );
    
    handleDrop( inMapX, inMapY, inPlayer, inPlayerIndicesToSendUpdatesAbout );
    
    
    inPlayer->holdingID = inTargetID;
    inPlayer->holdingEtaDecay = newHoldingEtaDecay;
    
    setContained( inPlayer, f );


    // does bare-hand action apply to this newly-held object
    // one that results in something new in the hand and
    // nothing on the ground?
    
    // if so, it is a pick-up action, and it should apply here
    
    TransRecord *pickupTrans = getPTrans( 0, inTargetID );

    char newHandled = false;
                
    if( pickupTrans != NULL && pickupTrans->newActor > 0 &&
        pickupTrans->newTarget == 0 ) {
                    
        int newTargetID = pickupTrans->newActor;
        
        if( newTargetID != inTargetID ) {
            handleHoldingChange( inPlayer, newTargetID );
            newHandled = true;
            }
        }
    
    if( ! newHandled ) {
        holdingSomethingNew( inPlayer );
        }
    
    inPlayer->heldOriginValid = 1;
    inPlayer->heldOriginX = inMapX;
    inPlayer->heldOriginY = inMapY;
    inPlayer->heldTransitionSourceID = -1;
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



typedef struct UpdateRecord{
        char *formatString;
        char posUsed;
        int absolutePosX, absolutePosY;
        GridPos absoluteActionTarget;
        int absoluteHeldOriginX, absoluteHeldOriginY;
    } UpdateRecord;



static char *getUpdateLineFromRecord( 
    UpdateRecord *inRecord, GridPos inRelativeToPos ) {
    
    if( inRecord->posUsed ) {
        return autoSprintf( inRecord->formatString,
                            inRecord->absoluteActionTarget.x 
                            - inRelativeToPos.x,
                            inRecord->absoluteActionTarget.y 
                            - inRelativeToPos.y,
                            inRecord->absoluteHeldOriginX - inRelativeToPos.x, 
                            inRecord->absoluteHeldOriginY - inRelativeToPos.y,
                            inRecord->absolutePosX - inRelativeToPos.x, 
                            inRecord->absolutePosY - inRelativeToPos.y );
        }
    else {
        return autoSprintf( inRecord->formatString,
                            inRecord->absoluteActionTarget.x 
                            - inRelativeToPos.x,
                            inRecord->absoluteActionTarget.y 
                            - inRelativeToPos.y,
                            inRecord->absoluteHeldOriginX - inRelativeToPos.x, 
                            inRecord->absoluteHeldOriginY - inRelativeToPos.y );
        }
    }



static char isYummy( LiveObject *inPlayer, int inObjectID ) {
    ObjectRecord *o = getObject( inObjectID );
    
    if( o->isUseDummy ) {
        inObjectID = o->useDummyParent;
        o = getObject( inObjectID );
        }

    if( o->foodValue == 0 ) {
        return false;
        }

    for( int i=0; i<inPlayer->yummyFoodChain.size(); i++ ) {
        if( inObjectID == inPlayer->yummyFoodChain.getElementDirect(i) ) {
            return false;
            }
        }
    return true;
    }



static void updateYum( LiveObject *inPlayer, int inFoodEatenID ) {

    if( ! isYummy( inPlayer, inFoodEatenID ) ) {
        // chain broken
        inPlayer->yummyFoodChain.deleteAll();
        }
    
    
    ObjectRecord *o = getObject( inFoodEatenID );
    
    if( o->isUseDummy ) {
        inFoodEatenID = o->useDummyParent;
        }
    
    
    // add to chain
    // might be starting a new chain
    inPlayer->yummyFoodChain.push_back( inFoodEatenID );


    int currentBonus = inPlayer->yummyFoodChain.size() - 1;

    if( currentBonus < 0 ) {
        currentBonus = 0;
        }    

    inPlayer->yummyBonusStore += currentBonus;
    }





static UpdateRecord getUpdateRecord( 
    LiveObject *inPlayer,
    char inDelete,
    char inPartial = false ) {

    char *holdingString = getHoldingString( inPlayer );
    
    // this is 0 if still in motion (mid-move update)
    int doneMoving = 0;
    
    if( inPlayer->xs == inPlayer->xd &&
        inPlayer->ys == inPlayer->yd &&
        ! inPlayer->heldByOther ) {
        // not moving
        doneMoving = inPlayer->lastMoveSequenceNumber;
        }
    
    UpdateRecord r;
        

    char *posString;
    if( inDelete ) {
        posString = stringDuplicate( "0 0 X X" );
        r.posUsed = false;
        }
    else {
        int x, y;

        r.posUsed = true;

        if( doneMoving > 0 || ! inPartial ) {
            x = inPlayer->xs;
            y = inPlayer->ys;
            }
        else {
            // mid-move, and partial position requested
            GridPos p = computePartialMoveSpot( inPlayer );
            
            x = p.x;
            y = p.y;
            }
        
        posString = autoSprintf( "%d %d %%d %%d",          
                                 doneMoving,
                                 inPlayer->posForced );
        r.absolutePosX = x;
        r.absolutePosY = y;
        }
    
    SimpleVector<char> clothingListBuffer;
    
    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        ObjectRecord *cObj = clothingByIndex( inPlayer->clothing, c );
        int id = 0;
        
        if( cObj != NULL ) {
            id = objectRecordToID( cObj );
            }
        
        char *idString = autoSprintf( "%d", hideIDForClient( id ) );
        
        clothingListBuffer.appendElementString( idString );
        delete [] idString;
        
        if( cObj != NULL && cObj->numSlots > 0 ) {
            
            for( int cc=0; cc<inPlayer->clothingContained[c].size(); cc++ ) {
                char *contString = 
                    autoSprintf( 
                        ",%d", 
                        hideIDForClient( 
                            inPlayer->
                            clothingContained[c].getElementDirect( cc ) ) );
                
                clothingListBuffer.appendElementString( contString );
                delete [] contString;
                }
            }

        if( c < NUM_CLOTHING_PIECES - 1 ) {
            clothingListBuffer.push_back( ';' );
            }
        }
    
    char *clothingList = clothingListBuffer.getElementString();


    char *deathReason;
    
    if( inDelete && inPlayer->deathReason != NULL ) {
        deathReason = stringDuplicate( inPlayer->deathReason );
        }
    else {
        deathReason = stringDuplicate( "" );
        }
    
    
    int heldYum = 0;
    
    if( inPlayer->holdingID > 0 &&
        isYummy( inPlayer, inPlayer->holdingID ) ) {
        heldYum = 1;
        }


    r.formatString = autoSprintf( 
        "%d %d %d %d %%d %%d %s %d %%d %%d %d "
        "%.2f %s %.2f %.2f %.2f %s %d %d %d %d%s\n",
        inPlayer->id,
        inPlayer->displayID,
        inPlayer->facingOverride,
        inPlayer->actionAttempt,
        //inPlayer->actionTarget.x - inRelativeToPos.x,
        //inPlayer->actionTarget.y - inRelativeToPos.y,
        holdingString,
        inPlayer->heldOriginValid,
        //inPlayer->heldOriginX - inRelativeToPos.x,
        //inPlayer->heldOriginY - inRelativeToPos.y,
        hideIDForClient( inPlayer->heldTransitionSourceID ),
        inPlayer->heat,
        posString,
        computeAge( inPlayer ),
        1.0 / getAgeRate(),
        computeMoveSpeed( inPlayer ),
        clothingList,
        inPlayer->justAte,
        hideIDForClient( inPlayer->justAteID ),
        inPlayer->responsiblePlayerID,
        heldYum,
        deathReason );
    
    delete [] deathReason;
    

    r.absoluteActionTarget = inPlayer->actionTarget;
    
    if( inPlayer->heldOriginValid ) {
        r.absoluteHeldOriginX = inPlayer->heldOriginX;
        r.absoluteHeldOriginY = inPlayer->heldOriginY;
        }
    else {
        // we set 0,0 to clear held origins in many places in the code
        // if we leave that as an absolute pos, our birth pos leaks through
        // when we make it birth-pos relative
        
        // instead, substitute our birth pos for all invalid held pos coords
        // to prevent this
        r.absoluteHeldOriginX = inPlayer->birthPos.x;
        r.absoluteHeldOriginY = inPlayer->birthPos.y;
        }
    
        

    inPlayer->justAte = false;
    inPlayer->justAteID = 0;
    
    // held origin only valid once
    inPlayer->heldOriginValid = 0;
    
    inPlayer->facingOverride = 0;
    inPlayer->actionAttempt = 0;

    delete [] holdingString;
    delete [] posString;
    delete [] clothingList;
    
    return r;
    }



// inDelete true to send X X for position
// inPartial gets update line for player's current possition mid-path
// positions in update line will be relative to inRelativeToPos
static char *getUpdateLine( LiveObject *inPlayer, GridPos inRelativeToPos,
                            char inDelete,
                            char inPartial = false ) {
    
    UpdateRecord r = getUpdateRecord( inPlayer, inDelete, inPartial );
    
    char *line = getUpdateLineFromRecord( &r, inRelativeToPos );

    delete [] r.formatString;
    
    return line;
    }




// if inTargetID set, we only detect whether inTargetID is close enough to
// be hit
// otherwise, we find the lowest-id player that is hit and return that
static LiveObject *getHitPlayer( int inX, int inY,
                                 int inTargetID = -1,
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
        
        if( otherPlayer->heldByOther ) {
            // ghost position of a held baby
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
        
        if( inTargetID != -1 &&
            otherPlayer->id != inTargetID ) {
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

    



// for placement of tutorials out of the way 
static int maxPlacementX = 5000000;

// tutorial is alwasy placed 400,000 to East of furthest birth/Eve
// location
static int tutorialOffsetX = 400000;


// each subsequent tutorial gets put in a diferent place
static int tutorialCount = 0;

        

// returns ID of new player,
// or -1 if this player reconnected to an existing ID
int processLoggedInPlayer( Socket *inSock,
                           SimpleVector<char> *inSockBuffer,
                           char *inEmail,
                           int inTutorialNumber,
                           CurseStatus inCurseStatus,
                           // set to -2 to force Eve
                           int inForceParentID = -1,
                           int inForceDisplayID = -1,
                           GridPos *inForcePlayerPos = NULL ) {
    
    // see if player was previously disconnected
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( ! o->error && ! o->connected &&
            strcmp( o->email, inEmail ) == 0 ) {

            
            // give them this new socket and buffer
            if( o->sock != NULL ) {
                delete o->sock;
                o->sock = NULL;
                }
            if( o->sockBuffer != NULL ) {
                delete o->sockBuffer;
                o->sockBuffer = NULL;
                }
            
            o->sock = inSock;
            o->sockBuffer = inSockBuffer;
            
            // they are connecting again, need to send them everything again
            o->firstMapSent = false;
            o->firstMessageSent = false;
            o->inFlight = false;
            
            o->connected = true;
            
            if( o->heldByOther ) {
                // they're held, so they may have moved far away from their
                // original location
                
                // their first PU on reconnect should give an estimate of this
                // new location
                
                LiveObject *holdingPlayer = 
                    getLiveObject( o->heldByOtherID );
                
                if( holdingPlayer != NULL ) {
                    o->xd = holdingPlayer->xd;
                    o->yd = holdingPlayer->yd;
                    
                    o->xs = holdingPlayer->xs;
                    o->ys = holdingPlayer->ys;
                    }
                }
            
            AppLog::infoF( "Player %d (%s) has reconnected.",
                           o->id, o->email );

            delete [] inEmail;
            
            return -1;
            }
        }
             

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
    
    newObject.isTutorial = false;
    
    if( inTutorialNumber > 0 ) {
        newObject.isTutorial = true;
        }

    newObject.trueStartTimeSeconds = Time::getCurrentTime();
    newObject.lifeStartTimeSeconds = newObject.trueStartTimeSeconds;
                            

    newObject.lastSayTimeSeconds = Time::getCurrentTime();
    

    newObject.heldByOther = false;
    newObject.everHeldByParent = false;
    

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
    
    
    primeLineageTest( numPlayers );
    

    for( int i=0; i<numPlayers; i++ ) {
        LiveObject *player = players.getElement( i );
        
        if( player->error ) {
            continue;
            }

        if( player->isTutorial ) {
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
            
            if( ! isLinePermitted( newObject.email, player->lineageEveID ) ) {
                // this line forbidden for new player
                continue;
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
                if( ( inCurseStatus.curseLevel == 0 && 
                      player->curseStatus.curseLevel == 0 ) 
                    || 
                    ( inCurseStatus.curseLevel > 0 && 
                      player->curseStatus.curseLevel > 0 ) ) {
                    // cursed babies only born to cursed mothers
                    // non-cursed babies never born to cursed mothers
                    parentChoices.push_back( player );
                    }
                }
            }
        }


    char forceParentChoices = false;
    

    if( inTutorialNumber > 0 ) {
        // Tutorial always played full-grown
        parentChoices.deleteAll();
        forceParentChoices = true;
        }

    if( inForceParentID == -2 ) {
        // force eve
        parentChoices.deleteAll();
        forceParentChoices = true;
        }
    else if( inForceParentID > -1 ) {
        // force parent choice
        parentChoices.deleteAll();
        
        LiveObject *forcedParent = getLiveObject( inForceParentID );
        
        if( forcedParent != NULL ) {
            parentChoices.push_back( forcedParent );
            forceParentChoices = true;
            }
        }
    
    
    if( SettingsManager::getIntSetting( "forceAllPlayersEve", 0 ) ) {
        parentChoices.deleteAll();
        forceParentChoices = true;
        }
    


    newObject.parentChainLength = 1;

    if( parentChoices.size() == 0 || numOfAge == 0 ) {
        // new Eve
        // she starts almost full grown

        newObject.isEve = true;
        newObject.lineageEveID = newObject.id;
        
        newObject.lifeStartTimeSeconds -= 14 * ( 1.0 / getAgeRate() );

        
        int femaleID = getRandomFemalePersonObject();
        
        if( femaleID != -1 ) {
            newObject.displayID = femaleID;
            }
        }
    

    if( !forceParentChoices && 
        parentChoices.size() == 0 && numOfAge == 0 && 
        inCurseStatus.curseLevel == 0 ) {
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
                

    newObject.foodCapModifier = 1.0;

    newObject.fever = 0;

    // start full up to capacity with food
    newObject.foodStore = computeFoodCapacity( &newObject );

    

    if( ! newObject.isEve ) {
        // babies start out almost starving
        newObject.foodStore = 2;
        }
    
    if( newObject.isTutorial && newObject.foodStore > 10 ) {
        // so they can practice eating at the beginning of the tutorial
        newObject.foodStore -= 6;
        }
    

    newObject.envHeat = targetHeat;
    newObject.bodyHeat = targetHeat;
    newObject.biomeHeat = targetHeat;
    newObject.lastBiomeHeat = targetHeat;
    newObject.heat = 0.5;
    newObject.heatUpdate = false;
    newObject.lastHeatUpdate = Time::getCurrentTime();
    

    newObject.foodDecrementETASeconds =
        Time::getCurrentTime() + 
        computeFoodDecrementTimeSeconds( &newObject );
                
    newObject.foodUpdate = true;
    newObject.lastAteID = 0;
    newObject.lastAteFillMax = 0;
    newObject.justAte = false;
    newObject.justAteID = 0;
    
    newObject.yummyBonusStore = 0;

    newObject.clothing = getEmptyClothingSet();

    for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
        newObject.clothingEtaDecay[c] = 0;
        }
    
    newObject.xs = 0;
    newObject.ys = 0;
    newObject.xd = 0;
    newObject.yd = 0;
    
    newObject.lastRegionLookTime = 0;
    newObject.playerCrossingCheckTime = 0;
    
    
    LiveObject *parent = NULL;

    char placed = false;
    
    if( parentChoices.size() > 0 ) {
        placed = true;
        
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
            
            // AND each mother's current YUM multiplier
            
            int maxYumMult = 1;

            for( int i=0; i<parentChoices.size(); i++ ) {
                LiveObject *p = parentChoices.getElementDirect( i );
                
                int yumMult = p->yummyFoodChain.size() - 1;
                
                if( yumMult < 0 ) {
                    yumMult = 0;
                    }
                
                if( yumMult > maxYumMult ) {
                    maxYumMult = yumMult;
                    }
                }
            
            // 0.5 temp is worth .5 weight
            // 1.0 temp and 0 are worth 0 weight
            
            // max YumMult worth same that perfect temp is worth (0.5 weight)

            double totalWeight = 0;
            
            for( int i=0; i<parentChoices.size(); i++ ) {
                LiveObject *p = parentChoices.getElementDirect( i );

                // temp part of weight
                totalWeight += 0.5 - fabs( p->heat - 0.5 );
                

                int yumMult = p->yummyFoodChain.size() - 1;
                                
                if( yumMult < 0 ) {
                    yumMult = 0;
                    }

                // yum mult part of weight
                totalWeight += 0.5 * yumMult / (double) maxYumMult;
                }

            double choice = 
                randSource.getRandomBoundedDouble( 0, totalWeight );
            
            
            totalWeight = 0;
            
            for( int i=0; i<parentChoices.size(); i++ ) {
                LiveObject *p = parentChoices.getElementDirect( i );

                totalWeight += 0.5 - fabs( p->heat - 0.5 );


                int yumMult = p->yummyFoodChain.size() - 1;
                                
                if( yumMult < 0 ) {
                    yumMult = 0;
                    }

                // yum mult part of weight
                totalWeight += 0.5 * yumMult / (double) maxYumMult;

                if( totalWeight >= choice ) {
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
                
                char forceDifferentRace = false;

                if( getRaceSize( parentObject->race ) < 4 ) {
                    // no room in race for diverse family members
                    
                    // pick a different race for child to ensure village 
                    // diversity
                    // (otherwise, almost everyone is going to look the same)
                    forceDifferentRace = true;
                    }
                
                // everyone has a small chance of having a neighboring-race
                // baby, even if not forced by parent's small race size
                if( forceDifferentRace ||
                    randSource.getRandomDouble() > 
                    childSameRaceLikelihood ) {
                    
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
        
        if( newObject.xs > maxPlacementX ) {
            maxPlacementX = newObject.xs;
            }
        }
    else if( inTutorialNumber > 0 ) {
        
        int startX = maxPlacementX + tutorialOffsetX;
        int startY = tutorialCount * 25;

        newObject.xs = startX;
        newObject.ys = startY;
        
        newObject.xd = startX;
        newObject.yd = startY;

        char *mapFileName = autoSprintf( "tutorial%d.txt", inTutorialNumber );
        
        placed = loadTutorialStart( &( newObject.tutorialLoad ),
                                    mapFileName, startX, startY );
        
        delete [] mapFileName;

        tutorialCount ++;

        int maxPlayers = 
            SettingsManager::getIntSetting( "maxPlayers", 200 );

        if( tutorialCount > maxPlayers ) {
            // wrap back to 0 so we don't keep getting farther
            // and farther away on map if server runs for a long time.

            // The earlier-placed tutorials are over by now, because
            // we can't have more than maxPlayers tutorials running at once
            
            tutorialCount = 0;
            }
        }
    
    
    if( !placed ) {
        // tutorial didn't happen if not placed
        newObject.isTutorial = false;
        
        char allowEveRespawn = true;
        
        if( numOfAge >= 4 ) {
            // there are at least 4 fertile females on the server
            // why is this player spawning as Eve?
            // they must be on lineage ban everywhere
            // (and they are NOT a solo player on an empty server)
            // don't allow them to spawn back at their last old-age Eve death
            // location.
            allowEveRespawn = false;
            }

        // else starts at civ outskirts (lone Eve)
        int startX, startY;
        getEvePosition( newObject.email, &startX, &startY, allowEveRespawn );

        if( inCurseStatus.curseLevel > 0 ) {
            // keep cursed players away

            // 20K away in X and 20K away in Y, pushing out away from 0
            // in both directions

            if( startX > 0 )
                startX += 20000;
            else
                startX -= 20000;
            
            if( startY > 0 )
                startY += 20000;
            else
                startY -= 20000;
            }
        

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

        if( newObject.xs > maxPlacementX ) {
            maxPlacementX = newObject.xs;
            }
        }
    

    if( inForceDisplayID != -1 ) {
        newObject.displayID = inForceDisplayID;
        }

    if( inForcePlayerPos != NULL ) {
        int startX = inForcePlayerPos->x;
        int startY = inForcePlayerPos->y;
        
        newObject.xs = startX;
        newObject.ys = startY;
        
        newObject.xd = startX;
        newObject.yd = startY;

        if( newObject.xs > maxPlacementX ) {
            maxPlacementX = newObject.xs;
            }
        }
    

    
    if( parent == NULL ) {
        // Eve
        int forceID = SettingsManager::getIntSetting( "forceEveObject", 0 );
    
        if( forceID > 0 ) {
            newObject.displayID = forceID;
            }
        
        
        float forceAge = SettingsManager::getFloatSetting( "forceEveAge", 0.0 );
        
        if( forceAge > 0 ) {
            newObject.lifeStartTimeSeconds = 
                Time::getCurrentTime() - forceAge * ( 1.0 / getAgeRate() );
            }
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
    newObject.nameHasSuffix = false;
    newObject.lastSay = NULL;
    newObject.curseStatus = inCurseStatus;
    

    if( newObject.curseStatus.curseLevel == 0 &&
        hasCurseToken( inEmail ) ) {
        newObject.curseTokenCount = 1;
        }
    else {
        newObject.curseTokenCount = 0;
        }

    newObject.curseTokenUpdate = true;

    
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

    newObject.heldGraveOriginX = 0;
    newObject.heldGraveOriginY = 0;

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
    
    newObject.murderPerpID = 0;
    newObject.murderPerpEmail = NULL;
    
    newObject.deathSourceID = 0;
    
    newObject.everKilledAnyone = false;
    newObject.suicide = false;
    

    newObject.sock = inSock;
    newObject.sockBuffer = inSockBuffer;
    
    newObject.gotPartOfThisFrame = false;
    
    newObject.isNew = true;
    newObject.firstMessageSent = false;
    newObject.inFlight = false;
    
    newObject.dying = false;
    newObject.dyingETA = 0;
    
    newObject.emotFrozen = false;

    newObject.connected = true;
    newObject.error = false;
    newObject.errorCauseString = "";
    
    newObject.customGraveID = -1;
    newObject.deathReason = NULL;
    
    newObject.deleteSent = false;
    newObject.deathLogged = false;
    newObject.newMove = false;
    
    newObject.posForced = false;
    newObject.waitingForForceResponse = false;
    
    // first move that player sends will be 2
    newObject.lastMoveSequenceNumber = 1;

    newObject.needsUpdate = false;
    newObject.updateSent = false;
    newObject.updateGlobal = false;
    
    newObject.babyBirthTimes = new SimpleVector<timeSec_t>();
    newObject.babyIDs = new SimpleVector<int>();
    
    newObject.birthCoolDown = 0;
    
    newObject.monumentPosSet = false;
    newObject.monumentPosSent = true;
    
    newObject.holdingFlightObject = false;

                
    for( int i=0; i<HEAT_MAP_D * HEAT_MAP_D; i++ ) {
        newObject.heatMap[i] = 0;
        }

    
    newObject.parentID = -1;
    char *parentEmail = NULL;

    if( parent != NULL && isFertileAge( parent ) ) {
        // do not log babies that new Eve spawns next to as parents
        newObject.parentID = parent->id;
        parentEmail = parent->email;

        newObject.lineageEveID = parent->lineageEveID;

        newObject.parentChainLength = parent->parentChainLength + 1;

        // mother
        newObject.lineage->push_back( newObject.parentID );

        // inherit last heard monument, if any, from parent
        newObject.monumentPosSet = parent->monumentPosSet;
        newObject.lastMonumentPos = parent->lastMonumentPos;
        newObject.lastMonumentID = parent->lastMonumentID;
        if( newObject.monumentPosSet ) {
            newObject.monumentPosSent = false;
            }
        
        
        for( int i=0; 
             i < parent->lineage->size() && 
                 i < maxLineageTracked - 1;
             i++ ) {
            
            newObject.lineage->push_back( 
                parent->lineage->getElementDirect( i ) );
            }
        }

    newObject.birthPos.x = newObject.xd;
    newObject.birthPos.y = newObject.yd;
    
    newObject.heldOriginX = newObject.xd;
    newObject.heldOriginY = newObject.yd;
    
    newObject.actionTarget = newObject.birthPos;
    

    
    // parent pointer possibly no longer valid after push_back, which
    // can resize the vector
    parent = NULL;


    if( newObject.isTutorial ) {
        AppLog::infoF( "New player %s pending tutorial load (tutorial=%d)",
                       newObject.email,
                       inTutorialNumber );

        // holding bay for loading tutorial maps incrementally
        tutorialLoadingPlayers.push_back( newObject );
        }
    else {
        players.push_back( newObject );            
        }
    

    if( ! newObject.isTutorial )        
    logBirth( newObject.id,
              newObject.email,
              newObject.parentID,
              parentEmail,
              ! getFemale( &newObject ),
              newObject.xd,
              newObject.yd,
              players.size(),
              newObject.parentChainLength );
    
    AppLog::infoF( "New player %s connected as player %d (tutorial=%d) (%d,%d)"
                   " (maxPlacementX=%d)",
                   newObject.email, newObject.id,
                   inTutorialNumber, newObject.xs, newObject.ys,
                   maxPlacementX );
    
    return newObject.id;
    }




static void processWaitingTwinConnection( FreshConnection inConnection ) {
    AppLog::infoF( "Player %s waiting for twin party of %d", 
                   inConnection.email,
                   inConnection.twinCount );
    waitingForTwinConnections.push_back( inConnection );
    
    CurseStatus anyTwinCurseLevel = inConnection.curseStatus;
    

    // count how many match twin code from inConnection
    // is this the last one to join the party?
    SimpleVector<FreshConnection*> twinConnections;
    

    for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
        FreshConnection *nextConnection = 
            waitingForTwinConnections.getElement( i );
        
        if( nextConnection->error ) {
            continue;
            }
        
        if( nextConnection->twinCode != NULL
            &&
            strcmp( inConnection.twinCode, nextConnection->twinCode ) == 0 
            &&
            inConnection.twinCount == nextConnection->twinCount ) {
            
            if( strcmp( inConnection.email, nextConnection->email ) == 0 ) {
                // don't count this connection itself
                continue;
                }

            if( nextConnection->curseStatus.curseLevel > 
                anyTwinCurseLevel.curseLevel ) {
                anyTwinCurseLevel = nextConnection->curseStatus;
                }
            
            twinConnections.push_back( nextConnection );
            }
        }

    
    if( twinConnections.size() + 1 >= inConnection.twinCount ) {
        // everyone connected and ready in twin party

        AppLog::infoF( "Found %d other people waiting for twin party of %s, "
                       "ready", 
                       twinConnections.size(), inConnection.email );
        
        char *emailCopy = stringDuplicate( inConnection.email );
        
        int newID = processLoggedInPlayer( inConnection.sock,
                                           inConnection.sockBuffer,
                                           inConnection.email,
                                           inConnection.tutorialNumber,
                                           anyTwinCurseLevel );

        if( newID == -1 ) {
            AppLog::infoF( "%s reconnected to existing life, not triggering "
                           "fellow twins to spawn now.",
                           emailCopy );

            // take them out of waiting list too
            for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
                if( waitingForTwinConnections.getElement( i )->sock ==
                    inConnection.sock ) {
                    // found
                    
                    waitingForTwinConnections.deleteElement( i );
                    break;
                    }
                }

            delete [] emailCopy;

            if( inConnection.twinCode != NULL ) {
                delete [] inConnection.twinCode;
                inConnection.twinCode = NULL;
                }
            return;
            }

        delete [] emailCopy;
        
        
        LiveObject *newPlayer = NULL;

        if( inConnection.tutorialNumber == 0 ) {
            newPlayer = getLiveObject( newID );
            }
        else {
            newPlayer = tutorialLoadingPlayers.getElement(
                tutorialLoadingPlayers.size() - 1 );
            }


        int parent = newPlayer->parentID;
        int displayID = newPlayer->displayID;
        GridPos playerPos = { newPlayer->xd, newPlayer->yd };
        
        GridPos *forcedEvePos = NULL;
        
        if( parent == -1 ) {
            // first twin placed was Eve
            // others are identical Eves
            forcedEvePos = &playerPos;
            // trigger forced Eve placement
            parent = -2;
            }

        // save these out here, because newPlayer points into 
        // tutorialLoadingPlayers, which may expand during this loop,
        // invalidating that pointer
        char isTutorial = newPlayer->isTutorial;
        TutorialLoadProgress sharedTutorialLoad = newPlayer->tutorialLoad;

        for( int i=0; i<twinConnections.size(); i++ ) {
            FreshConnection *nextConnection = 
                twinConnections.getElementDirect( i );
            
            processLoggedInPlayer( nextConnection->sock,
                                   nextConnection->sockBuffer,
                                   nextConnection->email,
                                   // ignore tutorial number of all but
                                   // first player
                                   0,
                                   anyTwinCurseLevel,
                                   parent,
                                   displayID,
                                   forcedEvePos );
            
            // just added is always last object in list
            LiveObject newTwinPlayer = 
                players.getElementDirect( players.size() - 1 );

            if( isTutorial ) {
                // force this one to wait for same tutorial map load
                newTwinPlayer.tutorialLoad = sharedTutorialLoad;

                // flag them as a tutorial player too, so they can't have
                // babies in the tutorial, and they won't be remembered
                // as a long-lineage position at shutdown
                newTwinPlayer.isTutorial = true;

                players.deleteElement( players.size() - 1 );
                
                tutorialLoadingPlayers.push_back( newTwinPlayer );
                }
            }
        

        char *twinCode = stringDuplicate( inConnection.twinCode );
        
        for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
            FreshConnection *nextConnection = 
                waitingForTwinConnections.getElement( i );
            
            if( nextConnection->error ) {
                continue;
                }
            
            if( nextConnection->twinCode != NULL 
                &&
                nextConnection->twinCount == inConnection.twinCount
                &&
                strcmp( nextConnection->twinCode, twinCode ) == 0 ) {
                
                delete [] nextConnection->twinCode;
                waitingForTwinConnections.deleteElement( i );
                i--;
                }
            }
        
        delete [] twinCode;
        }
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
    if( targetObj->slotsLocked ) {
        return false;
        }

    float slotSize =
        targetObj->slotSize;
    
    float containSize =
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
        

        // adding something to a container acts like a drop
        // but some non-permanent held objects are supposed to go through 
        // a transition when they drop (example:  held wild piglet is
        // non-permanent, so it can be containable, but it supposed to
        // switch to a moving wild piglet when dropped.... we should
        // switch to this other wild piglet when putting it into a container
        // too)

        // "set-down" type bare ground
        // trans exists for what we're 
        // holding?
        TransRecord *r = getPTrans( inPlayer->holdingID, -1 );

        if( r != NULL && r->newActor == 0 && r->newTarget > 0 ) {
                                            
            // only applies if the 
            // bare-ground
            // trans leaves nothing in
            // our hand
            
            // first, change what they
            // are holding to this 
            // newTarget
            

            handleHoldingChange( 
                inPlayer,
                r->newTarget );
            }


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
                            same = false;
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
                            
            if( target > 0 && getObject( target )->slotsLocked ) {
                return false;
                }

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

            
            if( toRemoveID == 0 ) {
                // this should never happen, except due to map corruption
                
                // clear container, to be safe
                clearAllContained( inContX, inContY );
                return false;
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
                

                // does bare-hand action apply to this newly-held object
                // one that results in something new in the hand and
                // nothing on the ground?

                // if so, it is a pick-up action, and it should apply here

                TransRecord *pickupTrans = getPTrans( 0, inPlayer->holdingID );
                
                if( pickupTrans != NULL && pickupTrans->newActor > 0 &&
                    pickupTrans->newTarget == 0 ) {
                    
                    handleHoldingChange( inPlayer, pickupTrans->newActor );
                    }
                else {
                    holdingSomethingNew( inPlayer );
                    }
                
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
                                        
        float slotSize =
            cObj->slotSize;
                                        
        float containSize =
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




static void pickupToHold( LiveObject *inPlayer, int inX, int inY, 
                          int inTargetID ) {
    inPlayer->holdingEtaDecay = 
        getEtaDecay( inX, inY );
    
    FullMapContained f =
        getFullMapContained( inX, inY );
    
    setContained( inPlayer, f );
    
    clearAllContained( inX, inY );
    
    setResponsiblePlayer( - inPlayer->id );
    setMapObject( inX, inY, 0 );
    setResponsiblePlayer( -1 );
    
    inPlayer->holdingID = inTargetID;
    holdingSomethingNew( inPlayer );
    
    inPlayer->heldGraveOriginX = inX;
    inPlayer->heldGraveOriginY = inY;
    
    inPlayer->heldOriginValid = 1;
    inPlayer->heldOriginX = inX;
    inPlayer->heldOriginY = inY;
    inPlayer->heldTransitionSourceID = -1;
    }


static void removeFromClothingContainerToHold( LiveObject *inPlayer,
                                               int inC,
                                               int inI = -1 ) {    
    
    ObjectRecord *cObj = 
        clothingByIndex( inPlayer->clothing, 
                         inC );
                                
    float stretch = 1.0f;
    
    if( cObj != NULL ) {
        stretch = cObj->slotTimeStretch;
        }
    
    int oldNumContained = 
        inPlayer->clothingContained[inC].size();

    int slotToRemove = inI;
                                
    if( slotToRemove < 0 ) {
        slotToRemove = oldNumContained - 1;
        }
                                
    int toRemoveID = -1;
                                
    if( oldNumContained > 0 &&
        oldNumContained > slotToRemove &&
        slotToRemove >= 0 ) {
                                    
        toRemoveID = 
            inPlayer->clothingContained[inC].
            getElementDirect( slotToRemove );
        }

    if( oldNumContained > 0 &&
        oldNumContained > slotToRemove &&
        slotToRemove >= 0 &&
        // old enough to handle it
        getObject( toRemoveID )->minPickupAge <= 
        computeAge( inPlayer ) ) {
                                    

        inPlayer->holdingID = 
            inPlayer->clothingContained[inC].
            getElementDirect( slotToRemove );
        holdingSomethingNew( inPlayer );

        inPlayer->holdingEtaDecay = 
            inPlayer->
            clothingContainedEtaDecays[inC].
            getElementDirect( slotToRemove );
                                    
        timeSec_t curTime = Time::timeSec();

        if( inPlayer->holdingEtaDecay != 0 ) {
                                        
            timeSec_t offset = 
                inPlayer->holdingEtaDecay
                - curTime;
            offset = offset * stretch;
            inPlayer->holdingEtaDecay =
                curTime + offset;
            }

        inPlayer->clothingContained[inC].
            deleteElement( slotToRemove );
        inPlayer->clothingContainedEtaDecays[inC].
            deleteElement( slotToRemove );

        inPlayer->heldOriginValid = 0;
        inPlayer->heldOriginX = 0;
        inPlayer->heldOriginY = 0;
        inPlayer->heldTransitionSourceID = -1;
        }
    }


// change held as the result of a transition
static void handleHoldingChange( LiveObject *inPlayer, int inNewHeldID ) {
    
    LiveObject *nextPlayer = inPlayer;

    int oldHolding = nextPlayer->holdingID;
    
    int oldContained = 
        nextPlayer->numContained;
    
    
    nextPlayer->heldOriginValid = 0;
    nextPlayer->heldOriginX = 0;
    nextPlayer->heldOriginY = 0;
    
    // can newly changed container hold
    // less than what it could contain
    // before?
    
    int newHeldSlots = getNumContainerSlots( inNewHeldID );
    
    if( newHeldSlots < oldContained ) {
        // new container can hold less
        // truncate
                            
        GridPos dropPos = 
            getPlayerPos( inPlayer );
                            
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
                inPlayer,
                // only temporary, don't worry about blocking players
                // with this drop
                NULL );
                                

            // responsible player for stuff thrown on map by shrink
            setResponsiblePlayer( inPlayer->id );

            // shrink contianer on map
            shrinkContainer( spot.x, spot.y, 
                             newHeldSlots );
            
            setResponsiblePlayer( -1 );
            

            // pick it back up
            nextPlayer->holdingEtaDecay = 
                getEtaDecay( spot.x, spot.y );
                                    
            FullMapContained f =
                getFullMapContained( spot.x, spot.y );

            setContained( inPlayer, f );
            
            clearAllContained( spot.x, spot.y );
            setMapObject( spot.x, spot.y, 0 );
            }
        else {
            // no spot to throw it
            // cannot leverage map's container-shrinking
            // just truncate held container directly
            
            // truncated contained items will be lost
            inPlayer->numContained = newHeldSlots;
            }
        }

    nextPlayer->holdingID = inNewHeldID;
    holdingSomethingNew( inPlayer, oldHolding );

    if( newHeldSlots > 0 && 
        oldHolding != 0 ) {
                                        
        restretchDecays( 
            newHeldSlots,
            nextPlayer->containedEtaDecays,
            oldHolding,
            nextPlayer->holdingID );
        }
    
    
    if( oldHolding != inNewHeldID ) {
            
        char kept = false;

        // keep old decay timeer going...
        // if they both decay to the same thing in the same time
        if( oldHolding > 0 && inNewHeldID > 0 ) {
            
            TransRecord *oldDecayT = getMetaTrans( -1, oldHolding );
            TransRecord *newDecayT = getMetaTrans( -1, inNewHeldID );
            
            if( oldDecayT != NULL && newDecayT != NULL ) {
                if( oldDecayT->autoDecaySeconds == newDecayT->autoDecaySeconds
                    && 
                    oldDecayT->newTarget == newDecayT->newTarget ) {
                    
                    kept = true;
                    }
                }
            }
        if( !kept ) {
            setFreshEtaDecayForHeld( nextPlayer );
            }
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
    if( ! inPlayer->connected ) {
        // stop sending messages to disconnected players
        return;
        }
    
    
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
        setPlayerDisconnected( inPlayer, "Socket write failed" );
        }

    inPlayer->gotPartOfThisFrame = true;
    
    if( deleteMessage ) {
        delete [] message;
        }
    }
    


void readPhrases( const char *inSettingsName, 
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
    char *saidString = inSaidString;
    
    if( saidString[0] == ':' ) {
        // first : indicates reading a written phrase.
        // reading written phrase aloud does not have usual effects
        // (block curse exploit)
        return NULL;
        }
    
    for( int i=0; i<inPhraseList->size(); i++ ) {
        char *testString = inPhraseList->getElementDirect( i );
        
        if( strstr( inSaidString, testString ) == saidString ) {
            // hit
            int phraseLen = strlen( testString );
            // skip spaces after
            while( saidString[ phraseLen ] == ' ' ) {
                phraseLen++;
                }
            return &( saidString[ phraseLen ] );
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

char *isCurseNamingSay( char *inSaidString ) {
    return isNamingSay( inSaidString, &cursingPhrases );
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

            lastRemoteApocalypseCheckTime = curTime;

            // don't actually send request to reflector if apocalypse
            // not possible locally
            // or if broadcast mode disabled
            if( SettingsManager::getIntSetting( "apocalypsePossible", 0 ) &&
                SettingsManager::getIntSetting( "apocalypseBroadcast", 0 ) ) {

                printf( "Checking for remote apocalypse\n" );
            
                char *url = autoSprintf( "%s?action=check_apocalypse", 
                                         reflectorURL );
        
                apocalypseRequest =
                    new WebRequest( "GET", url, NULL );
            
                delete [] url;
                }
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
            
            AppLog::info( "Apocalypse triggerered, starting it" );


            // only broadcast to reflector if apocalypseBroadcast set
            if( !apocalypseRemote &&
                SettingsManager::getIntSetting( "apocalypseBroadcast", 0 ) &&
                apocalypseRequest == NULL && reflectorURL != NULL ) {
                
                AppLog::info( "Apocalypse broadcast set, telling reflector" );

                
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
                if( !nextPlayer->error && nextPlayer->connected ) {
                    
                    int numSent = 
                        nextPlayer->sock->send( 
                            (unsigned char*)message, 
                            messageLength,
                            false, false );
                    
                    nextPlayer->gotPartOfThisFrame = true;
                    
                    if( numSent != messageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    }
                }
            
            apocalypseStartTime = Time::getCurrentTime();
            apocalypseStarted = true;
            postApocalypseStarted = false;
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
            
            if( ! postApocalypseStarted  ) {
                AppLog::infoF( "Enough warning time, %d players still alive",
                               players.size() );
                
                
                double startTime = Time::getCurrentTime();
                
                // clear map
                freeMap( true );

                AppLog::infoF( "Apocalypse freeMap took %f sec",
                               Time::getCurrentTime() - startTime );
                wipeMapFiles();

                AppLog::infoF( "Apocalypse wipeMapFiles took %f sec",
                               Time::getCurrentTime() - startTime );
                
                initMap();
                
                AppLog::infoF( "Apocalypse initMap took %f sec",
                               Time::getCurrentTime() - startTime );
                

                lastRemoteApocalypseCheckTime = curTime;
                
                for( int i=0; i<players.size(); i++ ) {
                    LiveObject *nextPlayer = players.getElement( i );
                    backToBasics( nextPlayer );
                    }
                
                // send everyone update about everyone
                for( int i=0; i<players.size(); i++ ) {
                    LiveObject *nextPlayer = players.getElement( i );
                    nextPlayer->firstMessageSent = false;
                    nextPlayer->firstMapSent = false;
                    nextPlayer->inFlight = false;
                    }

                postApocalypseStarted = true;
                }
            else {
                // make sure all players have gotten map and update
                char allMapAndUpdate = true;
                
                for( int i=0; i<players.size(); i++ ) {
                    LiveObject *nextPlayer = players.getElement( i );
                    if( ! nextPlayer->firstMapSent ) {
                        allMapAndUpdate = false;
                        break;
                        }
                    }
                
                if( allMapAndUpdate ) {
                    
                    // send all players the AD message
                    const char *message = "AD\n#";
                    int messageLength = strlen( message );
            
                    for( int i=0; i<players.size(); i++ ) {
                        LiveObject *nextPlayer = players.getElement( i );
                        if( !nextPlayer->error && nextPlayer->connected ) {
                    
                            int numSent = 
                                nextPlayer->sock->send( 
                                    (unsigned char*)message, 
                                    messageLength,
                                    false, false );
                            
                            nextPlayer->gotPartOfThisFrame = true;
                    
                            if( numSent != messageLength ) {
                                setPlayerDisconnected( nextPlayer, 
                                                       "Socket write failed" );
                                }
                            }
                        }

                    // totally done
                    apocalypseStarted = false;
                    apocalypseTriggered = false;
                    apocalypseRemote = false;
                    postApocalypseStarted = false;
                    }
                }    
            }
        }
    }




void monumentStep() {
    if( monumentCallPending ) {
        
        // send to all players
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            // remember it to tell babies about it
            nextPlayer->monumentPosSet = true;
            nextPlayer->lastMonumentPos.x = monumentCallX;
            nextPlayer->lastMonumentPos.y = monumentCallY;
            nextPlayer->lastMonumentID = monumentCallID;
            nextPlayer->monumentPosSent = true;
            
            if( !nextPlayer->error && nextPlayer->connected ) {
                
                char *message = autoSprintf( "MN\n%d %d %d\n#", 
                                             monumentCallX -
                                             nextPlayer->birthPos.x, 
                                             monumentCallY -
                                             nextPlayer->birthPos.y,
                                             hideIDForClient( 
                                                 monumentCallID ) );
                int messageLength = strlen( message );


                int numSent = 
                    nextPlayer->sock->send( 
                        (unsigned char*)message, 
                        messageLength,
                        false, false );
                
                nextPlayer->gotPartOfThisFrame = true;
                
                delete [] message;

                if( numSent != messageLength ) {
                    setPlayerDisconnected( nextPlayer, "Socket write failed" );
                    }
                }
            }

        monumentCallPending = false;
        }
    }




// inPlayerName may be destroyed inside this function
// returns a uniquified name, sometimes newly allocated.
// return value destroyed by caller
char *getUniqueCursableName( char *inPlayerName, char *outSuffixAdded ) {
    
    char dup = isNameDuplicateForCurses( inPlayerName );
    
    if( ! dup ) {
        *outSuffixAdded = false;
        return inPlayerName;
        }
    else {
        *outSuffixAdded = true;

        int targetPersonNumber = 1;
        
        char *fullName = stringDuplicate( inPlayerName );

        while( dup ) {
            // try next roman numeral
            targetPersonNumber++;
            
            int personNumber = targetPersonNumber;            
        
            SimpleVector<char> romanNumeralList;
        
            while( personNumber >= 100 ) {
                romanNumeralList.push_back( 'C' );
                personNumber -= 100;
                }
            while( personNumber >= 50 ) {
                romanNumeralList.push_back( 'L' );
                personNumber -= 50;
                }
            while( personNumber >= 40 ) {
                romanNumeralList.push_back( 'X' );
                romanNumeralList.push_back( 'L' );
                personNumber -= 40;
                }
            while( personNumber >= 10 ) {
                romanNumeralList.push_back( 'X' );
                personNumber -= 10;
                }
            while( personNumber >= 9 ) {
                romanNumeralList.push_back( 'I' );
                romanNumeralList.push_back( 'X' );
                personNumber -= 9;
                }
            while( personNumber >= 5 ) {
                romanNumeralList.push_back( 'V' );
                personNumber -= 5;
                }
            while( personNumber >= 4 ) {
                romanNumeralList.push_back( 'I' );
                romanNumeralList.push_back( 'V' );
                personNumber -= 4;
                }
            while( personNumber >= 1 ) {
                romanNumeralList.push_back( 'I' );
                personNumber -= 1;
                }
            
            char *romanString = romanNumeralList.getElementString();

            delete [] fullName;
            
            fullName = autoSprintf( "%s %s", inPlayerName, romanString );
            delete [] romanString;
            
            dup = isNameDuplicateForCurses( fullName );
            }
        
        delete [] inPlayerName;
        
        return fullName;
        }
    
    }




typedef struct ForcedEffects {
        // -1 if no emot specified
        int emotIndex;
        int ttlSec;
        
        char foodModifierSet;
        double foodCapModifier;
        
        char feverSet;
        float fever;
    } ForcedEffects;
        


ForcedEffects checkForForcedEffects( int inHeldObjectID ) {
    ForcedEffects e = { -1, 0, false, 1.0, false, 0.0f };
    
    ObjectRecord *o = getObject( inHeldObjectID );
    
    if( o != NULL ) {
        char *emotPos = strstr( o->description, "emot_" );
        
        if( emotPos != NULL ) {
            sscanf( emotPos, "emot_%d_%d", 
                    &( e.emotIndex ), &( e.ttlSec ) );
            }

        char *foodPos = strstr( o->description, "food_" );
        
        if( foodPos != NULL ) {
            int numRead = sscanf( foodPos, "food_%lf", 
                                  &( e.foodCapModifier ) );
            if( numRead == 1 ) {
                e.foodModifierSet = true;
                }
            }

        char *feverPos = strstr( o->description, "fever_" );
        
        if( feverPos != NULL ) {
            int numRead = sscanf( feverPos, "fever_%f", 
                                  &( e.fever ) );
            if( numRead == 1 ) {
                e.feverSet = true;
                }
            }
        }
    
    
    return e;
    }




void setNoLongerDying( LiveObject *inPlayer, 
                       SimpleVector<int> *inPlayerIndicesToSendHealingAbout ) {
    inPlayer->dying = false;
    inPlayer->murderSourceID = 0;
    inPlayer->murderPerpID = 0;
    if( inPlayer->murderPerpEmail != 
        NULL ) {
        delete [] 
            inPlayer->murderPerpEmail;
        inPlayer->murderPerpEmail =
            NULL;
        }
    
    inPlayer->deathSourceID = 0;
    inPlayer->holdingWound = false;
    inPlayer->customGraveID = -1;
    
    inPlayer->emotFrozen = false;
    inPlayer->foodCapModifier = 1.0;
    inPlayer->foodUpdate = true;

    inPlayer->fever = 0;

    if( inPlayer->deathReason 
        != NULL ) {
        delete [] inPlayer->deathReason;
        inPlayer->deathReason = NULL;
        }
                                        
    inPlayerIndicesToSendHealingAbout->
        push_back( 
            getLiveObjectIndex( 
                inPlayer->id ) );
    }



typedef struct FlightDest {
        int playerID;
        GridPos destPos;
    } FlightDest;
        



int main() {

    if( checkReadOnly() ) {
        printf( "File system read-only.  Server exiting.\n" );
        return 1;
        }
    

    memset( allowedSayCharMap, false, 256 );
    
    int numAllowed = strlen( allowedSayChars );
    for( int i=0; i<numAllowed; i++ ) {
        allowedSayCharMap[ (int)( allowedSayChars[i] ) ] = true;
        }
    

    nextID = 
        SettingsManager::getIntSetting( "nextPlayerID", 2 );


    // make backup and delete old backup every day
    AppLog::setLog( new FileLog( "log.txt", 86400 ) );

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
    
    
    readPhrases( "babyNamingPhrases", &nameGivingPhrases );
    readPhrases( "familyNamingPhrases", &familyNameGivingPhrases );

    readPhrases( "cursingPhrases", &cursingPhrases );
    
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

    initCurses();
    

    initLifeLog();
    //initBackup();
    
    initPlayerStats();
    initLineageLog();
    
    initLineageLimit();
    

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


    if( initMap() != true ) {
        // cannot continue after map init fails
        return 1;
        }
    


    if( false ) {
        
        printf( "Running map sampling\n" );
    
        int idA = 290;
        int idB = 942;
        
        int totalCountA = 0;
        int totalCountB = 0;
        int numRuns = 2;

        for( int i=0; i<numRuns; i++ ) {
        
        
            int countA = 0;
            int countB = 0;
        
            int x = randSource.getRandomBoundedInt( 10000, 300000 );
            int y = randSource.getRandomBoundedInt( 10000, 300000 );
        
            printf( "Sampling at %d,%d\n", x, y );


            for( int yd=y; yd<y + 2400; yd++ ) {
                for( int xd=x; xd<x + 2400; xd++ ) {
                    int oID = getMapObject( xd, yd );
                
                    if( oID == idA ) {
                        countA ++;
                        }
                    else if( oID == idB ) {
                        countB ++;
                        }
                    }
                }
            printf( "   Count at %d,%d is %d = %d, %d = %d\n",
                    x, y, idA, countA, idB, countB );

            totalCountA += countA;
            totalCountB += countB;
            }
        printf( "Average count %d (%s) = %f,  %d (%s) = %f  over %d runs\n",
                idA, getObject( idA )->description, 
                totalCountA / (double)numRuns,
                idB, getObject( idB )->description, 
                totalCountB / (double)numRuns,
                numRuns );
        printf( "Press ENTER to continue:\n" );
    
        int readInt;
        scanf( "%d", &readInt );
        }
    


    
    int port = 
        SettingsManager::getIntSetting( "port", 5077 );
    
    
    
    SocketServer *server = new SocketServer( port, 256 );
    
    sockPoll.addSocketServer( server );
    
    AppLog::infoF( "Listening for connection on port %d", port );

    // if we received one the last time we looped, don't sleep when
    // polling for socket being ready, because there could be more data
    // waiting in the buffer for a given socket
    char someClientMessageReceived = false;
    
    
    int shutdownMode = SettingsManager::getIntSetting( "shutdownMode", 0 );
    int forceShutdownMode = 
            SettingsManager::getIntSetting( "forceShutdownMode", 0 );
        

    while( !quit ) {

        double curStepTime = Time::getCurrentTime();
        
        char periodicStepThisStep = false;
        
        if( curStepTime - lastPeriodicStepTime > periodicStepTime ) {
            periodicStepThisStep = true;
            lastPeriodicStepTime = curStepTime;
            }
        
        
        if( periodicStepThisStep ) {
            shutdownMode = SettingsManager::getIntSetting( "shutdownMode", 0 );
            forceShutdownMode = 
                SettingsManager::getIntSetting( "forceShutdownMode", 0 );
            
            if( checkReadOnly() ) {
                // read-only file system causes all kinds of weird 
                // behavior
                // shut this server down NOW
                printf( "File system read only, forcing server shutdown.\n" );

                // force-run cron script one time here
                // this will send warning email to admin
                // (cron jobs stop running if filesystem read-only)
                system( "../scripts/checkServerRunningCron.sh" );

                shutdownMode = 1;
                forceShutdownMode = 1;
                }
            }
        
        
        if( forceShutdownMode ) {
            shutdownMode = 1;
        
            const char *shutdownMessage = "SD\n#";
            int messageLength = strlen( shutdownMessage );
            
            // send everyone who's still alive a shutdown message
            for( int i=0; i<players.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( i );
                
                if( nextPlayer->error ) {
                    continue;
                    }

                if( nextPlayer->connected ) {    
                    nextPlayer->sock->send( 
                        (unsigned char*)shutdownMessage, 
                        messageLength,
                        false, false );
                
                    nextPlayer->gotPartOfThisFrame = true;
                    }
                
                // don't worry about num sent
                // it's the last message to this client anyway
                setDeathReason( nextPlayer, 
                                "forced_shutdown" );
                nextPlayer->error = true;
                nextPlayer->errorCauseString =
                    "Forced server shutdown";
                }
            }
        else if( shutdownMode ) {
            // any disconnected players should be killed now
            for( int i=0; i<players.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( i );
                if( ! nextPlayer->error && ! nextPlayer->connected ) {
                    
                    setDeathReason( nextPlayer, 
                                    "disconnect_shutdown" );
                    
                    nextPlayer->error = true;
                    nextPlayer->errorCauseString =
                        "Disconnected during shutdown";
                    }
                }
            }
        

        if( periodicStepThisStep ) {
            
            apocalypseStep();
            monumentStep();
            
            //checkBackup();

            stepFoodLog();
            stepFailureLog();
            
            stepPlayerStats();
            stepLineageLog();
            stepCurseServerRequests();
            }
        
        
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

        if( tutorialLoadingPlayers.size() > 0 ) {
            // don't wait at all if there are tutorial maps to load
            pollTimeout = 0;
            }


        // we thus use zero CPU as long as no messages or new connections
        // come in, and only wake up when some timed action needs to be
        // handled
        
        readySock = sockPoll.wait( (int)( pollTimeout * 1000 ) );
        
        
        
        
        if( readySock != NULL && !readySock->isSocket ) {
            // server ready
            Socket *sock = server->acceptConnection( 0 );

            if( sock != NULL ) {
                HostAddress *a = sock->getRemoteHostAddress();
                
                if( a == NULL ) {    
                    AppLog::info( "Got connection from unknown address" );
                    }
                else {
                    AppLog::infoF( "Got connection from %s:%d",
                                  a->mAddressString, a->mPort );
                    delete a;
                    }
            

                FreshConnection newConnection;
                
                newConnection.connectionStartTimeSeconds = 
                    Time::getCurrentTime();

                newConnection.email = NULL;

                newConnection.sock = sock;

                newConnection.sequenceNumber = nextSequenceNumber;

                

                char *secretString = 
                    SettingsManager::getStringSetting( 
                        "statsServerSharedSecret", "sdfmlk3490sadfm3ug9324" );

                char *numberString = 
                    autoSprintf( "%lu", newConnection.sequenceNumber );
                
                char *nonce = hmac_sha1( secretString, numberString );

                delete [] secretString;
                delete [] numberString;

                newConnection.sequenceNumberString = 
                    autoSprintf( "%s%lu", nonce, 
                                 newConnection.sequenceNumber );
                
                delete [] nonce;
                    

                newConnection.tutorialNumber = 0;
                newConnection.curseStatus.curseLevel = 0;
                newConnection.curseStatus.excessPoints = 0;

                newConnection.twinCode = NULL;
                newConnection.twinCount = 0;
                
                
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
                                           "%s\n"
                                           "%lu\n#",
                                           currentPlayers, maxPlayers,
                                           newConnection.sequenceNumberString,
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
        double currentTime = Time::getCurrentTime();
        
        for( int i=0; i<newConnections.size(); i++ ) {
            
            FreshConnection *nextConnection = newConnections.getElement( i );
            
            
            if( nextConnection->email != NULL &&
                nextConnection->curseStatus.curseLevel == -1 ) {
                // keep checking if curse level has arrived from
                // curse server
                nextConnection->curseStatus =
                    getCurseLevel( nextConnection->email );
                if( nextConnection->curseStatus.curseLevel != -1 ) {
                    AppLog::infoF( 
                        "Got curse level for %s from curse server: "
                        "%d (excess %d)",
                        nextConnection->email,
                        nextConnection->curseStatus.curseLevel,
                        nextConnection->curseStatus.excessPoints );
                    }
                }
            else if( nextConnection->ticketServerRequest != NULL ) {
                
                int result;

                if( currentTime - nextConnection->ticketServerRequestStartTime
                    < 8 ) {
                    // 8-second timeout on ticket server requests
                    result = nextConnection->ticketServerRequest->step();
                    }
                else {
                    result = -1;
                    }

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
                            
                            delete nextConnection->ticketServerRequest;
                            nextConnection->ticketServerRequest = NULL;

                            delete [] nextConnection->sequenceNumberString;
                            nextConnection->sequenceNumberString = NULL;
                            
                            if( nextConnection->twinCode != NULL
                                && 
                                nextConnection->twinCount > 0 ) {
                                processWaitingTwinConnection( *nextConnection );
                                }
                            else {
                                if( nextConnection->twinCode != NULL ) {
                                    delete [] nextConnection->twinCode;
                                    nextConnection->twinCode = NULL;
                                    }
                                
                                processLoggedInPlayer( 
                                    nextConnection->sock,
                                    nextConnection->sockBuffer,
                                    nextConnection->email,
                                    nextConnection->tutorialNumber,
                                    nextConnection->curseStatus );
                                }
                                                        
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
                        
                        if( tokens->size() == 4 || tokens->size() == 5 ||
                            tokens->size() == 7 ) {
                            
                            nextConnection->email = 
                                stringDuplicate( 
                                    tokens->getElementDirect( 1 ) );
                            char *pwHash = tokens->getElementDirect( 2 );
                            char *keyHash = tokens->getElementDirect( 3 );
                            
                            if( tokens->size() >= 5 ) {
                                sscanf( tokens->getElementDirect( 4 ),
                                        "%d", 
                                        &( nextConnection->tutorialNumber ) );
                                }
                            
                            if( tokens->size() == 7 ) {
                                nextConnection->twinCode =
                                    stringDuplicate( 
                                        tokens->getElementDirect( 5 ) );
                                
                                sscanf( tokens->getElementDirect( 6 ),
                                        "%d", 
                                        &( nextConnection->twinCount ) );

                                int maxCount = 
                                    SettingsManager::getIntSetting( 
                                        "maxTwinPartySize", 4 );
                                
                                if( nextConnection->twinCount > maxCount ) {
                                    nextConnection->twinCount = maxCount;
                                    }
                                }
                            

                            char emailAlreadyLoggedIn = false;
                            

                            for( int p=0; p<players.size(); p++ ) {
                                LiveObject *o = players.getElement( p );
                                

                                if( ! o->error && 
                                    o->connected && 
                                    strcmp( o->email, 
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
                                nextConnection->curseStatus.curseLevel = 0;
                                nextConnection->curseStatus.excessPoints = 0;
                                }
                            else {
                                // this may return -1 if curse server
                                // request is pending
                                // we'll catch that case later above
                                nextConnection->curseStatus =
                                    getCurseLevel( nextConnection->email );
                                }
                            

                            if( requireClientPassword &&
                                ! nextConnection->error  ) {

                                char *trueHash = 
                                    hmac_sha1( 
                                        clientPassword, 
                                        nextConnection->sequenceNumberString );
                                

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
                                    "&string_to_hash=%s",
                                    ticketServerURL,
                                    encodedEmail,
                                    keyHash,
                                    nextConnection->sequenceNumberString );

                                delete [] encodedEmail;

                                nextConnection->ticketServerRequest =
                                    new WebRequest( "GET", url, NULL );
                                
                                nextConnection->ticketServerRequestStartTime
                                    = currentTime;

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
                                    
                                    delete nextConnection->ticketServerRequest;
                                    nextConnection->ticketServerRequest = NULL;
                                    
                                    delete [] 
                                        nextConnection->sequenceNumberString;
                                    nextConnection->sequenceNumberString = NULL;


                                    if( nextConnection->twinCode != NULL
                                        && 
                                        nextConnection->twinCount > 0 ) {
                                        processWaitingTwinConnection(
                                            *nextConnection );
                                        }
                                    else {
                                        if( nextConnection->twinCode != NULL ) {
                                            delete [] nextConnection->twinCode;
                                            nextConnection->twinCode = NULL;
                                            }
                                        processLoggedInPlayer( 
                                            nextConnection->sock,
                                            nextConnection->sockBuffer,
                                            nextConnection->email,
                                            nextConnection->tutorialNumber,
                                            nextConnection->curseStatus );
                                        }
                                                                        
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
            


        // make sure all twin-waiting sockets are still connected
        for( int i=0; i<waitingForTwinConnections.size(); i++ ) {
            FreshConnection *nextConnection = 
                waitingForTwinConnections.getElement( i );
            
            char result = 
                readSocketFull( nextConnection->sock,
                                nextConnection->sockBuffer );
            
            if( ! result ) {
                AppLog::info( "Failed to read from twin-waiting client socket, "
                              "client rejected." );
                nextConnection->error = true;
                nextConnection->errorCauseString =
                    "Socket read failed";
                }
            }
            
        

        // now clean up any new connections that have errors
        
        // FreshConnections are in two different lists
        // clean up errors in both
        SimpleVector<FreshConnection> *connectionLists[2] =
            { &newConnections, &waitingForTwinConnections };
        for( int c=0; c<2; c++ ) {
            SimpleVector<FreshConnection> *list = connectionLists[c];
        
            for( int i=0; i<list->size(); i++ ) {
            
                FreshConnection *nextConnection = list->getElement( i );
            
                if( nextConnection->error ) {
                
                    // try sending REJECTED message at end

                    const char *message = "REJECTED\n#";
                    nextConnection->sock->send( (unsigned char*)message,
                                                strlen( message ), 
                                                false, false );

                    AppLog::infoF( "Closing new connection on error "
                                   "(cause: %s)",
                                   nextConnection->errorCauseString );

                    if( nextConnection->sock != NULL ) {
                        sockPoll.removeSocket( nextConnection->sock );
                        }
                    
                    deleteMembers( nextConnection );
                    
                    list->deleteElement( i );
                    i--;
                    }
                }
            }
    

        // step tutorial map load for player at front of line
        
        // 5 ms
        double timeLimit = 0.005;
        
        for( int i=0; i<tutorialLoadingPlayers.size(); i++ ) {
            LiveObject *nextPlayer = tutorialLoadingPlayers.getElement( i );
            
            char moreLeft = loadTutorialStep( &( nextPlayer->tutorialLoad ),
                                              timeLimit );
            
            if( moreLeft ) {
                // only load one step from first in line
                break;
                }
            
            // first in line is done
            
            AppLog::infoF( "New player %s tutorial loaded after %u steps, "
                           "%f total sec (loadID = %u )",
                           nextPlayer->email,
                           nextPlayer->tutorialLoad.stepCount,
                           Time::getCurrentTime() - 
                           nextPlayer->tutorialLoad.startTime,
                           nextPlayer->tutorialLoad.uniqueLoadID );

            // remove it and any twins
            unsigned int uniqueID = nextPlayer->tutorialLoad.uniqueLoadID;
            

            players.push_back( *nextPlayer );

            tutorialLoadingPlayers.deleteElement( i );
            
            LiveObject *twinPlayer = NULL;
            
            if( i < tutorialLoadingPlayers.size() ) {
                twinPlayer = tutorialLoadingPlayers.getElement( i );
                }
            
            while( twinPlayer != NULL && 
                   twinPlayer->tutorialLoad.uniqueLoadID == uniqueID ) {
                
                AppLog::infoF( "Twin %s tutorial loaded too (loadID = %u )",
                               twinPlayer->email,
                               uniqueID );
            
                players.push_back( *twinPlayer );

                tutorialLoadingPlayers.deleteElement( i );
                
                twinPlayer = NULL;
                
                if( i < tutorialLoadingPlayers.size() ) {
                    twinPlayer = tutorialLoadingPlayers.getElement( i );
                    }
                }
            break;
            
            }
        


        
    
        someClientMessageReceived = false;

        numLive = players.size();
        

        // listen for any messages from clients 

        // track index of each player that needs an update sent about it
        // we compose the full update message below
        SimpleVector<int> playerIndicesToSendUpdatesAbout;
        
        SimpleVector<int> playerIndicesToSendLineageAbout;

        SimpleVector<int> playerIndicesToSendCursesAbout;

        SimpleVector<int> playerIndicesToSendNamesAbout;

        SimpleVector<int> playerIndicesToSendDyingAbout;

        SimpleVector<int> playerIndicesToSendHealingAbout;

        SimpleVector<GraveInfo> newGraves;
        SimpleVector<GraveMoveInfo> newGraveMoves;

        SimpleVector<int> newEmotPlayerIDs;
        SimpleVector<int> newEmotIndices;
        // 0 if no ttl specified
        SimpleVector<int> newEmotTTLs;


        SimpleVector<UpdateRecord> newUpdates;
        SimpleVector<ChangePosition> newUpdatesPos;
        SimpleVector<int> newUpdatePlayerIDs;


        // these are global, so they're not tagged with positions for
        // spatial filtering
        SimpleVector<UpdateRecord> newDeleteUpdates;
        

        SimpleVector<MapChangeRecord> mapChanges;
        SimpleVector<ChangePosition> mapChangesPos;
        

        SimpleVector<FlightDest> newFlightDest;
        


        
        timeSec_t curLookTime = Time::timeSec();
        
        for( int i=0; i<numLive; i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            
            nextPlayer->updateSent = false;

            if( nextPlayer->error ) {
                continue;
                }            

            double curCrossTime = Time::getCurrentTime();

            char checkCrossing = true;
            
            if( curCrossTime < nextPlayer->playerCrossingCheckTime +
                playerCrossingCheckStepTime ) {
                // player not due for another check yet
                checkCrossing = false;
                }
            else {
                // time for next check for this player
                nextPlayer->playerCrossingCheckTime = curCrossTime;
                checkCrossing = true;
                }
            
            
            if( checkCrossing ) {
                GridPos curPos = { nextPlayer->xd, nextPlayer->yd };
            
                if( nextPlayer->xd != nextPlayer->xs ||
                    nextPlayer->yd != nextPlayer->ys ) {
                
                    curPos = computePartialMoveSpot( nextPlayer );
                    }
            
                int curOverID = getMapObject( curPos.x, curPos.y );
            

                if( ! nextPlayer->heldByOther &&
                    curOverID != 0 && 
                    ! isMapObjectInTransit( curPos.x, curPos.y ) &&
                    ! wasRecentlyDeadly( curPos ) ) {
                
                    ObjectRecord *curOverObj = getObject( curOverID );
                
                    char riding = false;
                
                    if( nextPlayer->holdingID > 0 && 
                        getObject( nextPlayer->holdingID )->rideable ) {
                        riding = true;
                        }

                    if( !riding &&
                        curOverObj->permanent && 
                        curOverObj->deadlyDistance > 0 ) {
                    
                        char wasSick = false;
                                        
                        if( nextPlayer->holdingID > 0 &&
                            strstr(
                                getObject( nextPlayer->holdingID )->
                                description,
                                "sick" ) != NULL ) {
                            wasSick = true;
                            }


                        addDeadlyMapSpot( curPos );
                    
                        setDeathReason( nextPlayer, 
                                        "killed",
                                        curOverID );
                    
                        nextPlayer->deathSourceID = curOverID;
                    
                        if( curOverObj->isUseDummy ) {
                            nextPlayer->deathSourceID = 
                                curOverObj->useDummyParent;
                            }

                        nextPlayer->errorCauseString =
                            "Player killed by permanent object";
                    
                        if( ! nextPlayer->dying || wasSick ) {
                            // if was sick, they had a long stagger
                            // time set, so cutting it in half makes no sense
                        
                            int staggerTime = 
                                SettingsManager::getIntSetting(
                                    "deathStaggerTime", 20 );
                        
                            double currentTime = 
                                Time::getCurrentTime();
                        
                            nextPlayer->dying = true;
                            nextPlayer->dyingETA = 
                                currentTime + staggerTime;

                            playerIndicesToSendDyingAbout.
                                push_back( 
                                    getLiveObjectIndex( 
                                        nextPlayer->id ) );
                            }
                        else {
                            // already dying, and getting attacked again
                        
                            // halve their remaining stagger time
                            double currentTime = 
                                Time::getCurrentTime();
                        
                            double staggerTimeLeft = 
                                nextPlayer->dyingETA - currentTime;
                        
                            if( staggerTimeLeft > 0 ) {
                                staggerTimeLeft /= 2;
                                nextPlayer->dyingETA = 
                                    currentTime + staggerTimeLeft;
                                }
                            }
                
                    
                        // generic on-person
                        TransRecord *r = 
                            getPTrans( curOverID, 0 );

                        if( r != NULL ) {
                            setMapObject( curPos.x, curPos.y, r->newActor );

                            // new target specifies wound
                            // but never replace an existing wound
                            // death time is shortened above
                            // however, wounds can replace sickness 
                            if( r->newTarget > 0 &&
                                ( ! nextPlayer->holdingWound || wasSick ) ) {
                                // don't drop their wound
                                if( nextPlayer->holdingID != 0 &&
                                    ! nextPlayer->holdingWound ) {
                                    handleDrop( 
                                        curPos.x, curPos.y, 
                                        nextPlayer,
                                        &playerIndicesToSendUpdatesAbout );
                                    }
                                nextPlayer->holdingID = 
                                    r->newTarget;
                                holdingSomethingNew( nextPlayer );
                            
                                setFreshEtaDecayForHeld( nextPlayer );
                            
                                char isSick = false;
                            
                                if( strstr(
                                        getObject( nextPlayer->holdingID )->
                                        description,
                                        "sick" ) != NULL ) {
                                    isSick = true;

                                    // sicknesses override basic death-stagger
                                    // time.  The person can live forever
                                    // if they are taken care of until
                                    // the sickness passes
                                
                                    int staggerTime = 
                                        SettingsManager::getIntSetting(
                                            "deathStaggerTime", 20 );
                                
                                    double currentTime = 
                                        Time::getCurrentTime();

                                    // 10x base stagger time should
                                    // give them enough time to either heal
                                    // from the disease or die from its
                                    // side-effects
                                    nextPlayer->dyingETA = 
                                        currentTime + 10 * staggerTime;
                                    }

                                if( isSick ) {
                                    // what they have will heal on its own 
                                    // with time.  Sickness, not wound.
                                
                                    // death source is sickness, not
                                    // source
                                    nextPlayer->deathSourceID = 
                                        nextPlayer->holdingID;
                                
                                    setDeathReason( nextPlayer, 
                                                    "succumbed",
                                                    nextPlayer->holdingID );
                                    }
                            
                                nextPlayer->holdingWound = true;
                            
                                ForcedEffects e = 
                                    checkForForcedEffects( 
                                        nextPlayer->holdingID );
                            
                                if( e.emotIndex != -1 ) {
                                    nextPlayer->emotFrozen = true;
                                    newEmotPlayerIDs.push_back( 
                                        nextPlayer->id );
                                    newEmotIndices.push_back( e.emotIndex );
                                    newEmotTTLs.push_back( e.ttlSec );
                                    }
                                if( e.foodModifierSet && 
                                    e.foodCapModifier != 1 ) {
                                
                                    nextPlayer->foodCapModifier = 
                                        e.foodCapModifier;
                                    nextPlayer->foodUpdate = true;
                                    }
                                if( e.feverSet ) {
                                    nextPlayer->fever = e.fever;
                                    }
                            

                                playerIndicesToSendUpdatesAbout.
                                    push_back( 
                                        getLiveObjectIndex( 
                                            nextPlayer->id ) );
                                }
                            }
                        }
                    else if( riding && 
                             curOverObj->permanent && 
                             curOverObj->deadlyDistance > 0 ) {
                        // rode over something deadly
                        // see if it affects what we're riding

                        TransRecord *r = 
                            getPTrans( nextPlayer->holdingID, curOverID );
                    
                        if( r != NULL ) {
                            handleHoldingChange( nextPlayer,
                                                 r->newActor );
                            nextPlayer->heldTransitionSourceID = curOverID;
                            playerIndicesToSendUpdatesAbout.push_back( i );

                            setMapObject( curPos.x, curPos.y, r->newTarget );

                            // it attacked their vehicle 
                            // put it on cooldown so it won't immediately
                            // attack them
                            addDeadlyMapSpot( curPos );
                            }
                        }                
                    }
                }
            
            
            if( curLookTime - nextPlayer->lastRegionLookTime > 5 ) {
                lookAtRegion( nextPlayer->xd - 8, nextPlayer->yd - 7,
                              nextPlayer->xd + 8, nextPlayer->yd + 7 );
                nextPlayer->lastRegionLookTime = curLookTime;
                }

            char *message = NULL;
            
            if( nextPlayer->connected ) {    
                char result = 
                    readSocketFull( nextPlayer->sock, nextPlayer->sockBuffer );
            
                if( ! result ) {
                    setPlayerDisconnected( nextPlayer, "Socket read failed" );
                    }
                else {
                    // don't even bother parsing message buffer for players
                    // that are not currently connected
                    message = getNextClientMessage( nextPlayer->sockBuffer );
                    }
                }
            
            
            if( message != NULL ) {
                someClientMessageReceived = true;
                
                AppLog::infoF( "Got client message from %d: %s",
                               nextPlayer->id, message );
                
                ClientMessage m = parseMessage( nextPlayer, message );
                
                delete [] message;
                
                if( m.type == UNKNOWN ) {
                    AppLog::info( "Client error, unknown message type." );
                    
                    setPlayerDisconnected( nextPlayer, 
                                           "Unknown message type" );
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
                    

                    if( allow && nextPlayer->connected ) {
                        int length;
                        unsigned char *mapChunkMessage = 
                            getChunkMessage( m.x - chunkDimensionX / 2, 
                                             m.y - chunkDimensionY / 2,
                                             chunkDimensionX,
                                             chunkDimensionY,
                                             nextPlayer->birthPos,
                                             &length );
                        
                        int numSent = 
                            nextPlayer->sock->send( mapChunkMessage, 
                                                    length, 
                                                    false, false );
                        
                        nextPlayer->gotPartOfThisFrame = true;
                        
                        delete [] mapChunkMessage;

                        if( numSent != length ) {
                            setPlayerDisconnected( nextPlayer, 
                                                   "Socket write failed" );
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
                else if( m.type == FORCE ) {
                    if( m.x == nextPlayer->xd &&
                        m.y == nextPlayer->yd ) {
                        
                        // player has ack'ed their forced pos correctly
                        
                        // stop ignoring their messages now
                        nextPlayer->waitingForForceResponse = false;
                        }
                    else {
                        AppLog::infoF( 
                            "FORCE message has unexpected "
                            "absolute pos (%d,%d), expecting (%d,%d)",
                            m.x, m.y,
                            nextPlayer->xd, nextPlayer->yd );
                        }
                    }
                else if( m.type == PING ) {
                    // immediately send pong
                    char *message = autoSprintf( "PONG\n%d#", m.id );

                    sendMessageToPlayer( nextPlayer, message, 
                                         strlen( message ) );
                    delete [] message;
                    }
                else if( m.type == DIE ) {
                    if( computeAge( nextPlayer ) < 2 ) {
                        
                        // killed self
                        // SID triggers a lineage ban
                        nextPlayer->suicide = true;

                        setDeathReason( nextPlayer, "SID" );

                        nextPlayer->error = true;
                        nextPlayer->errorCauseString = "Baby suicide";
                        int parentID = nextPlayer->parentID;
                        
                        LiveObject *parentO = 
                            getLiveObject( parentID );
                        
                        if( parentO != NULL && nextPlayer->everHeldByParent ) {
                            // mother picked up this SID baby at least
                            // one time
                            // mother can have another baby right away
                            parentO->birthCoolDown = 0;
                            }
                        
                        
                        int holdingAdultID = nextPlayer->heldByOtherID;

                        LiveObject *adult = NULL;
                        if( nextPlayer->heldByOther ) {
                            adult = getLiveObject( holdingAdultID );
                            }

                        if( adult != NULL ) {
                            
                            int babyBonesID = 
                                SettingsManager::getIntSetting( 
                                    "babyBones", -1 );

                            if( babyBonesID != -1 ) {
                                ObjectRecord *babyBonesO = 
                                    getObject( babyBonesID );
                                
                                if( babyBonesO != NULL ) {
                                    
                                    // don't leave grave on ground just yet
                                    nextPlayer->customGraveID = 0;
                            
                                    GridPos adultPos = 
                                        getPlayerPos( adult );

                                    // put invisible grave there for now
                                    GraveInfo graveInfo = { adultPos, 
                                                            nextPlayer->id };
                                    newGraves.push_back( graveInfo );
                                    
                                    adult->heldGraveOriginX = adultPos.x;
                                    
                                    adult->heldGraveOriginY = adultPos.y;
                                 
                                    playerIndicesToSendUpdatesAbout.push_back(
                                        getLiveObjectIndex( holdingAdultID ) );
                                    
                                    // what if baby wearing clothes?
                                    for( int c=0; 
                                         c < NUM_CLOTHING_PIECES; 
                                         c++ ) {
                                             
                                        ObjectRecord *cObj = clothingByIndex(
                                            nextPlayer->clothing, c );
                                        
                                        if( cObj != NULL ) {
                                            // put clothing in adult's hand
                                            // and then drop
                                            adult->holdingID = cObj->id;
                                            if( nextPlayer->
                                                clothingContained[c].
                                                size() > 0 ) {
                                                
                                                adult->numContained =
                                                    nextPlayer->
                                                    clothingContained[c].
                                                    size();
                                                
                                                adult->containedIDs =
                                                    nextPlayer->
                                                    clothingContained[c].
                                                    getElementArray();
                                                adult->containedEtaDecays =
                                                    nextPlayer->
                                                    clothingContainedEtaDecays
                                                    [c].
                                                    getElementArray();
                                                
                                                adult->subContainedIDs
                                                    = new 
                                                    SimpleVector<int>[
                                                    adult->numContained ];
                                                adult->subContainedEtaDecays
                                                    = new 
                                                    SimpleVector<timeSec_t>[
                                                    adult->numContained ];
                                                }
                                            
                                            handleDrop( 
                                                adultPos.x, adultPos.y, 
                                                adult,
                                                NULL );
                                            }
                                        }
                                    
                                    // finally leave baby bones
                                    // in their hands
                                    adult->holdingID = babyBonesID;
                                    
                                    // this works to force client to play
                                    // creation sound for baby bones.
                                    adult->heldTransitionSourceID = 
                                        nextPlayer->displayID;
                                    
                                    nextPlayer->heldByOther = false;
                                    }
                                }
                            }
                        // else let normal grave appear for this dead baby
                        }
                    }
                else if( m.type != SAY && m.type != EMOT &&
                         nextPlayer->waitingForForceResponse ) {
                    // if we're waiting for a FORCE response, ignore
                    // all messages from player except SAY and EMOT
                    
                    AppLog::infoF( "Ignoring client message because we're "
                                   "waiting for FORCE ack message after a "
                                   "forced-pos PU at (%d, %d), "
                                   "relative=(%d, %d)",
                                   nextPlayer->xd, nextPlayer->yd,
                                   nextPlayer->xd - nextPlayer->birthPos.x,
                                   nextPlayer->yd - nextPlayer->birthPos.y );
                    }
                // if player is still moving (or held by an adult), 
                // ignore all actions
                // except for move interrupts
                else if( ( nextPlayer->xs == nextPlayer->xd &&
                           nextPlayer->ys == nextPlayer->yd &&
                           ! nextPlayer->heldByOther )
                         ||
                         m.type == MOVE ||
                         m.type == JUMP || 
                         m.type == SAY ||
                         m.type == EMOT ) {

                    if( m.type == MOVE &&
                        m.sequenceNumber != -1 ) {
                        nextPlayer->lastMoveSequenceNumber = m.sequenceNumber;
                        }

                    if( ( m.type == MOVE || m.type == JUMP ) && 
                        nextPlayer->heldByOther ) {
                        
                        // only JUMP actually makes them jump out
                        if( m.type == JUMP ) {
                            // baby wiggling out of parent's arms
                            
                            // block them from wiggling from their own 
                            // mother's arms if they are under 1
                            
                            if( computeAge( nextPlayer ) >= 1  ||
                                nextPlayer->heldByOtherID != 
                                nextPlayer->parentID ) {
                                
                                handleForcedBabyDrop( 
                                    nextPlayer,
                                    &playerIndicesToSendUpdatesAbout );
                                }
                            }
                        
                        // ignore their move requests while
                        // in-arms, until they JUMP out
                        }
                    else if( m.type == MOVE && nextPlayer->holdingID > 0 &&
                             getObject( nextPlayer->holdingID )->
                             speedMult == 0 ) {
                        // next player holding something that prevents
                        // movement entirely
                        printf( "  Processing move, "
                                "but player holding a speed-0 object, "
                                "ending now\n" );
                        nextPlayer->xd = nextPlayer->xs;
                        nextPlayer->yd = nextPlayer->ys;
                        
                        nextPlayer->posForced = true;
                        
                        // send update about them to end the move
                        // right now
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        }
                    else if( m.type == MOVE ) {
                        //Thread::staticSleep( 1000 );

                        /*
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
                        */
                        
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
                            
                            /*
                            printf( "   we think player in motion or "
                                    "done moving at %d,%d\n",
                                    cPos.x,
                                    cPos.y );
                            */
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
                                
                                /*
                                printf( "They are on our path at index %d\n",
                                        theirPathIndex );
                                */

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

                                /*
                                printf( "Prefixing their path "
                                        "with %d steps\n",
                                        unfilteredPath.size() );
                                */
                                }
                            }
                        
                        if( unfilteredPath.size() > 0 ) {
                            pathPrefixAdded = true;
                            }

                        // now add path player says they want to go down

                        for( int p=0; p < m.numExtraPos; p++ ) {
                            unfilteredPath.push_back( m.extraPos[p] );
                            }
                        
                        /*
                        printf( "Unfiltered path = " );
                        for( int p=0; p<unfilteredPath.size(); p++ ) {
                            printf( "(%d, %d) ",
                                    unfilteredPath.getElementDirect(p).x, 
                                    unfilteredPath.getElementDirect(p).y );
                            }
                        printf( "\n" );
                        */

                        // remove any duplicate spots due to doubling back

                        for( int p=1; p<unfilteredPath.size(); p++ ) {
                            
                            if( equal( unfilteredPath.getElementDirect(p-1),
                                       unfilteredPath.getElementDirect(p) ) ) {
                                unfilteredPath.deleteElement( p );
                                p--;
                                //printf( "FOUND duplicate path element\n" );
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
                            /*
                            printf( "A move that takes player "
                                    "where they already are, "
                                    "ending move now\n" );
                            */
                            }
                        else {
                            // an actual move away from current xs,ys

                            if( interrupt ) {
                                //printf( "Got valid move interrupt\n" );
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
                            /*
                            printf( "Start index = %d (startFound = %d)\n", 
                                    startIndex, startFound );
                            */

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
                                    nextPlayer ) *
                                    getPathSpeedModifier( 
                                        nextPlayer->pathToDest,
                                        nextPlayer->pathLength );
                                
                                nextPlayer->moveTotalSeconds = dist / 
                                    moveSpeed;
                           
                                double secondsAlreadyDone = distAlreadyDone / 
                                    moveSpeed;
                                /*
                                printf( "Skipping %f seconds along new %f-"
                                        "second path\n",
                                        secondsAlreadyDone, 
                                        nextPlayer->moveTotalSeconds );
                                */
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

                        int len = strlen( m.saidText );
                        
                        // replace not-allowed characters with spaces
                        for( int c=0; c<len; c++ ) {
                            if( ! allowedSayCharMap[ 
                                    (int)( m.saidText[c] ) ] ) {
                                
                                m.saidText[c] = ' ';
                                }
                            }
                        


                        
                        if( nextPlayer->isEve && nextPlayer->name == NULL ) {
                            char *name = isFamilyNamingSay( m.saidText );
                            
                            if( name != NULL && strcmp( name, "" ) != 0 ) {
                                const char *close = findCloseLastName( name );
                                nextPlayer->name = autoSprintf( "%s %s",
                                                                eveName, 
                                                                close );
                                nextPlayer->name = getUniqueCursableName( 
                                    nextPlayer->name, 
                                    &( nextPlayer->nameHasSuffix ) );

                                logName( nextPlayer->id,
                                         nextPlayer->email,
                                         nextPlayer->name );
                                playerIndicesToSendNamesAbout.push_back( i );
                                }
                            }

                        if( nextPlayer->holdingID < 0 ) {

                            // we're holding a baby
                            // (no longer matters if it's our own baby)
                            // (we let adoptive parents name too)
                            
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
                                        else if( nextPlayer->nameHasSuffix ) {
                                            // only keep last name
                                            // if it contains another
                                            // space (the suffix is after
                                            // the last name).  Otherwise
                                            // we are probably confused,
                                            // and what we think
                                            // is the last name IS the suffix.
                                            
                                            char *suffixPos =
                                                strstr( (char*)&( lastName[1] ),
                                                        " " );
                                            
                                            if( suffixPos == NULL ) {
                                                // last name is suffix, actually
                                                // don't pass suffix on to baby
                                                lastName = "";
                                                }
                                            else {
                                                // last name plus suffix
                                                // okay to pass to baby
                                                // because we strip off
                                                // third part of name
                                                // (suffix) below.
                                                }
                                            }
                                        }

                                    const char *close = 
                                        findCloseFirstName( name );
                                    babyO->name = autoSprintf( "%s%s",
                                                               close, 
                                                               lastName );

                                    int spaceCount = 0;
                                    int lastSpaceIndex = -1;

                                    int nameLen = strlen( babyO->name );
                                    for( int s=0; s<nameLen; s++ ) {
                                        if( babyO->name[s] == ' ' ) {
                                            lastSpaceIndex = s;
                                            spaceCount++;
                                            }
                                        }
                                    
                                    if( spaceCount > 1 ) {
                                        // remove suffix from end
                                        babyO->name[ lastSpaceIndex ] = '\0';
                                        }
                                    
                                    babyO->name = getUniqueCursableName( 
                                        babyO->name, 
                                        &( babyO->nameHasSuffix ) );
                                    
                                    logName( babyO->id,
                                             babyO->email,
                                             babyO->name );
                                    
                                    playerIndicesToSendNamesAbout.push_back( 
                                        getLiveObjectIndex( babyO->id ) );
                                    }
                                }
                            }
                        else {
                            // not holding anyone
                        
                            char *name = isBabyNamingSay( m.saidText );
                                
                            if( name != NULL && strcmp( name, "" ) != 0 ) {
                                // still, check if we're naming a nearby,
                                // nameless non-baby
                                GridPos thisPos = getPlayerPos( nextPlayer );
                                
                                // don't consider anyone who is too far away
                                double closestDist = 20;
                                LiveObject *closestOther = NULL;
                                
                                for( int j=0; j<numLive; j++ ) {
                                    LiveObject *otherPlayer = 
                                        players.getElement(j);
                                    
                                    if( otherPlayer != nextPlayer &&
                                        computeAge( otherPlayer ) >= babyAge &&
                                        otherPlayer->name == NULL ) {
                                        
                                        GridPos otherPos = 
                                            getPlayerPos( otherPlayer );
                                        
                                        double dist =
                                            distance( thisPos, otherPos );
                                        
                                        if( dist < closestDist ) {
                                            closestDist = dist;
                                            closestOther = otherPlayer;
                                            }
                                        }
                                    }
                                if( closestOther != NULL ) {
                                    const char *close = 
                                        findCloseFirstName( name );
                                    
                                    closestOther->name = 
                                        stringDuplicate( close );
                                    
                                    closestOther->name = getUniqueCursableName( 
                                        closestOther->name,
                                        &( closestOther->nameHasSuffix ) );

                                    logName( closestOther->id,
                                             closestOther->email,
                                             closestOther->name );
                                    
                                    playerIndicesToSendNamesAbout.push_back( 
                                        getLiveObjectIndex( 
                                            closestOther->id ) );
                                    }
                                }

                            // also check if we're holding something writable
                            unsigned char metaData[ MAP_METADATA_LENGTH ];
                            int len = strlen( m.saidText );
                            
                            if( nextPlayer->holdingID > 0 &&
                                len < MAP_METADATA_LENGTH &&
                                getObject( 
                                    nextPlayer->holdingID )->writable &&
                                // and no metadata already on it
                                ! getMetadata( nextPlayer->holdingID, 
                                               metaData ) ) {

                                memset( metaData, 0, MAP_METADATA_LENGTH );
                                memcpy( metaData, m.saidText, len + 1 );
                                
                                nextPlayer->holdingID = 
                                    addMetadata( nextPlayer->holdingID,
                                                 metaData );

                                TransRecord *writingHappenTrans =
                                    getMetaTrans( 0, nextPlayer->holdingID );
                                
                                if( writingHappenTrans != NULL &&
                                    writingHappenTrans->newTarget > 0 &&
                                    getObject( writingHappenTrans->newTarget )
                                        ->written ) {
                                    // bare hands transition going from
                                    // writable to written
                                    // use this to transform object in 
                                    // hands as we write
                                    handleHoldingChange( 
                                        nextPlayer,
                                        writingHappenTrans->newTarget );
                                    playerIndicesToSendUpdatesAbout.
                                        push_back( i );
                                    }                    
                                }    
                            }
                        
                        makePlayerSay( nextPlayer, m.saidText );
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
                                        getHitPlayer( m.x, m.y, m.id, true );
                                    
                                    char someoneHit = false;

                                    if( hitPlayer != NULL ) {
                                        someoneHit = true;
                                        // break the connection with 
                                        // them, eventually
                                        // let them stagger a bit first

                                        hitPlayer->murderSourceID =
                                            nextPlayer->holdingID;
                                        
                                        hitPlayer->murderPerpID =
                                            nextPlayer->id;
                                        
                                        // brand this player as a murderer
                                        nextPlayer->everKilledAnyone = true;

                                        if( hitPlayer->murderPerpEmail 
                                            != NULL ) {
                                            delete [] 
                                                hitPlayer->murderPerpEmail;
                                            }
                                        
                                        hitPlayer->murderPerpEmail =
                                            stringDuplicate( 
                                                nextPlayer->email );
                                        

                                        setDeathReason( hitPlayer, 
                                                        "killed",
                                                        nextPlayer->holdingID );

                                        // if not already dying
                                        if( ! hitPlayer->dying ) {
                                            int staggerTime = 
                                                SettingsManager::getIntSetting(
                                                    "deathStaggerTime", 20 );
                                            
                                            double currentTime = 
                                                Time::getCurrentTime();
                                            
                                            hitPlayer->dying = true;
                                            hitPlayer->dyingETA = 
                                                currentTime + staggerTime;

                                            playerIndicesToSendDyingAbout.
                                                push_back( 
                                                    getLiveObjectIndex( 
                                                        hitPlayer->id ) );
                                        
                                            hitPlayer->errorCauseString =
                                                "Player killed by other player";
                                            }
                                         else {
                                             // already dying, 
                                             // and getting attacked again
                        
                                             // halve their remaining 
                                             // stagger time
                                             double currentTime = 
                                                 Time::getCurrentTime();
                                             
                                             double staggerTimeLeft = 
                                                 hitPlayer->dyingETA - 
                                                 currentTime;
                        
                                             if( staggerTimeLeft > 0 ) {
                                                 staggerTimeLeft /= 2;
                                                 hitPlayer->dyingETA = 
                                                     currentTime + 
                                                     staggerTimeLeft;
                                                 }
                                             }
                                        }
                                    
                                    
                                    // a player either hit or not
                                    // in either case, weapon was used
                                    
                                    // check for a transition for weapon

                                    // 0 is generic "on person" target
                                    TransRecord *r = 
                                        getPTrans( nextPlayer->holdingID, 
                                                  0 );

                                    TransRecord *rHit = NULL;
                                    TransRecord *woundHit = NULL;
                                    
                                    if( someoneHit ) {
                                        // last use on target specifies
                                        // grave and weapon change on hit
                                        // non-last use (r above) specifies
                                        // what projectile ends up in grave
                                        // or on ground
                                        rHit = 
                                            getPTrans( nextPlayer->holdingID, 
                                                      0, false, true );
                                        
                                        if( rHit != NULL &&
                                            rHit->newTarget > 0 ) {
                                            hitPlayer->customGraveID = 
                                                rHit->newTarget;
                                            }
                                        
                                        char wasSick = false;
                                        
                                        if( hitPlayer->holdingID > 0 &&
                                            strstr(
                                                getObject( 
                                                    hitPlayer->holdingID )->
                                                description,
                                                "sick" ) != NULL ) {
                                            wasSick = true;
                                            }

                                        // last use on actor specifies
                                        // what is left in victim's hand
                                        woundHit = 
                                            getPTrans( nextPlayer->holdingID, 
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

                                            // give them a new wound
                                            // if they don't already have
                                            // one, but never replace their
                                            // original wound.  That allows
                                            // a healing exploit where you
                                            // intentionally give someone
                                            // an easier-to-treat wound
                                            // to replace their hard-to-treat
                                            // wound

                                            // however, do let wounds replace
                                            // sickness
                                            char woundChange = false;
                                            
                                            if( ! hitPlayer->holdingWound ||
                                                wasSick ) {
                                                woundChange = true;
                                                
                                                hitPlayer->holdingID = 
                                                    woundHit->newTarget;
                                                holdingSomethingNew( 
                                                    hitPlayer );
                                                setFreshEtaDecayForHeld( 
                                                    hitPlayer );
                                                }
                                            
                                            
                                            hitPlayer->holdingWound = true;
                                            
                                            if( woundChange ) {
                                                
                                                ForcedEffects e = 
                                                    checkForForcedEffects( 
                                                        hitPlayer->holdingID );
                            
                                                if( e.emotIndex != -1 ) {
                                                    hitPlayer->emotFrozen = 
                                                        true;
                                                    newEmotPlayerIDs.push_back( 
                                                        hitPlayer->id );
                                                    newEmotIndices.push_back( 
                                                        e.emotIndex );
                                                    newEmotTTLs.push_back( 
                                                        e.ttlSec );
                                                    }
                                            
                                                if( e.foodModifierSet && 
                                                    e.foodCapModifier != 1 ) {
                                                
                                                    hitPlayer->
                                                        foodCapModifier = 
                                                        e.foodCapModifier;
                                                    hitPlayer->foodUpdate = 
                                                        true;
                                                    }
                                                
                                                if( e.feverSet ) {
                                                    hitPlayer->fever = e.fever;
                                                    }


                                                playerIndicesToSendUpdatesAbout.
                                                    push_back( 
                                                        getLiveObjectIndex( 
                                                            hitPlayer->id ) );
                                                }   
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
                                        holdingSomethingNew( nextPlayer,
                                                             oldHolding);
                                        }
                                    else if( woundHit != NULL ) {
                                        // result of hit on held weapon 
                                        // could also be
                                        // specified in wound trans
                                        nextPlayer->holdingID = 
                                            woundHit->newActor;
                                        holdingSomethingNew( nextPlayer,
                                                             oldHolding);
                                        }
                                    else if( r != NULL ) {
                                        nextPlayer->holdingID = r->newActor;
                                        holdingSomethingNew( nextPlayer,
                                                             oldHolding );
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
                                                    getMetaTrans( 
                                                        -1, 
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
                        
                        // track whether this USE resulted in something
                        // new on the ground in case of placing a grave
                        int newGroundObject = -1;
                        GridPos newGroundObjectOrigin =
                            { nextPlayer->heldGraveOriginX,
                              nextPlayer->heldGraveOriginY };
                        

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
                            
                            char wrongSide = false;
                            
                            if( target != 0 &&
                                isGridAdjacent( m.x, m.y,
                                                nextPlayer->xd, 
                                                nextPlayer->yd ) ) {
                                ObjectRecord *targetObj = getObject( target );

                                if( targetObj->sideAccess ) {
                                    
                                    if( m.y > nextPlayer->yd ||
                                        m.y < nextPlayer->yd ) {
                                        // access from N or S
                                        wrongSide = true;
                                        }
                                    }
                                }
                            

                            
                            if( wrongSide ) {
                                // ignore action from wrong side
                                }
                            else if( target != 0 ) {
                                ObjectRecord *targetObj = getObject( target );
                                
                                // try using object on this target 
                                
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

                                    r = getPTrans( nextPlayer->holdingID,
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

                                    if( r == NULL ) {
                                        // no transition applies for this
                                        // held, whether full or empty
                                        
                                        // let it be used anyway, so
                                        // that generic transition (below)
                                        // might apply
                                        heldCanBeUsed = true;
                                        }
                                    
                                    r = NULL;
                                    }
                                

                                if( nextPlayer->holdingID >= 0 &&
                                    heldCanBeUsed ) {
                                    // negative holding is ID of baby
                                    // which can't be used
                                    // (and no bare hand action available)
                                    r = getPTrans( nextPlayer->holdingID,
                                                  target );
                                    }
                                
                                if( r == NULL && 
                                    ( nextPlayer->holdingID != 0 || 
                                      targetObj->permanent ) ) {
                                    
                                    // search for default 
                                    r = getPTrans( -2, target );
                                        
                                    if( r != NULL ) {
                                        defaultTrans = true;
                                        }
                                    else if( nextPlayer->holdingID <= 0 || 
                                             targetObj->numSlots == 0 ) {
                                        // also consider bare-hand
                                        // action that produces
                                        // no new held item
                                        
                                        // but only on non-container
                                        // objects (example:  we don't
                                        // want to kick minecart into
                                        // motion every time we try
                                        // to add something to it)
                                        
                                        // treat this the same as
                                        // default
                                        r = getPTrans( 0, target );
                                        
                                        if( r != NULL && 
                                            r->newActor == 0 ) {
                                            
                                            defaultTrans = true;
                                            }
                                        else {
                                            r = NULL;
                                            }
                                        }
                                    }
                                

                                if( r != NULL &&
                                    r->newTarget > 0 &&
                                    r->newTarget != target ) {
                                    
                                    // target would change here
                                    if( getMapFloor( m.x, m.y ) != 0 ) {
                                        // floor present
                                        
                                        // make sure new target allowed 
                                        // to exist on floor
                                        if( strstr( getObject( r->newTarget )->
                                                    description, 
                                                    "groundOnly" ) != NULL ) {
                                            r = NULL;
                                            }
                                        }
                                    }
                                

                                if( r == NULL && 
                                    nextPlayer->holdingID > 0 ) {
                                    
                                    logTransitionFailure( 
                                        nextPlayer->holdingID,
                                        target );
                                    }
                                
                                double playerAge = computeAge( nextPlayer );
                                

                                if( r != NULL && containmentTransfer ) {
                                    // special case contained items
                                    // moving from actor into new target
                                    // (and hand left empty)
                                    setResponsiblePlayer( - nextPlayer->id );
                                    
                                    setMapObject( m.x, m.y, r->newTarget );
                                    newGroundObject = r->newTarget;
                                    
                                    setResponsiblePlayer( -1 );
                                    
                                    transferHeldContainedToMap( nextPlayer,
                                                                m.x, m.y );
                                    handleHoldingChange( nextPlayer,
                                                         r->newActor );

                                    nextPlayer->heldGraveOriginX = m.x;
                                    nextPlayer->heldGraveOriginY = m.y;
                                    }
                                else if( r != NULL &&
                                    // are we old enough to handle
                                    // what we'd get out of this transition?
                                    ( ( r->newActor == 0 &&
                                        playerAge >= defaultActionAge )
                                      || 
                                      ( r->newActor > 0 &&
                                        getObject( r->newActor )->minPickupAge 
                                        <= 
                                        playerAge ) ) 
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
                                        nextPlayer->heldGraveOriginX = m.x;
                                        nextPlayer->heldGraveOriginY = m.y;
                                    
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
                                        newGroundObject = r->newTarget;
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
                                    
                                    pickupToHold( nextPlayer, m.x, m.y,
                                                  target );
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
                                    
                                    addHeldToContainer(
                                        nextPlayer, target, m.x, m.y );
                                    }
                                

                                if( targetObj->permanent &&
                                    targetObj->foodValue > 0 ) {
                                    
                                    // just touching this object
                                    // causes us to eat from it
                                    
                                    nextPlayer->justAte = true;
                                    nextPlayer->justAteID = 
                                        targetObj->id;

                                    nextPlayer->lastAteID = 
                                        targetObj->id;
                                    nextPlayer->lastAteFillMax =
                                        nextPlayer->foodStore;
                                    
                                    nextPlayer->foodStore += 
                                        targetObj->foodValue;
                                    
                                    updateYum( nextPlayer, targetObj->id );
                                    

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
                                }
                            else if( nextPlayer->holdingID > 0 ) {
                                // target location emtpy
                                // target not where we're standing
                                // we're holding something
                                
                                char usedOnFloor = false;
                                int floorID = getMapFloor( m.x, m.y );
                                
                                if( floorID > 0 ) {
                                    
                                    TransRecord *r = 
                                        getPTrans( nextPlayer->holdingID,
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
                                            
                                            nextPlayer->heldGraveOriginX = m.x;
                                            nextPlayer->heldGraveOriginY = m.y;
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
                                                nextPlayer->
                                                    heldGraveOriginX = m.x;
                                                nextPlayer->
                                                    heldGraveOriginY = m.y;
                                    
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
                                        getPTrans( nextPlayer->holdingID, 
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
                                        nextPlayer->heldTransitionSourceID =
                                            nextPlayer->holdingID;
                                        
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
                                            newGroundObject = r->newTarget;

                                            setResponsiblePlayer( -1 );
                                    
                                            transferHeldContainedToMap( 
                                                nextPlayer, m.x, m.y );
                                            
                                            handleHoldingChange( nextPlayer,
                                                                 r->newActor );
                                            
                                            nextPlayer->heldGraveOriginX = m.x;
                                            nextPlayer->heldGraveOriginY = m.y;
                                            }
                                        else {
                                            handleHoldingChange( nextPlayer,
                                                                 r->newActor );
                                            
                                            nextPlayer->heldGraveOriginX = m.x;
                                            nextPlayer->heldGraveOriginY = m.y;
                                    
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
                                                newGroundObject = r->newTarget;
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


                        if( newGroundObject > 0 ) {

                            ObjectRecord *o = getObject( newGroundObject );
                            
                            if( strstr( o->description, "origGrave" ) 
                                != NULL ) {
                                
                                GraveMoveInfo g = { 
                                    { newGroundObjectOrigin.x,
                                      newGroundObjectOrigin.y },
                                    { m.x,
                                      m.y } };
                                newGraveMoves.push_back( g );
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
                                    getHitPlayer( m.x, m.y, m.id, 
                                                  false, babyAge );
                                
                                if( hitPlayer != NULL &&
                                    !hitPlayer->heldByOther &&
                                    computeAge( hitPlayer ) < babyAge  ) {
                                    
                                    // negative holding IDs to indicate
                                    // holding another player
                                    nextPlayer->holdingID = -hitPlayer->id;
                                    holdingSomethingNew( nextPlayer );
                                    
                                    nextPlayer->holdingEtaDecay = 0;

                                    hitPlayer->heldByOther = true;
                                    hitPlayer->heldByOtherID = nextPlayer->id;
                                    
                                    if( hitPlayer->heldByOtherID ==
                                        hitPlayer->parentID ) {
                                        hitPlayer->everHeldByParent = true;
                                        }
                                    

                                    // force baby to drop what they are
                                    // holding

                                    if( hitPlayer->holdingID != 0 ) {
                                        // never drop held wounds
                                        // they are the only thing a baby can
                                        // while held
                                        if( ! hitPlayer->holdingWound && 
                                            hitPlayer->holdingID > 0 ) {
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
                                        
                                        if( nextPlayer->yummyBonusStore > 0 ) {
                                            nextPlayer->yummyBonusStore -= 
                                                nurseCost;
                                            nurseCost = 0;
                                            if( nextPlayer->yummyBonusStore < 
                                                0 ) {
                                                
                                                // not enough to cover full 
                                                // nurse cost

                                                // pass remaining nurse
                                                // cost onto main food store
                                                nurseCost = - nextPlayer->
                                                    yummyBonusStore;
                                                nextPlayer->yummyBonusStore = 0;
                                                }
                                            }
                                        

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
                                    getHitPlayer( m.x, m.y, m.id,
                                                  false, 
                                                  babyAge, -1, &hitIndex );
                                
                                if( hitPlayer != NULL && holdingDrugs ) {
                                    // can't even feed baby drugs
                                    // too confusing
                                    hitPlayer = NULL;
                                    }

                                if( hitPlayer == NULL ||
                                    hitPlayer == nextPlayer ) {
                                    // try click on elderly
                                    hitPlayer = 
                                        getHitPlayer( m.x, m.y, m.id,
                                                      false, -1, 
                                                      55, &hitIndex );
                                    }
                                
                                if( ( hitPlayer == NULL ||
                                      hitPlayer == nextPlayer )
                                    &&
                                    holdingFood ) {
                                    
                                    // feeding action 
                                    // try click on everyone
                                    hitPlayer = 
                                        getHitPlayer( m.x, m.y, m.id,
                                                      false, -1, -1, 
                                                      &hitIndex );
                                    }
                                
                                
                                if( ( hitPlayer == NULL ||
                                      hitPlayer == nextPlayer )
                                    &&
                                    ! holdingDrugs ) {
                                    
                                    // see if clicked-on player is dying
                                    hitPlayer = 
                                        getHitPlayer( m.x, m.y, m.id,
                                                      false, -1, -1, 
                                                      &hitIndex );
                                    if( hitPlayer != NULL &&
                                        ! hitPlayer->dying ) {
                                        hitPlayer = NULL;
                                        }
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
                            

                            if( targetPlayer != nextPlayer &&
                                targetPlayer->dying &&
                                ! holdingFood ) {
                                
                                // try healing wound
                                    
                                TransRecord *healTrans =
                                    getMetaTrans( nextPlayer->holdingID,
                                                  targetPlayer->holdingID );
                                
                                char oldEnough = true;

                                if( healTrans != NULL ) {
                                    int healerWillHold = healTrans->newActor;
                                    
                                    if( healerWillHold > 0 ) {
                                        if( computeAge( nextPlayer ) < 
                                            getObject( healerWillHold )->
                                            minPickupAge ) {
                                            oldEnough = false;
                                            }
                                        }
                                    }
                                

                                if( oldEnough && healTrans != NULL ) {
                                    targetPlayer->holdingID =
                                        healTrans->newTarget;
                                    holdingSomethingNew( targetPlayer );
                                    
                                    // their wound has been changed
                                    // no longer track embedded weapon
                                    targetPlayer->embeddedWeaponID = 0;
                                    targetPlayer->embeddedWeaponEtaDecay = 0;
                                    
                                    
                                    nextPlayer->holdingID = 
                                        healTrans->newActor;
                                    holdingSomethingNew( nextPlayer );
                                    
                                    setFreshEtaDecayForHeld( 
                                        nextPlayer );
                                    setFreshEtaDecayForHeld( 
                                        targetPlayer );
                                    
                                    nextPlayer->heldOriginValid = 0;
                                    nextPlayer->heldOriginX = 0;
                                    nextPlayer->heldOriginY = 0;
                                    nextPlayer->heldTransitionSourceID = 
                                        healTrans->target;
                                    
                                    targetPlayer->heldOriginValid = 0;
                                    targetPlayer->heldOriginX = 0;
                                    targetPlayer->heldOriginY = 0;
                                    targetPlayer->heldTransitionSourceID = 
                                        -1;
                                    
                                    if( targetPlayer->holdingID == 0 ) {
                                        // not dying anymore
                                        setNoLongerDying( 
                                            targetPlayer,
                                            &playerIndicesToSendHealingAbout );
                                        }
                                    else {
                                        // wound changed?

                                        ForcedEffects e = 
                                            checkForForcedEffects( 
                                                targetPlayer->holdingID );
                            
                                        if( e.emotIndex != -1 ) {
                                            targetPlayer->emotFrozen = true;
                                            newEmotPlayerIDs.push_back( 
                                                targetPlayer->id );
                                            newEmotIndices.push_back( 
                                                e.emotIndex );
                                            newEmotTTLs.push_back( e.ttlSec );
                                            }
                                        if( e.foodCapModifier != 1 ) {
                                            targetPlayer->foodCapModifier = 
                                                e.foodCapModifier;
                                            targetPlayer->foodUpdate = true;
                                            }
                                        if( e.feverSet ) {
                                            targetPlayer->fever = e.fever;
                                            }
                                        }
                                    }
                                }
                            else if( nextPlayer->holdingID > 0 ) {
                                ObjectRecord *obj = 
                                    getObject( nextPlayer->holdingID );
                                
                                int cap = computeFoodCapacity( targetPlayer );
                                

                                // first case:
                                // player clicked on clothing
                                // try adding held into clothing, but if
                                // that fails go on to other cases
                                if( targetPlayer == nextPlayer &&
                                    m.i >= 0 && 
                                    m.i < NUM_CLOTHING_PIECES &&
                                    addHeldToClothingContainer( nextPlayer,
                                                                m.i ) ) {
                                    // worked!
                                    }
                                // next case, holding food
                                // that couldn't be put into clicked clothing
                                else if( obj->foodValue > 0 && 
                                         targetPlayer->foodStore < cap ) {
                                    
                                    targetPlayer->justAte = true;
                                    targetPlayer->justAteID = 
                                        nextPlayer->holdingID;

                                    targetPlayer->lastAteID = 
                                        nextPlayer->holdingID;
                                    targetPlayer->lastAteFillMax =
                                        targetPlayer->foodStore;
                                    
                                    targetPlayer->foodStore += obj->foodValue;
                                    
                                    updateYum( targetPlayer, obj->id );

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
                                        getPTrans( nextPlayer->holdingID, 
                                                  -1 );

                                    

                                    if( r != NULL ) {
                                        int oldHolding = nextPlayer->holdingID;
                                        nextPlayer->holdingID = r->newActor;
                                        holdingSomethingNew( nextPlayer,
                                                             oldHolding );

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
                                // final case, holding clothing that
                                // we could put on
                                else if( obj->clothing != 'n' &&
                                         ( targetPlayer == nextPlayer
                                           || 
                                           computeAge( targetPlayer ) < 
                                           babyAge) ) {
                                    
                                    // wearable, dress self or baby
                                    
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
                                            holdingSomethingNew( nextPlayer );
                                            
                                            nextPlayer->holdingEtaDecay
                                                = oldCEtaDecay;
                                            
                                            nextPlayer->numContained =
                                                oldNumContained;
                                            
                                            freePlayerContainedArrays(
                                                nextPlayer );
                                            
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
                                    holdingSomethingNew( nextPlayer );

                                    *clothingSlot = NULL;
                                    nextPlayer->holdingEtaDecay =
                                        targetPlayer->clothingEtaDecay[ind];
                                    targetPlayer->clothingEtaDecay[ind] = 0;
                                    
                                    nextPlayer->numContained =
                                        targetPlayer->
                                        clothingContained[ind].size();
                                    
                                    freePlayerContainedArrays( nextPlayer );
                                    
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

                        if( ( isGridAdjacent( m.x, m.y,
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
                                else if( canDrop && 
                                         isMapSpotEmpty( m.x, m.y ) ) {
                                
                                    // empty spot to drop non-baby into
                                    
                                    handleDrop( 
                                        m.x, m.y, nextPlayer,
                                        &playerIndicesToSendUpdatesAbout );
                                    }
                                else if( canDrop &&
                                         m.c >= 0 && 
                                         m.c < NUM_CLOTHING_PIECES &&
                                         m.x == nextPlayer->xd &&
                                         m.y == nextPlayer->yd  &&
                                         nextPlayer->holdingID > 0 ) {
                                    
                                    // drop into clothing indicates right-click
                                    // so swap
                                    
                                    int oldHeld = nextPlayer->holdingID;
                                    
                                    // first add to top of container
                                    // if possible
                                    addHeldToClothingContainer( nextPlayer,
                                                                m.c );
                                    if( nextPlayer->holdingID == 0 ) {
                                        // add to top worked

                                        // now take off bottom to hold
                                        // but keep looking to find something
                                        // different than what we were
                                        // holding before
                                        for( int s=0; 
                                             s < nextPlayer->
                                                 clothingContained[m.c].size() 
                                                 - 1;
                                             s++ ) {
                                            
                                            if( nextPlayer->
                                                 clothingContained[m.c].
                                                getElementDirect( s ) != 
                                                oldHeld ) {
                                                
                                              removeFromClothingContainerToHold(
                                                    nextPlayer, m.c, s );
                                                break;
                                                }
                                            }
                                        }
                                    
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
                                        

                                        if( !canDrop ) {
                                            // user may have a permanent object
                                            // stuck in their hand with no place
                                            // to drop it
                                            
                                            // need to check if 
                                            // a use-on-bare-ground
                                            // transition applies.  If so, we
                                            // can treat it like a swap

                                    
                                            if( ! targetObj->permanent ) {
                                                // target can be picked up

                                                // "set-down" type bare ground 
                                                // trans exists?
                                                TransRecord
                                                *r = getPTrans( 
                                                    nextPlayer->holdingID, 
                                                    -1 );

                                                if( r != NULL && 
                                                    r->newActor == 0 &&
                                                    r->newTarget > 0 ) {
                                            
                                                    // only applies if the 
                                                    // bare-ground
                                                    // trans leaves nothing in
                                                    // our hand
                                            
                                                    // first, change what they
                                                    // are holding to this 
                                                    // newTarget
                                                    handleHoldingChange( 
                                                        nextPlayer,
                                                        r->newTarget );
                                            
                                                    // this will handle 
                                                    // container
                                                    // size changes, etc.
                                                    // This is what should 
                                                    // end up
                                                    // on the ground
                                                    // as the result
                                                    // of the use-on-bare-ground
                                                    // transition.

                                                    // now swap it with the 
                                                    // non-permanent object
                                                    // on the ground.

                                                    swapHeldWithGround( 
                                                        nextPlayer,
                                                        target,
                                                        m.x,
                                                        m.y,
                                            &playerIndicesToSendUpdatesAbout );
                                                    }
                                                }
                                            }


                                        int targetSlots =
                                            targetObj->numSlots;
                                        
                                        float targetSlotSize = 0;
                                        
                                        if( targetSlots > 0 ) {
                                            targetSlotSize =
                                                targetObj->slotSize;
                                            }
                                        
                                        char canGoIn = false;
                                        
                                        if( canDrop &&
                                            droppedObj->containable &&
                                            targetSlotSize >=
                                            droppedObj->containSize ) {
                                            canGoIn = true;
                                            }
                                        
                                        

                                        // DROP indicates they 
                                        // right-clicked on container
                                        // so use swap mode
                                        if( canDrop && 
                                            canGoIn && 
                                            addHeldToContainer( 
                                                nextPlayer,
                                                target,
                                                m.x, m.y, true ) ) {
                                            // handled
                                            }
                                        else if( canDrop && 
                                                 ! canGoIn &&
                                                 targetObj->permanent &&
                                                 nextPlayer->numContained 
                                                 == 0 ) {
                                            // try treating it like
                                            // a USE action
                                            
                                            TransRecord *useTrans =
                                                getPTrans( 
                                                    nextPlayer->holdingID,
                                                    target );
                                            // handle simple case
                                            // stacking containers
                                            // client sends DROP for this
                                            if( useTrans != NULL &&
                                                useTrans->newActor == 0 ) {
                                                
                                                handleHoldingChange(
                                                    nextPlayer,
                                                    useTrans->newActor );
                                                
                                                setMapObject( 
                                                    m.x, m.y,
                                                    useTrans->newTarget );
                                                }
                                            }
                                        else if( canDrop && 
                                                 ! canGoIn &&
                                                 ! targetObj->permanent 
                                                 &&
                                                 targetObj->minPickupAge <=
                                                 computeAge( nextPlayer ) ) {
                                            // drop onto a spot where
                                            // something exists, and it's
                                            // not a container

                                            // swap what we're holding for
                                            // target

                                            // "set-down" type bare ground 
                                            // trans exists for what we're 
                                            // holding?
                                            TransRecord
                                                *r = getPTrans( 
                                                    nextPlayer->holdingID, 
                                                    -1 );

                                            if( r != NULL && 
                                                r->newActor == 0 &&
                                                r->newTarget > 0 ) {
                                            
                                                // only applies if the 
                                                // bare-ground
                                                // trans leaves nothing in
                                                // our hand
                                            
                                                // first, change what they
                                                // are holding to this 
                                                // newTarget
                                                handleHoldingChange( 
                                                    nextPlayer,
                                                    r->newTarget );
                                                }
                                            
                                            int oldHeld = 
                                                nextPlayer->holdingID;
                                            
                                            // now swap
                                            swapHeldWithGround( 
                                             nextPlayer, target, m.x, m.y,
                                             &playerIndicesToSendUpdatesAbout );
                                            
                                            if( oldHeld == 
                                                nextPlayer->holdingID ) {
                                                // no change
                                                // are they the same object?
                                                if( oldHeld == target ) {
                                                    // try using held
                                                    // on target
                                                    TransRecord *sameTrans
                                                        = getPTrans(
                                                            oldHeld, target );
                                                    if( sameTrans != NULL &&
                                                        sameTrans->newActor ==
                                                        0 ) {
                                                        // keep it simple
                                                        // for now
                                                        // this is usually
                                                        // just about
                                                        // stacking
                                                        handleHoldingChange(
                                                            nextPlayer,
                                                            sameTrans->
                                                            newActor );
                                                        
                                                        setMapObject(
                                                            m.x, m.y,
                                                            sameTrans->
                                                            newTarget );
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    else if( canDrop ) {
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
                        
                        char handEmpty = ( nextPlayer->holdingID == 0 );
                        
                        removeFromContainerToHold( nextPlayer,
                                                   m.x, m.y, m.i );

                        if( handEmpty &&
                            nextPlayer->holdingID == 0 ) {
                            // hand still empty?
                            
                            int target = getMapObject( m.x, m.y );

                            if( target > 0 ) {
                                ObjectRecord *targetObj = getObject( target );
                                
                                if( ! targetObj->permanent &&
                                    targetObj->minPickupAge <= 
                                    computeAge( nextPlayer ) ) {
                                    
                                    // treat it like pick up   
                                    pickupToHold( nextPlayer, m.x, m.y, 
                                                  target );
                                    }
                                else if( targetObj->permanent ) {
                                    // consider bare-hand action
                                    TransRecord *handTrans = getPTrans(
                                        0, target );
                                    
                                    // handle only simplest case here
                                    // (to avoid side-effects)
                                    // REMV on container stack
                                    // (make sure they have the same
                                    //  use parent)
                                    if( handTrans != NULL &&
                                        handTrans->newTarget > 0 &&
                                        getObject( handTrans->newTarget )->
                                        numSlots == targetObj->numSlots &&
                                        handTrans->newActor > 0 &&
                                        getObject( handTrans->newActor )->
                                        minPickupAge <= 
                                        computeAge( nextPlayer ) ) {
                                        
                                        handleHoldingChange( 
                                            nextPlayer,
                                            handTrans->newActor );
                                        setMapObject( m.x, m.y, 
                                                      handTrans->newTarget );
                                        }
                                    }
                                }
                            }
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
                                removeFromClothingContainerToHold(
                                    nextPlayer, m.c, m.i );
                                }
                            }
                        }
                    else if( m.type == EMOT && 
                             ! nextPlayer->emotFrozen ) {
                        // ignore new EMOT requres from player if emot
                        // frozen
                        
                        if( m.i <= SettingsManager::getIntSetting( 
                                "allowedEmotRange", 6 ) ) {
                            newEmotPlayerIDs.push_back( nextPlayer->id );
                            
                            newEmotIndices.push_back( m.i );
                            // player-requested emots have no specific TTL
                            newEmotTTLs.push_back( 0 );
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

                
                if( ! nextPlayer->isTutorial ) {
                    logDeath( nextPlayer->id,
                              nextPlayer->email,
                              nextPlayer->isEve,
                              computeAge( nextPlayer ),
                              getSecondsPlayed( 
                                  nextPlayer ),
                              ! getFemale( nextPlayer ),
                              nextPlayer->xd, nextPlayer->yd,
                              players.size() - 1,
                              false,
                              nextPlayer->murderPerpID,
                              nextPlayer->murderPerpEmail );
                                            
                    if( shutdownMode ) {
                        handleShutdownDeath( 
                            nextPlayer, nextPlayer->xd, nextPlayer->yd );
                        }
                    }
                
                nextPlayer->deathLogged = true;
                }
            

                
            if( nextPlayer->isNew ) {
                // their first position is an update
                

                playerIndicesToSendUpdatesAbout.push_back( i );
                playerIndicesToSendLineageAbout.push_back( i );
                
                
                if( nextPlayer->curseStatus.curseLevel > 0 ) {
                    playerIndicesToSendCursesAbout.push_back( i );
                    }

                nextPlayer->isNew = false;
                
                // force this PU to be sent to everyone
                nextPlayer->updateGlobal = true;
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
                

                newDeleteUpdates.push_back( 
                    getUpdateRecord( nextPlayer, true ) );                
                

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
                    if( nextPlayer->sock != NULL ) {
                        // stop listening for activity on this socket
                        sockPoll.removeSocket( nextPlayer->sock );
                        }
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
                
                // report to lineage server once here
                double age = computeAge( nextPlayer );
                
                int killerID = -1;
                if( nextPlayer->murderPerpID > 0 ) {
                    killerID = nextPlayer->murderPerpID;
                    }
                else if( nextPlayer->deathSourceID > 0 ) {
                    // include as negative of ID
                    killerID = - nextPlayer->deathSourceID;
                    }
                else if( nextPlayer->suicide ) {
                    // self id is killer
                    killerID = nextPlayer->id;
                    }
                
                
                
                char male = ! getFemale( nextPlayer );
                
                if( ! nextPlayer->isTutorial )
                recordPlayerLineage( nextPlayer->email, 
                                     age,
                                     nextPlayer->id,
                                     nextPlayer->parentID,
                                     nextPlayer->displayID,
                                     killerID,
                                     nextPlayer->name,
                                     nextPlayer->lastSay,
                                     male );
                
                // don't use age here, because it unfairly gives Eve
                // +14 years that she didn't actually live
                // use true played years instead
                double yearsLived = 
                    getSecondsPlayed( nextPlayer ) * getAgeRate();

                if( ! nextPlayer->isTutorial )
                recordLineage( nextPlayer->email, 
                               nextPlayer->lineageEveID,
                               yearsLived, 
                               // count true murder victims here, not suicide
                               ( killerID > 0 && killerID != nextPlayer->id ),
                               // killed other or committed SID suicide
                               nextPlayer->everKilledAnyone || 
                               nextPlayer->suicide );
        
                if( ! nextPlayer->deathLogged ) {
                    char disconnect = true;
                    
                    if( age >= forceDeathAge ) {
                        disconnect = false;
                        }
                    
                    if( ! nextPlayer->isTutorial ) {    
                        logDeath( nextPlayer->id,
                                  nextPlayer->email,
                                  nextPlayer->isEve,
                                  age,
                                  getSecondsPlayed( nextPlayer ),
                                  male,
                                  dropPos.x, dropPos.y,
                                  players.size() - 1,
                                  disconnect );
                    
                        if( shutdownMode ) {
                            handleShutdownDeath( 
                                nextPlayer, dropPos.x, dropPos.y );
                            }
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

                int deathID = getRandomDeathMarker();
                    
                if( nextPlayer->customGraveID > -1 ) {
                    deathID = nextPlayer->customGraveID;
                    }

                int oldObject = getMapObject( dropPos.x, dropPos.y );
                
                SimpleVector<int> oldContained;
                SimpleVector<timeSec_t> oldContainedETADecay;
                
                if( deathID != 0 ) {
                    
                
                    int nX[4] = { -1, 1,  0, 0 };
                    int nY[4] = {  0, 0, -1, 1 };
                    
                    int n = 0;
                    GridPos centerDropPos = dropPos;
                    
                    while( oldObject != 0 && n <= 4 ) {
                        
                        // don't combine graves
                        if( ! isGrave( oldObject ) ) {
                            ObjectRecord *r = getObject( oldObject );
                            
                            if( r->numSlots == 0 && ! r->permanent 
                                && ! r->rideable ) {
                                
                                // found a containble object
                                // we can empty this spot to make room
                                // for a grave that can go here, and
                                // put the old object into the new grave.
                                
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
                    }
                

                if( ! isMapSpotEmpty( dropPos.x, dropPos.y, false ) ) {
                    
                    // failed to find an empty spot, or a containable object
                    // at center or four neighbors
                    
                    // search outward in spiral of up to 100 points
                    // look for some empty spot
                    
                    char foundEmpty = false;
                    
                    GridPos newDropPos = findClosestEmptyMapSpot(
                        dropPos.x, dropPos.y, 100, &foundEmpty );
                    
                    if( foundEmpty ) {
                        dropPos = newDropPos;
                        }
                    }


                // assume death markes non-blocking, so it's safe
                // to drop one even if other players standing here
                if( isMapSpotEmpty( dropPos.x, dropPos.y, false ) ) {

                    if( deathID > 0 ) {
                        
                        setResponsiblePlayer( - nextPlayer->id );
                        setMapObject( dropPos.x, dropPos.y, 
                                      deathID );
                        setResponsiblePlayer( -1 );
                        
                        GraveInfo graveInfo = { dropPos, nextPlayer->id };
                        newGraves.push_back( graveInfo );
                        

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
                            getPTrans( nextPlayer->murderSourceID, 
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
                    if( nextPlayer->holdingWound ) {
                        // holding a wound from some other, non-murder cause
                        // of death
                        doNotDrop = true;
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
                
                    TransRecord *t = getPTrans( -1, oldID );

                    if( t != NULL ) {

                        int newID = t->newTarget;
                        
                        handleHoldingChange( nextPlayer, newID );
                        
                        if( newID == 0 &&
                            nextPlayer->holdingWound &&
                            nextPlayer->dying ) {
                            
                            // wound decayed naturally, count as healed
                            setNoLongerDying( 
                                nextPlayer,
                                &playerIndicesToSendHealingAbout );            
                            }
                        

                        nextPlayer->heldTransitionSourceID = -1;
                        
                        ObjectRecord *newObj = getObject( newID );
                        ObjectRecord *oldObj = getObject( oldID );
                        
                        
                        if( newObj != NULL && newObj->permanent &&
                            oldObj != NULL && ! oldObj->permanent ) {
                            // object decayed into a permanent
                            // force drop
                             GridPos dropPos = 
                                getPlayerPos( nextPlayer );
                            
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
                            
                            TransRecord *t = getPTrans( -1, oldID );

                            newDecay = 0;

                            if( t != NULL ) {
                                
                                newID = t->newTarget;
                            
                                if( newID != 0 ) {
                                    float stretch = 
                                        getObject( nextPlayer->holdingID )->
                                        slotTimeStretch;
                                    
                                    TransRecord *newDecayT = 
                                        getMetaTrans( -1, newID );
                                
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
                            int oldSlots = subCont.size();
                            
                            int newSlots = getObject( newID )->numSlots;
                            
                            if( newID != oldID
                                &&
                                newSlots < oldSlots ) {
                                
                                // shrink sub-contained
                                // this involves items getting lost
                                // but that's okay for now.
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
                            
                                TransRecord *t = getPTrans( -1, oldSubID );

                                newSubDecay = 0;

                                if( t != NULL ) {
                                
                                    newSubID = t->newTarget;
                            
                                    if( newSubID != 0 ) {
                                        float subStretch = 
                                            getObject( newID )->
                                            slotTimeStretch;
                                    
                                        TransRecord *newSubDecayT = 
                                            getMetaTrans( -1, newSubID );
                                
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
                        
                        freePlayerContainedArrays( nextPlayer );
                        
                        nextPlayer->numContained = newContained.size();

                        if( nextPlayer->numContained == 0 ) {
                            nextPlayer->containedIDs = NULL;
                            nextPlayer->containedEtaDecays = NULL;
                            nextPlayer->subContainedIDs = NULL;
                            nextPlayer->subContainedEtaDecays = NULL;
                            }
                        else {
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
                
                        TransRecord *t = getPTrans( -1, oldID );

                        if( t != NULL ) {

                            int newID = t->newTarget;
                            
                            ObjectRecord *newCObj = NULL;
                            if( newID != 0 ) {
                                newCObj = getObject( newID );
                                
                                TransRecord *newDecayT = 
                                    getMetaTrans( -1, newID );
                                
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
                                nextPlayer->clothingContained[c].size();

                            int newSlots = getNumContainerSlots( newID );
                    
                            if( newSlots < oldSlots ) {
                                // new container can hold less
                                // truncate
                                
                                // drop extras onto map
                                timeSec_t curTime = Time::timeSec();
                                float stretch = cObj->slotTimeStretch;
                                
                                GridPos dropPos = 
                                    getPlayerPos( nextPlayer );
                            
                                // offset to counter-act offsets built into
                                // drop code
                                dropPos.x += 1;
                                dropPos.y += 1;

                                for( int s=newSlots; s<oldSlots; s++ ) {
                                    
                                    char found = false;
                                    GridPos spot;
                                
                                    if( getMapObject( dropPos.x, 
                                                      dropPos.y ) == 0 ) {
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
                                        setMapObject( 
                                            spot.x, spot.y,
                                            nextPlayer->
                                            clothingContained[c].
                                            getElementDirect( s ) );
                                        
                                        timeSec_t eta =
                                            nextPlayer->
                                            clothingContainedEtaDecays[c].
                                            getElementDirect( s );
                                        
                                        if( stretch != 1.0 ) {
                                            timeSec_t offset = 
                                                eta - curTime;
                    
                                            offset = offset / stretch;
                                            eta = curTime + offset;
                                            }
                                        
                                        setEtaDecay( spot.x, spot.y, eta );
                                        }
                                    }

                                nextPlayer->
                                    clothingContained[c].
                                    shrink( newSlots );
                                
                                nextPlayer->
                                    clothingContainedEtaDecays[c].
                                    shrink( newSlots );
                                }
                            
                            float oldStretch = 
                                cObj->slotTimeStretch;
                            float newStretch;
                            
                            if( newCObj != NULL ) {
                                newStretch = newCObj->slotTimeStretch;
                                }
                            else {
                                newStretch = oldStretch;
                                }
                            
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
                            
                                TransRecord *t = getPTrans( -1, oldID );
                                
                                newDecay = 0;

                                if( t != NULL ) {
                                    newID = t->newTarget;
                            
                                    if( newID != 0 ) {
                                        TransRecord *newDecayT = 
                                            getMetaTrans( -1, newID );
                                        
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
                
                    
                    // don't end new moves here (moves that 
                    // other players haven't been told about)
                    // even if they have come to an end time-wise
                    // wait until after we've told everyone about them
                    if( ! nextPlayer->newMove && 
                        Time::getCurrentTime() - nextPlayer->moveStartTime
                        >
                        nextPlayer->moveTotalSeconds ) {
                        
                        double moveSpeed = computeMoveSpeed( nextPlayer ) *
                            getPathSpeedModifier( nextPlayer->pathToDest,
                                                  nextPlayer->pathLength );


                        // done
                        nextPlayer->xs = nextPlayer->xd;
                        nextPlayer->ys = nextPlayer->yd;                        

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

                        
                        // if they went far enough and fast enough
                        if( nextPlayer->holdingFlightObject &&
                            moveSpeed >= minFlightSpeed &&
                            ! nextPlayer->pathTruncated &&
                            nextPlayer->pathLength >= 2 ) {
                                    
                            // player takes off ?
                            
                            double xDir = 
                                nextPlayer->pathToDest[ 
                                      nextPlayer->pathLength - 1 ].x
                                  -
                                  nextPlayer->pathToDest[ 
                                      nextPlayer->pathLength - 2 ].x;
                            double yDir = 
                                nextPlayer->pathToDest[ 
                                      nextPlayer->pathLength - 1 ].y
                                  -
                                  nextPlayer->pathToDest[ 
                                      nextPlayer->pathLength - 2 ].y;
                            
                            int beyondEndX = nextPlayer->xs + xDir;
                            int beyondEndY = nextPlayer->ys + yDir;
                            
                            int endFloorID = getMapFloor( nextPlayer->xs,
                                                          nextPlayer->ys );
                            
                            int beyondEndFloorID = getMapFloor( beyondEndX,
                                                                beyondEndY );
                            
                            if( beyondEndFloorID != endFloorID ) {
                                // went all the way to the end of the 
                                // current floor in this direction, 
                                // take off there
                            
                                doublePair takeOffDir = { xDir, yDir };

                                GridPos destPos = 
                                    getNextFlightLandingPos(
                                        nextPlayer->xs,
                                        nextPlayer->ys,
                                        takeOffDir );
                            
                                AppLog::infoF( 
                                    "Player %d flight taking off from (%d,%d), "
                                    "flightDir (%f,%f), dest (%d,%d)",
                                    nextPlayer->id,
                                    nextPlayer->xs, nextPlayer->ys,
                                    xDir, yDir,
                                    destPos.x, destPos.y );
                                
                                
                            
                                // send them a brand new map chunk
                                // around their new location
                                // and re-tell them about all players
                                // (relative to their new "birth" location...
                                //  see below)
                                nextPlayer->firstMessageSent = false;
                                nextPlayer->firstMapSent = false;
                                nextPlayer->inFlight = true;
                                
                                int destID = getMapObject( destPos.x,
                                                           destPos.y );
                                    
                                char heldTransHappened = false;
                                    
                                if( destID > 0 &&
                                    getObject( destID )->isFlightLanding ) {
                                    // found a landing place
                                    TransRecord *tr =
                                        getPTrans( nextPlayer->holdingID,
                                                   destID );
                                        
                                    if( tr != NULL ) {
                                        heldTransHappened = true;
                                            
                                        setMapObject( destPos.x, destPos.y,
                                                      tr->newTarget );

                                        transferHeldContainedToMap( 
                                            nextPlayer,
                                            destPos.x, destPos.y );

                                        handleHoldingChange(
                                            nextPlayer,
                                            tr->newActor );
                                            
                                        // stick player next to landing
                                        // pad
                                        destPos.x --;
                                        }
                                    }
                                if( ! heldTransHappened ) {
                                    // crash landing
                                    // force decay of held
                                    // no matter how much time is left
                                    // (flight uses fuel)
                                    TransRecord *decayTrans =
                                        getPTrans( -1, 
                                                   nextPlayer->holdingID );
                                        
                                    if( decayTrans != NULL ) {
                                        handleHoldingChange( 
                                            nextPlayer,
                                            decayTrans->newTarget );
                                        }
                                    }
                                    
                                FlightDest fd = {
                                    nextPlayer->id,
                                    destPos };

                                newFlightDest.push_back( fd );
                                
                                nextPlayer->xd = destPos.x;
                                nextPlayer->xs = destPos.x;
                                nextPlayer->yd = destPos.y;
                                nextPlayer->ys = destPos.y;

                                // reset their birth location
                                // their landing position becomes their
                                // new 0,0 for now
                                
                                // birth-relative coordinates enable the client
                                // (which is on a GPU with 32-bit floats)
                                // to operate at true coordinates well above
                                // the 23-bit preciions of 32-bit floats.
                                
                                // We keep the coordinates small by assuming
                                // that a player can never get too far from
                                // their birth location in one lifetime.
                                
                                // Flight teleportation violates this 
                                // assumption.
                                nextPlayer->birthPos.x = nextPlayer->xs;
                                nextPlayer->birthPos.y = nextPlayer->ys;
                                nextPlayer->heldOriginX = nextPlayer->xs;
                                nextPlayer->heldOriginY = nextPlayer->ys;
                                
                                nextPlayer->actionTarget.x = nextPlayer->xs;
                                nextPlayer->actionTarget.y = nextPlayer->ys;
                                }
                            }
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

                        if( nextPlayer->yummyBonusStore > 0 ) {
                            nextPlayer->yummyBonusStore--;
                            }
                        else {
                            nextPlayer->foodStore--;
                            }
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
                        
                        if( ! decrementedPlayer->deathLogged &&
                            ! decrementedPlayer->isTutorial ) {    
                            logDeath( decrementedPlayer->id,
                                      decrementedPlayer->email,
                                      decrementedPlayer->isEve,
                                      computeAge( decrementedPlayer ),
                                      getSecondsPlayed( decrementedPlayer ),
                                      ! getFemale( decrementedPlayer ),
                                      deathPos.x, deathPos.y,
                                      players.size() - 1,
                                      false );
                            }
                        
                        if( shutdownMode &&
                            ! decrementedPlayer->isTutorial ) {
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
        

        if( playerIndicesToSendUpdatesAbout.size() > 0 ) {
            
            SimpleVector<char> updateList;
        
            for( int i=0; i<playerIndicesToSendUpdatesAbout.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( 
                    playerIndicesToSendUpdatesAbout.getElementDirect( i ) );
                
                char *playerString = autoSprintf( "%d, ", nextPlayer->id );
                updateList.appendElementString( playerString );
                
                delete [] playerString;
                }
            
            char *updateListString = updateList.getElementString();
            
            AppLog::infoF( "Need to send updates about these %d players: %s",
                           playerIndicesToSendUpdatesAbout.size(),
                           updateListString );
            delete [] updateListString;
            }
        


        double currentTimeHeat = Time::getCurrentTime();
        
        if( currentTimeHeat - lastHeatUpdateTime >= heatUpdateTimeStep ) {
            // a heat step has passed
            
            
            // recompute heat map here for next players in line
            int r = 0;
            for( r=lastPlayerIndexHeatRecomputed+1; 
                 r < lastPlayerIndexHeatRecomputed + 1 + 
                     numPlayersRecomputeHeatPerStep
                     &&
                     r < players.size(); r++ ) {
                
                recomputeHeatMap( players.getElement( r ) );
                }
            
            lastPlayerIndexHeatRecomputed = r - 1;
            
            if( r == players.size() ) {
                // done updating for last player
                // start over
                lastPlayerIndexHeatRecomputed = -1;
                }
            lastHeatUpdateTime = currentTimeHeat;
            }
        



        // update personal heat value of any player that is due
        // once every 2 seconds
        currentTime = Time::getCurrentTime();
        for( int i=0; i< players.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            
            if( nextPlayer->error ||
                currentTime - nextPlayer->lastHeatUpdate < heatUpdateSeconds ) {
                continue;
                }
            
            // in case we cross a biome boundary since last time
            // there will be thermal shock that will take them to
            // other side of target temp.
            // 
            // but never make them more comfortable (closer to
            // target) then they were before
            float oldDiffFromTarget = 
                targetHeat - nextPlayer->bodyHeat;


            
            // body produces its own heat
            // but only in a cold env
            if( nextPlayer->envHeat < targetHeat ) {
                nextPlayer->bodyHeat += 0.25;
                }

            nextPlayer->bodyHeat += computeClothingHeat( nextPlayer );

            float clothingR = computeClothingR( nextPlayer );

            // clothingR modulates heat lost (or gained) from environment
            float clothingLeak = 1 - clothingR;

            float heatDelta = 
                clothingLeak * ( nextPlayer->envHeat - nextPlayer->bodyHeat );

            // slow this down a bit
            heatDelta *= 0.5;
            
            // feed through curve that is asymtotic at 1
            // (so we never change heat faster than 1 unit per timestep)
            
            float heatDeltaAbs = fabs( heatDelta );
            float heatDeltaSign = sign( heatDelta );

            float maxDelta = 2;
            // larger values make a sharper "knee"
            float deltaSlope = 0.5;
            
            // max - max/(slope*x+1)
            
            float heatDeltaScaled = 
                maxDelta - maxDelta / ( deltaSlope * heatDeltaAbs + 1 );
            
            heatDeltaScaled *= heatDeltaSign;


            nextPlayer->bodyHeat += heatDeltaScaled;
            
  
            if( nextPlayer->lastBiomeHeat != nextPlayer->biomeHeat ) {
                
          
                float lastBiomeDiffFromTarget = 
                    targetHeat - nextPlayer->lastBiomeHeat;
            
                float biomeDiffFromTarget = targetHeat - nextPlayer->biomeHeat;
            
                // for any biome
                // there's a "shock" when you enter it, if it's heat value
                // is on the other side of "perfect" from the last biome
                // you were in.
                if( lastBiomeDiffFromTarget != 0 &&
                    biomeDiffFromTarget != 0 &&
                    sign( lastBiomeDiffFromTarget ) != 
                    sign( biomeDiffFromTarget ) ) {
                
                    // modulate this shock by clothing

                    // but only if player does not have fever
                    // we don't want to punish them for wearing clothes
                    // if they get a fever
                    if( nextPlayer->fever == 0 ) {
                        nextPlayer->bodyHeat = 
                            targetHeat - clothingLeak * biomeDiffFromTarget;
                        
                        float newDiffFromTarget =
                            targetHeat - nextPlayer->bodyHeat;
                        
                        float oldAbs = fabs( oldDiffFromTarget );
                        
                        if( fabs( newDiffFromTarget ) < 
                            oldAbs ) {
                            // they used crossing boundary to become more
                            // comfortable
                            
                            // force them to be no more comfortable than
                            // they used to be
                            nextPlayer->bodyHeat = 
                                targetHeat - 
                                sign( newDiffFromTarget ) * oldAbs;
                            }

                        }
                    else {
                        // direct shock, as if unclothed
                        float airLeak = 1 - rAir;
                        nextPlayer->bodyHeat = 
                            targetHeat - airLeak * biomeDiffFromTarget;
                        }
                    }

                // we've handled this shock
                nextPlayer->lastBiomeHeat = nextPlayer->biomeHeat;
                }
            
            float totalBodyHeat = nextPlayer->bodyHeat + nextPlayer->fever;
            
            // convert into 0..1 range, where 0.5 represents targetHeat
            nextPlayer->heat = ( totalBodyHeat / targetHeat ) / 2;
            if( nextPlayer->heat > 1 ) {
                nextPlayer->heat = 1;
                }
            if( nextPlayer->heat < 0 ) {
                nextPlayer->heat = 0;
                }

            nextPlayer->heatUpdate = true;
            nextPlayer->lastHeatUpdate = currentTime;
            }
        

        
        for( int i=0; i<playerIndicesToSendUpdatesAbout.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement( 
                playerIndicesToSendUpdatesAbout.getElementDirect( i ) );

            if( nextPlayer->updateSent ) {
                continue;
                }
            
            // also force-recompute heat maps for players that are getting
            // updated
            // don't bother with this for now
            // all players update on the same cycle
            // recomputeHeatMap( nextPlayer );
            
            
            
            newUpdates.push_back( getUpdateRecord( nextPlayer, false ) );
            
            newUpdatePlayerIDs.push_back( nextPlayer->id );
            

            if( nextPlayer->posForced &&
                nextPlayer->connected &&
                SettingsManager::getIntSetting( "requireClientForceAck", 1 ) ) {
                // block additional moves/actions from this player until
                // we get a FORCE response, syncing them up with
                // their forced position.
                
                // don't do this for disconnected players
                nextPlayer->waitingForForceResponse = true;
                }
            nextPlayer->posForced = false;


            ChangePosition p = { nextPlayer->xs, nextPlayer->ys, 
                                 nextPlayer->updateGlobal };
            newUpdatesPos.push_back( p );


            nextPlayer->updateSent = true;
            nextPlayer->updateGlobal = false;
            }
        
        

        if( newUpdates.size() > 0 ) {
            
            SimpleVector<char> trueUpdateList;
            
            
            for( int i=0; i<newUpdates.size(); i++ ) {
                char *s = autoSprintf( 
                    "%d, ", newUpdatePlayerIDs.getElementDirect( i ) );
                trueUpdateList.appendElementString( s );
                delete [] s;
                }
            
            char *updateListString = trueUpdateList.getElementString();
            
            AppLog::infoF( "Sending updates about these %d players: %s",
                           newUpdatePlayerIDs.size(),
                           updateListString );
            delete [] updateListString;
            }
        
        

        
        SimpleVector<ChangePosition> movesPos;        

        SimpleVector<MoveRecord> moveList = getMoveRecords( true, &movesPos );
        
        
                







        

        

        // add changes from auto-decays on map, 
        // mixed with player-caused changes
        stepMap( &mapChanges, &mapChangesPos );
        
        

        
        if( periodicStepThisStep ) {

            // figure out who has recieved a new curse token
            // they are sent a message about it below (CX)
            SimpleVector<char*> newCurseTokenEmails;
            getNewCurseTokenHolders( &newCurseTokenEmails );
        
            for( int i=0; i<newCurseTokenEmails.size(); i++ ) {
                char *email = newCurseTokenEmails.getElementDirect( i );
                
                for( int j=0; j<numLive; j++ ) {
                    LiveObject *nextPlayer = players.getElement(j);
                    
                    if( strcmp( nextPlayer->email, email ) == 0 ) {
                        
                        nextPlayer->curseTokenCount = 1;
                        nextPlayer->curseTokenUpdate = true;
                        break;
                        }
                    }
                
                delete [] email;
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




        unsigned char *cursesMessage = NULL;
        int cursesMessageLength = 0;
        
        if( playerIndicesToSendCursesAbout.size() > 0 ) {
            SimpleVector<char> curseWorking;
            curseWorking.appendElementString( "CU\n" );
            
            int numAdded = 0;
            for( int i=0; i<playerIndicesToSendCursesAbout.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( 
                    playerIndicesToSendCursesAbout.getElementDirect( i ) );

                if( nextPlayer->error ) {
                    continue;
                    }

                char *line = autoSprintf( "%d %d\n", nextPlayer->id,
                                         nextPlayer->curseStatus.curseLevel );
                
                curseWorking.appendElementString( line );
                delete [] line;
                numAdded++;
                }
            
            curseWorking.push_back( '#' );
            
            if( numAdded > 0 ) {

                char *cursesMessageText = curseWorking.getElementString();
                
                cursesMessageLength = strlen( cursesMessageText );
                
                if( cursesMessageLength < maxUncompressedSize ) {
                    cursesMessage = (unsigned char*)cursesMessageText;
                    }
                else {
                    // compress for all players once here
                    cursesMessage = makeCompressedMessage( 
                        cursesMessageText, 
                        cursesMessageLength, &cursesMessageLength );
                    
                    delete [] cursesMessageText;
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
                
                char *line;
                
                if( nextPlayer->holdingEtaDecay > 0 ) {
                    // what they have will cure itself in time
                    // flag as sick
                    line = autoSprintf( "%d 1\n", nextPlayer->id );
                    }
                else {
                    line = autoSprintf( "%d\n", nextPlayer->id );
                    }
                
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




        unsigned char *healingMessage = NULL;
        int healingMessageLength = 0;
        
        if( playerIndicesToSendHealingAbout.size() > 0 ) {
            SimpleVector<char> healingWorking;
            healingWorking.appendElementString( "HE\n" );
            
            int numAdded = 0;
            for( int i=0; i<playerIndicesToSendHealingAbout.size(); i++ ) {
                LiveObject *nextPlayer = players.getElement( 
                    playerIndicesToSendHealingAbout.getElementDirect( i ) );

                if( nextPlayer->error ) {
                    continue;
                    }

                char *line = autoSprintf( "%d\n", nextPlayer->id );

                numAdded++;
                healingWorking.appendElementString( line );
                delete [] line;
                }
            
            healingWorking.push_back( '#' );
            
            if( numAdded > 0 ) {

                char *healingMessageText = healingWorking.getElementString();
                
                healingMessageLength = strlen( healingMessageText );
                
                if( healingMessageLength < maxUncompressedSize ) {
                    healingMessage = (unsigned char*)healingMessageText;
                    }
                else {
                    // compress for all players once here
                    healingMessage = makeCompressedMessage( 
                        healingMessageText, 
                        healingMessageLength, &healingMessageLength );
                    
                    delete [] healingMessageText;
                    }
                }
            }




        unsigned char *emotMessage = NULL;
        int emotMessageLength = 0;
        
        if( newEmotPlayerIDs.size() > 0 ) {
            SimpleVector<char> emotWorking;
            emotWorking.appendElementString( "PE\n" );
            
            int numAdded = 0;
            for( int i=0; i<newEmotPlayerIDs.size(); i++ ) {
                
                int ttl = newEmotTTLs.getElementDirect( i );

                char *line;
                
                if( ttl == 0  ) {
                    line = autoSprintf( 
                        "%d %d\n", 
                        newEmotPlayerIDs.getElementDirect( i ), 
                        newEmotIndices.getElementDirect( i ) );
                    }
                else {
                    line = autoSprintf( 
                        "%d %d %d\n", 
                        newEmotPlayerIDs.getElementDirect( i ), 
                        newEmotIndices.getElementDirect( i ),
                        newEmotTTLs.getElementDirect( i ) );
                    }
                
                numAdded++;
                emotWorking.appendElementString( line );
                delete [] line;
                }
            
            emotWorking.push_back( '#' );
            
            if( numAdded > 0 ) {

                char *emotMessageText = emotWorking.getElementString();
                
                emotMessageLength = strlen( emotMessageText );
                
                if( emotMessageLength < maxUncompressedSize ) {
                    emotMessage = (unsigned char*)emotMessageText;
                    }
                else {
                    // compress for all players once here
                    emotMessage = makeCompressedMessage( 
                        emotMessageText, 
                        emotMessageLength, &emotMessageLength );
                    
                    delete [] emotMessageText;
                    }
                }
            }

        
        
        // send moves and updates to clients
        
        
        SimpleVector<int> playersReceivingPlayerUpdate;
        

        for( int i=0; i<numLive; i++ ) {
            
            LiveObject *nextPlayer = players.getElement(i);
            
            
            // everyone gets all flight messages
            // even if they haven't gotten first message yet
            // (because the flier will get their first message again
            // when they land, and we need to tell them about flight first)
            if( nextPlayer->firstMapSent ||
                nextPlayer->inFlight ) {
                                
                if( newFlightDest.size() > 0 ) {
                    
                    // compose FD messages for this player
                    
                    for( int u=0; u<newFlightDest.size(); u++ ) {
                        FlightDest *f = newFlightDest.getElement( u );
                        
                        char *flightMessage = 
                            autoSprintf( "FD\n%d %d %d\n#",
                                         f->playerID,
                                         f->destPos.x -
                                         nextPlayer->birthPos.x, 
                                         f->destPos.y -
                                         nextPlayer->birthPos.y );
                        
                        sendMessageToPlayer( nextPlayer, flightMessage,
                                             strlen( flightMessage ) );
                        delete [] flightMessage;
                        }
                    }
                }

            

            
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

                    char oWasForced = o->posForced;
                    
                    if( nextPlayer->inFlight ) {
                        // not a true first message
                        
                        // force all positions for all players
                        o->posForced = true;
                        }
                    

                    // true mid-move positions for first message
                    // all relative to new player's birth pos
                    char *messageLine = getUpdateLine( o, 
                                                       nextPlayer->birthPos,
                                                       false, true );
                    
                    if( nextPlayer->inFlight ) {
                        // restore
                        o->posForced = oWasForced;
                        }
                    

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
                

                char *movesMessage = getMovesMessage( false, 
                                                      nextPlayer->birthPos );
                
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



                // send cursed status for all living cursed
                
                SimpleVector<char> cursesWorking;
                cursesWorking.appendElementString( "CU\n" );

                numAdded = 0;
                
                for( int i=0; i<numPlayers; i++ ) {
                
                    LiveObject *o = players.getElement( i );
                
                    if( o->error ) {
                        continue;
                        }

                    int level = o->curseStatus.curseLevel;
                    
                    if( level == 0 ) {
                        continue;
                        }
                    

                    char *line = autoSprintf( "%d %d\n", o->id, level );
                    cursesWorking.appendElementString( line );
                    delete [] line;
                    
                    numAdded++;
                    }
                
                cursesWorking.push_back( '#' );
            
                if( numAdded > 0 ) {
                    char *cursesMessage = cursesWorking.getElementString();


                    sendMessageToPlayer( nextPlayer, cursesMessage, 
                                         strlen( cursesMessage ) );
                
                    delete [] cursesMessage;
                    }
                

                if( nextPlayer->curseStatus.curseLevel > 0 ) {
                    // send player their personal report about how
                    // many excess curse points they have
                    
                    char *message = autoSprintf( 
                        "CS\n%d#", 
                        nextPlayer->curseStatus.excessPoints );

                    sendMessageToPlayer( nextPlayer, message, 
                                         strlen( message ) );
                
                    delete [] message;
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
                nextPlayer->inFlight = false;
                }
            else {
                // this player has first message, ready for updates/moves
                

                if( nextPlayer->monumentPosSet && 
                    ! nextPlayer->monumentPosSent &&
                    computeAge( nextPlayer ) > 0.5 ) {
                    
                    // they learned about a monument from their mother
                    
                    // wait until they are half a year old to tell them
                    // so they have a chance to load the sound first
                    
                    char *monMessage = 
                        autoSprintf( "MN\n%d %d %d\n#", 
                                     nextPlayer->lastMonumentPos.x -
                                     nextPlayer->birthPos.x, 
                                     nextPlayer->lastMonumentPos.y -
                                     nextPlayer->birthPos.y,
                                     nextPlayer->lastMonumentID );
                    
                    sendMessageToPlayer( nextPlayer, monMessage, 
                                         strlen( monMessage ) );
                    
                    nextPlayer->monumentPosSent = true;
                    
                    delete [] monMessage;
                    }




                // everyone gets all grave messages
                if( newGraves.size() > 0 ) {
                    
                    // compose GV messages for this player
                    
                    for( int u=0; u<newGraves.size(); u++ ) {
                        GraveInfo *g = newGraves.getElement( u );
                        
                        char *graveMessage = 
                            autoSprintf( "GV\n%d %d %d\n#", 
                                         g->pos.x -
                                         nextPlayer->birthPos.x, 
                                         g->pos.y -
                                         nextPlayer->birthPos.y,
                                         g->playerID );
                        
                        sendMessageToPlayer( nextPlayer, graveMessage,
                                             strlen( graveMessage ) );
                        delete [] graveMessage;
                        }
                    }


                // everyone gets all grave move messages
                if( newGraveMoves.size() > 0 ) {
                    
                    // compose GM messages for this player
                    
                    for( int u=0; u<newGraveMoves.size(); u++ ) {
                        GraveMoveInfo *g = newGraveMoves.getElement( u );
                        
                        char *graveMessage = 
                            autoSprintf( "GM\n%d %d %d %d\n#", 
                                         g->posStart.x -
                                         nextPlayer->birthPos.x,
                                         g->posStart.y -
                                         nextPlayer->birthPos.y,
                                         g->posEnd.x -
                                         nextPlayer->birthPos.x,
                                         g->posEnd.y -
                                         nextPlayer->birthPos.y );
                        
                        sendMessageToPlayer( nextPlayer, graveMessage,
                                             strlen( graveMessage ) );
                        delete [] graveMessage;
                        }
                    }
                
                

                int playerXD = nextPlayer->xd;
                int playerYD = nextPlayer->yd;
                
                if( nextPlayer->heldByOther ) {
                    LiveObject *holdingPlayer = 
                        getLiveObject( nextPlayer->heldByOtherID );
                
                    if( holdingPlayer != NULL ) {
                        playerXD = holdingPlayer->xd;
                        playerYD = holdingPlayer->yd;
                        }
                    }


                if( abs( playerXD - nextPlayer->lastSentMapX ) > 7
                    ||
                    abs( playerYD - nextPlayer->lastSentMapY ) > 8 
                    ||
                    ! nextPlayer->firstMapSent ) {
                
                    // moving out of bounds of chunk, send update
                    // or player flagged as needing first map again
                    
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
                                                           nextPlayer->birthPos,
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
                        LiveObject *adult = getAdultHolding( nextPlayer );
                        
                        if( adult != NULL ) {
                            ourHolderID = adult->id;
                            }
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
                                    getUpdateLine( otherPlayer, 
                                                   nextPlayer->birthPos,
                                                   false ); 
                                    
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
                if( dyingMessage != NULL && nextPlayer->connected ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            dyingMessage, 
                            dyingMessageLength, 
                            false, false );
                    
                    nextPlayer->gotPartOfThisFrame = true;

                    if( numSent != dyingMessageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    }


                // EVERYONE gets info about now-healed players           
                if( healingMessage != NULL && nextPlayer->connected ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            healingMessage, 
                            healingMessageLength, 
                            false, false );
                    
                    nextPlayer->gotPartOfThisFrame = true;
                    
                    if( numSent != healingMessageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    }


                // EVERYONE gets info about emots           
                if( emotMessage != NULL && nextPlayer->connected ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            emotMessage, 
                            emotMessageLength, 
                            false, false );
                    
                    nextPlayer->gotPartOfThisFrame = true;
                    
                    if( numSent != emotMessageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    }

                

                double maxDist = 32;
                double maxDist2 = maxDist * 2;

                if( newUpdates.size() > 0 && nextPlayer->connected ) {

                    double minUpdateDist = maxDist2 * 2;
                    
                    // greater than maxDis but within maxDist2
                    SimpleVector<int> middleDistancePlayerIDs;
                    

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
                            if( d > maxDist && d <= maxDist2 ) {
                                middleDistancePlayerIDs.push_back(
                                    newUpdatePlayerIDs.getElementDirect( u ) );
                                }
                            }
                        }

                    if( minUpdateDist <= maxDist ) {
                        // some updates close enough

                        // compose PU mesage for this player
                        
                        unsigned char *updateMessage = NULL;
                        int updateMessageLength = 0;
                        SimpleVector<char> updateChars;
                        
                        for( int u=0; u<newUpdates.size(); u++ ) {
                            ChangePosition *p = newUpdatesPos.getElement( u );
                        
                            double d = intDist( p->x, p->y, 
                                                playerXD, playerYD );
                            
                            if( ! p->global && d > maxDist ) {
                                // skip this one, too far away
                                continue;
                                }
                            
                            char *line =
                                getUpdateLineFromRecord( 
                                    newUpdates.getElement( u ),
                                    nextPlayer->birthPos );
                            
                            updateChars.appendElementString( line );
                            delete [] line;
                            }
                        

                        if( updateChars.size() > 0 ) {
                            updateChars.push_back( '#' );
                            char *temp = updateChars.getElementString();

                            char *updateMessageText = 
                                concatonate( "PU\n", temp );
                            delete [] temp;
                            
                            updateMessageLength = strlen( updateMessageText );

                            if( updateMessageLength < maxUncompressedSize ) {
                                updateMessage = 
                                    (unsigned char*)updateMessageText;
                                }
                            else {
                                updateMessage = makeCompressedMessage( 
                                    updateMessageText, 
                                    updateMessageLength, &updateMessageLength );
                
                                delete [] updateMessageText;
                                }
                            }

                        if( updateMessage != NULL ) {
                            playersReceivingPlayerUpdate.push_back( 
                                nextPlayer->id );
                            
                            int numSent = 
                                nextPlayer->sock->send( 
                                    updateMessage, 
                                    updateMessageLength, 
                                    false, false );
                            
                            nextPlayer->gotPartOfThisFrame = true;
                            
                            delete [] updateMessage;
                            
                            if( numSent != updateMessageLength ) {
                                setPlayerDisconnected( nextPlayer, 
                                                       "Socket write failed" );
                                }
                            }
                        }
                    

                    if( middleDistancePlayerIDs.size() > 0 ) {

                        unsigned char *outOfRangeMessage = NULL;
                        int outOfRangeMessageLength = 0;
        
                        if( middleDistancePlayerIDs.size() > 0 ) {
                            SimpleVector<char> messageChars;
            
                            messageChars.appendElementString( "PO\n" );
            
                            for( int i=0; 
                                 i<middleDistancePlayerIDs.size(); i++ ) {
                                char buffer[20];
                                sprintf( 
                                    buffer, "%d\n",
                                    middleDistancePlayerIDs.
                                    getElementDirect( i ) );
                                
                                messageChars.appendElementString( buffer );
                                }
                            messageChars.push_back( '#' );

                            char *outOfRangeMessageText = 
                                messageChars.getElementString();

                            outOfRangeMessageLength = 
                                strlen( outOfRangeMessageText );

                            if( outOfRangeMessageLength < 
                                maxUncompressedSize ) {
                                outOfRangeMessage = 
                                    (unsigned char*)outOfRangeMessageText;
                                }
                            else {
                                // compress 
                                outOfRangeMessage = makeCompressedMessage( 
                                    outOfRangeMessageText, 
                                    outOfRangeMessageLength, 
                                    &outOfRangeMessageLength );
                
                                delete [] outOfRangeMessageText;
                                }
                            }
                        
                        int numSent = 
                            nextPlayer->sock->send( 
                                outOfRangeMessage, 
                                outOfRangeMessageLength, 
                                false, false );
                        
                        nextPlayer->gotPartOfThisFrame = true;

                        delete [] outOfRangeMessage;

                        if( numSent != outOfRangeMessageLength ) {
                            setPlayerDisconnected( nextPlayer, 
                                                   "Socket write failed" );
                            }
                        }
                    }




                if( moveList.size() > 0 && nextPlayer->connected ) {
                    
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
                        
                        SimpleVector<MoveRecord> closeMoves;
                        
                        for( int u=0; u<movesPos.size(); u++ ) {
                            ChangePosition *p = movesPos.getElement( u );
                            
                            // move messages are never global
                            
                            double d = intDist( p->x, p->y, 
                                                playerXD, playerYD );
                    
                            if( d > maxDist ) {
                                continue;
                                }
                            closeMoves.push_back( 
                                moveList.getElementDirect( u ) );
                            }
                        
                        if( closeMoves.size() > 0 ) {
                            
                            char *moveMessageText = getMovesMessageFromList( 
                                &closeMoves, nextPlayer->birthPos );
                        
                            unsigned char *moveMessage = NULL;
                            int moveMessageLength = 0;
        
                            if( moveMessageText != NULL ) {
                                moveMessage = (unsigned char*)moveMessageText;
                                moveMessageLength = strlen( moveMessageText );

                                if( moveMessageLength > maxUncompressedSize ) {
                                    moveMessage = makeCompressedMessage( 
                                        moveMessageText,
                                        moveMessageLength,
                                        &moveMessageLength );
                                    delete [] moveMessageText;
                                    }    
                                }

                            int numSent = 
                                nextPlayer->sock->send( 
                                    moveMessage, 
                                    moveMessageLength, 
                                    false, false );
                            
                            nextPlayer->gotPartOfThisFrame = true;
                            
                            delete [] moveMessage;
                            
                            if( numSent != moveMessageLength ) {
                                setPlayerDisconnected( nextPlayer, 
                                                       "Socket write failed" );
                                }
                            }
                        }
                    }


                
                if( mapChanges.size() > 0 && nextPlayer->connected ) {
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
                        // at least one thing in map change list is close
                        // enough to this player

                        // format custom map change message for this player
                        
                        
                        unsigned char *mapChangeMessage = NULL;
                        int mapChangeMessageLength = 0;
                        SimpleVector<char> mapChangeChars;

                        for( int u=0; u<mapChanges.size(); u++ ) {
                            ChangePosition *p = mapChangesPos.getElement( u );
                        
                            double d = intDist( p->x, p->y, 
                                                playerXD, playerYD );
                            
                            if( d > maxDist ) {
                                // skip this one, too far away
                                continue;
                                }
                            MapChangeRecord *r = 
                                mapChanges.getElement( u );
                            
                            char *lineString =
                                getMapChangeLineString( 
                                    r,
                                    nextPlayer->birthPos.x,
                                    nextPlayer->birthPos.y );
                            
                            mapChangeChars.appendElementString( lineString );
                            delete [] lineString;
                            }
                        
                        
                        if( mapChangeChars.size() > 0 ) {
                            mapChangeChars.push_back( '#' );
                            char *temp = mapChangeChars.getElementString();

                            char *mapChangeMessageText = 
                                concatonate( "MX\n", temp );
                            delete [] temp;

                            mapChangeMessageLength = 
                                strlen( mapChangeMessageText );
            
                            if( mapChangeMessageLength < 
                                maxUncompressedSize ) {
                                mapChangeMessage = 
                                    (unsigned char*)mapChangeMessageText;
                                }
                            else {
                                mapChangeMessage = makeCompressedMessage( 
                                    mapChangeMessageText, 
                                    mapChangeMessageLength, 
                                    &mapChangeMessageLength );
                
                                delete [] mapChangeMessageText;
                                }
                            }

                        
                        if( mapChangeMessage != NULL ) {

                            int numSent = 
                                nextPlayer->sock->send( 
                                    mapChangeMessage, 
                                    mapChangeMessageLength, 
                                    false, false );
                            
                            nextPlayer->gotPartOfThisFrame = true;
                            
                            delete [] mapChangeMessage;

                            if( numSent != mapChangeMessageLength ) {
                                setPlayerDisconnected( nextPlayer, 
                                                       "Socket write failed" );
                                }
                            }
                        }
                    }
                if( speechMessage != NULL && nextPlayer->connected ) {
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
                        
                        nextPlayer->gotPartOfThisFrame = true;
                        
                        if( numSent != speechMessageLength ) {
                            setPlayerDisconnected( nextPlayer, 
                                                   "Socket write failed" );
                            }
                        }
                    }


                if( newLocationSpeech.size() > 0 && nextPlayer->connected ) {
                    double minUpdateDist = 64;
                    
                    for( int u=0; u<newLocationSpeechPos.size(); u++ ) {
                        ChangePosition *p = 
                            newLocationSpeechPos.getElement( u );
                        
                        // locationSpeech never global

                        double d = intDist( p->x, p->y, 
                                            playerXD, playerYD );
                        
                        if( d < minUpdateDist ) {
                            minUpdateDist = d;
                            }
                        }

                    if( minUpdateDist <= maxDist ) {
                        // some of location speech in range
                        
                        SimpleVector<char> working;
                        
                        working.appendElementString( "LS\n" );
                        
                        for( int u=0; u<newLocationSpeechPos.size(); u++ ) {
                            ChangePosition *p = 
                                newLocationSpeechPos.getElement( u );
                            
                            char *line = autoSprintf( 
                                "%d %d %s\n",
                                p->x - nextPlayer->birthPos.x, 
                                p->y - nextPlayer->birthPos.y,
                                newLocationSpeech.getElementDirect( u ) );
                            working.appendElementString( line );
                            
                            delete [] line;
                            }
                        working.push_back( '#' );
                        
                        char *message = 
                            working.getElementString();
                        int len = working.size();
                        

                        if( len > maxUncompressedSize ) {
                            int compLen = 0;
                            
                            unsigned char *compMessage = makeCompressedMessage( 
                                message, 
                                len, 
                                &compLen );
                
                            delete [] message;
                            len = compLen;
                            message = (char*)compMessage;
                            }

                        int numSent = 
                            nextPlayer->sock->send( 
                                (unsigned char*)message,
                                len, 
                                false, false );
                        
                        delete [] message;
                        
                        nextPlayer->gotPartOfThisFrame = true;
                        
                        if( numSent != len ) {
                            setPlayerDisconnected( nextPlayer, 
                                                   "Socket write failed" );
                            }
                        }
                    }
                


                // EVERYONE gets updates about deleted players                
                if( nextPlayer->connected ) {
                    
                    unsigned char *deleteUpdateMessage = NULL;
                    int deleteUpdateMessageLength = 0;
        
                    SimpleVector<char> deleteUpdateChars;
                
                    for( int u=0; u<newDeleteUpdates.size(); u++ ) {
                    
                        char *line = getUpdateLineFromRecord(
                            newDeleteUpdates.getElement( u ),
                            nextPlayer->birthPos );
                    
                        deleteUpdateChars.appendElementString( line );
                    
                        delete [] line;
                        }
                

                    if( deleteUpdateChars.size() > 0 ) {
                        deleteUpdateChars.push_back( '#' );
                        char *temp = deleteUpdateChars.getElementString();
                    
                        char *deleteUpdateMessageText = 
                            concatonate( "PU\n", temp );
                        delete [] temp;
                    
                        deleteUpdateMessageLength = 
                            strlen( deleteUpdateMessageText );

                        if( deleteUpdateMessageLength < maxUncompressedSize ) {
                            deleteUpdateMessage = 
                                (unsigned char*)deleteUpdateMessageText;
                            }
                        else {
                            // compress for all players once here
                            deleteUpdateMessage = makeCompressedMessage( 
                                deleteUpdateMessageText, 
                                deleteUpdateMessageLength, 
                                &deleteUpdateMessageLength );
                
                            delete [] deleteUpdateMessageText;
                            }
                        }

                    if( deleteUpdateMessage != NULL ) {
                        int numSent = 
                            nextPlayer->sock->send( 
                                deleteUpdateMessage, 
                                deleteUpdateMessageLength, 
                                false, false );
                    
                        nextPlayer->gotPartOfThisFrame = true;
                    
                        delete [] deleteUpdateMessage;
                    
                        if( numSent != deleteUpdateMessageLength ) {
                            setPlayerDisconnected( nextPlayer, 
                                                   "Socket write failed" );
                            }
                        }
                    }



                // EVERYONE gets lineage info for new babies
                if( lineageMessage != NULL && nextPlayer->connected ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            lineageMessage, 
                            lineageMessageLength, 
                            false, false );
                    
                    nextPlayer->gotPartOfThisFrame = true;
                    
                    if( numSent != lineageMessageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    }


                // EVERYONE gets curse info for new babies
                if( cursesMessage != NULL && nextPlayer->connected ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            cursesMessage, 
                            cursesMessageLength, 
                            false, false );
                    
                    nextPlayer->gotPartOfThisFrame = true;
                    
                    if( numSent != cursesMessageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    }

                // EVERYONE gets newly-given names
                if( namesMessage != NULL && nextPlayer->connected ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            namesMessage, 
                            namesMessageLength, 
                            false, false );
                    
                    nextPlayer->gotPartOfThisFrame = true;
                    
                    if( numSent != namesMessageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    }

                


                if( nextPlayer->foodUpdate ) {
                    // send this player a food status change
                    
                    int cap = computeFoodCapacity( nextPlayer );
                    
                    if( cap < nextPlayer->foodStore ) {
                        nextPlayer->foodStore = cap;
                        }
                    
                    int yumMult = nextPlayer->yummyFoodChain.size() - 1;
                    
                    if( yumMult < 0 ) {
                        yumMult = 0;
                        }
                    
                    if( nextPlayer->connected ) {
                        
                        char *foodMessage = autoSprintf( 
                            "FX\n"
                            "%d %d %d %d %.2f %d "
                            "%d %d\n"
                            "#",
                            nextPlayer->foodStore,
                            cap,
                            hideIDForClient( nextPlayer->lastAteID ),
                            nextPlayer->lastAteFillMax,
                            computeMoveSpeed( nextPlayer ),
                            nextPlayer->responsiblePlayerID,
                            nextPlayer->yummyBonusStore,
                            yumMult );
                        
                        int messageLength = strlen( foodMessage );
                        
                        int numSent = 
                            nextPlayer->sock->send( 
                                (unsigned char*)foodMessage, 
                                messageLength,
                                false, false );
                        
                        nextPlayer->gotPartOfThisFrame = true;
                        
                        if( numSent != messageLength ) {
                            setPlayerDisconnected( nextPlayer, 
                                                   "Socket write failed" );
                            }
                        
                        delete [] foodMessage;
                        }
                    
                    nextPlayer->foodUpdate = false;
                    nextPlayer->lastAteID = 0;
                    nextPlayer->lastAteFillMax = 0;
                    }



                if( nextPlayer->heatUpdate && nextPlayer->connected ) {
                    // send this player a heat status change
                    
                    char *heatMessage = autoSprintf( 
                        "HX\n"
                        "%.2f#",
                        nextPlayer->heat );
                     
                    int messageLength = strlen( heatMessage );
                    
                    int numSent = 
                         nextPlayer->sock->send( 
                             (unsigned char*)heatMessage, 
                             messageLength,
                             false, false );
                    
                    nextPlayer->gotPartOfThisFrame = true;
                    
                    if( numSent != messageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    
                    delete [] heatMessage;
                    }
                nextPlayer->heatUpdate = false;
                    

                if( nextPlayer->curseTokenUpdate &&
                    nextPlayer->connected ) {
                    // send this player a curse token status change
                    
                    char *tokenMessage = autoSprintf( 
                        "CX\n"
                        "%d#",
                        nextPlayer->curseTokenCount );
                     
                    int messageLength = strlen( tokenMessage );
                    
                    int numSent = 
                         nextPlayer->sock->send( 
                             (unsigned char*)tokenMessage, 
                             messageLength,
                             false, false );

                    nextPlayer->gotPartOfThisFrame = true;
                    
                    if( numSent != messageLength ) {
                        setPlayerDisconnected( nextPlayer, 
                                               "Socket write failed" );
                        }
                    
                    delete [] tokenMessage;                    
                    }
                nextPlayer->curseTokenUpdate = false;

                }
            }


        for( int u=0; u<moveList.size(); u++ ) {
            MoveRecord *r = moveList.getElement( u );
            delete [] r->formatString;
            }



        for( int u=0; u<mapChanges.size(); u++ ) {
            MapChangeRecord *r = mapChanges.getElement( u );
            delete [] r->formatString;
            }

        if( newUpdates.size() > 0 ) {
            
            SimpleVector<char> playerList;
            
            for( int i=0; i<playersReceivingPlayerUpdate.size(); i++ ) {
                char *playerString = 
                    autoSprintf( 
                        "%d, ",
                        playersReceivingPlayerUpdate.getElementDirect( i ) );
                playerList.appendElementString( playerString );
                delete [] playerString;
                }
            
            char *playerListString = playerList.getElementString();

            AppLog::infoF( "%d/%d players were sent part of a %d-line PU: %s",
                           playersReceivingPlayerUpdate.size(),
                           numLive, newUpdates.size(),
                           playerListString );
            
            delete [] playerListString;
            }
        

        for( int u=0; u<newUpdates.size(); u++ ) {
            UpdateRecord *r = newUpdates.getElement( u );
            delete [] r->formatString;
            }
        
        for( int u=0; u<newDeleteUpdates.size(); u++ ) {
            UpdateRecord *r = newDeleteUpdates.getElement( u );
            delete [] r->formatString;
            }

        
        if( speechMessage != NULL ) {
            delete [] speechMessage;
            }
        if( lineageMessage != NULL ) {
            delete [] lineageMessage;
            }
        if( cursesMessage != NULL ) {
            delete [] cursesMessage;
            }
        if( namesMessage != NULL ) {
            delete [] namesMessage;
            }
        if( dyingMessage != NULL ) {
            delete [] dyingMessage;
            }
        if( healingMessage != NULL ) {
            delete [] healingMessage;
            }
        if( emotMessage != NULL ) {
            delete [] emotMessage;
            }
        
        
        // this one is global, so we must clear it every loop
        newSpeech.deleteAll();
        newSpeechPos.deleteAll();
        
        newLocationSpeech.deallocateStringElements();
        newLocationSpeechPos.deleteAll();
        

        
        // handle end-of-frame for all players that need it
        const char *frameMessage = "FM\n#";
        int frameMessageLength = strlen( frameMessage );
        
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement(i);
            
            if( nextPlayer->gotPartOfThisFrame && nextPlayer->connected ) {
                int numSent = 
                    nextPlayer->sock->send( 
                        (unsigned char*)frameMessage, 
                        frameMessageLength,
                        false, false );

                if( numSent != frameMessageLength ) {
                    setPlayerDisconnected( nextPlayer, "Socket write failed" );
                    }
                }
            nextPlayer->gotPartOfThisFrame = false;
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
                
                if( nextPlayer->sock != NULL ) {
                    sockPoll.removeSocket( nextPlayer->sock );
                
                    delete nextPlayer->sock;
                    nextPlayer->sock = NULL;
                    }
                
                if( nextPlayer->sockBuffer != NULL ) {
                    delete nextPlayer->sockBuffer;
                    nextPlayer->sockBuffer = NULL;
                    }
                
                delete nextPlayer->lineage;
                
                if( nextPlayer->name != NULL ) {
                    delete [] nextPlayer->name;
                    }

                if( nextPlayer->lastSay != NULL ) {
                    delete [] nextPlayer->lastSay;
                    }
                
                freePlayerContainedArrays( nextPlayer );
                
                if( nextPlayer->pathToDest != NULL ) {
                    delete [] nextPlayer->pathToDest;
                    }

                if( nextPlayer->email != NULL ) {
                    delete [] nextPlayer->email;
                    }

                if( nextPlayer->murderPerpEmail != NULL ) {
                    delete [] nextPlayer->murderPerpEmail;
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
    
    // stop listening on server socket immediately, before running
    // cleanup steps.  Cleanup may take a while, and we don't want to leave
    // server socket listening, because it will answer reflector and player
    // connection requests but then just hang there.

    // Closing the server socket makes these connection requests fail
    // instantly (instead of relying on client timeouts).
    delete server;

    quitCleanup();
    
    
    AppLog::info( "Done." );

    return 0;
    }



// implement null versions of these to allow a headless build
// we never call drawObject, but we need to use other objectBank functions


void *getSprite( int ) {
    return NULL;
    }

char *getSpriteTag( int ) {
    return NULL;
    }

char isSpriteBankLoaded() {
    return false;
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

void toggleAdditiveBlend( char ) {
    }

void drawSquare( doublePair, double ) {
    }

void startAddingToStencil( char, char, float ) {
    }

void startDrawingThroughStencil( char ) {
    }

void stopStencil() {
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

