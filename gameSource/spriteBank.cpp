
#include "spriteBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"

#include "minorGems/game/game.h"


#include "folderCache.h"



static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static SpriteRecord **idMap;


static StringTree tree;


static FolderCache cache;

static int currentFile;


static SimpleVector<SpriteRecord*> records;
static int maxID;


static File spritesDir( NULL, "sprites" );


static SpriteHandle blankSprite;



typedef struct SpriteLoadingRecord {
        int spriteID;
        int asyncLoadHandle;
        
    } SpriteLoadingRecord;


static SimpleVector<SpriteLoadingRecord> loadingSprites;

static SimpleVector<int> loadedSprites;



int getMaxSpriteID() {
    return maxID;
    }



int initSpriteBankStart( char *outRebuildingCache ) {
    maxID = 0;
    
    currentFile = 0;

    cache = initFolderCache( "sprites", outRebuildingCache );

    unsigned char onePixel[4] = { 0, 0, 0, 0 };
    

    blankSprite = fillSprite( onePixel, 1, 1 );
    
    return cache.numFiles;
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




float initSpriteBankStep() {
    
    if( currentFile == cache.numFiles ) {
        return 1.0;
        }
    
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
        
        r->maxD = 2;

        r->id = 0;
        
        sscanf( fileName, "%d.txt", &( r->id ) );
                
                
        char *contents = getFileContents( cache, i );
                
        r->tag = NULL;

        r->centerXOffset = 0;
        r->centerYOffset = 0;

        if( contents != NULL ) {
            
            SimpleVector<char *> *tokens = tokenizeString( contents );
            int numTokens = tokens->size();
            
            if( numTokens >= 2 ) {
                        
                r->tag = 
                    stringDuplicate( tokens->getElementDirect( 0 ) );
                        
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
    return (float)( currentFile ) / (float)( cache.numFiles );
    }

 

void initSpriteBankFinish() {    

    freeFolderCache( cache );
    
    mapSize = maxID + 1;
    
    idMap = new SpriteRecord*[ mapSize ];
    
    for( int i=0; i<mapSize; i++ ) {
        idMap[i] = NULL;
        }

    int numRecords = records.size();
    for( int i=0; i<numRecords; i++ ) {
        SpriteRecord *r = records.getElementDirect(i);
        
        idMap[ r->id ] = r;
        
        char *lower = stringToLowerCase( r->tag );
        
        tree.insert( lower, r );
        
        delete [] lower;
        }

    printf( "Loaded %d tagged sprites from sprites folder\n", numRecords );
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

    freeSprite( blankSprite );
    }



void stepSpriteBank() {
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
                }
            else {
                
                
                RawRGBAImage *spriteImage = readTGAFileRawFromBuffer( data, 
                                                                      length );

                if( spriteImage != NULL && spriteImage->mNumChannels != 4 ) {
                    printf( "Sprite loading for id %d not a 4-channel image, "
                            "failed to load.\n",
                            loadingR->spriteID );
                    delete spriteImage;
                    spriteImage = NULL;
                    }
                            
                if( spriteImage != NULL ) {
                
                        
                    r->sprite =
                        fillSprite( spriteImage->mRGBABytes, 
                                    spriteImage->mWidth,
                                    spriteImage->mHeight );
                
                    r->w = spriteImage->mWidth;
                    r->h = spriteImage->mHeight;                
                    
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
                    
                    delete spriteImage;
                    }
                
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
            
            freeSprite( r->sprite );
            r->sprite = NULL;
            
            delete [] r->hitMap;
            r->hitMap = NULL;

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



char getUsesMultiplicativeBlending( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            return idMap[inID]->multiplicativeBlend;
            }
        }
    return false;
    }
    


SpriteHandle getSprite( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            if( idMap[inID]->sprite == NULL ) {
                loadSpriteImage( inID );
                return blankSprite;
                }
            
            idMap[inID]->numStepsUnused = 0;
            return idMap[inID]->sprite;
            }
        }
    return NULL;
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
               char inMultiplicativeBlending ) {

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
        
        char *metaContents = autoSprintf( "%s %d", inTag, multFlag );

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
    
    int numPixels = r->w * r->h;
    r->hitMap = new char[ numPixels ];
    
    memset( r->hitMap, 1, numPixels );
    
    if( inSourceImage->getNumChannels() == 4 ) {
        
        double *a = inSourceImage->getChannel(3);
        
        for( int p=0; p<numPixels; p++ ) {
            if( a[p] < 0.25 ) {
                r->hitMap[p] = 0;
                }
            }
        }
    
    for( int e=0; e<3; e++ ) {    
        expandMap( r->hitMap, r->w, r->h );
        }
    
    
    // delete old
    freeSpriteRecord( newID );
    
    idMap[newID] = r;
    
    char *lower = stringToLowerCase( inTag );
    
    tree.insert( lower, idMap[newID] );

    delete [] lower;

    loadedSprites.push_back( r->id );

    r->loading = false;
    r->numStepsUnused = 0;

    return newID;
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
            
            int pixX = inXCenterOffset + idMap[inID]->w / 2;
            int pixY = - inYCenterOffset + idMap[inID]->h / 2;
            
            if( pixX >=0 && pixX < idMap[inID]->w 
                &&
                pixY >=0 && pixY < idMap[inID]->h ) {

                int pixI = idMap[inID]->w * pixY + pixX;
                
                return idMap[inID]->hitMap[ pixI ];
                }
            }
        }
    
    return false;
    }



