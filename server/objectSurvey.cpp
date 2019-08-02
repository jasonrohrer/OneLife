#include "minorGems/system/Time.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/io/file/File.h"
#include "minorGems/util/log/AppLog.h"

#include "../gameSource/objectBank.h"

#include "map.h"

#include <stdlib.h>

static double lastCheckTime = 0;

void initObjectSurvey() {
    lastCheckTime = Time::getCurrentTime();
    }



void freeObjectSurvey() {
    }


static char surveyRunning = false;

static SimpleVector<GridPos> playerPosToCheck;

static int nextPlayerPosToCheck = 0;


static SimpleVector<GridPos> finalPlayerPos;

static int nextFinalPosToAdd = 0;


static SimpleVector<GridPos> mapPosToCheck;


static int mapPosBatch = 100;


static int playerBoxRadius = 10;


typedef struct SurveyRecord {
        int id;
        int count;
    } SurveyRecord;


static SimpleVector<SurveyRecord> objectSurveyRecords;


static SurveyRecord *findRecord( int inID ) {
    for( int i=0; i<objectSurveyRecords.size(); i++ ) {
        
        SurveyRecord *r = objectSurveyRecords.getElement( i );
        
        if( r->id == inID ) {
            return r;
            }
        }
    return NULL;
    }

    


void stepObjectSurvey() {
    if( surveyRunning ) {
        
        if( nextPlayerPosToCheck < playerPosToCheck.size() ) {
            // run one step of checking player pos against final list
            // on 200 player server, this will take 200 server steps
            // (spread work out)
            GridPos pos = 
                playerPosToCheck.getElementDirect( nextPlayerPosToCheck );
            
            nextPlayerPosToCheck ++;
            
            char tooClose = false;
            
            for( int p=0; p<finalPlayerPos.size(); p++ ) {
                GridPos thisPos = finalPlayerPos.getElementDirect( p );
                
                if( abs( pos.x - thisPos.x ) <= playerBoxRadius &&
                    abs( pos.y - thisPos.y ) <= playerBoxRadius ) {
                    
                    tooClose = true;
                    break;
                    }
                }

            if( ! tooClose ) {
                finalPlayerPos.push_back( pos );
                }
            
            if( nextPlayerPosToCheck == playerPosToCheck.size() ) {
                AppLog::infoF( 
                    "Final object survey list of %d player positions ready",
                    finalPlayerPos.size() );
                }
            
            return;
            }

        // else final list ready

        
        
        if( nextFinalPosToAdd < finalPlayerPos.size() ) {
            // add all map pos in box radius around player
            GridPos pos = 
                finalPlayerPos.getElementDirect( nextFinalPosToAdd );
            
            nextFinalPosToAdd ++;

            for( int y=-playerBoxRadius; y<=playerBoxRadius; y++ ) {
                for( int x=-playerBoxRadius; x<=playerBoxRadius; x++ ) {
                    GridPos boxPos = pos;
                    boxPos.x += x;
                    boxPos.y += y;
                    
                    mapPosToCheck.push_back( boxPos );
                    }
                }
            
            if( nextFinalPosToAdd == finalPlayerPos.size() ) {
                AppLog::infoF( 
                    "Final object survey list of %d map positions ready",
                    mapPosToCheck.size() );
                }
            
            
            return;
            }

        
        int numMapPosLeft = mapPosToCheck.size();
        if( numMapPosLeft > 0 ) {
            

            int thisBatchSize = mapPosBatch;
            
            if( thisBatchSize > numMapPosLeft ) {
                thisBatchSize = numMapPosLeft;
                }
            
            for( int i=0; i<thisBatchSize; i++ ) {
                
                GridPos pos = 
                    mapPosToCheck.getElementDirect( numMapPosLeft - 1 );

                mapPosToCheck.deleteElement( numMapPosLeft - 1 );

                numMapPosLeft--;
                

                int id = getMapObject( pos.x, pos.y );
                

                if( id > 0 ) {    
                    SurveyRecord *r = findRecord( id );
                    if( r != NULL ) {
                        r->count ++;
                        }
                    else {
                        SurveyRecord newRec = { id, 1 };
                        objectSurveyRecords.push_back( newRec );
                        }
                    }
                }
            return;
            }


        // else totally done, print report
        
        surveyRunning = false;
        

        AppLog::infoF( 
            "Saving object survey report for %d unique objects",
            objectSurveyRecords.size() );

        
        File logDir( NULL, "objectSurveys" );
    
        if( ! logDir.exists() ) {
            Directory::makeDirectory( &logDir );
            }

        if( ! logDir.isDirectory() ) {
            AppLog::error( "Non-directory objectSurveys is in the way" );
            return;
            }

        int nextSurveyID = 1;
        
        int numFiles = 0;
        File **childFiles = logDir.getChildFiles( &numFiles );
        
        if( numFiles > 0 ) {
            for( int i=0; i<numFiles; i++ ) {
                char *name = childFiles[i]->getFileName();
                
                int thisNum = 0;
                sscanf( name, "survey%d.txt", &thisNum );
                
                if( thisNum >= nextSurveyID ) {
                    nextSurveyID = thisNum + 1;
                    }

                delete [] name;
                delete childFiles[i];
                }
            }
        delete [] childFiles;
        
        char *thisFileName = autoSprintf( "survey%d.txt", nextSurveyID );
        
        File *thisFile = logDir.getChildFile( thisFileName );
        
        char *thisFilePath = thisFile->getFullFileName();
        
        delete [] thisFileName;
        delete thisFile;
        
        // easy enough to sort with other tools externally
        // just print the counts and IDs and names

        FILE *f = fopen( thisFilePath, "w" );

        AppLog::infoF( 
            "Saving object survey report into file %s", thisFilePath );

        
        if( f != NULL ) {
            
            for( int i=0; i<objectSurveyRecords.size(); i++ ) {
                SurveyRecord *r = objectSurveyRecords.getElement( i );
                
                fprintf( f, "%d [%d] %s\n",
                         r->count, r->id, getObject( r->id )->description );
                }
            fclose( f );
            }
        else {
            AppLog::errorF( "Failed to open %s for writing", thisFilePath );
            }
        delete [] thisFilePath;
        
        }
    }





char shouldRunObjectSurvey() {

    double curTime = Time::getCurrentTime();
    
    if( curTime > lastCheckTime + 10 ) {
        lastCheckTime = curTime;
        
        char run = false;
        if( SettingsManager::getIntSetting( "runObjectSurveyNow", 0 ) ) {
            run = true;

            SettingsManager::setSetting( "runObjectSurveyNow", 0 );
            }
        return run;
        }
    return false;
    }



void startObjectSurvey( SimpleVector<GridPos> *inLivingPlayerPositions ) {
    AppLog::infoF( "Starting object survey around %d players",
                   inLivingPlayerPositions->size() );
    
    surveyRunning = true;
    
    playerPosToCheck.deleteAll();
    finalPlayerPos.deleteAll();
    mapPosToCheck.deleteAll();

    objectSurveyRecords.deleteAll();
    
    playerPosToCheck.push_back_other( inLivingPlayerPositions );
    nextPlayerPosToCheck = 0;
    nextFinalPosToAdd = 0;
    }

