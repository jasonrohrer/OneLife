#include "spellCheck.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/io/file/File.h"
#include "minorGems/system/Time.h"


#include <malloc.h>


/*

Hash table based dictionary implementation.

This is superior to other options in terms of space and performance.

Initially, I had a Trie implementat, which had the following space/performance
for a 122K word dictionary file:

2.121696 MiB allocated before loading dictionary
Reading dictionary file took 1.999964 sec
Parsing dictionary file took 155.999964 ms, making 280104 nodes
61.958389 MiB allocated after loading dictionary
Looking up monster 1M times took  64.999899 ms
Looking up Andrianampoinimerina 1M times took  184.000080 ms
Test:  monster is in dictionary:  1
Test:  monnnster is in dictionary:  0



The hash table implementation has the following performance:


2.122192 MiB allocated before loading dictionary
Reading dictionary file took 0.999973 sec
Parsing dictionary file took 77.999973 ms, making 122809 nodes
4.790070 MiB allocated after loading dictionary
Looking up monster 1M times took  66.999905 ms
Looking up strawberry 1M times took  70.999979 ms
Looking up Andrianampoinimerina 1M times took  85.999992 ms
Test:  monster is in dictionary:  1
Test:  monnnster is in dictionary:  0

Note that hash table itself takes up roughly 1MiB in addition to this
(bulk allocations don't show up in mallinfo).

Worst case performance for a word that has a 5-collision chain:

Looking up psychiatry's 1M times took  106.000118 ms




*/



//-----------------------------------------------------------------------------
// MurmurHash2, by Austin Appleby

// Note - This code makes a few assumptions about how your machine behaves -

// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4

// And it has a few limitations -

// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.

unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed )
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value

	unsigned int h = seed ^ len;

	// Mix 4 bytes at a time into the hash

	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4)
	{
		unsigned int k = *(unsigned int *)data;

		k *= m; 
		k ^= k >> r; 
		k *= m; 
		
		h *= m; 
		h ^= k;

		data += 4;
		len -= 4;
	}
	
	// Handle the last few bytes of the input array

	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
	        h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
} 





typedef struct HashNode {
        // both NULL for empty nodes
        char *string;
        HashNode *next;
    } HashNode;


// slightly larger than list of words
static int tableSize = 0;
static HashNode *hashTable = NULL;

int numNodes = 0;

int numCollisions = 0;

char *worstWord = (char*)"";
int worstNumSteps = 0;


static void insertString( char *inString ) {

    int key = MurmurHash2( inString, strlen( inString ), 0 ) % tableSize;

    HashNode *node = &( hashTable[ key ] );
    
    if( node->string == NULL ) {
        // empty spot at hit
        node->string = inString;
        numNodes++;
        }
    else {
        // full spot 
        int step = 0;
        
        // walk chain
        while( node->next != NULL ) {
            //printf( "Collision for %s with %s, step %d\n",
            //        inString, node->string, step );
            node = node->next;
            numCollisions++;
            step++;
            }
        node->next = new HashNode;
        node->next->string = inString;
        node->next->next = NULL;
        numNodes++;

        if( worstNumSteps < step ) {
            worstNumSteps = step;
            worstWord = inString;
            printf( "New worst collision word %d steps %s\n",
                    step, inString );
            }
        }
    }



static char lookupString( char *inString ) {
    
    int key = MurmurHash2( inString, strlen( inString ), 0 ) % tableSize;
        

    HashNode *node = &( hashTable[ key ] );
    
    if( node->string == NULL ) {
        // empty spot at hit
        return false;
        }
    else if( strcmp( node->string, inString ) == 0 ) {
        // direct hit
        return true;
        }
    else {
        // full spot 

        // walk chain
        while( node->next != NULL ) {
            node = node->next;

            if( strcmp( node->string, inString ) == 0 ) {
                // hit in chain
                return true;
                }
            }
        // walked off end of chain
        return false;
        }
    }



int lastAllocCount = 0;


int getAllocTotal() {
    struct mallinfo mi = mallinfo();
    return mi.uordblks;
    }


