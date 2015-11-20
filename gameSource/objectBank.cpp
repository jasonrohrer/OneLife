
#include "objectBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"


#include "spriteBank.h"

#include "ageControl.h"


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

                        if( numLines >= 7 ) {
                            ObjectRecord *r = new ObjectRecord;
                            
                            int next = 0;
                            
                            r->id = 0;
                            sscanf( lines[next], "id=%d", 
                                    &( r->id ) );
                            
                            if( r->id > maxID ) {
                                maxID = r->id;
                                }
                            
                            next++;
                            
                            r->description = stringDuplicate( lines[next] );
                            
                            next++;
                            
                            int contRead = 0;                            
                            sscanf( lines[next], "containable=%d", 
                                    &( contRead ) );
                            
                            r->containable = contRead;
                            
                            next++;
                            
                            
                            int permRead = 0;                            
                            sscanf( lines[next], "permanent=%d", 
                                    &( permRead ) );
                            
                            r->permanent = permRead;

                            next++;


                            r->heatValue = 0;                            
                            sscanf( lines[next], "heatValue=%d", 
                                    &( r->heatValue ) );
                            
                            next++;


                            r->rValue = 0;                            
                            sscanf( lines[next], "rValue=%f", 
                                    &( r->rValue ) );
                            
                            next++;



                            int personRead = 0;                            
                            sscanf( lines[next], "person=%d", 
                                    &( personRead ) );
                            
                            r->person = personRead;
                            
                            next++;



                            r->numSlots = 0;
                            sscanf( lines[next], "numSlots=%d", 
                                    &( r->numSlots ) );
                            
                            next++;

                            r->slotPos = new doublePair[ r->numSlots ];
                            
                            for( int i=0; i< r->numSlots; i++ ) {
                                sscanf( lines[ next ], "slotPos=%lf,%lf", 
                                        &( r->slotPos[i].x ),
                                        &( r->slotPos[i].y ) );
                                next++;
                                }
                            

                            r->numSprites = 0;
                            sscanf( lines[next], "numSprites=%d", 
                                    &( r->numSprites ) );
                            
                            next++;

                            r->sprites = new int[r->numSprites];
                            r->spritePos = new doublePair[ r->numSprites ];
                            
                            for( int i=0; i< r->numSprites; i++ ) {
                                sscanf( lines[next], "spriteID=%d", 
                                        &( r->sprites[i] ) );
                                
                                next++;
                                
                                sscanf( lines[next], "pos=%lf,%lf", 
                                        &( r->spritePos[i].x ),
                                        &( r->spritePos[i].y ) );
                                
                                next++;
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
        
        char *lowercase = stringToLowerCase( r->description );
        
        tree.insert( lowercase, r );

        delete [] lowercase;
        }

    printf( "Loaded %d objects from objects folder\n", numRecords );
    }




static void freeObjectRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            
            char *lower = stringToLowerCase( idMap[inID]->description );
            
            tree.remove( lower, idMap[inID] );
            
            delete [] lower;
            

            delete [] idMap[inID]->description;
            delete [] idMap[inID]->slotPos;
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
            
            delete [] idMap[i]->slotPos;
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



int getNumContainerSlots( int inID ) {
    ObjectRecord *r = getObject( inID );
    
    if( r == NULL ) {
        return 0;
        }
    else {
        return r->numSlots;
        }
    }



char isContainable( int inID ) {
    ObjectRecord *r = getObject( inID );
    
    if( r == NULL ) {
        return false;
        }
    else {
        return r->containable;
        }
    }

    


// return array destroyed by caller, NULL if none found
ObjectRecord **searchObjects( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining ) {
    
    char *lowerSearch = stringToLowerCase( inSearch );

    int numTotalMatches = tree.countMatches( lowerSearch );
        
    
    int numAfterSkip = numTotalMatches - inNumToSkip;
    
    int numToGet = inNumToGet;
    if( numToGet > numAfterSkip ) {
        numToGet = numAfterSkip;
        }
    
    *outNumRemaining = numAfterSkip - numToGet;
        
    ObjectRecord **results = new ObjectRecord*[ numToGet ];
    
    
    *outNumResults = 
        tree.getMatches( lowerSearch, inNumToSkip, numToGet, (void**)results );
    
    delete [] lowerSearch;

    return results;
    }



