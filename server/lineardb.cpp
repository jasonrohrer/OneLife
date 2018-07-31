#define _FILE_OFFSET_BITS 64

#include "lineardb.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef _WIN32
#define fseeko fseeko64
#define ftello ftello64
#endif


#define DEFAULT_MAX_LOAD 0.5


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
// static uint64_t LINEARDB_hash( const void *inB, unsigned int inLen );
// murmur2 seems to have equal performance on real world data
// and it just feels safer than djb2, which must have done well on test
// data for a weird reson
#define LINEARDB_hash(inB, inLen) MurmurHash64( inB, inLen, 0xb9115a39 )

// djb2 is resulting in way fewer collisions in test data
//#define LINEARDB_hash(inB, inLen) djb2( inB, inLen )


/*
// computes 8-bit hashing using different method from LINEARDB_hash
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


// computes 16-bit hashing using different method from LINEARDB_hash
static uint16_t shortHash( const void *inB, unsigned int inLen ) {
    // use different seed
    uint64_t bigHash = MurmurHash64( inB, inLen, 0x202a025d );
    
    // xor all 2-byte chunks together
    uint16_t smallHash = bigHash & 0xFFFF;
    
    smallHash = smallHash ^ ( ( bigHash >> 16 ) & 0xFFFF );
    smallHash = smallHash ^ ( ( bigHash >> 32 ) & 0xFFFF );
    smallHash = smallHash ^ ( ( bigHash >> 48 ) & 0xFFFF );
    
    return smallHash;
    }





static const char *magicString = "Ldb";

// Ldb magic characters plus
// four 32-bit ints
#define LINEARDB_HEADER_SIZE 19





static void recreateMaps( LINEARDB *inDB, 
                          unsigned int inOldTableSizeA = 0 ) {
    uint8_t *oldExistenceMap = inDB->existenceMap;
    

    inDB->existenceMap = new uint8_t[ inDB->hashTableSizeA * 2 ];
    
    memset( inDB->existenceMap, 0, inDB->hashTableSizeA * 2 );
    

    if( oldExistenceMap != NULL ) {
        if( inOldTableSizeA > 0 ) {
            memcpy( inDB->existenceMap,
                    oldExistenceMap,
                    2 * inOldTableSizeA * sizeof( uint8_t ) );
            
            }
        
        delete [] oldExistenceMap;
        }
    


    uint16_t *oldFingerprintMap = inDB->fingerprintMap;
    
    inDB->fingerprintMap = new uint16_t[ inDB->hashTableSizeA * 2 ];
    
    memset( inDB->fingerprintMap, 0, 
            inDB->hashTableSizeA * 2 * sizeof( uint16_t ) );


    if( oldFingerprintMap != NULL ) {
        if( inOldTableSizeA > 0 ) {
            memcpy( inDB->fingerprintMap,
                    oldFingerprintMap,
                    inOldTableSizeA * 2 * sizeof( uint16_t ) );
            }
        
        delete [] oldFingerprintMap;
        }

    }






static uint64_t getBinLoc( LINEARDB *inDB, uint64_t inBinNumber ) {    
    return inBinNumber * inDB->recordSizeBytes + LINEARDB_HEADER_SIZE;
    }



// returns 0 on success, -1 on error
static int writeHeader( LINEARDB *inDB ) {
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


static void setDeletedFlagFromTableSize( LINEARDB *inDB ) {
    inDB->currentDeletedFlag = 2;
    
    while( pow( 2, inDB->currentDeletedFlag - 2 ) < inDB->hashTableSizeA ) {
        inDB->currentDeletedFlag ++;
        }
    }




int LINEARDB_open(
    LINEARDB *inDB,
    const char *inPath,
    int inMode,
    unsigned int inHashTableStartSize,
    unsigned int inKeySize,
    unsigned int inValueSize ) {
    
    inDB->recordBuffer = NULL;
    inDB->existenceMap = NULL;
    inDB->fingerprintMap = NULL;
    inDB->maxProbeDepth = 0;

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
        }
    // else file already set by forceFile
    

    inDB->hashTableSizeA = inHashTableStartSize;
    inDB->hashTableSizeB = inHashTableStartSize;
    
    inDB->keySize = inKeySize;
    inDB->valueSize = inValueSize;
    
    // first byte in record is present flag
    inDB->recordSizeBytes = 1 + inKeySize + inValueSize;
    
    inDB->recordBuffer = new uint8_t[ inDB->recordSizeBytes ];



    // does the file already contain a header
    
    // seek to the end to find out file size

    if( fseeko( inDB->file, 0, SEEK_END ) ) {
        fclose( inDB->file );
        inDB->file = NULL;
        return 1;
        }



    
    if( inPath == NULL ||
        ftello( inDB->file ) < LINEARDB_HEADER_SIZE ) {
        // file that doesn't even contain the header

        // or it's a forced file (which is forced to be empty)
        

        // write fresh header and hash table

        // rewrite header
    
        if( writeHeader( inDB ) != 0 ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        
        

        inDB->tableSizeBytes = 
            (uint64_t)( inDB->recordSizeBytes ) * 
            (uint64_t)( inDB->hashTableSizeB );

        // now write empty hash table, full of 0 values

        unsigned char buff[ 4096 ];
        memset( buff, 0, 4096 );
        
        uint64_t i = 0;
        if( inDB->tableSizeBytes > 4096 ) {    
            for( i=0; i < inDB->tableSizeBytes-4096; i += 4096 ) {
                int numWritten = fwrite( buff, 4096, 1, inDB->file );
                if( numWritten != 1 ) {
                    fclose( inDB->file );
                    inDB->file = NULL;
                    return 1;
                    }
                }
            }
        
        if( i < inDB->tableSizeBytes ) {
            // last partial buffer
            int numWritten = fwrite( buff, inDB->tableSizeBytes - i, 1, 
                                     inDB->file );
            if( numWritten != 1 ) {
                fclose( inDB->file );
                inDB->file = NULL;
                return 1;
                }
            }

        // empty existence and fingerprint map
        recreateMaps( inDB );

        setDeletedFlagFromTableSize( inDB );
        }
    else {
        // read header
        if( fseeko( inDB->file, 0, SEEK_SET ) ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        
        
        int numRead;
        
        char magicBuffer[ 4 ];
        
        numRead = fread( magicBuffer, 3, 1, inDB->file );

        if( numRead != 1 ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }

        magicBuffer[3] = '\0';
        
        if( strcmp( magicBuffer, magicString ) != 0 ) {
            printf( "lineardb magic string '%s' not found at start of  "
                    "file header\n", magicString );
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        

        uint32_t val32;

        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }        

        // can vary in size from what's been requested
        inDB->hashTableSizeA = val32;

        
        // now read sizeB
        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }

        

        if( val32 < inDB->hashTableSizeA ) {
            printf( "lineardb hash table base size of %u is larger than "
                    "expanded size of %u in file header\n", 
                    inDB->hashTableSizeA, val32 );
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }


        if( val32 >= inDB->hashTableSizeA * 2 ) {
            printf( "lineardb hash table expanded size of %u is 2x or more "
                    "larger than base size of %u in file header\n", 
                    val32, inDB->hashTableSizeA );
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }

        // can vary in size from what's been requested
        inDB->hashTableSizeB = val32;
        

        
        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        
        if( val32 != inKeySize ) {
            printf( "Requested lineardb key size of %u does not match "
                    "size of %u in file header\n", inKeySize, val32 );
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        


        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        
        if( val32 != inValueSize ) {
            printf( "Requested lineardb value size of %u does not match "
                    "size of %u in file header\n", inValueSize, val32 );
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        

        // got here, header matches


        inDB->tableSizeBytes = 
            (uint64_t)( inDB->recordSizeBytes ) * 
            (uint64_t)( inDB->hashTableSizeB );
        
        // make sure hash table exists in file
        if( fseeko( inDB->file, 0, SEEK_END ) ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        
        if( ftello( inDB->file ) < 
            (int64_t)( LINEARDB_HEADER_SIZE + inDB->tableSizeBytes ) ) {
            
            printf( "lineardb file contains correct header but is missing "
                    "hash table.\n" );
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        


        recreateMaps( inDB );
        
        setDeletedFlagFromTableSize( inDB );


        // now populate existence map and fingerprint map
        if( fseeko( inDB->file, LINEARDB_HEADER_SIZE, SEEK_SET ) ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        
        for( unsigned int i=0; i<inDB->hashTableSizeB; i++ ) {
            
            if( fseeko( inDB->file, 
                        LINEARDB_HEADER_SIZE + i * inDB->recordSizeBytes, 
                        SEEK_SET ) ) {
                fclose( inDB->file );
                inDB->file = NULL;
                return 1;
                }

            uint8_t present = 0;
            int numRead = fread( &present, 1, 1, inDB->file );
            
            if( numRead != 1 ) {
                printf( "Failed to scan hash table from lineardb file\n" );
                fclose( inDB->file );
                inDB->file = NULL;
                return 1;
                }
            
            // present flag can be 0, 1, or:
            // currentDeletedFlag 
            // or
            // currentDeletedFlag + 1
            
            inDB->existenceMap[i] = present;
            
            // but no record actually present unless flag == 1
            if( present == 1 ) {
                
                inDB->numRecords ++;
    
                // now read key
                numRead = fread( inDB->recordBuffer, 
                                 inDB->keySize, 1, inDB->file );
            
                if( numRead != 1 ) {
                    printf( "Failed to scan hash table from lineardb file\n" );
                    fclose( inDB->file );
                    inDB->file = NULL;
                    return 1;
                    }
                
                inDB->fingerprintMap[ i ] =
                    shortHash( inDB->recordBuffer, inDB->keySize );
                }
            }
        }
    

    


    return 0;
    }




void LINEARDB_close( LINEARDB *inDB ) {
    if( inDB->recordBuffer != NULL ) {
        delete [] inDB->recordBuffer;
        inDB->recordBuffer = NULL;
        }    

    if( inDB->existenceMap != NULL ) {
        delete [] inDB->existenceMap;
        inDB->existenceMap = NULL;
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




void LINEARDB_setMaxLoad( LINEARDB *inDB, double inMaxLoad ) {
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


// removes a segment of cells from the table, one by one,
// from left to right,
// starting at inFirstBinNumber, and reinserts them
// examines exactly inCellCount cells
// returns 0 on success, -1 on failure
static int reinsertCellSegment( LINEARDB *inDB, uint64_t inFirstBinNumber,
                                uint64_t inCellCount ) {
    
    uint64_t c = inFirstBinNumber;
    
    // don't infinite loop if table is 100% full
    uint64_t numCellsTouched = 0;

    uint8_t deletedFlag = inDB->currentDeletedFlag;
    

    while( numCellsTouched < inDB->hashTableSizeB 
           && 
           numCellsTouched < inCellCount ) {
        
        if( inDB->existenceMap[ c ] == 1 ) {
            
            // a full cell is here

            uint64_t binLoc = getBinLoc( inDB, c );
        
            if( fseeko( inDB->file, binLoc, SEEK_SET ) ) {
                return -1;
                }
        
            int numRead = fread( inDB->recordBuffer, 
                                 inDB->recordSizeBytes, 1, inDB->file );
        
            if( numRead != 1 ) {
                return -1;
                }
        
            // now deal with present byte
            if( fseeko( inDB->file, binLoc, SEEK_SET ) ) {
                return -1;
                }

            // write deleted flag there instead
            int numWritten = fwrite( &deletedFlag, 1, 1, inDB->file );
        
            if( numWritten != 1 ) {
                return -1;
                }
        
        
            inDB->existenceMap[c] = deletedFlag;
        
            // decrease count before reinsert, which will increment count
            inDB->numRecords --;
        
        
            int putResult = 
                LINEARDB_put( inDB,
                              // key
                              &( inDB->recordBuffer[ 1 ] ), 
                              // value
                              &( inDB->recordBuffer[ 1 + inDB->keySize ] ) );
        
            if( putResult != 0 ) {
                return -1;
                }
            }
        
        c++;

        if( c >= inDB->hashTableSizeB ) {
            c -= inDB->hashTableSizeB;
            }
        
        numCellsTouched++;
        }
    
    return 0;
    }




// uses method described here:
// https://en.wikipedia.org/wiki/Linear_hashing
// But adds support for linear probing by potentially rehashing
// all cells hit by a linear probe from the split point
// returns 0 on success, -1 on failure
//
// This call may expand the table by more than one cell, until the table
// is big enough that it's at or below the maxLoad
static int expandTable( LINEARDB *inDB ) {

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
            
            // old deleted flags become stale and irrelevant now
            inDB->currentDeletedFlag++;
            }
        }
    


    // add extra cells at end of the file
    if( fseeko( inDB->file, 0, SEEK_END ) ) {
        return -1;
        }
    memset( inDB->recordBuffer, 0, inDB->recordSizeBytes );

    // always leave NEXT deleted flag value in blank cells that we add to the
    // end of the table
    uint8_t present = inDB->currentDeletedFlag + 1;
    inDB->recordBuffer[0] = present;
    

    for( int c=0; c<numExpanded; c++ ) {    
        int numWritten = 
            fwrite( inDB->recordBuffer, inDB->recordSizeBytes, 1, inDB->file );
        
        if( numWritten != 1 ) {
            return -1;
            }

        // also leave flag in existenceMap
        inDB->existenceMap[ c + inDB->hashTableSizeB - numExpanded ] = present;
        }
    


    // existence and fingerprint maps already 0 for these extra cells
    // (they are big enough already to have room for it at the end)




    // remove and re-insert all cells in the old region
    // between the old and new split point
    

    int result = reinsertCellSegment( inDB, oldSplitPoint, numExpanded );
    if( result == -1 ) {
        return -1;
        }

    
    

    inDB->tableSizeBytes = 
        (uint64_t)( inDB->recordSizeBytes ) * 
        (uint64_t)( inDB->hashTableSizeB );

    // write latest sizes into header
    return writeHeader( inDB );
    }






// returns 0 if found or 1 if not found, -1 on error
// if inPut, it will create a new location for inKey and and write
//              the contents of inOutValue into that spot, or overwrite
//              existing value.
// if inPut is false, it will read the contents of the value into 
//              inOutValue.
static int locateValue( LINEARDB *inDB, const void *inKey, 
                        void *inOutValue,
                        char inPut = false ) {
    
    unsigned int probeDepth = 0;
    
    // hash to find first possible bin for inKey


    uint64_t hashVal = LINEARDB_hash( inKey, inDB->keySize );
    
    uint64_t binNumberA = hashVal % (uint64_t)( inDB->hashTableSizeA );
    
    uint64_t binNumberB = binNumberA;
    


    unsigned int splitPoint = inDB->hashTableSizeB - inDB->hashTableSizeA;
    
    
    if( binNumberA < splitPoint ) {
        // points before split can be mod'ed with double base table size

        // binNumberB will always fit in hashTableSizeB, the expanded table
        binNumberB = hashVal % (uint64_t)( inDB->hashTableSizeA * 2 );
        }
    
    
    uint64_t binNumberBStart = binNumberB;
    
    
    uint16_t fingerprint = shortHash( inKey, inDB->keySize );

    uint64_t binsExamined = 0;
    
    // linear prob after that
    // watch for case where all empty bins have deleted flags
    while( binsExamined < inDB->hashTableSizeB ) {

        uint64_t binLoc = getBinLoc( inDB, binNumberB );
        
        uint8_t present = inDB->existenceMap[ binNumberB ];
        
        binsExamined++;

        // either a record present
        // or a non-stale deleted record flag
        if( present == 1 || present >= inDB->currentDeletedFlag ) {
            
            if( present == 1 &&
                fingerprint == inDB->fingerprintMap[ binNumberB ] ) {
                
                // match in fingerprint, but might be false positive
                
                // check full key on disk too

                // skip present flag to key
                if( fseeko( inDB->file, binLoc + 1, SEEK_SET ) ) {
                    return -1;
                    }        
                
                int numRead = fread( inDB->recordBuffer, 
                                     inDB->keySize, 1, inDB->file );
            
                if( numRead != 1 ) {
                    return -1;
                    }
            
                if( keyComp( inDB->keySize, inKey, inDB->recordBuffer ) ) {
                    // key match!

                    if( inPut ) {
                        // replace value
                        
                        // C99 standard says we must seek after reading
                        // before writing
                        
                        // we're already at the right spot
                        fseeko( inDB->file, 0, SEEK_CUR );
                        
                        int numWritten = 
                            fwrite( inOutValue, inDB->valueSize, 
                                    1, inDB->file );
            
                        if( numWritten != 1 ) {
                            return -1;
                            }
                        
                        // present flag and fingerprint already set
                        
                        return 0;
                        }
                    else {
                        
                        // read value
                        numRead = fread( inOutValue, 
                                         inDB->valueSize, 1, inDB->file );
            
                        if( numRead != 1 ) {
                            return -1;
                            }
                    
                        return 0;
                        }
                    }
                }
            
            // no key match, collision in this bin

            // go on to next bin
            binNumberB++;
            probeDepth ++;
            
            if( probeDepth > inDB->maxProbeDepth ) {
                inDB->maxProbeDepth = probeDepth;
                }

            // wrap around
            if( binNumberB >= inDB->hashTableSizeB ) {
                binNumberB -= inDB->hashTableSizeB;
                }
            }
        else {
            // a truly empty bin hit, and we never found a matching record
            // break out of the loop
            break;
            }
        }
    
    if( ! inPut ) {
        // not found on get
        return 1;
        }

    

    // else put mode, and not found

    // we need to start back at the first bin examined, and try again
    // because we want to insert at the first empty bin OR deleted flag
    // that we encounter along the way
    
    binNumberB = binNumberBStart;
    binsExamined = 0;
    
    // we should never examine too many bins, because table should never
    // be 100% full.  Still, if user specifies a load factor that is 1.0 or 
    // higher, we could get there, and then infinite loop here
    while( binsExamined < inDB->hashTableSizeB ) {
        uint64_t binLoc = getBinLoc( inDB, binNumberB );
        
        uint8_t present = inDB->existenceMap[ binNumberB ];
        
        binsExamined++;
        

        if( present == 1 ) {
            // only skip truly present bins on insert
            binNumberB++;
            
            // wrap around
            if( binNumberB >= inDB->hashTableSizeB ) {
                binNumberB -= inDB->hashTableSizeB;
                }
            }
        else {
            // found an empty bin (either truly empty or a deleted flag)
            
            // write inserted value there

            if( fseeko( inDB->file, binLoc, SEEK_SET ) ) {
                return -1;
                }

            // write present flag
            uint8_t presentFlag = 1;
            
            int numWritten = fwrite( &presentFlag, 1, 1, inDB->file );
            
            if( numWritten != 1 ) {
                return -1;
                }

            // write key
            numWritten = fwrite( inKey, inDB->keySize, 1, inDB->file );
            
            if( numWritten != 1 ) {
                return -1;
                }

            // write value
            numWritten = fwrite( inOutValue, inDB->valueSize, 1, inDB->file );
            
            if( numWritten != 1 ) {
                return -1;
                }
            
            inDB->existenceMap[ binNumberB ] = presentFlag;
            
            inDB->fingerprintMap[ binNumberB ] = fingerprint;

            inDB->numRecords++;
            
            if( (double)( inDB->numRecords ) /
                (double)( inDB->hashTableSizeB ) > inDB->maxLoad ) {
                
                return expandTable( inDB );
                }
            
            return 0;
            }
        }    
    
    // examined all bins without finding empty or deleted?
    return -1;
    }



int LINEARDB_get( LINEARDB *inDB, const void *inKey, void *outValue ) {
    return locateValue( inDB, inKey, outValue, false );
    }



int LINEARDB_put( LINEARDB *inDB, const void *inKey, const void *inValue ) {
    return locateValue( inDB, inKey, (void*)inValue, true );
    }



void LINEARDB_Iterator_init( LINEARDB *inDB, LINEARDB_Iterator *inDBi ) {
    inDBi->db = inDB;
    inDBi->nextRecordLoc = LINEARDB_HEADER_SIZE;
    inDBi->currentRunLength = 0;
    inDB->maxProbeDepth = 0;
    }




int LINEARDB_Iterator_next( LINEARDB_Iterator *inDBi, 
                            void *outKey, void *outValue ) {
    LINEARDB *db = inDBi->db;

    
    while( true ) {        

        if( inDBi->nextRecordLoc > db->tableSizeBytes + LINEARDB_HEADER_SIZE ) {
            return 0;
            }

        // fseek is needed here to make iterator safe to interleave
        // with other calls
        // If iterator calls are not interleaved, this seek should have
        // little impact on performance (seek to current location between
        // reads).
        if( fseeko( db->file, inDBi->nextRecordLoc, SEEK_SET ) ) {
            return -1;
            }

        int numRead = fread( db->recordBuffer, 
                             db->recordSizeBytes, 1,
                             db->file );
        if( numRead != 1 ) {
            return -1;
            }
    
        inDBi->nextRecordLoc += db->recordSizeBytes;

        if( db->recordBuffer[0] ) {
            inDBi->currentRunLength++;
            
            if( inDBi->currentRunLength > db->maxProbeDepth ) {
                db->maxProbeDepth = inDBi->currentRunLength;
                }
            
            // present
            memcpy( outKey, 
                    &( db->recordBuffer[1] ), 
                    db->keySize );
            
            memcpy( outValue, 
                    &( db->recordBuffer[1 + db->keySize] ), 
                    db->valueSize );
            return 1;
            }
        else {
            // empty table cell, run broken
            inDBi->currentRunLength = 0;
            }
        }
    }




unsigned int LINEARDB_getCurrentSize( LINEARDB *inDB ) {
    return inDB->hashTableSizeB;
    }



unsigned int LINEARDB_getNumRecords( LINEARDB *inDB ) {
    return inDB->numRecords;
    }




unsigned int LINEARDB_getShrinkSize( LINEARDB *inDB,
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




uint64_t LINEARDB_getMaxFileSize( unsigned int inTableStartSize,
                                  unsigned int inKeySize,
                                  unsigned int inValueSize,
                                  uint64_t inNumRecords,
                                  double inMaxLoad ) {
    if( inMaxLoad == 0 ) {
        inMaxLoad = DEFAULT_MAX_LOAD;
        }

    uint64_t recordSize = 1 + inKeySize + inValueSize;
    
    double load = (double)inNumRecords / (double)inTableStartSize;
    
    uint64_t tableSize = inTableStartSize;

    if( load > inMaxLoad ) {
        // too big for original table

        tableSize = (uint64_t)( ceil( ( inNumRecords / inMaxLoad ) ) );
        }

    return LINEARDB_HEADER_SIZE + tableSize * recordSize;
    }





void LINEARDB_forceFile( LINEARDB *inDB,
                         FILE *inFile ) {    
    inDB->file = inFile;
    }

