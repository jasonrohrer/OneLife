#include "minorGems/util/SimpleVector.h"

#include "minorGems/io/file/File.h"

#include "minorGems/formats/encodingUtils.h"



static SimpleVector<File*> modFileList;

static int currentModFileIndex;
static float currentModFileProgress;

static float perModFileProgressFraction;


static char currentModScanned;


// for current mod that we're working on loading
static SimpleVector<char *> scannedBlockHeaders;
        
static SimpleVector<int> scannedDataBlockLengths;
        
static SimpleVector<unsigned char*> scannedDataBlocks;

static int currentScannedBlock;

    



int initModLoaderStart() {
    File modDir( NULL, "mods" );
    
    if( ! modDir.exists() ) {
        modDir.makeDirectory();
        }
    
    if( modDir.exists() && modDir.isDirectory() ) {
        int numMods;
        File **mods = modDir.getChildFiles( &numMods );
        
        for( int i=0; i<numMods; i++ ) {
            modFileList.push_back( mods[i] );
            }
        }
    
    currentModFileIndex = 0;
    currentModFileProgress = 0;

    perModFileProgressFraction = 1.0;
    
    if( modFileList.size() > 0 ) {
        perModFileProgressFraction /= modFileList.size();
        }
    
    currentModScanned = false;


    // FIXME:
    
    // actually, just peek at header of each file here
    // to get total number of blocks to load

    return 0;
    }



static float getModLoadProgress() {
    return currentModFileProgress * perModFileProgressFraction +
        currentModFileIndex * perModFileProgressFraction;
    }

    

// returns progress... ready for Finish when progress == 1.0
float initModLoaderStep() {
    
    if( currentModFileIndex >= modFileList.size() ) {
        return 1.0f;
        }

    float scanProgressFraction = 0.25;
    float blockProcessingFraction = 1.0 - scanProgressFraction;

    if( ! currentModScanned ) {
        // scan it

        currentScannedBlock = 0;
        
        File *currentFile = modFileList.getElementDirect( currentModFileIndex );
        

        int dataSize;
        unsigned char *data = currentFile->readFileContents( &dataSize );
        
        if( data == NULL ) {
            currentModFileIndex ++;
            
            char *fileName = currentFile->getFileName();
            
            printf( "Failed to read from mod file %s\n", fileName );
            
            delete [] fileName;

            
            return getModLoadProgress();
            }

        char isZip = false;
        
        char *fileName = currentFile->getFileName();

        if( strstr( fileName, ".oxz" ) != NULL ) {
            isZip = true;
            }

        delete [] fileName;

        if( isZip ) {
            
            char scanSuccess = false;
            
            int plainDataLength =
                scanIntAndSkip( (char **)( &data ), &scanSuccess );
            
            if( ! scanSuccess ) {
                // failed to scan header size value
                delete [] data;

                char *fileName = currentFile->getFileName();

                printf( "Failed to scan zip header from mod file %s\n",
                        fileName );
                
                delete [] fileName;

                currentModFileIndex ++;
                
                return getModLoadProgress();
                }

            unsigned char *plainData =
                zipDecompress( data, dataSize, plainDataLength );
            
            delete [] data;
            
            if( plainData == NULL ) {
                
                char *fileName = currentFile->getFileName();

                printf( "Failed to unzip from mod file %s\n", fileName );
                
                delete [] fileName;

                currentModFileIndex ++;
                
                return getModLoadProgress();
                }
            

            data = plainData;
            dataSize = plainDataLength;
            }
        else {
            // not a zip file!
            // oxp files are not loadable (nor are other non .oxz files)
            delete [] data;
            
            
            char *fileName = currentFile->getFileName();
            
            printf( "Found unloadable file in mods folder: %s\n", fileName );
            
            delete [] fileName;
            
            currentModFileIndex ++;
            
            return getModLoadProgress();
            }
        
        
        // now have raw data

        // FIXME

        // scan each block and save it
        

        delete [] data;
        }
    else {
        // walk through blocks and process them

        // FIXME


        currentScannedBlock++;

        if( currentScannedBlock >= scannedBlockHeaders.size() ) {
            // done processing scanned blocks from current mod
            
            currentModFileProgress = 1.0;

            // free scanned data

            scannedBlockHeaders.deallocateStringElements();
            
            scannedDataBlockLengths.deleteAll();
            
            for( int i=0; i<scannedDataBlocks.size(); i++ ) {
                delete [] scannedDataBlocks.getElementDirect( i );
                }
            scannedDataBlocks.deleteAll();
            
            // prepare for scanning next mod, if there is one
            currentModFileIndex++;
            
            currentModScanned = false;
            }
        else {
            // done with another block from current mod
            
            currentModFileProgress = blockProcessingFraction *
                (float) currentScannedBlock / scannedBlockHeaders.size()
                + scanProgressFraction;
            }
        }
    
    

    
return getModLoadProgress();

    }




void initModLoaderFinish() {
    for( int i=0; i< modFileList.size(); i++ ) {
        delete modFileList.getElementDirect( i );
        }
    
    modFileList.deleteAll();
    }




void freeImporter() {
    // clean up, in case we got interrupted during loading

    for( int i=0; i< modFileList.size(); i++ ) {
        delete modFileList.getElementDirect( i );
        }
    
    modFileList.deleteAll();

    
    scannedBlockHeaders.deallocateStringElements();
            
    scannedDataBlockLengths.deleteAll();
    
    for( int i=0; i<scannedDataBlocks.size(); i++ ) {
        delete [] scannedDataBlocks.getElementDirect( i );
        }
    scannedDataBlocks.deleteAll();        
    }

