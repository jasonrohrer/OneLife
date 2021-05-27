
#include <stdlib.h>


#include "minorGems/io/file/File.h"
#include "minorGems/util/stringUtils.h"


void usage() {
    printf( "Usage:\n" );
    printf( "printOldAgePlayAgainStats path_to_life_log_dir\n\n" );
    
    printf( "NOTE:  dir must contain logs for one server\n\n" );

    printf( "Example:\n" );
    printf( "printLifeLogPlayerData "
            "~/checkout/OneLife/server/lifeLog\n\n" );
    
    exit( 1 );
    }


typedef struct DayRecord {
        // in seconds
        double startTimeStamp;
        
        double playAgainFraction;
    } DayRecord;



typedef struct EmailRecord {
        double deathTime;
        
        char *email;
    } EmailRecord;


SimpleVector<EmailRecord> liveEmailRecords;


SimpleVector<DayRecord> completeRecords;


void insertRecord( DayRecord inR ) {
    char pushed = false;
    
    for( int i=0; i<completeRecords.size(); i++ ) {
        DayRecord r = completeRecords.getElementDirect( i );
        
        if( inR.startTimeStamp < r.startTimeStamp ) {
            completeRecords.push_middle( inR, i );
            pushed = true;
            break;
            }
        }
    
    if( ! pushed ) {
        completeRecords.push_back( inR );
        }
    }




void clearLiveRecords() {
    for( int i=0; i<liveEmailRecords.size(); i++ ) {
        delete [] liveEmailRecords.getElementDirect( i ).email;
        }
    liveEmailRecords.deleteAll();
    }




void processLogFile( File *inFile ) {
    clearLiveRecords();

    int numLivedToSixty = 0;
    int numPlayedAgain = 0;

    double startTimeStamp = -1;

    
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
                
                if( startTimeStamp == -1 ) {
                    startTimeStamp = time;
                    }

                char *lowerEmail = stringToLowerCase( email );
                
                for( int i=0; i<liveEmailRecords.size(); i++ ) {
                    EmailRecord r = liveEmailRecords.getElementDirect( i );
                    
                    if( strcmp( r.email, lowerEmail ) == 0 ) {
                        
                        // ten minutes or less
                        if( ( time - r.deathTime ) < 60 * 10 ) {
                            numPlayedAgain ++;
                            }
                        

                        delete [] r.email;
                        liveEmailRecords.deleteElement( i );
                        
                        break;
                        }
                    }
                }
            else if( event == 'D' ) {
                fscanf( f, "%lf %d %999s age=%lf %c (%d,%d) %999s pop=%d\n",
                        &time, &id, email, &age, &gender, &locX, &locY, 
                        deathReason, &pop );

                if( age == 60 ) {
                    EmailRecord r = { time, stringToLowerCase( email ) };
                    
                    liveEmailRecords.push_back( r );
                    numLivedToSixty ++;
                    }
                }
            else {
                scannedLine = false;
                }
            }
        

        fclose( f );
        }
    delete [] path;


    if( startTimeStamp != -1 &&
        numLivedToSixty > 0 ) {
        DayRecord r = { startTimeStamp, 
                        (double)numPlayedAgain / (double)numLivedToSixty };
        
        insertRecord( r );
        }
    }




int main( int inNumArgs, char **inArgs ) {

    if( inNumArgs != 2 ) {
        usage();
        }
    
    char *path = inArgs[1];

        if( path[strlen(path) - 1] == '/' ) {
        path[strlen(path) - 1] = '\0';
        }
    
    File mainDir( NULL, path );
    
    if( mainDir.exists() && mainDir.isDirectory() ) {
        
        
        int numFilesProcessed = 0;
        
        int numChildFiles;
        File **childFiles = mainDir.getChildFiles( &numChildFiles );
        
        for( int i=0; i<numChildFiles; i++ ) {

            char *name = childFiles[i]->getFileName();
            
            if( strstr( name, "_names.txt" ) == NULL ) {
                processLogFile( childFiles[i] );
                }

            delete [] name;
            delete childFiles[i];
            }
        delete [] childFiles;
        }

    clearLiveRecords();

    for( int i=0; i<completeRecords.size(); i++ ) {
        DayRecord r = completeRecords.getElementDirect( i );
    
        printf( "%.0f %f\n", r.startTimeStamp, r.playAgainFraction );
        }
    
    }


        
