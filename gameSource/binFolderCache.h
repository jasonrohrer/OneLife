#include "minorGems/io/file/File.h"
#include "minorGems/util/SimpleVector.h"


typedef struct BinFolderCache {

        SimpleVector<File*> *dirFiles;
        
        int numFiles;
        FILE *cacheFile;

    } BinFolderCache;



BinFolderCache initBinFolderCache( const char *inFolderName,
                                   // example:  ".tga"
                                   // multiple patterns can be separated
                                   // by | like:  ".aiff|.ogg"
                                   const char *inFilePattern,
                                   char *outRebuildingCache );


// note these to calls are expected to be called on files in order
// if they are cached

// if they aren't cached, these can be called out of order, and skip
// reading data for certain files

// both results destroyed by caller
char *getFileName( BinFolderCache inCache, int inFileNumber );

unsigned char *getFileContents( BinFolderCache inCache, int inFileNumber, 
                                char *inFileName, int *outLen );


// writes new cache to disk, based on read contents, as needed
void freeBinFolderCache( BinFolderCache inCache );



void clearAllBinCacheFiles( File *inFolder );



// defaults to true
void setAutoClearOldBinCacheFiles( char inAutoClear );
