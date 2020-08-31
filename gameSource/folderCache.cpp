#include "folderCache.h"

#include "minorGems/formats/encodingUtils.h"


#include "minorGems/system/Time.h"


#include <stdio.h>
#include <stdlib.h>



static int stringSortCompare(const void *inA, const void * inB ) {
    
    return strcmp( *(char**)inA, *(char**)inB );
    }



FolderCache initFolderCache( const char *inFolderName, 
                             char *outRebuildingCache,
                             char (*inInclusionTest)( char *inFileName ),
                             char inForceRebuild ) {
    *outRebuildingCache = false;
    
    File *folderDir = new File( NULL, inFolderName );

    if( ! folderDir->exists() || ! folderDir->isDirectory() ) {
        
        FolderCache c = { folderDir, 0, NULL, NULL, NULL };
        
        return c;
        }
    
    File *cacheFile = folderDir->getChildFile( "cache.fcz" );
    
    if( inForceRebuild && cacheFile->exists() ) {
        printf( "Forcing remove/rebuild of cache.fcz in %s folder\n",
                inFolderName );

        cacheFile->remove();
        }


    FolderCache c;
    c.folderDir = folderDir;
    c.newDataBlock = new SimpleVector<char>();

    char cacheGood = false;

    if( cacheFile->exists() ) {

        int rawLength;
        unsigned char *rawCacheContents = 
            cacheFile->readFileContents( &rawLength );

        char *nextRawScanPointer = (char*)rawCacheContents;
            
        // don't use sscanf here because it scans the entire buffer
        // (and this buffer has binary data at end)
        int rawSize = scanIntAndSkip( &nextRawScanPointer );
        int compSize = scanIntAndSkip( &nextRawScanPointer );
            
        if( rawSize > 0 && compSize > 0 ) {
                
            unsigned char *compData = (unsigned char*)nextRawScanPointer;
                            
            double startTime = Time::getCurrentTime();
                
            unsigned char *rawData = zipDecompress( compData,
                                                    compSize,
                                                    rawSize );
            printf( "Decompressing took %f seconds\n", 
                    Time::getCurrentTime() - startTime );


            if( rawData != NULL ) {
                char *charData = new char[ rawSize + 1 ];
                    
                memcpy( charData, rawData, rawSize );
                    
                charData[rawSize] = '\0';

                delete [] rawData;
                    
                    
                char *nextScanPointer = charData;
                    
                c.numFiles = scanIntAndSkip( &nextScanPointer );
                    
                c.fileRecords = new CacheFileRecord[ c.numFiles ];
                    
                for( int i=0; i<c.numFiles; i++ ) {
                    c.fileRecords[i].file = NULL;

                    char *spacePos = strchr( nextScanPointer, ' ' );
                        
                    spacePos[0] = '\0';
                        
                    c.fileRecords[i].fileName = 
                        stringDuplicate( nextScanPointer );
                        
                    nextScanPointer = &( spacePos[1] );
                        
                    c.fileRecords[i].dataBlockOffset =
                        scanIntAndSkip( &nextScanPointer );
                        
                    c.fileRecords[i].length =
                        scanIntAndSkip( &nextScanPointer );
                    }

                char *dataBlockStart =
                    strstr( nextScanPointer, "#" );
                    
                    
                if( dataBlockStart != NULL ) {
                    dataBlockStart = &( dataBlockStart[ 1 ] );
                        
                    c.dataBlock = stringDuplicate( dataBlockStart );
                    }

                cacheGood = true;
                
                *outRebuildingCache = false;

                printf( "cache.fcz file in folder %s good with %d files\n",
                        inFolderName, c.numFiles );

                delete [] charData;
                }
            }
        delete [] rawCacheContents;
        }
        
    delete cacheFile;


    if( cacheGood ) {
        // make sure cache matches existing folder contents

        printf( "Checking that cache contains all of %s folder's "
                "current files...\n", inFolderName );

        double startTime = Time::getCurrentTime();

        int numChildFiles;
        File **childFiles = 
            folderDir->getChildFilesSorted( &numChildFiles );

        
        char **cacheFileNames = new char*[ c.numFiles ];
        
        for( int j=0; j<c.numFiles; j++ ) {
            cacheFileNames[j] = c.fileRecords[j].fileName;
            }

        
        qsort( cacheFileNames, c.numFiles, sizeof(char*), stringSortCompare );
        

        char cacheStale = false;

        // index into sorted cache
        int j = 0;
        
        for( int i=0; i<numChildFiles; i++ ) {
            char *fileName = childFiles[i]->getFileName();
    
            // skip our special cache data file
            if( ! childFiles[i]->isDirectory()
                &&
                // make sure file should be included
                inInclusionTest( fileName )
                &&
                strcmp( fileName, "cache.fcz" ) != 0 ) {

                
                if( j < c.numFiles ) {
                    if( strcmp( cacheFileNames[j], fileName ) != 0 ) {
                        cacheStale = true;
                        printf( 
                            "Cache file %s does not match folder file %s\n",
                            cacheFileNames[j], fileName );
                        delete [] fileName;
                        break;
                        }
                    }
                else {
                    cacheStale = true;
                    printf( 
                        "Folder file %s not present in cache.\n", fileName );
                    delete [] fileName;
                    break;
                    }
                j++;
                }
            delete [] fileName;
            }
        
        if( ! cacheStale && j < c.numFiles ) {
            printf( "Cache contains %d extra files not present in folder\n",
                    c.numFiles - j );
            cacheStale = true;
            }

        delete [] cacheFileNames;
        
        for( int i=0; i<numChildFiles; i++ ) {
            delete childFiles[i];
            }
        delete [] childFiles;
        
        if( ! cacheStale ) {
            printf( "Cache not stale -- " );
            }
        else {
            printf( "Cache stale, rebuilding -- " );
            }
        
        printf( "Checking took %f ms\n",
                1000 * ( Time::getCurrentTime() - startTime ) );
        
        
        if( cacheStale ) {
            // rebuild it
            
            // don't write out or free folderDir
            c.folderDir = NULL;
            
            freeFolderCache( c );
            
            c.folderDir = folderDir;
            c.newDataBlock = new SimpleVector<char>();
            
            cacheGood = false;
            }
        }
    
    
    

    
    if( !cacheGood ) {
        // cache stale or not present
        // read from raw files again
        
        *outRebuildingCache = true;

        int numChildFiles;
        
        File **childFiles = 
            folderDir->getChildFiles( &numChildFiles );
        

        SimpleVector<CacheFileRecord> records;
         
        for( int i=0; i<numChildFiles; i++ ) {
            char *fileName = childFiles[i]->getFileName();
            
            // skip our special cache data file
            if( ! childFiles[i]->isDirectory()
                &&
                // make sure file should be included
                inInclusionTest( fileName )
                &&
                strcmp( fileName, "cache.fcz" ) != 0 ) {
            
                CacheFileRecord r;
                r.fileName = NULL;
                r.file = childFiles[i];
                r.dataBlockOffset = -1;
                r.length = 0;
                
                records.push_back( r );
                }
            else {
                delete childFiles[i];
                }
            
            delete [] fileName;
            }
        
        c.dataBlock = NULL;
        c.numFiles = records.size();
        c.fileRecords = records.getElementArray();
        
        delete [] childFiles;
        
        }
    
    return c;
    }



