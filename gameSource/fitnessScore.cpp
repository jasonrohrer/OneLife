#include "fitnessScore.h"
#include "accountHmac.h"
#include "message.h"


#include "minorGems/game/game.h"
#include "minorGems/game/drawUtils.h"


#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/crypto/hashes/sha1.h"
#include "minorGems/network/web/URLUtils.h"


static int webRequest = -1;

static const char *nextAction = "";

static int nextSequenceNumber = -1;

static char *readyResult = NULL;


static char *fitnessServerURL = NULL;

static int useFitnessServer = 0;


extern char *userEmail;
extern Font *mainFont;
extern Font *numbersFontFixed;


static char *leaderboardName = NULL;

static double score = -1;
static int rank = -1;


static double triggerTime = 0;


static float topShadingFade = 0;
static float bottomShadingFade = 0;



typedef struct OffspringRecord {
        char *name;
        char *relationName;
        int displayID;
        int diedSecAgo;
        double age;
        double oldScore;
        double newScore;
    } OffspringRecord;


static SimpleVector<OffspringRecord> recentOffspring;


static void freeOffspringRecord( OffspringRecord inR ) {
    delete [] inR.name;
    delete [] inR.relationName;
    }


static void freeAllOffspring() {
    for( int i=0; i<recentOffspring.size(); i++ ) {
        freeOffspringRecord( recentOffspring.getElementDirect( i ) );
        }
    recentOffspring.deleteAll();
    }




void initFitnessScore() {
    fitnessServerURL = SettingsManager::getStringSetting( 
        "fitnessServerURL", 
        "http://localhost/jcr13/fitnessServer/server.php" );
    
    useFitnessServer = SettingsManager::getIntSetting( "useFitnessServer", 0 );
    }
                                                          


void freeFitnessScore() {
    delete [] fitnessServerURL;
    fitnessServerURL = NULL;

    if( webRequest != -1 ) {
        clearWebRequest( webRequest );
        webRequest = -1;
        }
    
    if( readyResult != NULL ) {
        delete [] readyResult;
        readyResult = NULL;
        }

    if( leaderboardName != NULL ) {
        delete [] leaderboardName;
        leaderboardName = NULL;
        }

    freeAllOffspring();
    }



char usingFitnessServer() {
    return useFitnessServer;
    }
    


static void startGettingSequenceNumber() {
    if( webRequest != -1 ) {
        clearWebRequest( webRequest );
        webRequest = -1;
        }

    nextSequenceNumber = -1;
    
    if( readyResult != NULL ) {
        delete [] readyResult;
        readyResult = NULL;
        }
    

    char *encodedEmail = URLUtils::urlEncode( userEmail );

    char *url = autoSprintf( "%s?action=get_client_sequence_number"
                             "&email=%s",
                             fitnessServerURL,
                             encodedEmail );
    delete [] encodedEmail;
    
    webRequest = startWebRequest( "GET", url, NULL );
    triggerTime = game_getCurrentTime();
    
    delete [] url;
    }



void triggerFitnessScoreUpdate() {
    if( !useFitnessServer ) {
        return;
        }

    score = -1;
    rank = -1;
    
    nextAction = "get_client_score";
    startGettingSequenceNumber();
    }



void triggerFitnessScoreDetailsUpdate() {
    if( !useFitnessServer ) {
        return;
        }
    
    score = -1;
    rank = -1;
    
    topShadingFade = 0;
    bottomShadingFade = 1;
    

    freeAllOffspring();

    nextAction = "get_client_score_details";
    startGettingSequenceNumber();
    }



static void shortenLongString( char *inString ) {
    
    while( mainFont->measureString( inString ) >= 460 ) {
        
        // truncate by 2 each time, to speed up the process
        int len = strlen( inString );
        
        inString[ len - 5 ] = '.';
        inString[ len - 4 ] = '.';
        inString[ len - 3 ] = '.';
        inString[ len - 2 ] = '\0';
        }
    }
            


