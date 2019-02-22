
#include "spriteBank.h"

#include <stdlib.h>

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"

#include "minorGems/game/game.h"


#include "folderCache.h"
#include "binFolderCache.h"



static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static SpriteRecord **idMap;


static StringTree tree;


static FolderCache cache;

static BinFolderCache binCache;

static int currentFile;
static int currentBinFile;


static SimpleVector<SpriteRecord*> records;
static int maxID;


static File spritesDir( NULL, "sprites" );


static SpriteHandle blankSprite = NULL;



typedef struct SpriteLoadingRecord {
        int spriteID;
        int asyncLoadHandle;
        
    } SpriteLoadingRecord;


static SimpleVector<SpriteLoadingRecord> loadingSprites;

static SimpleVector<int> loadedSprites;


static char *loadingFailureFileName = NULL;




int getMaxSpriteID() {
    return maxID;
    }


static char makeNewSpritesSearchable =  false;


void enableSpriteSearch( char inEnable ) {
    makeNewSpritesSearchable = inEnable;
    }




int initSpriteBankStart( char *outRebuildingCache ) {
    maxID = 0;
    
    currentFile = 0;
    currentBinFile = 0;
    
    char rebuildingA, rebuildingB;
    cache = initFolderCache( "sprites", &rebuildingA );

    binCache = initBinFolderCache( "sprites", ".tga", &rebuildingB );

    *outRebuildingCache = rebuildingA || rebuildingB;
    

    unsigned char onePixel[4] = { 0, 0, 0, 0 };
    

    blankSprite = fillSprite( onePixel, 1, 1 );
    
    return cache.numFiles + binCache.numFiles;
    }




// expands true regions by making neighbor pixels true also
void expandMap( char *inMap, int inW, int inH ) {
    int numPixels = inW * inH;
    
    char *copy = new char[ numPixels ];
    
    memcpy( copy, inMap, numPixels );
    
    // avoid edges
    for( int y = 1; y < inH-1; y++ ) {
        for( int x = 1; x < inW-1; x++ ) {
            int index = y * inW + x;
            
            if( copy[index] ) {
                // make neighbors true also

                inMap[index-1] = true;
                inMap[index+1] = true;

                inMap[index-inW] = true;
                inMap[index+inW] = true;                
                }
            }
        }
    
    delete [] copy;
    }


static void setLoadingFailureFileName( char *inNewFileName ) {
    if( loadingFailureFileName != NULL ) {
        delete [] loadingFailureFileName;
        }
    loadingFailureFileName = inNewFileName;
    }




static void loadSpriteFromRawTGAData( int inSpriteID, unsigned char *inTGAData,
                                      int inDataLength ) {
    
    RawRGBAImage *spriteImage = readTGAFileRawFromBuffer( inTGAData, 
                                                          inDataLength);

    if( spriteImage != NULL && spriteImage->mNumChannels != 4 ) {
        printf( "Sprite loading for id %d not a 4-channel image, "
                "failed to load.\n",
                inSpriteID );
        delete spriteImage;
        spriteImage = NULL;
        
        setLoadingFailureFileName(
            autoSprintf( "sprites/%d.tga", inSpriteID ) );
        }
                            
    if( spriteImage != NULL ) {
        SpriteRecord *r = getSpriteRecord( inSpriteID );
                        
        r->sprite =
            fillSprite( spriteImage->mRGBABytes, 
                        spriteImage->mWidth,
                        spriteImage->mHeight );
        
        r->w = spriteImage->mWidth;
        r->h = spriteImage->mHeight;                
        
        doublePair offset = { (double)( r->centerAnchorXOffset ),
                              (double)( r->centerAnchorYOffset ) };
        
        setSpriteCenterOffset( r->sprite, offset );
        
        
        
        r->maxD = r->w;
        if( r->h > r->maxD ) {
            r->maxD = r->h;
            }        
        
        int numPixels = r->w * r->h;
        r->hitMap = new char[ numPixels ];
        
        memset( r->hitMap, 1, numPixels );
        
                    
        int numBytes = numPixels * 4;
                    
        unsigned char *bytes = spriteImage->mRGBABytes;
                    
        // track max/min x and y to compute average for center

        int minX = r->w;
        int maxX = 0;
                    
        int minY = r->h;
        int maxY = 0;
                    
        int w = r->w;
                    

        // alpha is 4th byte
        int p=0;
        for( int b=3; b<numBytes; b+=4 ) {
            if( bytes[b] < 64 ) {
                r->hitMap[p] = 0;
                }
            else {
                int y = p / w;
                int x = p % w;

                if( y < minY ) {
                    minY = y;
                    }
                if( y > maxY ) {
                    maxY = y;
                    }

                if( x < minX ) {
                    minX = x;
                    }
                if( x > maxX ) {
                    maxX = x;
                    }
                }
                        
            p++;
            }
                    
        for( int e=0; e<3; e++ ) {    
            expandMap( r->hitMap, r->w, r->h );
            }

        r->centerXOffset = 
            ( maxX + minX ) / 2 - 
            r->w / 2;

        r->centerYOffset = 
            ( maxY + minY ) / 2 - 
            r->h / 2;
                    
        r->visibleW = maxX - minX;
        r->visibleH = maxY - minY;

        delete spriteImage;
        }
    }





