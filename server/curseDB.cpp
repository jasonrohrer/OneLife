#include "curseDB.h"



#include "kissdb.h"
#include "lineardb3.h"
#include "dbCommon.h"


#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/io/file/File.h"



static LINEARDB3 db;
static char dbOpen = false;


static double lastSettingCheckTime = 0;
static double curseDuration = 48 * 3600;
static int curseBlockRadius = 50;

static double settingCheckInterval = 60;


static void checkSettings() {
    double curTime = Time::getCurrentTime();
    
    if( curTime - lastSettingCheckTime > settingCheckInterval ) {
        curseDuration = SettingsManager::getDoubleSetting( "curseBlockDuration",
                                                           48 * 3600.0 );
        curseBlockRadius = SettingsManager::getIntSetting( "curseBlockRadius",
                                                           50);
        
        lastSettingCheckTime = curTime;
        }
    }



// returns true if db left in an open state
static char cullStale() {
    LINEARDB3 tempDB;
    
    int error = LINEARDB3_open( &tempDB, 
                                "curses.db.temp", 
                                KISSDB_OPEN_MODE_RWCREAT,
                                10000,
                                80, 
                                8 );

    if( error ) {
        AppLog::errorF( "Error %d opening curses temp LinearDB3", error );
        return true;
        }
    

    LINEARDB3_Iterator dbi;
    
    
    LINEARDB3_Iterator_init( &db, &dbi );
    
    unsigned char key[80];
    
    unsigned char value[8];
    
    int total = 0;
    int stale = 0;
    int nonStale = 0;
    
    // first, just count
    while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 ) {
        total++;
        
        timeSec_t curseTime = valueToTime( value );

        timeSec_t elapsedTime = Time::timeSec() - curseTime;
        
        if( elapsedTime > curseDuration ) {
            stale ++;
            }
        else {
            nonStale ++;
            LINEARDB3_put( &tempDB, key, value );
            }
        }

    printf( "Culling curses.db found "
            "%d total entries, %d stale, %d non-stale\n",
            total, stale, nonStale );
    
    

    LINEARDB3_close( &tempDB );
    LINEARDB3_close( &db );

    
    File dbFile( NULL, "curses.db" );
    File dbTempFile( NULL, "curses.db.temp" );
    
    dbTempFile.copy( &dbFile );
    dbTempFile.remove();

    error = LINEARDB3_open( &db, 
                            "curses.db", 
                            KISSDB_OPEN_MODE_RWCREAT,
                            10000,
                            80, 
                            8 );

    if( error ) {
        AppLog::errorF( "Error %d re-opening curses LinearDB3", error );
        return false;
        }

    return true;
    }




void initCurseDB() {
    int error = LINEARDB3_open( &db, 
                                "curses.db", 
                                KISSDB_OPEN_MODE_RWCREAT,
                                10000,
                                // sender email (truncated to 40 chars max)
                                // with receiver email (trucated to 40) 
                                // appended.
                                // append spaces to the end if needed 
                                80, 
                                // one 64-bit double, representing the time
                                // the cursed was placed
                                // in whatever binary format and byte order
                                // "double" on the server platform uses
                                8 );
    
    if( error ) {
        AppLog::errorF( "Error %d opening curses LinearDB3", error );
        return;
        }
    
    checkSettings();


    dbOpen = cullStale();
    }




void freeCurseDB() {
    if( dbOpen ) {
        LINEARDB3_close( &db );
        dbOpen = false;
        }
    }





static void getKey( const char *inSenderEmail, const char *inReceiverEmail, 
                    unsigned char *outKey ) {
    memset( outKey, ' ', 80 );

    sprintf( (char*)outKey, "%.40s%.40s", inSenderEmail, inReceiverEmail );
    }


                    

void setDBCurse( const char *inSenderEmail, const char *inReceiverEmail ) {
    checkSettings();

    unsigned char key[80];
    unsigned char value[8];
    

    getKey( inSenderEmail, inReceiverEmail, key );
    
    
    timeToValue( Time::timeSec(), value );

    
    LINEARDB3_put( &db, key, value );
    printf( "Setting personal curse for %s by %s\n", 
            inReceiverEmail, inSenderEmail );
    }




char isCursed( const char *inSenderEmail, const char *inReceiverEmail ) {
    unsigned char key[80];
    unsigned char value[8];

    getKey( inSenderEmail, inReceiverEmail, key );
    
    int result = LINEARDB3_get( &db, key, value );

    if( result == 0 ) {
        timeSec_t curseTime = valueToTime( value );

        timeSec_t elapsedTime = Time::timeSec() - curseTime;
        
        if( elapsedTime > curseDuration ) {
            return false;
            }
        return true;
        }

    return false;
    }







typedef struct PersonRecord {
        const char *email;
        GridPos pos;
        // -1 if unknown
        int blocking;
    } PersonRecord;

    

SimpleVector<PersonRecord> blockingRecords;

void initPersonalCurseTest() {
    checkSettings();
    
    blockingRecords.deleteAll();
    }

    

void addPersonToPersonalCurseTest( const char *inEmail,
                                   GridPos inPos ) {
    PersonRecord r = { inEmail, inPos, -1 };
    blockingRecords.push_back( r );
    }



char isBirthLocationCurseBlocked( const char *inTargetEmail, GridPos inPos ) {
    for( int i=0; i<blockingRecords.size(); i++ ) {
        PersonRecord *r = blockingRecords.getElement( i );
        
        if( r->blocking != 0 && 
            distance( inPos, r->pos ) <= curseBlockRadius ) {
            
            // in radius, and not known to be non-blocking
            
            if( r->blocking == -1 ) {
                // look it up and remember it
                r->blocking = isCursed( r->email, inTargetEmail );
                }
            
            return ( r->blocking == 1 );
            }
        }
    return false;
    }





