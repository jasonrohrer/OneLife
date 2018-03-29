#include "names.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/StringTree.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/random/JenkinsRandomSource.h"

#include <stdio.h>


static SimpleVector<char*> firstNames;
static SimpleVector<char*> lastNames;


static StringTree *firstNameTree;
static StringTree *lastNameTree;



static JenkinsRandomSource randSource;

static const char *defaultName = "";


static void readNameFile( const char *inFileName, 
                          SimpleVector<char*> *inNameList,
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
        inNameTree->insert( name, name );
        }
    }



void initNames() {
    firstNameTree = new StringTree( false );
    lastNameTree = new StringTree( false );
    
    readNameFile( "firstNames.txt", &firstNames, firstNameTree );
    readNameFile( "lastNames.txt", &lastNames, lastNameTree );

    printf( "Testing names\n" );
    for( int i = 0; i<10; i++ ) {
        const char *first = findCloseFirstName( "RO" );
        const char *last = findCloseLastName( "RO" );
        printf( "First: %s, Last: %s\n", first, last );
        }
    }




void freeNames() {
    delete firstNameTree;
    delete lastNameTree;

    firstNames.deallocateStringElements();
    lastNames.deallocateStringElements();
    }




const char *findCloseName( char *inString, StringTree *inTree ) {
    char *tempString = stringToUpperCase( inString );
    int len = strlen( tempString );
    
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
    }



// results destroyed internally when freeNames called
const char *findCloseFirstName( char *inString ) {
    return findCloseName( inString, firstNameTree );
    }



const char *findCloseLastName( char *inString ) {
    return findCloseName( inString, lastNameTree );
    }