typedef struct LoadedSpritePlaceholder {
        int id;
        SpriteHandle sprite;
    } LoadedSpritePlaceholder;
    

SimpleVector<LoadedSpritePlaceholder> loadedPlaceholders;



float initSpriteBankStep() {
    
    if( currentFile == cache.numFiles &&
        currentBinFile == binCache.numFiles ) {
        return 1.0;
        }

    
    if( currentFile < cache.numFiles ) {
        
        int i = currentFile;

        char *fileName = getFileName( cache, i );
    
        // skip all non-txt files (only read meta data files on init, 
        // not bulk data tga files)
        if( strstr( fileName, ".txt" ) != NULL &&
            strcmp( fileName, "nextSpriteNumber.txt" ) != 0 ) {
                            
            //printf( "Loading sprite from path %s\n", fileName );

            SpriteRecord *r = new SpriteRecord;


            r->sprite = NULL;
            r->hitMap = NULL;
            r->loading = false;
            r->numStepsUnused = 0;
        
            r->remappable = true;
            r->remapTarget = true;

            r->maxD = 2;
        
            // dummy values until we load the image later
            r->w = 2;
            r->h = 2;
        
            r->visibleW = 2;
            r->visibleH = 2;


            r->id = 0;
        
            sscanf( fileName, "%d.txt", &( r->id ) );
                
                
            char *contents = getFileContents( cache, i );
                
            r->tag = NULL;

            r->centerXOffset = 0;
            r->centerYOffset = 0;
        
            r->centerAnchorXOffset = 0;
            r->centerAnchorYOffset = 0;

            if( contents != NULL ) {
            
                SimpleVector<char *> *tokens = tokenizeString( contents );
                int numTokens = tokens->size();
            
                if( numTokens >= 2 ) {
                        
                    r->tag = 
                        stringDuplicate( tokens->getElementDirect( 0 ) );
                
                    if( strstr( r->tag, "_" ) != NULL ) {
                        r->remapTarget = false;
                        r->remappable = false;
                        }
                    else if( strncmp( r->tag, "Category", 8 ) == 0 ) {
                        r->remapTarget = false;
                        }
                    else if( strncmp( r->tag, "BodyWhite", 9 ) == 0 ) {
                        r->remapTarget = false;
                        }
                    else if( strncmp( r->tag, "HeadWhite", 9 ) == 0 ) {
                        r->remapTarget = false;
                        }
                
                
        
                    int mult;
                    sscanf( tokens->getElementDirect( 1 ),
                            "%d", &mult );
                        
                    if( mult == 1 ) {
                        r->multiplicativeBlend = true;
                        }
                    else {
                        r->multiplicativeBlend = false;
                        }
                    }
                if( numTokens >= 4 ) {
                    sscanf( tokens->getElementDirect( 2 ),
                            "%d", &( r->centerAnchorXOffset ) );

                    sscanf( tokens->getElementDirect( 3 ),
                            "%d", &( r->centerAnchorYOffset ) );
                    }
            

                tokens->deallocateStringElements();
                delete tokens;
                    
                delete [] contents;
                }
                
            if( r->tag == NULL ) {
                r->tag = stringDuplicate( "tag" );
                r->multiplicativeBlend = false;
                }

            records.push_back( r );

            if( maxID < r->id ) {
                maxID = r->id;
                }
            }    
        delete [] fileName;


        currentFile ++;


        if( currentFile == cache.numFiles ) {
            // done loading all .txt files
            // ready to populate record ID map
            mapSize = maxID + 1;
            
            idMap = new SpriteRecord*[ mapSize ];
            
            for( int i=0; i<mapSize; i++ ) {
                idMap[i] = NULL;
                }
            
            int numRecords = records.size();
            for( int i=0; i<numRecords; i++ ) {
                SpriteRecord *r = records.getElementDirect(i);
                
                idMap[ r->id ] = r;
                
                if( makeNewSpritesSearchable ) {    
                    char *lower = stringToLowerCase( r->tag );
            
                    tree.insert( lower, r );
                    
                    delete [] lower;
                    }
                }
            printf( "Loaded %d tagged sprites from sprites folder\n", 
                    numRecords );
            }
        }
    else if( currentBinFile < binCache.numFiles ) {
        // all .txt files have been loaded from cache.fcz
        
        // that means all sprite records have been created
        
        // now load all tga files from bin_cache.fcz

        // and use tga data to populate sprite records with image data
        
        int i = currentBinFile;

        char *fileName = getFileName( binCache, i );
    
        // skip all non-txt files (only read meta data files on init, 
        // not bulk data tga files)
        if( strstr( fileName, ".tga" ) != NULL ) {
            
            int spriteID = 0;
            sscanf( fileName, "%d.txt", &spriteID );

            if( spriteID > 0 ) {
                
                int contSize;
                unsigned char *contents = getFileContents( binCache, i,
                                                           fileName, 
                                                           &contSize );
                if( contents != NULL ) {
                    loadSpriteFromRawTGAData( spriteID, contents, contSize );
                    
                    SpriteRecord *r = getSpriteRecord( spriteID );
                    
                    r->numStepsUnused = 0;
                    loadedSprites.push_back( spriteID );

                    delete [] contents;
                    }
                }
            }
        delete [] fileName;
        currentBinFile++;
        }
    
    

    return (float)( currentFile + currentBinFile ) / 
        (float)( cache.numFiles + binCache.numFiles );
    }


