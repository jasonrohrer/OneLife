/*
 compile with 

g++ -g -I../.. -o testFolderCache testFolderCache.cpp folderCache.cpp \
../../minorGems/io/file/linux/PathLinux.cpp \
../../minorGems/formats/encodingUtils.cpp \
../../minorGems/util/stringUtils.cpp \
../../minorGems/system/unix/TimeUnix.cpp
*/


#include "folderCache.h"

#include "minorGems/system/Time.h"


int main() {

    double startTime = Time::getCurrentTime();

    FolderCache c = initFolderCache( "objects" );

    printf( "Init took %f seconds\n", 
            Time::getCurrentTime() - startTime );


    for( int i=0; i<c.numFiles; i++ ) {
        char *name = getFileName( c, i );
        
        char *contents = 
            getFileContents( c, i );

        if( false )printf( "Cache reading file %s, length %d\n", 
                name, strlen( contents ) );

        delete [] name;
        delete [] contents;
        }
    freeFolderCache( c );
    
    return 1;
    }
