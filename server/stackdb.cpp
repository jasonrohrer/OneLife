#define _FILE_OFFSET_BITS 64

#include "stackdb.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef _WIN32
#define fseeko fseeko64
#define ftello ftello64
#endif


// djb2 hash function
static uint64_t STACKDB_hash( const void *inB, unsigned int inLen ) {
    uint64_t hash = 5381;
    for( unsigned int i=0; i<inLen; i++ ) {
        hash = ((hash << 5) + hash) + (uint64_t)(((const uint8_t *)inB)[i]);
        }
    return hash;
    }



const char *magicString = "Sdb";

// Sdb magic characters plus
// three 32-bit ints
#define STACKDB_HEADER_SIZE 15



int STACKDB_open(
    STACKDB *inDB,
    const char *inPath,
    int inMode,
    unsigned int inHashTableSize,
    unsigned int inKeySize,
    unsigned int inValueSize ) {

    inDB->hashBinBuffer = NULL;
    
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
    
    inDB->hashBinSize = inDB->keySize + sizeof( uint64_t );
    
    inDB->hashBinBuffer = new uint8_t[ inDB->hashBinSize ];
    

    if( fseeko( inDB->file, 0, SEEK_END ) ) {
        fclose( inDB->file );
        inDB->file = NULL;
        return 1;
        }


    unsigned int tableSizeBytes = 
        ( inKeySize + sizeof( uint64_t ) ) * inHashTableSize;

    
    if( ftello( inDB->file ) < STACKDB_HEADER_SIZE ) {
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
        
        // now write empty hash table
        int binSize = inDB->hashBinSize;
        memset( inDB->hashBinBuffer, 0, binSize );

        for( unsigned int i=0; i<inHashTableSize; i++ ) {
            numWritten = fwrite( inDB->hashBinBuffer, binSize, 1, inDB->file );
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
            printf( "Stackdb magic string '%s' not found at start of  "
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
            printf( "Requested stackdb hash table size of %u does not match "
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
            printf( "Requested stackdb key size of %u does not match "
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
            printf( "Requested stackdb value size of %u does not match "
                    "size of %u in file header\n", inValueSize, val32 );
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        

        // got here, header matches
        
        // make sure hash table exists in file
        fseeko( inDB->file, 0, SEEK_END );
        
        if( ftello( inDB->file ) < STACKDB_HEADER_SIZE + tableSizeBytes ) {
            printf( "stackdb file contains correct header but is missing "
                    "hash table.\n" );
            fclose( inDB->file );
            inDB->file = NULL;
            return 1;
            }
        }
    
    return 0;
    }




void STACKDB_close( STACKDB *inDB ) {
    if( inDB->hashBinBuffer != NULL ) {
        delete [] inDB->hashBinBuffer;
        inDB->hashBinBuffer = NULL;
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



// if key found, moves key to top of hash stack
// upon return
// inDB->lastHashBinLoc is set with the file pos of the hash bin.
// inDB->lastValueLoc is set with the file pos of the value.
// inDB->lastWasQuickMiss is set to true if not found and we encountered
//                        the miss at the top of the stack
// 
// returns -1 on error
//          0 if found
//          1 if not found
static int findValue( STACKDB *inDB, const void *inKey, 
                      char inRecordMiss, void *outValue,
                      char inNewForSure ) {

    uint64_t hash = 
        STACKDB_hash( inKey, inDB->keySize )
        %
        (uint64_t)inDB->hashTableSize;
  
    inDB->lastHashBinLoc = 
        STACKDB_HEADER_SIZE + hash * inDB->hashBinSize;


    if( inNewForSure ) {
        inDB->lastWasQuickMiss = false;
        return 1;
        }
    

    fseeko( inDB->file, inDB->lastHashBinLoc, SEEK_SET );

    int numRead = fread( inDB->hashBinBuffer, inDB->keySize, 1, inDB->file );
    
    if( numRead != 1 ) {
        return -1;
        }
    
    if( keyComp( inDB->keySize, inDB->hashBinBuffer, inKey ) ) {
        // key is marked at top of bin stack as known-missing for this bin
        inDB->lastWasQuickMiss = true;
        return 1;
        }
    else {
        inDB->lastWasQuickMiss = false;
        }


    uint64_t val64 = 0;
    
    numRead = fread( &val64, sizeof(uint64_t), 1, inDB->file );
    
    if( numRead != 1 ) {
        return -1;
        }
    
    if( val64 == 0 ) {
        // empty bin

        if( inRecordMiss ) {
            // remeber that this key was a miss
            fseeko( inDB->file, inDB->lastHashBinLoc, SEEK_SET );
            int numWritten = fwrite( inKey, inDB->keySize, 1, inDB->file );
            
            if( numWritten != 1 ) {
                return -1;
                }
            }
        
        return 1;
        }
    
    uint64_t oldTop64 = val64;

    uint64_t lastRecordPointerLoc64 = 0;
    uint64_t thisRecordPointerLoc64 = 0;
    uint64_t thisRecordStart64 = 0;
    uint64_t nextRecordStart64 = 0;
    

    char keyFound = false;

    int stackPos = 0;

    while( ! keyFound ) {
        lastRecordPointerLoc64 = thisRecordPointerLoc64;

        fseeko( inDB->file, val64, SEEK_SET );

        thisRecordStart64 = val64;
        
        numRead = fread( inDB->hashBinBuffer, inDB->keySize, 1, inDB->file );
        if( numRead != 1 ) {
            return -1;
            }
        
        if( keyComp( inDB->keySize, inDB->hashBinBuffer, inKey ) ) {
            keyFound = true;
            }
        else {
            stackPos++;
            }
        
        // compute it ourself instead of ftell
        // thisRecordPointerLoc64 = ftello( inDB->file );
        thisRecordPointerLoc64 = thisRecordStart64 + inDB->keySize;        
        

        numRead = fread( &val64, sizeof(uint64_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            return -1;
            }
        
        nextRecordStart64 = val64;

        if( val64 == 0 && ! keyFound ) {
            // reached end of stack
            
            
            if( inRecordMiss ) {    
                // remeber that this key was a miss
                // we don't need to walk to the bottom of the stack
                // next time we look for it
                fseeko( inDB->file, inDB->lastHashBinLoc, SEEK_SET );
                int numWritten = fwrite( inKey, inDB->keySize, 1, inDB->file );
                
                if( numWritten != 1 ) {
                    return -1;
                    }
                }
            
            return 1;
            }
        }
    
    
    inDB->lastValueLoc = ftello( inDB->file );

    if( outValue != NULL ) {
        
        numRead = fread( outValue, inDB->valueSize, 1, inDB->file );
    
        if( numRead != 1 ) {
            return -1;
            }
        }

    

    int numWritten;
    
    if( stackPos != 0 ) {
        // move to top of stack.

        if( lastRecordPointerLoc64 != 0 ) {
            // patch hole
            fseeko( inDB->file, lastRecordPointerLoc64, SEEK_SET );
            
            numWritten = 
                fwrite( &nextRecordStart64, sizeof(uint64_t), 1, inDB->file );
            if( numWritten != 1 ) {
                return -1;
                }
            }
        
        // add direct pointer to this record in hash table
        // pointer to top of stack comes after recently-missed key in hash bin
        fseeko( inDB->file, inDB->lastHashBinLoc + inDB->keySize, SEEK_SET );
        numWritten = fwrite( &thisRecordStart64, 
                             sizeof(uint64_t), 1, inDB->file );
        if( numWritten != 1 ) {
            return -1;
            }
        
        // patch this record to point to old top of stack
        fseeko( inDB->file, thisRecordPointerLoc64, SEEK_SET );
        numWritten = fwrite( &oldTop64, sizeof(uint64_t), 1, inDB->file );
        if( numWritten != 1 ) {
            return -1;
            }
        }
    
    return 0;
    }




int STACKDB_get( STACKDB *inDB, const void *inKey, void *outValue ) {
    int result = findValue( inDB, inKey, true, outValue, false );

    return result;
    }





static int STACKDB_put_internal( STACKDB *inDB, 
                                 const void *inKey, const void *inValue,
                                 char inNew ) {
    // don't spend time recording miss
    // we're inserting, so miss will be overwritten anyway
    int result = findValue( inDB, inKey, false, NULL, inNew );
    
    int numWritten;

    if( result == -1 ) {
        return -1;
        }
    if( result == 0 ) {
        // hit
        // replace value
        fseeko( inDB->file, inDB->lastValueLoc, SEEK_SET );
        numWritten = fwrite( inValue, inDB->valueSize, 1, inDB->file );
        if( numWritten != 1 ) {
            return -1;
            }
        }
    else if( result == 1 ) {
        // does not exist yet
        // insert at top of stack

        if( inDB->lastWasQuickMiss ) {
            // top of stack miss record matches our key

            // overwrite with 0
            fseeko( inDB->file, inDB->lastHashBinLoc, SEEK_SET );
            
            memset( inDB->hashBinBuffer, 0, inDB->keySize );
            
            int numWritten = fwrite( inDB->hashBinBuffer, inDB->keySize,
                                     1, inDB->file );
            
            if( numWritten != 1 ) {
                return -1;
                }
            }
        else {
            // no matching miss field to clear at top of stack
            
            // jump straight to file pointer
            fseeko( inDB->file, 
                    inDB->lastHashBinLoc + inDB->keySize, SEEK_SET );
            }
        

        uint64_t val64 = 0;
    
        int numRead = fread( &val64, sizeof(uint64_t), 1, inDB->file );
        
        if( numRead != 1 ) {
            return -1;
            }
    


        // first, add record at end of file
        fseeko( inDB->file, 0, SEEK_END );
        uint64_t recordLoc64 = ftello( inDB->file );
        
        numWritten = fwrite( inKey, inDB->keySize, 1, inDB->file );
        if( numWritten != 1 ) {
            return -1;
            }
        
        // this record points to old head
        numWritten = fwrite( &val64, sizeof( uint64_t ), 1, inDB->file );
        if( numWritten != 1 ) {
            return -1;
            }
        
        // finally record value
        numWritten = fwrite( inValue, inDB->valueSize, 1, inDB->file );
        if( numWritten != 1 ) {
            return -1;
            }


        // now update hash table to point to new head
        fseeko( inDB->file, inDB->lastHashBinLoc + inDB->keySize, SEEK_SET );

        numWritten = fwrite( &recordLoc64, sizeof( uint64_t ), 1, inDB->file );
        if( numWritten != 1 ) {
            return -1;
            }
        }
    

    return 0;
    }




int STACKDB_put( STACKDB *inDB, const void *inKey, const void *inValue ) {
    return STACKDB_put_internal( inDB, inKey, inValue, false );
    }



int STACKDB_put_new( STACKDB *inDB, const void *inKey, const void *inValue ) {
    return STACKDB_put_internal( inDB, inKey, inValue, true );
    }





void STACKDB_Iterator_init( STACKDB *inDB, STACKDB_Iterator *inDBi ) {
    inDBi->db = inDB;
    inDBi->hashBin = 0;
    inDBi->nextRecordLoc = 0;
    }



int STACKDB_Iterator_next( STACKDB_Iterator *inDBi, 
                           void *outKey, void *outValue ) {
    
    FILE *f = inDBi->db->file;
    
    uint64_t val64 = 0;

    char foundFullBin = false;
    
    int numRead;

    int binSize = inDBi->db->hashBinSize;
    int keySize = inDBi->db->keySize;


    if( inDBi->nextRecordLoc == 0 ) {
        
        while( !foundFullBin && inDBi->hashBin < inDBi->db->hashTableSize ) {

            // skip right to file pointer (ignore key in bin)
            fseeko( f, 
                    STACKDB_HEADER_SIZE + inDBi->hashBin * binSize + keySize, 
                    SEEK_SET );
            
            numRead = fread( &val64, sizeof( uint64_t ), 1, f );
            if( numRead != 1 ) {
                return -1;
                }
            if( val64 != 0 ) {
                foundFullBin = true;
                }
            else {
                inDBi->hashBin ++;
                }
            }
    
    
        if( inDBi->hashBin >= inDBi->db->hashTableSize ) {
            return 0;
            }
        
        // jump to first record in this hash bin
        inDBi->nextRecordLoc = val64;
        }

    
    fseeko( f, inDBi->nextRecordLoc, SEEK_SET );

    // tried combining these into one fread with memcpy to 
    // distribute values after fread, but it doesn't change performance.
    
    numRead = fread( outKey, keySize, 1, f );
    if( numRead != 1 ) {
        return -1;
        }

    numRead = fread( &( inDBi->nextRecordLoc ), sizeof( uint64_t ), 1, f );
    if( numRead != 1 ) {
        return -1;
        }


    numRead = fread( outValue, inDBi->db->valueSize, 1, f );
    if( numRead != 1 ) {
        return -1;
        }

    if( inDBi->nextRecordLoc == 0 ) {
        inDBi->hashBin ++;
        }

    return 1;    
    }




