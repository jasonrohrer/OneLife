
#include "serialWebRequests.h"

#include "minorGems/game/game.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/system/Time.h"

#include "accountHmac.h"



extern int webRetrySeconds;

static int nextHandle = 0;


// can be used externally to follow current request
int currentActiveSerialWebRequest = -1;



typedef struct SerialWebRecord {
        // for finding request externally
        int handle;
        
        // handle for the active web request
        int activeHandle;
        

        char done;
        
        // false until first step done
        // don't start timing until we're actually running the request
        // (don't want to retry a request early because slow requests were
        //  queued in front of it).
        char started;
        
        timeSec_t startTime;
        
        int retryCount;

        // save for retry
        char *method;
        char *url;
        char *body;
        
        } SerialWebRecord;
        

static SimpleVector<SerialWebRecord> serialRecords;


static char serverShutdown = false;
static char playerPermadead = false;

extern char *shutdownMessage;



int startWebRequestSerial( const char *inMethod, const char *inURL,
                           const char *inBody ) {
    //if( true ) return startWebRequest( inMethod, inURL, inBody );

    SerialWebRecord r;

    r.method = NULL;
    r.url = NULL;
    r.body = NULL;
    

    if( inMethod != NULL ) {
        r.method = stringDuplicate( inMethod );
        }
    if( inURL != NULL ) {
        r.url = stringDuplicate( inURL );
        }
    if( inBody != NULL ) {
        r.body = stringDuplicate( inBody );
        }
        

    r.activeHandle = -1;
    
    r.handle = nextHandle;
    nextHandle ++;
    
    r.done = false;

    r.started = false;
    
    r.retryCount = 0;

    serialRecords.push_back( r );
    
    return r.handle;
    }



void checkForServerShutdown( SerialWebRecord *inR ) {
    char *result = getWebResult( inR->activeHandle );

    if( result != NULL ) {
        
        if( strstr( result, "SHUTDOWN" ) != NULL ) {
            serverShutdown = true;
            
            int numParts;
            char **parts = split( result, "\n", &numParts );
            
            if( numParts > 1 ) {
                
                if( shutdownMessage != NULL ) {
                    delete [] shutdownMessage;
                    }
                shutdownMessage = stringDuplicate( parts[1] );
                }
            for( int i=0; i<numParts; i++ ) {
                delete [] parts[i];
                }
            delete [] parts;
            }

        if( strstr( result, "PERMADEAD" ) != NULL ) {
            playerPermadead = true;
            }

        delete [] result;
        }
    }


static void checkForRequestRetry( SerialWebRecord *r ) {
    timeSec_t timePassed = game_timeSec() - r->startTime;

    if( timePassed > webRetrySeconds ) {
        // start a fresh request
        
        // note whether old one failed first, though
        // (we may have been ignoring the request's error state until
        //  we reached this retry)
        int stepResult = stepWebRequest( r->activeHandle );
        if( stepResult == -1 ) {
            printf( "\nWeb request %d failed\n", r->activeHandle );
            }


        int oldActiveHandle = r->activeHandle;
        
        clearWebRequest( r->activeHandle );

        r->url = replaceAccountHmac( r->url );
        r->body = replaceAccountHmac( r->body );

        r->activeHandle = startWebRequest( r->method, r->url, r->body );
        printf( "\nRestarting web request %d as %d [%s %s %s]\n\n", 
                oldActiveHandle, r->activeHandle, r->method, r->url, r->body );
        
        r->startTime = game_timeSec();
        r->retryCount++;


        // also need to generate new serial numbers for all requests that
        // come after this one, so they aren't out-of-sequence
        int numRecords = serialRecords.size();
    
        int foundIndex = -1;
        
        for( int i=0; i<numRecords; i++ ) {
            
            SerialWebRecord *otherRecord = serialRecords.getElement( i );

            if( otherRecord == r ) {
                foundIndex = i;
                break;
                }
            }

        for( int i=foundIndex + 1; i<numRecords; i++ ) {
            
            SerialWebRecord *otherRecord = serialRecords.getElement( i );
            
            if( ! otherRecord->started ) {
                otherRecord->url = replaceAccountHmac( otherRecord->url );
                otherRecord->body = replaceAccountHmac( otherRecord->body );
                }
            }
        }
    
    }


