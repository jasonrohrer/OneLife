#define _FILE_OFFSET_BITS 64

#include "lineardb2.h"

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
// static uint64_t LINEARDB2_hash( const void *inB, unsigned int inLen );
// murmur2 seems to have equal performance on real world data
// and it just feels safer than djb2, which must have done well on test
// data for a weird reson
#define LINEARDB2_hash(inB, inLen) MurmurHash64( inB, inLen, 0xb9115a39 )

// djb2 is resulting in way fewer collisions in test data
//#define LINEARDB2_hash(inB, inLen) djb2( inB, inLen )


/*
// computes 8-bit hashing using different method from LINEARDB2_hash
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


// computes 16-bit hashing using different method from LINEARDB2_hash
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





static const char *magicString = "Ld2";

// Ld2 magic characters plus
// four 32-bit ints
#define LINEARDB2_HEADER_SIZE 19






static void recreateMaps( LINEARDB2 *inDB, 
                          unsigned int inOldTableSizeA = 0 ) {
    


    FingerprintBucket *oldFingerprintMap = inDB->fingerprintMap;
    
    inDB->fingerprintMap = new FingerprintBucket[ inDB->hashTableSizeA * 2 ];
    
    memset( inDB->fingerprintMap, 0, 
            inDB->hashTableSizeA * 2 * sizeof( FingerprintBucket ) );


    if( oldFingerprintMap != NULL ) {
        if( inOldTableSizeA > 0 ) {
            memcpy( inDB->fingerprintMap,
                    oldFingerprintMap,
                    inOldTableSizeA * 2 * sizeof( FingerprintBucket ) );
            }
        
        delete [] oldFingerprintMap;
        }

    }






// returns 0 on success, -1 on error
static int writeHeader( LINEARDB2 *inDB ) {
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




// uses inDB->bucketBuffer to recursively read a fingerprint record
// and full overflow chain
//
// returns 0 on success, 1 on failure
static int readFingerprintRecord( LINEARDB2 *inDB, 
                                   FingerprintBucket *inDestBucket ) {
    char foundEmpty = false;
    for( int r=0; r<LDB2_RECORDS_PER_BUCKET; r++ ) {
                    
        uint8_t *rec =
            &( inDB->bucketBuffer[ r * inDB->recordSizeBytes ] );
        
        if( rec[0] ) {
            // present
            inDB->numRecords ++;
            inDestBucket->fingerprints[r] =
                shortHash( &( rec[1] ), inDB->keySize );
            }
        else {
            // empty record slot
            foundEmpty = true;
            break;
            }
        }
                
    if( !foundEmpty ) {
        // consider overflow 
        uint8_t *overflowP = 
            &( inDB->bucketBuffer[ LDB2_RECORDS_PER_BUCKET *
                                   inDB->recordSizeBytes ] );
        // check overflow flag
        if( overflowP[0] ) {
            inDestBucket->overflow = true;
                        
            inDestBucket->overflowIndex = 
                *( (uint32_t*)( &( overflowP[1] ) ) );

            // recurse into overflow chain

            if( fseeko( inDB->overflowFile, 
                        inDestBucket->overflowIndex * inDB->bucketSizeBytes, 
                        SEEK_SET ) ) {
                return 1;
                }
            
            int numRead = fread( inDB->bucketBuffer, inDB->bucketSizeBytes, 1, 
                                 inDB->overflowFile );

            if( numRead != 1 ) {
                return 1;
                }
            
            FingerprintBucket *overflowBucket = 
                &( inDB->overflowFingerprintBuckets[ 
                       inDestBucket->overflowIndex ] );
            
            return readFingerprintRecord( inDB, overflowBucket );
            }
        }

    return 0;
    }



static int getRecordSizeBytes( int inKeySize, int inValueSize ) {
    return 1 + inKeySize + inValueSize;
    }


static int getBucketSizeBytes( int inRecordSizeBytes ) {
    return inRecordSizeBytes * LDB2_RECORDS_PER_BUCKET +
        // overflow flag
        sizeof( uint8_t ) +
        // index in overflow file
        sizeof( uint32_t );
    }



int LINEARDB2_open(
    LINEARDB2 *inDB,
    const char *inPath,
    int inMode,
    unsigned int inHashTableStartSize,
    unsigned int inKeySize,
    unsigned int inValueSize ) {
    
    inDB->bucketBuffer = NULL;
    inDB->fingerprintMap = NULL;
    inDB->maxOverflowDepth = 0;

    inDB->numRecords = 0;
    
    inDB->maxLoad = DEFAULT_MAX_LOAD;
    

    if( inPath != NULL ) {
        inDB->file = fopen( inPath, "r+b" );
    
        if( inDB->file == NULL ) {
            // doesn't exist yet
            inDB->file = fopen( inPath, "w+b" );
            }
        
        if( inDB->file == NULL ) {
            return 1;
            }

        char overflowPath[200];
        sprintf( overflowPath, "%.198s%s", inPath, "o" );
        
        inDB->overflowFile = fopen( overflowPath, "r+b" );
    
        if( inDB->overflowFile == NULL ) {
            // doesn't exist yet
            inDB->overflowFile = fopen( overflowPath, "w+b" );
            }
        
        if( inDB->overflowFile == NULL ) {
            return 1;
            }
        }

    // else file already set by forceFile
    

    inDB->hashTableSizeA = inHashTableStartSize;
    inDB->hashTableSizeB = inHashTableStartSize;
    
    inDB->keySize = inKeySize;
    inDB->valueSize = inValueSize;
    
    // first byte in record is present flag
    inDB->recordSizeBytes = getRecordSizeBytes( inKeySize, inValueSize );

    inDB->bucketSizeBytes = getBucketSizeBytes( inDB->recordSizeBytes );
    
    
    inDB->bucketBuffer = new uint8_t[ inDB->bucketSizeBytes ];



    // does the file already contain a header
    
    // seek to the end to find out file size

    if( fseeko( inDB->file, 0, SEEK_END ) ) {
        return 1;
        }



    
    if( inPath == NULL ||
        ftello( inDB->file ) < LINEARDB2_HEADER_SIZE ) {
        // file that doesn't even contain the header

        // or it's a forced file (which is forced to be empty)
        

        // write fresh header and hash table

        // rewrite header
    
        if( writeHeader( inDB ) != 0 ) {
            return 1;
            }
        
        

        inDB->tableSizeBytes = 
            (uint64_t)( inDB->bucketSizeBytes ) * 
            (uint64_t)( inDB->hashTableSizeB );

        // now write empty hash table, full of 0 values

        unsigned char buff[ 4096 ];
        memset( buff, 0, 4096 );
        
        uint64_t i = 0;
        if( inDB->tableSizeBytes > 4096 ) {    
            for( i=0; i < inDB->tableSizeBytes-4096; i += 4096 ) {
                int numWritten = fwrite( buff, 4096, 1, inDB->file );
                if( numWritten != 1 ) {
                    return 1;
                    }
                }
            }
        
        if( i < inDB->tableSizeBytes ) {
            // last partial buffer
            int numWritten = fwrite( buff, inDB->tableSizeBytes - i, 1, 
                                     inDB->file );
            if( numWritten != 1 ) {
                return 1;
                }
            }

        // empty fingerprint map
        recreateMaps( inDB );

        // empty overflow area
        inDB->overflowAreaSize = MIN_OVERFLOW_SIZE;
        inDB->overflowFingerprintBuckets = 
            new FingerprintBucket[ inDB->overflowAreaSize ];
        
        memset( inDB->overflowFingerprintBuckets, 0, 
                inDB->overflowAreaSize * sizeof( FingerprintBucket ) );
        
        
        // write empty overflow file to match
        memset( inDB->bucketBuffer, 0, inDB->bucketSizeBytes );

        for( int i=0; i<inDB->overflowAreaSize; i++ ) {
            int numWritten = fwrite( inDB->bucketBuffer, inDB->bucketSizeBytes,
                                     1, inDB->overflowFile );
            if( numWritten != 1 ) {
                return 1;
                }
            }
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
            printf( "lineardb2 magic string '%s' not found at start of  "
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
            printf( "lineardb2 hash table base size of %u is larger than "
                    "expanded size of %u in file header\n", 
                    inDB->hashTableSizeA, val32 );
            return 1;
            }


        if( val32 >= inDB->hashTableSizeA * 2 ) {
            printf( "lineardb2 hash table expanded size of %u is 2x or more "
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
            printf( "Requested lineardb2 key size of %u does not match "
                    "size of %u in file header\n", inKeySize, val32 );
            return 1;
            }
        


        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            return 1;
            }
        
        if( val32 != inValueSize ) {
            printf( "Requested lineardb2 value size of %u does not match "
                    "size of %u in file header\n", inValueSize, val32 );
            return 1;
            }
        

        // got here, header matches


        inDB->tableSizeBytes = 
            (uint64_t)( inDB->bucketSizeBytes ) * 
            (uint64_t)( inDB->hashTableSizeB );
        
        // make sure hash table exists in file
        if( fseeko( inDB->file, 0, SEEK_END ) ) {
            return 1;
            }
        
        if( ftello( inDB->file ) < 
            (int64_t)( LINEARDB2_HEADER_SIZE + inDB->tableSizeBytes ) ) {
            
            printf( "lineardb2 file contains correct header but is missing "
                    "hash table.\n" );
            return 1;
            }
        


        recreateMaps( inDB );


        // set up initial overflow area based on overflow file

        if( fseeko( inDB->overflowFile, 0, SEEK_END ) ) {
            return 1;
            }
        
        uint64_t overflowBytes = ftello( inDB->file );
        
        inDB->overflowAreaSize = overflowBytes / inDB->bucketSizeBytes;
        
        if( inDB->overflowAreaSize < MIN_OVERFLOW_SIZE ) {
            printf( "lineardb2 overflow file smaller than minimum size of %d "
                    "buckets.\n", MIN_OVERFLOW_SIZE );
            return 1;
            }
        

        uint64_t wholeBytes = inDB->overflowAreaSize * inDB->bucketSizeBytes;
        
        if( wholeBytes != overflowBytes ) {
            printf( "lineardb2 overflow file not sized to whole number of "
                    "buckets.\n" );
            return 1;
            }
        

        inDB->overflowFingerprintBuckets = 
            new FingerprintBucket[ inDB->overflowAreaSize ];
        
        
        
        // now populate fingerprint map
        if( fseeko( inDB->file, LINEARDB2_HEADER_SIZE, SEEK_SET ) ) {
            return 1;
            }
        
        for( unsigned int i=0; i<inDB->hashTableSizeB; i++ ) {
            
            if( fseeko( inDB->file, 
                        LINEARDB2_HEADER_SIZE + i * inDB->bucketSizeBytes, 
                        SEEK_SET ) ) {
                return 1;
                }
            
            fread( inDB->bucketBuffer, inDB->bucketSizeBytes, 1, inDB->file );

            if( numRead != 1 ) {
                printf( "Failed to scan hash table from lineardb2 file\n" );
                return 1;
                }

            // recursively construct fingerprint map from this record
            // and all linked overflow records
            if( readFingerprintRecord( inDB, &( inDB->fingerprintMap[ i ] ) )
                == 1 ) {
                
                printf( "Failed to scan hash table from lineardb2 file\n" );
                return 1;
                }
            }
        }
    

    


    return 0;
    }




void LINEARDB2_close( LINEARDB2 *inDB ) {
    if( inDB->bucketBuffer != NULL ) {
        delete [] inDB->bucketBuffer;
        inDB->bucketBuffer = NULL;
        }    


    if( inDB->fingerprintMap != NULL ) {
        delete [] inDB->fingerprintMap;
        inDB->fingerprintMap = NULL;
        }    

    if( inDB->file != NULL ) {
        fclose( inDB->file );
        inDB->file = NULL;
        }
    if( inDB->overflowFile != NULL ) {
        fclose( inDB->overflowFile );
        inDB->overflowFile = NULL;
        }
    }




void LINEARDB2_setMaxLoad( LINEARDB2 *inDB, double inMaxLoad ) {
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
static int expandTable( LINEARDB2 *inDB ) {

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




static uint64_t getBinNumber( LINEARDB2 *inDB, const void *inKey ) {
    uint64_t hashVal = LINEARDB2_hash( inKey, inDB->keySize );
    
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








static int makeNewOverflowBucket( LINEARDB2 *inDB, uint16_t inFingerprint,
                                  const void *inKey, const void *inValue,
                                  uint32_t *outOverflowIndex ) {
    char found = false;
    uint32_t foundIndex = 0;
    
    for( uint32_t i=0; i<inDB->overflowAreaSize; i++ ) {
        
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
        FingerprintBucket *oldOverflow = inDB->overflowFingerprintBuckets;
        
        inDB->overflowAreaSize *= 2;
        
        inDB->overflowFingerprintBuckets = 
            new FingerprintBucket[ inDB->overflowAreaSize ];
        
        // zero new space
        memset( inDB->overflowFingerprintBuckets, 0,
                inDB->overflowAreaSize * sizeof( FingerprintBucket ) );
        
        memcpy( inDB->overflowFingerprintBuckets,
                oldOverflow, oldSize * sizeof( FingerprintBucket ) );
        
        delete [] oldOverflow;


        // double space in overflow file as well
        // write 0's in there

        uint32_t extraBuckets = inDB->overflowAreaSize - oldSize;

        if( fseeko( inDB->overflowFile, 0, SEEK_END ) ) {
            return -1;
            }
        
        memset( inDB->bucketBuffer, 0, inDB->bucketSizeBytes );
        
        for( int i=0; i<extraBuckets; i++ ) {
            int numWritten = fwrite( inDB->bucketBuffer, 
                                     inDB->bucketSizeBytes, 1, 
                                     inDB->overflowFile );
            if( numWritten != 1 ) {
                return -1;
                }
            }

        // first bin in extended area
        found = true;
        foundIndex = oldSize;
        }
    
    // fingerprint
    inDB->overflowFingerprintBuckets[ foundIndex ].fingerprints[0] = 
        inFingerprint;
    
    // write in file
    uint64_t fileRecPos = foundIndex * inDB->bucketSizeBytes;
    
    if( fseeko( inDB->overflowFile, fileRecPos, SEEK_SET ) ) {
        return -1;
        }

    uint8_t present = 1;
    
    int numWritten = 0;
    
    numWritten += fwrite( &present, 1, 1, inDB->overflowFile );
    
    numWritten += fwrite( inKey, inDB->keySize, 1, inDB->overflowFile );
    numWritten += fwrite( inValue, inDB->valueSize, 1, inDB->overflowFile );
    
    if( numWritten != 3 ) {
        return -1;
        }
    
    *outOverflowIndex = foundIndex;
    return 0;
    }

        
                           




int LINEARDB2_getOrPut( LINEARDB2 *inDB, const void *inKey, void *inOutValue,
                        char inPut ) {

    uint64_t binNumber = getBinNumber( inDB, inKey );

    uint16_t fingerprint = shortHash( inKey, inDB->keySize );

    int overflowDepth = 0;

    char foundEmpty = false;
    for( int i=0; i<LDB2_RECORDS_PER_BUCKET; i++ ) {
        uint16_t binFP = inDB->fingerprintMap[ binNumber ].fingerprints[ i ];
        
        char emptyRec = false;
        
        if( binFP == 0 ) {
            emptyRec = true;
            
            if( inPut ) {
                // set fingerprint for insert
                binFP = fingerprint;
                inDB->fingerprintMap[ binNumber ].fingerprints[ i ] = 
                    fingerprint;
                
                inDB->numRecords++;
                }
            else {
                

                foundEmpty = true;
                break;
                }
            }
        
        if( binFP == fingerprint ) {
            // hit

            uint64_t filePosRec = 
                LINEARDB2_HEADER_SIZE 
                + binNumber * inDB->bucketSizeBytes
                + i * inDB->recordSizeBytes;


            if( !emptyRec ) {
                // read key to make sure it actually matches
                if( fseeko( inDB->file, filePosRec + 1, SEEK_SET ) ) {
                    return -1;
                    }
                int numRead = fread( inDB->bucketBuffer, inDB->keySize, 1,
                                     inDB->file );
                
                if( numRead != 1 ) {
                    return -1;
                    }
                if( ! keyComp( inDB->keySize, inDB->bucketBuffer, inKey ) ) {
                    // false match on non-empty rec because of fingerprint
                    // collision
                    continue;
                    }
                }

            
            if( inPut ) {
                if( fseeko( inDB->file, filePosRec, SEEK_SET ) ) {
                    return -1;
                    }
                uint8_t present = 1;
                int numWritten = fwrite( &present, 1, 1, inDB->file );
                
                numWritten += fwrite( inKey, inDB->keySize, 1, inDB->file );
                
                numWritten += fwrite( inOutValue, inDB->valueSize, 1, 
                                      inDB->file );
                
                if( numWritten != 3 ) {
                    return -1;
                    }
                return 0;
                }
            else {
                uint64_t filePosVal = filePosRec + 1 + inDB->keySize;
            
                if( fseeko( inDB->file, filePosVal, SEEK_SET ) ) {
                    return -1;
                    }
                int numRead = fread( inOutValue, inDB->valueSize, 1, 
                                     inDB->file );
            
                if( numRead != 1 ) {
                    return -1;
                    }
                return 0;
                }
            }
        }
    
    FingerprintBucket *thisBucket = &( inDB->fingerprintMap[ binNumber ] );
    char thisBucketIsOverflow = false;
    uint32_t thisBucketIndex = 0;
    
    while( !foundEmpty && thisBucket->overflow ) {
        // consider overflow
        overflowDepth++;

        if( overflowDepth > inDB->maxOverflowDepth ) {
            inDB->maxOverflowDepth = overflowDepth;
            }
        
        
        thisBucketIndex = thisBucket->overflowIndex;
        
        thisBucket = 
            &( inDB->overflowFingerprintBuckets[ thisBucketIndex ] );
        
        thisBucketIsOverflow = true;

        for( int i=0; i<LDB2_RECORDS_PER_BUCKET; i++ ) {
            uint16_t binFP = thisBucket->fingerprints[ i ];
            
            char emptyRec = false;
            if( binFP == 0 ) {
                emptyRec = true;
                
                if( inPut ) {
                    // set fingerprint for insert
                    binFP = fingerprint;
                    thisBucket->fingerprints[ i ] = fingerprint;
                    inDB->numRecords++;
                    }
                else {
                    
                    foundEmpty = true;
                    break;
                    }
                }
            
            if( binFP == fingerprint ) {
                // hit (or put in empty bin)

                uint64_t filePosRec = 
                    thisBucketIndex * inDB->bucketSizeBytes
                    + i * inDB->recordSizeBytes;
                
                
                if( !emptyRec ) {
                    // read key to make sure it actually matches
                    if( fseeko( inDB->overflowFile, 
                                filePosRec + 1, SEEK_SET ) ) {
                        return -1;
                        }
                    int numRead = fread( inDB->bucketBuffer, inDB->keySize, 1,
                                         inDB->overflowFile );
                
                    if( numRead != 1 ) {
                        return -1;
                        }
                    if( ! keyComp( inDB->keySize, 
                                   inDB->bucketBuffer, inKey ) ) {
                        // false match on non-empty rec because of fingerprint
                        // collision
                        continue;
                        }
                    }

                if( inPut ) {
                    if( fseeko( inDB->overflowFile, filePosRec, SEEK_SET ) ) {
                        return -1;
                        }
                    uint8_t present = 1;
                    int numWritten = 
                        fwrite( &present, 1, 1, inDB->overflowFile );
                    
                    numWritten += fwrite( inKey, inDB->keySize, 1,
                                          inDB->overflowFile );
                    
                    numWritten += fwrite( inOutValue, inDB->valueSize, 1,
                                          inDB->overflowFile );
                    if( numWritten != 3 ) {
                        return -1;
                        }
                    return 0;
                    }
                else {

                    // pos of value
                    uint64_t filePosVal = filePosRec + 1 + inDB->keySize;
            
                    if( fseeko( inDB->overflowFile, filePosVal, SEEK_SET ) ) {
                        return -1;
                        }
                    int numRead = fread( inOutValue, inDB->valueSize, 1, 
                                         inDB->overflowFile );
            
                    if( numRead != 1 ) {
                        return -1;
                        }
                    return 0;
                    }
                }
            }
        }
    
    if( ! foundEmpty && 
        inPut && 
        ! thisBucket->overflow ) {
            
        // reached end of overflow chain without finding place to put value
        
        // need to make a new overflow bucket

        overflowDepth++;

        if( overflowDepth > inDB->maxOverflowDepth ) {
            inDB->maxOverflowDepth = overflowDepth;
            }
        

        uint32_t overflowIndex;
        
        int result = makeNewOverflowBucket( inDB, fingerprint, 
                                            inKey, inOutValue,
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
        
        thisBucket->overflow = true;

        
        if( thisBucketIsOverflow ) {
            // seek to overflow index
            if( fseeko( inDB->overflowFile, 
                        thisBucketIndex * inDB->bucketSizeBytes +
                        LDB2_RECORDS_PER_BUCKET * inDB->recordSizeBytes, 
                        SEEK_SET ) ) {
                return -1;
                }
            // write it
            int numWritten = fwrite( &( thisBucket->overflowIndex ), 
                                     sizeof( uint32_t ),
                                     1, inDB->overflowFile );
            if( numWritten != 1 ) {
                return -1;
                }
            }
        else {
            // new overflow from primary bucket
            if( fseeko( inDB->file, 
                        LINEARDB2_HEADER_SIZE + 
                        binNumber * inDB->bucketSizeBytes +
                        LDB2_RECORDS_PER_BUCKET * inDB->recordSizeBytes, 
                        SEEK_SET ) ) {
                return -1;
                }
            // write it
            int numWritten = fwrite( &( thisBucket->overflowIndex ), 
                                     sizeof( uint32_t ),
                                     1, inDB->file );
            if( numWritten != 1 ) {
                return -1;
                }
            }

        inDB->numRecords++;
        return 0;
        }



    // not found
    return 1;
    }



int LINEARDB2_get( LINEARDB2 *inDB, const void *inKey, void *outValue ) {
    return LINEARDB2_getOrPut( inDB, inKey, outValue, false );
    }



int LINEARDB2_put( LINEARDB2 *inDB, const void *inKey, const void *inValue ) {
    return LINEARDB2_getOrPut( inDB, inKey, (void *)inValue, true );
    // fixme... if over load limit, expand table
    }



void LINEARDB2_Iterator_init( LINEARDB2 *inDB, LINEARDB2_Iterator *inDBi ) {
    inDBi->db = inDB;

    inDBi->nextRecordFile = inDB->file;
    inDBi->nextBucketIndex = 0;
    inDBi->nextRecordIndex = 0;
    }




int LINEARDB2_Iterator_next( LINEARDB2_Iterator *inDBi, 
                            void *outKey, void *outValue ) {
    LINEARDB2 *db = inDBi->db;

    
    while( true ) {        
        
        if( inDBi->nextRecordIndex >= LDB2_RECORDS_PER_BUCKET ) {
            inDBi->nextBucketIndex++;
            inDBi->nextRecordIndex = 0;
            }

        if( inDBi->nextRecordFile == db->file &&
            inDBi->nextBucketIndex >= db->hashTableSizeB ) {
        
            inDBi->nextRecordFile = db->overflowFile;
            inDBi->nextBucketIndex = 0;
            inDBi->nextRecordIndex = 0;
            }
        
        if( inDBi->nextRecordFile == db->overflowFile &&
            inDBi->nextBucketIndex >= db->overflowAreaSize ) {
            return 0;
            }
        
        uint64_t nextRecordLoc = 0;

        nextRecordLoc = 
            inDBi->nextBucketIndex * db->bucketSizeBytes +
            inDBi->nextRecordIndex * db->recordSizeBytes;
        
        if( inDBi->nextRecordFile == db->file ) {
            nextRecordLoc += LINEARDB2_HEADER_SIZE;
            }
        
        


        // fseek is needed here to make iterator safe to interleave
        // with other calls
        // If iterator calls are not interleaved, this seek should have
        // little impact on performance (seek to current location between
        // reads).
        if( fseeko( inDBi->nextRecordFile, nextRecordLoc, SEEK_SET ) ) {
            return -1;
            }

        uint8_t present = 0;
        
        
        int numRead = fread( &present, 
                             1, 1,
                             inDBi->nextRecordFile );
        if( numRead != 1 ) {
            return -1;
            }
        
        inDBi->nextRecordIndex ++;

        if( present ) {
            numRead = fread( outKey, 
                             db->keySize, 1,
                             inDBi->nextRecordFile );
            if( numRead != 1 ) {
                return -1;
                }
        
            numRead = fread( outValue, 
                             db->valueSize, 1,
                             inDBi->nextRecordFile );
            if( numRead != 1 ) {
                return -1;
                }
            return 1;
            }
        }
    }




unsigned int LINEARDB2_getCurrentSize( LINEARDB2 *inDB ) {
    return inDB->hashTableSizeB;
    }



unsigned int LINEARDB2_getNumRecords( LINEARDB2 *inDB ) {
    return inDB->numRecords;
    }




unsigned int LINEARDB2_getShrinkSize( LINEARDB2 *inDB,
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




uint64_t LINEARDB2_getMaxFileSize( unsigned int inTableStartSize,
                                   unsigned int inKeySize,
                                   unsigned int inValueSize,
                                   uint32_t inNumRecords,
                                   double inMaxLoad ) {
    if( inMaxLoad == 0 ) {
        inMaxLoad = DEFAULT_MAX_LOAD;
        }

    int recordSize = getRecordSizeBytes( inKeySize, inValueSize );
    int bucketSize = getBucketSizeBytes( recordSize );
    
    uint32_t numBuckets = 
        (uint32_t)ceil( inNumRecords / LDB2_RECORDS_PER_BUCKET );
    

    double load = 
        (double)inNumRecords / 
        (double)( inTableStartSize * LDB2_RECORDS_PER_BUCKET );
    
    uint64_t tableSize = inTableStartSize;

    if( load > inMaxLoad ) {
        // too big for original table

        uint64_t tableSizeRec = 
            (uint64_t)( ceil( ( inNumRecords / inMaxLoad ) ) );
        
        tableSize = (uint64_t)ceil( tableSizeRec / LDB2_RECORDS_PER_BUCKET );
        }

    return LINEARDB2_HEADER_SIZE + tableSize * bucketSize;
    }





void LINEARDB2_forceFile( LINEARDB2 *inDB,
                          FILE *inFile,
                          FILE *inOverflowFile ) {    
    inDB->file = inFile;
    inDB->overflowFile = inOverflowFile;
    }

