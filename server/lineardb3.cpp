#define _FILE_OFFSET_BITS 64

#include "lineardb3.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef _WIN32
#define fseeko fseeko64
#define ftello ftello64
#endif


#define DEFAULT_MAX_LOAD 0.5


#define MIN_OVERFLOW_SIZE 128


#include "murmurhash2_64.cpp"

/*
// djb2 hash function
static uint64_t djb2( const void *inB, unsigned int inLen ) {
    uint64_t hash = 5381;
    for( unsigned int i=0; i<inLen; i++ ) {
        hash = ((hash << 5) + hash) + (uint64_t)(((const uint8_t *)inB)[i]);
        }
    return hash;
    }
*/

// function used here must have the following signature:
// static uint64_t LINEARDB3_hash( const void *inB, unsigned int inLen );
// murmur2 seems to have equal performance on real world data
// and it just feels safer than djb2, which must have done well on test
// data for a weird reson
#define LINEARDB3_hash(inB, inLen) MurmurHash64( inB, inLen, 0xb9115a39 )

// djb2 is resulting in way fewer collisions in test data
//#define LINEARDB3_hash(inB, inLen) djb2( inB, inLen )


/*
// computes 8-bit hashing using different method from LINEARDB3_hash
static uint8_t byteHash( const void *inB, unsigned int inLen ) {
    // use different seed
    uint64_t bigHash = MurmurHash64( inB, inLen, 0x202a025d );
    
    // xor all 8 bytes together
    uint8_t smallHash = bigHash & 0xFF;
    
    smallHash = smallHash ^ ( ( bigHash >> 8 ) & 0xFF );
    smallHash = smallHash ^ ( ( bigHash >> 16 ) & 0xFF );
    smallHash = smallHash ^ ( ( bigHash >> 24 ) & 0xFF );
    smallHash = smallHash ^ ( ( bigHash >> 32 ) & 0xFF );
    smallHash = smallHash ^ ( ( bigHash >> 40 ) & 0xFF );
    smallHash = smallHash ^ ( ( bigHash >> 48 ) & 0xFF );
    smallHash = smallHash ^ ( ( bigHash >> 56 ) & 0xFF );
    
    return smallHash;
    }
*/


/*
// computes 16-bit hashing using different method from LINEARDB3_hash
// hash value is > 0  (so 0 can be used to mark empty cells)
static uint16_t shortHash( const void *inB, unsigned int inLen ) {
    // use different seed
    uint64_t bigHash = MurmurHash64( inB, inLen, 0x202a025d );
    
    // xor all 2-byte chunks together
    uint16_t smallHash = bigHash & 0xFFFF;
    
    smallHash = smallHash ^ ( ( bigHash >> 16 ) & 0xFFFF );
    smallHash = smallHash ^ ( ( bigHash >> 32 ) & 0xFFFF );
    smallHash = smallHash ^ ( ( bigHash >> 48 ) & 0xFFFF );
    
    if( smallHash == 0 ) {
        smallHash = 1;
        }

    return smallHash;
    }
*/




static const char *magicString = "Ld2";

// Ld2 magic characters plus
// four 32-bit ints
#define LINEARDB3_HEADER_SIZE 19






static void recreateMaps( LINEARDB3 *inDB, 
                          unsigned int inOldTableSizeA = 0 ) {
    


    FingerprintBucket3 *oldFingerprintMap = inDB->fingerprintMap;
    
    inDB->fingerprintMap = new FingerprintBucket3[ inDB->hashTableSizeA * 2 ];
    
    memset( inDB->fingerprintMap, 0, 
            inDB->hashTableSizeA * 2 * sizeof( FingerprintBucket3 ) );


    if( oldFingerprintMap != NULL ) {
        if( inOldTableSizeA > 0 ) {
            memcpy( inDB->fingerprintMap,
                    oldFingerprintMap,
                    inOldTableSizeA * 2 * sizeof( FingerprintBucket3 ) );
            }
        
        delete [] oldFingerprintMap;
        }

    }






