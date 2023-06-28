#define _FILE_OFFSET_BITS 64

#include "lineardb3.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifdef _WIN32
#define fseeko fseeko64
#define ftello ftello64
#endif



// shorten names for internal code
#define FingerprintBucket LINEARDB3_FingerprintBucket
#define BucketPage LINEARDB3_BucketPage
#define PageManager LINEARDB3_PageManager

#define BUCKETS_PER_PAGE LINEARDB3_BUCKETS_PER_PAGE
#define RECORDS_PER_BUCKET LINEARDB3_RECORDS_PER_BUCKET



static double maxLoadForOpenCalls = 0.5;


void LINEARDB3_setMaxLoad( double inMaxLoad ) {
    maxLoadForOpenCalls = inMaxLoad;
    }




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






// prototypes for page manager
// listed separately for readability

static void initPageManager( PageManager *inPM, uint32_t inNumStartingBuckets );


static void freePageManager( PageManager *inPM );


//  returns pointer to newly created bucket
static FingerprintBucket *addBucket( PageManager *inPM );

// no bounds checking
static FingerprintBucket *getBucket( PageManager *inPM, 
                                     uint32_t inBucketIndex );

// always skips bucket at index 0
// assuming that this call is used for overflowBuckets only, where
// index 0 is used to mark buckets with no further overflow
static uint32_t getFirstEmptyBucketIndex( PageManager *inPM );


static void markBucketEmpty( PageManager *inPM, uint32_t inBucketIndex );






static void initPageManager( PageManager *inPM, 
                             uint32_t inNumStartingBuckets ) {
    inPM->numPages = 1 + inNumStartingBuckets / BUCKETS_PER_PAGE;

    inPM->pageAreaSize = 2 * inPM->numPages;
    
    inPM->pages = new BucketPage*[ inPM->pageAreaSize ];
    
    for( uint32_t i=0; i<inPM->pageAreaSize; i++ ) {
        inPM->pages[i] = NULL;
        }
    
    
    for( uint32_t i=0; i<inPM->numPages; i++ ) {
        inPM->pages[i] = new BucketPage;
        
        memset( inPM->pages[i], 0, sizeof( BucketPage ) );
        }
    
    inPM->numBuckets = inNumStartingBuckets;

    inPM->firstEmptyBucket = 0;
    }



static void freePageManager( PageManager *inPM ) {
    for( uint32_t i=0; i<inPM->numPages; i++ ) {
        delete inPM->pages[i];
        }
    delete [] inPM->pages;

    inPM->pageAreaSize = 0;
    inPM->numPages = 0;
    inPM->numBuckets = 0;
    }



static FingerprintBucket *addBucket( PageManager *inPM ) {
    if( inPM->numPages * BUCKETS_PER_PAGE == inPM->numBuckets ) {
        // need to allocate a new page

        // first make sure there's room    
        if( inPM->numPages == inPM->pageAreaSize ) {
    
            // enlarge page area
            
            uint32_t oldSize = inPM->pageAreaSize;
            
            BucketPage **oldArea = inPM->pages;
            
            
            // double it
            inPM->pageAreaSize = 2 * oldSize;
            
            inPM->pages = new BucketPage*[ inPM->pageAreaSize ];
            
            // NULL just the new slots
            for( uint32_t i=oldSize; i<inPM->pageAreaSize; i++ ) {
                inPM->pages[i] = NULL;
                }
            
            memcpy( inPM->pages, oldArea, oldSize * sizeof( BucketPage* ) );
            
            delete [] oldArea;
            }
        
        // stick new page at end

        inPM->pages[ inPM->numPages ] = new BucketPage;
        
        memset( inPM->pages[ inPM->numPages ], 0, sizeof( BucketPage ) );
        
        inPM->numPages++;
        }


    // room exists, return the empty bucket at end
    FingerprintBucket *newBucket = getBucket( inPM, inPM->numBuckets );
    
    inPM->numBuckets++;
    
    return newBucket;
    }



