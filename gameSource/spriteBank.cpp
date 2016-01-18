
#include "spriteBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"



static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static SpriteRecord **idMap;


static StringTree tree;



static int numFiles;
static File **childFiles;

static int currentFile;


static SimpleVector<SpriteRecord*> records;
static int maxID;


void initSpriteBankStart() {
    maxID = 0;


    File spritesDir( NULL, "sprites" );
    
    numFiles = 0;
    currentFile = 0;

    if( spritesDir.exists() && spritesDir.isDirectory() ) {

        childFiles = spritesDir.getChildFiles( &numFiles );
        }
    }


float initSpriteBankStep() {
    
    if( numFiles == 0 ) {
        return 1.0;
        }
    
    int i = currentFile;
            
    if( childFiles[i]->isDirectory() ) {
                
        char *tag = childFiles[i]->getFileName();
                
        int numTGAFiles;
        File **tgaFiles = childFiles[i]->getChildFiles( &numTGAFiles );
                
        for( int t=0; t<numTGAFiles; t++ ) {
                    
            if( ! tgaFiles[t]->isDirectory() ) {
                        
                char *tgaFileName = tgaFiles[t]->getFileName();
                        
                if( strstr( tgaFileName, ".tga" ) != NULL ) {
                            
                    // a tga file!

                    char *fullName = tgaFiles[t]->getFullFileName();
                            
                    printf( "Loading sprite from path %s, tag %s\n", 
                            fullName, tag );
                            
                    SpriteHandle sprite =
                        loadSpriteBase( fullName, false );
                            
                    delete [] fullName;
                            
                            
                    if( sprite != NULL ) {
                        SpriteRecord *r = new SpriteRecord;

                        r->id = 0;
                                
                        sscanf( tgaFileName, "%d.tga", &( r->id ) );
                                
                        printf( "Scanned id = %d\n", r->id );
                                
                        r->sprite = sprite;
                        r->tag = stringDuplicate( tag );
                                
                        records.push_back( r );

                        if( maxID < r->id ) {
                            maxID = r->id;
                            }
                        }
                    }
                delete [] tgaFileName;
                }
                    
            delete tgaFiles[t];
            }
                
        delete [] tag;
                
        delete [] tgaFiles;
        }
            
    delete childFiles[i];


    currentFile ++;
    return (float)( currentFile ) / (float)( numFiles );
    }
 

void initSpriteBankFinish() {    

    delete [] childFiles;
    
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




static void freeSpriteRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
         
            freeSprite( idMap[inID]->sprite );
            
            char *lower = stringToLowerCase( idMap[inID]->tag );
            
            tree.remove( lower, idMap[inID] );

            delete [] lower;

            delete [] idMap[inID]->tag;
            
            delete idMap[inID];
            idMap[inID] = NULL;
            return ;
            }
        }
    }




void freeSpriteBank() {
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            freeSprite( idMap[i]->sprite );
            delete [] idMap[i]->tag;

            delete idMap[i];
            }
        }

    delete [] idMap;
    }





SpriteHandle getSprite( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            return idMap[inID]->sprite;
            }
        }
    return NULL;
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
               Image *inSourceImage ) {

    
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
                
                    
        File *tagDir = spritesDir.getChildFile( inTag );
        
        if( !tagDir->exists() ) {
            tagDir->makeDirectory();
            }
                
        
        if( tagDir->exists() && tagDir->isDirectory() ) {
            
            char *fileName = autoSprintf( "%d.tga", nextSpriteNumber );
            
            newID = nextSpriteNumber;

            File *spriteFile = tagDir->getChildFile( fileName );
            
            TGAImageConverter tga;
            
            FileOutputStream stream( spriteFile );
            
            tga.formatImage( inSourceImage, &stream );
                    
            delete [] fileName;
            delete spriteFile;

            nextSpriteNumber++;
            }

                
        char *nextNumberString = autoSprintf( "%d", nextSpriteNumber );
        
        nextNumberFile->writeToFile( nextNumberString );
        
        delete [] nextNumberString;
                
        
        delete nextNumberFile;
        delete tagDir;
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
    

    // delete old
    freeSpriteRecord( newID );
    
    idMap[newID] = r;
    
    char *lower = stringToLowerCase( inTag );
    
    tree.insert( lower, idMap[newID] );

    delete [] lower;

    return newID;
    }



void deleteSpriteFromBank( int inID ) {
    SpriteRecord *r = idMap[ inID ];

    
    File spritesDir( NULL, "sprites" );
    
    
    if( spritesDir.exists() && spritesDir.isDirectory() ) {                
                    
        File *tagDir = spritesDir.getChildFile( r->tag );
        
        if( tagDir->exists() && tagDir->isDirectory() ) {
            
            char *fileName = autoSprintf( "%d.tga", inID );
            
            File *spriteFile = tagDir->getChildFile( fileName );
            
            spriteFile->remove();
            
            delete [] fileName;
            delete spriteFile;
            
            tagDir->remove();
            }
        delete tagDir;
        }
    
    
    freeSpriteRecord( inID );
    }



