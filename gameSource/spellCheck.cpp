#include "spellCheck.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/io/file/File.h"
#include "minorGems/system/Time.h"


#include <malloc.h>


// A-Z a-z and '
#define ALPHABET_SIZE 53

typedef struct TrieNode {
        TrieNode *children[ ALPHABET_SIZE ];
        char isEndOfWord;
    } TrieNode;


// maps ASCII characters to 0..52 indices, or -1 if not in alphabet
int indexMap[256];


static TrieNode rootNode;

static int numNodes = 0;



static void freeNodeAndChildren( TrieNode *inNode ) {
    for( int c=0; c<ALPHABET_SIZE; c++ ) {
        if( inNode->children[c] != NULL ) {
            freeNodeAndChildren( inNode->children[c] );
            }
        delete inNode;
        }
    }



static void insertString( TrieNode *inNode, char *inString ) {
    if( inString[0] == '\0' ) {
        inNode->isEndOfWord = true;
        return;
        }
    
    int alphabetIndex = indexMap[ (unsigned char)( inString[0] ) ];
    
    if( alphabetIndex == -1 ) {
        // forbidden character, stop here
        return;
        }
    
    if( inNode->children[ alphabetIndex ] == NULL ) {
        // new node
        TrieNode *newNode = new TrieNode;
        
        numNodes++;
        
        newNode->isEndOfWord = false;
        
        for( int c=0; c<ALPHABET_SIZE; c++ ) {
            newNode->children[c] = NULL;
            }
        
        inNode->children[ alphabetIndex ] = newNode;
        }

    
    // add rest of string to child node
    insertString( inNode->children[ alphabetIndex ], &( inString[1] ) );
    }



// returns NULL if not found, or end of word node if found
static TrieNode *lookupString( TrieNode *inNode, char *inString ) {

    if( inString[0] == '\0' &&
        inNode->isEndOfWord == true ) {

        return inNode;
        }
    
    int alphabetIndex = indexMap[ (unsigned char)( inString[0] ) ];

    if( alphabetIndex == -1 ) {
        // forbidden character, stop here
        return NULL;
        }
    
    if( inNode->children[ alphabetIndex ] == NULL ) {
        return NULL;
        }
    else {
        return lookupString( inNode->children[ alphabetIndex ],
                             &( inString[1] ) );
        }
    }




void initSpellCheck() {
    
    for( int c=0; c<256; c++ ) {
        indexMap[c] = -1;
        
        if( c >= 65 && c <= 90 ) {
            indexMap[c] = c - 65;
            }
        if( c >= 97 && c <= 122 ) {
            indexMap[c] = c - 97 + 26;
            }
        if( c == 39 ) {
            // apostrophe
            indexMap[c] = 52;
            }
        }

    rootNode.isEndOfWord = false;
    
    for( int c=0; c<ALPHABET_SIZE; c++ ) {
        rootNode.children[c] = NULL;
        }

    
    struct mallinfo mi = mallinfo();
    
    printf( "%f MiB allocated before loading dictionary\n",
            mi.uordblks / ( 1024.0 * 1024.0 ) );


    char *dictName = 
        SettingsManager::getStringSetting( "spellingDictionary.ini", 
                                           "us_english_60.txt" );
    
    File dictFile( NULL, dictName );
    
    delete [] dictName;
    
    if( dictFile.exists() ) {
        double startTime = Time::getCurrentTime();
        
        char *listText = dictFile.readFileContents();
        
        printf( "Reading dictionary file took %f sec\n",
                (Time::getCurrentTime() - startTime)*1000 );
        
        if( listText != NULL ) {
            SimpleVector<char *> *tokenizeString( const char *inString );
            
            SimpleVector<char *> *list = tokenizeString( listText );

            for( int i=0; i<list->size(); i++ ) {
                insertString( &rootNode, list->getElementDirect( i ) );
                }
            

            list->deallocateStringElements();
            delete list;

            delete [] listText;

            printf( "Parsing dictionary file took %f ms, making %d nodes\n",
                    (Time::getCurrentTime() - startTime)*1000,
                    numNodes );
            }
        }

    
    mi = mallinfo();
    printf( "%f MiB allocated after loading dictionary\n",
            mi.uordblks / ( 1024.0 * 1024.0 ) );


    double startTime = Time::getCurrentTime();
    for( int i = 0; i<1000000; i++ ) {
        checkWord( (char*)"monster" );
        }
    printf( "Looking up monster 1M times took  %f ms\n",
            (Time::getCurrentTime() - startTime)*1000 );
    

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
    

    }





void freeSpellCheck() {
    for( int c=0; c<ALPHABET_SIZE; c++ ) {
        if( rootNode.children[c] != NULL ) {
            freeNodeAndChildren( rootNode.children[c] );
            rootNode.children[c] = NULL;
            }
        }
    numNodes = 0;
    }




char checkWord( char *inWord ) {
    TrieNode *foundNode = lookupString( &rootNode, inWord );

    if( foundNode != NULL ) {
        return true;
        }

    return false;
    }