// returns 0 on success, -1 on error
static int writeHeader( LINEARDB3 *inDB ) {
    if( fseeko( inDB->file, 0, SEEK_SET ) ) {
        return -1;
        }

    int numWritten;
        
    numWritten = fwrite( magicString, strlen( magicString ), 
                         1, inDB->file );
    if( numWritten != 1 ) {
        return -1;
        }


    uint32_t val32;

    val32 = inDB->hashTableSizeA;
    
    numWritten = fwrite( &val32, sizeof(uint32_t), 1, inDB->file );
    if( numWritten != 1 ) {
        return -1;
        }
        
    val32 = inDB->hashTableSizeB;
    
    numWritten = fwrite( &val32, sizeof(uint32_t), 1, inDB->file );
    if( numWritten != 1 ) {
        return -1;
        }

    
    val32 = inDB->keySize;
    
    numWritten = fwrite( &val32, sizeof(uint32_t), 1, inDB->file );
    if( numWritten != 1 ) {
        return -1;
        }


    val32 = inDB->valueSize;
    
    numWritten = fwrite( &val32, sizeof(uint32_t), 1, inDB->file );
    if( numWritten != 1 ) {
        return -1;
        }

    return 0;
    }



static int getRecordSizeBytes( int inKeySize, int inValueSize ) {
    return inKeySize + inValueSize;
    }


static void recomputeFingerprintMod( LINEARDB3 *inDB ) {
    inDB->fingerprintMod = inDB->hashTableSizeA;
    
    while( true ) {
        
        uint32_t newMod = inDB->fingerprintMod * 2;
        
        if( newMod <= inDB->fingerprintMod ) {
            // reached 32-bit limit
            return;
            }
        else {
            inDB->fingerprintMod = newMod;
            }
        }
    }



int LINEARDB3_getOrPut( LINEARDB3 *inDB, const void *inKey, void *inOutValue,
                        char inPut, char inWriteDataFile = true );




