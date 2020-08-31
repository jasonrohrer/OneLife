#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "minorGems/io/file/File.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/StringTree.h"
#include "minorGems/util/crc32.h"


int binSeconds = 3600 * 24 * 7;


void usage() {
    printf( "Usage:\n\ngetFirstWeekStats lifeLogDir [bin_seconds]\n\n" );
    exit( 0 );
    }


typedef struct PlayerRecord {
        char *email;
        double firstGameTimeSeconds;
        double lastGameTimeSeconds;
        double totalGameTimeSecondsInBin;
        double totalGameTimeSeconds;

        double lastBirthTimeSeconds;
        int lastBirthID;
    } PlayerRecord;




SimpleVector<PlayerRecord> records;


int totalEmailAllocation = 0;


int hashTableSize = 10000;

typedef struct HashRecord {
        char *email;
        int index;
    } HashRecord;
      
HashRecord *hashTable = new HashRecord[ hashTableSize ];

int hashElements = 0;

void hashTableInit() {
    memset( hashTable, 0, hashTableSize * sizeof( HashRecord ) );
    }



int getHashKey( char *inEmail ) {
    return (int)( crc32( (unsigned char*)inEmail, 
                         strlen( inEmail ) ) % hashTableSize );
    }




void insertRecord( HashRecord inR ) {
    int k = getHashKey( inR.email );
    
    while( hashTable[k].email != NULL ) {
        
        k++;
        if( k >= hashTableSize ) {
            k -= hashTableSize;
            }
        }

    // found empty
    hashTable[k] = inR;
    }



void expandHashTable() {
    int oldSize = hashTableSize;
    HashRecord *oldTable = hashTable;
    
    hashTableSize *= 2;
    hashTable = new HashRecord[ hashTableSize ];
    hashTableInit();
    
    for( int i=0; i<oldSize; i++ ) {
        
        HashRecord r = oldTable[i];
        
        if( r.email != NULL ) {
            insertRecord( r );
            }
        }
    }



HashRecord *findHashRecord( char *inEmail ) {
    
    int k = getHashKey( inEmail );
    
    HashRecord *r = &( hashTable[ k ] );

    while( r->email == NULL ||
           strcmp( r->email, inEmail ) != 0 ) {
        
        if( r->email == NULL ) {
            // not found
            return NULL;
            }
        
        // try next
        k++;
        if( k >= hashTableSize ) {
            k -= hashTableSize;
            }
        r = &( hashTable[ k ] );
        }

    return r;
    }



int findIndex( char *inEmail ) {
    HashRecord *r = findHashRecord( inEmail );
    
    if( r == NULL ) {
        return -1;
        }
    return r->index;
    }



void addIndex( char *inEmail, int inIndex ) {
    HashRecord r = { stringDuplicate( inEmail ), inIndex };
    
    insertRecord( r );

    hashElements ++;
    
    if( hashElements > hashTableSize / 2 ) {
        // half full, expand
        expandHashTable();
        }
    
    }



PlayerRecord *findRecord( char *inEmail ) {

    int index = findIndex( inEmail );
    
    if( index != -1 ) {
        PlayerRecord *r = records.getElement( index );

        if( strcmp( inEmail, r->email ) == 0 ) {
            return r;
            }
        }

    // not found, create one
    PlayerRecord r = { stringDuplicate( inEmail ),
                       DBL_MAX,
                       0,
                       0,
                       0,
                       -1 };
    records.push_back( r );

    totalEmailAllocation += strlen( inEmail );

    index = records.size() - 1;

    addIndex( inEmail, index );
    
    
    return records.getElement( index );
    }



void addBirth( char *inEmail, double inBirthTimeSeconds, int inID ) {
    PlayerRecord *r = findRecord( inEmail );
    
    if( r->firstGameTimeSeconds > inBirthTimeSeconds ) {
        r->firstGameTimeSeconds = inBirthTimeSeconds;
        }
    if( inBirthTimeSeconds > r->lastGameTimeSeconds ) {
        r->lastGameTimeSeconds = inBirthTimeSeconds;
        }

    r->lastBirthTimeSeconds = inBirthTimeSeconds;
    r->lastBirthID = inID;
    }



