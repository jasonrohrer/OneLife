
#include "categoryBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/util/random/JenkinsRandomSource.h"

#include "minorGems/io/file/File.h"

#include "folderCache.h"


static JenkinsRandomSource randSource;


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
            
    if( strstr( txtFileName, ".txt" ) != NULL ) {
                            
        // a category txt file!
                    
        char *categoryText = getFileContents( cache, i );
        
        if( categoryText != NULL ) {
            int numLines;
                        
            char **lines = split( categoryText, "\n", &numLines );
                        
            delete [] categoryText;

            if( numLines >= 2 ) {
                CategoryRecord *r = new CategoryRecord;
                            
                int next = 0;
                
                r->parentID = 0;
                sscanf( lines[next], "parentID=%d", 
                        &( r->parentID ) );
                
                if( r->parentID > maxID ) {
                    maxID = r->parentID;
                    }
                
                next++;
                
                r->isPattern = false;
                r->isProbabilitySet = false;
                
                if( strstr( lines[next], "pattern" ) != NULL ) {
                    r->isPattern = true;
                    next++;
                    }
                else if( strstr( lines[next], "probSet" ) != NULL ) {
                    r->isProbabilitySet = true;
                    next++;
                    }
                

                int numObjects = 0;
                sscanf( lines[next], "numObjects=%d", 
                        &( numObjects ) );
                            
                next++;

                for( int i=0; i<numObjects; i++ ) {
                    int objID = 0;
                    float prob = 0.0f;
                    
                    if( r->isProbabilitySet ) {
                        sscanf( lines[next], "%d %f", &objID, &prob );
                        }
                    else {
                        sscanf( lines[next], "%d", &objID );
                        }
                    
                    next++;
                    
                    if( objID > 0 ) {
                        if( objID > maxObjectID ) {
                            maxObjectID = objID;
                            }
                        r->objectIDSet.push_back( objID );
                        r->objectWeights.push_back( prob );
                        }
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
        
        idMap[ r->parentID ] = r;

        for( int j=0; j<r->objectIDSet.size(); j++ ) {
            int objID = r->objectIDSet.getElementDirect( j );
            
            ReverseCategoryRecord *rr = reverseMap[ objID ];
            
            if( rr == NULL ) {
                rr = new ReverseCategoryRecord;
                rr->childID = objID;
                
                reverseMap[ objID ] = rr;
                }
            
            rr->categoryIDSet.push_back( r->parentID );
            }
        
        }
    

    
    printf( "Loaded %d categories from categories folder\n", numRecords );
    }




static void freeCategoryRecord( int inParentID ) {
    if( inParentID < mapSize ) {
        if( idMap[inParentID] != NULL ) {
            
            CategoryRecord *r = idMap[inParentID];

            for( int i=0; i< r->objectIDSet.size(); i++ ) {
                
                reverseMap[ r->objectIDSet.getElementDirect( i ) ]->
                    categoryIDSet.deleteElementEqualTo( inParentID );
                }
            

            delete idMap[inParentID];
            idMap[inParentID] = NULL;
            }
        }    
    }




void freeCategoryBank() {
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
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








CategoryRecord *getCategory( int inParentID ) {
    if( inParentID < mapSize ) {
        return idMap[inParentID];
        }
    return NULL;
    }    



ReverseCategoryRecord *getReverseCategory( int inChildID ) {
    if( inChildID < reverseMapSize ) {
        return reverseMap[ inChildID ];
        }
    return NULL;
    }



void saveCategoryToDisk( int inParentID ) {
    CategoryRecord *r = getCategory( inParentID );
    

    if( r == NULL ) {
        return;
        }
    
    File categoriesDir( NULL, "categories" );
            
    if( !categoriesDir.exists() ) {
        categoriesDir.makeDirectory();
        }
    
    if( ! categoriesDir.exists() || ! categoriesDir.isDirectory() ) {

        printf( "Failed to make categories directory\n" );
        
        return;
        }


    
    File *cacheFile = categoriesDir.getChildFile( "cache.fcz" );

    cacheFile->remove();
    
    delete cacheFile;


        
    char *fileName = autoSprintf( "%d.txt", inParentID );


    File *categoryFile = categoriesDir.getChildFile( fileName );

    if( r->objectIDSet.size() == 0 ) {
        // empty category, simply remove it
        
        categoryFile->remove();
        }
    else {
        // resave
    
        SimpleVector<char*> lines;
        
        lines.push_back( autoSprintf( "parentID=%d", inParentID ) );

        if( r->isPattern ) {
            lines.push_back( stringDuplicate( "pattern" ) );
            }
        else if( r->isProbabilitySet ) {
            lines.push_back( stringDuplicate( "probSet" ) );
            }
        
        // start with 0 objects in a new category
        lines.push_back( autoSprintf( "numObjects=%d", 
                                      r->objectIDSet.size() ) );
        
        for( int i=0; i<r->objectIDSet.size(); i++ ) {
            if( r->isProbabilitySet ) {
                lines.push_back( 
                    autoSprintf( "%d %f", 
                                 r->objectIDSet.getElementDirect(i),
                                 r->objectWeights.getElementDirect(i) ) );
                }
            else {
                lines.push_back( 
                    autoSprintf( "%d", r->objectIDSet.getElementDirect(i) ) );
                }
            }
        
        
        char **linesArray = lines.getElementArray();
        
        
        char *contents = join( linesArray, lines.size(), "\n" );
        
        categoryFile->writeToFile( contents );
        
        delete [] contents;

        delete [] linesArray;
        lines.deallocateStringElements();
        }    
    
        
            
    delete [] fileName;
    delete categoryFile;
    
    return;
    }



static void autoAdjustWeights( int inParentID, int inHoldIndex = -1 ) {
    
    CategoryRecord *r = getCategory( inParentID );
    if( r != NULL && r->isProbabilitySet ) {        
        
        float weightSum = 0;
        
        for( int i=0; i< r->objectWeights.size(); i++ ) {
            
            weightSum += r->objectWeights.getElementDirect( i );
            }

        int nextIndex = 0;
        while( weightSum > 1 ) {
            float extra = weightSum - 1;

            if( nextIndex == inHoldIndex ) {
                nextIndex++;
                }

            if( nextIndex >= r->objectWeights.size() ) {
                break;
                }
            
            float weight = r->objectWeights.getElementDirect( nextIndex );
            
            if( weight > extra ) {
                weight -= extra;
                weightSum -= extra;
                }
            else {
                weightSum -= weight;
                weight = 0;
                }
            
            *( r->objectWeights.getElement( nextIndex ) ) = weight;

            nextIndex ++;
            
            if( nextIndex >= r->objectWeights.size() ) {
                break;
                }
            }
        
        nextIndex = 0;
        while( weightSum < 1 ) {
            float extra = 1 - weightSum;

            if( nextIndex == inHoldIndex ) {
                nextIndex++;
                }
            if( nextIndex >= r->objectWeights.size() ) {
                break;
                }
            
            float weight = r->objectWeights.getElementDirect( nextIndex );
            
            if( weight + extra <= 1 ) {
                weight += extra;
                weightSum += extra;
                }
            // else weird case that should never happen
            
            *( r->objectWeights.getElement( nextIndex ) ) = weight;

            nextIndex ++;
            
            if( nextIndex >= r->objectWeights.size() ) {
                break;
                }
            }
        }    
    }




static void addCategory( int inParentID ) {
    
    if( getCategory( inParentID ) != NULL ) {
        // already exists
        return;
        }
    

    // add it to file structure
    File categoriesDir( NULL, "categories" );
            
    if( !categoriesDir.exists() ) {
        categoriesDir.makeDirectory();
        }
    
    if( ! categoriesDir.exists() || ! categoriesDir.isDirectory() ) {        

        printf( "Failed to make categories directory\n" );
        
        return;
        }
    
    
    // now add it to live, in memory database
    if( inParentID >= mapSize ) {
        // expand map

        int newMapSize = inParentID + 1;
        
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
    
    r->parentID = inParentID;
    r->isPattern = false;
    r->isProbabilitySet = false;
    
    idMap[ inParentID ] = r;
    
    // don't save to disk now, because it's empty
    }







void deleteCategoryFromBank( int inParentID ) {
    
    File categoriesDir( NULL, "categories" );
    
    
    if( categoriesDir.exists() && categoriesDir.isDirectory() ) {

        File *cacheFile = categoriesDir.getChildFile( "cache.fcz" );

        cacheFile->remove();
        
        delete cacheFile;


        char *fileName = autoSprintf( "%d.txt", inParentID );
        
        File *categoryFile = categoriesDir.getChildFile( fileName );
            
        categoryFile->remove();
        
        delete [] fileName;
        delete categoryFile;
        }

    freeCategoryRecord( inParentID );
    }




void addCategoryToObject( int inObjectID, int inParentID ) {

    if( getCategory( inParentID ) == NULL ) {
        addCategory( inParentID );
        }

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
    
    
    CategoryRecord *r = getCategory( inParentID );
    
    if( r != NULL ) {

        for( int i=0; i< r->objectIDSet.size(); i++ ) {
            if( r->objectIDSet.getElementDirect( i ) == inObjectID ) {
                // already there
                return;
                }
            }

        r->objectIDSet.push_back( inObjectID );
        r->objectWeights.push_back( 0.0f );
        autoAdjustWeights( inParentID );
        
        ReverseCategoryRecord *rr = getReverseCategory( inObjectID );

        
        if( rr == NULL ) {
            rr = new ReverseCategoryRecord;

            rr->childID = inObjectID;
            
            reverseMap[ inObjectID ] = rr;        
            }
        
        rr->categoryIDSet.push_back( inParentID );

        saveCategoryToDisk( inParentID );
        }
    
    }




void setCategoryIsPattern( int inParentID, char inIsPattern ) {
    CategoryRecord *r = getCategory( inParentID );
    
    if( r != NULL ) {
        r->isPattern = inIsPattern;
        if( r->isPattern ) {
            r->isProbabilitySet = false;
            // zero all weights
            for( int i=0; i< r->objectWeights.size(); i++ ) {
                *( r->objectWeights.getElement( i ) ) = 0;
                }
            }
        saveCategoryToDisk( inParentID );
        }
    }



void setCategoryIsProbabilitySet( int inParentID, char inIsProbabilitySet ) {
    CategoryRecord *r = getCategory( inParentID );
    
    if( r != NULL ) {
        char oldVal = r->isProbabilitySet;
        r->isProbabilitySet = inIsProbabilitySet;
        if( r->isProbabilitySet ) {
            r->isPattern = false;
            if( !oldVal ) {
                // all zero weights, fix it
                if( r->objectWeights.size() > 0 ) {
                    *( r->objectWeights.getElement( 0 ) ) = 1;
                    }
                }
            }
        else {
            // zero all weights
            for( int i=0; i< r->objectWeights.size(); i++ ) {
                *( r->objectWeights.getElement( i ) ) = 0;
                }
            }
        saveCategoryToDisk( inParentID );
        }
    }




void removeCategoryFromObject( int inObjectID, int inParentID ) {
    CategoryRecord *r = getCategory( inParentID );
    
    if( r != NULL ) {

        int index = r->objectIDSet.getElementIndex( inObjectID );
        
        if( index != -1 ) {
            r->objectIDSet.deleteElement( index );
            r->objectWeights.deleteElement( index );
            }

        autoAdjustWeights( inParentID );
        

        ReverseCategoryRecord *rr = getReverseCategory( inObjectID );
        
        if( rr != NULL ) {    
            rr->categoryIDSet.deleteElementEqualTo( inParentID );
            }
            
        saveCategoryToDisk( inParentID );
        }

    }




void removeObjectFromAllCategories( int inObjectID ) {
    ReverseCategoryRecord *rr = getReverseCategory( inObjectID );
        
    if( rr != NULL ) {    
            
        for( int i=0; i< rr->categoryIDSet.size(); i++ ) {
                
            int cID = rr->categoryIDSet.getElementDirect( i );
            
            CategoryRecord *r = getCategory( cID );
                
            if( r != NULL ) {
                int index = r->objectIDSet.getElementIndex( inObjectID );
                
                if( index != -1 ) {
                    r->objectIDSet.deleteElement( index );
                    r->objectWeights.deleteElement( index );
                    }
                
                saveCategoryToDisk( cID );
                }
            }
            
        delete rr;
            
        reverseMap[ inObjectID ] = NULL;
        }
    }



// NOTE:
// implementation not functional, because reverse records not stored on 
// disk currently
void moveCategoryUp( int inObjectID, int inParentID ) {
    ReverseCategoryRecord *rr = getReverseCategory( inObjectID );
        
    if( rr != NULL ) {    
        int index = rr->categoryIDSet.getElementIndex( inParentID );
    
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



// NOTE:
// implementation not functional, because reverse records not stored on 
// disk currently
void moveCategoryDown( int inObjectID, int inParentID ) {
    ReverseCategoryRecord *rr = getReverseCategory( inObjectID );
        
    if( rr != NULL ) {    
        int index = rr->categoryIDSet.getElementIndex( inParentID );
    
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





void moveCategoryMemberUp( int inParentID, int inObjectID ) {

    CategoryRecord *r = getCategory( inParentID );
    
    if( r != NULL ) {        
        int index = r->objectIDSet.getElementIndex( inObjectID );
        
        if( index != -1 && index != 0 ) {
            
            int *id = r->objectIDSet.getElement( index );
            int *idToSwapWith = r->objectIDSet.getElement( index - 1 );

            int temp = *idToSwapWith;

            *idToSwapWith = *id;
            *id = temp;
            
            float *prob = r->objectWeights.getElement( index );
            float *probToSwapWith = r->objectWeights.getElement( index - 1 );

            float tempProb = *probToSwapWith;            

            *probToSwapWith = *prob;
            *prob = tempProb;

            saveCategoryToDisk( inParentID );
            }
        }
    }




void moveCategoryMemberDown( int inParentID, int inObjectID ) {

    CategoryRecord *r = getCategory( inParentID );
    
    if( r != NULL ) {        
        int index = r->objectIDSet.getElementIndex( inObjectID );
        
        if( index != -1 && 
            index != r->objectIDSet.size() - 1 ) {
            
            int *id = r->objectIDSet.getElement( index );
            int *idToSwapWith = r->objectIDSet.getElement( index + 1 );
            
            int temp = *idToSwapWith;            

            *idToSwapWith = *id;
            *id = temp;

            float *prob = r->objectWeights.getElement( index );
            float *probToSwapWith = r->objectWeights.getElement( index + 1 );

            float tempProb = *probToSwapWith;            

            *probToSwapWith = *prob;
            *prob = tempProb;

            saveCategoryToDisk( inParentID );
            }
        }
    }



void setMemberWeight( int inParentID, int inObjectID, float inWeight ) {

    CategoryRecord *r = getCategory( inParentID );
    if( r != NULL && r->isProbabilitySet ) {        
        int index = r->objectIDSet.getElementIndex( inObjectID );
        
        if( index != -1 ) {
            
            *( r->objectWeights.getElement( index ) ) = inWeight;
            autoAdjustWeights( inParentID, index );
            }
        saveCategoryToDisk( inParentID );
        }
    }



void makeWeightUniform( int inParentID ) {
    CategoryRecord *r = getCategory( inParentID );
    if( r != NULL && r->isProbabilitySet ) {        
        
        int num = r->objectWeights.size();
        
        float uniformWeight = 1.0f / num;
        
        for( int i=0; i<num; i++ ) {
            *( r->objectWeights.getElement( i ) ) = uniformWeight;
            }
        saveCategoryToDisk( inParentID );
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



char isObjectUsedInCategories( int inObjectID ) {
    CategoryRecord *r = getCategory( inObjectID );
    
    if( r != NULL && r->objectIDSet.size() > 0 ) {
        return true;
        }
    
    int num = getNumCategoriesForObject( inObjectID );
    

    return ( num > 0 );
    }





int pickFromProbSet( int inParentID ) {
    CategoryRecord *r = getCategory( inParentID );

    float pick = randSource.getRandomFloat();
    
    float weightSum = 0;
    
    for( int i=0; i<  r->objectIDSet.size(); i++ ) {
        weightSum += r->objectWeights.getElementDirect( i );
        
        if( weightSum >= pick ) {
            return r->objectIDSet.getElementDirect( i );
            }
        }
    
    // weights didn't sum to 1?
    return inParentID;
    }



char isProbabilitySet( int inParentID ) {
    CategoryRecord *r = getCategory( inParentID );
    if( r != NULL && r->isProbabilitySet ) { 
        return true;
        }
    return false;
    }

