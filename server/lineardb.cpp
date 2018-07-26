#define _FILE_OFFSET_BITS 64

#include "lineardb.h"

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#define fseeko fseeko64
#define ftello ftello64
#endif


#include "murmurhash2_64.cpp"


// djb2 hash function
static uint64_t djb2( const void *inB, unsigned int inLen ) {
    uint64_t hash = 5381;
    for( unsigned int i=0; i<inLen; i++ ) {
        hash = ((hash << 5) + hash) + (uint64_t)(((const uint8_t *)inB)[i]);
        }
    return hash;
    }

// function used here must have the following signature:
// static uint64_t LINEARDB_hash( const void *inB, unsigned int inLen );
#define LINEARDB_hash(inB, inLen) MurmurHash64( inB, inLen, 0xb9115a39 )

// djb2 is resulting in way fewer collisions in test data
//#define LINEARDB_hash(inB, inLen) djb2( inB, inLen )



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
// three 32-bit ints
#define LINEARDB_HEADER_SIZE 15





int LINEARDB_open(
    LINEARDB *inDB,
    const char *inPath,
    int inMode,
    unsigned int inHashTableSize,
    unsigned int inKeySize,
    unsigned int inValueSize ) {
    
    inDB->recordBuffer = NULL;
    inDB->existenceMap = NULL;
    inDB->fingerprintMap = NULL;
    inDB->maxProbeDepth = 0;
    

    inDB->file = fopen( inPath, "r+b" );
    
    if( inDB->file == NULL ) {
        // doesn't exist yet
        inDB->file = fopen( inPath, "w+b" );
        }
    
    if( inDB->file == NULL ) {
        return 1;
        }

    inDB->hashTableSize = inHashTableSize;
    inDB->keySize = inKeySize;
    inDB->valueSize = inValueSize;
    
    // first byte in record is present flag
    inDB->recordSizeBytes = 1 + inKeySize + inValueSize;
    
    inDB->recordBuffer = new uint8_t[ inDB->recordSizeBytes ];

    inDB->existenceMap = new uint8_t[ ( inHashTableSize / 8 ) + 1 ];
    
    memset( inDB->existenceMap, 0, ( inHashTableSize / 8 ) + 1 );


    inDB->fingerprintMap = new uint16_t[ inHashTableSize ];
    
    memset( inDB->fingerprintMap, 0, inHashTableSize & sizeof( uint16_t ) );


    // does the file already contain a header
    
    // seek to the end to find out file size

    if( fseeko( inDB->file, 0, SEEK_END ) ) {
        fclose( inDB->file );
        inDB->file = NULL;
        return 1;
        }


    // first byte is present flag
    inDB->tableSizeBytes = 
        (uint64_t)( 1 + inKeySize + inValueSize ) * (uint64_t)inHashTableSize;

    
    if( ftello( inDB->file ) < LINEARDB_HEADER_SIZE ) {
        // file that doesn't even contain the header

        // write fresh header and hash table

        // rewrite header
    
        fseeko( inDB->file, 0, SEEK_SET );
        
        

        int numWritten;
        
        numWritten = fwrite( magicString, strlen( magicString ), 
                             1, inDB->file );
        if( numWritten != 1 ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }


        uint32_t val32;

        val32 = inHashTableSize;
        
        numWritten = fwrite( &val32, sizeof(uint32_t), 1, inDB->file );
        if( numWritten != 1 ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        
        val32 = inKeySize;
        
        numWritten = fwrite( &val32, sizeof(uint32_t), 1, inDB->file );
        if( numWritten != 1 ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }


        val32 = inValueSize;
        
        numWritten = fwrite( &val32, sizeof(uint32_t), 1, inDB->file );
        if( numWritten != 1 ) {
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        
        // now write empty hash table, full of 0 values

        unsigned char buff[ 4096 ];
        memset( buff, 0, 4096 );
        
        uint64_t i = 0;
        for( i=0; i < inDB->tableSizeBytes-4096; i += 4096 ) {
            numWritten = fwrite( buff, 4096, 1, inDB->file );
            if( numWritten != 1 ) {
                fclose( inDB->file );
                inDB->file = NULL;
                return 1;
                }
            }
        if( i < inDB->tableSizeBytes ) {
            // last partial buffer
            numWritten = fwrite( buff, inDB->tableSizeBytes - i, 1, 
                                 inDB->file );
            if( numWritten != 1 ) {
                fclose( inDB->file );
                inDB->file = NULL;
                return 1;
                }
            }
        }
    else {
        // read header
        fseeko( inDB->file, 0, SEEK_SET );
        
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
        
        if( val32 != inHashTableSize ) {
            printf( "Requested lineardb hash table size of %u does not match "
                    "size of %u in file header\n", inHashTableSize, val32 );
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
        
        // make sure hash table exists in file
        fseeko( inDB->file, 0, SEEK_END );
        
        if( ftello( inDB->file ) < 
            LINEARDB_HEADER_SIZE + inDB->tableSizeBytes ) {
            
            printf( "lineardb file contains correct header but is missing "
                    "hash table.\n" );
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }


        // now populate existence map
        fseeko( inDB->file, LINEARDB_HEADER_SIZE, SEEK_SET );
        
        for( int i=0; i<inDB->hashTableSize; i++ ) {
            
            fseeko( inDB->file, 
                    LINEARDB_HEADER_SIZE + inDB->recordSizeBytes, SEEK_SET );

            char present = 0;
            int numRead = fread( &present, 1, 1, inDB->file );
            
            if( numRead != 1 ) {
                printf( "Failed to scan hash table from lineardb file\n" );
                fclose( inDB->file );
                inDB->file = NULL;
                return 1;
                }
            if( present ) {
                uint8_t presentFlag = 1 << ( i % 8 );
                
                inDB->existenceMap[ i / 8 ] |= presentFlag;

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



static char exists( LINEARDB *inDB, uint64_t inBinNumber ) {
    return 
        ( inDB->existenceMap[ inBinNumber / 8 ] >> ( inBinNumber % 8 ) ) 
        & 0x01;
    }




static void setExists( LINEARDB *inDB, uint64_t inBinNumber ) {
    
    uint8_t presentFlag = 1 << ( inBinNumber % 8 );
    
    inDB->existenceMap[ inBinNumber / 8 ] |= presentFlag;
    }




// finds file location where value is
// returns 0 if found or 1 if not found, -1 on error
// if inInsert, it will create a new location for inKey and and write
//              the contents of inOutValue into that spot.
// if inInsert is false, it will read the contents of the value into 
//              inOutValue.
static int findValueSpot( LINEARDB *inDB, const void *inKey, 
                          void *inOutValue,
                          char inInsert = false ) {
    
    int probeDepth = 0;
    
    // hash to find first possible bin for inKey
    uint64_t binNumber = 
        LINEARDB_hash( inKey, inDB->keySize ) % 
        (uint64_t)( inDB->hashTableSize );

    
    // linear prob after that
    while( true ) {

        uint64_t binLoc = 
            binNumber * inDB->recordSizeBytes + LINEARDB_HEADER_SIZE;
        
        uint16_t fingerprint = shortHash( inKey, inDB->keySize );
        
        char present = exists( inDB, binNumber );
        
        if( present ) {
            
            if( fingerprint == inDB->fingerprintMap[ binNumber ] ) {
                
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

                    if( inInsert ) {
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
            binNumber++;
            probeDepth ++;
            
            if( probeDepth > inDB->maxProbeDepth ) {
                inDB->maxProbeDepth = probeDepth;
                }

            // wrap around
            if( binNumber >= inDB->hashTableSize ) {
                binNumber -= inDB->hashTableSize;
                }
            }
        else if( inInsert ) {
            // empty bin, insert mode
            
            if( fseeko( inDB->file, binLoc, SEEK_SET ) ) {
                return -1;
                }

            // write present flag
            unsigned char presentFlag = 1;
            
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
            
            // write present flag and key
            
            setExists( inDB, binNumber );
            
            inDB->fingerprintMap[ binNumber ] = fingerprint;
            
            return 0;
            }
        else {
            // empty bin hit, not insert mode
            return 1;
            }
        }
    
    }



int LINEARDB_get( LINEARDB *inDB, const void *inKey, void *outValue ) {
    return findValueSpot( inDB, inKey, outValue, false );
    }



int LINEARDB_put( LINEARDB *inDB, const void *inKey, const void *inValue ) {
    return findValueSpot( inDB, inKey, (void*)inValue, true );
    }



void LINEARDB_Iterator_init( LINEARDB *inDB, LINEARDB_Iterator *inDBi ) {
    inDBi->db = inDB;
    inDBi->nextRecordLoc = LINEARDB_HEADER_SIZE;
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
            // present
            memcpy( outKey, 
                    &( db->recordBuffer[1] ), 
                    db->keySize );
            
            memcpy( outValue, 
                    &( db->recordBuffer[1 + db->keySize] ), 
                    db->valueSize );
            return 1;
            }
        }
    }

    
