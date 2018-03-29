#include "names.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/StringTree.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/io/file/File.h"
#include "minorGems/util/random/JenkinsRandomSource.h"

#include <stdio.h>
//#include <assert.h>


//static SimpleVector<char*> firstNames;
//static SimpleVector<char*> lastNames;

static char *firstNames;
static char *lastNames;

// total length of arrays
static int firstNamesLen;
static int lastNamesLen;




#include <malloc.h>

int lastAllocCount = 0;

int getAllocTotal() {
    struct mallinfo mi = mallinfo();
    int tot = mi.hblkhd + mi.uordblks;
    //printf( "malloc sees %d blocks\n", tot );
    return tot;
    }


int getAllocDelta() {
    int total = getAllocTotal();
    
    int delta = total - lastAllocCount;
    
    lastAllocCount = total;
    
    return delta;
    }



static JenkinsRandomSource randSource;

static const char *defaultName = "";


static char *readNameFile( const char *inFileName, int *outLen ) {
    
    File nameFile( NULL, inFileName );

    //getAllocDelta();
    char *contents = nameFile.readFileContents();
    //printf( "Reading consumed %d bytes\n", getAllocDelta() );

    if( contents == NULL ) {
        AppLog::errorF( "Failed to open name file %s for reading", inFileName );
        return NULL;
        }
    char *temp = contents;
    contents = stringToUpperCase( temp );
    delete [] temp;
    
    int len = strlen( contents );
    printf( "Read length %d file\n", len );
    
    for( int i=0; i<len; i++ ) {
        if( contents[i] == '\n' ) {
            contents[i] = '\0';
            }
        }
    *outLen = len;
    
    return contents;
    }


void initNames() {
    getAllocDelta();
    double startTime = Time::getCurrentTime();
    
    firstNames = readNameFile( "firstNames.txt", &firstNamesLen  );
    //printf( "   first names consumed %d bytes\n", getAllocDelta() );
    
    lastNames = readNameFile( "lastNames.txt", &lastNamesLen );
    //printf( "   last names consumed %d bytes\n", getAllocDelta() );
    printf( "Name init took %f sec\n", Time::getCurrentTime() - startTime );

    printf( "   and consumed %d bytes\n", getAllocDelta() );

    printf( "Testing names with 100000 lookups\n" );
    startTime = Time::getCurrentTime();
    for( int i = 0; i<100000; i++ ) {
        //for( int i = 0; i<1; i++ ) {
        const char *first = findCloseFirstName( "FUCKHEAD" );
        //printf( "\n" );
        
        const char *last = findCloseLastName( "ASSHOLE" );
        //printf( "First: %s, Last: %s\n", first, last );
        }
    
    printf( "Lookups took %f sec (%d bytes)\n", 
            Time::getCurrentTime() - startTime, getAllocDelta() );
    }




void freeNames() {
    //firstNames.deallocateStringElements();
    //lastNames.deallocateStringElements();
    }




static int getSign( int inX ) {
    // found here: https://stackoverflow.com/a/1903975/744011
    return (inX > 0) - (inX < 0);
    }




int sharedPrefixLength( const char *inA, const char *inB ) {
    
    int i=0;
    
    while( inA[i] != '\0' && inB[i] != '\0' &&
           inA[i] == inB[i] ) {
        i++;
        }
    return i;
    }




const char *findCloseName( char *inString, SimpleVector<char*> *inNameList ) {
    char *tempString = stringToUpperCase( inString );
    
    int limit = inNameList->size();

    int jumpSize = limit / 2;
    int offset = jumpSize;
    int lastDiff = 256;
    
    char print = false;
    
    

    while( lastDiff != 0 ) {
        char *testString = inNameList->getElementDirect( offset );
        lastDiff = strcmp( tempString, testString );
        
        if( print )printf( "Jumping %d and "
                          "considering %s at offset %d/%d (diff=%d)\n", 
                          jumpSize, testString, offset, limit, lastDiff );
        
        
        jumpSize /= 2;
        if( jumpSize == 0 ) {
            break;
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
            if( print )printf( "Stepping %d and "
                               "considering %s at offset %d/%d (diff=%d)\n", 
                               step, testString, offset, limit, nextDiff );
            }
        
        if( nextDiff != 0 ) {
            // string does not exist
            // we've found the two strings alphabetically around it though
            // use one with longest shared prefix

            int crossOffset = offset;
            int prevOffset = offset - step;
            
            int crossSim = sharedPrefixLength( tempString, 
                                               inNameList->getElementDirect( 
                                                   crossOffset ) );
            int prevSim = sharedPrefixLength( tempString, 
                                               inNameList->getElementDirect( 
                                                   prevOffset ) );
            
            if( print )
            printf( "No match found, nearby strings %s (s=%d) and %s (s=%d)\n",
                    inNameList->getElementDirect( crossOffset ),
                    crossSim,
                    inNameList->getElementDirect( prevOffset ),
                    prevSim );


            if( crossSim > prevSim ) {
                offset = crossOffset;
                }
            else if( crossSim < prevSim ) {
                offset = prevOffset;
                }
            else {
                // share same prefix
                // return shorter one
                if( print ) 
                    printf( "Shared prefix length same, returning shorter\n" );
                
                if( strlen( inNameList->getElementDirect( prevOffset ) ) <
                    strlen( inNameList->getElementDirect( crossOffset ) ) ) {
                    offset = prevOffset;
                    }
                else {
                    offset = crossOffset;
                    }
                    
                }
            
            }
            
        }
    

    
    delete [] tempString;

    return inNameList->getElementDirect( offset );
    }



// results destroyed internally when freeNames called
const char *findCloseFirstName( char *inString ) {
    //return findCloseName( inString, &firstNames );
    }



const char *findCloseLastName( char *inString ) {
    //return findCloseName( inString, &lastNames );
    }