void addDeath( char *inEmail, double inDeathTimeSeconds, int inID ) {

    PlayerRecord *r = findRecord( inEmail );

    if( r->firstGameTimeSeconds > inDeathTimeSeconds ) {
        r->firstGameTimeSeconds = inDeathTimeSeconds;
        }

    if( inDeathTimeSeconds > r->lastGameTimeSeconds ) {
        r->lastGameTimeSeconds = inDeathTimeSeconds;
        }

    // ignore ID mismatch
    // this means a death with no matching birth
    if( r->lastBirthTimeSeconds > 0  &&
        inID == r->lastBirthID ) {
        
        double lifeSeconds = inDeathTimeSeconds - r->lastBirthTimeSeconds;

        // only add to total if this death is within bin for this player
        if( inDeathTimeSeconds - r->firstGameTimeSeconds < binSeconds ) {
            r->totalGameTimeSecondsInBin += lifeSeconds;
            }

        r->totalGameTimeSeconds += lifeSeconds;
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
                        addBirth( emailBuffer, t, id );
                        }
                    else if( bOrD == 'D' ) {
                        numDeaths++;
                        addDeath( emailBuffer, t, id );                        
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
    char *fileName = inFile->getFileName();
    
    char nonLog = false;
    if( strstr( fileName, "lifeLog" ) != fileName ) {
        nonLog = true;
        }

    if( ! nonLog && inFile->isDirectory() ) {
        
        printf( "Processing dir %s\n", fileName );
            
        
        int numChildFiles;
        
        File **childFiles = inFile->getChildFiles( &numChildFiles );
        
        for( int i=0; i<numChildFiles; i++ ) {
            printf("\r%5d/%d (%5d unique records) (%7d total email bytes)", 
                   i, numChildFiles,
                   records.size(),
                   totalEmailAllocation );
            fflush( stdout );
            
            processFile( childFiles[i] );
            
            delete childFiles[i];
            }
        delete [] childFiles;
        printf( "\n" );
        }
    delete [] fileName;
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

    hashTableInit();
    
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
    printf( "\n\nFirst pass\n" );
    processDir( &dirFile );
    
    
    // now clean all records of everything BUT first and last life time
    for( int i=0; i<records.size(); i++ ) {
        PlayerRecord *r = records.getElement( i );
        r->totalGameTimeSeconds = 0;
        r->totalGameTimeSecondsInBin = 0;
        r->lastBirthTimeSeconds = 0;
        r->lastBirthID = -1;
        }
    
    // process again here
    printf( "\n\nSecond pass\n" );
    processDir( &dirFile );

    
    // now we have actual binned life times, in bin starting at
    // each player's first game

    // will work up to year 3900
    #define YEARS 3000
    #define MONTHS 12
    double yearMonthSums[YEARS][MONTHS];
    int yearMonthCounts[YEARS][MONTHS];
    int yearMonthQuitCounts[YEARS][MONTHS];

    #define HOUR_BINS 9
    int hourBins[ HOUR_BINS ] = { 1, 5, 10, 20, 50, 100, 200, 500, 1000 };
    
    int yearMonthHourCounts[YEARS][MONTHS][ HOUR_BINS ];


    // spans in days
    #define SPAN_BINS 8
    int spanBins[ SPAN_BINS ] = { 1, 5, 10, 30, 60, 90, 182, 365 };
    
    int yearMonthSpanCounts[YEARS][MONTHS][ SPAN_BINS ];
    
    
    for( int y=0; y<YEARS; y++ ) {
        for( int m=0; m<MONTHS; m++ ) {
            yearMonthSums[y][m] = 0;
            yearMonthCounts[y][m] = 0;
            yearMonthQuitCounts[y][m] = 0;
            for( int h=0; h<HOUR_BINS; h++ ) {
                yearMonthHourCounts[y][m][h] = 0;
                }
            for( int s=0; s<SPAN_BINS; s++ ) {
                yearMonthSpanCounts[y][m][s] = 0;
                }
            }
        }
    

    for( int i=0; i<records.size(); i++ ) {
        PlayerRecord *r = records.getElement( i );


        time_t t = (time_t)( r->firstGameTimeSeconds );

        struct tm ts;
        ts = *localtime(&t);
        
        int y = ts.tm_year;
        int m = ts.tm_mon;
        
        yearMonthSums[y][m] += r->totalGameTimeSecondsInBin;
        yearMonthCounts[y][m] += 1;

        if( r->lastGameTimeSeconds - r->firstGameTimeSeconds < binSeconds ) {
            // they quit playing within this window
            yearMonthQuitCounts[y][m] += 1;
            }

        double totalHours = r->totalGameTimeSeconds / 3600;
        
        for( int h=0; h<HOUR_BINS; h++ ) {
            if( totalHours > hourBins[h] ) {
                yearMonthHourCounts[y][m][h] += 1;
                }
            }

        
        double daySpan = 
            ( r->lastGameTimeSeconds - r->firstGameTimeSeconds ) / 
            ( 3600 * 24 );
        
        for( int s=0; s<SPAN_BINS; s++ ) {
            if( daySpan > spanBins[s] ) {
                yearMonthSpanCounts[y][m][s] += 1;
                }
            }
        }

    printf( "\n\n%d-day Report:\n", binSeconds / ( 3600 * 24 ) );
    
    printf( "date, aveHrInWind, plrCnt, quitCnt, quitFrct\n" );
    
    
    for( int y=0; y<YEARS; y++ ) {
        for( int m=0; m<MONTHS; m++ ) {
            if( yearMonthCounts[y][m] > 0 ) {
                
                double ave = yearMonthSums[y][m] / yearMonthCounts[y][m];
                
                double quitFraction = 
                    yearMonthQuitCounts[y][m] / (double) yearMonthCounts[y][m];

                printf( "%4d-%02d %5.3lf %6d %6d %5.3f\n", y + 1900, m + 1, 
                        ave / 3600.0,
                        yearMonthCounts[y][m],
                        yearMonthQuitCounts[y][m],
                        quitFraction );
                }
            }
        }
    

    printf( "\n\nHour bin report:\n" );
    printf( "date, " );
    for( int h=0; h<HOUR_BINS; h++ ) {
        printf( ">-%d-hrFrct, ", hourBins[ h ] );
        }
    printf( "\n" );

    for( int y=0; y<YEARS; y++ ) {
        for( int m=0; m<MONTHS; m++ ) {
            if( yearMonthCounts[y][m] > 0 ) {
                
                printf( "%4d-%02d ", y + 1900, m + 1 );
                
                for( int h=0; h<HOUR_BINS; h++ ) {
                    double hourFraction =
                        yearMonthHourCounts[y][m][h] / 
                        (double) yearMonthCounts[y][m];
                    
                    printf( "%5.3f ", hourFraction );
                    }
                
                printf( "\n" );
                }
            }
        }


    printf( "\n\nDay-span bin report:\n" );
    printf( "date, " );
    for( int s=0; s<SPAN_BINS; s++ ) {
        printf( ">-%d-days, ", spanBins[ s ] );
        }
    printf( "\n" );

    for( int y=0; y<YEARS; y++ ) {
        for( int m=0; m<MONTHS; m++ ) {
            if( yearMonthCounts[y][m] > 0 ) {
                
                printf( "%4d-%02d ", y + 1900, m + 1 );
                
                for( int s=0; s<SPAN_BINS; s++ ) {
                    double spanFraction =
                        yearMonthSpanCounts[y][m][s] / 
                        (double) yearMonthCounts[y][m];
                    
                    printf( "%5.3f ", spanFraction );
                    }
                
                printf( "\n" );
                }
            }
        }

    
    

    for( int i=0; i<records.size(); i++ ) {
        PlayerRecord *r = records.getElement( i );
        delete [] r->email;
        }

    
    for( int i=0; i<hashTableSize; i++ ) {
        if( hashTable[i].email != NULL ) {
            delete [] hashTable[i].email;
            }
        }
    delete [] hashTable;

    return 0;
    }
