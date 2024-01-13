#include "minorGems/util/SimpleVector.h"

#include "minorGems/io/file/File.h"

#include "minorGems/formats/encodingUtils.h"
#include "minorGems/graphics/RGBAImage.h"


#include "exporter.h"

#include "spriteBank.h"
#include "soundBank.h"
#include "objectBank.h"
#include "animationBank.h"



static SimpleVector<File*> loadFileList;

static int currentLoadFileIndex;


static char currentLoadScanned;


// for current mod/import that we're working on loading
static SimpleVector<char *> scannedLoadBlockHeaders;
        
static SimpleVector<int> scannedLoadDataBlockLengths;
        
static SimpleVector<unsigned char*> scannedLoadDataBlocks;

static int currentScannedLoadBlock;

// these are the object that were actually inserted into our objectBank
SimpleVector<int> scannedLoadObjectActuallyInserted;

// these are maintained by animationBank
SimpleVector<AnimationRecord *> scannedLoadAnimationsActuallyInserted;




typedef struct IDMapEntry {
        int loadedID;
        int bankID;
    } IDMapEntry;


// maps IDs in mod/import file to live IDs in our banks
// (in some cases, we may add resources to our banks, in other cases
//  we may find resources with the same hashes)
static SimpleVector<IDMapEntry> spriteIDMap;
static SimpleVector<IDMapEntry> soundIDMap;


// this map only used for ADD mode when importing
// we remap each object to a new ID (not replacing existing IDs)
// so we need to remap our animations which load later
static SimpleVector<IDMapEntry> objectIDMap;



// returns NULL if not found
static IDMapEntry* idMapLookup( SimpleVector<IDMapEntry> *inMap,
                                int inLoadedID ) {
    for( int i=0; i< inMap->size(); i++ ) {
        IDMapEntry *e = inMap->getElement( i );
        
        if( e->loadedID == inLoadedID ) {
            return e;
            }
        }
    return NULL;    
    }



static int applyMap( SimpleVector<IDMapEntry> *inMap,
                     int inLoadedID ) {
    IDMapEntry *e = idMapLookup( inMap, inLoadedID );
    
    if( e != NULL ) {
        return e->bankID;
        }
    
    // else no map found
    
    // return an invalid ID
    return -1;
    }



static void remapSounds( SoundUsage *inUsage ) {
    for( int i=0; i< inUsage->numSubSounds; i++ ) {
        inUsage->ids[i] = applyMap( &soundIDMap, inUsage->ids[i] );
        }
    }



static char *remapSounds( char *inUsageString ) {
    SoundUsage u = scanSoundUsage( inUsageString );

    remapSounds( &u );    
    
    char *usageString = stringDuplicate( printSoundUsage( u ) );

    clearSoundUsage( &u );

    return usageString;
    }



static void freeScannedData() {    
    scannedLoadBlockHeaders.deallocateStringElements();
    
    scannedLoadDataBlockLengths.deleteAll();
    
    for( int i=0; i<scannedLoadDataBlocks.size(); i++ ) {
        delete [] scannedLoadDataBlocks.getElementDirect( i );
        }
    scannedLoadDataBlocks.deleteAll();
    
    scannedLoadObjectActuallyInserted.deleteAll();
    scannedLoadAnimationsActuallyInserted.deleteAll();
    }



static int totalNumLoadBlocksToScan;

static int totalNumLoadBlocksScannedSoFar;







static int initLoaderStartInternal( const char *inDirName ) {

    File loadDir( NULL, inDirName );
    
    if( ! loadDir.exists() ) {
        loadDir.makeDirectory();
        }
    
    if( loadDir.exists() && loadDir.isDirectory() ) {
        int numLoads;
        File **loads = loadDir.getChildFiles( &numLoads );
        
        for( int i=0; i<numLoads; i++ ) {
            loadFileList.push_back( loads[i] );
            }
        delete [] loads;
        }
    
    currentLoadFileIndex = 0;
    
    currentLoadScanned = false;


    // peek at header of each file here
    // to get total number of blocks to load

    totalNumLoadBlocksToScan = 0;

    for( int i=0; i<loadFileList.size(); i++ ) {
        File *file = loadFileList.getElementDirect( i );
        
        char *name = file->getFullFileName();
        
        FILE *f = fopen( name, "rb" );
        
        if( f == NULL ) {
            printf( "Failed to open mod/import file for reading: %s\n", name );
            delete [] name;
            delete file;
            loadFileList.deleteElement( i );
            i--;
            continue;
            }
        
        int blocks = 0;
        int numRead = fscanf( f, "%d", &blocks );
        
        if( numRead != 1 ) {
            printf( "Failed to read header block count "
                    "from mod/import file: %s\n",
                    name );
            delete [] name;
            delete file;
            loadFileList.deleteElement( i );
            i--;
            continue;
            }
        
        delete [] name;

        totalNumLoadBlocksToScan += blocks;
        }
    
    totalNumLoadBlocksScannedSoFar = 0;
    
    printf( "Loader sees %d total blocks to scan from folder '%s'\n",
            totalNumLoadBlocksToScan, inDirName );

    return totalNumLoadBlocksToScan;
    }