static char spriteBankLoaded = false;

 

void initSpriteBankFinish() {    

    freeFolderCache( cache );
    freeBinFolderCache( binCache );
    
    

    spriteBankLoaded = true;
    }



char isSpriteBankLoaded() {
    return spriteBankLoaded;
    }



static void loadSpriteImage( int inID ) {
    SpriteRecord *r = getSpriteRecord( inID );
    
    if( r != NULL ) {
        
        if( r->sprite == NULL && ! r->loading ) {
                
            File spritesDir( NULL, "sprites" );
            

            const char *printFormatTGA = "%d.tga";
        
            char *fileNameTGA = autoSprintf( printFormatTGA, inID );
        

            File *spriteFile = spritesDir.getChildFile( fileNameTGA );
            
            delete [] fileNameTGA;
            

            char *fullName = spriteFile->getFullFileName();
        
            delete spriteFile;
            

            SpriteLoadingRecord loadingR;
            
            loadingR.spriteID = inID;

            loadingR.asyncLoadHandle = startAsyncFileRead( fullName );
            
            delete [] fullName;

            loadingSprites.push_back( loadingR );
            
            r->loading = true;
            }

        }
    }






static void freeSpriteRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            
            if( idMap[inID]->sprite != NULL ) {    
                freeSprite( idMap[inID]->sprite );
                
                for( int i=0; i<loadedSprites.size(); i++ ) {
                    int id = loadedSprites.getElementDirect( i );
                    
                    if( id == inID ) {
                        loadedSprites.deleteElement( i );
                        break;
                        }
                    }
                }

            char *lower = stringToLowerCase( idMap[inID]->tag );
            
            tree.remove( lower, idMap[inID] );

            delete [] lower;

            delete [] idMap[inID]->tag;
                        
            if( idMap[inID]->hitMap != NULL ) {
                delete [] idMap[inID]->hitMap;    
                }
            
            
            delete idMap[inID];
            idMap[inID] = NULL;
            
            return;
            }
        }
    }




void freeSpriteBank() {
    
    if( loadingFailureFileName != NULL ) {
        delete [] loadingFailureFileName;
        }
    

    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            if( idMap[i]->sprite != NULL ) {    
                freeSprite( idMap[i]->sprite );
                }
            
            delete [] idMap[i]->tag;

             if( idMap[i]->hitMap != NULL ) {
                delete [] idMap[i]->hitMap;    
                }

            delete idMap[i];
            }
        }

    delete [] idMap;

    if( blankSprite != NULL ) {
        freeSprite( blankSprite );
        }

    spriteBankLoaded = false;
    }






char *getSpriteBankLoadFailure() {
    return loadingFailureFileName;
    }



