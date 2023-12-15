#include "minorGems/util/SimpleVector.h"

#include "minorGems/io/file/File.h"

#include "minorGems/formats/encodingUtils.h"

#include "exporter.h"

#include "spriteBank.h"
#include "soundBank.h"
#include "objectBank.h"
#include "animationBank.h"



static SimpleVector<File*> modFileList;

static int currentModFileIndex;


static char currentModScanned;


// for current mod that we're working on loading
static SimpleVector<char *> scannedModBlockHeaders;
        
static SimpleVector<int> scannedModDataBlockLengths;
        
static SimpleVector<unsigned char*> scannedModDataBlocks;

static int currentScannedModBlock;

// these are the object that were actually inserted into our objectBank
SimpleVector<int> scannedModObjectActuallyInserted;

// these are maintained by animationBank
SimpleVector<AnimationRecord *> scannedModAnimationsActuallyInserted;




typedef struct IDMapEntry {
        int loadedID;
        int bankID;
    } IDMapEntry;


// maps IDs in mod file to live IDs in our banks
// (in some cases, we may add resources to our banks, in other cases
//  we may find resources with the same hashes)
static SimpleVector<IDMapEntry> spriteIDMap;
static SimpleVector<IDMapEntry> soundIDMap;


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
    
    // return an infalid ID
    return -1;
    }



static void remapSounds( SoundUsage *inUsage ) {
    for( int i=0; i< inUsage->numSubSounds; i++ ) {
        inUsage->ids[i] = applyMap( &soundIDMap, inUsage->ids[i] );
        }
    }



static void freeScannedData() {    
    scannedModBlockHeaders.deallocateStringElements();
    
    scannedModDataBlockLengths.deleteAll();
    
    for( int i=0; i<scannedModDataBlocks.size(); i++ ) {
        delete [] scannedModDataBlocks.getElementDirect( i );
        }
    scannedModDataBlocks.deleteAll();
    
    scannedModObjectActuallyInserted.deleteAll();
    scannedModAnimationsActuallyInserted.deleteAll();
    }



static int totalNumModBlocksToScan;

static int totalNumModBlocksScannedSoFar;




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
        delete [] mods;
        }
    
    currentModFileIndex = 0;
    
    currentModScanned = false;


    // peek at header of each file here
    // to get total number of blocks to load

    totalNumModBlocksToScan = 0;

    for( int i=0; i<modFileList.size(); i++ ) {
        File *file = modFileList.getElementDirect( i );
        
        char *name = file->getFullFileName();
        
        FILE *f = fopen( name, "rb" );
        
        if( f == NULL ) {
            printf( "Failed to open mod file for reading: %s\n", name );
            delete [] name;
            delete file;
            modFileList.deleteElement( i );
            i--;
            continue;
            }
        
        int blocks = 0;
        int numRead = fscanf( f, "%d", &blocks );
        
        if( numRead != 1 ) {
            printf( "Failed to read header block count from mod file: %s\n",
                    name );
            delete [] name;
            delete file;
            modFileList.deleteElement( i );
            i--;
            continue;
            }
        
        delete [] name;

        totalNumModBlocksToScan += blocks;
        }
    
    totalNumModBlocksScannedSoFar = 0;
    
    printf( "Mod loader sees %d total blocks to scan from mod folder\n",
            totalNumModBlocksToScan );

    return totalNumModBlocksToScan;
    }



static float getModLoadProgress() {
    return (float)totalNumModBlocksScannedSoFar / 
        (float)totalNumModBlocksToScan;
    }

    