int initModLoaderStart() {
    return initLoaderStartInternal( "mods" );
    }


int initImportAddStart() {
    return initLoaderStartInternal( "import_add" );
    }


int initImportReplaceStart() {
    return initLoaderStartInternal( "import_replace" );
    }



// inText destroyed by this call
// returns newly allocated string, destroyed by caller
static char *replaceIDLine( char *inText, int inOldID, int inNewID ) {

    char *lineToReplace = autoSprintf( "id=%d", inOldID );
    char *newLine = autoSprintf( "id=%d", inNewID );
    
    char found = false;
                            
    char *replacedString =
        replaceOnce( inText, lineToReplace,
                     newLine,
                     &found );
    
    delete [] lineToReplace;
    
    delete [] newLine;

    delete [] inText;
                    
    return replacedString;
    }



static float getLoadProgress() {
    return (float)totalNumLoadBlocksScannedSoFar / 
        (float)totalNumLoadBlocksToScan;
    }

    


// inReplaceObjects only observed if inSaveIntoDataDirs is true
// if inSaveIntoDataDirs is false, objects are always replaced
static float initLoaderStepInternal( char inSaveIntoDataDirs = false, 
                                     char inReplaceObjects = false ) {
    
    if( currentLoadFileIndex >= loadFileList.size() ) {
        return 1.0f;
        }

    if( ! currentLoadScanned ) {
        // scan it

        currentScannedLoadBlock = 0;
        
        File *currentFile =
            loadFileList.getElementDirect( currentLoadFileIndex );
        

        int dataSize;
        unsigned char *data = currentFile->readFileContents( &dataSize );
        
        if( data == NULL ) {
            currentLoadFileIndex ++;
            
            char *fileName = currentFile->getFileName();
            
            printf( "Failed to read from mod/import file %s\n", fileName );
            
            delete [] fileName;

            
            return getLoadProgress();
            }

        char isZip = false;
        
        char *fileName = currentFile->getFileName();

        if( strstr( fileName, ".oxz" ) != NULL ) {
            isZip = true;
            }

        delete [] fileName;

        if( isZip ) {
            
            char scanSuccess = false;
            
            unsigned char *dataWorking = data;
            // skip numBlocks at beginning of header
            scanIntAndSkip( (char **)( &dataWorking ), &scanSuccess );
            
            if( ! scanSuccess ) {
                // failed to scan header num blocks value
                delete [] data;

                char *fileName = currentFile->getFileName();

                printf( "Failed to scan block count header "
                        "from mod/import file %s\n",
                        fileName );
                
                delete [] fileName;

                currentLoadFileIndex ++;
                
                return getLoadProgress();
                }
            
            
            // this will skip the trailing # in the header too
            int plainDataLength =
                scanIntAndSkip( (char **)( &dataWorking ), &scanSuccess );
            
            // make sure # exists too, one char back from where we
            // ended our scanIntAndSkip
            if( ! scanSuccess || dataWorking[-1]  != '#' ) {

                // failed to scan header size value or trailing #
                delete [] data;

                char *fileName = currentFile->getFileName();

                printf( 
                    "Failed to scan decompressed size header "
                    "from mod/import file %s\n",
                    fileName );
                
                delete [] fileName;

                currentLoadFileIndex ++;
                
                return getLoadProgress();
                }
            
            // how far have we scanned ahead in data when skipping headers?
            int headerScanSize = (int)( dataWorking - data );
            

            unsigned char *plainData =
                zipDecompress( dataWorking,
                               // how much zipped data is left
                               // after headers scanned
                               dataSize - headerScanSize, 
                               plainDataLength );
            
            delete [] data;
            
            if( plainData == NULL ) {
                
                char *fileName = currentFile->getFileName();

                printf( "Failed to unzip from mod/import file %s\n", fileName );
                
                delete [] fileName;

                currentLoadFileIndex ++;
                
                return getLoadProgress();
                }
            

            data = plainData;
            dataSize = plainDataLength;
            }
        else {
            // not a zip file!
            // oxp files are not loadable (nor are other non .oxz files)
            delete [] data;
            
            
            char *fileName = currentFile->getFileName();
            
            printf( "Found unloadable file in mods/import folder: %s\n",
                    fileName );
            
            delete [] fileName;
            
            currentLoadFileIndex ++;
            
            return getLoadProgress();
            }
        
        
        // now have raw data


        // scan each block and save it
        
        // gather object ids as we go along
        SimpleVector<int> scannedObjectIDs;

        int dataPos = 0;
        
        int blockCount = 0;
        
        while( dataPos < dataSize ) {
            SimpleVector<char> scannedHeader;
            
            int headerStartPos = dataPos;

            while( data[ dataPos ] != '#' &&
                   dataPos < dataSize ) {
                
                scannedHeader.push_back( (char)( data[ dataPos ] ) );
                
                dataPos++;
                }
            
            if( data[dataPos] != '#' ) {
                printf( "Failed to find end of header when "
                        "scanning mod/import block %d\n", blockCount );
                
                break;
                }
            
            // terminate string
            data[dataPos] = '\0';
            
            char *header = stringDuplicate( (char*)( & data[headerStartPos] ) );
            
            if( strstr( header, "object " ) == header ) {
                // header is an object header
                
                int id = -1;
                sscanf( header, "object %d", &id );
                
                if( id != -1 ) {
                    scannedObjectIDs.push_back( id );
                    }
                }    
            
            int spacePos = dataPos;
            
            // walk backward to find space before end of header
            while( data[spacePos] != ' ' &&
                   spacePos >= 0 ) {
                
                spacePos--;
                }
            
            if( data[spacePos] != ' ' ) {
                printf( "Failed to find space near end of header when "
                        "scanning mod/import block %d\n", blockCount );
                
                delete [] header;
                break;
                }
            
            int blockDataLength = -1;
            
            sscanf( (char*)( & data[spacePos] ), "%d", &blockDataLength );
            
            if( blockDataLength == -1 ) {
                printf( "Failed to find block data length in header when "
                        "scanning mod/import block %d\n", blockCount );
                
                delete [] header;
                break;
                }
            

            // skip # in data (which has been replaced with \0 )
            dataPos++;
            
            if( dataSize - dataPos < blockDataLength ) {
                printf( "Block %d in mod/import file specifies data "
                        "length of %d "
                        "when there's only %d bytes left in data\n",
                        blockCount, blockDataLength, dataSize - dataPos );
                
                delete [] header;
                break;
                }
            
            unsigned char *blockData = new unsigned char[ blockDataLength ];
            
            memcpy( blockData, & data[ dataPos ], blockDataLength );
            

            scannedLoadBlockHeaders.push_back( header );
            scannedLoadDataBlockLengths.push_back( blockDataLength );
            scannedLoadDataBlocks.push_back( blockData );

            // skip the data, now that we've scanned it.
            dataPos += blockDataLength;
            
            blockCount++;
            }
        
        delete [] data;
        
        printf( "Scanned %d blocks from mod/import file\n", 
                scannedLoadBlockHeaders.size() );
        
        
        // compute hashes of object IDs

        char *correctHash = getObjectIDListHash( & scannedObjectIDs );
        
        fileName = currentFile->getFileName();

        char *extensionPos = strstr( fileName, ".oxz" );
        
        if( extensionPos != NULL ) {
            // terminate at .
            extensionPos[0] = '\0';
            }

        if( // extension not found
            extensionPos == NULL ||
            // OR stuff before extension too short for 6-letter hash
            ( extensionPos - fileName ) < 6 ||
            // OR six letters before extension do not match correct hash
            strcmp( correctHash, & extensionPos[ -6 ] ) != 0 ) {

            printf( "Hash check for mod/import file "
                    "failed (correct hash=%s): %s\n",
                    correctHash, & extensionPos[ -6 ] );
            
            delete [] correctHash;
            delete [] fileName;

            // dump this data
            freeScannedData();
            
            // move on to the next mod/import file
            currentLoadFileIndex ++;
            
            return getLoadProgress();
            }

        // hash check okay
        printf( "Hash for mod/import computed as %s, "
                "matches file name hash in %s\n",
                correctHash, fileName );
        delete [] correctHash;
        delete [] fileName;

        currentLoadScanned = true;
        }
    else {
        // walk through blocks and process them
        
        char *currentHeader = 
            scannedLoadBlockHeaders.getElementDirect( currentScannedLoadBlock );
        
        int currentDataLength = 
            scannedLoadDataBlockLengths.getElementDirect( 
                currentScannedLoadBlock );
        
        unsigned char *currentDataBlock =
            scannedLoadDataBlocks.getElementDirect( currentScannedLoadBlock );
        
        if( strstr( currentHeader, "sprite " ) == currentHeader ) {
            
            char blockType[100];
            int id = -1;
            char tag[100];
            tag[0] = '\0';
            int multiplicativeBlend = 0;
            int centerAnchorXOffset = 0;
            int centerAnchorYOffset = 0;
            
            sscanf( currentHeader, "%99s %d %99s %d %d %d",
                    blockType, &id, tag, &multiplicativeBlend, 
                    &centerAnchorXOffset,
                    &centerAnchorYOffset );
            
            if( id > -1 ) {
                // header at least contained an ID                

                if( strstr( tag, "_HASH_" ) ) {
                    // restore hashes in tag from escape sequences
                    
                    char found;
                    char *tagWithHash = 
                        replaceAll( tag, "_HASH_", "#", &found );
                    
                    memcpy( tag, tagWithHash, strlen( tagWithHash ) + 1 );
                    
                    delete [] tagWithHash;
                    }
                
                

                int bankID = 
                    doesSpriteRecordExist(
                        currentDataLength,
                        currentDataBlock,
                        tag,
                        multiplicativeBlend,
                        centerAnchorXOffset, centerAnchorYOffset );
                
                if( bankID == -1 ) {
                    // no sprite found, need to add a new one to bank

                    if( inSaveIntoDataDirs ) {                        
                        RawRGBAImage *rawImage = 
                            readTGAFileRawFromBuffer( 
                                currentDataBlock, 
                                currentDataLength );
                        
                        if( rawImage != NULL ) {
                            SpriteHandle sprite =
                                fillSprite( rawImage );
                            
                            Image *im = RGBAImage::getImageFromBytes( 
                                rawImage->mRGBABytes,
                                rawImage->mWidth, rawImage->mHeight,
                                rawImage->mNumChannels );
                            
                            delete rawImage;

                            bankID = addSprite( tag, 
                                                sprite, 
                                                im,
                                                multiplicativeBlend,
                                                centerAnchorXOffset,
                                                centerAnchorYOffset );
                            delete im;
                            }
                        }
                    else {    
                        // add a new sprite into RAM only
                        // don't save to disk
                        bankID = 
                            addSprite( tag, currentDataBlock, currentDataLength,
                                       multiplicativeBlend,
                                       centerAnchorXOffset,
                                       centerAnchorYOffset );
                        }
                    
                    if( bankID == -1 ) {
                        printf( "Loading sprite TGA data from data block "
                                "with %d bytes from mod/import failed\n",
                                currentDataLength );
                        }
                    }
                
                if( bankID != -1 &&
                    idMapLookup( &spriteIDMap, id ) == NULL ) {
                    IDMapEntry e = { id, bankID };
                    spriteIDMap.push_back( e );
                    }
                }
            }
        else if( strstr( currentHeader, "sound " ) == currentHeader ) {
                        
            char blockType[100];
            int id = -1;
            char soundType[100];
            
            sscanf( currentHeader, "%99s %d %99s",
                    blockType, &id, soundType );
            
            if( id > -1 ) {
                // header at least contained an ID

                int bankID = 
                    doesSoundRecordExist(
                        currentDataLength,
                        currentDataBlock );
                
                if( bankID == -1 ) {
                    // no sound found, need to add a new one to bank
                    
                    bankID = addSoundToBank( currentDataLength,
                                             currentDataBlock,
                                             soundType,
                                             inSaveIntoDataDirs );
                    
                    if( bankID == -1 ) {
                        printf( "Loading sound data from data block "
                                "with %d bytes, type %s, "
                                "from mod/import failed\n",
                                currentDataLength, soundType );
                        }
                    }
                else {
                    printf( "Mod/import sound id %d exists in bank as ID %d\n",
                            id, bankID );
                    }
                
                if( bankID != -1 &&
                    idMapLookup( &soundIDMap, id ) == NULL ) {
                    IDMapEntry e = { id, bankID };
                    soundIDMap.push_back( e );
                    printf( "Mapping mod/import sound id %d to bank id %d\n",
                            id, bankID );
                    }
                }
            }
        else if( strstr( currentHeader, "object " ) == currentHeader ) {
            char blockType[100];
            int id = -1;
            
            sscanf( currentHeader, "%99s %d", blockType, &id );
            
            if( id > -1 ) {
                // header at least contained an ID
                
                if( inSaveIntoDataDirs ) {
                    // copy data block into a string for parsing
                    char *objectString = new char[ currentDataLength + 1 ];
                    memcpy( objectString, 
                            currentDataBlock, currentDataLength );
                    objectString[ currentDataLength ] = '\0';
                    
                    int numLines = 0;
                    
                    char **objectLines = split( objectString, "\n", &numLines );
                    
                    delete [] objectString;

                    for( int i=0; i<numLines; i++ ) {
                        
                        char *line = objectLines[i];
                        
                        if( strstr( line, "sounds=" ) == line ) {
                            // sounds line in object text

                            int numSoundParts = 0;
                            // skip sounds= header in line
                            char **soundParts = split( &( line[7] ), ",", 
                                                       &numSoundParts );
                            
                            for( int j=0; j<numSoundParts; j++ ) {
                                char *newPart = remapSounds( soundParts[j] );
                                
                                delete [] soundParts[j];
                                
                                soundParts[j] = newPart;
                                }
                            
                            char *newSoundsString = 
                                join( soundParts, numSoundParts, "," );
                            
                            for( int j=0; j<numSoundParts; j++ ) {
                                delete [] soundParts[j];
                                }
                            delete [] soundParts;
                            

                            delete [] objectLines[i];

                            objectLines[i] = autoSprintf( "sounds=%s",
                                                          newSoundsString );
                            delete [] newSoundsString;
                            }
                        else  if( strstr( line, "spriteID=" ) == line ) {
                            // sprite line in object text
                            
                            int scannedID = -1;
                            sscanf( line, "spriteID=%d", &scannedID );
                            
                            if( scannedID != -1 ) {
                                
                                int mappedID = applyMap( &spriteIDMap,
                                                         scannedID );
                                
                                delete [] objectLines[i];
                                objectLines[i] = autoSprintf( "spriteID=%d",
                                                              mappedID );
                                }
                            }
                        }
                    
                    char *newObjectString = 
                        join( objectLines, numLines, "\n" );
                    
                    for( int i=0; i<numLines; i++ ) {
                        delete [] objectLines[i];
                        }
                    delete [] objectLines;
                    
                    File objectsDir( NULL, "objects" );
                
                    if( objectsDir.exists() && objectsDir.isDirectory() ) {
                        File *nextObjNumFile = 
                            objectsDir.getChildFile( "nextObjectNumber.txt" );
                        
                        int nextObjectNumber =
                            nextObjNumFile->readFileIntContents( -1 );
                        
                        if( nextObjectNumber < 1 ) {
                            nextObjectNumber = 1;
                            }

                        int idToWrite = id;
                
                        if( ! inReplaceObjects ) {
                            idToWrite = nextObjectNumber;
                            }
                        
                        if( idToWrite >= nextObjectNumber ) {
                            nextObjectNumber = idToWrite + 1;
                            
                            nextObjNumFile->writeToFile( nextObjectNumber );
                            }
                        
                        delete nextObjNumFile;
                        

                        char *fileName = autoSprintf( "%d.txt", idToWrite );
                        
                        File *objFile = objectsDir.getChildFile( fileName );
                        
                        delete [] fileName;
                        
                        if( id != idToWrite ) {
                            // replace ID line in object file data
                            newObjectString = 
                                replaceIDLine( newObjectString, id, idToWrite );
                            }


                        objFile->writeToFile( newObjectString );
                        
                        delete objFile;

                        
                        // clear cache
                        File *cacheFile =
                            objectsDir.getChildFile( "cache.fcz" );
                        
                        cacheFile->remove();
                        
                        delete cacheFile;

                        
                        if( idMapLookup( &objectIDMap, id ) == NULL ) {
                            IDMapEntry e = { id, idToWrite };
                            objectIDMap.push_back( e );
                            }
                        }
                    
                    delete [] newObjectString;
                    }
                else {
                    // only replace in live bank
                    // don't save to disk

                    // don't accept default object here
                    ObjectRecord *existingRecord = getObject( id, true );
                
                    if( existingRecord != NULL ) {
                    
                        // copy data block into a string for parsing
                        char *objectString = new char[ currentDataLength + 1 ];
                        memcpy( objectString, 
                                currentDataBlock, currentDataLength );
                        objectString[ currentDataLength ] = '\0';
                        
                        
                        ObjectRecord *modRecord =
                            scanObjectRecordFromString( objectString );
                    
                        delete [] objectString;
                    
                        if( modRecord != NULL ) {
                        
                            // remap sprites in our mod object
                            for( int i=0; i< modRecord->numSprites; i++ ) {
                                modRecord->sprites[i] = 
                                    applyMap( &spriteIDMap,
                                              modRecord->sprites[i] );
                                }
                            // same for sounds
                            remapSounds( & modRecord->creationSound );
                            remapSounds( & modRecord->usingSound );
                            remapSounds( & modRecord->eatingSound );
                            remapSounds( & modRecord->decaySound );
                        
                            // apply all visual/sound changes from our
                            // mod record into the object bank object
                            copyObjectAppearance( id, modRecord );
                        
                            freeObjectRecord( modRecord );
                        
                            scannedLoadObjectActuallyInserted.push_back( id );
                        
                            // also track useDummy and variableOummies
                            ObjectRecord *o = getObject( id );
                        
                            if( o->useDummyIDs != NULL ) {
                                for( int i=0; i< o->numUses - 1; i++ ) {
                                    scannedLoadObjectActuallyInserted.push_back(
                                        o->useDummyIDs[ i ] );
                                    }
                                }
                            if( o->variableDummyIDs != NULL ) {
                                for( int i=0; i< o->numVariableDummyIDs; i++ ) {
                                    scannedLoadObjectActuallyInserted.push_back(
                                        o->variableDummyIDs[ i ] );
                                    }
                                }
                            }
                        else {
                            printf( "Parsing object from mod/import failed\n" );
                            }
                        }
                    else {
                        printf( "Mod object id %d not found in object bank, "
                                "skipping\n", id );
                        }
                    }
                }
            }
        else if( strstr( currentHeader, "anim " ) == currentHeader ) {
            char blockType[100];
            int id = -1;
            char animSlot[100];
            
            sscanf( currentHeader, "%99s %d %99s", blockType, &id, animSlot );
            
            if( id > -1 ) {
                // header at least contained an ID
                
                int slotNumber = -1;
                
                char isExtra = false;
                int extraSlotNumber = -1;
                if( strstr( animSlot, "7x" ) == animSlot ) {
                    
                    sscanf( &( animSlot[2] ), "%d", &extraSlotNumber );
                
                    if( extraSlotNumber != -1 ) {
                        isExtra = true;
                        slotNumber = 7;
                        }
                    }

                if( ! isExtra ) {
                    sscanf( animSlot, "%d", &slotNumber );
                    }
                
                if( ( slotNumber >= 0 && slotNumber <= 2 )
                    ||
                    ( slotNumber >= 4 && slotNumber <= 5 )
                    ||
                    ( slotNumber == 7 && isExtra ) ) {
                    
                    // correct slot number present
                    

                    if( inSaveIntoDataDirs ) {
                        
                        int idToWrite =  applyMap( &objectIDMap, id );
                        
                        if( idToWrite != -1 ) {
                            // our animation's object is part of this 
                            // loaded import
                            
                            char *animationString = 
                                new char[ currentDataLength + 1 ];
                            memcpy( animationString, 
                                    currentDataBlock, currentDataLength );
                            animationString[ currentDataLength ] = '\0';

                            if( id != idToWrite ) {
                                animationString = 
                                    replaceIDLine( animationString, 
                                                   id, idToWrite );
                                }
                            
                            int numLines = 0;
                            
                            char **animLines = split( animationString, "\n", 
                                                      &numLines );
                            
                            delete [] animationString;

                            for( int i=0; i<numLines; i++ ) {
                                
                                char *line = animLines[i];
                        
                                if( strstr( line, "soundParam=" ) == line ) {

                                    char *spacePos = strstr( line, " " );

                                    const char *stuffAfterSpace = "";

                                    if( spacePos != NULL ) {
                                        // terminate at space
                                        spacePos[0] = '\0';
                                        
                                        stuffAfterSpace = &( spacePos[1] );
                                        }

                                    
                                    
                                    

                                    // skip soundParam= header
                                    char *newSoundString =
                                        remapSounds( &( line[11] ) );
                                    
                                    animLines[i] = 
                                        autoSprintf( "soundParam=%s %s",
                                                     newSoundString,
                                                     stuffAfterSpace );
                                    
                                    delete [] newSoundString;
                                    
                                    delete [] line;
                                    }
                                }

                            char *newAnimString = 
                                join( animLines, numLines, "\n" );
                                                
                            for( int i=0; i<numLines; i++ ) {
                                delete [] animLines[i];
                                }
                            delete [] animLines;
                            
                            File animDir( NULL, "animations" );
                

                            char *fileName = autoSprintf( "%d_%s.txt", 
                                                          idToWrite,
                                                          animSlot );
                        
                            File *animFile = animDir.getChildFile( fileName );
                        
                            delete [] fileName;
                         

                            animFile->writeToFile( newAnimString );
                        
                            delete animFile;
                    
                            delete [] newAnimString;

                            // clear cache
                            File *cacheFile = 
                                animDir.getChildFile( "cache.fcz" );
                            
                            cacheFile->remove();
                            
                            delete cacheFile;
                            }
                        }
                    else {
                        // only replace in live bank
                        // don't save to disk

                        ObjectRecord *existingRecord = getObject( id, true );
                
                        if( existingRecord != NULL ) {
                            // an object exists mathing this id
                        
                            // replace its animation
                        
                            // copy data block into a string for parsing
                            char *animationString = 
                                new char[ currentDataLength + 1 ];
                            memcpy( animationString, 
                                    currentDataBlock, currentDataLength );
                            animationString[ currentDataLength ] = '\0';
                    

                            AnimationRecord *modRecord =
                                scanAnimationRecordFromString( 
                                    animationString );
                        
                            delete [] animationString;
                        
                            if( modRecord != NULL ) {
                                // remap sounds                            
                                for( int i=0; i< modRecord->numSounds; i++ ) {

                                    remapSounds( 
                                        &( modRecord->soundAnim[i].sound ) );
                                    }
                            
                                // set extra slot if needed
                                if( isExtra ) {
                                    setExtraIndex( extraSlotNumber );
                                    }

                                // add to bank (will replace existing anim,
                                // if any) don't write to file
                                addAnimation( modRecord, true );
                            
                                AnimationRecord *insertedRecord =
                                    getAnimation( id, modRecord->type );

                                if( insertedRecord != NULL ) {
                                    scannedLoadAnimationsActuallyInserted.
                                        push_back(
                                            insertedRecord );
                                    }
                            
                            
                                // also re-insert animations for
                                // all use/var dummies
                                ObjectRecord *o = getObject( id );
                            
                                if( o->useDummyIDs != NULL ) {
                                
                                    for( int i=0; i< o->numUses - 1; i++ ) {
                                        modRecord->objectID =
                                            o->useDummyIDs[ i ];
                                    
                                        addAnimation( modRecord, true );
                                        insertedRecord =
                                            getAnimation( modRecord->objectID, 
                                                          modRecord->type );
                                    
                                        if( insertedRecord != NULL ) {
                                          scannedLoadAnimationsActuallyInserted.
                                                push_back( insertedRecord );
                                            }
                                        }
                                    }
                            
                                if( o->variableDummyIDs != NULL ) {
                                
                                    for( int i=0; 
                                         i< o->numVariableDummyIDs; i++ ) {
                                       
                                        modRecord->objectID = 
                                            o->variableDummyIDs[ i ];
                                    
                                        addAnimation( modRecord, true );
                                        insertedRecord =
                                            getAnimation( modRecord->objectID, 
                                                          modRecord->type );
                                    
                                        if( insertedRecord != NULL ) {
                                         scannedLoadAnimationsActuallyInserted.
                                                push_back( insertedRecord );
                                            }
                                        }
                                    }
                                // if these dummy lists are NULL, we may
                                // be in the Editor which doesn't generate
                                // dummies at startup.
                            
                                freeRecord( modRecord );
                                }
                            }
                        }
                    }
                }
            }

        currentScannedLoadBlock++;
        totalNumLoadBlocksScannedSoFar ++;
        
        if( currentScannedLoadBlock >= scannedLoadBlockHeaders.size() ) {
            // done processing scanned blocks from current mod
            

            // now clear any animations for mod objects
            // where an animation was NOT loaded for that slot
            for( int i=0; i< scannedLoadObjectActuallyInserted.size(); i++ ) {
                
                int id = 
                    scannedLoadObjectActuallyInserted.getElementDirect( i );
                
                for( int t=ground; t<endAnimType; t++ ) {
                    
                    AnimationRecord *a = getAnimation( id, (AnimType)t );
                    
                    if( a != NULL &&
                        scannedLoadAnimationsActuallyInserted.getElementIndex(
                            a  ) == -1 ) {
                        // base animation exists, but we didn't add one
                        // to replace it.  Mod has "no animation" for this slot
                        // which means object is intentionally not animated
                        // in mod
                        clearAnimation( id, (AnimType)t, true );
                        }
                    }
                // next check extras
                int numBankExtras = getNumExtraAnim( id );
                
                if( numBankExtras > 0 ) {
                    AnimationRecord **bankExtras = getAllExtraAnimations( id );
                    
                    for( int e=0; e<numBankExtras; e++ ) {
                        AnimationRecord *a = bankExtras[e];
                        
                        if( a != NULL &&
                            scannedLoadAnimationsActuallyInserted.
                            getElementIndex( a ) == -1 ) {
                            
                            setExtraIndex( a->extraIndex );
                            
                            clearAnimation( id, extra, true );
                            }
                        }
                    delete [] bankExtras;
                    }
                }
            

            // prepare for scanning next mod/import, if there is one
            currentLoadFileIndex++;
            
            currentLoadScanned = false;
            
            freeScannedData();
            }
        else {
            // done with another block from current mod/import
            }
        }
    
    

    
    return getLoadProgress();
    }




