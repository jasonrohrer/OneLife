#include "curseDB.h"



#include "kissdb.h"
#include "lineardb3.h"
#include "dbCommon.h"


#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/io/file/File.h"



static LINEARDB3 db;
static char dbOpen = false;


static LINEARDB3 dbCount;
static char dbCountOpen = false;


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




// returns true if db left in an open state
static char cullStaleCount() {
    LINEARDB3 tempDB;
    
    int error = LINEARDB3_open( &tempDB, 
                                "curseCount.db.temp", 
                                KISSDB_OPEN_MODE_RWCREAT,
                                10000,
                                40, 
                                12 );

    if( error ) {
        AppLog::errorF( "Error %d opening curseCount temp LinearDB3", error );
        return true;
        }
    

    LINEARDB3_Iterator dbi;
    
    
    LINEARDB3_Iterator_init( &dbCount, &dbi );
    
    unsigned char key[40];
    
    unsigned char value[12];
    
    int total = 0;
    int stale = 0;
    int nonStale = 0;
    
    // first, just count
    while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 ) {
        total++;
        
        int count = valueToInt( value );

        timeSec_t curseTime = valueToTime( &( value[4] ) );

        timeSec_t elapsedTime = Time::timeSec() - curseTime;
        
        if( elapsedTime > curseDuration * count ) {
            // completely decremented to 0 due to elapsed time
            stale ++;
            }
        else {
            nonStale ++;
            LINEARDB3_put( &tempDB, key, value );
            }
        }

    printf( "Culling curseCount.db found "
            "%d total entries, %d stale, %d non-stale\n",
            total, stale, nonStale );
    
    

    LINEARDB3_close( &tempDB );
    LINEARDB3_close( &dbCount );

    
    File dbFile( NULL, "curseCount.db" );
    File dbTempFile( NULL, "curseCount.db.temp" );
    
    dbTempFile.copy( &dbFile );
    dbTempFile.remove();

    error = LINEARDB3_open( &dbCount, 
                            "curseCount.db", 
                            KISSDB_OPEN_MODE_RWCREAT,
                            10000,
                            40, 
                            12 );

    if( error ) {
        AppLog::errorF( "Error %d re-opening curseCount LinearDB3", error );
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
                                // with receiver email (trucated to 39) 
                                // appended, terminated by a NULL character
                                // append spaces to the end if needed
                                // (after the NULL character) to fill
                                // the full 80 characters consistently
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



    dbCountOpen = false;
    
    error = LINEARDB3_open( &dbCount, 
                            "curseCount.db", 
                            KISSDB_OPEN_MODE_RWCREAT,
                            10000,
                            // receiver email (truncated to 39 chars max)
                            // followed by a NULL character
                            // append spaces to the end if needed, after
                            // the NULL character, to fill the whole 40
                            40,
                            // one 32-bit int, representing the count
                            //
                            // followed by
                            //
                            // one 64-bit double, representing the time
                            // the count was last incremented
                            // in whatever binary format and byte order
                            // "double" on the server platform uses
                            12 );
    
    if( error ) {
        AppLog::errorF( "Error %d opening curseCount LinearDB3", error );
        return;
        }
    
    dbCountOpen = cullStaleCount();
    }




void freeCurseDB() {
    if( dbOpen ) {
        LINEARDB3_close( &db );
        dbOpen = false;
        }
    if( dbCountOpen ) {
        LINEARDB3_close( &dbCount );
        dbCountOpen = false;
        }
    }





static void getKey( const char *inSenderEmail, const char *inReceiverEmail, 
                    unsigned char *outKey ) {
    memset( outKey, ' ', 80 );

    sprintf( (char*)outKey, "%.40s%.39s", inSenderEmail, inReceiverEmail );
    }



static void getCountKey( const char *inReceiverEmail, 
                    unsigned char *outKey ) {
    memset( outKey, ' ', 40 );

    sprintf( (char*)outKey, "%.39s", inReceiverEmail );
    }





