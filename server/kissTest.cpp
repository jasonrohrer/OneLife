

#include "kissdb.h"
#include "stackdb.h"
#include "lineardb.h"
#include "lineardb2.h"
#include "lineardb3.h"

#include "dbCommon.h"
#include "minorGems/system/Time.h"

#include <malloc.h>
#include <math.h>

#include "minorGems/util/random/CustomRandomSource.h"


//#define TABLE_SIZE 800000
#define TABLE_SIZE 100000

//#define INSERT_SIZE 15000000
//#define INSERT_SIZE 2000000
#define INSERT_SIZE 200000


#define NUM_RUNS 500


#define FLUSH_BETWEEN_OPS


//#define USE_KISSDB
//#define USE_STACKDB
//#define USE_LINEARDB
//#define USE_LINEARDB2
#define USE_LINEARDB3



#ifdef USE_KISSDB

#define DB_name "KissDB"
#define DB KISSDB
#define DB_open KISSDB_open
#define DB_close KISSDB_close
#define DB_get KISSDB_get
#define DB_put KISSDB_put
// kiss makes no distiction between new put and regular put
#define DB_put_new KISSDB_put
#define DB_Iterator  KISSDB_Iterator
#define DB_Iterator_init  KISSDB_Iterator_init
#define DB_Iterator_next  KISSDB_Iterator_next
#define DB_maxStack (int)( db.num_hash_tables )

#endif


#ifdef USE_STACKDB

#define DB_name "StackDB"
#define DB STACKDB
#define DB_open STACKDB_open
#define DB_close STACKDB_close
#define DB_get STACKDB_get
#define DB_put STACKDB_put
// stack db has a faster put_new
#define DB_put_new STACKDB_put_new
#define DB_Iterator  STACKDB_Iterator
#define DB_Iterator_init  STACKDB_Iterator_init
#define DB_Iterator_next  STACKDB_Iterator_next
#define DB_maxStack db.maxStackDepth

#endif



#ifdef USE_LINEARDB

#define DB_name "LinearDB"
#define DB LINEARDB
#define DB_open LINEARDB_open
#define DB_close LINEARDB_close
#define DB_get LINEARDB_get
#define DB_put LINEARDB_put
// linear db has no put_new
#define DB_put_new LINEARDB_put
#define DB_Iterator  LINEARDB_Iterator
#define DB_Iterator_init  LINEARDB_Iterator_init
#define DB_Iterator_next  LINEARDB_Iterator_next
#define DB_maxStack db.maxProbeDepth

#endif



#ifdef USE_LINEARDB2

#define DB_name "LinearDB2"
#define DB LINEARDB2
#define DB_open LINEARDB2_open
#define DB_close LINEARDB2_close
#define DB_get LINEARDB2_get
#define DB_put LINEARDB2_put
// linear db has no put_new
#define DB_put_new LINEARDB2_put
#define DB_Iterator  LINEARDB2_Iterator
#define DB_Iterator_init  LINEARDB2_Iterator_init
#define DB_Iterator_next  LINEARDB2_Iterator_next
#define DB_maxStack db.maxOverflowDepth

#endif



#ifdef USE_LINEARDB3

#define DB_name "LinearDB3"
#define DB LINEARDB3
#define DB_open LINEARDB3_open
#define DB_close LINEARDB3_close
#define DB_get LINEARDB3_get
#define DB_put LINEARDB3_put
// linear db has no put_new
#define DB_put_new LINEARDB3_put
#define DB_Iterator  LINEARDB3_Iterator
#define DB_Iterator_init  LINEARDB3_Iterator_init
#define DB_Iterator_next  LINEARDB3_Iterator_next
#define DB_maxStack db.maxOverflowDepth

#endif



void maybeFlush() {
    #ifdef FLUSH_BETWEEN_OPS
    printf( "Flushing system disk caches.\n" );
    system( "sudo ../gameSource/flushDiskCaches.sh" );
    #endif
    }


CustomRandomSource randSource( 0 );



