
#include <stdio.h>


#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/Directory.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/system/Time.h"

#include "../gameSource/objectBank.h"


static FILE *logFile;

static int currentYear;
static int currentDay;
static int currentHour;



static FILE *openCurrentLogFile() {
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    char fileName[100];
    
    strftime( fileName, 99, "%Y_%m%B_%d_%A.txt", timeStruct );

    File logDir( NULL, "failureLog" );
    
    if( ! logDir.exists() ) {
        Directory::makeDirectory( &logDir );
        }

    if( ! logDir.isDirectory() ) {
        AppLog::error( "Non-directory failureLog is in the way" );
        return NULL;
        }

    File *newFile = logDir.getChildFile( fileName );
    
    char *newFileName = newFile->getFullFileName();
    
    FILE *file = fopen( newFileName, "a" );
    
    if( file == NULL ) {
        AppLog::errorF( "Failed to open log file %s", newFileName );
        }
    else {
        currentYear = timeStruct->tm_year;
        currentDay = timeStruct->tm_yday;
        }
    
    delete newFile;
    delete [] newFileName;
    
    return file;
    }




static int maxObjectID;


typedef struct FailureRecord {
    int actorID;
    int targetID;
    int failureCount;
    } FailureRecord;


// one vector per target id
static SimpleVector<FailureRecord> *failureLists;

static int maxSeenObjectID;


static void clearTallies() {
    for( int i=0; i<=maxObjectID; i++ ) {
        failureLists[i].deleteAll();
        }
    maxSeenObjectID = 0;
    }



void initFailureLog() {
    AppLog::info( "failureLog starting up" );
    
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    currentYear = timeStruct->tm_year;
    currentDay = timeStruct->tm_yday;
    currentHour = timeStruct->tm_hour;
    

    logFile = openCurrentLogFile();
    
    maxObjectID = getMaxObjectID();
    
    failureLists = new SimpleVector<FailureRecord>[ maxObjectID + 1 ];
    
    clearTallies();
    }










static void stepLog( char inForceOutput ) {
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    if( timeStruct->tm_hour != currentHour || inForceOutput ) {
        // hour change
        // add latest data averages to file
        
        fprintf( logFile, "hour=%d\n", currentHour );

        for( int i=0; i<=maxSeenObjectID; i++ ) {
            
            if( failureLists[i].size() > 0 ) {
                
                for( int j=0; j<failureLists[i].size(); j++ ) {
                    
                    FailureRecord *r = failureLists[i].getElement( j );
                    
                    fprintf( 
                        logFile, 
                        "%d + %d  count=%d\n",
                        r->actorID,
                        r->targetID,
                        r->failureCount );
                    }
                
                failureLists[i].deleteAll();
                }
            }
        
        fflush( logFile );

        maxSeenObjectID = 0;        
        currentHour = timeStruct->tm_hour;
        }
    

    if( timeStruct->tm_year != currentYear ||
        timeStruct->tm_yday != currentDay ) {

        if( logFile != NULL ) {
            fclose( logFile );
            }
        
        logFile = openCurrentLogFile();
        }    
    }



void freeFailureLog() {
    
    if( logFile != NULL ) {
        // final output
        stepLog( true );
        
        fclose( logFile );
        }
    delete [] failureLists;
    }



void stepFailureLog() {
    if( logFile != NULL ) {
        stepLog( false );
        }
    }




void logTransitionFailure( int inActorID, int inTargetID ) {
    if( logFile != NULL ) {
        stepLog( false );
        }

    if( inActorID > 0 && inTargetID > 0 && inTargetID <= maxObjectID ) {
        

        ObjectRecord *a = getObject( inActorID );
                
        if( a->isUseDummy ) {
            inActorID = a->useDummyParent;
            }

        ObjectRecord *t = getObject( inTargetID );
                
        if( t->isUseDummy ) {
            inTargetID = t->useDummyParent;
            }
    
        if( inTargetID > maxSeenObjectID ) {
            maxSeenObjectID = inTargetID;
            }

        char found = false;
        for( int i=0; i<failureLists[inTargetID].size(); i++ ) {
            FailureRecord *rt = failureLists[inTargetID].getElement( i );
            
            if( rt->actorID == inActorID && rt->targetID == inTargetID ) {
                rt->failureCount ++;
                found = true;
                break;
                }
            }
        if( ! found ) {
            // add new record
            FailureRecord r = { inActorID, inTargetID, 1 };
            failureLists[inTargetID].push_back( r );
            }    
        }
    }
