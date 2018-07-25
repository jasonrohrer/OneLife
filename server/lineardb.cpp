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
//#define LINEARDB_hash(inB, inLen) djb2( inB, inLen )


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
        }
    
    return 0;
    }




void LINEARDB_close( LINEARDB *inDB ) {
    if( inDB->recordBuffer != NULL ) {
        delete [] inDB->recordBuffer;
        inDB->recordBuffer = NULL;
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



// finds file location where value is
// returns 0 if not found
// if inInsert, it will create a new location for inKey and
//              return the location where the value for inKey should go
uint64_t findValueSpot( LINEARDB *inDB, const void *inKey, 
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
    
        if( fseeko( inDB->file, binLoc, SEEK_SET ) ) {
            return 0;
            }
        
        char present;
        
        int numRead = fread( &present, 1, 1, inDB->file );
        if( numRead != 1 ) {
            return 0;
            }
        
        if( present ) {
            int numRead = fread( inDB->recordBuffer, 
                                 inDB->keySize, 1, inDB->file );
            
            if( numRead != 1 ) {
                return 0;
                }
            
            if( keyComp( inDB->keySize, inKey, inDB->recordBuffer ) ) {
                // key match!
                return binLoc + 1 + inDB->keySize;
                }
            else {            
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
            }
        else if( inInsert ) {
            // empty bin, insert mode
            
            if( fseeko( inDB->file, binLoc, SEEK_SET ) ) {
                return 0;
                }
            inDB->recordBuffer[0] = 1;
            
            memcpy( &( inDB->recordBuffer[1] ), inKey, inDB->keySize );
            
            // write present flag and key
            int numWritten = fwrite( inDB->recordBuffer, 1 + inDB->keySize,
                                     1, inDB->file );
            
            if( numWritten != 1 ) {
                return 0;
                }
            return binLoc + 1 + inDB->keySize;
            }
        else {
            // empty bin hit, not insert mode
            return 0;
            }
        }
    
    }



int LINEARDB_get( LINEARDB *inDB, const void *inKey, void *outValue ) {
    uint64_t spot = findValueSpot( inDB, inKey );
    
    if( spot != 0 ) {
        if( fseeko( inDB->file, spot, SEEK_SET ) ) {
            return -1;
            }

        int numRead = fread( outValue, inDB->valueSize, 1, inDB->file );
        
        if( numRead != 1 ) {
            return -1;
            }
        return 0;
        }
    return 1;
    }



int LINEARDB_put( LINEARDB *inDB, const void *inKey, const void *inValue ) {
    uint64_t spot = findValueSpot( inDB, inKey, true );
    
    if( spot != 0 ) {
        if( fseeko( inDB->file, spot, SEEK_SET ) ) {
            return -1;
            }

        int numWritten = fwrite( inValue, inDB->valueSize, 1, inDB->file );
        
        if( numWritten != inDB->valueSize ) {
            return -1;
            }
        return 0;
        }
    return -1;
    }



void LINEARDB_Iterator_init( LINEARDB *inDB, LINEARDB_Iterator *inDBi ) {
    inDBi->db = inDB;
    inDBi->nextRecordLoc = LINEARDB_HEADER_SIZE;
    }




int LINEARDB_Iterator_next( LINEARDB_Iterator *inDBi, 
                            void *outKey, void *outValue ) {
    LINEARDB *db = inDBi->db;
    
    if( inDBi->nextRecordLoc > db->tableSizeBytes + LINEARDB_HEADER_SIZE ) {
        return 0;
        }
    
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

    
