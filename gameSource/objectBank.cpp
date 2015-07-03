
#include "objectBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"


#include "spriteBank.h"



static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static ObjectRecord **idMap;


static StringTree tree;




void initObjectBank() {
    SimpleVector<ObjectRecord*> records;
    int maxID = 0;
    
    File objectsDir( NULL, "objects" );
    if( objectsDir.exists() && objectsDir.isDirectory() ) {

        int numFiles;
        File **childFiles = objectsDir.getChildFiles( &numFiles );

        for( int i=0; i<numFiles; i++ ) {
            
            if( ! childFiles[i]->isDirectory() ) {
                                        
                char *txtFileName = childFiles[i]->getFileName();
                        
                if( strstr( txtFileName, ".txt" ) != NULL &&
                    strcmp( txtFileName, "nextObjectNumber.txt" ) != 0 ) {
                            
                    // an object txt file!

                    char *fullName = childFiles[i]->getFullFileName();
                            
                    printf( "Loading object from path %s\n", fullName );
                            
                    delete [] fullName;
                    
                    char *objectText = childFiles[i]->readFileContents();
                    
        
                    if( objectText != NULL ) {
                        int numLines;
                        
                        char **lines = split( objectText, "\n", &numLines );
                        
                        delete [] objectText;

                        if( numLines >= 3 ) {
                            ObjectRecord *r = new ObjectRecord;
                            
                            r->id = 0;
                            sscanf( lines[0], "id=%d", 
                                    &( r->id ) );
                            
                            if( r->id > maxID ) {
                                maxID = r->id;
                                }
                            
                            r->description = stringDuplicate( lines[1] );
                            
                            r->numSprites = 0;
                            sscanf( lines[2], "numSprites=%d", 
                                    &( r->numSprites ) );
                            
                            r->sprites = new int[r->numSprites];
                            r->spritePos = new doublePair[ r->numSprites ];
                            
                            for( int i=0; i< r->numSprites; i++ ) {
                                sscanf( lines[3 + i * 2], "spriteID=%d", 
                                        &( r->sprites[i] ) );
                                
                                sscanf( lines[4 + i * 2], "pos=%lf,%lf", 
                                        &( r->spritePos[i].x ),
                                        &( r->spritePos[i].y ) );
                                }
                            
                            records.push_back( r );
                            }
                            
                        for( int i=0; i<numLines; i++ ) {
                            delete [] lines[i];
                            }
                        delete [] lines;
                        }
                    }
                
                delete [] txtFileName;
                }
            delete childFiles[i];
            }
        

        delete [] childFiles;
        }
    
    mapSize = maxID + 1;
    
    idMap = new ObjectRecord*[ mapSize ];
    
    for( int i=0; i<mapSize; i++ ) {
        idMap[i] = NULL;
        }

    int numRecords = records.size();
    for( int i=0; i<numRecords; i++ ) {
        ObjectRecord *r = records.getElementDirect(i);
        
        idMap[ r->id ] = r;
        
        tree.insert( r->description, r );
        }

    printf( "Loaded %d objects from objects folder\n", numRecords );
    }




static void freeObjectRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            
            tree.remove( idMap[inID]->description, idMap[inID] );
            
            delete [] idMap[inID]->description;
            delete [] idMap[inID]->sprites;
            delete [] idMap[inID]->spritePos;
            
            delete idMap[inID];
            idMap[inID] = NULL;
            return ;
            }
        }
    }




void freeObjectBank() {
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            delete [] idMap[i]->description;
            delete [] idMap[i]->sprites;
            delete [] idMap[i]->spritePos;

            delete idMap[i];
            }
        }

    delete [] idMap;
    }





ObjectRecord *getObject( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            return idMap[inID];
            }
        }
    return NULL;
    }

    


// return array destroyed by caller, NULL if none found
ObjectRecord **searchObjects( const char *inSearch, 
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
        
    ObjectRecord **results = new ObjectRecord*[ numToGet ];
    
    
        // outValues must have space allocated by caller for inNumToGet 
        // pointers
    *outNumResults = 
        tree.getMatches( inSearch, inNumToSkip, numToGet, (void**)results );
    
    return results;
    }



int addObject( const char *inDescription,
               int inNumSprites, int *inSprites, 
               doublePair *inSpritePos ) {
    

    
    int newID = -1;


    // add it to file structure
    File objectsDir( NULL, "objects" );
            
    if( !objectsDir.exists() ) {
        objectsDir.makeDirectory();
        }
    
    if( objectsDir.exists() && objectsDir.isDirectory() ) {
                
                
        int nextObjectNumber = 1;
                
        File *nextNumberFile = 
            objectsDir.getChildFile( "nextObjectNumber.txt" );
                
        if( nextNumberFile->exists() ) {
                    
            char *nextNumberString = 
                nextNumberFile->readFileContents();

            if( nextNumberString != NULL ) {
                sscanf( nextNumberString, "%d", &nextObjectNumber );
                
                delete [] nextNumberString;
                }
            }
                
            
        char *fileName = autoSprintf( "%d.txt", nextObjectNumber );
            
        newID = nextObjectNumber;

        File *objectFile = objectsDir.getChildFile( fileName );
        

        SimpleVector<char*> lines;
        
        lines.push_back( autoSprintf( "id=%d", newID ) );
        lines.push_back( stringDuplicate( inDescription ) );
        
        lines.push_back( autoSprintf( "numSprites=%d", inNumSprites ) );

        for( int i=0; i<inNumSprites; i++ ) {
            lines.push_back( autoSprintf( "spriteID=%d", inSprites[i] ) );
            lines.push_back( autoSprintf( "pos=%f,%f", 
                                          inSpritePos[i].x,
                                          inSpritePos[i].y ) );
            }
        
        char **linesArray = lines.getElementArray();
        
        
        char *contents = join( linesArray, lines.size(), "\n" );

        delete [] linesArray;
        lines.deallocateStringElements();
        

        objectFile->writeToFile( contents );
        
        delete [] contents;
        
            
        delete [] fileName;
        delete objectFile;
        
        nextObjectNumber++;
        

        
        char *nextNumberString = autoSprintf( "%d", nextObjectNumber );
        
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
        

        
        ObjectRecord **newMap = new ObjectRecord*[newMapSize];
        
        for( int i=0; i<newMapSize; i++ ) {
            newMap[i] = NULL;
            }

        memcpy( newMap, idMap, sizeof(ObjectRecord*) * mapSize );

        delete [] idMap;
        idMap = newMap;
        mapSize = newMapSize;
        }

    ObjectRecord *r = new ObjectRecord;
    
    r->id = newID;
    r->description = stringDuplicate( inDescription );
    r->numSprites = inNumSprites;
    
    r->sprites = new int[ inNumSprites ];
    r->spritePos = new doublePair[ inNumSprites ];
    
    memcpy( r->sprites, inSprites, inNumSprites * sizeof( int ) );
    memcpy( r->spritePos, inSpritePos, inNumSprites * sizeof( doublePair ) );
    


    // delete old
    freeObjectRecord( newID );
    
    idMap[newID] = r;
    
    tree.insert( inDescription, idMap[newID] );

    return newID;
    }




void drawObject( ObjectRecord *inObject, doublePair inPos ) {
    for( int i=0; i<inObject->numSprites; i++ ) {
        doublePair pos = add( inObject->spritePos[i], inPos );
        
        drawSprite( getSprite( inObject->sprites[i] ), pos );
        }    
    }

