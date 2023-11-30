#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include "objectBank.h"
#include "spriteBank.h"
#include "animationBank.h"




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
    
    fprintf( inFILE, "anim %s %d#%s",
             animTypeTag,
             strlen( cleanedAnimTextData ), cleanedAnimTextData );
    
    delete [] animTypeTag;
    delete [] cleanedAnimTextData;
    }




void exportObject( int inObjectID ) {

    File exportDir( NULL, "exports" );
    
    if( !exportDir.exists() ) {
        exportDir.makeDirectory();
        }
    
    if( ! exportDir.exists() || ! exportDir.isDirectory() ) {
        printf( "Export failed:  'exports' directory doesn't exist.\n" );
        
        return;
        }
    
    ObjectRecord *o = getObject( inObjectID );
    
    char *objectName = stringDuplicate( o->description );
    
    stripDescriptionComment( objectName );
    
    // also strip off any $10 style variable placeholders
    // (usually these are part of comment, but for some objects, they occur
    // in the object name itself)

    char *dollarPos = strstr( objectName, "$" );
    
    if( dollarPos != NULL ) {
        dollarPos[0] = '\0';
        }

    char *fileName = autoSprintf( "%d_%s.obj", inObjectID, objectName );
    
    delete [] objectName;
    

    File *outFile = exportDir.getChildFile( fileName );
    
    delete [] fileName;
    
    char *outFileAccessPath = outFile->getFullFileName();

    delete outFile;

    FILE *outFILE = fopen( outFileAccessPath, "wb" );


    if( outFILE == NULL ) {
    
        printf( "Export failed:  failed to open file for writing:  %s\n",
                outFileAccessPath );
        
        delete [] outFileAccessPath;
    
        return;
        }
    
    delete [] outFileAccessPath;

    
    // first thing in file is full export of all sprites
    // but first, get list of unique sprite IDs
    // don't repeat data if same sprite occurs multiple times in object
    
    SimpleVector<int> uniqueSpriteIDs;
    
    for( int s=0; s< o->numSprites; s++ ) {
        if( uniqueSpriteIDs.getElementIndex( o->sprites[ s ] ) == -1 ) {
            // not already seen on our list of unique IDs
            
            uniqueSpriteIDs.push_back( o->sprites[ s ] );
            }
        }
    
    File spriteFolder( NULL, "sprites" );
    
    if( ! spriteFolder.exists() || ! spriteFolder.isDirectory() ) {
        printf( "Export failed:  'sprites' directory does not exist\n" );
        
        fclose( outFILE );
        
        return;
        }

    File objectFolder( NULL, "objects" );
    
    if( ! objectFolder.exists() || ! objectFolder.isDirectory() ) {
        printf( "Export failed:  'objects' directory does not exist\n" );
        
        fclose( outFILE );
        
        return;
        }


    File animationFolder( NULL, "animations" );
    
    if( ! animationFolder.exists() || ! animationFolder.isDirectory() ) {
        printf( "Export failed:  'animations' directory does not exist\n" );
        
        fclose( outFILE );
        
        return;
        }


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
    

    // next output the object itself
    char *objectFileName = autoSprintf( "%d.txt", inObjectID );
            
    File *objectFile = objectFolder.getChildFile( objectFileName );
          
    delete [] objectFileName;
    
  
    if( ! objectFile->exists() ) {
        printf( "Export failed, objects/%d.txt does not exist\n",
                inObjectID );
        delete objectFile;
        
        fclose( outFILE );
        return;
        }
    
    char *objectTextData = objectFile->readFileContents();
    
    delete objectFile;
    

    if( objectTextData == NULL ) {
        printf( "Export failed, failed to read from objects/%d.txt\n",
                inObjectID );
        
        fclose( outFILE );
        return;
        }
    
    
    // normalize line endings

    char found;
    char *cleanedObjectTextData = 
        replaceAll( objectTextData, "\r\n", "\n", &found );
    
    delete [] objectTextData;
    
    fprintf( outFILE, "object %d#%s",
             strlen( cleanedObjectTextData ), cleanedObjectTextData );
    
    delete [] cleanedObjectTextData;
    
    
    for( int a=ground; a<endAnimType; a++ ) {
        AnimationRecord *r = getAnimation( inObjectID, (AnimType)a );
        
        if( r != NULL ) {
            writeAnimRecordToFile( outFILE, r );
            }
        }
    
    
    int numExtra = getNumExtraAnim( inObjectID );
    

    for( int e=0; e<numExtra; e++ ) {
        setExtraIndex( e );
        
        AnimationRecord *r = getAnimation( inObjectID, extra );

        if( r != NULL ) {
            writeAnimRecordToFile( outFILE, r );
            }
        }
    
    fclose( outFILE );
    }