// two ints to an 8-byte key
void intPairToKey( int inX, int inY, unsigned char *outKey ) {
    for( int i=0; i<4; i++ ) {
        int offset = i * 8;
        outKey[i] = ( inX >> offset ) & 0xFF;
        outKey[i+4] = ( inY >> offset ) & 0xFF;
        }    
    }


// four ints to a 16-byte key
void intQuadToKey( int inX, int inY, int inSlot, int inB, 
                   unsigned char *outKey ) {
    for( int i=0; i<4; i++ ) {
        int offset = i * 8;
        outKey[i] = ( inX >> offset ) & 0xFF;
        outKey[i+4] = ( inY >> offset ) & 0xFF;
        outKey[i+8] = ( inSlot >> offset ) & 0xFF;
        outKey[i+12] = ( inB >> offset ) & 0xFF;
        }    
    }



int lastMallocCheck = 0;

int getMallocDelta() {
    struct mallinfo m = mallinfo();
    
    int current = m.hblkhd + m.uordblks;

    int d = current - lastMallocCheck;
    
    lastMallocCheck = current;
    
    return d;
    }


DB db;
unsigned char key[16];
unsigned char value[4];


// returns num values
unsigned int iterateValues() {
    DB_Iterator dbi;

    double startTime = Time::getCurrentTime();

    DB_Iterator_init( &db, &dbi );
    
    
    unsigned int count = 0;
    int checksum = 0;
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        count++;
        int v = valueToInt( value );
        checksum += v;
        }
    printf( "Iterated %d, checksum %u\n", count, checksum );

    printf( "Iterating used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );


    printf( "Max bin depth = %d\n", DB_maxStack );
    
    return count;
    }





