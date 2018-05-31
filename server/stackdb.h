

#include <stdint.h>
#include <stdio.h>


typedef struct {
        unsigned int hashTableSize;
        unsigned int keySize;
        unsigned int valueSize;
        FILE *file;
        uint64_t lastHashBinLoc;
        uint64_t lastValueLoc;
        
        char lastWasQuickMiss;
        
        // each hash bin contains the most recently missed key for
        // that bin and a 64-bit file location for the top of the stack
        unsigned int hashBinSize;
        uint8_t *hashBinBuffer;
    } STACKDB;

    


/**
 * Open database
 *
 * The three _size parameters must be specified if the database could
 * be created or re-created. Otherwise an error will occur. If the
 * database already exists, these parameters are ignored and are read
 * from the database. You can check the struture afterwords to see what
 * they were.
 *
 * @param db Database struct
 * @param path Path to file
 * @param inMode is ignored, and always opened in RW-create mode
 *   (left for compatibility with KISSDB api)
 * @param hash_table_size Size of hash table in 64-bit entries (must be >0)
 * @param key_size Size of keys in bytes
 * @param value_size Size of values in bytes
 * @return 0 on success, nonzero on error
 */
int STACKDB_open(
    STACKDB *inDB,
    const char *inPath,
    int inMode,
    unsigned int inHashTableSize,
    unsigned int inKeySize,
    unsigned int inValueSize );

/**
 * Close database
 *
 * @param db Database struct
 */
void STACKDB_close( STACKDB *inDB );

/**
 * Get an entry
 *
 * @param db Database struct
 * @param key Key (key_size bytes)
 * @param vbuf Value buffer (value_size bytes capacity)
 * @return -1 on I/O error, 0 on success, 1 on not found
 */
int STACKDB_get( STACKDB *inDB, const void *inKey, void *outValue );

/**
 * Put an entry (overwriting it if it already exists)
 *
 * In the already-exists case the size of the database file does not
 * change.
 *
 * @param db Database struct
 * @param key Key (key_size bytes)
 * @param value Value (value_size bytes)
 * @return -1 on I/O error, 0 on success
 */
int STACKDB_put( STACKDB *inDB, const void *inKey, const void *inValue );



// version of put where caller guarantees that inKey does not exist
// in database yet (nor has been gotten from DB as a miss previously)
//
// Insertion operation is much faster if we don't need to find an existing
// record for inKey.
int STACKDB_put_new( STACKDB *inDB, const void *inKey, const void *inValue );


/**
 * Cursor used for iterating over all entries in database
 */
typedef struct {
        STACKDB *db;
        unsigned int hashBin;
        uint64_t nextRecordLoc;
} STACKDB_Iterator;

/**
 * Initialize an iterator
 *
 * @param db Database struct
 * @param i Iterator to initialize
 */
void STACKDB_Iterator_init( STACKDB *inDB, STACKDB_Iterator *inDBi );

/**
 * Get the next entry
 *
 * The order of entries returned by iterator is undefined. It depends on
 * how keys hash.
 *
 * @param Database iterator
 * @param kbuf Buffer to fill with next key (key_size bytes)
 * @param vbuf Buffer to fill with next value (value_size bytes)
 * @return 0 if there are no more entries, negative on error, positive if an kbuf/vbuf have been filled
 */
int STACKDB_Iterator_next( STACKDB_Iterator *inDBi, 
                           void *outKey, void *outValue );
