#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/io/file/File.h"

#include "minorGems/crypto/hashes/sha1.h"

#include "minorGems/formats/encodingUtils.h"

#include "objectBank.h"
#include "spriteBank.h"
#include "animationBank.h"



static SimpleVector<int> currentBundleObjectIDs;



char doesExportContain( int inObjectID ) {
    if( currentBundleObjectIDs.getElementIndex( inObjectID ) == -1 ) {
        return false;
        }
    return true;
    }


void addExportObject( int inObjectID ) {
    if( ! doesExportContain( inObjectID ) ) {
        currentBundleObjectIDs.push_back( inObjectID );
        }
    }



void removeExportObject( int inObjectID ) {
    currentBundleObjectIDs.deleteElementEqualTo( inObjectID );
    }



void clearExportBundle() {
    currentBundleObjectIDs.deleteAll();
    }






void writeAnimRecordToFile( FILE *inFILE, AnimationRecord *inRecord ) {
    File animationFolder( NULL, "animations" );

    if( ! animationFolder.exists() || ! animationFolder.isDirectory() ) {
        return;
        }
    char *animFileName = NULL;

    char *animTypeTag = NULL;
    
    if( inRecord->type < endAnimType ) {
        animFileName = 
            autoSprintf( "%d_%d.txt", inRecord->objectID, inRecord->type );
        animTypeTag = autoSprintf( "%d", inRecord->type );
        }
    else if( inRecord->type == extra ) {
        
        animFileName = 
            autoSprintf( "%d_7x%d.txt", inRecord->objectID, 
                         inRecord->extraIndex );
        animTypeTag = autoSprintf( "7x%d", inRecord->extraIndex );
        }
    else {
        // unsupported type
        printf( "Animation type %d for object id %d unsupported for export\n",
                inRecord->type, inRecord->objectID );
        return;
        }
    
    
    File *animFile = animationFolder.getChildFile( animFileName );
    
    if( ! animFile->exists() ) {
        printf( "Animation file %s does not exist\n", animFileName );
        
        delete [] animFileName;
        delete [] animTypeTag;
        delete animFile;
        return;
        }
    
    char *animTextData = animFile->readFileContents();
    
    delete animFile;
    
    if( animTextData == NULL ) {
        printf( "Failed to read animation file %s\n", animFileName );
        
        delete [] animFileName;
        delete [] animTypeTag;
        return;
        }
    
    delete [] animFileName;
    
    char found;
    char *cleanedAnimTextData = 
        replaceAll( animTextData, "\r\n", "\n", &found );
    
    delete [] animTextData;
    
    fprintf( inFILE, "anim %d %s %d#%s",
             inRecord->objectID,
             animTypeTag,
             strlen( cleanedAnimTextData ), cleanedAnimTextData );
    
    delete [] animTypeTag;
    delete [] cleanedAnimTextData;
    }


SimpleVector<int> *getCurrentExportList() {
    return &currentBundleObjectIDs;
    }




char *getCurrentBundleHash() {
    // now make checksum of sorted list
    SimpleVector<char> idTextList;
    
    for( int i=0; i<currentBundleObjectIDs.size(); i++ ) {
        
        char *s = autoSprintf( "%d ", 
                               currentBundleObjectIDs.getElementDirect( i ) );
        
        idTextList.appendElementString( s );
        
        delete [] s;
        }

    char *idText = idTextList.getElementString();
    
    char *sha1Hash = computeSHA1Digest( idText );
    
    delete [] idText;

    // truncate to 6 characters
    sha1Hash[6] ='\0';

    char *lowerHash = stringToLowerCase( sha1Hash );
    
    delete [] sha1Hash;
    
    return lowerHash;
    }



static int intCompare( const void *inA, const void * inB ) {
    int *a = (int*)inA;
    int *b = (int*)inB;
    
    if( *a < *b ) {
        return -1;
        }
    if( *a > *b ) {
        return 1;
        }
    return 0;
    }



