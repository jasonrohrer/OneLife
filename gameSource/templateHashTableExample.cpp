

struct V {
        int a;
        int b;

        V( int inA, int inB );
        
    };

V::V( int inA, int inB )
        : a( inA ), b( inB ) {

    }



struct A {
        int x;
        
        int y;

        void incrementX( V inV );
    };

void A::incrementX( V inV ) {
    x += inV.a + inV.b;
    }

#include <stdio.h>


// pass TRUE into UCustomHash if a specialization of customHash
// has been defined
template< typename TKey, typename TValue, char UCustomHash = false >
struct HashTable {
        void insert( TKey inKey, TValue inValue );
        
        int hash( TKey inKey );
        
        int genericHash( TKey inKey );
        
        int customHash( TKey inKey );
        
        int hashBytes( unsigned char *inBytes, int inNumBytes);
        
    };

template< typename TKey, typename TValue, char UCustomHash >
void HashTable<TKey, TValue, UCustomHash>::insert( TKey inKey, 
                                                    TValue inValue ) {
    int h = hash( inKey );

    printf( "Insert, hash = %d\n", h );
    }


template< typename TKey, typename TValue, char UCustomHash  >
int HashTable<TKey, TValue, UCustomHash>::hash( TKey inKey ) {
    
    if( UCustomHash ) {
        return customHash( inKey );
        }
    else {
        return genericHash( inKey );
        }
    }


template< typename TKey, typename TValue, char UCustomHash >
int HashTable<TKey, TValue, UCustomHash>::genericHash( TKey inKey ) {
    
    unsigned char *keyBytes = (unsigned char*)( &inKey );
    int numKeyBytes = sizeof( inKey );

    return hashBytes( keyBytes, numKeyBytes );
    }


template< typename TKey, typename TValue, char UCustomHash >
int HashTable<TKey, TValue, UCustomHash>::hashBytes( unsigned char *inBytes, 
                                                      int inNumBytes) {
    int byteSum = 0;
    for( int i=0; i<inNumBytes; i++ ) {
        byteSum += inBytes[i];
        }
    
    return byteSum;
    }


#include <string.h>

template<>
int HashTable<char *, V, true>::customHash( char *inStringKey ) {
    
    unsigned char *keyBytes = (unsigned char*)( inStringKey );
    int numKeyBytes = strlen( inStringKey );
    
    return hashBytes( keyBytes, numKeyBytes );
    }




int main() {
    
    A a;
    
    HashTable<V,V> test;
    
    test.insert( V( 5, 4 ), V( 6, 7 ) );

    HashTable<int,V> test2;
    
    test2.insert( 5, V( 8, 9 ) );

    HashTable<char*,V, true> test3;

    test3.insert( (char*)"hey", V( 8, 9 ) );
    

    a.x = 1;
    a.incrementX( V(5, 4) );
    
    printf( "a.x = %d\n", a.x );
    
    return 0;
    }
