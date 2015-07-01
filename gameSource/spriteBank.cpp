
#include "spriteBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"



static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static SpriteRecord **idMap;


static StringTree tree;




void initSpriteBank() {
    SimpleVector<SpriteRecord*> records;
    int maxID = 0;
    
    File spritesDir( NULL, "sprites" );
    if( spritesDir.exists() && spritesDir.isDirectory() ) {

        int numFiles;
        File **childFiles = spritesDir.getChildFiles( &numFiles );

        for( int i=0; i<numFiles; i++ ) {
            
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
            }
        

        delete [] childFiles;
        }
    
    mapSize = maxID + 1;
    
    idMap = new SpriteRecord*[ mapSize ];
    
    for( int i=0; i<mapSize; i++ ) {
        idMap[i] = NULL;
        }

    int numRecords = records.size();
    for( int i=0; i<numRecords; i++ ) {
        SpriteRecord *r = records.getElementDirect(i);
        
        idMap[ r->id ] = r;
        
        tree.insert( r->tag, r );
        }

    printf( "Loaded %d tagged sprites from sprites folder\n", numRecords );
    }




static void freeSpriteRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
         
            freeSprite( idMap[inID]->sprite );
            
            tree.remove( idMap[inID]->tag, idMap[inID] );
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
    
    int numTotalMatches = tree.countMatches( inSearch );
        
    int numAfterSkip = numTotalMatches - inNumToSkip;
    
    int numToGet = inNumToGet;
    if( numToGet > numAfterSkip ) {
        numToGet = numAfterSkip;
        }
    
    *outNumRemaining = numAfterSkip - numToGet;
        
    SpriteRecord **results = new SpriteRecord*[ numToGet ];
    
    
        // outValues must have space allocated by caller for inNumToGet 
        // pointers
    *outNumResults = 
        tree.getMatches( inSearch, inNumToSkip, numToGet, (void**)results );
    
    return results;
    }



void addSprite( int inID, const char *inTag, SpriteHandle inSprite ) {
    if( inID >= mapSize ) {
        // expand map

        int newMapSize = inID + 1;
        

        
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
    
    r->id = inID;
    r->sprite = inSprite;
    r->tag = stringDuplicate( inTag );
    

    // delete old
    freeSpriteRecord( inID );
    
    idMap[inID] = r;
    
    tree.insert( inTag, idMap[inID] );
    }


