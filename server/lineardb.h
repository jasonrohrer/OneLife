

#include <stdint.h>
#include <stdio.h>


typedef struct {
        // load above this causes table to expand incrementally
        double maxLoad;
        
        // number of inserted records in database
        unsigned int numRecords;
        

        // for linear hashing table expansion
        // number of slots in base table
        unsigned int hashTableSizeA;

        // number of slots in expanded table
        // when this reaches hashTableSizeA * 2
        // hash table is done with a full round of expansion
        // and hashTableSizeA is set to hashTableSizeB at that point
        unsigned int hashTableSizeB;
        

        unsigned int keySize;
        unsigned int valueSize;
        FILE *file;
        
        // size of fully expanded (hashTableSizeB) table in bytes
        // (size of all records together)
        uint64_t tableSizeBytes;

        unsigned int recordSizeBytes;
        uint8_t *recordBuffer;
        unsigned int maxProbeDepth;

        // sized to ( hashTableSizeA * 2 ) / 8 + 1
        uint8_t *existenceMap;

        // 16 bit hash fingerprints of key in each spot in table
        // we can verify matches (with false positives and no false negatives) 
        // without touching the disk
        // sized to ( hashTableSizeA * 2 ) 16-bit values
        uint16_t *fingerprintMap;
        
    } LINEARDB;

    


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
 * @param path Path to file, or NULL to use previously forced
 *   FILE * already set in db struct by previous forceFile call.
 * @param inMode is ignored, and always opened in RW-create mode
 *   (left for compatibility with KISSDB api)
 * @param inHashTableStartSize Size of hash table in entries
 *   This is the starting size of the table, which will grow as the table
 *   becomes full.
 * @param key_size Size of keys in bytes
 * @param value_size Size of values in bytes
 * @return 0 on success, nonzero on error
 */
int LINEARDB_open(
    LINEARDB *inDB,
    const char *inPath,
    int inMode,
    unsigned int inHashTableStartSize,
    unsigned int inKeySize,
    unsigned int inValueSize );



/**
 * Close database
 *
 * @param db Database struct
 */
void LINEARDB_close( LINEARDB *inDB );



/**
 * Sets max load, (number of elements)/(table size), before table starts
 * to expand incremntally.
 *
 * Defaults to 0.5, which is a good value for the underlying linear probing
 * and linear hashing algorithms.  Higher load factors cause linear probing to 
 * degrade substantially (inserts become expensive due to linear hashing table
 * expansion and the necessity to rehash long chains of cells), while lower
 * load factors increase performance of linear probing, but waste 
 * more space and spread data out more, potentially resulting in slower
 * disk operations.
 *
 * Note that in performance-critical situations, the optimial load factor
 * may vary depending on hardware and other factors.
 *
 * Thus, when in doubt, it may be best to benchmark various load factors
 * on your own hardware and with your own data and your own insert vs read
 * ratio.
 */
void LINEARDB_setMaxLoad( LINEARDB *inDB, double inMaxLoad );



/**
 * Get an entry
 *
 * @param db Database struct
 * @param key Key (key_size bytes)
 * @param vbuf Value buffer (value_size bytes capacity)
 * @return -1 on I/O error, 0 on success, 1 on not found
 */
int LINEARDB_get( LINEARDB *inDB, const void *inKey, void *outValue );



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
int LINEARDB_put( LINEARDB *inDB, const void *inKey, const void *inValue );



/**
 * Cursor used for iterating over all entries in database
 */
typedef struct {
        LINEARDB *db;
        uint64_t nextRecordLoc;
        unsigned int currentRunLength;
} LINEARDB_Iterator;



/**
 * Initialize an iterator
 *
 * @param db Database struct
 * @param i Iterator to initialize
 */
void LINEARDB_Iterator_init( LINEARDB *inDB, LINEARDB_Iterator *inDBi );



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
int LINEARDB_Iterator_next( LINEARDB_Iterator *inDBi, 
                            void *outKey, void *outValue );







/**
 * More advanced functions below.
 *
 * These can be ignored for most usages, which just need open, close,
 * get, put, and iterators.
 */





/**
 * Total number of cells in table, including those added through
 * incremental expansion due to Linear Hashing algorithm.
 */
unsigned int LINEARDB_getCurrentSize( LINEARDB *inDB );


/**
 * Number of records that have been inserted in the database.
 */
unsigned int LINEARDB_getNumRecords( LINEARDB *inDB );




/**
 * Gets the optimal starting table size, based on an existing inDB, to house 
 * inNewNumRecords.  Pays attention to inDB's set maxLoad.
 * This is useful when iterating through one DB to insert items into a new, 
 * smaller DB.
 *
 * Iteration happens in file order, and if an optimal shrunken database
 * table size is used, the inserts will happen in file order as well.
 *
 * Return value can be used for inHashTableStartSize in LINEARDB_open.
 */
unsigned int LINEARDB_getShrinkSize( LINEARDB *inDB,
                                     unsigned int inNewNumRecords );





/**
 * For the given maxLoad that is currently set, get the maximum
 * file size that the database will fill for the given parameters.
 *
 * If inMaxLoad is  0, default max load is used.
 */
uint64_t LINEARDB_getMaxFileSize( unsigned int inTableStartSize,
                                  unsigned int inKeySize,
                                  unsigned int inValueSize,
                                  uint64_t inNumRecords,
                                  double inMaxLoad = 0 );



/**
 * Force the file that will be used by the as-of-yet unopened database.
 *
 * Pass NULL for inPath to open call to use the file thus forced.
 *
 * Allows for memory buffered files and other such things.
 *
 * Note that forced files are treated as empty and ready to be rewritten
 * with a new header and new data.
 */
void LINEARDB_forceFile( LINEARDB *inDB,
                         FILE *inFile );
