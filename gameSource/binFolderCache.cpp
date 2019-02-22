
#include "binFolderCache.h"
#include "minorGems/formats/encodingUtils.h"



BinFolderCache initBinFolderCache( const char *inFolderName,
                                   const char *inPattern,
                                   char *outRebuildingCache ) {
    
    *outRebuildingCache = false;
    
    File *folderDir = new File( NULL, inFolderName );

    SimpleVector<File*> *dirFiles = new SimpleVector<File*>();

    BinFolderCache c = { dirFiles, 0, NULL };


    if( ! folderDir->exists() || ! folderDir->isDirectory() ) {
        
        delete folderDir;

        return c;
        }
    
    File *cacheFile = folderDir->getChildFile( "bin_cache.fcz" );

    if( cacheFile->exists() ) {
        char *cacheFileName = cacheFile->getFullFileName();
        
        c.cacheFile = fopen( cacheFileName, "r" );
        
        fscanf( c.cacheFile, "%d#", &( c.numFiles ) );

        printf( "Opened a bin_cache.fcz from the %s folder with %d files\n",
                inFolderName, c.numFiles );

        delete [] cacheFileName;
        }
    else {
        *outRebuildingCache = true;
        
        int numChildFiles;
        
        File **childFiles = 
            folderDir->getChildFiles( &numChildFiles );
        
        c.numFiles = numChildFiles;
        
        for( int i=0; i<numChildFiles; i++ ) {
            char *name = childFiles[i]->getFileName();
            
            if( strstr( name, inPattern ) != NULL ) {
                c.dirFiles->push_back( childFiles[i] );
                }
            delete [] name;
            }
        delete [] childFiles;

        c.numFiles = c.dirFiles->size();

        char *cacheFileName = cacheFile->getFullFileName();
        
        c.cacheFile = fopen( cacheFileName, "w" );
        
        fprintf( c.cacheFile, "%d#", c.numFiles );
        delete [] cacheFileName;
        }

    delete cacheFile;
    delete folderDir;
    
    return c;
    }



// both results destroyed by caller
char *getFileName( BinFolderCache inCache, int inFileNumber ) {
    if( inCache.dirFiles->size() > inFileNumber ) {
        return inCache.dirFiles->
            getElementDirect( inFileNumber )->getFileName();
        }
    else if( inCache.dirFiles->size() > 0 ) {
        return NULL;
        }
    else {
        // cached?
        // we can read the file name from cache        
        char nameBuffer[200];
        
        int numRead = fscanf( inCache.cacheFile, "%199s #", nameBuffer );
        
        if( numRead != 1 ) {
            return NULL;
            }
        
        return stringDuplicate( nameBuffer );
        }
    }


unsigned char *getFileContents( BinFolderCache inCache, int inFileNumber, 
                                char *inFileName, int *outLen ) {
    if( inCache.dirFiles->size() > inFileNumber ) {
        int rawLen;
        unsigned char *rawBuff =
            inCache.dirFiles->
            getElementDirect( inFileNumber )->readFileContents( &rawLen );

        int compLen;
        unsigned char *compBuff = 
            zipCompress( rawBuff, rawLen, &compLen );
        
        if( compBuff == NULL ) {
            delete [] rawBuff;
            return NULL;
            }
        // build cache file
        fprintf( inCache.cacheFile, "%s #%d %d#", inFileName, compLen, rawLen );
        
        int numWritten = fwrite( compBuff, 1, compLen, inCache.cacheFile );

        delete [] compBuff;
        
        if( numWritten != compLen ) {
            delete [] rawBuff;
            return NULL;
            }
        *outLen = rawLen;
        return rawBuff;
        }
    else if( inCache.dirFiles->size() > 0 ) {
        return NULL;
        }
    else {
        // cached?
        // we can read comp/decomp size from cache        
        
        int compSize, rawSize;
        
        int numRead = fscanf( inCache.cacheFile, "%d %d#", 
                              &compSize, &rawSize );
        
        if( numRead != 2 ) {
            return NULL;
            }
        
        unsigned char *compBuff = new unsigned char[ compSize ];
        
        numRead = fread( compBuff, 1, compSize, inCache.cacheFile );
        
        if( numRead != compSize ) {
            delete [] compBuff;
            return NULL;
            }
        
        
        unsigned char *rawBuff = zipDecompress( compBuff, compSize, rawSize );
        
        delete [] compBuff;

        if( rawBuff != NULL ) {
            *outLen = rawSize;
            return rawBuff;
            }
        
        return NULL;
        }
    }



// writes new cache to disk, based on read contents, as needed
void freeBinFolderCache( BinFolderCache inCache ) {

    if( inCache.dirFiles != NULL ) {
        for( int i=0; i<inCache.dirFiles->size(); i++ ) {
            delete inCache.dirFiles->getElementDirect( i );
            }
        delete inCache.dirFiles;
        }

    if( inCache.cacheFile != NULL ) {
        fclose( inCache.cacheFile );
        }
    
    }