int getCurseCount( const char *inReceiverEmail ) {
    unsigned char key[40];
    unsigned char value[12];

    
    getCountKey( inReceiverEmail, key );


    int result = LINEARDB3_get( &dbCount, key, value );

    if( result == 0 ) {

        int count = valueToInt( value );

        timeSec_t curseTime = valueToTime( &( value[4] ) );

        timeSec_t elapsedTime = Time::timeSec() - curseTime;
        
        if( elapsedTime > curseDuration ) {
            // decrement by however many multiples

            int decr = lrint( elapsedTime ) / lrint( curseDuration );

            count -= decr;
            
            if( count < 0 ) {
                count = 0;
                }
            }
        
        return count;
        }
    else {
        return 0;
        }
    }




void incrementCurseCount( const char *inReceiverEmail ) {
    unsigned char key[40];
    unsigned char value[12];

    int oldCount = getCurseCount( inReceiverEmail );
    
    int newCount = oldCount + 1;

    intToValue( newCount, value );

    // reset time of increment to current time
    timeToValue( Time::timeSec(), &( value[4] ) );

    
    getCountKey( inReceiverEmail, key );
    
    
    LINEARDB3_put( &dbCount, key, value );
    }






                    

void setDBCurse( const char *inSenderEmail, const char *inReceiverEmail ) {
    checkSettings();

    char alreadyCursedByThisPerson = isCursed( inSenderEmail, inReceiverEmail );
    

    unsigned char key[80];
    unsigned char value[8];
    

    getKey( inSenderEmail, inReceiverEmail, key );
    
    
    timeToValue( Time::timeSec(), value );

    
    LINEARDB3_put( &db, key, value );
    printf( "Setting personal curse for %s by %s\n", 
            inReceiverEmail, inSenderEmail );


    if( ! alreadyCursedByThisPerson ) {
        incrementCurseCount( inReceiverEmail );
        }
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

int personalLiveCurseCount = 0;

void initPersonalCurseTest( const char *inTargetEmail ) {
    checkSettings();
    
    blockingRecords.deleteAll();

    personalLiveCurseCount = 0;
    }

    

void addPersonToPersonalCurseTest( const char *inEmail,
                                   const char *inTargetEmail,
                                   GridPos inPos ) {
    
    int blocking = isCursed( inEmail, inTargetEmail );

    PersonRecord r = { inEmail, inPos, blocking };
    blockingRecords.push_back( r );

    if( blocking ) {
        personalLiveCurseCount ++;
        }
    }


static int getCurseRadius( int inLiveCurseCount ) {
    // 0 if no one live is blocking
    return curseBlockRadius * inLiveCurseCount;
    }




char isBirthLocationCurseBlocked( const char *inTargetEmail, GridPos inPos ) {
    
    int radius = getCurseRadius( personalLiveCurseCount );
    
    for( int i=0; i<blockingRecords.size(); i++ ) {
        PersonRecord *r = blockingRecords.getElement( i );
        
        if( r->blocking == 1 && 
            distance( inPos, r->pos ) <= radius ) {
            
            // in radius, and blocking
            return true;
            }
        }
    return false;
    }



char isBirthLocationCurseBlockedNoCache( const char *inTargetEmail, 
                                         GridPos inPos ) {

    int liveCurseCount = 0;

    SimpleVector<char> curseCache;
    
    for( int i=0; i<blockingRecords.size(); i++ ) {
        PersonRecord *r = blockingRecords.getElement( i );
        
        if( isCursed( r->email, inTargetEmail ) ) {
            liveCurseCount ++;
            curseCache.push_back( true );
            }
        else {
            curseCache.push_back( false );
            }
        }
    
    
    
    int radius = getCurseRadius( liveCurseCount );
    
    for( int i=0; i<blockingRecords.size(); i++ ) {
        PersonRecord *r = blockingRecords.getElement( i );
        
        if( curseCache.getElementDirect( i ) ) {
            // cursed
            
            if( distance( inPos, r->pos ) <= radius ) {
                // in radius
                return true;
                }
            }
        }
    return false;
    }




