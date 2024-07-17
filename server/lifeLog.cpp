
#include <stdio.h>

#include "map.h"
#include "playerStats.h"
#include "lineageLog.h"

#include "curses.h"

#include "../gameSource/objectBank.h"



#include "minorGems/util/stringUtils.h"
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/Directory.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/system/Time.h"

static FILE *logFile;
static FILE *nameLogFile;

static int currentYear;
static int currentDay;


static int deadYoungEveCount = 0;


extern double forceDeathAge;


static void openCurrentLogFiles() {
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    char fileName[100];

    File logDir( NULL, "lifeLog" );
    
    if( ! logDir.exists() ) {
        Directory::makeDirectory( &logDir );
        }

    if( ! logDir.isDirectory() ) {
        AppLog::error( "Non-directory lifeLog is in the way" );
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

    delete [] newFileName;


    strftime( fileName, 99, "%Y_%m%B_%d_%A_names.txt", timeStruct );
    
    newFile = logDir.getChildFile( fileName );
    
    newFileName = newFile->getFullFileName();
    
    nameLogFile = fopen( newFileName, "a" );
    
    delete newFile;

    if( nameLogFile == NULL ) {
        AppLog::errorF( "Failed to open log file %s", newFileName );
        }
    else {
        // only set these if BOTH opened successfully
        currentYear = timeStruct->tm_year;
        currentDay = timeStruct->tm_yday;
        }
    
    delete [] newFileName;
    }



void initLifeLog() {
    AppLog::info( "lifeLog starting up" );
    
    time_t t = time( NULL );
    struct tm *timeStruct = localtime( &t );
    
    currentYear = timeStruct->tm_year;
    currentDay = timeStruct->tm_yday;
    

    openCurrentLogFiles();
    }



void freeLifeLog() {
    if( logFile != NULL ) {
        fclose( logFile );
        }
    if( nameLogFile != NULL ) {
        fclose( nameLogFile );
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
        if( nameLogFile != NULL ) {
            fclose( nameLogFile );
            nameLogFile = NULL;
            }
        
        openCurrentLogFiles();
        }    
    }




void logBirth( int inPlayerID, char *inPlayerEmail,
               int inParentID, char *inParentEmail,
               char inIsMale,
               int inRace,
               int inMapX, int inMapY,
               int inTotalPopulation,
               int inParentChainLength,
               char inDonkeytown ) {
    
    cursesLogBirth( inPlayerEmail );
    
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
            
            char raceChar = (char)( inRace - 1  + 'A' );
            
            char statusChar = 'N';
            
            if( inDonkeytown ) {
                statusChar = 'D';
                }

            fprintf( logFile, 
                     "B %.f %d %s %c (%d,%d) %s pop=%d chain=%d race=%c "
                     "status=%c\n",
                     Time::timeSec(),
                     inPlayerID, inPlayerEmail, genderChar, inMapX, inMapY, 
                     parentString,
                     inTotalPopulation, inParentChainLength, raceChar,
                     statusChar );
            
            fflush( logFile );

            delete [] parentString;
            }
        }
    }



// killer email NULL if died of natural causes
void logDeath( int inPlayerID, char *inPlayerEmail,
               char inEve,
               char *inName,
               double inAge,
               int inSecPlayed,
               char inIsMale,
               int inMapX, int inMapY, 
               int inTotalRemainingPopulation,
               char inDisconnect, int inKillerID, 
               char *inKillerEmail ) {
    
    double yearsLived = inAge;
    if( inEve ) {
        yearsLived -= 14;
        }

    GridPos deathPos = { inMapX, inMapY };
    cursesLogDeath( inPlayerEmail, inName, yearsLived, deathPos );

    recordPlayerLifeStats( inPlayerEmail, inSecPlayed );
    
    if( inEve ) {
        
        GridPos deathPos = { inMapX, inMapY };
        
        mapEveDeath( inPlayerEmail, inAge, deathPos );
        
        if( inAge > 20 ) {
            resetEveRadius();
            deadYoungEveCount = 0;
            }
        else if( inAge < 16 ){
            deadYoungEveCount ++;
            
            if( deadYoungEveCount > 10 ) {
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
                // inDisconnect doesn't mean anything anymore
                // because no one ever dies from being disconnected
                // so ignore this signal

                if( inAge >= forceDeathAge ) {
                    causeString = stringDuplicate( "oldAge" );
                    }
                else {
                        
                    if( inKillerID == inPlayerID ) {
                        causeString = stringDuplicate( "suicide" );
                        }
                    else if( inKillerID == -999999998 ) {
                        causeString = 
                            stringDuplicate( "rocket_ride_interrupted" );
                        }
                    else if( inKillerID == -999999999 ) {
                        causeString = 
                            stringDuplicate( "rocket_ride_completed" );
                        }
                    else if( inKillerID < -1 ) {
                        
                        // use cleaned-up non-human object string
                        // as cause of death
                        
                        ObjectRecord *o = getObject( - inKillerID, true );
                        if( o != NULL ) {
                            causeString = 
                                stringDuplicate( o->description );
                            
                            // terminate at comment
                            char *commentStart = strstr( causeString, "#" );
                            if( commentStart != NULL ) {
                                commentStart[0] = '\0';
                                }
                            
                            char *nextSpace = strstr( causeString, " " );
                            while( nextSpace != NULL ) {
                                nextSpace[0] = '_';
                                nextSpace = strstr( nextSpace, " " );
                                }
                            }
                        else {
                            causeString = 
                                autoSprintf( "killed_by_unknown_object_%d",
                                             -inKillerID );
                            }
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
                     Time::timeSec(),
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





void logName( int inPlayerID, char *inEmail, char *inName,
              int inLineageEveID ) {
    if( nameLogFile != NULL ) {
        fprintf( nameLogFile, "%d %s\n", inPlayerID, inName );
        fflush( nameLogFile );
        }
    logPlayerNameForCurses( inEmail, inName, inLineageEveID );
    }

    