int LINEARDB3_open(
    LINEARDB3 *inDB,
    const char *inPath,
    int inMode,
    unsigned int inHashTableStartSize,
    unsigned int inKeySize,
    unsigned int inValueSize ) {
    
    inDB->recordBuffer = NULL;
    inDB->fingerprintMap = NULL;
    inDB->maxOverflowDepth = 0;

    inDB->numRecords = 0;
    
    inDB->maxLoad = DEFAULT_MAX_LOAD;
    

    inDB->file = fopen( inPath, "r+b" );
    
    if( inDB->file == NULL ) {
        // doesn't exist yet
        inDB->file = fopen( inPath, "w+b" );
        }
    
    if( inDB->file == NULL ) {
        return 1;
        }
    

    inDB->hashTableSizeA = inHashTableStartSize;
    inDB->hashTableSizeB = inHashTableStartSize;
    
    inDB->keySize = inKeySize;
    inDB->valueSize = inValueSize;
    
    // first byte in record is present flag
    inDB->recordSizeBytes = getRecordSizeBytes( inKeySize, inValueSize );
    
    inDB->recordBuffer = new uint8_t[ inDB->recordSizeBytes ];


    recomputeFingerprintMod( inDB );
    

    // does the file already contain a header
    
    // seek to the end to find out file size

    if( fseeko( inDB->file, 0, SEEK_END ) ) {
        return 1;
        }

    

    // empty overflow area
    inDB->overflowAreaSize = MIN_OVERFLOW_SIZE;
    inDB->overflowFingerprintBuckets = 
        new FingerprintBucket3[ inDB->overflowAreaSize ];
    
    memset( inDB->overflowFingerprintBuckets, 0, 
            inDB->overflowAreaSize * sizeof( FingerprintBucket3 ) );
    
    
    
    if( ftello( inDB->file ) < LINEARDB3_HEADER_SIZE ) {
        // file that doesn't even contain the header


        // write fresh header and hash table

        // rewrite header
    
        if( writeHeader( inDB ) != 0 ) {
            return 1;
            }

        
        // empty fingerprint map
        recreateMaps( inDB );
        }
    else {
        // read header
        if( fseeko( inDB->file, 0, SEEK_SET ) ) {
            return 1;
            }
        
        
        int numRead;
        
        char magicBuffer[ 4 ];
        
        numRead = fread( magicBuffer, 3, 1, inDB->file );

        if( numRead != 1 ) {
            return 1;
            }

        magicBuffer[3] = '\0';
        
        if( strcmp( magicBuffer, magicString ) != 0 ) {
            printf( "lineardb3 magic string '%s' not found at start of  "
                    "file header\n", magicString );
            return 1;
            }
        

        uint32_t val32;

        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            return 1;
            }        

        // can vary in size from what's been requested
        inDB->hashTableSizeA = val32;

        
        // now read sizeB
        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            return 1;
            }

        

        if( val32 < inDB->hashTableSizeA ) {
            printf( "lineardb3 hash table base size of %u is larger than "
                    "expanded size of %u in file header\n", 
                    inDB->hashTableSizeA, val32 );
            return 1;
            }


        if( val32 >= inDB->hashTableSizeA * 2 ) {
            printf( "lineardb3 hash table expanded size of %u is 2x or more "
                    "larger than base size of %u in file header\n", 
                    val32, inDB->hashTableSizeA );
            return 1;
            }

        // can vary in size from what's been requested
        inDB->hashTableSizeB = val32;
        

        
        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            return 1;
            }
        
        if( val32 != inKeySize ) {
            printf( "Requested lineardb3 key size of %u does not match "
                    "size of %u in file header\n", inKeySize, val32 );
            return 1;
            }
        


        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            return 1;
            }
        
        if( val32 != inValueSize ) {
            printf( "Requested lineardb3 value size of %u does not match "
                    "size of %u in file header\n", inValueSize, val32 );
            return 1;
            }
        

        // got here, header matches


        // make sure hash table exists in file
        if( fseeko( inDB->file, 0, SEEK_END ) ) {
            return 1;
            }
 
        uint64_t fileSize = ftello( inDB->file );


        recreateMaps( inDB );
        recomputeFingerprintMod( inDB );

        uint64_t numRecordsInFile = 
            ( fileSize - LINEARDB3_HEADER_SIZE ) / inDB->recordSizeBytes;
        
        if( inDB->recordSizeBytes * numRecordsInFile + LINEARDB3_HEADER_SIZE
            != fileSize ) {
            
            printf( "Requested lineardb3 file does not contain a whole number "
                    "of %d-byte records.\n", inDB->recordSizeBytes );
            return 1;
            }
        
        
        // now populate hash table

        if( fseeko( inDB->file, LINEARDB3_HEADER_SIZE, SEEK_SET ) ) {
            return 1;
            }
        
        for( uint64_t i=0; i<numRecordsInFile; i++ ) {
            int numRead = fread( inDB->recordBuffer, 
                                 inDB->recordSizeBytes, 1, inDB->file );
            
            if( numRead != 1 ) {
                printf( "Failed to read record from lineardb3 file\n" );
                return 1;
                }

            // put only in RAM part of table
            int result = 
                LINEARDB3_getOrPut( inDB,
                                    &( inDB->recordBuffer[0] ),
                                    &( inDB->recordBuffer[inDB->keySize] ),
                                    true, false );
            if( result != 0 ) {
                printf( "Putting lineardb3 record in RAM hash table failed\n" );
                return 1;
                }
            }
        }
    

    


    return 0;
    }




void LINEARDB3_close( LINEARDB3 *inDB ) {
    if( inDB->recordBuffer != NULL ) {
        delete [] inDB->recordBuffer;
        inDB->recordBuffer = NULL;
        }    


    if( inDB->fingerprintMap != NULL ) {
        delete [] inDB->fingerprintMap;
        inDB->fingerprintMap = NULL;
        }    

    if( inDB->file != NULL ) {
        fclose( inDB->file );
        inDB->file = NULL;
        }
    }