static void stepActiveRequest() {
    if( webRequest == -1 ) {
        return;
        }
    int result = stepWebRequest( webRequest );
    
    if( result == -1 ) {
        // error
        clearWebRequest( webRequest );
        webRequest = -1;
        return;
        }
    
    if( result == 1 &&
        game_getCurrentTime() - triggerTime < 1 ) {
        // wait 1 second to avoid flicker
        return;
        }

    if( result == 1 ) {
        // done!
        char *resultText = getWebResult( webRequest );

        clearWebRequest( webRequest );
        webRequest = -1;


        if( nextSequenceNumber == -1 ) {
            // fetching sequence number
            sscanf( resultText, "%d", &nextSequenceNumber );
            delete [] resultText;


            // use sequence number to make next request

            char *pureKey = getPureAccountKey();

            char *stringToHash = autoSprintf( "%d", nextSequenceNumber );
            
            char *hash = hmac_sha1( pureKey, stringToHash );

            delete [] pureKey;
            delete [] stringToHash;

            char *encodedEmail = URLUtils::urlEncode( userEmail );

            char *url = autoSprintf( "%s?action=%s"
                                     "&email=%s"
                                     "&sequence_number=%d"
                                     "&hash_value=%s",
                                     fitnessServerURL,
                                     nextAction,
                                     encodedEmail,
                                     nextSequenceNumber,
                                     hash );
            delete [] encodedEmail;
            delete [] hash;
    
            webRequest = startWebRequest( "GET", url, NULL );
            
            delete [] url;
            }
        else {
            // pass result out whole

            readyResult = resultText;
            }        
        }


    if( readyResult != NULL ) {    
        // in either case, scan score, name, and rank
        
        if( leaderboardName != NULL ) {
            delete [] leaderboardName;
            }
        leaderboardName = new char[100];
        
        sscanf( readyResult, "%99s\n%lf\n%d", leaderboardName, &score, &rank );

        char *leaderUpper = stringToUpperCase( leaderboardName );
        delete [] leaderboardName;
        
        char found;
        leaderboardName = replaceAll( leaderUpper, "_", " ", &found );
        delete [] leaderUpper;



        if( strcmp( nextAction, "get_client_score_details" ) == 0 ) {
            // parse details too
            SimpleVector<char *> *lines = tokenizeString( readyResult );

            // skip already-parsed header and OK at end
            for( int i=3; i< lines->size()-1; i++ ) {
                
                char *line = lines->getElementDirect( i );
                
                // Eve_Jones,You,353,4495,14.7841,1.32155,2.6678
                
                int numParts;
                char **parts = split( line, ",", &numParts );
                
                OffspringRecord r;
                char *nameWorking = stringToUpperCase( parts[0] );
                char *relationWorking = stringToUpperCase( parts[1] );
                
                char found;
                r.name = replaceAll( nameWorking, "_", " ", &found );
                delete [] nameWorking;
                
                shortenLongString( r.name );

                r.relationName = 
                    replaceAll( relationWorking, "_", " ", &found );
                delete [] relationWorking;

                shortenLongString( r.relationName );
                

                sscanf( parts[2], "%d", &( r.displayID ) );
                sscanf( parts[3], "%d", &( r.diedSecAgo ) );
                sscanf( parts[4], "%lf", &( r.age ) );
                sscanf( parts[5], "%lf", &( r.oldScore ) );
                sscanf( parts[6], "%lf", &( r.newScore ) );
                
                for( int j=0; j<numParts; j++ ) {
                    delete [] parts[j];
                    }
                delete [] parts;
                
                recentOffspring.push_back( r );
                }

            lines->deallocateStringElements();
            delete lines;
            }
        }
    
    }




