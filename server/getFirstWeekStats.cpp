#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "minorGems/io/file/File.h"
#include "minorGems/util/SimpleVector.h"


int binSeconds = 3600 * 24 * 7;


void usage() {
    printf( "Usage:\n\ngetFirstWeekStats lifeLogDir [bin_seconds]\n\n" );
    exit( 0 );
    }


typedef struct PlayerRecord {
        char *email;
        double firstGameTimeSeconds;
        double totalGameTimeSeconds;

        double lastBirthTimeSeconds;
    } PlayerRecord;


SimpleVector<PlayerRecord> records;


PlayerRecord *findRecord( char *inEmail ) {
    for( int i=0; i<records.size(); i++ ) {
        PlayerRecord *r = records.getElement( i );
        
        if( strcmp( inEmail, r->email ) == 0 ) {
            return r;
            }
        }

    // not found, create one
    PlayerRecord r = { stringDuplicate( inEmail ),
                       DBL_MAX,
                       0,
                       0 };
    records.push_back( r );
    
    return records.getElement( records.size() - 1 );
    }



void addBirth( char *inEmail, double inBirthTimeSeconds ) {
    PlayerRecord *r = findRecord( inEmail );
    
    if( r->firstGameTimeSeconds > inBirthTimeSeconds ) {
        r->firstGameTimeSeconds = inBirthTimeSeconds;
        }
    r->lastBirthTimeSeconds = inBirthTimeSeconds;    
    }



void addDeath( char *inEmail, double inDeathTimeSeconds ) {

    PlayerRecord *r = findRecord( inEmail );

    if( r->firstGameTimeSeconds > inDeathTimeSeconds ) {
        r->firstGameTimeSeconds = inDeathTimeSeconds;
        }

    // only add to total if this death is within bin for this player
    
    if( inDeathTimeSeconds > r->firstGameTimeSeconds &&
        inDeathTimeSeconds - r->firstGameTimeSeconds < binSeconds ) {
        
        if( r->lastBirthTimeSeconds > 0 ) {
            r->totalGameTimeSeconds += 
                inDeathTimeSeconds - r->lastBirthTimeSeconds;
            }
        }
    
    
    r->lastBirthTimeSeconds = 0;
    }




void processFile( File *inFile ) {
    char *fileName = inFile->getFullFileName();
    
    if( strstr( fileName, "day.txt" ) != NULL ) {
        
        // a day log file, not a name log file

        FILE *f = fopen( fileName, "r" );
        
        if( f != NULL ) {
            
            //printf( "Processing %s  ", fileName );

            int numBirths = 0;
            int numDeaths = 0;
            
            int numRead = 1;
            
            while( numRead > 0 ) {
                char emailBuffer[100];
                char bOrD;
                double t;
                int id;
                
                numRead = fscanf( f, "%c %lf %d %99s ", 
                                  &bOrD, &t, &id, emailBuffer );                

                if( numRead == 4 ) {

                    if( bOrD == 'B' ) {
                        numBirths++;
                        addBirth( emailBuffer, t );
                        }
                    else if( bOrD == 'D' ) {
                        numDeaths++;
                        addDeath( emailBuffer, t );                        
                        }

                    // skip rest of line
                    fscanf( f, "%*[^\n]" );
                    }
                }

            //printf( "%d births, %d deaths\n", numBirths, numDeaths );

            fclose( f );
            }
        }
    
    delete [] fileName;
    }



void processSubDir( File *inFile ) {
    if( inFile->isDirectory() ) {
        
        int numChildFiles;
        
        File **childFiles = inFile->getChildFiles( &numChildFiles );
        
        for( int i=0; i<numChildFiles; i++ ) {
            processFile( childFiles[i] );
            
            delete childFiles[i];
            }
        delete [] childFiles;
        }
    }


void processDir( File *inDir ) {
    int numChildFiles;
    File **childFiles = inDir->getChildFiles( &numChildFiles );

    for( int i=0; i<numChildFiles; i++ ) {
        processSubDir( childFiles[i] );
        delete childFiles[i];
        }
    
    delete [] childFiles;
    }




int main( int inNumArgs, char **inArgs ) {

    
    if( inNumArgs != 2 && inNumArgs != 3 ) {
        usage();
        }
    
    if( inNumArgs == 3 ) {
        sscanf( inArgs[2], "%d", &binSeconds );
        }
    
    char *dirName = inArgs[1];
    
    File dirFile( NULL, dirName );
    
    if( ! dirFile.exists() ||
        ! dirFile.isDirectory() ) {
        usage();
        }

    // process once to get first life time for each player
    processDir( &dirFile );
    
    
    // now clean all records of everything BUT first life time
    for( int i=0; i<records.size(); i++ ) {
        PlayerRecord *r = records.getElement( i );
        r->totalGameTimeSeconds = 0;
        r->lastBirthTimeSeconds = 0;
        }
    
    // process again here
    processDir( &dirFile );

    
    // now we have actual binned life times, in bin starting at
    // each player's first game

    // will work up to year 3900
    double yearMonthSums[3000][12];
    int yearMonthCounts[3000][12];
    
    for( int y=0; y<3000; y++ ) {
        for( int m=0; m<12; m++ ) {
            yearMonthSums[y][m] = 0;
            yearMonthCounts[y][m] = 0;
            }
        }
    

    for( int i=0; i<records.size(); i++ ) {
        PlayerRecord *r = records.getElement( i );


        time_t t = (time_t)( r->firstGameTimeSeconds );

        struct tm ts;
        ts = *localtime(&t);
        
        int y = ts.tm_year;
        int m = ts.tm_mon;
        
        yearMonthSums[y][m] += r->totalGameTimeSeconds;
        yearMonthCounts[y][m] += 1;
        }


    for( int y=0; y<3000; y++ ) {
        for( int m=0; m<12; m++ ) {
            if( yearMonthCounts[y][m] > 0 ) {
                
                double ave = yearMonthSums[y][m] / yearMonthCounts[y][m];
                
                printf( "%d-%02d %lf %d\n", y + 1900, m + 1, 
                        ave / 3600.0,
                        yearMonthCounts[y][m] );
                }
            }
        }
    

    
    return 0;
    }
