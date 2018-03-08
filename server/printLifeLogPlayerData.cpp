#include <stdlib.h>


#include "minorGems/io/file/File.h"
#include "minorGems/util/stringUtils.h"


void usage() {
    printf( "Usage:\n" );
    printf( "printLifeLogPlayerData path_to_server_dir outDataFile\n\n" );
    
    printf( "NOTE:  server dir can contain multiple lifeLog dirs\n" );
    printf( "       (lifeLog, lifeLog_server2, etc.)\n\nd" );

    printf( "Example:\n" );
    printf( "printLifeLogPlayerData "
            "~/checkout/OneLife/server data.txt\n\n" );
    
    exit( 1 );
    }


// used to keep track of hours passing
double startTime = 0;
int hoursPassed = 0;
    
SimpleVector<char *> uniqueEmails;


typedef struct HourRecord {
        double time;
        int uniquePlayers;
    } HourRecord;
    

SimpleVector<HourRecord> hourRecords;

        



// destroyed internally
void addEmail( char *inEmail ) {
    for( int i=0; i<uniqueEmails.size(); i++ ) {
        if( strcmp( inEmail, uniqueEmails.getElementDirect( i ) ) == 0 ) {
            delete [] inEmail;
            return;
            }
        }
    uniqueEmails.push_back( inEmail );
    }



void processLogFile( File *inFile ) {
    
    char *path = inFile->getFullFileName();

    
    FILE *f = fopen( path, "r" );
    
    if( f != NULL ) {
        char scannedLine = true;
        
        while( scannedLine ) {
            
            char event = 'X';
            double time = 0;
            int id = 0;
            char email[1000];
            char gender = 'F';
            double age = 0;
            int locX, locY;
            char parent[1000];
            char deathReason[1000];
        
            email[0] = '\0';
            parent[0] = '\0';
            deathReason[0] = '\0';        

            int pop = 0;
            int parentChain = 1;
        
            fscanf( f, "%c ", &event );
        
        
            if( event == 'B' ) {
                fscanf( f, "%lf %d %999s %c (%d,%d) %999s pop=%d chain=%d\n",
                        &time, &id, email, &gender, 
                        &locX, &locY, parent, &pop, &parentChain );
                
                if( startTime == 0 ) {
                    startTime = time;
                    }
                
                int deltaTime = time - startTime;
                
                char *lowerEmail = stringToLowerCase( email );
                
                
                addEmail( lowerEmail );
                
                if( deltaTime / 3600 > hoursPassed ) {
                    hoursPassed = lrint( floor( deltaTime / 3600 ) );
                    
                    double hourTime = hoursPassed * 3600 + startTime;
                    
                    HourRecord r = { hourTime, uniqueEmails.size() };
                    
                    hourRecords.push_back( r );
                    
                    uniqueEmails.deallocateStringElements();
                    }
                }
            else if( event == 'D' ) {
                }
            else {
                scannedLine = false;
                }
            }
        

        fclose( f );
        }
    delete [] path;
    }




const char *checkpointFileName = "statsCheckpoint.txt";



// returns num files processed
int processLifeLogFolder( File *inFolder ) {        
        
    int numFilesProcessed = 0;
        
    int numFiles;

    File **logs = inFolder->getChildFilesSorted( &numFiles );
        
    for( int i=0; i<numFiles; i++ ) {

        char *name = logs[i]->getFileName();
            
        if( strcmp( name, checkpointFileName ) != 0 ) {
            
            processLogFile( logs[i] );
                    
            numFilesProcessed++;
            }
        
        delete [] name;

        delete logs[i];
        }
    delete [] logs;

    return numFilesProcessed;
    }



void printCommaInt( FILE *inFile, int inInt ) {

    int thou = 1000;
    int mil = thou * thou;
    int bil = mil * thou;
    

    int billions = inInt / bil;
    inInt -= billions * bil;
    
    int millions = inInt / mil;
    inInt -= millions * mil;
    
    int thousands = inInt / thou;
    inInt -= thousands * thou;
    
    if( billions > 0 ) {
        fprintf( inFile, "%d,", billions );
        }
    if( millions > 0 ) {
        fprintf( inFile, "%d,", millions );
        }
    if( thousands > 0 ) {
        fprintf( inFile, "%d,", thousands );
        }

    fprintf( inFile, "%d", inInt );
    }



int main( int inNumArgs, char **inArgs ) {

    if( inNumArgs != 3 ) {
        usage();
        }
    
    char *path = inArgs[1];
    
    char *outPath = inArgs[2];
    
    if( path[strlen(path) - 1] == '/' ) {
        path[strlen(path) - 1] = '\0';
        }
    
    File mainDir( NULL, path );
    
    if( mainDir.exists() && mainDir.isDirectory() ) {
        
        
        int numFilesProcessed = 0;
        
        int numChildFiles;
        File **childFiles = mainDir.getChildFiles( &numChildFiles );
        
        for( int i=0; i<numChildFiles; i++ ) {
        
            if( childFiles[i]->isDirectory() ) {
                
                char *name = childFiles[i]->getFileName();
                
                if( strstr( name, "lifeLog" ) == name ) {
                    // file name starts with lifeLog
                    numFilesProcessed += processLifeLogFolder( childFiles[i] );
                    }
                delete [] name;
                }
            
            delete childFiles[i];
            }
        delete [] childFiles;
        

        
            
        
        printf( "Processed %d files\n", numFilesProcessed );

        FILE *outFile = fopen( outPath, "w" );
        

        if( outFile != NULL ) {
            
            for( int i=0; i<hourRecords.size(); i++ ) {
                HourRecord r = hourRecords.getElementDirect( i );

                fprintf( outFile, "%.0f %d\n",
                         r.time, r.uniquePlayers );
                }
            
            fclose( outFile );
            }
        }
    else {
        usage();
        }
    
    }
