
#include <stdio.h>
#include <time.h>


#include "minorGems/util/stringUtils.h"
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/Directory.h"

#include "minorGems/util/log/AppLog.h"

static FILE *logFile;

static int currentYear;
static int currentDay;


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
               int inMapX, int inMapY ) {
    if( logFile != NULL ) {
        stepLog();

        if( logFile != NULL ) {
        
            fprintf( logFile, "B %f %d %s (%d,%d)\n",
                     // portable way to convert time to a double for printing
                     difftime( time( NULL ), (time_t)0 ),
                     inPlayerID, inPlayerEmail, inMapX, inMapY );
            fflush( logFile );
            }
        }
    }



// killer email NULL if died of natural causes
void logDeath( int inPlayerID, char *inPlayerEmail,
               int inMapX, int inMapY, int inKillerID, 
               char *inKillerEmail ) {


    if( logFile != NULL ) {
        stepLog();

        if( logFile != NULL ) {
            
            char *causeString;
        
            if( inKillerEmail == NULL ) {
                causeString = stringDuplicate( "hunger" );
                }
            else {
                causeString = autoSprintf( "killer_%d_%s",
                                           inKillerID, inKillerEmail );
                }
            
            fprintf( logFile, "D %f %d %s (%d,%d) %s\n",
                     // portable way to convert time to a double for printing
                     difftime( time( NULL ), (time_t)0 ),
                     inPlayerID, inPlayerEmail, inMapX, inMapY,
                     causeString );
            fflush( logFile );
            
            delete [] causeString;
            }
        }
    }


