#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "message.h"

extern Font *mainFont;
extern Font *numbersFontFixed;
extern char *userEmail;
extern char isAHAP;


static int webRequest = -1;

static int numTokens = -1;
static int secondsPerToken = -1;
static int secondsLeft = -1;
static int cap = -1;

static double secondsLeftBaseTime = -1;

static double triggerTime = -1;


void initLifeTokens() {
    numTokens = -1;
    secondsPerToken = -1;
    secondsLeft = -1;
    cap = -1;
    secondsLeftBaseTime = -1;
    }


void freeLifeTokens() {
    if( webRequest > -1 ) {
        clearWebRequest( webRequest );
        webRequest = -1;
        }
    }



void triggerLifeTokenUpdate() {
    if( webRequest == -1 &&
        SettingsManager::getIntSetting( "useLifeTokenServer", 1 ) &&
        userEmail != NULL &&
        strcmp( userEmail, "" ) != 0 ) {

        numTokens = -1;
        secondsPerToken = -1;
        secondsLeft = -1;
        cap = -1;
        secondsLeftBaseTime = -1;
        
        char *emailEncoded = URLUtils::urlEncode( userEmail );
        
        char *serverURL;
        
        if( isAHAP ) {
            serverURL = SettingsManager::getStringSetting( 
                "ahapLifeTokenServerURL",
                "http://onehouronelife.com/lifeTokenServer/server.php" );
            }
        else {
            serverURL = SettingsManager::getStringSetting( 
                "lifeTokenServerURL",
                "http://onehouronelife.com/lifeTokenServer/server.php" );
            }

        
        char *url = autoSprintf( 
            "%s?action=get_token_count&email=%s",
            serverURL, emailEncoded );
        
        delete [] serverURL;
        delete [] emailEncoded;

        webRequest = startWebRequest( "GET", url, NULL );
        triggerTime = game_getCurrentTime();
        
        delete [] url;
        }
    }



static void processUpdate() {
    if( webRequest != -1 ) {
        
        int result = stepWebRequest( webRequest );

        if( result == 1 &&
            game_getCurrentTime() - triggerTime < 1 ) {
            // wait 1 second to avoid flicker
            return;
            }
        
        if( result != 0 ) {
            // done or error

            if( result == 1 ) {
                char *text = getWebResult( webRequest );
                
                if( strstr( text, "DENIED" ) == NULL ) {
                    
                    sscanf( text,
                            "%d\n%d\n%d\n%d\nOK",
                            &numTokens,
                            &secondsPerToken,
                            &cap,
                            &secondsLeft );

                    secondsLeftBaseTime = game_getCurrentTime();
                    }
                delete [] text;
                }

            clearWebRequest( webRequest );
            webRequest = -1;
            }
                
        }
    }



int getNumLifeTokens() {
    processUpdate();
    
    return numTokens;
    }



char *getLifeTokenString() {
    processUpdate();
    
    const char *firstLifeKey = "livesPlural";
    
    const char *secondLifeKey = "livesPlural";
    
    
    if( numTokens == 1 ) {
        firstLifeKey = "lifeSingular";
        }

    if( cap == 1 ) {
        secondLifeKey = "lifeSingular";
        }
    
    int minutes = secondsPerToken / 60;
    
    const char *minuteKey = "minutesPlural";
    
    if( minutes == 1 ) {
        minuteKey = "minuteSingular";
        }
    

    return autoSprintf( translate( "tokenMessage" ), 
                        numTokens, cap, translate( firstLifeKey ),
                        minutes,
                        translate( minuteKey ),
                        cap, translate( secondLifeKey ) );
    }


void getLifeTokenTime( int *outHours, int *outMinutes, int *outSeconds ) {
    processUpdate();

    // account for time that has passed since we fetched this value
    int remain = secondsLeft - ( game_getCurrentTime() - secondsLeftBaseTime );
    
    if( remain < 0 ) {
        // wait 1 second after sitting on 0 seconds in display
        // before fetching latest
        if( remain < -1 ) {
            triggerLifeTokenUpdate();
            }
        
        remain = 0;
        }

    *outHours = remain / 3600;

    remain -= *outHours * 3600;

    *outMinutes = remain / 60;

    remain -= *outMinutes * 60;

    *outSeconds = remain;
    }





void drawTokenMessage( doublePair inPos ) {
    int numTokens = getNumLifeTokens();

    if( numTokens != -1 &&
        // don't show this message if we have 5 or more lives left
        // don't bother the user with this information unless they are
        // close to running out of lives
        numTokens < 5 ) {
        
        if( numTokens > 0 ) {
            char *message = getLifeTokenString();
            
            drawMessage( message, inPos );
            
            delete [] message;
            }
        else if( numTokens == 0 ) {
            int h, m, s;
            
            getLifeTokenTime( &h, &m, &s );

            setDrawColor( 1, 1, 1, 1.0 );
            mainFont->drawString( translate( "tokenTimeMessage" ), 
                                  inPos, alignRight );

            char *timeString = autoSprintf( "%d:%02d:%02d", h, m, s );
            
            numbersFontFixed->drawString( timeString, 
                                          inPos, alignLeft );

            delete [] timeString;
            }
        }
    
    }


