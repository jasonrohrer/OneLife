
#include "overlayBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"



static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static OverlayRecord **idMap;


static StringTree tree;




static int numFiles;
static File **childFiles;

static int currentFile;


static SimpleVector<OverlayRecord*> records;
static int maxID;



void initOverlayBankStart() {
    maxID = 0;

    numFiles = 0;
    currentFile = 0;
    
    File overlaysDir( NULL, "overlays" );
    if( overlaysDir.exists() && overlaysDir.isDirectory() ) {

        childFiles = overlaysDir.getChildFiles( &numFiles );
        }
    }



float initOverlayBankStep() {

    if( currentFile == numFiles ) {
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
                            
                    printf( "Loading overlay from path %s, tag %s\n", 
                            fullName, tag );
                    if( strcmp( tag, "test3" ) == 0 ) {
                        printf( "HEY\n" );
                        }
                    SpriteHandle thumbnailSprite =
                        loadSpriteBase( fullName, false );
                            
                            
                    if( thumbnailSprite != NULL ) {
                        OverlayRecord *r = new OverlayRecord;

                        r->id = 0;
                                
                        sscanf( tgaFileName, "%d.tga", &( r->id ) );
                                
                        printf( "Scanned id = %d\n", r->id );
                                
                        r->thumbnailSprite = thumbnailSprite;
                        r->tag = stringDuplicate( tag );
                                

                        r->image = 
                            readTGAFileBase( fullName );

                        records.push_back( r );

                        if( maxID < r->id ) {
                            maxID = r->id;
                            }
                        }
                            
                    delete [] fullName;
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
        
 

void initOverlayBankFinish() {
     
    delete [] childFiles;
    
    mapSize = maxID + 1;
    
    idMap = new OverlayRecord*[ mapSize ];
    
    for( int i=0; i<mapSize; i++ ) {
        idMap[i] = NULL;
        }

    int numRecords = records.size();
    for( int i=0; i<numRecords; i++ ) {
        OverlayRecord *r = records.getElementDirect(i);
        
        idMap[ r->id ] = r;
        
        char *lower = stringToLowerCase( r->tag );
        
        tree.insert( lower, r );
        
        delete [] lower;
        }

    printf( "Loaded %d tagged overlays from overlays folder\n", numRecords );
    }




static void freeOverlayRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
         
            freeSprite( idMap[inID]->thumbnailSprite );
            
            delete idMap[inID]->image;

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




void freeOverlayBank() {
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            freeSprite( idMap[i]->thumbnailSprite );
            delete idMap[i]->image;

            delete [] idMap[i]->tag;

            delete idMap[i];
            }
        }

    delete [] idMap;
    }





OverlayRecord *getOverlay( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            return idMap[inID];
            }
        }
    return NULL;
    }

    


// return array destroyed by caller, NULL if none found
OverlayRecord **searchOverlays( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining ) {
    
    if( strcmp( inSearch, "" ) == 0 ) {
        // special case, show objects in reverse-id order, newest first
        SimpleVector< OverlayRecord *> results;
        
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
        
    OverlayRecord **results = new OverlayRecord*[ numToGet ];
    
    
    *outNumResults = 
        tree.getMatches( lower, inNumToSkip, numToGet, (void**)results );
    
    delete [] lower;

    return results;
    }



int addOverlay( const char *inTag,
               Image *inSourceImage ) {

    // first, make transparent areas white
    // the Import page expands image to a power of two, filling in the 
    // extra space with transparent black
    
    // but since overlays are used multiplicatively, ignoring alpha values,
    // we need to make this extra area white so that it's invisible

    Color transWhite( 1, 1, 1, 0 );
    
    int numP = inSourceImage->getHeight() * inSourceImage->getWidth();
    
    double *alpha = inSourceImage->getChannel( 3 );
    for( int i=0; i<numP; i++ ) {
        if( alpha[i] == 0 ) {
            inSourceImage->setColor( i, transWhite );
            }    
        }
    
    
    int newID = -1;


    // add it to file structure
    File overlaysDir( NULL, "overlays" );
            
    if( !overlaysDir.exists() ) {
        overlaysDir.makeDirectory();
        }
    
    if( overlaysDir.exists() && overlaysDir.isDirectory() ) {
                
                
        int nextOverlayNumber = 1;
                
        File *nextNumberFile = 
            overlaysDir.getChildFile( "nextOverlayNumber.txt" );
                
        if( nextNumberFile->exists() ) {
                    
            char *nextNumberString = 
                nextNumberFile->readFileContents();

            if( nextNumberString != NULL ) {
                sscanf( nextNumberString, "%d", &nextOverlayNumber );
                
                delete [] nextNumberString;
                }
            }
                
                    
        File *tagDir = overlaysDir.getChildFile( inTag );
        
        if( !tagDir->exists() ) {
            tagDir->makeDirectory();
            }
                
        
        if( tagDir->exists() && tagDir->isDirectory() ) {
            
            char *fileName = autoSprintf( "%d.tga", nextOverlayNumber );
            
            newID = nextOverlayNumber;

            File *overlayFile = tagDir->getChildFile( fileName );
            
            TGAImageConverter tga;
            
            FileOutputStream stream( overlayFile );
            
            tga.formatImage( inSourceImage, &stream );
                    
            delete [] fileName;
            delete overlayFile;

            nextOverlayNumber++;
            }

                
        char *nextNumberString = autoSprintf( "%d", nextOverlayNumber );
        
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
        

        
        OverlayRecord **newMap = new OverlayRecord*[newMapSize];
        
        for( int i=0; i<newMapSize; i++ ) {
            newMap[i] = NULL;
            }

        memcpy( newMap, idMap, sizeof(OverlayRecord*) * mapSize );

        delete [] idMap;
        idMap = newMap;
        mapSize = newMapSize;
        }

    OverlayRecord *r = new OverlayRecord;
    
    r->id = newID;
    r->thumbnailSprite = fillSprite( inSourceImage, false );
    r->image = inSourceImage;
    r->tag = stringDuplicate( inTag );
    

    // delete old
    freeOverlayRecord( newID );
    
    idMap[newID] = r;
    
    char *lower = stringToLowerCase( inTag );
    
    tree.insert( lower, idMap[newID] );

    delete [] lower;

    return newID;
    }



void deleteOverlayFromBank( int inID ) {
    OverlayRecord *r = idMap[ inID ];

    
    File overlaysDir( NULL, "overlays" );
    
    
    if( overlaysDir.exists() && overlaysDir.isDirectory() ) {                
                    
        File *tagDir = overlaysDir.getChildFile( r->tag );
        
        if( tagDir->exists() && tagDir->isDirectory() ) {
            
            char *fileName = autoSprintf( "%d.tga", inID );
            
            File *overlayFile = tagDir->getChildFile( fileName );
            
            overlayFile->remove();
            
            delete [] fileName;
            delete overlayFile;
            
            tagDir->remove();
            }
        delete tagDir;
        }
    
    
    freeOverlayRecord( inID );
    }



