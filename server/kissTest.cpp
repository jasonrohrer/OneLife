

#include "kissdb.h"
#include "dbCommon.h"
#include "minorGems/system/Time.h"

#include <malloc.h>

#include "minorGems/util/random/CustomRandomSource.h"

CustomRandomSource randSource( 0 );



// two ints to an 8-byte key
void intPairToKey( int inX, int inY, unsigned char *outKey ) {
    for( int i=0; i<4; i++ ) {
        int offset = i * 8;
        outKey[i] = ( inX >> offset ) & 0xFF;
        outKey[i+4] = ( inY >> offset ) & 0xFF;
        }    
    }


int lastMallocCheck = 0;

int getMallocDelta() {
    struct mallinfo m = mallinfo();
    
    int current = m.hblkhd + m.usmblks;

    int d = current - lastMallocCheck;
    
    lastMallocCheck = current;
    
    return d;
    }


int main() {

    getMallocDelta();
    
    double startTime = Time::getCurrentTime();
    
    KISSDB db;
    int error = KISSDB_open( &db, 
                             "test.db", 
                             KISSDB_OPEN_MODE_RWCREAT,
                             640000,
                             8, // two ints,  x_center, y_center
                             4 // one int
                             );

    printf( "Opening DB used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );

    if( error ) {
        printf( "Failed to open\n" );
        return 1;
        }


    startTime = Time::getCurrentTime();

    int num = 1000;
    
    unsigned char key[8];
    unsigned char value[4];
    
    int insertCount = 0;
    for( int x=0; x<num; x++ ) {
        for( int y=0; y<num; y++ ) {
            
            insertCount++;
            
            intToValue( x + y, value );
            intPairToKey( x, y, key );
            
            //printf( "Inserting %d,%d\n", x, y );
            
            KISSDB_put( &db, key, value );
            }
        }
    printf( "Inserted %d\n", insertCount );

    printf( "Inserts used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );


    startTime = Time::getCurrentTime();

    for( int i=0; i<insertCount; i++ ) {
        int x = randSource.getRandomBoundedInt( 0, num );
        int y = randSource.getRandomBoundedInt( 0, num );
        intPairToKey( x, y, key );
        KISSDB_get( &db, key, value );
        }

    printf( "Random lookup for %d\n", insertCount );

    printf( "Random look used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );
    

    
    KISSDB_Iterator dbi;

    startTime = Time::getCurrentTime();

    KISSDB_Iterator_init( &db, &dbi );
    
    
    int count = 0;
    while( KISSDB_Iterator_next( &dbi, key, value ) > 0 ) {
        count++;
        }
    printf( "Iterated %d\n", count );

    printf( "Iterating used %d bytes, took %f sec\n", getMallocDelta(),
            Time::getCurrentTime() - startTime );


    KISSDB_close( &db );

    return 0;
    }
