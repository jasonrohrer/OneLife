#include "backup.h"



#include "minorGems/util/SettingsManager.h"
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/Directory.h"
#include "minorGems/system/Time.h"

#include "minorGems/util/log/AppLog.h"


static timeSec_t lastBackupTime = 0;
static int targetHour = 8;

void initBackup() {
    lastBackupTime = SettingsManager::getTimeSetting( "lastBackupTimeUTC",
                                                      lastBackupTime );

    targetHour = SettingsManager::getIntSetting( "backupHourUTC",
                                                 targetHour );
    

    time_t t = time( NULL );
    

    struct tm timeStruct;
    struct tm *gmTM = gmtime( &t );
    
    // other calls overwrite it
    memcpy( &timeStruct, gmTM, sizeof( timeStruct ) );
    
    AppLog::infoF( "Backup system inited.  Backups on = %d, "
                   "target hour UTC = %d, current hour UTC = %d, "
                   "last backup was %f hours ago",
                   SettingsManager::getIntSetting( "saveBackups", 0 ),
                   targetHour,
                   timeStruct.tm_hour,
                   ( Time::timeSec() - lastBackupTime ) / 3600.0 );
    }


// call with "map" or "eve" for example
void backupDBFile( const char *inFileNamePrefix, char *inTimeFileNamePart,
                   File *inBackupFolder ) {
    char *fileName = autoSprintf( "%s.db", inFileNamePrefix );
    
    File dbFile( NULL, fileName );

    delete [] fileName;

    if( dbFile.exists() ) {
        

        char *backFileName =
            autoSprintf( "%s_%s.db", inFileNamePrefix, inTimeFileNamePart );
        
        File *backFile = 
            inBackupFolder->getChildFile( backFileName );
        
        dbFile.copy( backFile );
        
        delete [] backFileName;
        delete backFile;
        }
    }

    


// makes a new backup if needed
// also handles deleting old backups
void checkBackup() {
    timeSec_t curTime = Time::timeSec();
    
    if( curTime - lastBackupTime > 12 * 3600 
        ||
        curTime < lastBackupTime ) {
        
        // more than 12 hours have passed since last backup, or
        // last time not valid
        

        time_t curTimeT = time( NULL );
        
        struct tm timeStruct;
        struct tm *gmTM = gmtime( &curTimeT );
        
        // other calls overwrite it
        memcpy( &timeStruct, gmTM, sizeof( timeStruct ) );
        
    
    
        if( timeStruct.tm_hour == targetHour ) {

            if( SettingsManager::getIntSetting( "saveBackups", 0 ) ) {
                
                AppLog::info( 
                    "Saving a backup of map.db, mapTime.db, and biome.db "
                    "floor.db, floorTime.db, playerStats.db, and eve.db ..." );
                
                char backupsSaved = false;
                
                // save a backup now

                char *timeFileNamePart = 
                    autoSprintf( "%d_%02d_%02d__%02d_%02d_%02d",
                                 timeStruct.tm_year + 1900,
                                 timeStruct.tm_mon + 1,
                                 timeStruct.tm_mday,
                                 timeStruct.tm_hour,
                                 timeStruct.tm_min,
                                 timeStruct.tm_sec );

                File backupFolder( NULL, "backups" );
                
                if( ! backupFolder.exists() ) {
                    Directory::makeDirectory( &backupFolder );
                    }
                
                if( backupFolder.isDirectory() ) {
                    
                    backupDBFile( "lookTime", timeFileNamePart, &backupFolder );

                    backupDBFile( "map", timeFileNamePart, &backupFolder );
                    
                    backupDBFile( "mapTime", timeFileNamePart, &backupFolder );
                    
                    backupDBFile( "biome", timeFileNamePart, &backupFolder );
                    
                    backupDBFile( "eve", timeFileNamePart, &backupFolder );
                    
                    backupDBFile( "floor", timeFileNamePart, &backupFolder );
                    
                    backupDBFile( "floorTime", timeFileNamePart, 
                                  &backupFolder );
                    
                    backupDBFile( "playerStats", 
                                  timeFileNamePart, &backupFolder );


                    AppLog::info( "...Done saving backups" );
                    backupsSaved = true;

                    
                    int keepDays = 
                        SettingsManager::getIntSetting( "keepBackupsDays",
                                                        14 );


                    AppLog::infoF( "Checking for stale backup files "
                                   "(older than %d days)", keepDays );
                    
                    int keepSeconds = keepDays * 24 * 3600;

                    int numChildren;
                    File **childFiles = 
                        backupFolder.getChildFiles( &numChildren );
                    
                    int numRemoved = 0;
                    
                    for( int i=0; i<numChildren; i++ ) {
                        if( curTime - 
                            childFiles[i]->getModificationTime()
                            > keepSeconds ) {
                            
                            char removed = childFiles[i]->remove();

                            char *fileName = childFiles[i]->getFileName();
                            
                            if( removed ) {
                                AppLog::infoF( "Removed %s", fileName );
                                numRemoved++;
                                }
                            else {
                                AppLog::errorF( "Failed to remove %s",
                                                fileName );
                                }
                            delete [] fileName;
                            }
                        delete childFiles[i];
                        }
                    
                    delete [] childFiles;
                    
                    AppLog::infoF( "Done checking, removed %d stale backups", 
                                   numRemoved );
                    }
                
                if( !backupsSaved ) {
                    AppLog::error( "...Failed to save backups" );
                    }
                
                
                delete [] timeFileNamePart;
                

                lastBackupTime = curTime;
                SettingsManager::setSetting( "lastBackupTimeUTC",
                                             lastBackupTime );

                // check if target hour has changed
                targetHour = SettingsManager::getIntSetting( "backupHourUTC",
                                                             targetHour );
                }
            else {
                // push ahead, so we don't keep cheking the saveBackups setting
                // repeatedly
                lastBackupTime = curTime;
                
                AppLog::info( "Backups are still off" );
                }
            }
        }
    
    }