int addObject( const char *inDescription,
               char inContainable,
               char inPermanent,
               int inHeatValue,
               float inRValue,
               char inPerson,
               int inNumSlots, doublePair *inSlotPos,
               int inNumSprites, int *inSprites, 
               doublePair *inSpritePos,
               int inReplaceID ) {
    

    
    int newID = inReplaceID;


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
        
        if( newID == -1 ) {
            newID = nextObjectNumber;
            }
        
        char *fileName = autoSprintf( "%d.txt", newID );


        File *objectFile = objectsDir.getChildFile( fileName );
        

        SimpleVector<char*> lines;
        
        lines.push_back( autoSprintf( "id=%d", newID ) );
        lines.push_back( stringDuplicate( inDescription ) );

        lines.push_back( autoSprintf( "containable=%d", (int)inContainable ) );
        lines.push_back( autoSprintf( "permanent=%d", (int)inPermanent ) );
        
        lines.push_back( autoSprintf( "heatValue=%d", inHeatValue ) );
        lines.push_back( autoSprintf( "rValue=%f", inRValue ) );

        lines.push_back( autoSprintf( "person=%d", (int)inPerson ) );
        
        lines.push_back( autoSprintf( "numSlots=%d", inNumSlots ) );

        for( int i=0; i<inNumSlots; i++ ) {
            lines.push_back( autoSprintf( "slotPos=%f,%f", 
                                          inSlotPos[i].x,
                                          inSlotPos[i].y ) );
            }

        
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
        
        if( inReplaceID == -1 ) {
            nextObjectNumber++;
            }

        
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

    r->containable = inContainable;
    r->permanent = inPermanent;
    
    r->heatValue = inHeatValue;
    r->rValue = inRValue;

    r->person = inPerson;

    r->numSlots = inNumSlots;
    
    r->slotPos = new doublePair[ inNumSlots ];
    
    memcpy( r->slotPos, inSlotPos, inNumSlots * sizeof( doublePair ) );
    

    r->numSprites = inNumSprites;
    
    r->sprites = new int[ inNumSprites ];
    r->spritePos = new doublePair[ inNumSprites ];
    
    memcpy( r->sprites, inSprites, inNumSprites * sizeof( int ) );
    memcpy( r->spritePos, inSpritePos, inNumSprites * sizeof( doublePair ) );
    


    // delete old
    freeObjectRecord( newID );
    
    idMap[newID] = r;
    
    char *lower = stringToLowerCase( inDescription );
    
    tree.insert( lower, idMap[newID] );

    delete [] lower;
    
    return newID;
    }




void drawObject( ObjectRecord *inObject, doublePair inPos,
                 char inFlipH, double inAge ) {
    for( int i=0; i<inObject->numSprites; i++ ) {
        doublePair spritePos = inObject->spritePos[i];

        if( inFlipH ) {
            spritePos.x *= -1;
            }

        doublePair pos = add( spritePos, inPos );
        
        if( inObject->person && i == inObject->numSprites - 1 ) {
            pos = add( pos, getAgeHeadOffset( inAge, spritePos ) );
            }
        
        drawSprite( getSprite( inObject->sprites[i] ), pos, 1.0, 0, inFlipH );
        }    
    }



void drawObject( ObjectRecord *inObject, doublePair inPos,
                 char inFlipH, double inAge,
                 int inNumContained, int *inContainedIDs ) {

    int numSlots = getNumContainerSlots( inObject->id );
    
    if( inNumContained > numSlots ) {
        inNumContained = numSlots;
        }
    
    for( int i=0; i<inNumContained; i++ ) {
        doublePair slotPos = inObject->slotPos[i];

        if( inFlipH ) {
            slotPos.x *= -1;
            }

        doublePair pos = add( slotPos, inPos );
        drawObject( getObject( inContainedIDs[i] ), pos, inFlipH, inAge );
        }
    
    drawObject( inObject, inPos, inFlipH, inAge );
    }





void deleteObjectFromBank( int inID ) {
    
    File objectsDir( NULL, "objects" );
    
    
    if( objectsDir.exists() && objectsDir.isDirectory() ) {                
                    
        char *fileName = autoSprintf( "%d.txt", inID );
        
        File *objectFile = objectsDir.getChildFile( fileName );
            
        objectFile->remove();
        
        delete [] fileName;
        delete objectFile;
        }

    freeObjectRecord( inID );
    }



char isSpriteUsed( int inSpriteID ) {


    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            for( int s=0; s<idMap[i]->numSprites; s++ ) {
                if( idMap[i]->sprites[s] == inSpriteID ) {
                    return true;
                    }
                }
            }
        }
    return false;
    }



