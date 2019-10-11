#include <stdio.h>
#include <stdlib.h>


void usage() {
    printf( "playerStatsMovingAverage inFile.txt outFile.txt\n\n" );
    exit( 1 );
    }


// week average
#define WINDOW_BINS 168


double binAve( int *inBins, int inNumBins ) {
    double sum = 0;
    for( int i=0; i<inNumBins; i++ ) {
        sum += inBins[i];
        }
    return sum / inNumBins;
    }



int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs != 3 ) {
        usage();
        }
    
    FILE *inFile = fopen( inArgs[1], "r" );
    if( inFile == NULL ) {
        usage();
        }
    
    FILE *outFile = fopen( inArgs[2], "w" );
    
    int bins[ WINDOW_BINS ];
    
    for( int i=0; i<WINDOW_BINS; i++ ) {
        bins[ i ] = 0;
        }
    
    int binIndex = 0;

    double time;
    int pop;
    
    int numRead = fscanf( inFile, "%lf %d\n", &time, &pop );
    
    while( numRead == 2 ) {
        // replace oldest value
        bins[ binIndex ] = pop;
        
        binIndex ++;
        
        // wrap
        if( binIndex >= WINDOW_BINS ) {
            binIndex = binIndex % WINDOW_BINS;
            }
        
        double ave = binAve( bins, WINDOW_BINS );
        
        fprintf( outFile, "%.0lf %d %.2lf\n", time, pop, ave );
        

        numRead = fscanf( inFile, "%lf %d\n", &time, &pop );
        }
    
    fclose( inFile );
    fclose( outFile );
    
    
    return 1;
    }
