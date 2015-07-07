#include "transitionBank.h"


#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"



// track pointers to all records
static SimpleVector<TransRecord *> records;


// maps for faster-than-linear access

// both maps are the same size
static int mapSize;

// maps IDs to vectors of record pointers for records that use the id
// sparse, so some entries are empty vectors
static SimpleVector<TransRecord *> *usesMap;

// same, but for each ID, find records that produce it
static SimpleVector<TransRecord *> *producesMap;



void initTransBank() {
    
    int maxID = 0;
    
    File transDir( NULL, "transitions" );
                
    if( transDir.exists() && transDir.isDirectory() ) {
        int numFiles;
        File **childFiles = transDir.getChildFiles( &numFiles );

        for( int i=0; i<numFiles; i++ ) {
            
            if( ! childFiles[i]->isDirectory() ) {
                                        
                char *txtFileName = childFiles[i]->getFileName();
                        
                if( strstr( txtFileName, ".txt" ) != NULL ) {
                    
                    int actor = -1;
                    int target = -1;
                    
                    sscanf( txtFileName, "%d_%d.txt", &actor, &target );
                    
                    if(  target != -1 ) {
                        char *contents = childFiles[i]->readFileContents();
                        
                        if( contents != NULL ) {
                            
                            int newActor = -1;
                            int newTarget = -1;

                            sscanf( contents, "%d %d", &newActor, &newTarget );
                            
                            TransRecord *r = new TransRecord;
                            
                            r->actor = actor;
                            r->target = target;
                            r->newActor = newActor;
                            r->newTarget = newTarget;

                            records.push_back( r );
                            
                            if( actor > maxID ) {
                                maxID = actor;
                                }
                            if( target > maxID ) {
                                maxID = target;
                                }
                            if( newActor > maxID ) {
                                maxID = newActor;
                                }
                            if( newTarget > maxID ) {
                                maxID = newTarget;
                                }

                            delete [] contents;
                            }
                        }
                    }
                delete [] txtFileName;
                }
            delete childFiles[i];
            }
        delete [] childFiles;
        }
    

    mapSize = maxID + 1;
    

    usesMap = new SimpleVector<TransRecord *>[ mapSize ];
        
    producesMap = new SimpleVector<TransRecord *>[ mapSize ];
    
    
    int numRecords = records.size();
    
    for( int i=0; i<numRecords; i++ ) {
        TransRecord *t = records.getElementDirect( i );
        
        
        if( t->actor != -1 ) {
            usesMap[t->actor].push_back( t );
            }
        
        // no duplicate records
        if( t->target != t->actor ) {    
            usesMap[t->target].push_back( t );
            }
        
        if( t->newActor != -1 ) {
            producesMap[t->newActor].push_back( t );
            }
        
        // no duplicate records
        if( t->newTarget != -1 && t->newTarget != t->newActor ) {    
            producesMap[t->newTarget].push_back( t );
            }
        }
    
    }



void freeTransBank() {
    for( int i=0; i<records.size(); i++ ) {
        delete records.getElementDirect(i);
        }
    
    records.deleteAll();
    
    delete [] usesMap;
    delete [] producesMap;
    }




TransRecord *getTrans( int inActor, int inTarget ) {

    int numRecords = usesMap[inTarget].size();
    
    for( int i=0; i<numRecords; i++ ) {
        
        TransRecord *r = usesMap[inTarget].getElementDirect(i);
        
        if( r->actor == inActor && r->target == inTarget ) {
            return r;
            }
        }
    
    return NULL;
    }


static TransRecord **search( SimpleVector<TransRecord *> inMapToSearch[],
                             int inID, 
                             int inNumToSkip, 
                             int inNumToGet, 
                             int *outNumResults, int *outNumRemaining ) {
    
    if( inID >= mapSize ) {
        // no transition for an object with this ID range
        return NULL;
        }


    int numRecords = inMapToSearch[inID].size();
    
    int numLeft = numRecords - inNumToSkip;
    

    if( numLeft < inNumToGet ) {
        inNumToGet = numLeft;
        }

    if( inNumToGet <= 0 ) {
        *outNumResults = 0;
        *outNumRemaining = 0;
        
        return NULL;
        }
    
    
    *outNumResults = inNumToGet;
    

    *outNumRemaining = numLeft - inNumToGet;
    
    
    TransRecord **records = new TransRecord*[inNumToGet];
    

    int r = 0;
    for( int i=inNumToSkip; i<inNumToSkip + inNumToGet; i++ ) {
        records[r] = inMapToSearch[inID].getElementDirect(i);
        r++;
        }
    
    return records;
    }