char *getFileName( FolderCache inCache, int inFileNumber ) {
    if( inCache.dataBlock != NULL ) {
        return stringDuplicate( inCache.fileRecords[inFileNumber].fileName );
        }
    else {
        if( inCache.fileRecords[inFileNumber].fileName != NULL ) {
            delete [] inCache.fileRecords[inFileNumber].fileName;
            }
        inCache.fileRecords[inFileNumber].fileName =
            inCache.fileRecords[inFileNumber].file->getFileName();

        return stringDuplicate( inCache.fileRecords[inFileNumber].fileName );
        }
    }



char *getFileContents( FolderCache inCache, int inFileNumber ) {
    if( inCache.dataBlock != NULL ) {
        
        char *fileContents = 
            new char[ inCache.fileRecords[inFileNumber].length + 1 ];
        
        memcpy( fileContents, 
                &( inCache.dataBlock[ 
                       inCache.fileRecords[inFileNumber].dataBlockOffset ] ),
                inCache.fileRecords[inFileNumber].length );
        
        fileContents[ inCache.fileRecords[inFileNumber].length ] = '\0';
        
        return fileContents;
        }
    else {
        char *fileContents = 
            inCache.fileRecords[inFileNumber].file->readFileContents();
        
        if( inCache.fileRecords[inFileNumber].dataBlockOffset == -1 ) {
            // not in newDataBlock yet
            inCache.fileRecords[inFileNumber].length = strlen( fileContents );
            
            inCache.fileRecords[inFileNumber].dataBlockOffset =
                inCache.newDataBlock->size();
            
            inCache.newDataBlock->appendElementString( fileContents );
            }

        return fileContents;
        }
    }



