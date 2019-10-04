#include "names.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/StringTree.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/io/file/File.h"
#include "minorGems/util/random/JenkinsRandomSource.h"

#include <stdio.h>


static char *firstNamesMale = NULL;
static char *firstNamesFemale = NULL;
static char *lastNames = NULL;

// total length of arrays
static int firstNamesMaleLen;
static int firstNamesFemaleLen;
static int lastNamesLen;








static const char *defaultName = "";




static char *readNameFile( const char *inFileName, int *outLen ) {
    
    File nameFile( NULL, inFileName );

    char *contents = nameFile.readFileContents();

    if( contents == NULL ) {
        AppLog::errorF( "Failed to open name file %s for reading", inFileName );
        return NULL;
        }
    char *temp = contents;
    contents = stringToUpperCase( temp );
    delete [] temp;
    
    int len = strlen( contents );

    for( int i=0; i<len; i++ ) {
        if( contents[i] == '\n' ) {
            contents[i] = '\0';
            }
        }
    *outLen = len;
    
    return contents;
    }




void initNames() {    
    firstNamesMale = readNameFile( "maleNames.txt", &firstNamesMaleLen  );
    firstNamesFemale = readNameFile( "femaleNames.txt", &firstNamesFemaleLen  );
    lastNames = readNameFile( "lastNames.txt", &lastNamesLen );
    }




