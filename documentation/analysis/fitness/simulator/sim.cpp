#include <stdio.h>

#include "minorGems/util/random/JenkinsRandomSource.h"

double K = 46.5;

int numRuns = 100;


double addBabyToScore( double inOldScore, double inBabyAge ) {
    double s = inOldScore + ( ( inBabyAge - inOldScore )  ) / K;
    //printf( "Baby died at %lf, new score %lf\n", inBabyAge, s );
    return s;
    }


int main() {

    JenkinsRandomSource randSource;


    //K = 30;
    
    //for( int kk=0; kk<60; kk++ ) {


    int binSize = 10000;
    double aveErrorBin = 0;
    double overlapBin = 0;

    //for( int b=0; b<binSize; b++ ) {
            
        
    double babyDeathRate = 0.1;

    double low[9];
    double high[9];

    double totalAveError = 0;
    

    for( int r=0; r<9; r++ ) {
        

        double score = 0;
        
        

        // first warm up
        for( int i=0; i<numRuns; i++ ) {
            
            if( randSource.getRandomBoundedDouble( 0, 1 ) < babyDeathRate ) {
                score = addBabyToScore( score, 0 );
                }
            else {
                score = addBabyToScore( score, 60 );
                }
            }

        // now run again and watch stats

        double sum = 0;  
        
        double lowest = 60;
        double highest = 0;


        for( int i=0; i<numRuns; i++ ) {
            
            if( randSource.getRandomBoundedDouble( 0, 1 ) < babyDeathRate ) {
                score = addBabyToScore( score, 0 );
                }
            else {
                score = addBabyToScore( score, 60 );
                }

            if( score < lowest ) {
                lowest = score;
                }
            if( score > highest ) {
                highest = score;
                }
            
            sum += score;
            }
        
        low[r] = lowest;
        high[r] = highest;
        
        
        //if( false )
        printf( "Baby death rate %.1f, Lowest score seen = %f, highest = %f\n", 
                babyDeathRate, lowest, highest );

        double ave = sum / numRuns;
        double targetAve = numRuns * (1-babyDeathRate) * 60 / numRuns;
        
        //if( false )
        printf( "                     Overall Ave = %f\n", ave );
        //if( false )
        printf( "                     Desired Ave = %f\n", targetAve );
        
        double error = fabs( ave - targetAve );
        //if( false )
        printf( "                     Ave error = %f\n\n", error );

        totalAveError += error;

        babyDeathRate += 0.1;
        }

    double overlap = 0;
    for( int r=0; r<9; r++ ) {

        for( int o=r+1; o<9; o++ ) {
            if( high[o] > low[r] ) {
                overlap += high[o] - low[r];
                }
            }
        }
    
    //if( false )
    printf( "K=%f, Total ave error = %f, Total overlap = %f\n",
            K, totalAveError, overlap );

    if( false )
    printf( "%f %f %f %f\n",
            K, totalAveError, overlap, totalAveError + overlap );

    aveErrorBin += totalAveError;
    overlapBin += overlap;
    //}

    //printf( "%f %f %f %f\n",
    //        K, aveErrorBin / binSize, overlapBin / binSize, 
    //        aveErrorBin / binSize + overlapBin / binSize  );
    

    //K += 0.5;
    //}
    }
