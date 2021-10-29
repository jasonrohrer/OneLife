#include "arcReport.h"
#include "serverCalls.h"

#include "minorGems/network/web/WebRequest.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/crypto/hashes/sha1.h"


extern double secondsPerYear;

// only one request at a time
static WebRequest *webRequest = NULL;
static int sequenceNumber = -1;


static char *arcName = NULL;

static SimpleVector<char*> wordList;

static double arcStartTime;



void freeArcReport() {
    if( webRequest != NULL ) {
        delete webRequest;
        webRequest = NULL;
        }
    sequenceNumber = -1;

    if( arcName != NULL ) {
        delete [] arcName;
        }
    arcName = NULL;

    wordList.deallocateStringElements();
    }



void reportArcEnd() {
    if( webRequest != NULL ) {
        // already have a request pending
        return;
        }
    
    char sendReport = 
        SettingsManager::getIntSetting( "useArcServer", 0 ) &&
        SettingsManager::getIntSetting( "remoteReport", 0 );    
    
    if( !sendReport ) {
        return;
        }
    

    char *serverURL = 
        SettingsManager::getStringSetting( "arcServerURL", "" );
    
    if( strcmp( serverURL, "" ) == 0 ) {
        delete [] serverURL;
        return;
        }
    
    char *url = autoSprintf( "%s?action=get_sequence_number", serverURL );
    delete [] serverURL;

    sequenceNumber = -1;
    
    printf( "Starting new web request for %s\n", url );
    
    webRequest = defaultTimeoutWebRequest( url );
    delete [] url;

    resetArcName();
    }




void stepArcReport() {
    if( webRequest == NULL ) {
        return;
        }

    int v = webRequest->step();
    
    if( v == -1 ) {
        // error
        delete webRequest;
        webRequest = NULL;
        return;
        }
    
    if( v == 0 ) {
        return;
        }
    

    // result ready
    char *result = webRequest->getResult();
    delete webRequest;
    webRequest = NULL;
    
    
    if( sequenceNumber == -1 ) {
        sscanf( result, "%d", &sequenceNumber );
        
        if( sequenceNumber != -1 ) {
            // got a valid number
            // start report

            char *arcServerURL = 
                SettingsManager::getStringSetting( "arcServerURL", "" );

            char *sharedSecret = 
                SettingsManager::getStringSetting( 
                    "arcServerSharedSecret", "" );

            char *seqString = autoSprintf( "%d", sequenceNumber );
                    
            char *hash = hmac_sha1( sharedSecret,
                                    seqString );
            delete [] seqString;
            delete [] sharedSecret;

            char *serverName = 
                SettingsManager::getStringSetting( "serverID", "" );

            
            char *url = autoSprintf( 
                "%s?action=report_arc_end"
                "&server_name=%s"
                "&seconds_per_year=%f"
                "&sequence_number=%d"
                "&hash_value=%s",
                arcServerURL,
                serverName,
                secondsPerYear,
                sequenceNumber,
                hash );
                    
            
            delete [] arcServerURL;
            delete [] hash;
            delete [] serverName;

            
            printf( "Starting new web request for %s\n", url );
            
            webRequest = defaultTimeoutWebRequest( url );
            delete [] url;
            }
        }
    else {
        // this was our main request
        // result doesn't matter
        }
    
    delete [] result;
    }





char *getArcName() {
    if( arcName == NULL ) {
        arcName = SettingsManager::getSettingContents( "arcName", "" );
    
        if( strcmp( arcName, "" ) == 0 ) {
            resetArcName();
            }
        }
    return arcName;
    }



#include "minorGems/util/random/JenkinsRandomSource.h"

static JenkinsRandomSource randSource;

void resetArcName() {
    if( arcName != NULL ) {
        delete [] arcName;
        arcName = NULL;
        }
    

    if( wordList.size() == 0 ) {
        
        FILE *f = fopen( "wordList.txt", "r" );
        
        if( f != NULL ) {
            
            int numRead = 1;
            
            char buff[100];
            
            while( numRead == 1 ) {
                numRead = fscanf( f, "%99s", buff );
                
                if( numRead == 1 ) {
                    wordList.push_back( stringDuplicate( buff ) );
                    }
                }
            fclose( f );
            }
        }
    
    if( wordList.size() > 20 ) {
        // enough to work with
       
        randSource.reseed(  
            (unsigned int)fmod( Time::getCurrentTime(), UINT_MAX ) );
        
        char *pickedWord[2];
        
        for( int i=0; i<2; i++ ) {
            pickedWord[i] = 
                wordList.getElementDirect( randSource.getRandomBoundedInt( 
                                               0, wordList.size() - 1 ) );
            }
        char *name = autoSprintf( "%s %s",
                                  pickedWord[0],
                                  pickedWord[1] );
        arcName = stringToUpperCase( name );
        delete [] name;
        }
    else {
        arcName = stringDuplicate( "MYSTERIOUS" );
        }

    SettingsManager::setSetting( "arcName", arcName );

    
    arcStartTime = Time::getCurrentTime();
    
    SettingsManager::setSetting( "arcStartTime", arcStartTime );
    }



double getArcRunningSeconds() {
    if( arcStartTime == 0 ) {

        arcStartTime = SettingsManager::getFloatSetting( "arcStartTime", 0.0f );
        
        if( arcStartTime == 0 ) {
            arcStartTime = Time::getCurrentTime();
            SettingsManager::setSetting( "arcStartTime", arcStartTime );
            }
        }
    return Time::getCurrentTime() - arcStartTime;
    }



// counting number of milestones
static int lastMilestone = 0;

int getArcYearsToReport( double inSecondsPerYear,
                         int inMilestoneYears ) {

    double years = getArcRunningSeconds() / inSecondsPerYear;
    
    int milestoneCount = lrint( floor( years / inMilestoneYears ) );

    if( milestoneCount > lastMilestone ) {
        lastMilestone = milestoneCount;
        
        return milestoneCount * inMilestoneYears;
        }
    
    return -1;
    }





