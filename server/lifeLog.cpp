
#include <stdio.h>
#include <time.h>

#include "map.h"



#include "minorGems/util/stringUtils.h"
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/Directory.h"

#include "minorGems/util/log/AppLog.h"

static FILE *logFile;

static int currentYear;
static int currentDay;


static int deadYoungEveCount = 0;


extern double forceDeathAge;


static FILE *openCurrentLogFile() {
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    char fileName[100];
    
    strftime( fileName, 99, "%Y_%m%B_%d_%A.txt", timeStruct );

    File logDir( NULL, "lifeLog" );
    
    if( ! logDir.exists() ) {
        Directory::makeDirectory( &logDir );
        }

    if( ! logDir.isDirectory() ) {
        AppLog::error( "Non-directory lifeLog is in the way" );
        return NULL;
        }

    File *newFile = logDir.getChildFile( fileName );
    
    char *newFileName = newFile->getFullFileName();
    
    FILE *file = fopen( newFileName, "a" );
    
    if( file == NULL ) {
        AppLog::errorF( "Failed to open log file %s", newFileName );
        }

    delete newFile;
    delete [] newFileName;
    
    return file;
    }



void initLifeLog() {
    AppLog::info( "lifeLog starting up" );
    
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    currentYear = timeStruct->tm_year;
    currentDay = timeStruct->tm_yday;
    

    logFile = openCurrentLogFile();
    }



void freeLifeLog() {
    if( logFile != NULL ) {
        fclose( logFile );
        }
    }



static void stepLog() {
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    if( timeStruct->tm_year != currentYear ||
        timeStruct->tm_yday != currentDay ) {

        if( logFile != NULL ) {
            fclose( logFile );
            }
        
        logFile = openCurrentLogFile();
        }    
    }




void logBirth( int inPlayerID, char *inPlayerEmail,
               int inParentID, char *inParentEmail,
               char inIsMale,
               int inMapX, int inMapY,
               int inTotalPopulation ) {
    if( logFile != NULL ) {
        stepLog();

        if( logFile != NULL ) {
        
            char *parentString;
            
            if( inParentEmail == NULL ) {
                parentString = stringDuplicate( "noParent" );
                }
            else {
                parentString = autoSprintf( "parent=%d,%s",
                                            inParentID, inParentEmail );
                }

            char genderChar = 'F';
            if( inIsMale ) {
                genderChar = 'M';
                }

            fprintf( logFile, "B %.0f %d %s %c (%d,%d) %s pop=%d\n",
                     // portable way to convert time to a double for printing
                     difftime( time( NULL ), (time_t)0 ),
                     inPlayerID, inPlayerEmail, genderChar, inMapX, inMapY, 
                     parentString,
                     inTotalPopulation );
            
            fflush( logFile );

            delete [] parentString;
            }
        }
    }



// killer email NULL if died of natural causes
void logDeath( int inPlayerID, char *inPlayerEmail,
               char inEve,
               double inAge,
               char inIsMale,
               int inMapX, int inMapY, 
               int inTotalRemainingPopulation,
               char inDisconnect, int inKillerID, 
               char *inKillerEmail ) {
    
    if( inEve ) {
        
        if( inAge > 30 ) {
            resetEveRadius();
            deadYoungEveCount = 0;
            }
        else {
            deadYoungEveCount ++;
            
            if( deadYoungEveCount > 5 ) {
                deadYoungEveCount = 0;
                doubleEveRadius();
                }
            }
        }


    if( logFile != NULL ) {
        stepLog();

        if( logFile != NULL ) {
            
            char *causeString;
        
            if( inKillerEmail == NULL ) {
                if( inDisconnect ) {
                    causeString = stringDuplicate( "disconnect" );
                    }
                else {
                    if( inAge >= forceDeathAge ) {
                        causeString = stringDuplicate( "oldAge" );
                        }
                    else {
                        causeString = stringDuplicate( "hunger" );
                        }
                    }
                }
            else {
                causeString = autoSprintf( "killer_%d_%s",
                                           inKillerID, inKillerEmail );
                }

            char genderChar = 'F';
            if( inIsMale ) {
                genderChar = 'M';
                }

            fprintf( logFile, "D %.0f %d %s age=%.2f %c (%d,%d) %s pop=%d\n",
                     // portable way to convert time to a double for printing
                     difftime( time( NULL ), (time_t)0 ),
                     inPlayerID, inPlayerEmail, 
                     inAge, genderChar,
                     inMapX, inMapY,
                     causeString,
                     inTotalRemainingPopulation );
            fflush( logFile );
            
            delete [] causeString;
            }
        }
    }