// writes new cache to disk, based on read contents, as needed
void freeFolderCache( FolderCache inCache ) {
    if( inCache.dataBlock == NULL && 
        inCache.folderDir != NULL &&
        inCache.folderDir->exists() && 
        inCache.folderDir->isDirectory() ) {
        
        // write new cache out to file before freeing

        SimpleVector<char> uncompDataList;
        
        
        // only include files that actually had data read from them
        // other ones don't need to be cached
        SimpleVector<CacheFileRecord> usedRecords;
        
        for( int i=0; i<inCache.numFiles; i++ ) {
            if( inCache.fileRecords[i].fileName != NULL &&
                inCache.fileRecords[i].dataBlockOffset != -1 ) {
            
                usedRecords.push_back( inCache.fileRecords[i] );
                }
            }
        
        char *fileCount = autoSprintf( "%d\n", usedRecords.size() );
        
        uncompDataList.appendElementString( fileCount );
        delete [] fileCount;

        for( int i=0; i<usedRecords.size(); i++ ) {
            CacheFileRecord r = usedRecords.getElementDirect( i );
            
            uncompDataList.appendElementString( r.fileName );
        
            char *numbers = 
                autoSprintf( " %d %d\n",
                             r.dataBlockOffset, r.length );
            
            uncompDataList.appendElementString( numbers );
            
            delete [] numbers;
            }
        
        char *dataBlock = inCache.newDataBlock->getElementString();
        
        uncompDataList.push_back( '#' );
        
        uncompDataList.appendElementString( dataBlock );
        
        delete [] dataBlock;

        char *data = uncompDataList.getElementString();
        
        int rawLength = uncompDataList.size();
        
        double startTime = Time::getCurrentTime();
        
        int compSize;
        unsigned char *compData = zipCompress( (unsigned char*)data, 
                                               rawLength, &compSize );
        
        printf( "Compressing took %f seconds\n", 
                Time::getCurrentTime() - startTime );
        

        delete [] data;
        

        if( compData != NULL ) {
            
            File *cacheFile = inCache.folderDir->getChildFile( "cache.fcz" );
            
            char *path = cacheFile->getFullFileName();
            
            FILE *outFile = fopen( path, "wb" );
            
            if( outFile != NULL ) {
                
                fprintf( outFile, "%d %d ", rawLength, compSize );
                
                int numWritten = fwrite( compData, 1, compSize, outFile );
            
                fclose( outFile );
                
                if( numWritten != compSize ) {
                    printf( "Failed to write compressed data to file %s\n",
                            path );
                    
                    cacheFile->remove();
                    }
                }

            delete cacheFile;

            delete [] path;

            delete [] compData;
            }
        
        }
    

    for( int i=0; i<inCache.numFiles; i++ ) {
        if( inCache.fileRecords[i].fileName != NULL ) {
            delete [] inCache.fileRecords[i].fileName;
            }
        if( inCache.fileRecords[i].file != NULL ) {
            delete inCache.fileRecords[i].file;
            }
        
        }
    
    if( inCache.fileRecords != NULL ) {
        delete [] inCache.fileRecords;
        inCache.fileRecords = NULL;
        }
    
    if( inCache.dataBlock != NULL ) {
        delete [] inCache.dataBlock;
        inCache.dataBlock = NULL;
        }

    if( inCache.newDataBlock != NULL ) {
        delete inCache.newDataBlock;
        inCache.newDataBlock = NULL;
        }

    if( inCache.folderDir != NULL ) {
        delete inCache.folderDir;
        inCache.folderDir = NULL;
        }
    }


