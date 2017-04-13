#include "backup.h"

#include <time.h>


#include "minorGems/util/SettingsManager.h"
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/Directory.h"

#include "minorGems/util/log/AppLog.h"


static time_t lastBackupTime = 0;
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
                   ( time( NULL ) - lastBackupTime ) / 3600.0 );
    }

    


// makes a new backup if needed
// also handles deleting old backups
void checkBackup() {
    time_t curTime = time( NULL );
    
    if( curTime - lastBackupTime > 12 * 3600 
        ||
        curTime < lastBackupTime ) {
        
        // more than 12 hours have passed since last backup, or
        // last time not valid
        

        struct tm timeStruct;
        struct tm *gmTM = gmtime( &curTime );
        
        // other calls overwrite it
        memcpy( &timeStruct, gmTM, sizeof( timeStruct ) );
        
    
    
        if( timeStruct.tm_hour == targetHour ) {

            if( SettingsManager::getIntSetting( "saveBackups", 0 ) ) {
                
                AppLog::info( "Saving a backup of map.db and biome.db ..." );
                
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
                
                File mapFile( NULL, "map.db" );
                File biomeFile( NULL, "biome.db" );
                

                File backupFolder( NULL, "backups" );
                
                if( ! backupFolder.exists() ) {
                    Directory::makeDirectory( &backupFolder );
                    }
                
                if( backupFolder.isDirectory() &&
                    mapFile.exists() &&
                    biomeFile.exists() ) {
                    
                    char *mapBackFileName =
                        autoSprintf( "map_%s.db", timeFileNamePart );
                    
                    File *mapBackFile = 
                        backupFolder.getChildFile( mapBackFileName );

                    mapFile.copy( mapBackFile );
                    
                    delete [] mapBackFileName;
                    delete mapBackFile;
                    

                    char *biomeBackFileName =
                        autoSprintf( "biome_%s.db", timeFileNamePart );
                    
                    File *biomeBackFile = 
                        backupFolder.getChildFile( biomeBackFileName );

                    biomeFile.copy( biomeBackFile );
                    
                    delete [] biomeBackFileName;
                    delete biomeBackFile;

                    AppLog::info( "...Done saving backups" );
                    backupsSaved = true;
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