TransRecord **searchUses( int inUsesID, 
                          int inNumToSkip, 
                          int inNumToGet, 
                          int *outNumResults, int *outNumRemaining ) {
    
    return search( usesMap, inUsesID, inNumToSkip, inNumToGet,
                   outNumResults, outNumRemaining );
    }



TransRecord **searchProduces( int inProducesID, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining ) {
    
    return search( producesMap, inProducesID, inNumToSkip, inNumToGet,
                   outNumResults, outNumRemaining );
    }





void addTrans( int inActor, int inTarget,
               int inNewActor, int inNewTarget ) {

    // exapand id-indexed maps if a bigger ID is being added    
    if( inActor >= mapSize || inTarget >= mapSize 
        ||
        inNewActor >= mapSize || inNewTarget >= mapSize ) {

        int max = inActor;
        if( inTarget >= max ) {
            max = inTarget;
            }
        if( inNewActor >= max ) {
            max = inNewActor;
            }
        if( inNewTarget >= max ) {
            max = inNewTarget;
            }
        
        int newMapSize = max + 1;
        
        SimpleVector<TransRecord *> *newUsesMap =
            new SimpleVector<TransRecord *>[ newMapSize ];
        
        SimpleVector<TransRecord *> *newProducesMap =
            new SimpleVector<TransRecord *>[ newMapSize ];

        
        // invoke copy constructors (don't use memcpy)
        for( int i=0; i<mapSize; i++ ) {
            newUsesMap[i] = usesMap[i];
            newProducesMap[i] = producesMap[i];
            }
        
        delete [] usesMap;
        delete [] producesMap;
        
        usesMap = newUsesMap;
        producesMap = newProducesMap;
        
        mapSize = newMapSize;
        }

    

    // one exists?
    TransRecord *t = getTrans( inActor, inTarget );
    
    char writeToFile = false;
    

    if( t == NULL ) {
        t = new TransRecord;

        t->actor = inActor;
        t->target = inTarget;
        t->newActor = inNewActor;
        t->newTarget = inNewTarget;

        records.push_back( t );

        if( inActor != -1 ) {
            usesMap[inActor].push_back( t );
            }
        
        // no duplicate records when actor and target are the same
        if( inTarget != inActor ) {    
            usesMap[inTarget].push_back( t );
            }
        
        if( inNewActor != -1 ) {
            producesMap[inNewActor].push_back( t );
            }
        
        // avoid duplicate records here too
        if( inNewTarget != -1 && inNewTarget != inNewActor ) {    
            producesMap[inNewTarget].push_back( t );
            }
        
        writeToFile = true;
        }
    else {
        // update it
        
        // "records" already contains it, no change
        // usesMap already contains it, no change

        if( t->newTarget == inNewTarget &&
            t->newActor == inNewActor ) {
            
            // no change to produces map either... 

            // don't need to change record at all, done.
            }
        else {
            
            // remove record from producesMaps
            
            if( t->newActor != -1 ) {
                producesMap[t->newTarget].deleteElementEqualTo( t );
                }
            if( t->newTarget != -1 ) {                
                producesMap[t->newTarget].deleteElementEqualTo( t );
                }
            

            t->newActor = inNewActor;
            t->newTarget = inNewTarget;
            
            if( inNewActor != -1 ) {
                producesMap[inNewActor].push_back( t );
                }
            
            // no duplicate records
            if( inNewTarget != -1 && inNewTarget != inNewActor ) {    
                producesMap[inNewTarget].push_back( t );
                }
            
            writeToFile = true;
            }
        }
    
    
    if( writeToFile ) {
        
        File transDir( NULL, "transitions" );
            
        if( !transDir.exists() ) {
            transDir.makeDirectory();
            }
        
        if( transDir.exists() && transDir.isDirectory() ) {
        
            char *fileName = autoSprintf( "%d_%d.txt", inActor, inTarget );
            

            File *transFile = transDir.getChildFile( fileName );
            
            char *fileContents = autoSprintf( "%d %d", 
                                              inNewActor, inNewTarget );
            
            
            transFile->writeToFile( fileContents );
            
            delete [] fileContents;
            delete transFile;
            delete [] fileName;
            }
        }
    
    }