int getAllocDelta() {
    int total = getAllocTotal();
    
    int delta = total - lastAllocCount;
    
    lastAllocCount = total;
    
    return delta;
    }





void initSpellCheck() {

    int beforeAlloc = getAllocTotal();
    
    
    getAllocDelta();
    
    

    char *dictName = 
        SettingsManager::getStringSetting( "spellingDictionary.ini", 
                                           "us_english_60.txt" );
    
    File dictFile( NULL, dictName );
    
    delete [] dictName;
    
    if( dictFile.exists() ) {
        double startTime = Time::getCurrentTime();
        
        char *listText = dictFile.readFileContents();
        
        printf( "%d B allocated by loading dict file contents\n",
                getAllocDelta() );

        printf( "Reading dictionary file took %f sec\n",
                (Time::getCurrentTime() - startTime)*1000 );
        
        if( listText != NULL ) {
            SimpleVector<char *> *list = tokenizeString( listText );
            
            printf( "%d B allocated by tokenizing\n", getAllocDelta() );

            
            tableSize = list->size();
            hashTable = new HashNode[ tableSize ];
            
            for( int i=0; i<tableSize; i++ ) {
                hashTable[i].string = NULL;
                hashTable[i].next = NULL;
                }

            printf( "Allocating %f MiB of hash table space\n",
            sizeof( HashNode ) * tableSize / ( 1024.0 * 1024.0 ) );

            printf( "%d B allocated by allocating hash table\n", 
                    getAllocDelta() );


            for( int i=0; i<list->size(); i++ ) {
                insertString( list->getElementDirect( i ) );
                }

            printf( "%d B allocated by inserting words into table\n", 
                    getAllocDelta() );


            delete list;

            delete [] listText;

            printf( "Parsing dictionary file took %f ms, making %d nodes "
                    "with %d collisions\n",
                    (Time::getCurrentTime() - startTime)*1000,
                    numNodes, numCollisions );
            }
        }

    double startTime = Time::getCurrentTime();
    for( int i = 0; i<1000000; i++ ) {
        checkWord( (char*)"monster" );
        }
    printf( "Looking up monster 1M times took  %f ms\n",
            (Time::getCurrentTime() - startTime)*1000 );

    startTime = Time::getCurrentTime();
    for( int i = 0; i<1000000; i++ ) {
        checkWord( (char*)"strawberry" );
        }
    printf( "Looking up strawberry 1M times took  %f ms\n",
            (Time::getCurrentTime() - startTime)*1000 );


    startTime = Time::getCurrentTime();
    for( int i = 0; i<1000000; i++ ) {
        checkWord( worstWord );
        }
    printf( "Looking worst word %s 1M times took  %f ms\n",
            worstWord, (Time::getCurrentTime() - startTime)*1000 );


    startTime = Time::getCurrentTime();
    for( int i = 0; i<1000000; i++ ) {
        checkWord( (char*)"Andrianampoinimerina" );
        }
    printf( "Looking up Andrianampoinimerina 1M times took  %f ms\n",
            (Time::getCurrentTime() - startTime)*1000 );
    
    printf( "Test:  monster is in dictionary:  %d\n",
            checkWord( (char*)"monster" ) );
    printf( "Test:  monnnster is in dictionary:  %d\n",
            checkWord( (char*)"monnnster" ) );
    
    
    printf( "Dictionary total alloc = %d B\n",
            getAllocTotal() - beforeAlloc );
    }





void freeSpellCheck() {
    if( hashTable != NULL ) {
        
        for( int c=0; c<tableSize; c++ ) {
            HashNode *node = &( hashTable[c] );
        
            if( node->string != NULL ) {
                delete [] node->string;
                }

            HashNode *childNode = node->next;
        
            while( childNode != NULL ) {
                if( childNode->string != NULL ) {
                    delete [] childNode->string;
                    }

                HashNode *lastNode = childNode;
            
                childNode = childNode->next;
            
                delete lastNode;
                }

            node->next = NULL;
            }

        delete [] hashTable;
        hashTable = NULL;
        tableSize = 0;
        }
    
    numNodes = 0;
    }




char checkWord( char *inWord ) {
    return lookupString( inWord );
    }

