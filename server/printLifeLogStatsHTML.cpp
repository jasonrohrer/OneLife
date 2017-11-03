#include <stdlib.h>


#include "minorGems/io/file/File.h"
#include "minorGems/util/stringUtils.h"


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
        double birthTime;
        char *email;
    } Living;

    
SimpleVector<Living> currentLiving;


typedef struct Player {
        char *email;
        int gameCount;
        int gameTotalSeconds;
        double firstGameTime;
        double lastGameTime;
        int lastGameSeconds;
    } Player;

SimpleVector<Player> allPlayers;

        

// stats
double totalAge = 0;
int totalLives = 0;
int longestFamilyChain = 0;

int over55Count = 0;



void addPlayerGame( char *inEmail, 
                    double inGameStartTime, double inGameEndTime ) {

    Player *thisPlayer;
    char found = false;
    
    for( int i=0; i < allPlayers.size(); i++ ) {
        thisPlayer = allPlayers.getElement( i );
        
        if( strcmp( thisPlayer->email, inEmail ) == 0 ) {
            found = true;
            break;
            }
        }
    
    if( !found ) {    
        // else add a new one
        Player newPlayer;

        newPlayer.email = stringDuplicate( inEmail );
        newPlayer.gameCount = 0;
        newPlayer.gameTotalSeconds = 0;
        newPlayer.firstGameTime = inGameEndTime;
        
        allPlayers.push_back( newPlayer );
        
        thisPlayer = allPlayers.getElement( allPlayers.size() - 1 );
        }
    
    int gameSeconds = lrint( inGameEndTime - inGameStartTime );

    thisPlayer->gameCount ++;
    thisPlayer->gameTotalSeconds += gameSeconds;
    thisPlayer->lastGameSeconds = gameSeconds;
    thisPlayer->lastGameTime = inGameEndTime;    
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
            
                Living l;
                l.id = id;
            
                l.birthAge = 1;
                l.parentChainLength = parentChain;

                l.birthTime = time;
                l.email = stringDuplicate( email );

                if( strcmp( parent, "noParent" ) == 0 ) {
                    l.birthAge = 14;
                    }
                else if( l.parentChainLength == 1 ) {
                    // parent chain length not recorded in log
                    // (old-style record)
                    
                    // try recomputing it from scratch
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
            
                double yearsLived = age;

                for( int i=0; i<currentLiving.size(); i++ ) {
                    Living l = currentLiving.getElementDirect( i );
                    
                    if( l.id == id && strcmp( l.email, email ) == 0 ) {
                        yearsLived -= l.birthAge;
                        
                        addPlayerGame( l.email, l.birthTime, time );

                        delete [] l.email;
                        currentLiving.deleteElement( i );
                        break;
                        }
                    }
            
                totalAge += yearsLived;
            
                if( age >= 55 ) {
                    over55Count++;
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




const char *checkpointFileName = "statsCheckpoint.txt";


void saveNewCheckpoint( File *inCheckpointFile,
                        char *inLastScannedFileName ) {
    
    char *fileName = inCheckpointFile->getFullFileName();
            
    FILE *f = fopen( fileName, "w" );
            
    if( f != NULL ) {
        fprintf( f, "%s %f %d %d %d", inLastScannedFileName,
                 totalAge, totalLives, 
                 longestFamilyChain, over55Count );
        fclose( f );
        }
    delete [] fileName;
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

        
        File *checkpointFile = mainDir.getChildFile( checkpointFileName );
        
        char lastScannedFileName[200];
        lastScannedFileName[0] = '\0';
        
        char checkpointFound = false;
        
        if( checkpointFile->exists() ) {
            char *fileName = checkpointFile->getFullFileName();
            
            FILE *f = fopen( fileName, "r" );
            
            if( f != NULL ) {
                fscanf( f, "%199s %lf %d %d %d", lastScannedFileName,
                        &totalAge, &totalLives, 
                        &longestFamilyChain, &over55Count );
                checkpointFound = true;
                fclose( f );
                }
            delete [] fileName;
            }
        

        char checkpointReached = false;
        
        
        int numFilesProcessed = 0;
        
        int numFiles;

        File **logs = mainDir.getChildFilesSorted( &numFiles );
        
        for( int i=0; i<numFiles; i++ ) {

            char *name = logs[i]->getFileName();
            
            if( strcmp( name, checkpointFileName ) != 0 ) {
            
                if( ! checkpointFound ||
                    checkpointReached ) {
                    
                    processLogFile( logs[i] );
                    
                    if( i < numFiles - 2 ) {
                        // very last file is saved checkpoint file
                        // second to last may still be getting filled by server
                        saveNewCheckpoint( checkpointFile, name );
                        }
                    
                    numFilesProcessed++;
                    }
                else if( checkpointFound && ! checkpointReached ) {
                    
                    if( strcmp( name, lastScannedFileName ) == 0 ) {
                        // this is where we got on last scan
                        checkpointReached = true;
                        }
                    }
                }       

            delete [] name;

            delete logs[i];
            }
        delete [] logs;


        delete checkpointFile;
        
        printf( "Processed %d files\n", numFilesProcessed );

        FILE *outFile = fopen( outPath, "w" );
        

        if( outFile != NULL ) {
            
            fprintf( outFile, 
                     "%d lives lived for a total of %0.2f hours<br>\n"
                     "%d people lived past age fifty-five<br>\n"
                     "%d generations in longest family line",
                     totalLives, totalAge / 60, over55Count,
                     longestFamilyChain );
            
            fclose( outFile );
            }

        for( int i=0; i<currentLiving.size(); i++ ) {
            Living l = currentLiving.getElementDirect( i );
            printf( "Orphaned birth that had no matching death:  %.0f %d %s\n",
                    l.birthTime, l.id, l.email );
            delete [] l.email;
            }
        

        printf( "\n\nMySQL query to kickstart stats database:\n\n" );
        
        for( int i=0; i<allPlayers.size(); i++ ) {
            Player p = allPlayers.getElementDirect( i );
            
            
            printf( 
                "INSERT INTO reviewServer_user_stats SET " 
                "email='%s', sequence_number=0, "
                "first_game_date=FROM_UNIXTIME( %.0f ), "
                "last_game_date=FROM_UNIXTIME( %.0f ), "
                "last_game_seconds=%d, game_count=%d, game_total_seconds=%d, "
                "review_score=-1, review_name='', review_text='', "
                "review_date=CURRENT_TIMESTAMP, review_game_seconds=0, "
                "review_game_count=0, review_votes=0;\n\n",
                p.email, p.firstGameTime, p.lastGameTime, p.lastGameSeconds,
                p.gameCount, p.gameTotalSeconds );

            delete [] p.email;
            }
        }
    else {
        usage();
        }
    
    }