void LINEARDB3_setMaxLoad( LINEARDB3 *inDB, double inMaxLoad ) {
    inDB->maxLoad = inMaxLoad;
    }




inline char keyComp( int inKeySize, const void *inKeyA, const void *inKeyB ) {
    uint8_t *a = (uint8_t*)inKeyA;
    uint8_t *b = (uint8_t*)inKeyB;
    
    for( int i=0; i<inKeySize; i++ ) {
        if( a[i] != b[i] ) {
            return false;
            }
        }
    return true;
    }






/*
// uses method described here:
// https://en.wikipedia.org/wiki/Linear_hashing
// But adds support for linear probing by potentially rehashing
// all cells hit by a linear probe from the split point
// returns 0 on success, -1 on failure
//
// This call may expand the table by more than one cell, until the table
// is big enough that it's at or below the maxLoad
static int expandTable( LINEARDB3 *inDB ) {

    unsigned int oldSplitPoint = inDB->hashTableSizeB - inDB->hashTableSizeA;
    

    // expand table until we are back at or below maxLoad
    int numExpanded = 0;
    while( (double)( inDB->numRecords ) /
           (double)( inDB->hashTableSizeB ) > inDB->maxLoad ) {

        inDB->hashTableSizeB ++;
        numExpanded++;
        
        if( inDB->hashTableSizeB == inDB->hashTableSizeA * 2 ) {
            // full round of expansion is done.
        
            unsigned int oldTableSizeA = inDB->hashTableSizeA;
        
            inDB->hashTableSizeA = inDB->hashTableSizeB;
            

            recreateMaps( inDB, oldTableSizeA );
            }
        }
    


    // add extra cells at end of the file
    if( fseeko( inDB->file, 0, SEEK_END ) ) {
        return -1;
        }
    memset( inDB->recordBuffer, 0, inDB->recordSizeBytes );

    for( int c=0; c<numExpanded; c++ ) {    
        int numWritten = 
            fwrite( inDB->recordBuffer, inDB->recordSizeBytes, 1, inDB->file );
        
        if( numWritten != 1 ) {
            return -1;
            }
        }
    


    // existence and fingerprint maps already 0 for these extra cells
    // (they are big enough already to have room for it at the end)




    // remove and re-insert all contiguous cells from the region
    // between the old and new split point
    // we need to ensure there are no holes for future linear probes
    

    int result = reinsertCellSegment( inDB, oldSplitPoint, numExpanded );
    if( result == -1 ) {
        return -1;
        }

    
    
    
    // do the same for first cell of table, which might have wraped-around
    // cells in it due to linear probing, and there might be an empty cell
    // at the end of the table now that we've expanded it
    // there is no minimum number we must examine, if first cell is empty
    // looking at just that cell is enough
    result = reinsertCellSegment( inDB, 0, 1 );
    

    if( result == -1 ) {
        return -1;
        }

    
    

    inDB->tableSizeBytes = 
        (uint64_t)( inDB->recordSizeBytes ) * 
        (uint64_t)( inDB->hashTableSizeB );

    // write latest sizes into header
    return writeHeader( inDB );
    }
*/




static uint64_t getBinNumber( LINEARDB3 *inDB, const void *inKey,
                              uint32_t *outFingerprint ) {
    
    uint64_t hashVal = LINEARDB3_hash( inKey, inDB->keySize );

    *outFingerprint = hashVal & inDB->fingerprintMod;



    if( *outFingerprint == 0 ) {
        // forbid straight 0 as fingerprint value
        // we use 0-fingerprints as not-present flags
        // for the rare values that land on 0, we need to make sure
        // main hash changes along with fingerprint
        
        if( hashVal < UINT64_MAX ) {
            hashVal++;
            }
        else {
            hashVal--;
            }
        
        *outFingerprint = hashVal & inDB->fingerprintMod;
        }
    
    
    

    uint64_t binNumberA = hashVal % (uint64_t)( inDB->hashTableSizeA );
    
    uint64_t binNumberB = binNumberA;
    


    unsigned int splitPoint = inDB->hashTableSizeB - inDB->hashTableSizeA;
    
    
    if( binNumberA < splitPoint ) {
        // points before split can be mod'ed with double base table size

        // binNumberB will always fit in hashTableSizeB, the expanded table
        binNumberB = hashVal % (uint64_t)( inDB->hashTableSizeA * 2 );
        }
    return binNumberB;
    }







