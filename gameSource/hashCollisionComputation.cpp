
// compile with:
// g++ -o hashCollisionComputation hashCollisionComputation.cpp -lgmp

#include <gmp.h>





typedef struct BigFloat {
        mpf_t val;
    } BigFloat;
        


BigFloat initFromInt( int x ) {
    BigFloat bigX;
    
    mpf_init( bigX.val );

    mpf_set_ui( bigX.val, x );

    return bigX;
    }


BigFloat initFromString( const char *s ) {
    BigFloat bigX;
    
    mpf_init( bigX.val );

    mpf_set_str( bigX.val, s, 10 );

    return bigX;
    }




void clear( BigFloat inX ) {
    mpf_clear( inX.val );
    }



BigFloat pow( BigFloat inX, int inY ) {
    
    BigFloat result;
    mpf_init( result.val );
    
    mpf_pow_ui( result.val, inX.val, inY );
    
    return result;
    }



BigFloat add( BigFloat inX, BigFloat inY ) {
    BigFloat result;
    mpf_init( result.val );
    
    mpf_add( result.val, inX.val, inY.val );

    return result;
    }


BigFloat sub( BigFloat inX, BigFloat inY ) {
    BigFloat result;
    mpf_init( result.val );
    
    mpf_sub( result.val, inX.val, inY.val );

    return result;
    }


BigFloat mul( BigFloat inX, BigFloat inY ) {
    BigFloat result;
    mpf_init( result.val );
    
    mpf_mul( result.val, inX.val, inY.val );

    return result;
    }



BigFloat div( BigFloat inX, BigFloat inY ) {
    BigFloat result;
    mpf_init( result.val );
    
    mpf_div( result.val, inX.val, inY.val );

    return result;
    }



int main() {

    mpf_set_default_prec( 10000 );


    int hashHexDigits = 10;

    int numUsers = 1500000;

    BigFloat numUsersBig = initFromInt( numUsers );

    BigFloat hashBase = initFromInt( 16 );
    
    BigFloat hashSpace = pow( hashBase, hashHexDigits );
    

    int numDigits = 100;

    printf( "Num users = %d, num 16-bit hash digits = %d\n", 
            numUsers, hashHexDigits );


    double hashSpaceDouble = mpf_get_d( hashSpace.val );
    
    printf( "Hash space = %.5f\n", hashSpaceDouble );


    // formula found here:
    // https://matt.might.net/articles/counting-hash-collisions/


    // note that doing all the computation in-line like this leaks
    // memory, but it doesn't matter in one-time-code like this
    BigFloat expectedCollisions =
        sub( add( numUsersBig, 
                  mul( hashSpace, 
                       pow( div( sub( hashSpace,
                                      initFromInt( 1 ) ),
                                 hashSpace ),
                            numUsers ) ) ),
             hashSpace );
    
    
    
    double collisions = mpf_get_d( expectedCollisions.val );
    
    printf( "expectedCollisions = %.5f", collisions );


    
    // should clear other BigFloat here, but skipping it for now
    // this will leak memory
    
    clear( numUsersBig );

    clear( hashBase );

    clear( hashSpace );
    

    return 0;
    }