void freeNames() {
    if( firstNamesMale != NULL ) {
        delete [] firstNamesMale;
        firstNamesMale = NULL;
        }
    if( firstNamesFemale != NULL ) {
        delete [] firstNamesFemale;
        firstNamesFemale = NULL;
        }
    if( lastNames != NULL ) {
        delete [] lastNames;
        lastNames = NULL;
        }
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




// walk backward to find start of next name
int getNameOffsetBack( char *inNameList, int inListLen, int inOffset ) {
    while( inOffset > 0 && inNameList[ inOffset ] != '\0' ) {
        inOffset --;
        }
    if( inOffset == 0 ) {
        return 0;
        }
    else {
        // walked forward off of \0
        return inOffset + 1;
        }
    }



int getNameOffsetForward( char *inNameList, int inListLen, int inOffset ) {
    int limit = inListLen - 1;
    while( inOffset < limit && inNameList[ inOffset ] != '\0' ) {
        inOffset ++;
        }
    if( inOffset == limit ) {
        return getNameOffsetBack( inNameList, inListLen, limit - 1 );
        }
    else {
        // walked forward off of \0
        return inOffset + 1;
        }
    }





const char *findCloseName( char *inString, char *inNameList, int inListLen,
                           int *outIndex = NULL ) {
    if( inNameList == NULL ) {
        return defaultName;
        }
    
    char *tempString = stringToUpperCase( inString );
    
    int limit = inListLen;

    int jumpSize = limit / 2;
    int offset = jumpSize;
    offset = getNameOffsetForward( inNameList, inListLen, offset );
    
    int lastDiff = 1;


    int hitStartCount = 0;
    int hitEndCount = 0;
    

    while( lastDiff != 0 ) {
        char *testString = &( inNameList[ offset ] );
        int prevDiff = lastDiff;
        lastDiff = strcmp( tempString, testString );
        
        int lastUsedOffset = offset;
        
        
        if( getSign( lastDiff ) != getSign( prevDiff ) ) {
            // overshot
            // smaller jump in opposite direction
            jumpSize /= 2;
            }
        

        if( jumpSize == 0 ) {
            break;
            }


        if( lastDiff > 0 ) {
            // further down
            offset += jumpSize;

            if( offset >= limit ) {
                // walked off end
                offset = limit - 2;
                offset = getNameOffsetBack( inNameList, inListLen, offset );
                hitEndCount++;
                if( hitEndCount > 1 ) {
                    break;
                    }
                }
            else {
                offset = getNameOffsetForward( inNameList, inListLen, offset );
                hitEndCount = 0;
                if( offset == lastUsedOffset ) {
                    // back to same location as last time?
                    // stuck
                    break;
                    }
                }
            }
        else if( lastDiff < 0 ) {
            // further up
            offset -= jumpSize;

            if( offset < 0 ) {
                // walked off start
                offset = 0;
                hitStartCount++;
                if( hitStartCount > 1 ) {
                    break;
                    }
                }
            else {
                hitStartCount = 0;
                offset = getNameOffsetBack( inNameList, inListLen, offset );
                if( offset == lastUsedOffset ) {
                    // back to same location as last time?
                    // stuck
                    break;
                    }
                }
            }
        }
    
    

    if( lastDiff != 0 && hitEndCount == 0 && hitStartCount == 0 ) {

        // no exact match
        // step backward until we find names that are before
        // and after us alphabetically
        
        int step = 1;
        if( lastDiff < 0 ) {
            step = -1;
            }
        int nextDiff = lastDiff;
        int lastSign = getSign( lastDiff );
        while( getSign( nextDiff ) == lastSign ) {
            
            offset += 2 * step;
            
            if( offset <= 0 ) {
                offset = 0;
                break;
                }
            if( offset >= limit ) {
                offset = limit - 1;
                break;
                }

            if( step < 0 ) {
                offset = getNameOffsetBack( inNameList, inListLen, offset );
                }
            else {
                offset = getNameOffsetForward( inNameList, inListLen, offset );
                }
            
            char *testString = &( inNameList[offset] );
            nextDiff = strcmp( tempString, testString );
            }
        
        
        if( nextDiff != 0 ) {
            // string does not exist
            // we've found the two strings alphabetically around it though
            // use one with longest shared prefix

            int crossOffset = offset;
            int prevOffset = offset - 2 * step;
            
            if( step < 0 ) {
                prevOffset = 
                    getNameOffsetForward( inNameList, inListLen, prevOffset );
                }
            else {
                prevOffset = 
                    getNameOffsetBack( inNameList, inListLen, prevOffset );
                }
            

            int crossSim = sharedPrefixLength( tempString, 
                                               &( inNameList[crossOffset ] ) );
            int prevSim = sharedPrefixLength( tempString, 
                                              &( inNameList[prevOffset] ) );
            
            if( crossSim > prevSim ) {
                offset = crossOffset;
                }
            else if( crossSim < prevSim ) {
                offset = prevOffset;
                }
            else {
                // share same prefix
                // return shorter one

                if( strlen( &( inNameList[prevOffset] ) ) 
                    <
                    strlen( &( inNameList[crossOffset] ) ) ) {
                    offset = prevOffset;
                    }
                else {
                    offset = crossOffset;
                    }
                
                }
            }
        }
    

    
    delete [] tempString;

    if( outIndex != NULL ) {
        *outIndex = offset;
        }
    
    return &( inNameList[offset] );
    }



char *getFirstNamesArray( char inFemale ) {
    char *names = firstNamesMale;
    if( inFemale ) {
        names = firstNamesFemale;
        }
    return names;
    }



int getFirstNamesLen( char inFemale ) {
    int namesLen = firstNamesMaleLen;
    if( inFemale ) {
        namesLen = firstNamesFemaleLen;
        }
    return namesLen;
    }




// results destroyed internally when freeNames called
const char *findCloseFirstName( char *inString, char inFemale ) {
    
    return findCloseName( inString, 
                          getFirstNamesArray( inFemale ),
                          getFirstNamesLen( inFemale ) );
    }



const char *findCloseLastName( char *inString ) {
    return findCloseName( inString, lastNames, lastNamesLen );
    }



int getFirstNameIndex( char *inFirstName, char inFemale ) {
    int i;
    findCloseName( inFirstName, 
                   getFirstNamesArray( inFemale ), 
                   getFirstNamesLen( inFemale ),
                   &i );
    
    return i;
    }


int getLastNameIndex( char *inLastName ) {
    int i;
    findCloseName( inLastName, lastNames, lastNamesLen, &i );
   
    return i;
    }



const char *getFirstName( int inIndex, int *outNextIndex, char inFemale ) {
    *outNextIndex = getNameOffsetForward(
        getFirstNamesArray( inFemale ), 
        getFirstNamesLen( inFemale ),
        inIndex );

    if( inIndex == *outNextIndex ) {
        // loop back around
        *outNextIndex = 0;
        }
    
    return &( getFirstNamesArray( inFemale )[ inIndex ] );
    }



const char *getLastName( int inIndex, int *outNextIndex ) {
    *outNextIndex = getNameOffsetForward( lastNames, lastNamesLen, inIndex );
    
    if( inIndex == *outNextIndex ) {
        // loop back around
        *outNextIndex = 0;
        }    

    return &( lastNames[ inIndex ] );
    }