float initModLoaderStep() {
    return initLoaderStepInternal( false, false );
    }



float initImportAddStep() {
    return initLoaderStepInternal( true, false );
    }



float initImportReplaceStep() {
    return initLoaderStepInternal( true, true );
    }




static void importCleanup( const char *inDirName ) {

    File loadDir( NULL, inDirName );    
    
    if( ! loadDir.exists() || 
        ! loadDir.isDirectory() ) {
        return;
        }
    
    File destDir( NULL, "imported" );    
    
    if( ! destDir.exists() ) {
        destDir.makeDirectory();
        }
    
    if( ! destDir.exists() ||
        ! destDir.isDirectory() ) {
        return;
        }
    
    int numFiles = 0;
    File **files = loadDir.getChildFiles( &numFiles );
    
    for( int i=0; i<numFiles; i++ ) {
        
        char *name = files[i]->getFileName();
        
        File *destFile = destDir.getChildFile( name );

        delete [] name;

        files[i]->copy( destFile );
        
        delete destFile;

        files[i]->remove();

        delete files[i];
        }
    delete [] files;
    }




static void initLoaderFinishInternal() {
    for( int i=0; i< loadFileList.size(); i++ ) {
        delete loadFileList.getElementDirect( i );
        }
    
    loadFileList.deleteAll();

    spriteIDMap.deleteAll();
    soundIDMap.deleteAll();

    objectIDMap.deleteAll();
    }



void initModLoaderFinish() {
    initLoaderFinishInternal();
    }



void initImportAddFinish() {
    // move all files from 'import_add' into 'imported'
    importCleanup( "import_add" );    
    initLoaderFinishInternal();
    }



void initImportReplaceFinish() {
    // move all files from 'import_replace' into 'imported'
    importCleanup( "import_replace" );
    initLoaderFinishInternal();
    }



void freeImporter() {
    // clean up, in case we got interrupted during loading

    for( int i=0; i< loadFileList.size(); i++ ) {
        delete loadFileList.getElementDirect( i );
        }
    
    loadFileList.deleteAll();

    spriteIDMap.deleteAll();
    soundIDMap.deleteAll();

    
    freeScannedData();
    }