int main() {
    printf( "Starting test for database %s\n", DB_name );
    printf( "TableSize: %d, InsertSize: %d, RunSize: %d\n", 
            TABLE_SIZE, INSERT_SIZE, NUM_RUNS );
    
    getMallocDelta();
    
    double startTime = Time::getCurrentTime();
    
   

    int tableSize = TABLE_SIZE;
    
    int error = DB_open( &db, 
                             "test.db", 
                             KISSDB_OPEN_MODE_RWCREAT,
                             tableSize,
                             16, // four ints,  x_center, y_center, s, b
                             4 // one int
                             );

    printf( "Opening DB (table size %d) used %d bytes, took %f sec\n", 
            tableSize,
            getMallocDelta(),
            Time::getCurrentTime() - startTime );

    if( error ) {
        printf( "Failed to open\n" );
        return 1;
        }


    maybeFlush();
    
    printf( "Iterating through any initial items in database...\n" );
    
    iterateValues();
    

    maybeFlush();


    startTime = Time::getCurrentTime();

    // quad-root
    int num = (int)ceil( pow( INSERT_SIZE, 0.25 ) );
    

    

    // sanity check:
    int testVal = 3248934;
    intToValue( testVal, value );
    int testX = 349;
    int testY = 4480;
    
    intQuadToKey( testX, testY, 0, 0, key );

    error = DB_put_new( &db, key, value );

    if( error != 0 ) {
        printf( "DB_put_new failed\n" );
        return 1;
        }

    int result = DB_get( &db, key, value );
    if( result == 0 ) {
        int v = valueToInt( value );
        if( v != testVal ) {
            printf( "Sanity check failed.  Put %d at (%d,%d), got back %d\n",
                    testVal, testX, testY, v );
            return 1;
            }
        else {
            printf( "Sanity check passed\n" );
            }
        }
    else {
        printf( "Sanity check failed.  Put %d at (%d,%d), result not found\n",
                testVal, testX, testY );
        return 1;
        }
    
    


    int insertCount = 0;
    for( int x=0; x<num && insertCount < INSERT_SIZE; x++ ) {
        for( int y=0; y<num && insertCount < INSERT_SIZE; y++ ) {
            for( int s=0; s<num && insertCount < INSERT_SIZE; s++ ) {
                for( int b=0; b<num && insertCount < INSERT_SIZE; b++ ) {
            
                    insertCount++;
            
                    intToValue( x + y + s + b, value );
                    intQuadToKey( x, y, s, b, key );
                    
                    //printf( "Inserting %d,%d\n", x, y );
            
                    error = DB_put_new( &db, key, value );
                    if( error != 0 ) {
                        printf( "DB_put_new failed\n" );
                        return 1;
                        }
                    }
                }
            }
        }

    /*
#ifdef USE_LINEARDB
    if( db.numTableExpands > 0 ) {
        printf( "%llu cells read on average per table expand (%llu expands), "
                "worst read = %llu\n",
                db.cellsReadOnTableExpand / db.numTableExpands, 
                db.numTableExpands, db.worstCellsReadOnTableExpand );
        printf( "%llu cells moved on average per table expand (%llu expands), "
                "worst moved = %llu\n",
                db.cellsMovedOnTableExpand / db.numTableExpands, 
                db.numTableExpands, db.worstCellsMovedOnTableExpand );
        }
#endif
    */
    printf( "Inserted %d\n", insertCount );

    printf( "Inserts used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );
    printf( "After insert, max stack depth = %d\n", DB_maxStack );
    

    maybeFlush();
    

    startTime = Time::getCurrentTime();

    int lookupCount = 3000;
    int numRuns = NUM_RUNS;
    int numLooks = 0;
    int numHits = 0;
    
    unsigned int checksum = 0;

    for( int r=0; r<numRuns; r++ ) {
        CustomRandomSource runSource( 44493 );

        for( int i=0; i<lookupCount; i++ ) {
            int x = runSource.getRandomBoundedInt( 0, num-1 );
            int y = runSource.getRandomBoundedInt( 0, num-1 );
            int s = runSource.getRandomBoundedInt( 0, num-1 );
            int b = runSource.getRandomBoundedInt( 0, num-1 );
            intQuadToKey( x, y, s, b, key );
            
            int result = DB_get( &db, key, value );
            numLooks ++;
            if( result == 0 ) {
                int v = valueToInt( value );
                checksum += v;
                numHits++;
                }
            else if( result == -1 ) {
                printf( "DB_get failed\n" );
                return 1;
                }
            }
        }
    

    printf( "Random lookup for %d batchs of %d (%d/%d hits)\n", 
            numRuns, lookupCount, numHits, numLooks );

    printf( "Random look used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );

    printf( "Checksum = %u\n", checksum );

    
    maybeFlush();



    startTime = Time::getCurrentTime();
    

    numLooks = 0;
    numHits = 0;
    checksum = 0;
    
    int endNum = 7;

    for( int r=0; r<numRuns; r++ ) {
        for( int x=num-endNum; x<num; x++ ) {
            for( int y=num-endNum; y<num; y++ ) {
                for( int s=num-endNum; s<num; s++ ) {
                    for( int b=num-endNum; b<num; b++ ) {
                        intQuadToKey( x, y, s, b, key );
                        int result = DB_get( &db, key, value );
                        numLooks ++;
                        if( result == 0 ) {
                            int v = valueToInt( value );
                            checksum += v;
                            numHits++;
                            }
                        else if( result == -1 ) {
                            printf( "DB_get failed\n" );
                            return 1;
                            }
                        }
                    }
                }
            }
        }
    
    printf( "Last-inserted lookup for %d batchs of %d (%d/%d hits)\n", 
            numRuns, endNum * endNum * endNum * endNum, numHits, numLooks );

    printf( "Last-inserted lookup used %d bytes, took %f sec\n", 
            getMallocDelta(),
            Time::getCurrentTime() - startTime );

    printf( "Checksum = %u\n", checksum );


    maybeFlush();

    
    startTime = Time::getCurrentTime();
    numLooks = 0;
    numHits = 0;

    for( int r=0; r<numRuns; r++ ) {
        CustomRandomSource runSource( 0 );

        for( int i=0; i<lookupCount; i++ ) {
            // these don't exist
            int x = runSource.getRandomBoundedInt( num + 10, num + num );
            int y = runSource.getRandomBoundedInt( 0, num-1 );
            int s = runSource.getRandomBoundedInt( 0, num-1 );
            int b = runSource.getRandomBoundedInt( 0, num-1 );
            intQuadToKey( x, y, s, b, key );
            int result = DB_get( &db, key, value );
            numLooks ++;
            if( result == 0 ) {
                numHits++;
                }
            else if( result == -1 ) {
                printf( "DB_get failed\n" );
                return 1;
                }
            }
        }
    

    printf( "Random lookup for non-existing %d repeating batchs of %d (%d/%d hits)\n", 
            numRuns, lookupCount, numHits, numLooks );

    printf( "Random look/miss used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );
    
    maybeFlush();
    



    startTime = Time::getCurrentTime();
    numLooks = 0;
    numHits = 0;

    CustomRandomSource runSource( 0 );
    
    for( int r=0; r<numRuns; r++ ) {
        for( int i=0; i<lookupCount; i++ ) {
            // these don't exist
            int x = runSource.getRandomBoundedInt( num + 10, num + num );
            int y = runSource.getRandomBoundedInt( 0, num-1 );
            int s = runSource.getRandomBoundedInt( 0, num-1 );
            int b = runSource.getRandomBoundedInt( 0, num-1 );
            intQuadToKey( x, y, s, b, key );
            int result = DB_get( &db, key, value );
            numLooks ++;
            if( result == 0 ) {
                numHits++;
                }
            else if( result == -1 ) {
                printf( "DB_get failed\n" );
                return 1;
                }
            }
        }
    

    printf( "Random lookup for non-existing %d non-repeating values (%d/%d hits)\n", 
            numRuns * lookupCount, numHits, numLooks );

    printf( "Random look/miss used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );
    
    maybeFlush();
    


    
    
    startTime = Time::getCurrentTime();

    for( int r=0; r<numRuns; r++ ) {
        CustomRandomSource runSource( 0 );

        for( int i=0; i<lookupCount; i++ ) {
            // these don't exist
            int x = runSource.getRandomBoundedInt( num, num + num );
            int y = runSource.getRandomBoundedInt( 0, num-1 );
            int s = runSource.getRandomBoundedInt( 0, num-1 );
            int b = runSource.getRandomBoundedInt( 0, num-1 );
            intQuadToKey( x, y, s, b, key );
            intToValue( x + y + s + b, value );
            int error = DB_put( &db, key, value );
            
            if( error == -1 ) {
                printf( "DB_put failed\n" );
                return 1;
                }
            }
        }
    

    printf( "Inserts for previously non-existing %d batchs of %d\n", 
            numRuns, lookupCount);

    printf( "Inserts after miss used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );
    

    maybeFlush();


    
    unsigned int numValues = iterateValues();


#ifdef USE_LINEARDB

    printf( "Base size of db is %d\n", db.hashTableSizeA );
    printf( "Expanded size of db is %d\n", db.hashTableSizeB );

    unsigned int shrinkSize = LINEARDB_getShrinkSize( &db, numValues );
    
    printf( "Optimal shrink size for %d values is %d\n", numValues,
            shrinkSize );
    

    DB db2;
    
    error = DB_open( &db2, 
                     "test2.db", 
                     KISSDB_OPEN_MODE_RWCREAT,
                     shrinkSize,
                     //lrint( baseSize * 0.549382 ),
                     16, // four ints,  x_center, y_center, s, b
                     4 // one int
                     );

    maybeFlush();
    
    startTime = Time::getCurrentTime();
    
    DB_Iterator dbi;

    DB_Iterator_init( &db, &dbi );
    
    
    int count = 0;
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        count++;

        error = DB_put_new( &db2, key, value );

        if( error == -1 ) {
            printf( "DB_put_new failed\n" );
            return 1;
            }
        }
    
    
    printf( "Iterated through db and inserted %d values into db2, \n", count );

    printf( "Iterating/inserting used %d bytes, took %f sec\n", 
            getMallocDelta(),
            Time::getCurrentTime() - startTime );
    
    DB_close( &db2 );
#endif

    
    DB_close( &db );

    return 0;
    }
