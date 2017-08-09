
#include <stdio.h>


#include "minorGems/util/stringUtils.h"
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

    File logDir( NULL, "foodLog" );
    
    if( ! logDir.exists() ) {
        Directory::makeDirectory( &logDir );
        }

    if( ! logDir.isDirectory() ) {
        AppLog::error( "Non-directory foodLog is in the way" );
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




int maxObjectID;

// per id
int *eatFoodCounts;
int *eatFoodValueCounts;
double *eaterAgeSums;
doublePair *mapLocationSums;

int maxSeenObjectID;


void clearTallies() {
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
        // FIXME
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



void freeFoodLog() {
    // final output
    stepLog( true );
    
    if( logFile != NULL ) {
        fclose( logFile );
        }
    delete [] eatFoodCounts;
    delete [] eatFoodValueCounts;
    delete [] eaterAgeSums;
    delete [] mapLocationSums;
    }




void logEating( int inFoodID, int inFoodValue, double inEaterAge,
                int inMapX, int inMapY ) {
    
    if( logFile != NULL ) {
        stepLog( false );
        }


    
    eatFoodCounts[ inFoodID ] ++;
    eatFoodValueCounts[ inFoodID ] += inFoodValue;
    eaterAgeSums[ inFoodID ] += inEaterAge;
    mapLocationSums[ inFoodID ].x += inMapX;
    mapLocationSums[ inFoodID ].y += inMapY;

    if( inFoodID > maxSeenObjectID ) {
        maxSeenObjectID = inFoodID;
        }
    }