// returns progress... ready for Finish when progress == 1.0
float initModLoaderStep() {
    
    if( currentModFileIndex >= modFileList.size() ) {
        return 1.0f;
        }

    if( ! currentModScanned ) {
        // scan it

        currentScannedModBlock = 0;
        
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
            
            unsigned char *dataWorking = data;
            // skip numBlocks at beginning of header
            scanIntAndSkip( (char **)( &dataWorking ), &scanSuccess );
            
            if( ! scanSuccess ) {
                // failed to scan header num blocks value
                delete [] data;

                char *fileName = currentFile->getFileName();

                printf( "Failed to scan block count header from mod file %s\n",
                        fileName );
                
                delete [] fileName;

                currentModFileIndex ++;
                
                return getModLoadProgress();
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
                    "from mod file %s\n",
                    fileName );
                
                delete [] fileName;

                currentModFileIndex ++;
                
                return getModLoadProgress();
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
                        "scanning mod block %d\n", blockCount );
                
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
                        "scanning mod block %d\n", blockCount );
                
                delete [] header;
                break;
                }
            
            int blockDataLength = -1;
            
            sscanf( (char*)( & data[spacePos] ), "%d", &blockDataLength );
            
            if( blockDataLength == -1 ) {
                printf( "Failed to find block data length in header when "
                        "scanning mod block %d\n", blockCount );
                
                delete [] header;
                break;
                }
            

            // skip # in data (which has been replaced with \0 )
            dataPos++;
            
            if( dataSize - dataPos < blockDataLength ) {
                printf( "Block %d in mod file specifies data length of %d "
                        "when there's only %d bytes left in data\n",
                        blockCount, blockDataLength, dataSize - dataPos );
                
                delete [] header;
                break;
                }
            
            unsigned char *blockData = new unsigned char[ blockDataLength ];
            
            memcpy( blockData, & data[ dataPos ], blockDataLength );
            

            scannedModBlockHeaders.push_back( header );
            scannedModDataBlockLengths.push_back( blockDataLength );
            scannedModDataBlocks.push_back( blockData );

            // skip the data, now that we've scanned it.
            dataPos += blockDataLength;
            
            blockCount++;
            }
        
        delete [] data;
        
        printf( "Scanned %d blocks from mod file\n", 
                scannedModBlockHeaders.size() );
        
        
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

            printf( "Hash check for mod file failed (correct hash=%s): %s\n",
                    correctHash, & extensionPos[ -6 ] );
            
            delete [] correctHash;
            delete [] fileName;

            // dump this data
            freeScannedData();
            
            // move on to the next mod file
            currentModFileIndex ++;
            
            return getModLoadProgress();
            }

        // hash check okay
        printf( "Hash for mod computed as %s, matches file name hash in %s\n",
                correctHash, fileName );
        delete [] correctHash;
        delete [] fileName;

        currentModScanned = true;
        }
    else {
        // walk through blocks and process them

        // FIXME
        
        char *currentHeader = 
            scannedModBlockHeaders.getElementDirect( currentScannedModBlock );
        
        int currentDataLength = 
            scannedModDataBlockLengths.getElementDirect( 
                currentScannedModBlock );
        
        unsigned char *currentDataBlock =
            scannedModDataBlocks.getElementDirect( currentScannedModBlock );
        
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

                int bankID = 
                    doesSpriteRecordExist(
                        currentDataLength,
                        currentDataBlock,
                        tag,
                        multiplicativeBlend,
                        centerAnchorXOffset, centerAnchorYOffset );
                
                if( bankID == -1 ) {
                    // no sprite found, need to add a new one to bank
                    bankID = 
                        addSprite( tag, currentDataBlock, currentDataLength,
                                   multiplicativeBlend,
                                   centerAnchorXOffset,
                                   centerAnchorYOffset );

                    if( bankID == -1 ) {
                        printf( "Loading sprite TGA data from data block "
                                "with %d bytes from mod failed\n",
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
                    bankID = addSoundToLiveBank( currentDataLength,
                                                 currentDataBlock,
                                                 soundType );
                    
                    if( bankID == -1 ) {
                        printf( "Loading sound data from data block "
                                "with %d bytes, type %s, from mod failed\n",
                                currentDataLength, soundType );
                        }
                    }
                else {
                    printf( "Mod sound id %d exists in bank as ID %d\n",
                            id, bankID );
                    }
                
                if( bankID != -1 &&
                    idMapLookup( &soundIDMap, id ) == NULL ) {
                    IDMapEntry e = { id, bankID };
                    soundIDMap.push_back( e );
                    printf( "Mapping mod sound id %d to bank id %d\n",
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
                
                // don't accept default object here
                ObjectRecord *existingRecord = getObject( id, true );
                
                if( existingRecord != NULL ) {
                    
                    // copy data block into a string for parsing
                    char *objectString = new char[ currentDataLength + 1 ];
                    memcpy( objectString, currentDataBlock, currentDataLength );
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
                        
                        scannedModObjectActuallyInserted.push_back( id );
                        
                        // also track useDummy and variableOummies
                        ObjectRecord *o = getObject( id );
                        
                        if( o->useDummyIDs != NULL ) {
                            for( int i=0; i< o->numUses - 1; i++ ) {
                                scannedModObjectActuallyInserted.push_back(
                                    o->useDummyIDs[ i ] );
                                }
                            }
                        if( o->variableDummyIDs != NULL ) {
                            for( int i=0; i< o->numVariableDummyIDs; i++ ) {
                                scannedModObjectActuallyInserted.push_back(
                                    o->variableDummyIDs[ i ] );
                                }
                            }
                        }
                    else {
                        printf( "Parsing object from mod failed\n" );
                        }
                    }
                else {
                    printf( "Mod object id %d not found in object bank, "
                            "skipping\n", id );
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
                            scanAnimationRecordFromString( animationString );
                        
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

                            // add to bank (will replace existing anim, if any)
                            // don't write to file
                            addAnimation( modRecord, true );
                            
                            AnimationRecord *insertedRecord =
                                getAnimation( id, modRecord->type );

                            if( insertedRecord != NULL ) {
                                scannedModAnimationsActuallyInserted.push_back(
                                    insertedRecord );
                                }
                            
                            
                            // also re-insert animations for all use/var dummies
                            ObjectRecord *o = getObject( id );
                            
                            if( o->useDummyIDs != NULL ) {
                                
                                for( int i=0; i< o->numUses - 1; i++ ) {
                                    modRecord->objectID = o->useDummyIDs[ i ];
                                    
                                    addAnimation( modRecord, true );
                                    insertedRecord =
                                        getAnimation( modRecord->objectID, 
                                                      modRecord->type );
                                    
                                    if( insertedRecord != NULL ) {
                                        scannedModAnimationsActuallyInserted.
                                            push_back( insertedRecord );
                                        }
                                    }
                                }
                            
                            if( o->variableDummyIDs != NULL ) {
                                
                                for( int i=0; i< o->numVariableDummyIDs; i++ ) {
                                    modRecord->objectID = 
                                        o->variableDummyIDs[ i ];
                                    
                                    addAnimation( modRecord, true );
                                    insertedRecord =
                                        getAnimation( modRecord->objectID, 
                                                      modRecord->type );
                                    
                                    if( insertedRecord != NULL ) {
                                        scannedModAnimationsActuallyInserted.
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

        currentScannedModBlock++;
        totalNumModBlocksScannedSoFar ++;
        
        if( currentScannedModBlock >= scannedModBlockHeaders.size() ) {
            // done processing scanned blocks from current mod
            

            // now clear any animations for mod objects
            // where an animation was NOT loaded for that slot
            for( int i=0; i< scannedModObjectActuallyInserted.size(); i++ ) {
                
                int id = scannedModObjectActuallyInserted.getElementDirect( i );
                
                for( int t=ground; t<endAnimType; t++ ) {
                    
                    AnimationRecord *a = getAnimation( id, (AnimType)t );
                    
                    if( a != NULL &&
                        scannedModAnimationsActuallyInserted.getElementIndex(
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
                            scannedModAnimationsActuallyInserted.
                            getElementIndex( a ) == -1 ) {
                            
                            setExtraIndex( a->extraIndex );
                            
                            clearAnimation( id, extra, true );
                            }
                        }
                    delete [] bankExtras;
                    }
                }
            

            // prepare for scanning next mod, if there is one
            currentModFileIndex++;
            
            currentModScanned = false;
            
            freeScannedData();
            }
        else {
            // done with another block from current mod
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

    
    freeScannedData();
    }