void stepSpriteBank() {
    // no more dynamic loading or unloading
    // they are all loaded at startup
    return;
    
    // keep this code in case we ever want to go back...

    for( int i=0; i<loadingSprites.size(); i++ ) {
        SpriteLoadingRecord *loadingR = loadingSprites.getElement( i );
        
        if( checkAsyncFileReadDone( loadingR->asyncLoadHandle ) ) {
            
            int length;
            unsigned char *data = getAsyncFileData( loadingR->asyncLoadHandle, 
                                                    &length );
            SpriteRecord *r = getSpriteRecord( loadingR->spriteID );

            
            if( data == NULL ) {
                printf( "Reading sprite data from file failed, sprite ID %d\n",
                        loadingR->spriteID );

                setLoadingFailureFileName(
                    autoSprintf( "sprites/%d.tga", loadingR->spriteID ) );
                }
            else {
                
                loadSpriteFromRawTGAData( loadingR->spriteID, data, length );
                
                delete [] data;
                }
            
            r->numStepsUnused = 0;
            loadedSprites.push_back( loadingR->spriteID );

            loadingSprites.deleteElement( i );
            i++;
            }
        }
    

    for( int i=0; i<loadedSprites.size(); i++ ) {
        int id = loadedSprites.getElementDirect( i );
    
        SpriteRecord *r = getSpriteRecord( id );
        
        r->numStepsUnused ++;

        if( r->numStepsUnused > 600 ) {
            // 10 seconds not drawn
            
            if( r->sprite != NULL ) {    
                freeSprite( r->sprite );
                r->sprite = NULL;
                }
            
            if( r->hitMap != NULL ) {
                delete [] r->hitMap;
                r->hitMap = NULL;
                }
            
            r->loading = false;

            loadedSprites.deleteElement( i );
            i--;
            }
        }
    }





SpriteRecord *getSpriteRecord( int inID ) {
    if( inID < mapSize ) {
        return idMap[inID];
        }
    else {
        return NULL;
        }
    }



char *getSpriteTag( int inID ) {
    SpriteRecord *r = getSpriteRecord( inID );
    
    if( r == NULL ) {
        return NULL;
        }
    
    return r->tag;
    }



char getUsesMultiplicativeBlending( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            return idMap[inID]->multiplicativeBlend;
            }
        }
    return false;
    }
    



#include "minorGems/util/random/CustomRandomSource.h"


static double remapFraction = 0;
static char remap = false;
static int remapSeed = 100;



void setRemapSeed( int inSeed ) {
    remapSeed = inSeed;
    }


    
void setRemapFraction( double inFraction ) {
    remapFraction = inFraction;
    if( inFraction > 0 ) {
        remap = true;
        }
    else {
        remap = false;
        }
    }



SpriteHandle getSprite( int inID ) {
    if( inID >= mapSize || idMap[ inID ] == NULL ) {
        return NULL;
        }


    if( remap && idMap[ inID ]->remappable ) {
        char remapThis = false;
        
        CustomRandomSource tempRand( inID + remapSeed );
        
        if( tempRand.getRandomBoundedDouble( 0, 1 ) <= remapFraction ) {
            remapThis = true;
            }
        

        if( remapThis ) {
        
            char multi = false;
            if( idMap[inID] != NULL ) {
                multi = idMap[inID]->multiplicativeBlend;
                }
        
        
            int id = inID;
            
            id += tempRand.getRandomBoundedInt( 0, mapSize );
            while( id >= mapSize ) {
                id -= mapSize;
                }
            while( idMap[id] == NULL || idMap[id]->sprite == NULL ||
                   idMap[id]->multiplicativeBlend != multi ||
                   ! idMap[id]->remapTarget ) {

                id ++;

                if( id >= mapSize ) {
                    id -= mapSize;
                    }
                }
            inID = id;
            }
        }
    
    

    if( idMap[inID]->sprite == NULL ) {
        loadSpriteImage( inID );
        return blankSprite;
        }
            
    idMap[inID]->numStepsUnused = 0;
    return idMap[inID]->sprite;
    }


    
char markSpriteLive( int inID ) {
    SpriteRecord *r = getSpriteRecord( inID );
    
    if( r == NULL ) {
        return false;
        }

    r->numStepsUnused = 0;
    
    if( r->sprite == NULL && ! r->loading ) {
        loadSpriteImage( inID );
        return false;
        }
    

    if( r->sprite != NULL ) {
        return true;
        }
    else {
        return false;
        }
    }



