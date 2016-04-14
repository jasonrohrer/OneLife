
#include "minorGems/io/file/File.h"
#include "minorGems/util/SimpleVector.h"


typedef struct CacheFileRecord {
        char *fileName;
        
        File *file;

        int dataBlockOffset;
        int length;
    } FolderFileRecord;


typedef struct FolderCache {
        
        File *folderDir;
        
        int numFiles;

        FolderFileRecord *fileRecords;
        
        char *dataBlock;
        
        SimpleVector<char> *newDataBlock;

    } FolderCache;



FolderCache initFolderCache( const char *inFolderName,
                             char *outRebuildingCache );


// both results destroyed by caller
char *getFileName( FolderCache inCache, int inFileNumber );

char *getFileContents( FolderCache inCache, int inFileNumber );


// writes new cache to disk, based on read contents, as needed
void freeFolderCache( FolderCache inCache );