static FingerprintBucket *getBucket( PageManager *inPM, 
                                     uint32_t inBucketIndex ) {
    uint32_t pageNumber = inBucketIndex / BUCKETS_PER_PAGE;
    uint32_t bucketNumber = inBucketIndex % BUCKETS_PER_PAGE;
    
    return &( inPM->pages[ pageNumber ]->buckets[ bucketNumber ] );
    }



static uint32_t getFirstEmptyBucketIndex( PageManager *inPM ) {
    
    uint32_t firstPage = inPM->firstEmptyBucket / BUCKETS_PER_PAGE;

    uint32_t firstBucket = inPM->firstEmptyBucket % BUCKETS_PER_PAGE;


    for( uint32_t p=firstPage; p<inPM->numPages; p++ ) {
        
        BucketPage *page = inPM->pages[p];

        for( int b=firstBucket; b<BUCKETS_PER_PAGE; b++ ) {
            
            if( page->buckets[b].fingerprints[0] == 0 ) {
                uint32_t index = p * BUCKETS_PER_PAGE + b; 
                
                if( index >= inPM->numBuckets ) {
                    // off end of official list of buckets that we know
                    // about
                    // keep track of this
                    inPM->numBuckets ++;
                    }

                // ignore index 0
                if( index != 0 ) {    
                    // remember for next time
                    // to reduce how many we have to loop through next time
                    inPM->firstEmptyBucket = index;
                    
                    return index;
                    }
                
                }
            }

        // ignore first bucket index after we pass first page
        firstBucket = 0;
        }
    
    // none empty in existing pages
    // create new one off end
    uint32_t newIndex = inPM->numBuckets;
    addBucket( inPM );

    inPM->firstEmptyBucket = newIndex;
    
    return newIndex;
    }



static void markBucketEmpty( PageManager *inPM, uint32_t inBucketIndex ) {
    if( inBucketIndex < inPM->firstEmptyBucket ) {
        inPM->firstEmptyBucket = inBucketIndex;
        }
    }























static const char *magicString = "Ld2";

// Ld2 magic characters plus
// two 32-bit ints
#define LINEARDB3_HEADER_SIZE 11





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




uint32_t LINEARDB3_getPerfectTableSize( double inMaxLoad, 
                                        uint32_t inNumRecords ) {
    
    uint32_t minTableRecords = (uint32_t)ceil( inNumRecords / inMaxLoad );
        
    uint32_t minTableBuckets =(uint32_t)ceil( (double)minTableRecords / 
                                              (double)RECORDS_PER_BUCKET );

    // even if file contains no inserted records
    // use 2 buckets minimum
    if( minTableBuckets < 2 ) {
        minTableBuckets = 2;
        }
    return minTableBuckets;
    }



// if inIgnoreDataFile (which only applies if inPut is true), we completely
// ignore the data file and don't touch it, updating the RAM hash table only,
// and assuming all unique values on collision
int LINEARDB3_getOrPut( LINEARDB3 *inDB, const void *inKey, void *inOutValue,
                        char inPut, char inIgnoreDataFile = false );




