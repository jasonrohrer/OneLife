#include <stdlib.h>


#include "minorGems/io/file/File.h"


void usage() {
    printf( "Usage:\n" );
    printf( "printLifeLogStatsHTML path_to_lifeLog_dir outHTMLFile\n\n" );
    
    printf( "Example:\n" );
    printf( "printLifeLogStatsHTML "
            "~/checkout/OneLife/server/lifeLog out.html\n\n" );
    
    exit( 1 );
    }


typedef struct Living {
        int id;
        double birthAge;
        int parentChainLength;
    } Living;

    
SimpleVector<Living> currentLiving;


// stats
double totalAge = 0;
int totalLives = 0;
int longestFamilyChain = 0;

int over50Count = 0;




void processLogFile( File *inFile ) {
    
    char *path = inFile->getFullFileName();

    printf( "Processing %s\n", path );
    
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
        

            int pop;
        
            fscanf( f, "%c ", &event );
        
            printf( "Scanning %c\n", event );
        
            if( event == 'B' ) {
                fscanf( f, "%lf %d %999s %c (%d,%d) %999s pop=%d\n",
                        &time, &id, email, &gender, 
                        &locX, &locY, parent, &pop );
            
                Living l;
                l.id = id;
            
                l.birthAge = 0;
                l.parentChainLength = 0;

                if( strcmp( parent, "noParent" ) == 0 ) {
                    l.birthAge = 14;
                    }
                else {
                    int parentID = 0;
                
                    sscanf( parent, "parent=%d,", &parentID );
                
                    for( int i=0; i<currentLiving.size(); i++ ) {
                        Living lp = currentLiving.getElementDirect( i );
                    
                        if( lp.id == parentID ) {
                            l.parentChainLength = lp.parentChainLength + 1;
                            break;
                            }
                        }
                    }
                currentLiving.push_back( l );
                totalLives ++;

                if( l.parentChainLength > longestFamilyChain ) {
                    longestFamilyChain = l.parentChainLength;
                    }
                }
            else if( event == 'D' ) {
                fscanf( f, "%lf %d %999s age=%lf %c (%d,%d) %999s pop=%d\n",
                        &time, &id, email, &age, &gender, &locX, &locY, 
                        deathReason, &pop );
            
                printf( "Scanning death, age %f\n", age );
            
                double yearsLived = age;

                for( int i=0; i<currentLiving.size(); i++ ) {
                    Living l = currentLiving.getElementDirect( i );
                    
                    if( l.id == id ) {
                        yearsLived -= l.birthAge;
                    
                        currentLiving.deleteElement( i );
                        break;
                        }
                    }
            
                totalAge += yearsLived;
            
                if( age >= 50 ) {
                    over50Count++;
                    }
                }
            else {
                scannedLine = false;
                }
            }
        

        fclose( f );
        }
    delete [] path;
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
        int numFiles;
        File **logs = mainDir.getChildFiles( &numFiles );
        
        for( int i=0; i<numFiles; i++ ) {
            
            processLogFile( logs[i] );
            
            delete logs[i];
            }
        delete [] logs;


        FILE *outFile = fopen( outPath, "w" );
        

        if( outFile != NULL ) {
            
            fprintf( outFile, 
                     "%d lives lived for a total of %0.2f hours<br>\n"
                     "%d people lived past age 50<br>\n"
                     "%d generations in longest family line",
                     totalLives, totalAge, over50Count, longestFamilyChain );
            
            fclose( outFile );
            }
        
        }
    else {
        usage();
        }
    
    }
