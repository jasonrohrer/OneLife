
#include "categoryBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include "folderCache.h"



static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static CategoryRecord **idMap;


static int reverseMapSize;
// maps object IDs to reverse records
static ReverseCategoryRecord **reverseMap;


static StringTree tree;




static FolderCache cache;

static int currentFile;


static SimpleVector<CategoryRecord*> records;
static int maxID;

static int maxObjectID;



int initCategoryBankStart( char *outRebuildingCache ) {
    maxID = 0;
    maxObjectID = 0;
    
    currentFile = 0;
    

    cache = initFolderCache( "categories", outRebuildingCache );

    return cache.numFiles;
    }




float initCategoryBankStep() {
        
    if( currentFile == cache.numFiles ) {
        return 1.0;
        }
    
    int i = currentFile;

                
    char *txtFileName = getFileName( cache, i );
            
    if( strstr( txtFileName, ".txt" ) != NULL &&
        strcmp( txtFileName, "nextCategoryNumber.txt" ) != 0 ) {
                            
        // an category txt file!
                    
        char *categoryText = getFileContents( cache, i );
        
        if( categoryText != NULL ) {
            int numLines;
                        
            char **lines = split( categoryText, "\n", &numLines );
                        
            delete [] categoryText;

            if( numLines >= 3 ) {
                CategoryRecord *r = new CategoryRecord;
                            
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
                            
                int numObjects = 0;
                sscanf( lines[next], "numObjects=%d", 
                        &( numObjects ) );
                            
                next++;

                for( int i=0; i<numObjects; i++ ) {
                    int objID = 0;
                    
                    sscanf( lines[next], "%d", 
                            &( objID ) );
                    
                    next++;
                    
                    if( objID > 0 ) {
                        if( objID > maxObjectID ) {
                            maxObjectID++;
                            }
                        r->objectIDSet.push_back( objID );
                        }
                    }    
                }
                            
            for( int i=0; i<numLines; i++ ) {
                delete [] lines[i];
                }
            delete [] lines;
            }
        }
                
    delete [] txtFileName;


    currentFile ++;
    return (float)( currentFile ) / (float)( cache.numFiles );
    }
    
    

void initCategoryBankFinish() {
  
    freeFolderCache( cache );
    
    mapSize = maxID + 1;
    
    idMap = new CategoryRecord*[ mapSize ];
    
    for( int i=0; i<mapSize; i++ ) {
        idMap[i] = NULL;
        }


    reverseMapSize = maxObjectID + 1;
    
    reverseMap = new ReverseCategoryRecord*[reverseMapSize];
    
    for( int i=0; i<reverseMapSize; i++ ) {
        reverseMap[i] = NULL;
        }


    int numRecords = records.size();
    for( int i=0; i<numRecords; i++ ) {
        CategoryRecord *r = records.getElementDirect(i);
        
        idMap[ r->id ] = r;
        
        char *lowercase = stringToLowerCase( r->description );
        
        tree.insert( lowercase, r );

        delete [] lowercase;

        for( int j=0; j<r->objectIDSet.size(); j++ ) {
            int objID = r->objectIDSet.getElementDirect( j );
            
            ReverseCategoryRecord *rr = reverseMap[ objID ];
            
            if( rr == NULL ) {
                rr = new ReverseCategoryRecord;
                rr->objectID = objID;
                
                reverseMap[ objID ] = rr;
                }
            
            rr->categoryIDSet.push_back( r->id );
            }
        
        }
    

    
    printf( "Loaded %d categories from categories folder\n", numRecords );
    }




static void freeCategoryRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            
            char *lower = stringToLowerCase( idMap[inID]->description );
            
            tree.remove( lower, idMap[inID] );
            
            delete [] lower;

            delete [] idMap[inID]->description;
            
            CategoryRecord *r = idMap[inID];

            for( int i=0; i< r->objectIDSet.size(); i++ ) {
                
                reverseMap[ r->objectIDSet.getElementDirect( i ) ]->
                    categoryIDSet.deleteElementEqualTo( inID );
                }
            

            delete idMap[inID];
            idMap[inID] = NULL;
            }
        }    
    }




void freeCategoryBank() {
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            delete [] idMap[i]->description;
            delete idMap[i];
            }
        }

    delete [] idMap;

    for( int i=0; i<reverseMapSize; i++ ) {
        if( reverseMap[i] != NULL ) {
            
            delete reverseMap[i];
            }
        }

    delete [] reverseMap;
    }








CategoryRecord *getCategory( int inID ) {
    if( inID < mapSize ) {
        return idMap[inID];
        }
    return NULL;
    }    



ReverseCategoryRecord *getReverseCategory( int inObjecID ) {
    if( inObjecID < reverseMapSize ) {
        return reverseMap[ inObjecID ];
        }
    return NULL;
    }




