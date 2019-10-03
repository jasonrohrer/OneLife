#include <stdlib.h>
#include <stdio.h>
#include <string.h>


double mixedFraction = 0.05;


void usage() {
    printf( "Usage\n\n processNameYear in.txt maleOut.txt femaleOut.txt 0.05\n\n" );
    exit( 1 );
    }

typedef struct LineResult {
        char name[100];
        char mOrF;
        int freq;
    } LineResult;



char readNextLine( FILE *inFile, LineResult *outResult ) {
    int numRead = fscanf( inFile, "%99[^,],%c,%d\n", outResult->name,
                          &( outResult->mOrF ), &( outResult->freq ) );
    
    if( numRead == 3 ) {
        return true;
        }
    return false;
    }

    


int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs != 5 ) {
        usage();
        }

    sscanf( inArgs[4], "%lf", &mixedFraction );
    
    
    char *inFileName = inArgs[1];
    char *maleFileName = inArgs[2];
    char *femaleFileName = inArgs[3];
    
    FILE *inFile = fopen( inFileName, "r" );
    
    if( inFile == NULL )
        usage();

    FILE *maleFile = fopen( maleFileName, "w" );

    if( maleFile == NULL )
        usage();


    FILE *femaleFile = fopen( femaleFileName, "w" );

    if( femaleFile == NULL )
        usage();


    while( ! feof( inFile ) ) {
        LineResult firstLine;
        char gotLine = readNextLine( inFile, &firstLine );
        
        if( !gotLine ) {
            break;
            }
        
        // peek at next line to see if it matches
        long pos = ftell( inFile );
        
        char secondLineMatch = false;
        LineResult secondLine;
        
        gotLine = readNextLine( inFile, &secondLine );
        
        if( gotLine && strcmp( secondLine.name, firstLine.name ) == 0 ) {
            // same names
            secondLineMatch = true;
            }
        else {
            // non-matching next line
            // rewind for later
            fseek( inFile, pos, SEEK_SET );
            }

        // if they match, F always comes first

        char nameFemale = false;
        char nameMale = false;

        int fCount = 0;
        int mCount = 0;

        if( secondLineMatch ) {
            fCount = firstLine.freq;
            mCount = secondLine.freq;
            }
        else {
            if( firstLine.mOrF == 'M' ) {
                mCount = firstLine.freq;
                }
            else {
                fCount = firstLine.freq;
                }
            }
        
        
        if( fCount >= mCount ) {
            if( (double)mCount / (double)fCount < mixedFraction ) {
                // 95% female;
                nameFemale = true;
                }
            else {
                // female AND male
                nameFemale = true;
                nameMale = true;
                }
            }
        else if( fCount < mCount ) {
            if( (double)fCount / (double)mCount < mixedFraction ) {
                // 95% male;
                nameMale = true;
                }
            else {
                // female AND male
                nameFemale = true;
                nameMale = true;
                }
            }

        if( nameMale ) {
            fprintf( maleFile, "%s\n", firstLine.name );
            }
        if( nameFemale ) {
            fprintf( femaleFile, "%s\n", firstLine.name );
            }
        }
    

    

    fclose( inFile );
    fclose( maleFile );
    fclose( femaleFile );
    

    return 1;
    }