const char *getRankSuffix() {
    
    const char *rankSuffix = "TH";
    
    switch( rank % 10 ) {
        case 1:
            rankSuffix = "ST";
            break;
        case 2:
            rankSuffix = "ND";
            break;
        case 3:
            rankSuffix = "RD";
            break;
        default:
            rankSuffix = "TH";
            break;
        }

    return rankSuffix;
    }



// These draw nothing if latest data (after last trigger) not ready yet

void drawFitnessScore( doublePair inPos, char inMoreDigits ) {
    if( !useFitnessServer ) {
        return;
        }

    if( score != -1 ) {        
        
        const char *rankSuffix = getRankSuffix();        

        char *scoreString;
        
        if( inMoreDigits ) {
            scoreString = autoSprintf( "%0.2f", score );
            }
        else {
            scoreString = autoSprintf( "%0.1f", score );
            }
        
        char *message = 
            autoSprintf( translate( "scoreMessage" ), 
                         scoreString, rank, rankSuffix );
        
        delete [] scoreString;

        drawMessage( message, inPos );

        delete [] message;
        }
    else {
        stepActiveRequest();
        }
    }




static void drawFadeRect( doublePair inBottomLeft, doublePair inTopRight,
                          FloatColor inBottomColor, FloatColor inTopColor ) {
    
    double vert[8] = 
        { inBottomLeft.x, inBottomLeft.y,
          inBottomLeft.x, inTopRight.y,
          inTopRight.x, inTopRight.y,
          inTopRight.x, inBottomLeft.y };
    
    float vertColor[16] = 
        { inBottomColor.r, inBottomColor.g, inBottomColor.b, inBottomColor.a,
          inTopColor.r, inTopColor.g, inTopColor.b, inTopColor.a,
          inTopColor.r, inTopColor.g, inTopColor.b, inTopColor.a,
          inBottomColor.r, inBottomColor.g, inBottomColor.b, inBottomColor.a };
    
    drawQuads( 1, vert, vertColor );
    }