// return array destroyed by caller, NULL if none found
SpriteRecord **searchSprites( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining ) {
    
    if( strcmp( inSearch, "" ) == 0 ) {
        // special case, show sprites in reverse-id order, newest first
        SimpleVector< SpriteRecord *> results;
        
        int numSkipped = 0;
        int id = mapSize - 1;
        
        while( id > 0 && numSkipped < inNumToSkip ) {
            if( idMap[id] != NULL ) {
                numSkipped++;
                }
            id--;
            }
        
        int numGotten = 0;
        while( id > 0 && numGotten < inNumToGet ) {
            if( idMap[id] != NULL ) {
                results.push_back( idMap[id] );
                numGotten++;
                }
            id--;
            }
        
        // rough estimate
        *outNumRemaining = id;
        
        if( *outNumRemaining < 100 ) {
            // close enough to end, actually compute it
            *outNumRemaining = 0;
            
            while( id > 0 ) {
                if( idMap[id] != NULL ) {
                    *outNumRemaining = *outNumRemaining + 1;
                    }
                id--;
                }
            }
        

        *outNumResults = results.size();
        return results.getElementArray();
        }


    char *lower = stringToLowerCase( inSearch );
    
    int numTotalMatches = tree.countMatches( lower );
        
    int numAfterSkip = numTotalMatches - inNumToSkip;
    
    int numToGet = inNumToGet;
    if( numToGet > numAfterSkip ) {
        numToGet = numAfterSkip;
        }
    
    *outNumRemaining = numAfterSkip - numToGet;
        
    SpriteRecord **results = new SpriteRecord*[ numToGet ];
    
    
    *outNumResults = 
        tree.getMatches( lower, inNumToSkip, numToGet, (void**)results );
    
    delete [] lower;

    return results;
    }



int addSprite( const char *inTag, SpriteHandle inSprite,
               Image *inSourceImage,
               char inMultiplicativeBlending,
               int inCenterAnchorXOffset,
               int inCenterAnchorYOffset ) {

    int maxD = inSourceImage->getWidth();
    
    if( maxD < inSourceImage->getHeight() ) {
        maxD = inSourceImage->getHeight();
        }
    
    int newID = -1;


    // add it to file structure
    File spritesDir( NULL, "sprites" );
            
    if( !spritesDir.exists() ) {
        spritesDir.makeDirectory();
        }
    
    if( spritesDir.exists() && spritesDir.isDirectory() ) {
                
                
        int nextSpriteNumber = 1;
                
        File *nextNumberFile = 
            spritesDir.getChildFile( "nextSpriteNumber.txt" );
                
        if( nextNumberFile->exists() ) {
                    
            char *nextNumberString = 
                nextNumberFile->readFileContents();

            if( nextNumberString != NULL ) {
                sscanf( nextNumberString, "%d", &nextSpriteNumber );
                
                delete [] nextNumberString;
                }
            }
                
                    
            
        const char *printFormatTGA = "%d.tga";
        const char *printFormatTXT = "%d.txt";
        
        char *fileNameTGA = autoSprintf( printFormatTGA, nextSpriteNumber );
        char *fileNameTXT = autoSprintf( printFormatTXT, nextSpriteNumber );
            
        newID = nextSpriteNumber;

        File *cacheFile = spritesDir.getChildFile( "cache.fcz" );
        
        cacheFile->remove();
        
        delete cacheFile;


        File *spriteFile = spritesDir.getChildFile( fileNameTGA );
            
        TGAImageConverter tga;
            
        FileOutputStream stream( spriteFile );
        
        tga.formatImage( inSourceImage, &stream );
                    
        delete [] fileNameTGA;
        delete spriteFile;

        File *metaFile = spritesDir.getChildFile( fileNameTXT );

        int multFlag = 0;
        if( inMultiplicativeBlending ) {
            multFlag = 1;
            }
        
        char *metaContents = autoSprintf( "%s %d %d %d", inTag, multFlag,
                                          inCenterAnchorXOffset, 
                                          inCenterAnchorYOffset );

        metaFile->writeToFile( metaContents );
        
        delete [] metaContents;
        delete [] fileNameTXT;
        delete metaFile;

        nextSpriteNumber++;
        

                
        char *nextNumberString = autoSprintf( "%d", nextSpriteNumber );
        
        nextNumberFile->writeToFile( nextNumberString );
        
        delete [] nextNumberString;
                
        
        delete nextNumberFile;
        }
    
    if( newID == -1 ) {
        // failed to save it to disk
        return -1;
        }

    
    // now add it to live, in memory database
    if( newID >= mapSize ) {
        // expand map

        int newMapSize = newID + 1;
        

        
        SpriteRecord **newMap = new SpriteRecord*[newMapSize];
        
        for( int i=0; i<newMapSize; i++ ) {
            newMap[i] = NULL;
            }

        memcpy( newMap, idMap, sizeof(SpriteRecord*) * mapSize );

        delete [] idMap;
        idMap = newMap;
        mapSize = newMapSize;
        }

    SpriteRecord *r = new SpriteRecord;
    
    r->id = newID;
    r->sprite = inSprite;
    r->tag = stringDuplicate( inTag );
    r->maxD = maxD;
    r->multiplicativeBlend = inMultiplicativeBlending;
    

    r->w = inSourceImage->getWidth();
    r->h = inSourceImage->getHeight();
    
    r->visibleW = r->w;
    r->visibleH = r->h;

    r->centerXOffset = 0;
    r->centerYOffset = 0;
    
    r->centerAnchorXOffset = inCenterAnchorXOffset;
    r->centerAnchorYOffset = inCenterAnchorYOffset;
    
    doublePair offset = { (double)( r->centerAnchorXOffset ), 
                          (double)( r->centerAnchorYOffset ) };
    
    setSpriteCenterOffset( r->sprite, offset );

    int numPixels = r->w * r->h;
    r->hitMap = new char[ numPixels ];
    
    memset( r->hitMap, 1, numPixels );

    int minX = r->w;
    int maxX = 0;
    
    int minY = r->h;
    int maxY = 0;
    
    int w = r->w;
    
    
    if( inSourceImage->getNumChannels() == 4 ) {
        
        double *a = inSourceImage->getChannel(3);
        
        for( int p=0; p<numPixels; p++ ) {
            if( a[p] < 0.25 ) {
                r->hitMap[p] = 0;
                }
            else {
                int y = p / w;
                int x = p % w;
                
                if( y < minY ) {
                    minY = y;
                    }
                if( y > maxY ) {
                    maxY = y;
                    }
                
                if( x < minX ) {
                    minX = x;
                    }
                if( x > maxX ) {
                    maxX = x;
                    }
                }
            }
        }
    
    for( int e=0; e<3; e++ ) {    
        expandMap( r->hitMap, r->w, r->h );
        }
    
    
    r->centerXOffset = 
        ( maxX + minX ) / 2 - 
        r->w / 2;
    
    r->centerYOffset = 
        ( maxY + minY ) / 2 - 
        r->h / 2;

    r->visibleW = maxX - minX;
    r->visibleH = maxY - minY;
    
    
    // delete old
    freeSpriteRecord( newID );
    
    idMap[newID] = r;

    if( makeNewSpritesSearchable ) {
        
        char *lower = stringToLowerCase( inTag );
        
        tree.insert( lower, idMap[newID] );
        
        delete [] lower;
        }
    
    loadedSprites.push_back( r->id );

    r->loading = false;
    r->numStepsUnused = 0;

    return newID;
    }