int LINEARDB3_open(
    LINEARDB3 *inDB,
    const char *inPath,
    int inMode,
    unsigned int inHashTableStartSize,
    unsigned int inKeySize,
    unsigned int inValueSize ) {
    
    inDB->recordBuffer = NULL;
    inDB->maxOverflowDepth = 0;

    inDB->numRecords = 0;
    
    inDB->maxLoad = maxLoadForOpenCalls;
    

    inDB->file = fopen( inPath, "r+b" );
    
    if( inDB->file == NULL ) {
        // doesn't exist yet
        inDB->file = fopen( inPath, "w+b" );
        }
    
    if( inDB->file == NULL ) {
        return 1;
        }
    
    if( inHashTableStartSize < 2 ) {
        inHashTableStartSize = 2;
        }
    
    inDB->hashTableSizeA = inHashTableStartSize;
    inDB->hashTableSizeB = inHashTableStartSize;
    
    inDB->keySize = inKeySize;
    inDB->valueSize = inValueSize;
    
    // first byte in record is present flag
    inDB->recordSizeBytes = getRecordSizeBytes( inKeySize, inValueSize );
    
    inDB->recordBuffer = new uint8_t[ inDB->recordSizeBytes ];


    recomputeFingerprintMod( inDB );

    inDB->hashTable = new PageManager;
    inDB->overflowBuckets = new PageManager;
    

    // does the file already contain a header
    
    // seek to the end to find out file size

    if( fseeko( inDB->file, 0, SEEK_END ) ) {
        return 1;
        }
    
    
    
    if( ftello( inDB->file ) < LINEARDB3_HEADER_SIZE ) {
        // file that doesn't even contain the header


        // write fresh header and hash table

        // rewrite header
    
        if( writeHeader( inDB ) != 0 ) {
            return 1;
            }
        
        inDB->lastOp = opWrite;
        
        initPageManager( inDB->hashTable, inDB->hashTableSizeA );
        initPageManager( inDB->overflowBuckets, 2 );
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
                    "file header in %s\n", magicString, inPath );
            return 1;
            }
        

        uint32_t val32;
        
        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            return 1;
            }
        
        if( val32 != inKeySize ) {
            printf( "Requested lineardb3 key size of %u does not match "
                    "size of %u in file header in %s\n", inKeySize, val32,
                    inPath );
            return 1;
            }
        


        numRead = fread( &val32, sizeof(uint32_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            return 1;
            }
        
        if( val32 != inValueSize ) {
            printf( "Requested lineardb3 value size of %u does not match "
                    "size of %u in file header in %s\n", inValueSize, val32,
                    inPath );
            return 1;
            }
        

        // got here, header matches


        // make sure hash table exists in file
        if( fseeko( inDB->file, 0, SEEK_END ) ) {
            return 1;
            }
 
        uint64_t fileSize = ftello( inDB->file );


        uint64_t numRecordsInFile = 
            ( fileSize - LINEARDB3_HEADER_SIZE ) / inDB->recordSizeBytes;
        
        uint64_t expectedSize =
            inDB->recordSizeBytes * numRecordsInFile + LINEARDB3_HEADER_SIZE;
        

        if( expectedSize != fileSize ) {
            
            printf( "Requested lineardb3 file %s does not contain a "
                    "whole number of %d-byte records.  "
                    "Assuming final record is garbage and truncating it.\n", 
                    inPath, inDB->recordSizeBytes );
        
            char tempPath[200];
            sprintf( tempPath, "%.190s%s", inPath, ".trunc" );

            FILE *tempFile = fopen( tempPath, "wb" );
            
            if( tempFile == NULL ) {
                printf( "Failed to open temp file %s for truncation\n",
                        tempPath );
                return 1;
                }
            
            if( fseeko( inDB->file, 0, SEEK_SET ) ) {
                return 1;
                }
            
            unsigned char headerBuffer[ LINEARDB3_HEADER_SIZE ];
            
            int numRead = fread( headerBuffer, 
                                 LINEARDB3_HEADER_SIZE, 1, inDB->file );
            
            if( numRead != 1 ) {
                printf( "Failed to read header from lineardb3 file %s\n",
                        inPath );
                return 1;
                }
            int numWritten = fwrite( headerBuffer, 
                                     LINEARDB3_HEADER_SIZE, 1, tempFile );
            
            if( numWritten != 1 ) {
                printf( "Failed to write header to temp lineardb3 "
                        "truncation file %s\n", tempPath );
                return 1;
                }
                

            for( uint64_t i=0; i<numRecordsInFile; i++ ) {
                numRead = fread( inDB->recordBuffer, 
                                 inDB->recordSizeBytes, 1, inDB->file );
            
                if( numRead != 1 ) {
                    printf( "Failed to read record from lineardb3 file %s\n",
                            inPath );
                    return 1;
                    }
                
                numWritten = fwrite( inDB->recordBuffer, 
                                     inDB->recordSizeBytes, 1, tempFile );
            
                if( numWritten != 1 ) {
                    printf( "Failed to record to temp lineardb3 "
                            "truncation file %s\n", tempPath );
                    return 1;
                    }
                }

            fclose( inDB->file );
            fclose( tempFile );
            
            if( rename( tempPath, inPath ) != 0 ) {
                printf( "Failed overwrite lineardb3 file %s with "
                        "truncation file %s\n", inPath, tempPath );
                return 1;
                }

            inDB->file = fopen( inPath, "r+b" );

            if( inDB->file == NULL ) {
                printf( "Failed to re-open lineardb3 file %s after "
                        "trunctation\n", inPath );
                return 1;
                }
            }
        
        
        // now populate hash table

        uint32_t minTableBuckets = 
            LINEARDB3_getPerfectTableSize( inDB->maxLoad,
                                           numRecordsInFile );

        
        inDB->hashTableSizeA = minTableBuckets;
        inDB->hashTableSizeB = minTableBuckets;
        
        
        recomputeFingerprintMod( inDB );

        initPageManager( inDB->hashTable, inDB->hashTableSizeA );
        initPageManager( inDB->overflowBuckets, 2 );


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
            // note that this assumes that each key in the file is unique
            // (it should be, because we generated the file on a previous run)
            int result = 
                LINEARDB3_getOrPut( inDB,
                                    &( inDB->recordBuffer[0] ),
                                    &( inDB->recordBuffer[inDB->keySize] ),
                                    true, 
                                    // ignore data file
                                    // update ram only
                                    // don't even verify keys in data file
                                    // this preserves our fread position
                                    true );
            if( result != 0 ) {
                printf( "Putting lineardb3 record in RAM hash table failed\n" );
                return 1;
                }
            }
        
        inDB->lastOp = opRead;
        }
    

    


    return 0;
    }