// return array destroyed by caller, NULL if none found
CategoryRecord **searchCategories( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining ) {
    
    if( strcmp( inSearch, "" ) == 0 ) {
        // special case, show categories in reverse-id order, newest first
        SimpleVector< CategoryRecord *> results;
        
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
    

    char *lowerSearch = stringToLowerCase( inSearch );

    int numTotalMatches = tree.countMatches( lowerSearch );
        
    
    int numAfterSkip = numTotalMatches - inNumToSkip;
    
    int numToGet = inNumToGet;
    if( numToGet > numAfterSkip ) {
        numToGet = numAfterSkip;
        }
    
    *outNumRemaining = numAfterSkip - numToGet;
        
    CategoryRecord **results = new CategoryRecord*[ numToGet ];
    
    
    *outNumResults = 
        tree.getMatches( lowerSearch, inNumToSkip, numToGet, (void**)results );
    
    delete [] lowerSearch;

    return results;
    }



void saveCategoryToDisk( int inCategoryID ) {
    CategoryRecord *r = getCategory( inCategoryID );
    

    if( r == NULL ) {
        return;
        }
    
    File categoriesDir( NULL, "categories" );
            
    if( !categoriesDir.exists() ) {
        categoriesDir.makeDirectory();
        }
    
    if( ! categoriesDir.exists() || ! categoriesDir.isDirectory() ) {
        return;
        }
    
        
    char *fileName = autoSprintf( "%d.txt", inCategoryID );


    File *categoryFile = categoriesDir.getChildFile( fileName );
        
    
    SimpleVector<char*> lines;
    
    lines.push_back( autoSprintf( "id=%d", inCategoryID ) );
    lines.push_back( stringDuplicate( r->description ) );

    // start with 0 objects in a new category
    lines.push_back( autoSprintf( "numObjects=%d", r->objectIDSet.size() ) );

    for( int i=0; i<r->objectIDSet.size(); i++ ) {
        lines.push_back( 
            autoSprintf( "%d", r->objectIDSet.getElementDirect(i) ) );
        }
        
    char **linesArray = lines.getElementArray();
        
        
    char *contents = join( linesArray, lines.size(), "\n" );
    
    delete [] linesArray;
    lines.deallocateStringElements();
    

    File *cacheFile = categoriesDir.getChildFile( "cache.fcz" );

    cacheFile->remove();
    
    delete cacheFile;
    
    
    categoryFile->writeToFile( contents );
    
    delete [] contents;
        
            
    delete [] fileName;
    delete categoryFile;
    
    return;
    }




int addCategory( const char *inDescription ) {
    
    int newID = -1;


    // add it to file structure
    File categoriesDir( NULL, "categories" );
            
    if( !categoriesDir.exists() ) {
        categoriesDir.makeDirectory();
        }
    
    if( categoriesDir.exists() && categoriesDir.isDirectory() ) {
                
        int nextCategoryNumber = 1;
                
        File *nextNumberFile = 
            categoriesDir.getChildFile( "nextCategoryNumber.txt" );
                
        if( nextNumberFile->exists() ) {
                    
            char *nextNumberString = 
                nextNumberFile->readFileContents();

            if( nextNumberString != NULL ) {
                sscanf( nextNumberString, "%d", &nextCategoryNumber );
                
                delete [] nextNumberString;
                }
            }
        
        if( newID == -1 ) {
            newID = nextCategoryNumber;
            }
        
        char *nextNumberString = autoSprintf( "%d", nextCategoryNumber );
        
        nextNumberFile->writeToFile( nextNumberString );
        
        delete [] nextNumberString;
                
        
        delete nextNumberFile;
        }
    
    if( newID == -1 ) {
        // failed to find spot to save it to disk
        return -1;
        }

    
    // now add it to live, in memory database
    if( newID >= mapSize ) {
        // expand map

        int newMapSize = newID + 1;
        
        CategoryRecord **newMap = new CategoryRecord*[newMapSize];
        
        for( int i=0; i<newMapSize; i++ ) {
            newMap[i] = NULL;
            }

        memcpy( newMap, idMap, sizeof(CategoryRecord*) * mapSize );

        delete [] idMap;
        idMap = newMap;
        mapSize = newMapSize;
        }

    CategoryRecord *r = new CategoryRecord;
    
    r->id = newID;
    r->description = stringDuplicate( inDescription );




    char *lower = stringToLowerCase( inDescription );    

    idMap[newID] = r;
    
    tree.insert( lower, idMap[newID] );

    delete [] lower;
    
    saveCategoryToDisk( newID );
    
    return newID;
    }







void deleteCategoryFromBank( int inID ) {
    
    File categoriesDir( NULL, "categories" );
    
    
    if( categoriesDir.exists() && categoriesDir.isDirectory() ) {

        File *cacheFile = categoriesDir.getChildFile( "cache.fcz" );

        cacheFile->remove();
        
        delete cacheFile;


        char *fileName = autoSprintf( "%d.txt", inID );
        
        File *categoryFile = categoriesDir.getChildFile( fileName );
            
        categoryFile->remove();
        
        delete [] fileName;
        delete categoryFile;
        }

    freeCategoryRecord( inID );
    }




void addCategoryToObject( int inObjectID, int inCategoryID ) {
    if( inObjectID >= reverseMapSize ) {
        // expand map

        int newMapSize = inObjectID + 1;
        
        ReverseCategoryRecord **newMap = new ReverseCategoryRecord*[newMapSize];
        
        for( int i=0; i<newMapSize; i++ ) {
            newMap[i] = NULL;
            }

        memcpy( newMap, reverseMap, 
                sizeof(ReverseCategoryRecord*) * reverseMapSize );

        delete [] reverseMap;
        reverseMap = newMap;
        reverseMapSize = newMapSize;
        }
    
    
    CategoryRecord *r = getCategory( inCategoryID );
    
    if( r != NULL ) {

        for( int i=0; i< r->objectIDSet.size(); i++ ) {
            if( r->objectIDSet.getElementDirect( i ) == inObjectID ) {
                // already there
                return;
                }
            }

        r->objectIDSet.push_back( inObjectID );
        
        ReverseCategoryRecord *rr = getReverseCategory( inObjectID );

        
        if( rr == NULL ) {
            rr = new ReverseCategoryRecord;

            rr->objectID = inObjectID;
            
            reverseMap[ inObjectID ] = rr;        
            }
        
        rr->categoryIDSet.push_back( inCategoryID );

        saveCategoryToDisk( inCategoryID );
        }
    
    }



void removeCategoryFromObject( int inObjectID, int inCategoryID ) {
    CategoryRecord *r = getCategory( inCategoryID );
    
    if( r != NULL ) {

        r->objectIDSet.deleteElementEqualTo( inObjectID );
        
        ReverseCategoryRecord *rr = getReverseCategory( inObjectID );
        
        if( rr != NULL ) {    
            rr->categoryIDSet.deleteElementEqualTo( inCategoryID );
            }
            
        saveCategoryToDisk( inCategoryID );
        }

    }




void removeObjectFromAllCategories( int inObjectID ) {
    ReverseCategoryRecord *rr = getReverseCategory( inObjectID );
        
    if( rr != NULL ) {    
            
        for( int i=0; i< rr->categoryIDSet.size(); i++ ) {
                
            int cID = rr->categoryIDSet.getElementDirect( i );
            
            CategoryRecord *r = getCategory( cID );
                
            if( r != NULL ) {
                r->objectIDSet.deleteElementEqualTo( inObjectID );
                
                saveCategoryToDisk( cID );
                }
            }
            
        delete rr;
            
        reverseMap[ inObjectID ] = NULL;
        }
    }



void moveCategoryUp( int inObjectID, int inCategoryID ) {
    ReverseCategoryRecord *rr = getReverseCategory( inObjectID );
        
    if( rr != NULL ) {    
        int index = rr->categoryIDSet.getElementIndex( inCategoryID );
    
        if( index > 0 ) {
            
            // swap up
            int newIndex = index - 1;
            
            int tempID = rr->categoryIDSet.getElementDirect( newIndex );
            
            *( rr->categoryIDSet.getElement( newIndex ) ) =
                rr->categoryIDSet.getElementDirect( index );
            
            *( rr->categoryIDSet.getElement( index ) ) = tempID;
            }
        }
    
    }



void moveCategoryDown( int inObjectID, int inCategoryID ) {
    ReverseCategoryRecord *rr = getReverseCategory( inObjectID );
        
    if( rr != NULL ) {    
        int index = rr->categoryIDSet.getElementIndex( inCategoryID );
    
        if( index != -1 && 
            index < rr->categoryIDSet.size() - 1 ) {
            
            // swap down
            int newIndex = index + 1;
            
            int tempID = rr->categoryIDSet.getElementDirect( newIndex );
            
            *( rr->categoryIDSet.getElement( newIndex ) ) =
                rr->categoryIDSet.getElementDirect( index );
            
            *( rr->categoryIDSet.getElement( index ) ) = tempID;
            }
        }
    
    }



int getNumCategoriesForObject( int inObjectID ) {
    ReverseCategoryRecord *rr = getReverseCategory( inObjectID );
        
    if( rr != NULL ) {    
        return rr->categoryIDSet.size();
        }

    return 0;
    }



int getCategoryForObject( int inObjectID, int inCategoryIndex ) {
    ReverseCategoryRecord *rr = getReverseCategory( inObjectID );
        
    if( rr != NULL ) {    
        if( inCategoryIndex < rr->categoryIDSet.size() ) {
            
            return rr->categoryIDSet.getElementDirect( inCategoryIndex );
            }
        }

    return -1;
    }




