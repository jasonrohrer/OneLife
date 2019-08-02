

#include <stdint.h>
#include <stdio.h>


#define LDB2_RECORDS_PER_BUCKET 8


typedef struct {
        uint16_t fingerprints[ LDB2_RECORDS_PER_BUCKET ];
        
        // false if no overflow
        char overflow;
        // index of another FingerprintBucket in the overflow array
        uint32_t overflowIndex;
    } FingerprintBucket;

    


typedef struct {
        // load above this causes table to expand incrementally
        double maxLoad;
        
        // number of inserted records in database
        uint32_t numRecords;
        

        // for linear hashing table expansion
        // number of slots in base table
        uint32_t hashTableSizeA;

        // number of slots in expanded table
        // when this reaches hashTableSizeA * 2
        // hash table is done with a full round of expansion
        // and hashTableSizeA is set to hashTableSizeB at that point
        uint32_t hashTableSizeB;
        

        unsigned int keySize;
        unsigned int valueSize;

        FILE *file;
        FILE *overflowFile;
        
        // size of fully expanded (hashTableSizeB) table in bytes
        // (size of all records together)
        uint64_t tableSizeBytes;

        unsigned int recordSizeBytes;
        

        unsigned int bucketSizeBytes;
        uint8_t *bucketBuffer;

        unsigned int maxOverflowDepth;


        // sized to ( hashTableSizeA * 2 ) buckets
        FingerprintBucket *fingerprintMap;
        
        
        // dynamically sized
        uint32_t overflowAreaSize;
        FingerprintBucket *overflowFingerprintBuckets;
        

    } LINEARDB2;

    


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
 *   FILE * pointers already set in db struct by previous forceFile call.
 *   If path specified, "o" is appended to create the name of the overflow file.
 * @param inMode is ignored, and always opened in RW-create mode
 *   (left for compatibility with KISSDB api)
 * @param inHashTableStartSize Size of hash table in entries
 *   This is the starting size of the table, which will grow as the table
 *   becomes full.
 * @param key_size Size of keys in bytes
 * @param value_size Size of values in bytes
 * @return 0 on success, nonzero on error
 */
int LINEARDB2_open(
    LINEARDB2 *inDB,
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
void LINEARDB2_close( LINEARDB2 *inDB );



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
void LINEARDB2_setMaxLoad( LINEARDB2 *inDB, double inMaxLoad );



/**
 * Get an entry
 *
 * @param db Database struct
 * @param key Key (key_size bytes)
 * @param vbuf Value buffer (value_size bytes capacity)
 * @return -1 on I/O error, 0 on success, 1 on not found
 */
int LINEARDB2_get( LINEARDB2 *inDB, const void *inKey, void *outValue );



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
int LINEARDB2_put( LINEARDB2 *inDB, const void *inKey, const void *inValue );



/**
 * Cursor used for iterating over all entries in database
 */
typedef struct {
        LINEARDB2 *db;
        // first main file, then overflow
        FILE *nextRecordFile;
        uint32_t nextBucketIndex;
        // index in bucket
        int nextRecordIndex;
} LINEARDB2_Iterator;



/**
 * Initialize an iterator
 *
 * @param db Database struct
 * @param i Iterator to initialize
 */
void LINEARDB2_Iterator_init( LINEARDB2 *inDB, LINEARDB2_Iterator *inDBi );



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
int LINEARDB2_Iterator_next( LINEARDB2_Iterator *inDBi, 
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
unsigned int LINEARDB2_getCurrentSize( LINEARDB2 *inDB );


/**
 * Number of records that have been inserted in the database.
 */
unsigned int LINEARDB2_getNumRecords( LINEARDB2 *inDB );




/**
 * Gets the optimal starting table size, based on an existing inDB, to house 
 * inNewNumRecords.  Pays attention to inDB's set maxLoad.
 * This is useful when iterating through one DB to insert items into a new, 
 * smaller DB.
 *
 * Iteration happens in file order, and if an optimal shrunken database
 * table size is used, the inserts will happen in file order as well.
 *
 * Return value can be used for inHashTableStartSize in LINEARDB2_open.
 */
unsigned int LINEARDB2_getShrinkSize( LINEARDB2 *inDB,
                                      unsigned int inNewNumRecords );





    

/**
 * For the given maxLoad that is currently set, get the maximum
 * file size that the database will fill for the given parameters.
 *
 * If inMaxLoad is  0, default max load is used.
 */
uint64_t LINEARDB2_getMaxFileSize( unsigned int inTableStartSize,
                                   unsigned int inKeySize,
                                   unsigned int inValueSize,
                                   uint32_t inNumRecords,
                                   double inMaxLoad = 0 );



/**
 * Force the files that will be used by the as-of-yet unopened database.
 *
 * Pass NULL for inPath to open call to use the file thus forced.
 *
 * Allows for memory buffered files and other such things.
 *
 * Note that forced files are treated as empty and ready to be rewritten
 * with a new header and new data.
 */
void LINEARDB2_forceFile( LINEARDB2 *inDB,
                          FILE *inFile,
                          FILE *inOverflowFile );
