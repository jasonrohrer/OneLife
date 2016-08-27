#include "minorGems/util/SimpleVector.h"



template <class Type> 
class HashTable {
    
    public:
        
        HashTable( int inSize );
        
        ~HashTable();
        
        Type lookup( int inKeyA, int inKeyB, int inKeyC, char *outFound );
        
        void insert( int inKeyA, int inKeyB, int inKeyC, Type inItem );

        void remove( int inKeyA, int inKeyB, int inKeyC );
        

        int getNumElements() {
            return mNumElements;
            }
        
        // flush all entries from table
        void clear();
        
    private:
        int mSize;
        
        int mNumElements;
        

        SimpleVector<Type> *mTable;
        
        SimpleVector<int> *mKeysA;
        SimpleVector<int> *mKeysB;
        SimpleVector<int> *mKeysC;
        
        int computeHash( int inKeyA, int inKeyB, int inKeyC );
        
        char lookupBin( int inKeyA, int inKeyB, int inKeyC,
                        int *outHashKey, 
                        int *outBin );
        
    };



// Wow... never new that template function implementations must be in the
// same file as the declaration


template <class Type> 
HashTable<Type>::HashTable( int inSize )
        : mSize( inSize ),
          mNumElements( 0 ),
          mTable( new SimpleVector<Type>[ inSize ] ),
          mKeysA( new SimpleVector<int>[ inSize ] ),
          mKeysB( new SimpleVector<int>[ inSize ] ),
          mKeysC( new SimpleVector<int>[ inSize ] ) {
    
        
    }




        
template <class Type> 
HashTable<Type>::~HashTable() {
    delete [] mTable;
    delete [] mKeysA;
    delete [] mKeysB;
    delete [] mKeysC;
    }



template <class Type> 
inline int HashTable<Type>::computeHash( int inKeyA, int inKeyB, int inKeyC ) {
    
    int hashKey = ( inKeyA * 734727 + inKeyB * 263474 + inKeyC * 2753 ) % mSize;
    if( hashKey < 0 ) {
        hashKey += mSize;
        }
    return hashKey;
    }



template <class Type> 
char HashTable<Type>::lookupBin( int inKeyA, int inKeyB, int inKeyC,
                                 int *outHashKey, 
                                 int *outBin ) {

    int hashKey = computeHash( inKeyA, inKeyB, inKeyC );
        
    int numBins = mTable[hashKey].size();
    
    *outHashKey = hashKey;
    
    
    for( int i=0; i<numBins; i++ ) {
        if( mKeysA[hashKey].getElementDirect( i ) == inKeyA &&
            mKeysB[hashKey].getElementDirect( i ) == inKeyB &&
            mKeysC[hashKey].getElementDirect( i ) == inKeyC ) {
            
            *outBin = i;
            return true;
            }
        }
    
    return false;
    }


        
template <class Type> 
Type HashTable<Type>::lookup( int inKeyA, int inKeyB, int inKeyC, 
                              char *outFound ) {
    
    int hashKey;

    int bin;
    
    *outFound = lookupBin( inKeyA, inKeyB, inKeyC, &hashKey, &bin );

    if( *outFound ) {
                
        return mTable[ hashKey ].getElementDirect( bin );
        }

    // else return an undefined item (okay, since outFound is false);

    Type undefined;
    return undefined;
    }



template <class Type> 
void HashTable<Type>::insert( int inKeyA, int inKeyB, int inKeyC, 
                              Type inItem ) {
    
    int hashKey;

    int bin;
    
    char found = lookupBin( inKeyA, inKeyB, inKeyC, &hashKey, &bin );


    if( found ) {
        // replace
        *( mTable[ hashKey ].getElement( bin ) ) = inItem;
        }
    else {
        // add a new bin for it
        mTable[ hashKey ].push_back( inItem );
        mKeysA[ hashKey ].push_back( inKeyA );
        mKeysB[ hashKey ].push_back( inKeyB );
        mKeysC[ hashKey ].push_back( inKeyC );
        
        mNumElements++;
        }
    }



template <class Type>
void HashTable<Type>::remove( int inKeyA, int inKeyB, int inKeyC ) {
    int hashKey;

    int bin;
    
    char found = lookupBin( inKeyA, inKeyB, inKeyC, &hashKey, &bin );

    if( found ) {
        // remove
        mTable[ hashKey ].deleteElement( bin );
        mKeysA[ hashKey ].deleteElement( bin );
        mKeysB[ hashKey ].deleteElement( bin );
        mKeysC[ hashKey ].deleteElement( bin );
        
        mNumElements--;
        }
    }



template <class Type> 
void HashTable<Type>::clear() {
    
    for( int i=0; i<mSize; i++ ) {
        mTable[i].deleteAll();
        mKeysA[i].deleteAll();
        mKeysB[i].deleteAll();
        mKeysC[i].deleteAll();
        }

    mNumElements = 0;
    }