// NEVER uses overflow index 0
// always leaves it empty (so that index 0 can be used to signify no
// overflow pointer)
static int makeNewOverflowBucket( LINEARDB3 *inDB, uint32_t inFingerprint,
                                  uint32_t inDataFileIndex,
                                  uint32_t *outOverflowIndex ) {
    char found = false;
    uint32_t foundIndex = 0;
    
    // skip 0
    for( uint32_t i=1; i<inDB->overflowAreaSize; i++ ) {
        
        // only need to look at first bin to detect empty slot
        if( inDB->overflowFingerprintBuckets[ i ].fingerprints[0] == 0 ) {
            found = true;
            foundIndex = i;
            break;
            }
        }

    
    if( !found ) {
        // entire overflow area is full
        // double it
        uint32_t oldSize = inDB->overflowAreaSize;
        FingerprintBucket3 *oldOverflow = inDB->overflowFingerprintBuckets;
        
        inDB->overflowAreaSize *= 2;
        
        inDB->overflowFingerprintBuckets = 
            new FingerprintBucket3[ inDB->overflowAreaSize ];
        
        // zero new space
        memset( inDB->overflowFingerprintBuckets, 0,
                inDB->overflowAreaSize * sizeof( FingerprintBucket3 ) );

        memcpy( inDB->overflowFingerprintBuckets,
                oldOverflow, oldSize * sizeof( FingerprintBucket3 ) );
        
        delete [] oldOverflow;

        // first bin in extended area
        found = true;
        foundIndex = oldSize;
        }
    
    // fingerprint
    inDB->overflowFingerprintBuckets[ foundIndex ].fingerprints[0] = 
        inFingerprint;

    inDB->overflowFingerprintBuckets[ foundIndex ].fileIndex[0] = 
        inDataFileIndex;

    
    *outOverflowIndex = foundIndex;
    return 0;
    }



// Consider getting/putting from inBucket at inRecIndex
//
// returns 0 if handled and done
// returns -1 on error
// returns 1 if guaranteed not found
// returns 2 if bucket full and not found 
static int LINEARDB3_considerFingerprintBucket( LINEARDB3 *inDB, 
                                                const void *inKey, 
                                                void *inOutValue,
                                                uint32_t inFingerprint,
                                                char inPut, 
                                                char inWriteDataFile,
                                                FingerprintBucket3 *inBucket,
                                                int inRecIndex ) {       
    int i = inRecIndex;
    
    uint32_t binFP = inBucket->fingerprints[ i ];
        
    char emptyRec = false;
        
    if( binFP == 0 ) {
        emptyRec = true;
            
        if( inPut ) {
            // set fingerprint and file pos for insert
            binFP = inFingerprint;
            inBucket->fingerprints[ i ] = inFingerprint;
                
            // will go at end of file
            inBucket->fileIndex[ i ] = inDB->numRecords;
                

            inDB->numRecords++;
            }
        else {
            return 1;
            }
        }
        
    if( binFP == inFingerprint ) {
        // hit
            
        uint64_t filePosRec = 
            LINEARDB3_HEADER_SIZE +
            inBucket->fileIndex[ i ] * 
            inDB->recordSizeBytes;
            
        if( !emptyRec ) {
            
            // read key to make sure it actually matches
            if( fseeko( inDB->file, filePosRec, SEEK_SET ) ) {
                return -1;
                }
            int numRead = fread( inDB->recordBuffer, inDB->keySize, 1,
                                 inDB->file );
                
            if( numRead != 1 ) {
                return -1;
                }
            if( ! keyComp( inDB->keySize, inDB->recordBuffer, inKey ) ) {
                // false match on non-empty rec because of fingerprint
                // collision
                return 2;
                }
            }

            
        if( inPut && inWriteDataFile ) {
                
            if( emptyRec ) {
                // no seeking done yet
                // go to end of file
                if( fseeko( inDB->file, 0, SEEK_END ) ) {
                    return -1;
                    }

                // make sure it matches where we've documented that
                // the record should go
                if( ftello( inDB->file ) != filePosRec ) {
                    return -1;
                    }
                
                int numWritten = 
                    fwrite( inKey, inDB->keySize, 1, inDB->file );
                if( numWritten != 1 ) {
                    return -1;
                    }
                }
                
            // else already seeked and read key of non-empty record
            // ready to write value
            int numWritten = fwrite( inOutValue, inDB->valueSize, 1, 
                                     inDB->file );
                
            if( numWritten != 1 ) {
                return -1;
                }
            return 0;
            }
        else {
            // we don't need to seek here
            // we know (!emptyRec), so we already seeked and
            // read the key above
            // ready to read value now
                
            int numRead = fread( inOutValue, inDB->valueSize, 1, 
                                 inDB->file );
            
            if( numRead != 1 ) {
                return -1;
                }
            return 0;
            }
        }
    
    // rec full but didn't match
    return 2;
    }





