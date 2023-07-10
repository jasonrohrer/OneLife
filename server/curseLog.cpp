
#include <stdio.h>


#include "curses.h"



#include "minorGems/util/stringUtils.h"
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/Directory.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/system/Time.h"

static FILE *logFile;

static int currentYear;
static int currentDay;




static void openCurrentLogFiles() {
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    char fileName[100];

    File logDir( NULL, "curseLog" );
    
    if( ! logDir.exists() ) {
        Directory::makeDirectory( &logDir );
        }

    if( ! logDir.isDirectory() ) {
        AppLog::error( "Non-directory curseLog is in the way" );
        return;
        }

    strftime( fileName, 99, "%Y_%m%B_%d_%A.txt", timeStruct );
    
    File *newFile = logDir.getChildFile( fileName );
    
    char *newFileName = newFile->getFullFileName();
    
    logFile = fopen( newFileName, "a" );

    delete newFile;
    
    if( logFile == NULL ) {
        AppLog::errorF( "Failed to open log file %s", newFileName );
        delete [] newFileName;

        return;
        }

    // only set these if opened successfully
    currentYear = timeStruct->tm_year;
    currentDay = timeStruct->tm_yday;    

    delete [] newFileName;
    }



void initCurseLog() {
    AppLog::info( "curseLog starting up" );
    
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    currentYear = timeStruct->tm_year;
    currentDay = timeStruct->tm_yday;
    

    openCurrentLogFiles();

    if( logFile != NULL ) {

        fprintf( logFile, "START %.0f\n", Time::timeSec() );
        
        fflush( logFile );
        }
    }



void freeCurseLog() {
    if( logFile != NULL ) {
        fprintf( logFile, "STOP %.0f\n", Time::timeSec() );
        
        fflush( logFile );

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
            logFile = NULL;
            }
        
        openCurrentLogFiles();
        }    
    }



void logCurse( int inPlayerID, char *inPlayerEmail,
               char *inTargetPlayerEmail ) {
    
    if( logFile != NULL ) {
        stepLog();

        if( logFile != NULL ) {

            fprintf( logFile, "C %.0f %d %s => %s\n",
                     Time::timeSec(),
                     inPlayerID, inPlayerEmail, 
                     inTargetPlayerEmail );
            
            fflush( logFile );
            }
        }
    }



void logUnCurse( int inPlayerID, char *inPlayerEmail,
                 char *inTargetPlayerEmail ) {
    
    if( logFile != NULL ) {
        stepLog();

        if( logFile != NULL ) {

            fprintf( logFile, "F %.0f %d %s => %s\n",
                     Time::timeSec(),
                     inPlayerID, inPlayerEmail, 
                     inTargetPlayerEmail );
            
            fflush( logFile );
            }
        }
    }




void logTrust( int inPlayerID, char *inPlayerEmail,
               char *inTargetPlayerEmail ) {
    
    if( logFile != NULL ) {
        stepLog();

        if( logFile != NULL ) {

            fprintf( logFile, "T %.0f %d %s => %s\n",
                     Time::timeSec(),
                     inPlayerID, inPlayerEmail, 
                     inTargetPlayerEmail );
            
            fflush( logFile );
            }
        }
    }



void logCurseScore( char *inPlayerEmail,
                    int inCurseScore ) {
    
    if( logFile != NULL ) {
        stepLog();

        if( logFile != NULL ) {

            fprintf( logFile, "S %.0f %s %d\n",
                     Time::timeSec(), inPlayerEmail,
                     inCurseScore );
            
            fflush( logFile );
            }
        }
    }