int bakeSprite( const char *inTag,
                int inNumSprites,
                int *inSpriteIDs,
                doublePair *inSpritePos,
                double *inSpriteRot,
                char *inSpriteHFlips,
                FloatRGB *inSpriteColors ) {
    
    File spritesDir( NULL, "sprites" );
            
    // first, find max dimensions of base image
    int baseRadiusX = 0;
    int baseRadiusY = 0;
    
    int *xOffsets = new int[ inNumSprites ];
    int *yOffsets = new int[ inNumSprites ];


    for( int i=0; i<inNumSprites; i++ ) {
        xOffsets[i] = lrint( inSpritePos[i].x );
        yOffsets[i] = lrint( inSpritePos[i].y );

        SpriteRecord *r = getSpriteRecord( inSpriteIDs[i] );
        int radiusXA = r->w / 2 + 
            abs( r->centerAnchorXOffset ) + 
            abs( xOffsets[i] );
        
        // consider rotations
        int radiusXB = r->h / 2 + 
            abs( r->centerAnchorYOffset ) + 
            abs( xOffsets[i] );
        

        int radiusYA = r->h / 2 + 
                 abs( r->centerAnchorYOffset )  + 
                 abs( yOffsets[i] );

        // consider rotations
        int radiusYB = r->w / 2 + 
                 abs( r->centerAnchorXOffset )  + 
                 abs( yOffsets[i] );

        
        if( radiusXA > baseRadiusX ) {
            baseRadiusX = radiusXA;
            }
        if( radiusYA > baseRadiusY ) {
            baseRadiusY = radiusYA;
            }
        
        if( radiusXB > baseRadiusX ) {
            baseRadiusX = radiusXB;
            }
        if( radiusYB > baseRadiusY ) {
            baseRadiusY = radiusYB;
            }
        }

    int baseW = baseRadiusX * 2;
    int baseH = baseRadiusY * 2;

    int baseCenterX = baseW / 2;
    int baseCenterY = baseH / 2;

    Image baseImage( baseW, baseH, 4, true );

    double *baseChan[4];
    for( int c=0; c<4; c++ ) {
        baseChan[c] = baseImage.getChannel( c );
        }
            
    
    for( int i=0; i<inNumSprites; i++ ) {
        SpriteRecord *spriteRec = getSpriteRecord( inSpriteIDs[i] );
        
        char *fileNameTGA = autoSprintf( "%d.tga", inSpriteIDs[i] );
        

        File *spriteFile = spritesDir.getChildFile( fileNameTGA );
            
        delete [] fileNameTGA;
            

        char *fullName = spriteFile->getFullFileName();
        
        delete spriteFile;
        
        Image *image = readTGAFileBase( fullName );
        
        delete [] fullName;

        if( image != NULL ) {
            
            int w = image->getWidth();
            int h = image->getHeight();


            if( inSpriteHFlips[i] ||
                inSpriteRot[i] != 0 ) {
                
                // expand until square to permit rotations without
                // getting cut off
                
                int newWidth = w;
                int newHeight = h;
                if( w < h ) {
                    newWidth = h;
                    }
                else if( h < w ) {
                    newHeight = w;
                    }
                
                int totalPossibleOffset = 
                    2 * abs( spriteRec->centerAnchorXOffset ) +
                    2 * abs( spriteRec->centerAnchorYOffset );
                
                newWidth += totalPossibleOffset;
                newHeight += totalPossibleOffset;

                if( newWidth != w ||
                    newHeight != h ) {
                    Image *biggerImage = image->expandImage( newWidth,
                                                             newHeight );
                    delete image;
                    image = biggerImage;
                    
                    w = newWidth;
                    h = newHeight;
                    }
                }


            int centerX = w/2 + spriteRec->centerAnchorXOffset;
            int centerY = h/2 + spriteRec->centerAnchorYOffset;
            

            double *chan[4];
            for( int c=0; c<4; c++ ) {
                chan[c] = image->getChannel( c );
                }
            
            FloatRGB spriteColor = inSpriteColors[i];

            float spriteColorParts[3] = {
                spriteColor.r,
                spriteColor.g,
                spriteColor.b };
            

            // number of clockwise 90 degree rotations
            int numRotSteps = 0;
            
            if( inSpriteRot[i] != 0 ) {
                double rot = inSpriteRot[i];

                if( inSpriteHFlips[i] ) {
                    rot *= -1;
                    }

                while( rot < 0 ) {
                    rot += 1.0;
                    }
                while( rot > 1.0 ) {
                    rot -= 1.0;
                    }

                numRotSteps = lrint( rot / 0.25 );
                }
            

            for( int y = 0; y<h; y++ ) {
                int baseY = ( y - centerY ) - yOffsets[i] + baseCenterY;
                
                for( int x = 0; x<w; x++ ) {
                    int baseX = ( x - centerX ) + 
                        xOffsets[i] + baseCenterX;

                    int finalX = x;
                    int finalY = y;
                    
                    int xFromCenter = x - centerX;
                    int yFromCenter = (y - centerY);
                    
                    if( inSpriteHFlips[i] ) {
                        xFromCenter *= -1;
                        xFromCenter -= 1;
                        }

                    for( int r=0; r<numRotSteps; r++ ) {
                        int newX = yFromCenter;
                        int newY = - xFromCenter;
                        xFromCenter = newX;
                        yFromCenter = newY;
                        }
                    
                    finalX = xFromCenter + centerX;
                    finalY = yFromCenter + centerY;
                    
                    
                    // special case tweaks found by trial and error
                    if( numRotSteps == 1 ) {
                        finalY -= 1;
                        }
                    else if( numRotSteps == 2 ) {
                        finalY -= 1;
                        finalX -= 1;
                        }
                    else if( numRotSteps == 3 ) {
                        finalX -= 1;
                        }
                    
                    
                    // might rotate out, skip these pixels if so
                    if( finalY >= h || finalY < 0 ||
                        finalX >= w || finalX < 0 ) {
                        continue;
                        }
                        

                    int i = finalY * w + finalX;
                    
                    int baseI = baseY * baseW + baseX;
                    
                    if( ! spriteRec->multiplicativeBlend ) {
                        
                        for( int c=0; c<3; c++ ) {
                            // blend dest and source using source alpha
                            // as weight
                            baseChan[c][baseI] = 
                                (1 - chan[3][i] ) * baseChan[c][baseI] +
                                chan[3][i] * chan[c][i] * spriteColorParts[c];
                            }
                        
                        // add alphas
                        baseChan[3][baseI] += chan[3][i];
                        if( baseChan[3][baseI] > 1.0 ) {
                            baseChan[3][baseI] = 1.0;
                            }
                        }
                    else {
                        // multiplicative blend
                        // ignore alphas

                        // note that this will NOT work
                        // if multiplicative sprite hangs out
                        // beyond border of opaque non-multiplicative
                        // parts below it.
                        for( int c=0; c<3; c++ ) {
                            baseChan[c][baseI] *= chan[c][i];
                            }
                        }
                    }
                }
            
            delete image;
            }
        }
    
    delete [] xOffsets;
    delete [] yOffsets;
    
    // find max extent of non-transparent area

    int maxX = 0;
    int minX = baseW - 1;
    
    int maxY = 0;
    int minY = baseH - 1;

    for( int y=0; y<baseH; y++ ) {
        for( int x=0; x<baseW; x++ ) {
            int i = y * baseW + x;
            
            if( baseChan[3][i] > 0.0 ) {
                
                if( x > maxX  ) {
                    maxX = x;
                    }
                if( x < minX ) {
                    minX = x;
                    }
                if( y > maxY  ) {
                    maxY = y;
                    }
                if( y < minY ) {
                    minY = y;
                    }
                
                }
            }
        }
    
    int newCenterX = ( maxX + minX ) / 2;
    int newCenterY = ( maxY + minY ) / 2;
    
    int centerAnchorXOffset = baseCenterX - newCenterX - 1;
    int centerAnchorYOffset = baseCenterY - newCenterY - 1;

    Image *trimmed = baseImage.getSubImage( minX, minY,
                                            1 + maxX - minX, 1 + maxY - minY );
    
    int w = 1;
    int h = 1;
                    
    while( w < trimmed->getWidth() ) {
        w *= 2;
        }
    while( h < trimmed->getHeight() ) {
        h *= 2;
        }
    
    Image *expanded = trimmed->expandImage( w, h );

    delete trimmed;


    SpriteHandle s = fillSprite( expanded, false );
    
    int returnID = 
        addSprite( inTag, s, 
                   expanded,
                   false,
                   centerAnchorXOffset, centerAnchorYOffset );
    
    delete expanded;
    
    return returnID;
    }




