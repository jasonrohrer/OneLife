#include "minorGems/util/SimpleVector.h"



template <class Type> 
class HashTable {
    
    public:
        
        // note that inDefaultValue MUST be provided
        // for any Type that cannot have a value of NULL (example: a struct)
        HashTable( int inSize,
                   Type inDefaultValue = (Type)NULL );
        
        ~HashTable();
        
        Type lookup( int inKeyA, int inKeyB, int inKeyC, int inKeyD,
                     char *outFound );

        // pointer to entry
        Type *lookupPointer( int inKeyA, int inKeyB, int inKeyC, int inKeyD,
                             char *outFound );
        
        void insert( int inKeyA, int inKeyB, int inKeyC, int inKeyD,
                     Type inItem );

        void remove( int inKeyA, int inKeyB, int inKeyC, int inKeyD );
        

        int getNumElements() {
            return mNumElements;
            }
        
        // flush all entries from table
        void clear();
        
    private:
        int mSize;
        
        int mNumElements;
        
        Type mDefaultValue;

        SimpleVector<Type> *mTable;
        
        SimpleVector<int> *mKeysA;
        SimpleVector<int> *mKeysB;
        SimpleVector<int> *mKeysC;
        SimpleVector<int> *mKeysD;
        
        int computeHash( int inKeyA, int inKeyB, int inKeyC, int inKeyD );
        
        char lookupBin( int inKeyA, int inKeyB, int inKeyC, int inKeyD,
                        int *outHashKey, 
                        int *outBin );
        
    };



// Wow... never new that template function implementations must be in the
// same file as the declaration


template <class Type> 
HashTable<Type>::HashTable( int inSize, Type inDefaultValue )
        : mSize( inSize ),
          mNumElements( 0 ),
          mDefaultValue( inDefaultValue ),
          mTable( new SimpleVector<Type>[ inSize ] ),
          mKeysA( new SimpleVector<int>[ inSize ] ),
          mKeysB( new SimpleVector<int>[ inSize ] ),
          mKeysC( new SimpleVector<int>[ inSize ] ),
          mKeysD( new SimpleVector<int>[ inSize ] ) {
    
        
    }




        
template <class Type> 
HashTable<Type>::~HashTable() {
    delete [] mTable;
    delete [] mKeysA;
    delete [] mKeysB;
    delete [] mKeysC;
    delete [] mKeysD;
    }



template <class Type> 
inline int HashTable<Type>::computeHash( int inKeyA, int inKeyB, int inKeyC,
                                         int inKeyD ) {
    
    int hashKey = ( inKeyA * 734727 + inKeyB * 263471 + inKeyC * 2753 +
                    inKeyD * 948731 ) % mSize;
    if( hashKey < 0 ) {
        hashKey += mSize;
        }
    return hashKey;
    }



template <class Type> 
char HashTable<Type>::lookupBin( int inKeyA, int inKeyB, int inKeyC, int inKeyD,
                                 int *outHashKey, 
                                 int *outBin ) {

    int hashKey = computeHash( inKeyA, inKeyB, inKeyC, inKeyD );
        
    int numBins = mTable[hashKey].size();
    
    *outHashKey = hashKey;
    
    
    for( int i=0; i<numBins; i++ ) {
        if( mKeysA[hashKey].getElementDirect( i ) == inKeyA &&
            mKeysB[hashKey].getElementDirect( i ) == inKeyB &&
            mKeysC[hashKey].getElementDirect( i ) == inKeyC &&
            mKeysD[hashKey].getElementDirect( i ) == inKeyD ) {
            
            *outBin = i;
            return true;
            }
        }
    
    return false;
    }


        
template <class Type> 
Type HashTable<Type>::lookup( int inKeyA, int inKeyB, int inKeyC, int inKeyD, 
                              char *outFound ) {
    
    int hashKey;

    int bin;
    
    *outFound = lookupBin( inKeyA, inKeyB, inKeyC, inKeyD, &hashKey, &bin );

    if( *outFound ) {
                
        return mTable[ hashKey ].getElementDirect( bin );
        }

    // else return an undefined item (okay, since outFound is false);
    return mDefaultValue;
    }



template <class Type> 
Type *HashTable<Type>::lookupPointer( int inKeyA, int inKeyB, int inKeyC,
                                      int inKeyD,
                                      char *outFound ) {
    
    int hashKey;

    int bin;
    
    *outFound = lookupBin( inKeyA, inKeyB, inKeyC, inKeyD, &hashKey, &bin );

    if( *outFound ) {
                
        return mTable[ hashKey ].getElement( bin );
        }
    
    return NULL;
    }



template <class Type> 
void HashTable<Type>::insert( int inKeyA, int inKeyB, int inKeyC, int inKeyD, 
                              Type inItem ) {
    
    int hashKey;

    int bin;
    
    char found = lookupBin( inKeyA, inKeyB, inKeyC, inKeyD, &hashKey, &bin );


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
        mKeysD[ hashKey ].push_back( inKeyD );
        
        mNumElements++;
        }
    }



template <class Type>
void HashTable<Type>::remove( int inKeyA, int inKeyB, int inKeyC, int inKeyD ) {
    int hashKey;

    int bin;
    
    char found = lookupBin( inKeyA, inKeyB, inKeyC, inKeyD, &hashKey, &bin );

    if( found ) {
        // remove
        mTable[ hashKey ].deleteElement( bin );
        mKeysA[ hashKey ].deleteElement( bin );
        mKeysB[ hashKey ].deleteElement( bin );
        mKeysC[ hashKey ].deleteElement( bin );
        mKeysD[ hashKey ].deleteElement( bin );
        
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
        mKeysD[i].deleteAll();
        }

    mNumElements = 0;
    }

