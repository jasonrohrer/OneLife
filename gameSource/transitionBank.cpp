#include "transitionBank.h"


#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"


#include "folderCache.h"
#include "objectBank.h"
#include "categoryBank.h"



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




static FolderCache cache;

static int currentFile;

static int maxID;

static char autoGenerateCategoryTransitions = false;
static char autoGenerateUsedObjectTransitions = false;
static char autoGenerateGenericUseTransitions = false;


int initTransBankStart( char *outRebuildingCache,
                        char inAutoGenerateCategoryTransitions,
                        char inAutoGenerateUsedObjectTransitions,
                        char inAutoGenerateGenericUseTransitions ) {
    
    autoGenerateCategoryTransitions = inAutoGenerateCategoryTransitions;
    autoGenerateUsedObjectTransitions = inAutoGenerateUsedObjectTransitions;
    autoGenerateGenericUseTransitions = inAutoGenerateGenericUseTransitions;
    
    maxID = 0;

    currentFile = 0;


    cache = initFolderCache( "transitions", outRebuildingCache );

    return cache.numFiles;
    }




float initTransBankStep() {
        
    if( currentFile == cache.numFiles ) {
        return 1.0;
        }
    
    int i = currentFile;

    char *txtFileName = getFileName( cache, i );
                        
    if( strstr( txtFileName, ".txt" ) != NULL ) {
                    
        int actor = 0;
        int target = -2;
        char lastUse = false;
        
        if( strstr( txtFileName, "_L" ) != NULL ) {
            lastUse = true;
            
            sscanf( txtFileName, "%d_%d_L.txt", &actor, &target );
            }
        else {
            sscanf( txtFileName, "%d_%d.txt", &actor, &target );
            }
        
        if(  target != -2 ) {

            char *contents = getFileContents( cache, i );
                        
            if( contents != NULL ) {
                            
                int newActor = 0;
                int newTarget = 0;
                int autoDecaySeconds = 0;
                float actorMinUseFraction = 0.0f;
                float targetMinUseFraction = 0.0f;
                
                sscanf( contents, "%d %d %d %f %f", 
                        &newActor, &newTarget, &autoDecaySeconds,
                        &actorMinUseFraction, &targetMinUseFraction );
                            
                TransRecord *r = new TransRecord;
                            
                r->actor = actor;
                r->target = target;
                r->newActor = newActor;
                r->newTarget = newTarget;
                r->autoDecaySeconds = autoDecaySeconds;
                r->lastUse = lastUse;

                r->actorMinUseFraction = actorMinUseFraction;
                r->targetMinUseFraction = targetMinUseFraction;
                
                
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

    currentFile ++;
    return (float)( currentFile ) / (float)( cache.numFiles );
    }



void initTransBankFinish() {
    
    freeFolderCache( cache );
    
    mapSize = maxID + 1;
    

    usesMap = new SimpleVector<TransRecord *>[ mapSize ];
        
    producesMap = new SimpleVector<TransRecord *>[ mapSize ];
    
    
    int numRecords = records.size();
    
    for( int i=0; i<numRecords; i++ ) {
        TransRecord *t = records.getElementDirect( i );
        
        
        if( t->actor > 0 ) {
            usesMap[t->actor].push_back( t );
            }
        
        // no duplicate records
        if( t->target >= 0 && t->target != t->actor ) {    
            usesMap[t->target].push_back( t );
            }
        
        if( t->newActor != 0 ) {
            producesMap[t->newActor].push_back( t );
            }
        
        // no duplicate records
        if( t->newTarget != 0 && t->newTarget != t->newActor ) {    
            producesMap[t->newTarget].push_back( t );
            }
        }
    
    printf( "Loaded %d transitions from transitions folder\n", numRecords );

    if( autoGenerateCategoryTransitions ) {
        int numObjects;
        
        ObjectRecord **objects = getAllObjects( &numObjects );
        
        for( int i=0; i<numObjects; i++ ) {
            ObjectRecord *o = objects[i];
            
            int oID = o->id;
            
            int numCats = getNumCategoriesForObject( oID );
            
            for( int c=0; c<numCats; c++ ) {
                
                int parentID = getCategoryForObject( oID, c );
                
                SimpleVector<TransRecord*> *parentTrans = 
                    getAllUses( parentID );

                if( parentTrans == NULL ) {
                    continue;
                    }
                
                int numParentTrans = parentTrans->size(); 
                for( int t=0; t<numParentTrans; t++ ) {
                    
                    TransRecord *tr = parentTrans->getElementDirect( t );
                    
                    // override transitions might exist for object
                    // concretely

                    // OR they may have already been generated
                    // (we go through listed parents for this object in order
                    //  with previous earlier overriding later parents)

                    if( tr->actor == parentID ) {
                        // check if an override trans exists for the object
                        // as actor
                        
                        TransRecord *oTR = getTrans( oID, tr->target );
                        
                        if( oTR != NULL ) {
                            // skip this abstract terans
                            continue;
                            }
                        }
                    if( tr->target == parentID ) {
                        // check if an override trans exists for the object
                        // as target
                        
                        TransRecord *oTR = getTrans( tr->actor, oID );
                        
                        if( oTR != NULL ) {
                            // skip this abstract terans
                            continue;
                            }
                        }
                    
                    // got here:  no override transition exists for this
                    // object
                    
                    int actor = tr->actor;
                    int target = tr->target;
                    int newActor = tr->newActor;
                    int newTarget = tr->newTarget;
                    int autoDecaySeconds = tr->autoDecaySeconds;

                    
                    // plug object in to replace parent
                    if( actor == parentID ) {
                        actor = oID;
                        }
                    if( target == parentID ) {
                        target = oID;
                        }
                    if( newActor == parentID ) {
                        newActor = oID;
                        }
                    if( newTarget == parentID ) {
                        newTarget = oID;
                        }
                    
                    // don't write to disk
                    addTrans( actor, target, newActor, newTarget,
                              false, 
                              autoDecaySeconds, 
                              tr->actorMinUseFraction,
                              tr->targetMinUseFraction, 
                              true );
                    }
                }
            }
        
        delete [] objects;

        printf( "Auto-generated %d transitions based on categories\n", 
                records.size() - numRecords );
        
        numRecords = records.size();
        }
    




    if( autoGenerateGenericUseTransitions ) {
        
        int numGenerics = 0;
        
        int numChanged = 0;
        
        int numAdded = 0;
        
                
        int numObjects;
        
        SimpleVector<TransRecord> transToAdd;
        

        ObjectRecord **objects = getAllObjects( &numObjects );
        
        for( int i=0; i<numObjects; i++ ) {
            ObjectRecord *o = objects[i];
            
            if( o->foodValue == 0 ) {            
                
                int oID = o->id;

                TransRecord *genericTrans = getTrans( oID, -1, false );
                
                if( genericTrans == NULL ) {
                    // try last use instead
                    genericTrans = getTrans( oID, -1, true );
                    }
                
                
                if( genericTrans != NULL && genericTrans->newTarget == 0 ) {
                    
                    char lastUse = genericTrans->lastUse;

                    numGenerics ++;
                    

                    SimpleVector<TransRecord*> *objTrans = getAllUses( oID );

                    int numTrans = objTrans->size();
                    
                    for( int t=0; t<numTrans; t++ ) {
                        TransRecord *tr = objTrans->getElementDirect( t );
                    
                        if( tr->actor == oID && tr->newActor == oID ) {
                            
                            // self-to-self tool use

                            if( lastUse ) {
                                // add a new one

                                TransRecord newTrans = *tr;
                                
                                newTrans.lastUse = true;

                                newTrans.newActor = genericTrans->newActor;
                                
                                transToAdd.push_back( newTrans );

                                numAdded++;
                                }
                            else {
                                // replace in-place with generic use result
                                tr->newActor = genericTrans->newActor;
                                
                                numChanged ++;
                                }
                            }
                        }
                    }
                }
            }
        
        for( int t=0; t<transToAdd.size(); t++ ) {
            TransRecord tr = transToAdd.getElementDirect( t );
            
            addTrans( tr.actor, tr.target,
                      tr.newActor, tr.newTarget,
                      tr.lastUse,
                      tr.autoDecaySeconds,
                      tr.actorMinUseFraction,
                      tr.targetMinUseFraction,
                      true );
            }
        

        printf( "Auto-modified %d transitions based generic use transitions "
                "and auto-added %d last use generic transitions "
                "(%d objects had generic use transitions defined).\n", 
                numChanged, numAdded, numGenerics );
        }
    


    
    if( autoGenerateUsedObjectTransitions ) {
                
        int numGenerated = 0;
        int numRemoved = 0;


        int numObjects;
        
        ObjectRecord **objects = getAllObjects( &numObjects );
        
        for( int i=0; i<numObjects; i++ ) {
            ObjectRecord *o = objects[i];
            
            int oID = o->id;
            
            if( o->numUses > 1 && o->useDummyIDs != NULL ) {
                
                SimpleVector<TransRecord*> *objTransOrig = 
                    getAllUses( oID );
                
                int numTrans = objTransOrig->size();
                
                SimpleVector<TransRecord*> transToDelete;
                
                // can't rely on indexing in Orig, because it
                // will change as we add trans
                // copy them all first
                SimpleVector<TransRecord> obTransCopy;
                

                for( int t=0; t<numTrans; t++ ) {
                    TransRecord *tr = objTransOrig->getElementDirect( t );
                    obTransCopy.push_back( *tr );
                    }
                
                
                for( int t=0; t<numTrans; t++ ) {
                    TransRecord *tr = obTransCopy.getElement( t );
                    
                    TransRecord newTrans = *tr;
                    
                    if( tr->lastUse ) {
                        if( tr->actor == oID ) {
                            newTrans.actor = 
                                o->useDummyIDs[0]; 
                            
                            addTrans( newTrans.actor,
                                      newTrans.target,
                                      newTrans.newActor,
                                      newTrans.newTarget,
                                      false,
                                      newTrans.autoDecaySeconds,
                                      0.0f,
                                      0.0f,
                                      true );
                            
                            numGenerated++;
                            
                            transToDelete.push_back( tr );
                            }
                        else if( tr->target == oID ) {
                            newTrans.target = 
                                o->useDummyIDs[0];
                            
                            addTrans( newTrans.actor,
                                      newTrans.target,
                                      newTrans.newActor,
                                      newTrans.newTarget,
                                      false,
                                      newTrans.autoDecaySeconds,
                                      0.0f,
                                      0.0f,
                                      true );
                            
                            numGenerated++;
                            
                            transToDelete.push_back( tr );
                            }
                        }
                    else if( tr->actor == oID &&
                             tr->newActor == oID ) {
                            
                        for( int u=0; u<o->numUses-2; u++ ) {
                            float useFraction = 
                                (float)( u+1 ) / (float)( o->numUses );
                            
                            if( useFraction >= tr->actorMinUseFraction ) {
                                
                                newTrans.actor = o->useDummyIDs[ u + 1 ];
                                newTrans.newActor = o->useDummyIDs[ u ];
                                
                                addTrans( newTrans.actor,
                                          newTrans.target,
                                          newTrans.newActor,
                                          newTrans.newTarget,
                                          false,
                                          newTrans.autoDecaySeconds,
                                          0.0f,
                                          0.0f,
                                          true );
                                numGenerated++;
                                }
                            }
                        float useFraction = 
                            (float)( o->numUses - 2 ) / (float)( o->numUses );
                        
                        if( useFraction >= tr->actorMinUseFraction ) {
                            newTrans.actor = oID;
                            newTrans.newActor = 
                                o->useDummyIDs[ o->numUses - 2 ];
                        
                            addTrans( newTrans.actor,
                                      newTrans.target,
                                      newTrans.newActor,
                                      newTrans.newTarget,
                                      false,
                                      newTrans.autoDecaySeconds,
                                      0.0f,
                                      0.0f,
                                      true );
                            numGenerated++;
                            }
                        }
                    else if( tr->target == oID &&
                             tr->newTarget == oID ) {
                        
                        for( int u=0; u<o->numUses-2; u++ ) {    
                            float useFraction = 
                                (float)( u+1 ) / (float)( o->numUses );

                            if( useFraction >= tr->targetMinUseFraction ) {
                                
                                newTrans.target = o->useDummyIDs[ u + 1 ];
                                newTrans.newTarget = o->useDummyIDs[ u ];
                            
                                addTrans( newTrans.actor,
                                          newTrans.target,
                                          newTrans.newActor,
                                          newTrans.newTarget,
                                          false,
                                          newTrans.autoDecaySeconds,
                                          0.0f,
                                          0.0f,
                                          true );
                                numGenerated++;
                                }
                            }

                        
                        float useFraction = 
                            (float)( o->numUses - 2 ) / (float)( o->numUses );
                        
                        if( useFraction >= tr->targetMinUseFraction ) {
                            
                            newTrans.target = oID;
                            newTrans.newTarget = 
                                o->useDummyIDs[ o->numUses - 2 ];
                        
                            addTrans( newTrans.actor,
                                      newTrans.target,
                                      newTrans.newActor,
                                      newTrans.newTarget,
                                      false,
                                      newTrans.autoDecaySeconds,
                                      0.0f,
                                      0.0f,
                                      true );
                            numGenerated++;
                            }
                        }
                    else if( tr->actor == oID && 
                             tr->actorMinUseFraction < 1.0f ) {
                        
                        ObjectRecord *newActorObj = NULL;
                        
                        if( tr->newActor > 0 ) {
                            newActorObj = getObject( tr->newActor );
                            }

                        // this transition can apply to use dummies also
                        for( int u=0; u<o->numUses-1; u++ ) {
                            float useFraction = 
                                (float)( u ) / (float)( o->numUses );
                            if( useFraction >= tr->actorMinUseFraction ) {
                                
                                newTrans.actor = o->useDummyIDs[ u ];
                                
                                if( newActorObj != NULL &&
                                    newActorObj->numUses > 1 ) {
                                    
                                    // propage used status to new actor
                                    
                                    int usesLeft = 
                                        (int)( useFraction * 
                                               newActorObj->numUses );
                                    
                                    newTrans.newActor =
                                        newActorObj->useDummyIDs[ usesLeft ];
                                    }
                                

                                addTrans( newTrans.actor,
                                          newTrans.target,
                                          newTrans.newActor,
                                          newTrans.newTarget,
                                          false,
                                          newTrans.autoDecaySeconds,
                                          0.0f,
                                          0.0f,
                                          true );
                                numGenerated++;
                                }
                            }
                        }
                    else if( tr->target == oID && 
                             tr->targetMinUseFraction < 1.0f ) {
                        
                        ObjectRecord *newTargetObj = NULL;
                        
                        if( tr->newTarget > 0 ) {
                            newTargetObj = getObject( tr->newTarget );
                            }


                        // this transition can apply to use dummies also
                        for( int u=0; u<o->numUses-1; u++ ) {
                            float useFraction = 
                                (float)( u ) / (float)( o->numUses );
                            if( useFraction >= tr->targetMinUseFraction ) {
                                
                                newTrans.target = o->useDummyIDs[ u ];
                                
                                if( newTargetObj != NULL &&
                                    newTargetObj->numUses > 1 ) {
                                    
                                    // propage used status to new target
                                    
                                    int usesLeft = 
                                        (int)( useFraction * 
                                               newTargetObj->numUses );
                                    
                                    newTrans.newTarget =
                                        newTargetObj->useDummyIDs[ usesLeft ];
                                    }
                                

                                addTrans( newTrans.actor,
                                          newTrans.target,
                                          newTrans.newActor,
                                          newTrans.newTarget,
                                          false,
                                          newTrans.autoDecaySeconds,
                                          0.0f,
                                          0.0f,
                                          true );
                                numGenerated++;
                                }
                            }
                        
                        }
                    
                    }
                
                for( int t=0; t<transToDelete.size(); t++ ) {
                    TransRecord *tr = transToDelete.getElementDirect( t );
                    
                    deleteTransFromBank( tr->actor, tr->target,
                                         tr->lastUse,
                                         true );
                    numRemoved++;
                    }
                
                }
            }
        
        delete [] objects;

        printf( "Auto-generated %d transitions based used objects, "
                "removing %d transitions in the process\n", 
                numGenerated, numRemoved );
        
        numRecords = records.size();
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




TransRecord *getTrans( int inActor, int inTarget, char inLastUse ) {
    int mapIndex = inTarget;
    
    if( mapIndex < 0 ) {
        mapIndex = inActor;
        }
    
    if( mapIndex < 0 ) {
        return NULL;
        }

    if( mapIndex >= mapSize ) {
        return NULL;
        }
    
    int numRecords = usesMap[mapIndex].size();
    
    for( int i=0; i<numRecords; i++ ) {
        
        TransRecord *r = usesMap[mapIndex].getElementDirect(i);
        
        if( r->actor == inActor && r->target == inTarget &&
            r->lastUse == inLastUse ) {
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



SimpleVector<TransRecord*> *getAllUses( int inUsesID ) {
    if( inUsesID >= mapSize ) {
        return NULL;
        }

    return &( usesMap[inUsesID] );
    }




char isAncestor( int inTargetID, int inPossibleAncestorID, int inStepLimit ) {
    
    if( inStepLimit == 0 || inTargetID >= mapSize ) {
        return false;
        }
    
    for( int i=0; i< producesMap[inTargetID].size(); i++ ) {
        
        TransRecord *r = producesMap[inTargetID].getElementDirect( i );
        
        // make sure inTargetID came from something else as part of this
        // transition (it wasn't a tool itself)

        if( r->newTarget == inTargetID &&
            r->target == inTargetID ) {
            // inTargetID unchanged
            continue;
            }
        if( r->newActor == inTargetID &&
            r->actor == inTargetID ) {
            // unchanged as actor
            continue;
            }
        
        if( r->autoDecaySeconds > 0 ) {
            // don't count auto decays
            continue;
            }
        

        if( r->actor == inPossibleAncestorID || 
              r->target == inPossibleAncestorID ) {
            return true;
            }

        if( r->actor > 0 &&
            isAncestor( r->actor, inPossibleAncestorID, inStepLimit - 1 ) ) {
            return true;
            }
        
        if( r->target > 0 && 
            isAncestor( r->target, inPossibleAncestorID, inStepLimit - 1 ) ) {
            return true;
            }
        }
    
    return false;
    }





static char *getFileName( int inActor, int inTarget, char inLastUse ) {
    const char *lastUseString = "";
    
    if( inLastUse ) {
        lastUseString = "_L";
        }
    
    return autoSprintf( "%d_%d%s.txt", inActor, inTarget, lastUseString );
    }



void addTrans( int inActor, int inTarget,
               int inNewActor, int inNewTarget,
               char inLastUse,
               int inAutoDecaySeconds,
               float inActorMinUseFraction,
               float inTargetMinUseFraction,
               char inNoWriteToFile ) {
    
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
    TransRecord *t = getTrans( inActor, inTarget, inLastUse );
    
    char writeToFile = false;
    

    if( t == NULL ) {
        t = new TransRecord;

        t->actor = inActor;
        t->target = inTarget;
        t->newActor = inNewActor;
        t->newTarget = inNewTarget;
        t->autoDecaySeconds = inAutoDecaySeconds;
        t->lastUse = inLastUse;
        t->actorMinUseFraction = inActorMinUseFraction;
        t->targetMinUseFraction = inTargetMinUseFraction;

        records.push_back( t );

        if( inActor > 0 ) {
            usesMap[inActor].push_back( t );
            }
        
        // no duplicate records when actor and target are the same
        if( inTarget >= 0 && inTarget != inActor ) {    
            usesMap[inTarget].push_back( t );
            }
        
        if( inNewActor != 0 ) {
            producesMap[inNewActor].push_back( t );
            }
        
        // avoid duplicate records here too
        if( inNewTarget != 0 && inNewTarget != inNewActor ) {    
            producesMap[inNewTarget].push_back( t );
            }
        
        writeToFile = true;
        }
    else {
        // update it
        
        // "records" already contains it, no change
        // usesMap already contains it, no change

        if( t->newTarget == inNewTarget &&
            t->newActor == inNewActor &&
            t->autoDecaySeconds == inAutoDecaySeconds &&
            t->actorMinUseFraction == inActorMinUseFraction &&
            t->targetMinUseFraction == inTargetMinUseFraction ) {
            
            // no change to produces map either... 

            // don't need to change record at all, done.
            }
        else {
            
            // remove record from producesMaps
            
            if( t->newActor != 0 ) {
                producesMap[t->newActor].deleteElementEqualTo( t );
                }
            if( t->newTarget != 0 ) {                
                producesMap[t->newTarget].deleteElementEqualTo( t );
                }
            

            t->newActor = inNewActor;
            t->newTarget = inNewTarget;
            t->autoDecaySeconds = inAutoDecaySeconds;
            t->actorMinUseFraction = inActorMinUseFraction;
            t->targetMinUseFraction = inTargetMinUseFraction;
            
            if( inNewActor != 0 ) {
                producesMap[inNewActor].push_back( t );
                }
            
            // no duplicate records
            if( inNewTarget != 0 && inNewTarget != inNewActor ) {    
                producesMap[inNewTarget].push_back( t );
                }
            
            writeToFile = true;
            }
        }
    
    
    if( writeToFile && ! inNoWriteToFile ) {
        
        File transDir( NULL, "transitions" );
            
        if( !transDir.exists() ) {
            transDir.makeDirectory();
            }
        
        if( transDir.exists() && transDir.isDirectory() ) {

            char *fileName = getFileName( inActor, inTarget, inLastUse );
            

            File *transFile = transDir.getChildFile( fileName );
            
            char *fileContents = autoSprintf( "%d %d %d %f %f", 
                                              inNewActor, inNewTarget,
                                              inAutoDecaySeconds,
                                              inActorMinUseFraction,
                                              inTargetMinUseFraction );

        
            File *cacheFile = transDir.getChildFile( "cache.fcz" );

            cacheFile->remove();
            
            delete cacheFile;
            

            transFile->writeToFile( fileContents );
            
            delete [] fileContents;
            delete transFile;
            delete [] fileName;
            }
        }
    
    }



void deleteTransFromBank( int inActor, int inTarget,
                          char inLastUse,
                          char inNoWriteToFile ) {
    
    // one exists?
    TransRecord *t = getTrans( inActor, inTarget, inLastUse );
    
    if( t != NULL ) {
        
        if( ! inNoWriteToFile ) {    
            File transDir( NULL, "transitions" );
            
            if( transDir.exists() && transDir.isDirectory() ) {
                
                char *fileName = getFileName( inActor, inTarget, inLastUse );
                
                File *transFile = transDir.getChildFile( fileName );
                
                
                File *cacheFile = transDir.getChildFile( "cache.fcz" );
                
                cacheFile->remove();
                
                delete cacheFile;
                
                
                transFile->remove();
                delete transFile;
                delete [] fileName;
                }
            }
        
        

        if( t->newActor != 0 ) {
            producesMap[t->newActor].deleteElementEqualTo( t );
            }
        if( t->newTarget != 0 ) {                
            producesMap[t->newTarget].deleteElementEqualTo( t );
            }
        
        if( inActor > 0 ) {
            usesMap[inActor].deleteElementEqualTo( t );
            }
        
        if( inTarget >= 0 ) {    
            usesMap[inTarget].deleteElementEqualTo( t );
            }
        

        records.deleteElementEqualTo( t );

        delete t;
        }
    }




char isObjectUsed( int inObjectID ) {
    
    
    int numResults, numRemaining;
    
    TransRecord **results = searchUses( inObjectID, 
                                        0, 1,
                                        &numResults, &numRemaining );
    
    if( results != NULL ) {
        delete [] results;
        
        return true;
        }
    
    
    results = searchProduces( inObjectID, 
                              0, 1,
                              &numResults, &numRemaining );
    
    if( results != NULL ) {
        delete [] results;
        
        return true;
        }

    return false;
    }



char isGrave( int inObjectID ) {
    
    ObjectRecord *r = getObject( inObjectID );
    

    if( r->deathMarker ) {
        return true;
        }

    SimpleVector<int> seenIDs;
    
    seenIDs.push_back( inObjectID );

    while( r != NULL && ! r->deathMarker ) {
        int numResults;
        int numRemaining;
        
        TransRecord **records =
            searchProduces( r->id, 
                            0,
                            // only look at 1 thing that produces this
                            // grave decay chains are necessarily single-parent
                            1, 
                            &numResults, &numRemaining );
        
        if( numResults == 0 || numRemaining != 0 ) {
            r = NULL;
            }
        else {
            int newID = records[0]->target;
            
            if( newID > 0 &&
                seenIDs.getElementIndex( newID ) == -1 ) {
                r = getObject( newID );

                seenIDs.push_back( newID );
                }
            else {
                r = NULL;
                }
            }
        
        if( records != NULL ) {
            delete [] records;
            }
        }
    
    if( r == NULL  ) {
        return false;
        }
    else {
        return r->deathMarker;
        }
    }





const char *getObjName( int inObjectID ) {
    if( inObjectID > 0 ) {
        
        ObjectRecord *o = getObject( inObjectID );
        
        if( o != NULL ) {
            return o->description;
            }
        else {
            return "Unknown Obj ID";
            }
        }
    else if( inObjectID == 0 ) {
        return "0";
        }
    else if( inObjectID == -1 ) {
        return "-1";
        }
    else if( inObjectID == -2 ) {
        return "-2";
        }
    else {
        return "Unknown Code";
        }
    }



void printTrans( TransRecord *inTrans ) {
    printf( "[%s](%.2f) + [%s](%.2f) => [%s] + [%s]",
            getObjName( inTrans->actor ),
            inTrans->actorMinUseFraction,
            getObjName( inTrans->target ),
            inTrans->targetMinUseFraction,
            getObjName( inTrans->newActor ), 
            getObjName( inTrans->newTarget ) );
    
    if( inTrans->lastUse ) {
        printf( " (lastUse)\n" );
        }
    else {
        printf( "\n" );
        }
    }

