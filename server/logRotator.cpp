#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>


void usage() {
    printf( "Usage:\n" );
    printf( "logRotator B_per_log logFileName\n\n" );
    
    printf( "Reads from stdin and writes to logFileName, saving one "
            ".bak file every time the file gets bigger than B_per_log.\n\n" );

    printf( "Example:\n" );
    printf( "logRotator 20000000 serverOut.txt \n\n" );
    
    exit( 1 );
    }



static int getFileLength( char *inName ) {
    
    struct stat fileInfo;
	
	// get full file name
	int length;
	
	int statError = stat( inName, &fileInfo );
	
	if( statError == 0 ) {
		return fileInfo.st_size;
        }
    else {
        return 0;
        }
    }



int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs != 3 ) {
        usage();
        }
    
    int sizeB = 10000000;
    
    int numRead = sscanf( inArgs[1], "%d", &sizeB );
    
    if( numRead != 1 ) {
        usage();
        }

    char *logFileName = inArgs[2];
    
    char bakName[200];
    
    snprintf( bakName, 199, "%s.bak", logFileName );

    int bytesWritten = getFileLength( logFileName );

    FILE *logFile = fopen( logFileName, "a" );
    
    if( logFile == NULL ) {
        usage();
        }
    
    char buffer[16];
    
    int stillGoing = true;
    while( stillGoing ) {
        stillGoing = false;
        int numRead  = read( STDIN_FILENO, buffer, 16 );
    
        if( numRead > 0 ) {
            fwrite( buffer, 1, numRead, logFile );    
            fflush( logFile );
            
            stillGoing = true;
            

            bytesWritten += numRead;
            
            if( bytesWritten >= sizeB ) {
                bytesWritten = 0;
                
                fclose( logFile );
                logFile = NULL;
                
                rename( logFileName, bakName );
                
                logFile = fopen( logFileName, "w" );

                if( logFile == NULL ) {
                    stillGoing = false;
                    }
                }
            }

        }
    
        
    if( logFile != NULL ) {
        fclose( logFile );
        }
    


    return 0;
    }