char finalizeExportBundle( const char *inExportName ) {
    
    if( currentBundleObjectIDs.size() == 0 ) {
        printf( "Export failed:  current bundle object list is empty.\n" );
        
        return false;
        }


    File exportDir( NULL, "exports" );
    
    if( !exportDir.exists() ) {
        exportDir.makeDirectory();
        }
    
    if( ! exportDir.exists() || ! exportDir.isDirectory() ) {
        printf( "Export failed:  'exports' directory doesn't exist.\n" );
        
        return false;
        }
    


    // first, sort the objectIDs to include
    int *idArray = currentBundleObjectIDs.getElementArray();
    int numObjects = currentBundleObjectIDs.size();
    
    qsort( idArray, numObjects, sizeof( int ), intCompare );

    currentBundleObjectIDs.deleteAll();
    
    currentBundleObjectIDs.push_back( idArray, numObjects );
    
    delete [] idArray;
    

    
    char *hash = getCurrentBundleHash();

    
    char *oxpFileName = autoSprintf( "%s_%d_%s.oxp", inExportName, 
                                  currentBundleObjectIDs.size(),
                                  hash );
    
    delete [] hash;

    File *outFile = exportDir.getChildFile( oxpFileName );
    
    
    char *outFileAccessPath = outFile->getFullFileName();

    delete outFile;

    FILE *outFILE = fopen( outFileAccessPath, "wb" );

    delete [] outFileAccessPath;


    if( outFILE == NULL ) {
    
        printf( "Export failed:  failed to open file for writing:  exports/%s\n",
                oxpFileName );

        delete [] oxpFileName;
    
        return false;
        }
    




    // first thing in file is full export of all sprites
    // but first, get list of unique sprite IDs
    // don't repeat data if same sprite occurs multiple times in object
    
    SimpleVector<int> uniqueSpriteIDs;

    // do the same to accumulate list of unique sounds
    SimpleVector<int> uniqueSoundIDs;

    
    // when working on assembling uniqueSoundIDs, gather SoundUsages
    SimpleVector<SoundUsage *> soundUsages;

    

    // also scan animations for sounds
    // we'll use this list of animations later too
    SimpleVector<AnimationRecord *> animations;

    
    for( int i=0; i < currentBundleObjectIDs.size(); i++ ) {
        int objectID = currentBundleObjectIDs.getElementDirect( i );
        
        ObjectRecord *o = getObject( objectID );

        if( o == NULL ) {
            // skip any missing objects
            // they might have been deleted out from underneath us
            continue;
            }
        
        for( int s=0; s< o->numSprites; s++ ) {
            int id = o->sprites[ s ];
            
            if( uniqueSpriteIDs.getElementIndex( id ) == -1 ) {
                // not already seen on our list of unique IDs
                
                uniqueSpriteIDs.push_back( id );
                }
            }
        
    
        soundUsages.push_back( &( o->creationSound ) );
        soundUsages.push_back( &( o->usingSound ) );
        soundUsages.push_back( &( o->eatingSound ) );
        soundUsages.push_back( &( o->decaySound ) );
        
        SimpleVector<AnimationRecord*> thisObjectAnimations;
        
        for( int a=ground; a<endAnimType; a++ ) {
            AnimationRecord *r = getAnimation( objectID, (AnimType)a );
            
            if( r != NULL ) {
                thisObjectAnimations.push_back( r );
                }
            }
        
        int numExtra = getNumExtraAnim( objectID );
        
        for( int e=0; e<numExtra; e++ ) {
            setExtraIndex( e );
            
            AnimationRecord *r = getAnimation( objectID, extra );
            
            if( r != NULL ) {
                thisObjectAnimations.push_back( r );
                }
            }
    
        for( int a=0; a < thisObjectAnimations.size(); a++ ) {
            AnimationRecord *r = thisObjectAnimations.getElementDirect( a );
        
            for( int s=0; s < r->numSounds; s++ ) {
                soundUsages.push_back( &( r->soundAnim[s].sound ) );
                }
            }

        animations.push_back_other( & thisObjectAnimations );
        }
    
    

    // now that we've gathered all SoundUsages, scan them for unique sound IDs
    
    for( int s=0; s< soundUsages.size(); s++ ) {
        SoundUsage *u = soundUsages.getElementDirect( s );
        
        for( int b=0; b < u->numSubSounds; b++ ) {
            int id = u->ids[b];
            
            
            if( uniqueSoundIDs.getElementIndex( id ) == -1 ) {
                // not already seen on our list of unique IDs
                
                uniqueSoundIDs.push_back( id );
                }
            }
        }
    
    // done gathering unique sound IDs

        



    File spriteFolder( NULL, "sprites" );
    
    if( ! spriteFolder.exists() || ! spriteFolder.isDirectory() ) {
        printf( "Export failed:  'sprites' directory does not exist\n" );
        
        fclose( outFILE );
        
        delete [] oxpFileName;

        return false;
        }


    File soundFolder( NULL, "sounds" );
    
    if( ! soundFolder.exists() || ! soundFolder.isDirectory() ) {
        printf( "Export failed:  'sounds' directory does not exist\n" );
        
        fclose( outFILE );

        delete [] oxpFileName;
        
        return false;
        }


    File objectFolder( NULL, "objects" );
    
    if( ! objectFolder.exists() || ! objectFolder.isDirectory() ) {
        printf( "Export failed:  'objects' directory does not exist\n" );
        
        fclose( outFILE );
        
        delete [] oxpFileName;

        return false;
        }


    File animationFolder( NULL, "animations" );
    
    if( ! animationFolder.exists() || ! animationFolder.isDirectory() ) {
        printf( "Export failed:  'animations' directory does not exist\n" );
        
        fclose( outFILE );
        
        delete [] oxpFileName;
        
        return false;
        }



    // first in file, output unique sprites

    for( int s=0; s< uniqueSpriteIDs.size(); s++ ) {
        int id = uniqueSpriteIDs.getElementDirect( s );
        
        SpriteRecord *r = getSpriteRecord( id );
        
        if( r != NULL ) {
            
            int tgaFileSize;
            unsigned char *tgaData = NULL;
            
            char *tgaFileName = autoSprintf( "%d.tga", id );
            
            File *tgaFile = spriteFolder.getChildFile( tgaFileName );
            
            if( ! tgaFile->exists() ) {
                printf( "TGA file sprites/%s does not exist\n",
                        tgaFileName );
                }
            else {
                tgaData = tgaFile->readFileContents( &tgaFileSize );
                }
            
            delete tgaFile;
            delete [] tgaFileName;
                    
            
            if( tgaData == NULL ) {
                printf( "Failed to read from sprite TGA file for id %d\n", id );
                continue;
                }

            // header format:
            // sprite id multBlend centAncXOff centAncYOff tgaSize#
            fprintf( outFILE, "sprite %d %d %d %d %d#",
                     id, r->multiplicativeBlend, 
                     r->centerAnchorXOffset,
                     r->centerAnchorYOffset,
                     tgaFileSize );
            
            fwrite( tgaData, 1, tgaFileSize, outFILE );

            delete [] tgaData;
            }
        }



    
    // next in file, output unique sounds

    for( int s=0; s< uniqueSoundIDs.size(); s++ ) {
        int id = uniqueSoundIDs.getElementDirect( s );
            
        int soundFileSize;
        
        // can also be OGG
        const char *soundFileType = "AIFF";
        
        unsigned char *soundFileData = NULL;
        
        File *soundFile = NULL;
        
        
        char *aiffFileName = autoSprintf( "%d.aiff", id );
            
        soundFile = soundFolder.getChildFile( aiffFileName );
        
        delete [] aiffFileName;
        

        if( ! soundFile->exists() ) {
            
            delete soundFile;
            
            // try ogg
            soundFileType = "OGG";
            
            char *oggFileName = autoSprintf( "%d.ogg", id );
            
            soundFile = soundFolder.getChildFile( oggFileName );
            
            delete [] oggFileName;
            }
        
        if( ! soundFile->exists() ) {
            printf( "Neither file sounds/%d.aiff nor sounds/%d.ogg exists\n",
                    id, id );
            delete soundFile;
            continue;
            }
        
        soundFileData = soundFile->readFileContents( &soundFileSize );
        
        delete soundFile;
            
        if( soundFileData == NULL ) {
            printf( "Failed to read from sound AIFF or OGG file for id %d\n", 
                    id );
            continue;
            }

        // header format:
        // sound id type fileSize#
        fprintf( outFILE, "sound %d %s %d#",
                 id, soundFileType, soundFileSize );
        
        fwrite( soundFileData, 1, soundFileSize, outFILE );
        
        delete [] soundFileData;
        }
    
    

    

    // next output the objects
    for( int i=0; i < currentBundleObjectIDs.size(); i++ ) {
        int objectID = currentBundleObjectIDs.getElementDirect( i );

        char *objectFileName = autoSprintf( "%d.txt", objectID );
            
        File *objectFile = objectFolder.getChildFile( objectFileName );
        
        delete [] objectFileName;
    
  
        if( ! objectFile->exists() ) {
            printf( "Export non-fatal error, "
                    "objects/%d.txt does not exist\n", objectID );

            // skip this object that has no file
            delete objectFile;
        
            continue;
            }
    
        
        char *objectTextData = objectFile->readFileContents();
    
        delete objectFile;
    

        if( objectTextData == NULL ) {
            printf( "Export non-fatal error, "
                    "failed to read from objects/%d.txt\n", objectID );
            
            continue;
            }
    
        
        // normalize line endings
        
        char found;
        char *cleanedObjectTextData = 
            replaceAll( objectTextData, "\r\n", "\n", &found );
        
        delete [] objectTextData;
        
        fprintf( outFILE, "object %d %d#%s",
                 objectID,
                 strlen( cleanedObjectTextData ), cleanedObjectTextData );
        
        delete [] cleanedObjectTextData;    
        }
    
    

    // last, write animations to file
    // we assembled list of animations earlier, when working on sounds
    for( int a=0; a<animations.size(); a++ ) {
        writeAnimRecordToFile( outFILE, animations.getElementDirect( a ) );
        }
    
    
    fclose( outFILE );


    if( SettingsManager::getIntSetting( "compressExports", 1 ) != 1 ) {
        // compression of exports disabled
        // stop here
        delete [] oxpFileName;
        
        // start new bundle only on full success
        clearExportBundle();
        
        return true;
        }
    

    
    File *inFile = exportDir.getChildFile( oxpFileName );
    


    int inLength;
    unsigned char *inData = inFile->readFileContents( &inLength );
    
    if( inData == NULL ) {
        printf( "Export failed:  could not re-open %s for reading\n",
                oxpFileName );
    
        delete [] oxpFileName;
        
        delete inFile;

        return false;
        }

    


    int outLength;
    unsigned char *oxzData = zipCompress( inData, inLength, &outLength );
    
    delete [] inData;
    

    if( oxzData == NULL ) {
        printf( "Export failed:  zipCompress failed\n" );
    
        delete [] oxpFileName;
        
        delete inFile;

        return false;
        }

    
    // turn .oxp into .oxz
    int lastCharIndex = strlen( oxpFileName ) - 1;
    
    if( oxpFileName[ lastCharIndex ] == 'p' ) {
        oxpFileName[ lastCharIndex ] = 'z';
        }
    else {
        printf( "Export failed:  base file %s does not end with .oxp\n",
                oxpFileName );
        
        delete [] oxzData;
        delete [] oxpFileName;
        
        delete inFile;

        return false;
        }

    
    // this is the .oxz file
    outFile = exportDir.getChildFile( oxpFileName );
    
    
    char success = outFile->writeToFile( oxzData, outLength );
    

    delete [] oxzData;
    delete outFile;
    
    
    if( ! success ) {
        printf( "Export failed:  could not write to %s\n",
                oxpFileName );
    
        delete [] oxpFileName;
        
        delete inFile;

        return false;
        }    


    delete [] oxpFileName;

    
    
    // don't keep base .oxp file around after export
    inFile->remove();
    
    delete inFile;
    
    
    
    // start new bundle only on full success
    clearExportBundle();

    return true;
    }