void deleteSpriteFromBank( int inID ) {
    File spritesDir( NULL, "sprites" );

    for( int i=0; i<loadingSprites.size(); i++ ) {
        SpriteLoadingRecord *loadingR = loadingSprites.getElement( i );
    
        if( loadingR->spriteID == inID ) {
            // block deletion of sprite that hasn't loaded yet

            // this is a rare case of a user's clicks beating the disk
            // but we still need to prevent a crash here.
            return;
            }
        }

    
    
    if( spritesDir.exists() && spritesDir.isDirectory() ) {    

        const char *printFormatTGA = "%d.tga";
        const char *printFormatTXT = "%d.txt";

        char *fileNameTGA = autoSprintf( printFormatTGA, inID );
        char *fileNameTXT = autoSprintf( printFormatTXT, inID );
            
        File *spriteFileTGA = spritesDir.getChildFile( fileNameTGA );
        File *spriteFileTXT = spritesDir.getChildFile( fileNameTXT );

            
        File *cacheFile = spritesDir.getChildFile( "cache.fcz" );
            
        cacheFile->remove();
        
        delete cacheFile;


        loadedSprites.deleteElementEqualTo( inID );
        
        spriteFileTGA->remove();
        spriteFileTXT->remove();
            
        delete [] fileNameTGA;
        delete [] fileNameTXT;
        delete spriteFileTGA;
        delete spriteFileTXT;
        }
    
    
    freeSpriteRecord( inID );
    }




char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL && idMap[inID]->sprite != NULL ) {
            
            SpriteRecord *r = idMap[inID];

            int pixX = inXCenterOffset + r->w / 2;
            int pixY = - inYCenterOffset + r->h / 2;
            
            if( pixX >=0 && pixX < r->w 
                &&
                pixY >=0 && pixY < r->h ) {

                int pixI = r->w * pixY + pixX;
                
                return r->hitMap[ pixI ];
                }
            }
        }
    
    return false;
    }



void countLoadedSprites( int *outLoaded, int *outTotal ) {
    int loaded = 0;
    int total = 0;
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            total ++;
            if( idMap[i]->sprite != NULL ) {
                loaded ++;
                }
            }
        }
    
    *outLoaded = loaded;
    *outTotal = total;
    }

    



