
#include <stdio.h>


#include "minorGems/util/stringUtils.h"
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/Directory.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/system/Time.h"

#include "../gameSource/objectBank.h"


static FILE *logFile;
static FILE *logDetailFile;

static int currentYear;
static int currentDay;
static int currentHour;



static FILE *openCurrentLogFile( const char *inDirName ) {
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    char fileName[100];
    
    strftime( fileName, 99, "%Y_%m%B_%d_%A.txt", timeStruct );

    File logDir( NULL, inDirName );
    
    if( ! logDir.exists() ) {
        Directory::makeDirectory( &logDir );
        }

    if( ! logDir.isDirectory() ) {
        AppLog::errorF( "Non-directory %s is in the way", inDirName );
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




static FILE *openCurrentLogFile() {
    return openCurrentLogFile( "foodLog" );
    }


static FILE *openCurrentDetailLogFile() {
    return openCurrentLogFile( "foodLogDetail" );
    }




static int maxObjectID;

// per id
int *eatFoodCounts;
int *eatFoodValueCounts;
double *eaterAgeSums;
doublePair *mapLocationSums;

static int maxSeenObjectID;


static void clearTallies() {
    for( int i=0; i<=maxObjectID; i++ ) {
        eatFoodCounts[i] = 0;
        eatFoodValueCounts[i] = 0;
        eaterAgeSums[i] = 0.0;
        mapLocationSums[i].x = 0.0;
        mapLocationSums[i].y = 0.0;
        }
    maxSeenObjectID = 0;
    }



void initFoodLog() {
    AppLog::info( "foodLog starting up" );
    
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    currentYear = timeStruct->tm_year;
    currentDay = timeStruct->tm_yday;
    currentHour = timeStruct->tm_hour;
    

    logFile = openCurrentLogFile();
    logDetailFile = openCurrentDetailLogFile();
    
    
    maxObjectID = getMaxObjectID();
    
    eatFoodCounts = new int[ maxObjectID + 1 ];
    eatFoodValueCounts = new int[ maxObjectID + 1 ];
    eaterAgeSums = new double[ maxObjectID + 1 ];
    mapLocationSums = new doublePair[ maxObjectID + 1 ];
    
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
            
            if( eatFoodCounts[i] > 0 ) {
                
                fprintf( 
                    logFile, 
                    "id=%d count=%d value=%d av_age=%f "
                    "av_mapX=%d av_mapY=%d\n",
                    i, eatFoodCounts[i], eatFoodValueCounts[i],
                    eaterAgeSums[i] / eatFoodCounts[i],
                    (int)lrint( mapLocationSums[i].x / eatFoodCounts[i] ),
                    (int)lrint( mapLocationSums[i].y / eatFoodCounts[i] ) );
                
                
                eatFoodCounts[i] = 0;
                eatFoodValueCounts[i] = 0;
                eaterAgeSums[i] = 0.0;
                mapLocationSums[i].x = 0.0;
                mapLocationSums[i].y = 0.0;
                }
            }
        
        fflush( logFile );

        maxSeenObjectID = 0;        
        currentHour = timeStruct->tm_hour;
        }
    

    if( timeStruct->tm_year != currentYear ||
        timeStruct->tm_yday != currentDay ) {
        
        // open new files each day change

        if( logFile != NULL ) {
            fclose( logFile );
            }
        if( logDetailFile != NULL ) {
            fclose( logDetailFile );
            }
        
        logFile = openCurrentLogFile();
        logDetailFile = openCurrentDetailLogFile();
        }    
    }



void freeFoodLog() {
    
    if( logFile != NULL ) {
        // final output
        stepLog( true );
        
        fclose( logFile );
        }
    
    if( logDetailFile != NULL ) {
        fclose( logDetailFile );
        }
    
    delete [] eatFoodCounts;
    delete [] eatFoodValueCounts;
    delete [] eaterAgeSums;
    delete [] mapLocationSums;
    }



void stepFoodLog() {
    if( logFile != NULL ) {
        stepLog( false );
        }
    // logDetailFile doesn't need stepping
    }




void logEating( int inPlayerID, 
                int inFoodID, int inFoodValue, double inEaterAge,
                int inMapX, int inMapY ) {
    
    if( logFile != NULL ) {
        stepLog( false );
        }

    int idToLog = inFoodID;
    
    ObjectRecord *o = getObject( idToLog );
                
    if( o->isUseDummy ) {
        idToLog = o->useDummyParent;
        }
    
    eatFoodCounts[ idToLog ] ++;
    eatFoodValueCounts[ idToLog ] += inFoodValue;
    eaterAgeSums[ idToLog ] += inEaterAge;
    mapLocationSums[ idToLog ].x += inMapX;
    mapLocationSums[ idToLog ].y += inMapY;

    if( idToLog > maxSeenObjectID ) {
        maxSeenObjectID = idToLog;
        }

    if( logDetailFile != NULL ) {
        fprintf( logDetailFile, "%.2f %d %d\n", 
                 Time::getCurrentTime(), inPlayerID, idToLog );
        }

    }
