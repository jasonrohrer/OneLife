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
                    "Saving a backup of map.db, mapTime.db, and biome.db ..." );
                
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
                File mapTimeFile( NULL, "mapTime.db" );
                File biomeFile( NULL, "biome.db" );
                

                File backupFolder( NULL, "backups" );
                
                if( ! backupFolder.exists() ) {
                    Directory::makeDirectory( &backupFolder );
                    }
                
                if( backupFolder.isDirectory() &&
                    mapFile.exists() &&
                    mapTimeFile.exists() &&
                    biomeFile.exists() ) {
                    
                    char *mapBackFileName =
                        autoSprintf( "map_%s.db", timeFileNamePart );
                    
                    File *mapBackFile = 
                        backupFolder.getChildFile( mapBackFileName );

                    mapFile.copy( mapBackFile );
                    
                    delete [] mapBackFileName;
                    delete mapBackFile;


                    char *mapTimeBackFileName =
                        autoSprintf( "mapTime_%s.db", timeFileNamePart );
                    
                    File *mapTimeBackFile = 
                        backupFolder.getChildFile( mapTimeBackFileName );

                    mapTimeFile.copy( mapTimeBackFile );
                    
                    delete [] mapTimeBackFileName;
                    delete mapTimeBackFile;

                    

                    char *biomeBackFileName =
                        autoSprintf( "biome_%s.db", timeFileNamePart );
                    
                    File *biomeBackFile = 
                        backupFolder.getChildFile( biomeBackFileName );

                    biomeFile.copy( biomeBackFile );
                    
                    delete [] biomeBackFileName;
                    delete biomeBackFile;

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

