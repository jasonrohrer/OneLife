#include "curseDB.h"



#include "kissdb.h"
#include "lineardb3.h"
#include "dbCommon.h"

#include "curseLog.h"

#include "trustDB.h"


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

static double curseBlockOfflineFactor = 1.0;
static double curseBlockOfflineExponent = 2.0;

static double settingCheckInterval = 60;


// list of emails that have forgiven everyone
// we clear all discovered curses by these people during our 
// incremental cull operation
static SimpleVector<char*> everyoneForgiverList;
static SimpleVector<char*> nextEveryoneForgiverList;


static char isInEveryoneForgiverList( char *inEmail ) {
    for( int i=0; i<everyoneForgiverList.size(); i++ ) {
        char *email = everyoneForgiverList.getElementDirect( i );
        
        if( strcmp( email, inEmail ) == 0 ) {
            return true;
            }
        }
    return false;
    }





static void checkSettings() {
    double curTime = Time::getCurrentTime();
    
    if( curTime - lastSettingCheckTime > settingCheckInterval ) {
        curseDuration = SettingsManager::getDoubleSetting( "curseBlockDuration",
                                                           48 * 3600.0 );
        curseBlockRadius = SettingsManager::getIntSetting( "curseBlockRadius",
                                                           50);
        
        curseBlockOfflineFactor = 
            SettingsManager::getDoubleSetting( "curseBlockOfflineFactor", 
                                               1.0 );
        
        curseBlockOfflineExponent = 
            SettingsManager::getDoubleSetting( "curseBlockOfflineExponent", 
                                               2.0 );
        
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

        // look for those that have been previously marked as stale
        // with a 0 time
        // don't do time calculation for non-marked records here,
        // because we're not decrementing curseCount as we do this
        if( curseTime == 0 ) {
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
    
    char forceStale = false;
    
    if( SettingsManager::getIntSetting( "clearCurseCountsOnStartup", 0 ) ) {
        printf( "Culling curseCount.db clearCurseCountsOnStartup setting, "
            "treating all records as stale.\n" );
        forceStale = true;
        }
    
    while( LINEARDB3_Iterator_next( &dbi, key, value ) > 0 ) {
        total++;
        
        timeSec_t curseTime = valueToTime( &( value[4] ) );

        timeSec_t elapsedTime = Time::timeSec() - curseTime;
        
        if( forceStale || elapsedTime > curseDuration ) {
            // completely decremented to 0 due to elapsed time
            // the newest curse record for this person is stale
            // that means all of them are stale
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
                                // sender email (truncated to 39 chars max)
                                // with receiver email (trucated to 39) 
                                // with comma separating them
                                // terminated by a NULL character
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
                            // the count was last incremented or decremented
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

    everyoneForgiverList.deallocateStringElements();
    nextEveryoneForgiverList.deallocateStringElements();
    }





static void getKey( const char *inSenderEmail, const char *inReceiverEmail, 
                    unsigned char *outKey ) {
    memset( outKey, ' ', 80 );

    sprintf( (char*)outKey, "%.39s,%.39s", inSenderEmail, inReceiverEmail );
    }



// inIndex = 0 for sender, 1 for receiver
// result NOT destroyed by caller (pointer to internal buffer)

char senderBuffer[40];
char receiverBuffer[40];

static char *getEmailFromKey( unsigned char *inKey, int inIndex ) {
    
    sscanf( (char*)inKey, "%39[^,],%39s", senderBuffer, receiverBuffer );
    
    if( inIndex == 0 ) {
        return senderBuffer;
        }
    else {
        return receiverBuffer;
        }
    }



// old key has no , between addresses
// don't write new curses in this format
// but check for existing curses using this format
static void getOldKey( const char *inSenderEmail, const char *inReceiverEmail, 
                       unsigned char *outKey ) {
    memset( outKey, ' ', 80 );

    sprintf( (char*)outKey, "%.40s%.39s", inSenderEmail, inReceiverEmail );
    }


// set to false later, after no non-space keys remain in DB
static char considerOldKey = false;



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
            // our most recent curse is expired
            // that means that all of our older curses are expired too!

            // that means we have no active curses at all.
            
            return 0;
            }
        
        // if our most recent curse is not expired, this might be an 
        // over-estimate, because some of our older curses might
        // be expired.

        // however, if we're actively acquiring new curses, over-estimating
        // is fine.

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



void decrementCurseCount( const char *inReceiverEmail ) {
    
    int oldCount = getCurseCount( inReceiverEmail );
    
    if( oldCount > 0 ) {
        unsigned char key[40];
        unsigned char value[12];

    
        getCountKey( inReceiverEmail, key );
        
        
        int result = LINEARDB3_get( &dbCount, key, value );
        
        if( result == 0 ) {
            
            int newCount = oldCount - 1;
            
            intToValue( newCount, value );
        
            // keep time in value[4] the same
            // don't adjust it
        
            // by decrementing, we're saying that an older record has expired
            // when our newest record expires (based on time stored in value[4])
            // that means ALL of our records have expired.

            LINEARDB3_put( &dbCount, key, value );
            }
        }
    }





static char cullingIteratorSet = false;

static LINEARDB3_Iterator cullingIterator;

static int numCullsPerStep = 10;

static int numRecordsSeenByIterator = 0;
static int numRecordsMarked = 0;





static void stepStaleCurseCulling() {
    if( !cullingIteratorSet ) {
        LINEARDB3_Iterator_init( &db, &cullingIterator );
        cullingIteratorSet = true;
        numRecordsSeenByIterator = 0;
        }

    unsigned char key[80];
    
    unsigned char value[8];
    
    timeSec_t curTimeSec = Time::timeSec();
    
    for( int i=0; i<numCullsPerStep; i++ ) {        
        int result = 
            LINEARDB3_Iterator_next( &cullingIterator, key, value );

        if( result <= 0 ) {
            // restart the iterator back at the beginning
            LINEARDB3_Iterator_init( &db, &cullingIterator );
            if( numRecordsSeenByIterator != 0 ) {
                AppLog::infoF( 
                    "Curse stale cull iterated through %d curse db entries,"
                    " marked %d as stale (or forgiven).",
                    numRecordsSeenByIterator,
                    numRecordsMarked );
                }
            numRecordsSeenByIterator = 0;
            numRecordsMarked = 0;
            
            // we're done iterating
            // which means that we're done with this list
            everyoneForgiverList.deallocateStringElements();
            
            // nextEveryoneForgiverList contains new emails
            // that forgave everyone DURING our previous iteration
            // Now that we're done iterating and starting over, we can move
            // those pending emails into our main list
            everyoneForgiverList.push_back_other( &nextEveryoneForgiverList );
            nextEveryoneForgiverList.deleteAll();
            

            // break loop when we reach end, so we don't busy-cycle
            // in a very short list repeatedly in one step
            break;
            }
        else {
            numRecordsSeenByIterator ++;
            }

        timeSec_t curseTime = valueToTime( value );

        char forgiven = false;


        if( everyoneForgiverList.size() > 0 ) {
            // have emails that are in the process of forgiving everyone
            
            char *senderEmail = getEmailFromKey( key, 0 );
            
            if( isInEveryoneForgiverList( senderEmail ) ) {
                // they have forgiven everyone
                forgiven = true;
                }
            }
        

        
        if( curseTime != 0 &&
            ( forgiven || 
              curTimeSec - curseTime > curseDuration ) ) {
            
            // non-marked, but stale or to be forgiven
            

            // is this our newer-style key, with both emails
            // separated by comma?
            
            char *commaPos = strstr( (char*)key, "," );
            
            if( commaPos != NULL ) {
                // if so, we can strip out receiver email 
                // and decrement their count
                
                numRecordsMarked++;
                
                char *receiverEmail = &( commaPos[1] );
                
                char *senderEmail = getEmailFromKey( key, 0 );

                if( forgiven ) {    
                    logForgiveAllEffect( senderEmail, receiverEmail );
                    }
                else {
                    logCurseExpire( senderEmail, receiverEmail );
                    }
                

                decrementCurseCount( receiverEmail );
                
          
                logCurseScore( receiverEmail, getCurseCount( receiverEmail ) );


                // mark this curse so we don't decrement again in future
                timeToValue( 0, value );
                LINEARDB3_put( &db, key, value );
                }
            
            // otherwise, it's an old-style key.
            // we must wait until we check for this email pair
            // in a curse check to decrement it, so leave this record alone
            // as unmarked and still with the stale time
            }
        }
    }





                    

void setDBCurse( int inSenderID, 
                 const char *inSenderEmail, const char *inReceiverEmail ) {
    // a cursed person is no longer trusted
    // cursing someone is how to clear trust
    clearDBTrust( inSenderEmail, inReceiverEmail );
    
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

    
    logCurse( inSenderID, (char*)inSenderEmail, (char*)inReceiverEmail );

    logCurseScore( (char*)inReceiverEmail, getCurseCount( inReceiverEmail ) );
    }




void clearDBCurse( int inSenderID,
                   const char *inSenderEmail, const char *inReceiverEmail ) {
    
    unsigned char key[80];
    unsigned char value[8];

    getKey( inSenderEmail, inReceiverEmail, key );
    
    int result = LINEARDB3_get( &db, key, value );


    if( considerOldKey && result == 1 ) {
        getOldKey( inSenderEmail, inReceiverEmail, key );
        
        result = LINEARDB3_get( &db, key, value );
        }
    

    if( result == 0 ) {
        timeSec_t curseTime = valueToTime( value );

        if( curseTime > 0 ) {
            
            decrementCurseCount( inReceiverEmail );
                
            // mark it so we don't decrement again in future                
            timeToValue( 0, value );
            LINEARDB3_put( &db, key, value );
            }
        }


    logForgive( inSenderID, (char*)inSenderEmail, (char*)inReceiverEmail );

    logCurseScore( (char*)inReceiverEmail, getCurseCount( inReceiverEmail ) );
    }



void clearAllDBCurse( int inSenderID, const char *inSenderEmail ) {
    nextEveryoneForgiverList.push_back( stringDuplicate( inSenderEmail ) );
    
    logForgiveAll( inSenderID, (char*)inSenderEmail );
    }





char isCursed( const char *inSenderEmail, const char *inReceiverEmail ) {
    unsigned char key[80];
    unsigned char value[8];

    getKey( inSenderEmail, inReceiverEmail, key );
    
    int result = LINEARDB3_get( &db, key, value );


    if( considerOldKey && result == 1 ) {
        getOldKey( inSenderEmail, inReceiverEmail, key );
        
        result = LINEARDB3_get( &db, key, value );
        }
    

    if( result == 0 ) {
        timeSec_t curseTime = valueToTime( value );

        if( curseTime > 0 ) {

            timeSec_t elapsedTime = Time::timeSec() - curseTime;
        
            if( elapsedTime > curseDuration ) {
                // curse just expired now

                decrementCurseCount( inReceiverEmail );
                
                // mark it so we don't decrement again in future
                
                timeToValue( 0, value );
                LINEARDB3_put( &db, key, value );
                
                logCurseExpire( (char*)inSenderEmail,
                                (char*)inReceiverEmail );

                logCurseScore( (char*)inReceiverEmail, 
                               getCurseCount( inReceiverEmail ) );

                return false;
                }
            }
        else {
            // 0 means curse expired before
            // and was marked as such
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

int personalTotalCurseCount = 0;

void initPersonalCurseTest( const char *inTargetEmail ) {
    checkSettings();
    
    stepStaleCurseCulling();

    blockingRecords.deleteAll();

    personalLiveCurseCount = 0;

    personalTotalCurseCount = getCurseCount( inTargetEmail );
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


static int getCurseRadius( int inLiveCurseCount, int inTotalCurseCount ) {
    return 
        // 0 if no one live is blocking
        curseBlockRadius * inLiveCurseCount +
        // add in extra curve based on total, including offline people
        curseBlockOfflineFactor * 
        pow( inTotalCurseCount, curseBlockOfflineExponent );
    }




char isBirthLocationCurseBlocked( const char *inTargetEmail, GridPos inPos ) {
    
    int radius = getCurseRadius( personalLiveCurseCount, 
                                 personalTotalCurseCount );
    
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
    
    
    
    int radius = getCurseRadius( liveCurseCount,
                                 getCurseCount( inTargetEmail ) );
    
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