// remove house map from request print out
static char *trimMapFromRequest( char *inRequest ) {
    if( inRequest == NULL ) {
        return NULL;
        }
    
    char *working = stringDuplicate( inRequest );
    
    const char *mapTag = "house_map=";
    
    char *mapStart = strstr( working, mapTag );
    
    if( mapStart != NULL ) {
        mapStart = &( mapStart[ strlen( mapTag ) ] );
        
        // add ... and terminate
        mapStart[0] = '.';
        mapStart[1] = '.';
        mapStart[2] = '.';
        
        mapStart[3] = '\0';
        }
    
    
    return working;
    }



/*
// used for debugging

// extracts the action variable from a web request
static char *getActionFromRequest( char *inRequest ) {
    if( inRequest == NULL ) {
        return NULL;
        }
    
    char *working = stringDuplicate( inRequest );
    
    const char *actionTag = "action=";
       
    char *actionStart = strstr( working, actionTag );

    if( actionStart == NULL ) {
        delete [] working;
        return NULL;
        }
    

    int actionTagLength = strlen( actionTag );
    
    char *actionValueStart = &( actionStart[ actionTagLength ] );
    
    char *nextBoundary = strstr( working, "&" );
    
    if( nextBoundary != NULL ) {
        // terminate here    
        nextBoundary[0] = '\0';
        }
    
    char *actionValue = stringDuplicate( actionValueStart );
    
    delete [] working;
    
    return actionValue;
    }
*/



static void printRequestStart( SerialWebRecord *r ) {
    /*
    // hide contents of web request from stdout (prevent simple cheating)
    
    // at least show action value, for easier debugging
    char *action = getActionFromRequest( r->url );
    if( action == NULL ) {
        action = getActionFromRequest( r->body );
        }

    printf( "\nStarting web request %d [%s]\n\n", r->activeHandle,
            action );
    if( action != NULL ) {    
        delete [] action;
        }
    
    return;
    */

    // old:  show all parts of request being started
    
    char *trimmedBody = trimMapFromRequest( r->body );
                
    printf( "\nStarting web request %d [%s %s %s]\n\n", 
            r->activeHandle, r->method, r->url, trimmedBody );
    
    if( trimmedBody != NULL ) {
        delete [] trimmedBody;
        }
    }



