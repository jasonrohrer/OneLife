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



void initNames() {
    firstNameTree = new StringTree( false );
    lastNameTree = new StringTree( false );
    
    readNameFile( "firstNames.txt", 
                  &firstNames, &firstNamesLen, firstNameTree );
    
    readNameFile( "lastNames.txt", 
                  &lastNames, &lastNamesLen, lastNameTree );

    printf( "Testing names with 1000 lookups\n" );
    double startTime = Time::getCurrentTime();
    for( int i = 0; i<1; i++ ) {
        const char *first = findCloseFirstName( "RO" );
        const char *last = findCloseLastName( "RO" );
        printf( "First: %s, Last: %s\n", first, last );
        }
    printf( "Lookups took %f sec\n", Time::getCurrentTime() - startTime );
    }




void freeNames() {
    //delete firstNameTree;
    //delete lastNameTree;

    firstNames.deallocateStringElements();
    lastNames.deallocateStringElements();
    }



// returns 0 if A is identical to B
// returns alphabetical distance for first character difference
// postive if A is further down in the alphabet than B
// negative if A is further up than B
// Prefixes come alphabetically before longer strings, with the END
// of the string occurring before A alphabetically.
//
// Compare APPLE with APPLE returns 0
// Compare A with APPLE returns -16
// Compare DOG with APPLE returns 3
// Compare APE with DOGMA returns -3
// Compare APE with APPLE returns -11

// FIXME:
// APE should be closer to APPLE than A
int prefixCompare( const char *inA, const char *inB ) {
    
    int i = 0;
    while( inA[i] != '\0' &&
           inB[i] != '\0' ) {
        
        int diff = inA[i] - inB[i];
        
        if( diff > 0 ) {
            return diff;
            }
        else if( diff < 0 ) {
            return diff;
            }
        i++;
        }

    // walked off end with equal prefix
    if( inB[i] != '\0' ) {
        return -( ( inB[i] - 'A' ) + 1 );
        }
    if( inA[i] != '\0' ) {
        return ( inA[i] - 'A' ) + 1;
        }
    
    // equal and same length
    return 0;
    }

/*
void testPrefixCompare() {
    assert( prefixCompare( "APPLE", "APPLE" ) == 0 );
    assert( prefixCompare( "A", "APPLE" ) == -16 );
    assert( prefixCompare( "DOG", "APPLE" ) == 3 );
    assert( prefixCompare( "APE", "DOGMA" ) == -3 );
    assert( prefixCompare( "APE", "APPLE" ) == -11 );
    }
*/


const char *findCloseName( char *inString, SimpleVector<char*> *inNameList ) {
    char *tempString = stringToUpperCase( inString );
    
    int limit = inNameList->size();

    int jumpSize = limit / 2;
    int offset = jumpSize;
    int lastDiff = 256;
    
    
    

    while( jumpSize > 0 && lastDiff != 0 ) {
        char *testString = inNameList->getElementDirect( offset );
        printf( "Considering %s at offset %d/%d\n", testString, offset, limit );
        
        lastDiff = prefixCompare( tempString, testString );
        
        jumpSize /= 2;
        
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
