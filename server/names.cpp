#include "names.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/StringTree.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/random/JenkinsRandomSource.h"

#include <stdio.h>
//#include <assert.h>


static SimpleVector<char*> firstNames;
static SimpleVector<int> firstNamesLen;
static SimpleVector<char*> lastNames;
static SimpleVector<int> lastNamesLen;


static StringTree *firstNameTree;
static StringTree *lastNameTree;



static JenkinsRandomSource randSource;

static const char *defaultName = "";


static void readNameFile( const char *inFileName, 
                          SimpleVector<char*> *inNameList,
                          SimpleVector<int> *inNameLenList,
                          StringTree *inNameTree ) {
    
    FILE *nameFile = fopen( inFileName, "r" );
    
    if( nameFile == NULL ) {
        AppLog::errorF( "Failed to open name file %s for reading", inFileName );
        return;
        }
    char buffer[100];
    
    while( fscanf( nameFile, "%99s", buffer ) == 1 ) {
        // read another name
        char *name = stringToUpperCase( buffer );
        inNameList->push_back( name );
        //inNameLenList->push_back( strlen( name ) );
        //inNameTree->insert( name, name );
        }
    }

#include <malloc.h>

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

void initNames() {
    getAllocDelta();
    double startTime = Time::getCurrentTime();
    firstNameTree = new StringTree( false );
    lastNameTree = new StringTree( false );
    
    readNameFile( "firstNames.txt", 
                  &firstNames, &firstNamesLen, firstNameTree );
    
    readNameFile( "lastNames.txt", 
                  &lastNames, &lastNamesLen, lastNameTree );
    printf( "Name init took %f sec\n", Time::getCurrentTime() - startTime );

    int bytes = getAllocDelta();
    printf( "   and consumed %d bytes\n", bytes );

    printf( "Testing names with 100000 lookups\n" );
    startTime = Time::getCurrentTime();
    for( int i = 0; i<100000; i++ ) {
    //for( int i = 0; i<1; i++ ) {
        const char *first = findCloseFirstName( "RO" );
        //printf( "\n" );
        
        const char *last = findCloseLastName( "RO" );
        //printf( "First: %s, Last: %s\n", first, last );
        }
    
    printf( "Lookups took %f sec (%d bytes)\n", 
            Time::getCurrentTime() - startTime, getAllocDelta() );
    }




void freeNames() {
    //delete firstNameTree;
    //delete lastNameTree;

    firstNames.deallocateStringElements();
    lastNames.deallocateStringElements();
    }




static int getSign( int inX ) {
    // found here: https://stackoverflow.com/a/1903975/744011
    return (inX > 0) - (inX < 0);
    }



const char *findCloseName( char *inString, SimpleVector<char*> *inNameList ) {
    char *tempString = stringToUpperCase( inString );
    
    int limit = inNameList->size();

    int jumpSize = limit / 2;
    int offset = jumpSize;
    int lastDiff = 256;
    
    
    

    while( jumpSize > 1 && lastDiff != 0 ) {
        char *testString = inNameList->getElementDirect( offset );
        lastDiff = strcmp( tempString, testString );
        
        if( false)printf( "Jumping %d and "
                          "considering %s at offset %d/%d (diff=%d)\n", 
                          jumpSize, testString, offset, limit, lastDiff );
        
        
        jumpSize /= 2;
        if( jumpSize == 0 ) {
            jumpSize = 1;
            }
        if( lastDiff > 0 ) {
            // further down
            offset += jumpSize;
            if( offset >= limit ) {
                offset = limit - 1;
                }
            }
        else if( lastDiff < 0 ) {
            // further up
            offset -= jumpSize;
            if( offset < 0 ) {
                offset = 0;
                }
            }
        }
    
    if( lastDiff != 0 ) {
        int step = 1;
        if( lastDiff < 0 ) {
            step = -1;
            }
        int nextDiff = lastDiff;
        int lastSign = getSign( lastDiff );
        while( getSign( nextDiff ) == lastSign ) {
            
            offset += step;
            
            if( offset <= 0 ) {
                offset = 0;
                break;
                }
            if( offset >= limit ) {
                offset = limit - 1;
                break;
                }

            char *testString = inNameList->getElementDirect( offset );
            nextDiff = strcmp( tempString, testString );
            if( false )printf( "Stepping %d and "
                               "considering %s at offset %d/%d (diff=%d)\n", 
                               step, testString, offset, limit, nextDiff );
            }
        }
    

    
    delete [] tempString;

    if( lastDiff == 0 ) {
        // exact hit
        return inNameList->getElementDirect( offset );
        }
    else {
        // closest match
        return inNameList->getElementDirect( offset );
        }
    

    return defaultName;
    
    /*
    
    // try prefixes until we get a match
    while( len > 0 && inTree->countMatches( tempString ) == 0 ) {
        len--;
        tempString[ len ] = '\0';
        
        }
    
    if( len == 0 ) {
        delete [] tempString;
        return defaultName;
        }
    
    int numMatches = inTree->countMatches( tempString );
    
    if( numMatches > 20 ) {
        numMatches = 20;
        }

    void **matches = new void*[ numMatches ];
    
    numMatches = inTree->getMatches( tempString, 0, numMatches, matches );
    
    delete [] tempString;
    
    int pick = randSource.getRandomBoundedInt( 0, numMatches - 1 );
    
    const char *pickedName = (char*)( matches[ pick ] );


    delete [] matches;

    return pickedName;
    */
    }



// results destroyed internally when freeNames called
const char *findCloseFirstName( char *inString ) {
    return findCloseName( inString, &firstNames );
    }



const char *findCloseLastName( char *inString ) {
    return findCloseName( inString, &lastNames );
    }