int stepWebRequestSerial( int inHandle ) {

    //if( true ) return stepWebRequest( inHandle );
    

    int numRecords = serialRecords.size();
    
    for( int i=0; i<numRecords; i++ ) {
        
        SerialWebRecord *r = serialRecords.getElement( i );
        
        if( r->handle == inHandle ) {
            
            if( ! r->started ) {
                r->activeHandle = 
                    startWebRequest( r->method, r->url, r->body );
                
                printRequestStart( r );
                
                r->startTime = game_timeSec();
                r->started = true;
                
                currentActiveSerialWebRequest = r->handle;
                }            

            // found our request, just step it and return
            int stepResult = stepWebRequest( r->activeHandle );

            if( stepResult == -1 ) {
                // treat hard web errors as passing hiccups
                // act as if we're still waiting for result, and
                // let normal retry timeout pass before
                // retrying
                stepResult = 0;
                }

            if( stepResult != 0 ) {
                // not still processing (done or hit error)
                r->done = true;
                
                currentActiveSerialWebRequest = -1;
                
                if( stepResult == 1 ) {
                    checkForServerShutdown( r );
                    
                    if( serverShutdown || playerPermadead ) {
                        // block client's normal response to this
                        // request, by reporting that it's still in
                        // progress, so that client shutdown behavior
                        // can take over
                        return 0;
                        }
                    }
                }
            else {
                checkForRequestRetry( r );
                }
            

            return stepResult;
            }
        else {
            // not our request
            
            // is it a request that needs stepping?
            if( ! r->done ) {
                
                if( ! r->started ) {
                    r->activeHandle = 
                        startWebRequest( r->method, r->url, r->body );
                    
                    printRequestStart( r );

                    r->startTime = game_timeSec();
                    r->started = true;
                    
                    currentActiveSerialWebRequest = r->handle;
                    }

                int stepResult = stepWebRequest( r->activeHandle );

                if( stepResult == -1 ) {
                    // treat hard web errors as passing hiccups
                    // act as if we're still waiting for result, and
                    // let normal retry timeout pass before
                    // retrying
                    stepResult = 0;
                    }


                if( stepResult != 0 ) {
                    // not still processing (done or hit error)
                    r->done = true;
                    
                    currentActiveSerialWebRequest = -1;

                    if( stepResult == 1 ) {
                        checkForServerShutdown( r );
                        
                        if( serverShutdown || playerPermadead ) {
                            // block client's normal response to this
                            // request, by reporting that it's still in
                            // progress, so that client shutdown behavior
                            // can take over
                            return 0;
                            }
                        }
                    }
                else {
                    checkForRequestRetry( r );
                    }
                
                // OUR request not done yet, because we're still stepping
                // requests that come before in the queue
                return 0;
                }
            // else this request is in the queue, but it's done, so move
            // past it to the next request and check that
            }
        }
    

    // if we got here, we haven't found our request at all.... error?
    return -1;
    }



char *getWebResultSerial( int inHandle ) {
    int numRecords = serialRecords.size();

    for( int i=0; i<numRecords; i++ ) {
        
        SerialWebRecord *r = serialRecords.getElement( i );
        
        if( r->handle == inHandle && r->activeHandle != -1 ) {
            return getWebResult( r->activeHandle );
            }
        }
    
    return NULL;
    }



unsigned char *getWebResultSerial( int inHandle, int *outSize ) {
    int numRecords = serialRecords.size();

    for( int i=0; i<numRecords; i++ ) {
        
        SerialWebRecord *r = serialRecords.getElement( i );
        
        if( r->handle == inHandle && r->activeHandle != -1 ) {
            return getWebResult( r->activeHandle, outSize );
            }
        }
    
    return NULL;
    }





int getWebRequestRetryStatus( int inHandle ) {
    int numRecords = serialRecords.size();
    
    for( int i=0; i<numRecords; i++ ) {
        
        SerialWebRecord *r = serialRecords.getElement( i );
        
        if( r->handle == inHandle ) {
            
            if( ! r->started ) {
                return 0;
                }

            if( r->retryCount < 1 ) {
                
                timeSec_t timePassed = game_timeSec( ) - r->startTime;
                if( timePassed < webRetrySeconds / 2 ) {
                    return 0;
                    }
                else {
                    return 1;
                    }
                }
            else {
                return 1 + r->retryCount;
                }
            }
        }

    // not found?
    return 0;
    }



void clearWebRequestSerial( int inHandle ) {
    //if( true ) return clearWebRequest( inHandle );
    
    int numRecords = serialRecords.size();
    
    for( int i=0; i<numRecords; i++ ) {
        
        SerialWebRecord *r = serialRecords.getElement( i );
        
        if( r->handle == inHandle ) {
            if( r->activeHandle != -1 ) {    
                clearWebRequest( r->activeHandle );
                }
            
            if( r->method != NULL ) {
                delete [] r->method;
                }
            if( r->url != NULL ) {
                delete [] r->url;
                }
            if( r->body != NULL ) {
                delete [] r->body;
                }
            
            serialRecords.deleteElement( i );
            return;
            }
        }
    
    // else not found in queue?
    }



char getServerShutdown() {
    return serverShutdown;
    }



char getPermadead() {
    return playerPermadead;
    }

