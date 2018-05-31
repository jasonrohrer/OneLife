

#include "stackdb.h"



#include "dbCommon.h"

// two ints to an 8-byte key
void intPairToKey( int inX, int inY, unsigned char *outKey ) {
    for( int i=0; i<4; i++ ) {
        int offset = i * 8;
        outKey[i] = ( inX >> offset ) & 0xFF;
        outKey[i+4] = ( inY >> offset ) & 0xFF;
        }    
    }


int main() {
    
    STACKDB db;
    int error = STACKDB_open( &db, 
                              "testStack.db",
                              0,
                              5,
                              8, // two ints,  x_center, y_center
                              4 // one int
                              );
    
    if( error ) {
        printf( "Error\n" );
        }
    
    int r;
    
    unsigned char key[8];
    unsigned char val[4];
    
    //if( false )
    for( int i=0; i<100; i++ ) {
        
        intPairToKey( i, i, key );
        
        intToValue( i, val );
        
        r = STACKDB_put( &db, key, val );
        
        if( r != 0 ) {
            printf( "Put failed\n" );
            }
        }
    
    //if( false )
    for( int i=0; i<100; i++ ) {
        intPairToKey( i, i, key );
        intToValue( 0, val );

        r = STACKDB_get( &db, key, val );
    
        if( r != 0 ) {
            printf( "Get failed\n" );
            }
    

        int result = valueToInt( val );
    
        printf( "Result = %d\n", result );
        }
    

    STACKDB_Iterator dbi;
    
    STACKDB_Iterator_init( &db, &dbi );
    
    int more = 1;
    
    int hit[100];
    
    for( int i=0; i<100; i++ ) {
        hit[i] = 0;
        }
    
    while( more ) {
        
        more = STACKDB_Iterator_next( &dbi, key, val );
        
        if( more ) {
            int result = valueToInt( val );
            
            printf( "Iter. Result = %d\n", result );
            hit[result]++;
            }
        }
    for( int i=0; i<100; i++ ) {
        printf( "Hit %d:  %d\n", i, hit[i] );
        }
    

    STACKDB_close( &db );
    
    return 0;
    }