int LINEARDB3_getOrPut( LINEARDB3 *inDB, const void *inKey, void *inOutValue,
                        char inPut, char inWriteDataFile ) {

    uint32_t fingerprint;

    uint64_t binNumber = getBinNumber( inDB, inKey, &fingerprint );

    
    uint32_t nextFileIndex = inDB->numRecords;
    
    int overflowDepth = 0;


    for( int i=0; i<LDB3_RECORDS_PER_BUCKET; i++ ) {

        int result = LINEARDB3_considerFingerprintBucket(
            inDB, inKey, inOutValue,
            fingerprint,
            inPut, inWriteDataFile,
            &( inDB->fingerprintMap[ binNumber ] ), 
            i );
        
        if( result < 2 ) {
            return result;
            }
        // 2 means record didn't match, keep going
        }

    
    FingerprintBucket3 *thisBucket = &( inDB->fingerprintMap[ binNumber ] );
    char thisBucketIsOverflow = false;
    uint32_t thisBucketIndex = 0;
    
    while( thisBucket->overflowIndex > 0 ) {
        // consider overflow
        overflowDepth++;

        if( overflowDepth > inDB->maxOverflowDepth ) {
            inDB->maxOverflowDepth = overflowDepth;
            }
        
        
        thisBucketIndex = thisBucket->overflowIndex;
        
        thisBucket = 
            &( inDB->overflowFingerprintBuckets[ thisBucketIndex ] );
        
        thisBucketIsOverflow = true;

        for( int i=0; i<LDB3_RECORDS_PER_BUCKET; i++ ) {


            int result = LINEARDB3_considerFingerprintBucket(
                inDB, inKey, inOutValue,
                fingerprint,
                inPut, inWriteDataFile,
                thisBucket, 
                i );
        
            if( result < 2 ) {
                return result;
                }
            // 2 means record didn't match, keep going
            }
        }

    
    if( inPut && 
        thisBucket->overflowIndex == 0 ) {
            
        // reached end of overflow chain without finding place to put value
        
        // need to make a new overflow bucket

        overflowDepth++;

        if( overflowDepth > inDB->maxOverflowDepth ) {
            inDB->maxOverflowDepth = overflowDepth;
            }
        

        uint32_t overflowIndex;
        
        int result = makeNewOverflowBucket( inDB, fingerprint, 
                                            nextFileIndex,
                                            &overflowIndex );
        if( result == -1 ) {
            return -1;
            }

        if( thisBucketIsOverflow ) {
            // makeNewOverflowBucket may re-allocate, invalidating our
            // pointer
            // get a new pointer
            thisBucket = 
                &( inDB->overflowFingerprintBuckets[ thisBucketIndex ] );
            }

        thisBucket->overflowIndex = overflowIndex;

        inDB->numRecords++;
        
        if( inWriteDataFile ) {

            uint64_t filePosRec = 
                LINEARDB3_HEADER_SIZE +
                nextFileIndex * 
                inDB->recordSizeBytes;

            // go to end of file
            if( fseeko( inDB->file, 0, SEEK_END ) ) {
                return -1;
                }
            
            // make sure it matches where we've documented that
            // the record should go
            if( ftello( inDB->file ) != filePosRec ) {
                return -1;
                }
                
            int numWritten = fwrite( inKey, inDB->keySize, 1, inDB->file );
            
            numWritten += fwrite( inOutValue, inDB->valueSize, 1, inDB->file );
                
            if( numWritten != 2 ) {
                return -1;
                }
            return 0;
            }
        

        return 0;
        }



    // not found
    return 1;
    }