void drawFitnessScoreDetails( doublePair inPos, int inSkip ) {
    if( !useFitnessServer ) {
        return;
        }

    

    if( score != -1 ) {
        doublePair namePos = inPos;
        namePos.x -= 150;
        namePos.y += 8;
        
        const char *rankSuffix = getRankSuffix();

        char *leaderboardString = 
            autoSprintf( translate( "leaderboardMessage" ), 
                         rank, rankSuffix, leaderboardName );
        
        drawMessage( leaderboardString, namePos );
        delete [] leaderboardString;

        
        //drawFitnessScore( scorePos, true );
        

        inPos.y -= 85;
        
        doublePair titlePos = inPos;
        
        titlePos.x -= 480;
        
        mainFont->drawString( 
            translate( "geneticHistoryTitle" ), titlePos, alignLeft );
        
        titlePos = inPos;

        // 7 extra, to line up with fixed with number column
        titlePos.x += 480 + 7;

        
        mainFont->drawString( 
            translate( "fitnessTitle" ), titlePos, alignRight );
        

        inPos.y -= 65;
        
        doublePair startPos = inPos;
        
        FloatColor bgColor = { 0.2, 0.2, 0.2, 1.0 };
        FloatColor bgColorAlt = { 0.1, 0.1, 0.1, 1.0 };

        if( inSkip % 2 == 1 ) {
            FloatColor temp = bgColor;
            bgColor = bgColorAlt;
            bgColorAlt = temp;
            }

        int indLimit = recentOffspring.size();
        
        if( indLimit - inSkip > 8 ) {
            indLimit = inSkip + 8;
            }
        
        for( int i=inSkip; i<indLimit; i++ ) {
            setDrawColor( bgColor );
            
            OffspringRecord r = recentOffspring.getElementDirect( i );
            
            drawRect( inPos, 500, 40 );
            
            setDrawColor( 1, 1, 1, 1.0 );
            
            doublePair pos = inPos;
            
            pos.y += 16;
            pos.x -= 480;


            mainFont->drawString( r.name, pos, alignLeft );
            
            pos.y -= 32;
            
            mainFont->drawString( r.relationName, pos, alignLeft );
            
            pos.x = inPos.x;
            
            int yearsAgo = r.diedSecAgo / 60;
            
            char *diedAgoString;
            
            if( yearsAgo == 1 ) {
                diedAgoString = autoSprintf( "%s %d %s", 
                                             translate( "died" ),
                                             yearsAgo,
                                             translate( "yearAgo" ) );
                }
            else {
                diedAgoString = autoSprintf( "%s %d %s", 
                                             translate( "died" ),
                                             yearsAgo,
                                             translate( "yearsAgo" ) );
                }
            

            mainFont->drawString( diedAgoString, pos, alignLeft );
            delete [] diedAgoString;
            
            pos.y += 32;
            
            char *ageString = autoSprintf( "%s %0.1f",
                                           translate( "age" ),
                                           r.age );
            
            mainFont->drawString( ageString, pos, alignLeft );
            
            delete [] ageString;
            
            pos.x = inPos.x + 360;
            
            double scoreDelt = r.newScore - r.oldScore;
            
            char *deltString;

            if( scoreDelt > 0 ) {
                setDrawColor( 0.0, 1.0, 0.0, 1.0 );
            
                deltString = autoSprintf( "+%0.2f", scoreDelt );
                }
            else {
                setDrawColor( 1.0, 0.0, 0.0, 1.0 );
            
                deltString = autoSprintf( "%0.2f", scoreDelt );
                }
            
            numbersFontFixed->drawString( deltString, pos, alignRight );
            
            delete [] deltString;

            pos.x = inPos.x + 480;
            
            setDrawColor( 1, 1, 1, 1 );
            
            char *scoreString = autoSprintf( "%0.2f", r.newScore );
            
            numbersFontFixed->drawString( scoreString, pos, alignRight );


            delete [] scoreString;
            
            
            //swap
            FloatColor temp = bgColor;
            bgColor = bgColorAlt;
            bgColorAlt = temp;

            inPos.y -= 80;
            }

        
        if( recentOffspring.size() - inSkip > 6 ) {
            // off bottom
            
            // instant fade-in
            bottomShadingFade = 1;
            }
        else {
            if( bottomShadingFade > 0 ) {
                bottomShadingFade -= 0.1;
                if( bottomShadingFade < 0 ) {
                    bottomShadingFade = 0;
                    }
                }
            }
        
        
        if( bottomShadingFade ) {
            
            doublePair bottom  = { startPos.x - 500, startPos.y - 510 };
            doublePair top = { startPos.x + 500, startPos.y - 400 };
            
            FloatColor bottomColor = { 0, 0, 0, bottomShadingFade };
            FloatColor topColor = { 0, 0, 0, 0 };
            
            drawFadeRect( bottom, top,
                          bottomColor, topColor );
            }
        

        if( inSkip > 0 ) {
            // off top too
            if( topShadingFade < 1 ) {
                topShadingFade += 0.1;
                if( topShadingFade > 1 ) {
                    topShadingFade = 1;
                    }
                }
            }
        else {
            if( topShadingFade > 0 ) {
                topShadingFade -= 0.1;
                if( topShadingFade < 0 ) {
                    topShadingFade = 0;
                    }
                }
            }

        if( topShadingFade ) {
            
            doublePair bottom  = { startPos.x - 500, startPos.y - 70 };
            doublePair top = { startPos.x + 500, startPos.y + 40 };
            
            FloatColor bottomColor = { 0, 0, 0, 0 };
            FloatColor topColor = { 0, 0, 0, topShadingFade };
            
            drawFadeRect( bottom, top,
                          bottomColor, topColor );
            }


        }
    else {
        stepActiveRequest();
        }
    }




int getMaxFitnessListSkip() {
    return recentOffspring.size() - 6;
    }





char isFitnessScoreReady() {
    if( score != -1 ) {
        return true;
        }
    return false;
    }


