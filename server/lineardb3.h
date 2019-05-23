

// some compilers require this to access UINT64_MAX
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include <stdio.h>


// larger values here reduce RAM overhead per record slightly
// and may speed up lookup in over-full tables, but might slow
// down lookup in less full tables.
#define LINEARDB3_RECORDS_PER_BUCKET 8



typedef struct {
        // index of another FingerprintBucket in the overflow array,
        // or 0 if no overflow (0 overflow bucket never used)
        uint32_t overflowIndex;

        // record number in the data file
        uint32_t fileIndex[ LINEARDB3_RECORDS_PER_BUCKET ];

        // fingerprint mini-hash, mod the largest possible table size
        // in the 32-bit space.  We can mod this with our current table
        // size to find the current bin number (rehashing without
        // actually rehashing the full key)
        uint32_t fingerprints[ LINEARDB3_RECORDS_PER_BUCKET ];
    } LINEARDB3_FingerprintBucket;



#define LINEARDB3_BUCKETS_PER_PAGE 4096

typedef struct {
        LINEARDB3_FingerprintBucket buckets[ LINEARDB3_BUCKETS_PER_PAGE ];
    } LINEARDB3_BucketPage;



typedef struct {
        uint32_t numBuckets;
        
        // number of allocated pages
        uint32_t numPages;
        
        // number of slots in pages pointer array
        // beyond numPages, there are NULL pointers
        uint32_t pageAreaSize;
        LINEARDB3_BucketPage **pages;

        uint32_t firstEmptyBucket;

    } LINEARDB3_PageManager;
    
    

enum LastFileOp{ opRead, opWrite };


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

        // for deciding when fseek is needed between reads and writes
        LastFileOp lastOp;

        // equal to the largest possible 32-bit table size, given
        // our current table size
        // used as mod for computing 32-bit hash fingerprints
        uint32_t fingerprintMod;
        

        unsigned int recordSizeBytes;

        uint8_t *recordBuffer;

        unsigned int maxOverflowDepth;


        
        // sized to hashTableSizeB buckets
        LINEARDB3_PageManager *hashTable;

        LINEARDB3_PageManager *overflowBuckets;
        

    } LINEARDB3;









/**
 * Set maximum table load for all subsequent callst to LINEARDB3_open.
 *
 * Defaults to 0.5.
 *
 * When this load is surpassed, hash table expansion occurs.
 *
 * Lower values waste more RAM on table space but result in slightly higher 
 * performance.
 *
 *
 * Note that a given DB remembers what maxLoad was set when it was opened,
 * and ignores future calls to setMaxLoad.
 *
 * However, the max load is NOT written into the database file format.
 *
 * Thus, data can be loaded into a table with a different load by setting
 * a differen maxLoad before re-opening the file.
 */
void LINEARDB3_setMaxLoad( double inMaxLoad );




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
 * @param path Path to data file.
 * @param inMode is ignored, and always opened in RW-create mode
 *   (left for compatibility with KISSDB api)
 * @param inHashTableStartSize Size of hash table in entries
 *   This is the starting size of the table, which will grow as the table
 *   becomes full.  If less than 2, will be automatically raised to 2.
 * @param key_size Size of keys in bytes
 * @param value_size Size of values in bytes
 * @return 0 on success, nonzero on error
 */
int LINEARDB3_open(
    LINEARDB3 *inDB,
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
void LINEARDB3_close( LINEARDB3 *inDB );



/**
 * Get an entry
 *
 * @param db Database struct
 * @param key Key (key_size bytes)
 * @param vbuf Value buffer (value_size bytes capacity)
 * @return -1 on I/O error, 0 on success, 1 on not found
 */
int LINEARDB3_get( LINEARDB3 *inDB, const void *inKey, void *outValue );



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
int LINEARDB3_put( LINEARDB3 *inDB, const void *inKey, const void *inValue );



/**
 * Cursor used for iterating over all entries in database
 */
typedef struct {
        LINEARDB3 *db;
        uint32_t nextRecordIndex;
} LINEARDB3_Iterator;



/**
 * Initialize an iterator
 *
 * @param db Database struct
 * @param i Iterator to initialize
 */
void LINEARDB3_Iterator_init( LINEARDB3 *inDB, LINEARDB3_Iterator *inDBi );



/**
 * Get the next entry
 *
 * The order of entries returned by iterator is undefined. It depends on
 * how keys hash.
 *
 * @param Database iterator
 * @param kbuf Buffer to fill with next key (key_size bytes)
 * @param vbuf Buffer to fill with next value (value_size bytes)
 * @return 0 if there are no more entries, negative on error, 
 *         positive if an kbuf/vbuf have been filled
 */
int LINEARDB3_Iterator_next( LINEARDB3_Iterator *inDBi, 
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
unsigned int LINEARDB3_getCurrentSize( LINEARDB3 *inDB );


/**
 * Number of records that have been inserted in the database.
 */
unsigned int LINEARDB3_getNumRecords( LINEARDB3 *inDB );




/**
 * Gets optimal starting table size for a given load and number of records.
 *
 * Return value can be used for inHashTableStartSize in LINEARDB3_open.
 */
uint32_t LINEARDB3_getPerfectTableSize( double inMaxLoad, 
                                        uint32_t inNumRecords );



/**
 * Gets the optimal starting table size, based on an existing inDB, to house 
 * inNewNumRecords.  Pays attention to inDB's set maxLoad.
 * This is useful when iterating through one DB to insert items into a new, 
 * smaller DB.
 * 
 * Return value can be used for inHashTableStartSize in LINEARDB3_open.
 */
unsigned int LINEARDB3_getShrinkSize( LINEARDB3 *inDB,
                                      unsigned int inNewNumRecords );
