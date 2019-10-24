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
    r->lastBirthTimeSeconds = inBirthTimeSeconds;
    r->lastBirthID = inID;
    }



void addDeath( char *inEmail, double inDeathTimeSeconds, int inID ) {

    PlayerRecord *r = findRecord( inEmail );

    if( r->firstGameTimeSeconds > inDeathTimeSeconds ) {
        r->firstGameTimeSeconds = inDeathTimeSeconds;
        }

    // only add to total if this death is within bin for this player
    
    if( inDeathTimeSeconds > r->firstGameTimeSeconds &&
        inDeathTimeSeconds - r->firstGameTimeSeconds < binSeconds ) {

        // ignore ID mismatch
        // this means a death with no matching birth
        if( r->lastBirthTimeSeconds > 0  &&
            inID == r->lastBirthID ) {

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
    printf( "First pass\n" );
    processDir( &dirFile );
    
    
    // now clean all records of everything BUT first life time
    for( int i=0; i<records.size(); i++ ) {
        PlayerRecord *r = records.getElement( i );
        r->totalGameTimeSeconds = 0;
        r->lastBirthTimeSeconds = 0;
        r->lastBirthID = -1;
        }
    
    // process again here
    printf( "Second pass\n" );
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
