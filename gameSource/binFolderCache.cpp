
#include "binFolderCache.h"
#include "minorGems/formats/encodingUtils.h"


extern int versionNumber;


static char autoClear = true;


static char verifyCacheFile( FILE *inFile, int inNumFiles, 
                             int *outFailNumber ) {

    long oldPos = ftell( inFile );
    
    char fail = false;
    
    for( int i=0; i<inNumFiles; i++ ) {
        
        char nameBuffer[200];
        
        int numRead = fscanf( inFile, "%199s #", nameBuffer );
        
        if( numRead != 1 ) {
            fail = true;
            *outFailNumber = i;
            break;
            }
        
        int compSize, rawSize;
        
        numRead = fscanf( inFile, "%d %d#", &compSize, &rawSize );
        
        if( numRead != 2 ) {
            fail = true;
            *outFailNumber = i;
            break;
            }

        int seekResult = fseek( inFile, compSize, SEEK_CUR );
        
        if( seekResult != 0 ) {
            fail = true;
            *outFailNumber = i;
            break;
            }
        }

    
    // return to start pos
    fseek( inFile, oldPos, SEEK_SET );

    return !fail;
    }



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
    
    char *curCacheName = autoSprintf( "bin_v%d_cache.fcz", versionNumber );


    // clear any older version cache files
    int numChildFiles;
    File **childFiles = 
        folderDir->getChildFiles( &numChildFiles );
    
    for( int i=0; i<numChildFiles; i++ ) {
        char *name = childFiles[i]->getFileName();
        
        if( strcmp( name, curCacheName ) != 0 &&
            strstr( name, "bin_" ) != NULL &&
            strstr( name, "cache.fcz" ) != NULL ) {
            
            if( autoClear ) {
                printf( "Removing outdated bin_cache file:  %s\n", name );
                childFiles[i]->remove();
                }
            else {
                // different bin cache file discovered than
                // what we were expecting based on version number
                // use it!
                delete [] curCacheName;
                curCacheName = stringDuplicate( name );
                }
            }
        
        delete [] name;
        
        delete childFiles[i];
        }
    delete [] childFiles;



    File *cacheFile = folderDir->getChildFile( curCacheName );
    

    char cacheFileGood = false;
    

    if( cacheFile->exists() ) {
        char *cacheFileName = cacheFile->getFullFileName();
        
        c.cacheFile = fopen( cacheFileName, "rb" );
        
        int numRead = fscanf( c.cacheFile, "%d#", &( c.numFiles ) );

        if( numRead != 1 ) {
            printf( "Reading num files from %s failed, rebuilding\n",
                    curCacheName );
            fclose( c.cacheFile );
            }
        else {

            printf( "Opened a %s from the %s folder with %d files\n",
                    curCacheName, inFolderName, c.numFiles );
            
            int failNumber = 0;

            if( ! verifyCacheFile( c.cacheFile, c.numFiles, &failNumber ) ) {
                printf( "Cache file verification for %s failed on file %d/%d, "
                        "rebuilding\n",
                        curCacheName,
                        failNumber, c.numFiles );
                fclose( c.cacheFile );
                }
            else {
                cacheFileGood = true;
                }
            }

        delete [] cacheFileName;
        }

    
    if( !cacheFileGood ) {
        *outRebuildingCache = true;
        
        int numChildFiles;
        
        File **childFiles = 
            folderDir->getChildFiles( &numChildFiles );
        
        c.numFiles = numChildFiles;
        
        int numPatternParts = 0;
        char **patternParts = split( inPattern, "|", &numPatternParts );

        for( int i=0; i<numChildFiles; i++ ) {
            char *name = childFiles[i]->getFileName();
            
            char matchedPattern = false;

            for( int j=0; j<numPatternParts; j++ ) {
                if( strstr( name, patternParts[j] ) != NULL ) {
                    c.dirFiles->push_back( childFiles[i] );
                    matchedPattern = true;
                    break;
                    }
                }
            
            if( !matchedPattern ) {
                delete childFiles[i];
                }
            delete [] name;
            }
        delete [] childFiles;
        
        
        for( int j=0; j<numPatternParts; j++ ) {
            delete [] patternParts[j];
            }
        delete [] patternParts;
        

        c.numFiles = c.dirFiles->size();

        char *cacheFileName = cacheFile->getFullFileName();
        
        c.cacheFile = fopen( cacheFileName, "wb" );
        
        fprintf( c.cacheFile, "%d#", c.numFiles );
        delete [] cacheFileName;
        }

    delete [] curCacheName;


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




void clearAllBinCacheFiles( File *inFolder ) {
    int numChildFiles;
    File **childFiles = 
        inFolder->getChildFiles( &numChildFiles );
    
    for( int i=0; i<numChildFiles; i++ ) {
        char *name = childFiles[i]->getFileName();
        
        if( strstr( name, "bin_" ) != NULL &&
            strstr( name, "cache.fcz" ) != NULL ) {
            
            childFiles[i]->remove();
            }
        
        delete [] name;
        
        delete childFiles[i];
        }
    delete [] childFiles;
    }




void setAutoClearOldBinCacheFiles( char inAutoClear ) {
    autoClear = inAutoClear;
    }




