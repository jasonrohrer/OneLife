#include "spellCheck.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/io/file/File.h"
#include "minorGems/system/Time.h"


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



static char ready;

// slightly larger than list of words
static int tableSize = 0;
static HashNode *hashTable = NULL;

// all strings in dictionary in one long, \0-delimited block
// avoid allocation overhead of allocating them separately
static int numStrings = 0;
static char *allStrings = NULL;

int numNodes = 0;

int numNewNodes = 0;


// storage space for collisions
// allocate them in one big block instead of separately to avoid
// overhead from allocating them separately
static int numExtraNodes;
static int nextUnusedExtraNode;
static HashNode *extraNodes = NULL;



// maps extended ascii to true/false of whether character is in the dictionary
// other characters are punctuation
static char allowedChars[256];


// returns true on success, false on collision
static char insertString( char *inString ) {

    int key = MurmurHash2( inString, strlen( inString ), 0 ) % tableSize;

    HashNode *node = &( hashTable[ key ] );
    
    if( node->string == NULL ) {
        // empty spot at hit
        node->string = inString;
        numNodes++;
        return true;
        }
    return false;
    }


// must allocate space in extraNodes before calling this
static void insertCollisionStirng( char *inString ) {

    int key = MurmurHash2( inString, strlen( inString ), 0 ) % tableSize;

    HashNode *node = &( hashTable[ key ] );
    
    if( node->string != NULL ) {
        // full spot, as expected

        // walk chain
        while( node->next != NULL ) {
            node = node->next;
            }
        node->next = &( extraNodes[nextUnusedExtraNode] );
        nextUnusedExtraNode++;
        numNewNodes++;
        
        node->next->string = inString;
        node->next->next = NULL;
        numNodes++;
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



void initSpellCheck() {
    ready = false;
    
    double startTime = Time::getCurrentTime();

    char *dictName = 
        SettingsManager::getStringSetting( "spellingDictionary.ini", 
                                           "us_english_60.txt" );
    
    File dictFile( NULL, dictName );
    
    delete [] dictName;


    for( int i=0; i<256; i++ ) {
        allowedChars[i] = false;
        }
    
    
    if( dictFile.exists() ) {
        allStrings = dictFile.readFileContents();
        
        if( allStrings != NULL ) {

            int fullLen = strlen( allStrings );
            
            SimpleVector<char*> stringPointers;
            
            // pointer to first string
            stringPointers.push_back( allStrings );
            
            
            for( int i=0; i<fullLen; i++ ) {
                if( allStrings[i] == '\n' ) {
                    allStrings[i] = '\0';
                    
                    if( i < fullLen - 1 ) {
                        if( allStrings[i+1] != '\0' ) {
                            stringPointers.push_back( &( allStrings[i+1] ) );
                            }
                        }
                    }
                else {
                    allowedChars[ (unsigned char)( allStrings[i] ) ] = true;
                    }
                }
        
            numStrings = stringPointers.size();

            tableSize = numStrings;
            hashTable = new HashNode[ tableSize ];
            
            for( int i=0; i<tableSize; i++ ) {
                hashTable[i].string = NULL;
                hashTable[i].next = NULL;
                }

            SimpleVector<char*> failedStrings;
            
            for( int i=0; i<stringPointers.size(); i++ ) {
                char worked =
                    insertString( stringPointers.getElementDirect( i ) );
                
                if( ! worked ) {
                    failedStrings.push_back( 
                        stringPointers.getElementDirect( i ) );
                    }
                }

            numExtraNodes = failedStrings.size();
            extraNodes = new HashNode[ numExtraNodes ];
            nextUnusedExtraNode = 0;            

            for( int i=0; i<failedStrings.size(); i++ ) {
                insertCollisionStirng( failedStrings.getElementDirect( i ) );
                }


            printf( "Parsing dictionary file of %d words took %f ms\n",
                    numStrings, (Time::getCurrentTime() - startTime)*1000 );
            
            ready = true;
            }
        }
    }





void freeSpellCheck() {
    if( hashTable != NULL ) {
        delete [] hashTable;
        hashTable = NULL;
        tableSize = 0;
        }

    numNodes = 0;

    if( extraNodes != NULL ) {
        delete [] extraNodes;
        extraNodes = NULL;
        }
    
    numExtraNodes = 0;
    

    if( allStrings != NULL ) {
        delete [] allStrings;
        }
    
    allStrings = NULL;
    numStrings = 0;
    
    ready = false;
    }



char isSpellCheckReady() {
    return ready;
    }


char checkWord( char *inWord ) {

    char allAllowed = true;
    
    int len = strlen( inWord );
    
    for( int i=0; i<len; i++ ) {
        if( ! allowedChars[ (unsigned char)( inWord[i] ) ] ) {
            allAllowed = false;
            break;
            }
        }
    
    if( allAllowed ) {
        char inDict = lookupString( inWord );

        if( inDict ) {
            return true;
            }
        else if( inWord[0] >= 65 && inWord[0] <= 90 ) {
            // upper case first letter
            char *workingString = stringDuplicate( inWord );
            
            // try lower case
            workingString[0] += 32;
            
            inDict = lookupString( workingString );
            delete [] workingString;
            
            return inDict;
            }
        return false;
        }
    else {
        char *workingString = stringDuplicate( inWord );
        
        // replace forbidded chars with space
        for( int i=0; i<len; i++ ) {
            if( ! allowedChars[ (unsigned char)( workingString[i] ) ] ) {
                workingString[i] = ' ';
                }
            }

        // split into allowed-char runs
        SimpleVector<char*> *tokens = tokenizeString( workingString );
        
        char inDict = true;

        for( int i=0; i<tokens->size(); i++ ) {
            char *token = tokens->getElementDirect( i );
            
            inDict = lookupString( token );
            
            if( ! inDict && token[0] >= 65 && token[0] <= 90 ) {
                // upper case first letter
                
                // try lower case
                token[0] += 32;
            
                inDict = lookupString( token );
                }
            
            if( ! inDict ) {
                break;
                }
            }

        tokens->deallocateStringElements();
        delete tokens;
        
        delete [] workingString;

        return inDict;
        }
    }