void LINEARDB3_close( LINEARDB3 *inDB ) {
    if( inDB->recordBuffer != NULL ) {
        delete [] inDB->recordBuffer;
        inDB->recordBuffer = NULL;
        }    


    freePageManager( inDB->hashTable );
    freePageManager( inDB->overflowBuckets );
    
    delete inDB->hashTable;
    delete inDB->overflowBuckets;
    

    if( inDB->file != NULL ) {
        fclose( inDB->file );
        inDB->file = NULL;
        }
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




static uint64_t getBinNumber( LINEARDB3 *inDB, uint32_t inFingerprint );



typedef struct {
        FingerprintBucket *nextBucket;
        int nextRecord;
    } BucketIterator;

    


// Repeated calls to this function will insert a series of records
// into an empty bucket and any necessary overflow chain buckets
//
// Assumes bucket/record pointed to by iterator is empty and at end of
// chain (assumes this is a fresh series of inserts using an iterator
// that started with an empty bucket).
//
// Updates iterator
static void insertIntoBucket( LINEARDB3 *inDB,
                              BucketIterator *inBucketIterator,
                              uint32_t inFingerprint,
                              uint32_t inFileIndex ) {
    
    if( inBucketIterator->nextRecord == RECORDS_PER_BUCKET ) {
        inBucketIterator->nextBucket->overflowIndex = 
            getFirstEmptyBucketIndex( inDB->overflowBuckets );
        
        inBucketIterator->nextRecord = 0;
        inBucketIterator->nextBucket = 
            getBucket( inDB->overflowBuckets, 
                       inBucketIterator->nextBucket->overflowIndex );
        }
    inBucketIterator->nextBucket->
        fingerprints[ inBucketIterator->nextRecord ] = inFingerprint;
    
    inBucketIterator->nextBucket->
        fileIndex[ inBucketIterator->nextRecord ] = inFileIndex;
    
    inBucketIterator->nextRecord++;
    }

                                            


// uses method described here:
// https://en.wikipedia.org/wiki/Linear_hashing
//
// returns 0 on success, -1 on failure
//
// This call may expand the table by more than one cell, until the table
// is big enough that it's at or below the maxLoad
static int expandTable( LINEARDB3 *inDB ) {
    
    // expand table one cell at a time until we are back at or below maxLoad
    while( (double)( inDB->numRecords ) /
           (double)( inDB->hashTableSizeB * RECORDS_PER_BUCKET ) 
           > inDB->maxLoad ) {


        uint32_t oldSplitPoint = 
            inDB->hashTableSizeB - inDB->hashTableSizeA;


        // we only need to redistribute records from this bucket
        FingerprintBucket *oldBucket =
            getBucket( inDB->hashTable, oldSplitPoint );

        // add one bucket to end of of table
        FingerprintBucket *newBucket = addBucket( inDB->hashTable );

        uint32_t oldBucketIndex = oldSplitPoint;
        uint32_t newBucketIndex = inDB->hashTableSizeB;

        inDB->hashTableSizeB ++;


        BucketIterator oldIter = { oldBucket, 0 };
        BucketIterator newIter = { newBucket, 0 };
        

        // re-hash all cells at old split point, all the way down any
        // overflow chain that's there
        
        FingerprintBucket *nextOldBucket = oldBucket;
        
        while( nextOldBucket != NULL ) {
            // make a working copy 
            FingerprintBucket tempBucket;
            memcpy( &tempBucket, nextOldBucket, sizeof( FingerprintBucket ) );
            
            // clear the real bucket to make room for new inserts
            // this clears the overflow index as well
            memset( nextOldBucket, 0, sizeof( FingerprintBucket ) );
            

            for( int r=0; r<RECORDS_PER_BUCKET; r++ ) {
                uint32_t fingerprint = tempBucket.fingerprints[ r ];
                
                if( fingerprint == 0 ) {
                    // stop at first empty spot hit
                    // remaining records empty too
                    break;
                    }
                uint32_t fileIndex = tempBucket.fileIndex[ r ];
                
                uint64_t newBinNum = getBinNumber( inDB, fingerprint );
                
                BucketIterator *insertIterator;
                
                if( newBinNum == oldBucketIndex ) {
                    insertIterator = &oldIter;
                    }
                else if( newBinNum == newBucketIndex ) {
                    insertIterator = &newIter;
                    }
                else {
                    // rehash doesn't hit either bucket?
                    // should never happen
                    return -1;
                    }
                
                insertIntoBucket( inDB, insertIterator, 
                                  fingerprint, fileIndex );
                }
            
            if( tempBucket.overflowIndex != 0 ) {
                // process next in overflow chain, reinserting those
                // records next
                nextOldBucket = getBucket( inDB->overflowBuckets, 
                                           tempBucket.overflowIndex );
                // we're going to clear it in next iteration of loop, 
                // so report it as empty
                markBucketEmpty( inDB->overflowBuckets,
                                 tempBucket.overflowIndex );
                }
            else {
                // end of chain
                nextOldBucket = NULL;
                }
            }

        
        if( inDB->hashTableSizeB == inDB->hashTableSizeA * 2 ) {
            // full round of expansion is done.
            inDB->hashTableSizeA = inDB->hashTableSizeB;            
            }
        }
    
    return 0;
    }




static uint64_t getBinNumberFromHash( LINEARDB3 *inDB, uint64_t inHashVal ) {

    uint64_t binNumberA = inHashVal % (uint64_t)( inDB->hashTableSizeA );
    
    uint64_t binNumberB = binNumberA;
    


    unsigned int splitPoint = inDB->hashTableSizeB - inDB->hashTableSizeA;
    
    
    if( binNumberA < splitPoint ) {
        // points before split can be mod'ed with double base table size

        // binNumberB will always fit in hashTableSizeB, the expanded table
        binNumberB = inHashVal % (uint64_t)( inDB->hashTableSizeA * 2 );
        }
    return binNumberB;
    }



static uint64_t getBinNumber( LINEARDB3 *inDB, uint32_t inFingerprint ) {
    // fingerprint is such that it will always land in the same bin as
    // full hash val
    return getBinNumberFromHash( inDB, inFingerprint );
    }



static uint64_t getBinNumber( LINEARDB3 *inDB, const void *inKey,
                              uint32_t *outFingerprint ) {
    
    uint64_t hashVal = LINEARDB3_hash( inKey, inDB->keySize );

    *outFingerprint = hashVal % inDB->fingerprintMod;



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
        
        *outFingerprint = hashVal % inDB->fingerprintMod;
        }
    
    
    return getBinNumberFromHash( inDB, hashVal );
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
                                                char inIgnoreDataFile,
                                                FingerprintBucket *inBucket,
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

            if( inIgnoreDataFile ) {
                // finished non-file insert
                return 0;
                }
            }
        else {
            return 1;
            }
        }
        
    if( binFP == inFingerprint ) {
        // hit

        if( inIgnoreDataFile ) {
            // treat any fingerprint match as a collision
            // assume all unique data if we're ignoring the data file
            return 2;
            }
            
        uint64_t filePosRec = 
            LINEARDB3_HEADER_SIZE +
            inBucket->fileIndex[ i ] * 
            inDB->recordSizeBytes;
            
        if( !emptyRec ) {
            
            // read key to make sure it actually matches
            
            // never seek unless we have to
            if( inDB->lastOp == opWrite || 
                ftello( inDB->file ) != (off_t)filePosRec ) {

                if( fseeko( inDB->file, filePosRec, SEEK_SET ) ) {
                    return -1;
                    }
                }
            
            int numRead = fread( inDB->recordBuffer, inDB->keySize, 1,
                                 inDB->file );
            inDB->lastOp = opRead;
    
            if( numRead != 1 ) {
                return -1;
                }
            if( ! keyComp( inDB->keySize, inDB->recordBuffer, inKey ) ) {
                // false match on non-empty rec because of fingerprint
                // collision
                return 2;
                }
            }

            
        if( inPut ) {
            if( emptyRec ) {

                // don't seek unless we have to
                // if we're doing a series of fresh inserts,
                // the file pos is already waiting at the end of the file
                // for us
                if( inDB->lastOp == opRead ||
                    ftello( inDB->file ) != (off_t)filePosRec ) {
                    
                    // no seeking done yet
                    // go to end of file
                    if( fseeko( inDB->file, 0, SEEK_END ) ) {
                        return -1;
                        }
                    // make sure it matches where we've documented that
                    // the record should go
                    if( ftello( inDB->file ) != (off_t)filePosRec ) {
                        return -1;
                        }
                    }
                

                
                int numWritten = 
                    fwrite( inKey, inDB->keySize, 1, inDB->file );
                inDB->lastOp = opWrite;
                
                if( numWritten != 1 ) {
                    return -1;
                    }
                }
                
            // else already seeked and read key of non-empty record
            // ready to write value

            // still need to seek here after reading before writing
            // according to fopen docs
            fseeko( inDB->file, 0, SEEK_CUR );
            
            int numWritten = fwrite( inOutValue, inDB->valueSize, 1, 
                                     inDB->file );
            inDB->lastOp = opWrite;
    
            if( numWritten != 1 ) {
                return -1;
                }
            
            // successful put    
            return 0;
            }
        else {
            // we don't need to seek here
            // we know (!emptyRec), so we already seeked and
            // read the key above
            // ready to read value now
                
            int numRead = fread( inOutValue, inDB->valueSize, 1, 
                                 inDB->file );
            inDB->lastOp = opRead;
            
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
                        char inPut, char inIgnoreDataFile ) {

    uint32_t fingerprint;

    uint64_t binNumber = getBinNumber( inDB, inKey, &fingerprint );

    
    unsigned int overflowDepth = 0;

    FingerprintBucket *thisBucket = getBucket( inDB->hashTable, binNumber );


    char skipToOverflow = false;

    if( inPut && inIgnoreDataFile ) {
        skipToOverflow = true;
        }
    
    if( !skipToOverflow || thisBucket->overflowIndex == 0 )
    for( int i=0; i<RECORDS_PER_BUCKET; i++ ) {

        int result = LINEARDB3_considerFingerprintBucket(
            inDB, inKey, inOutValue,
            fingerprint,
            inPut, inIgnoreDataFile,
            thisBucket, 
            i );
        
        if( result < 2 ) {
            return result;
            }
        // 2 means record didn't match, keep going
        }

    
    uint32_t thisBucketIndex = 0;
    
    while( thisBucket->overflowIndex > 0 ) {
        // consider overflow
        overflowDepth++;

        if( overflowDepth > inDB->maxOverflowDepth ) {
            inDB->maxOverflowDepth = overflowDepth;
            }
        
        
        thisBucketIndex = thisBucket->overflowIndex;
        
        thisBucket = getBucket( inDB->overflowBuckets, thisBucketIndex );

        if( !skipToOverflow || thisBucket->overflowIndex == 0 )
        for( int i=0; i<RECORDS_PER_BUCKET; i++ ) {

            int result = LINEARDB3_considerFingerprintBucket(
                inDB, inKey, inOutValue,
                fingerprint,
                inPut, inIgnoreDataFile,
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
        

        thisBucket->overflowIndex = 
            getFirstEmptyBucketIndex( inDB->overflowBuckets );

        FingerprintBucket *newBucket = getBucket( inDB->overflowBuckets,
                                                  thisBucket->overflowIndex );
        newBucket->fingerprints[0] = fingerprint;

        // will go at end of file
        newBucket->fileIndex[0] = inDB->numRecords;
        
        
        inDB->numRecords++;
        
        if( ! inIgnoreDataFile ) {

            uint64_t filePosRec = 
                LINEARDB3_HEADER_SIZE +
                newBucket->fileIndex[0] * 
                inDB->recordSizeBytes;

            // don't seek unless we have to
            if( inDB->lastOp == opRead ||
                ftello( inDB->file ) != (off_t)filePosRec ) {
            
                // go to end of file
                if( fseeko( inDB->file, 0, SEEK_END ) ) {
                    return -1;
                    }
            
                // make sure it matches where we've documented that
                // the record should go
                if( ftello( inDB->file ) != (off_t)filePosRec ) {
                    return -1;
                    }
                }
            
                
            int numWritten = fwrite( inKey, inDB->keySize, 1, inDB->file );
            inDB->lastOp = opWrite;

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
    int result = LINEARDB3_getOrPut( inDB, inKey, (void *)inValue, 
                                     true, false );

    if( result == -1 ) {
        return result;
        }

    if( inDB->numRecords > 
        ( inDB->hashTableSizeB * RECORDS_PER_BUCKET ) * inDB->maxLoad ) {
        
        result = expandTable( inDB );
        }
    return result;
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
        
        // BUT, don't seek unless we have to
        // even seeking to current location has a performance hit
        
        uint64_t fileRecPos = 
            LINEARDB3_HEADER_SIZE + 
            inDBi->nextRecordIndex * db->recordSizeBytes;
        
                    
        if( db->lastOp == opWrite ||
            ftello( db->file ) != (off_t)fileRecPos ) {
    
            if( fseeko( db->file, fileRecPos, SEEK_SET ) ) {
                return -1;
                }
            }
        

        int numRead = fread( outKey, 
                             db->keySize, 1,
                             db->file );
        db->lastOp = opRead;
        
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

    // perfect size to insert this many with no table expansion
    
    uint32_t minTableBuckets = 
        LINEARDB3_getPerfectTableSize( inDB->maxLoad, inNewNumRecords );

    return minTableBuckets;
    }
