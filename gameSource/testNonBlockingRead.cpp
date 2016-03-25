// compile with
// g++ -o testNonBlockingRead testNonBlockingRead.cpp -I../.. ../../minorGems/io/file/linux/PathLinux.o


// make big file with:
// dd if=/dev/zero of=20MegFile bs=20M count=1


#include "minorGems/io/file/File.h"

#include <fcntl.h>

int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs < 2 ) {
        printf( "Must provide file name to read\n" );
        return 0;
        }
    
    unsigned char buffer[4096];
    
    if( false ) {
        
        FILE *file = fopen( inArgs[1], "rb" );
        
        if( file != NULL ) {

            //for( int i=0; i<2000; i++ ) {    
            int count = fread( buffer, 1, 4096, file );

            printf( "Read %d bytes\n", count );
                //    }
            
            fclose( file );
            }
        }
    else {
        int fd = open( inArgs[1], ( O_RDONLY | O_NONBLOCK ) );
        
        if( fd > 0 ) {
            //FILE *file = fdopen( fd, "rb" );
            
            //if( file != NULL ) {

            //int count = read( buffer, 1, 4096, file );
            int count = read( fd, buffer, 4096 );
                
            printf( "Read %d bytes\n", count );

            //fclose( file );
            //    }
            }
        }
    
    /*
    File f( NULL, inArgs[1]  );
    
    int size;
    unsigned char *contents = f.readFileContents( &size );
    
    delete [] contents;
    */
    return 0;
    }