int LINEARDB3_get( LINEARDB3 *inDB, const void *inKey, void *outValue ) {
    return LINEARDB3_getOrPut( inDB, inKey, outValue, false, false );
    }



int LINEARDB3_put( LINEARDB3 *inDB, const void *inKey, const void *inValue ) {
    return LINEARDB3_getOrPut( inDB, inKey, (void *)inValue, true, true );
    // fixme... if over load limit, expand table
    }



void LINEARDB3_Iterator_init( LINEARDB3 *inDB, LINEARDB3_Iterator *inDBi ) {
    inDBi->db = inDB;
    inDBi->nextRecordIndex = 0;
    }




int LINEARDB3_Iterator_next( LINEARDB3_Iterator *inDBi, 
                            void *outKey, void *outValue ) {
    LINEARDB3 *db = inDBi->db;

    
    while( true ) {        
        
        if( inDBi->nextRecordIndex >= db->numRecords ) {
            return 0;
            }


        // fseek is needed here to make iterator safe to interleave
        // with other calls
        // If iterator calls are not interleaved, this seek should have
        // little impact on performance (seek to current location between
        // reads).
        if( fseeko( db->file, 
                    LINEARDB3_HEADER_SIZE + 
                    inDBi->nextRecordIndex * db->recordSizeBytes, 
                    SEEK_SET ) ) {
            return -1;
            }

        int numRead = fread( outKey, 
                             db->keySize, 1,
                             db->file );
        if( numRead != 1 ) {
            return -1;
            }
        
        numRead = fread( outValue, 
                         db->valueSize, 1,
                         db->file );
        if( numRead != 1 ) {
            return -1;
            }
        
        inDBi->nextRecordIndex++;
        return 1;
        }
    }




unsigned int LINEARDB3_getCurrentSize( LINEARDB3 *inDB ) {
    return inDB->hashTableSizeB;
    }



unsigned int LINEARDB3_getNumRecords( LINEARDB3 *inDB ) {
    return inDB->numRecords;
    }




unsigned int LINEARDB3_getShrinkSize( LINEARDB3 *inDB,
                                      unsigned int inNewNumRecords ) {

    unsigned int curSize = inDB->hashTableSizeA;
    if( inDB->hashTableSizeA != inDB->hashTableSizeB ) {
        // use doubled size as cur size
        // it's big enough to contain current record load without
        // violating max load factor
        curSize *= 2;
        }
    
    
    if( inNewNumRecords >= curSize ) {
        // can't shrink
        return curSize;
        }
    

    unsigned int minSize = lrint( ceil( inNewNumRecords / inDB->maxLoad ) );
    
    

    // power of 2 that divides curSize and produces new size that is 
    // large enough for minSize
    unsigned int divisor = 1;
    
    while( true ) {
        unsigned int newDivisor = divisor * 2;
        
        if( curSize % newDivisor == 0 &&
            curSize / newDivisor >= minSize ) {
            
            divisor = newDivisor;
            }
        else {
            // divisor as large as it can be
            break;
            }
        }
    
    return curSize / divisor;
    }
