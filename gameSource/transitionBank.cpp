#include "transitionBank.h"


#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"



#include "folderCache.h"
#include "objectBank.h"
#include "categoryBank.h"



void regenerateDepthMap();

void regenerateHumanMadeMap();



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


static int depthMapSize = 0;
static int *depthMap = NULL;


static int humanMadeMapSize = 0;
static char *humanMadeMap = NULL;



static FolderCache cache;

static int currentFile;

static int maxID;

static char autoGenerateCategoryTransitions = false;
static char autoGenerateUsedObjectTransitions = false;
static char autoGenerateGenericUseTransitions = false;
static char autoGenerateVariableTransitions = false;


int initTransBankStart( char *outRebuildingCache,
                        char inAutoGenerateCategoryTransitions,
                        char inAutoGenerateUsedObjectTransitions,
                        char inAutoGenerateGenericUseTransitions,
                        char inAutoGenerateVariableTransitions ) {
    
    autoGenerateCategoryTransitions = inAutoGenerateCategoryTransitions;
    autoGenerateUsedObjectTransitions = inAutoGenerateUsedObjectTransitions;
    autoGenerateGenericUseTransitions = inAutoGenerateGenericUseTransitions;
    autoGenerateVariableTransitions = inAutoGenerateVariableTransitions;
    
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
        char lastUseActor = false;
        char lastUseTarget = false;
        
        if( strstr( txtFileName, "_LA" ) != NULL ) {
            lastUseActor = true;
            }
        if( strstr( txtFileName, "_LT" ) != NULL ) {
            lastUseTarget = true;
            }
        else if( strstr( txtFileName, "_L." ) != NULL ) {
            // old file name format
            // _L means last use of target
            lastUseTarget = true;
            }
        
        sscanf( txtFileName, "%d_%d", &actor, &target );
        
        if(  target != -2 ) {

            char *contents = getFileContents( cache, i );
                        
            if( contents != NULL ) {
                            
                int newActor = 0;
                int newTarget = 0;
                int autoDecaySeconds = 0;
                int epochAutoDecay = 0;
                float actorMinUseFraction = 0.0f;
                float targetMinUseFraction = 0.0f;
                
                int reverseUseActorFlag = 0;
                int reverseUseTargetFlag = 0;

                int move = 0;
                int desiredMoveDist = 1;
                
                sscanf( contents, "%d %d %d %f %f %d %d %d %d", 
                        &newActor, &newTarget, &autoDecaySeconds,
                        &actorMinUseFraction, &targetMinUseFraction,
                        &reverseUseActorFlag,
                        &reverseUseTargetFlag,
                        &move,
                        &desiredMoveDist );
                
                if( autoDecaySeconds < 0 ) {
                    epochAutoDecay = -autoDecaySeconds;
                    }

                TransRecord *r = new TransRecord;
                            
                r->actor = actor;
                r->target = target;
                r->newActor = newActor;
                r->newTarget = newTarget;
                r->autoDecaySeconds = autoDecaySeconds;
                r->epochAutoDecay = epochAutoDecay;
                r->lastUseActor = lastUseActor;
                r->lastUseTarget = lastUseTarget;

                r->move = move;
                r->desiredMoveDist = desiredMoveDist;


                r->reverseUseActor = false;
                if( reverseUseActorFlag == 1 ) {
                    r->reverseUseActor = true;
                    }

                r->reverseUseTarget = false;
                if( reverseUseTargetFlag == 1 ) {
                    r->reverseUseTarget = true;
                    }
                
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
                    //  with earlier overriding later parents)

                    if( tr->actor == parentID ) {
                        // check if an override trans exists for the object
                        // as actor
                        
                        TransRecord *oTR = getTrans( oID, tr->target, 
                                                     tr->lastUseActor,
                                                     tr->lastUseTarget );
                        
                        if( oTR != NULL ) {
                            // skip this abstract trans
                            continue;
                            }
                        }
                    if( tr->target == parentID ) {
                        // check if an override trans exists for the object
                        // as target
                        
                        TransRecord *oTR = getTrans( tr->actor, oID,
                                                     tr->lastUseActor,
                                                     tr->lastUseTarget );
                        
                        if( oTR != NULL ) {
                            // skip this abstract trans
                            continue;
                            }
                        }
                    
                    // got here:  no override transition exists for this
                    // object
                    
                    int actor = tr->actor;
                    int target = tr->target;
                    int newActor = tr->newActor;
                    int newTarget = tr->newTarget;
                    
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
                              tr->lastUseActor,
                              tr->lastUseTarget,
                              tr->reverseUseActor,
                              tr->reverseUseTarget,
                              tr->autoDecaySeconds,
                              tr->actorMinUseFraction,
                              tr->targetMinUseFraction, 
                              tr->move,
                              tr->desiredMoveDist,
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
                    
                    char lastUseActor = genericTrans->lastUseActor;
                    
                    numGenerics ++;
                    

                    SimpleVector<TransRecord*> *objTrans = getAllUses( oID );

                    int numTrans = objTrans->size();
                    
                    for( int t=0; t<numTrans; t++ ) {
                        TransRecord *tr = objTrans->getElementDirect( t );
                    
                        if( tr->actor == oID && tr->newActor == oID ) {
                            
                            // self-to-self tool use

                            if( lastUseActor ) {
                                // add a new one

                                TransRecord newTrans = *tr;
                                
                                newTrans.lastUseActor = true;

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
        
        delete [] objects;
        
        for( int t=0; t<transToAdd.size(); t++ ) {
            TransRecord tr = transToAdd.getElementDirect( t );
            
            addTrans( tr.actor, tr.target,
                      tr.newActor, tr.newTarget,
                      tr.lastUseActor,
                      tr.lastUseTarget,
                      tr.reverseUseActor,
                      tr.reverseUseTarget,
                      tr.autoDecaySeconds,
                      tr.actorMinUseFraction,
                      tr.targetMinUseFraction,
                      tr.move,
                      tr.desiredMoveDist,
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
        

        SimpleVector<TransRecord*> transToDelete;
        SimpleVector<TransRecord> transToAdd;
        
        
        int numObjects;
        
        ObjectRecord **objects = getAllObjects( &numObjects );
        
        for( int i=0; i<numObjects; i++ ) {
            ObjectRecord *o = objects[i];
            
            int oID = o->id;
            
            if( o->numUses > 1 && o->useDummyIDs != NULL ) {
                
                SimpleVector<TransRecord*> *objTransOrig = 
                    getAllUses( oID );
                
                int numTrans = 0;

                if( objTransOrig != NULL ) {
                    numTrans = objTransOrig->size();
                    }


                for( int t=0; t<numTrans; t++ ) {
                    TransRecord *tr = objTransOrig->getElementDirect( t );
                    
                    TransRecord newTrans = *tr;
                    newTrans.lastUseActor = false;
                    newTrans.lastUseTarget = false;
                    newTrans.reverseUseActor = false;
                    newTrans.reverseUseTarget = false;
                    newTrans.actorMinUseFraction = 0.0f;
                    newTrans.targetMinUseFraction = 0.0f;
                    

                    if( tr->lastUseActor 
                        &&
                        tr->actor == oID ) {
                            
                        if( ! tr->reverseUseActor ) {    
                            newTrans.actor = 
                                o->useDummyIDs[0]; 
                            }
                        
                        transToAdd.push_back( newTrans );
                        
                        transToDelete.push_back( tr );
                        }
                    else if( tr->lastUseTarget 
                             &&
                             tr->target == oID ) {
                            
                        if( ! tr->reverseUseTarget ) {
                            newTrans.target = 
                                o->useDummyIDs[0];
                            }
                        
                        transToAdd.push_back( newTrans );
                        
                        transToDelete.push_back( tr );
                        }
                    else if( tr->actor == oID &&
                             tr->newActor == oID ) {
                            
                        for( int u=0; u<o->numUses-2; u++ ) {
                            float useFraction = 
                                (float)( u+1 ) / (float)( o->numUses );
                            
                            if( useFraction >= tr->actorMinUseFraction ) {
                                
                                TransRecord newTransD = newTrans;

                                if( tr->reverseUseActor ) {
                                    newTransD.actor = o->useDummyIDs[ u ];
                                    newTransD.newActor = 
                                        o->useDummyIDs[ u + 1 ];
                                    }
                                else {
                                    newTransD.actor = o->useDummyIDs[ u + 1 ];
                                    newTransD.newActor = o->useDummyIDs[ u ];
                                    }
                                
                                transToAdd.push_back( newTransD );
                                }
                            }
                        float useFraction = 
                            (float)( o->numUses - 2 ) / (float)( o->numUses );
                        
                        if( useFraction >= tr->actorMinUseFraction ) {
                            
                            if( tr->reverseUseActor ) {
                                newTrans.actor = 
                                    o->useDummyIDs[ o->numUses - 2 ];
                                newTrans.newActor = oID;
                                
                                // this trans no longer applies
                                // directly to actor
                                transToDelete.push_back( tr );
                                }
                            else {
                                newTrans.actor = oID;
                                newTrans.newActor = 
                                    o->useDummyIDs[ o->numUses - 2 ];
                                }
                            
                            transToAdd.push_back( newTrans );
                            }
                        }
                    else if( tr->target == oID &&
                             tr->newTarget == oID ) {
                        
                        for( int u=0; u<o->numUses-2; u++ ) {    
                            float useFraction = 
                                (float)( u+1 ) / (float)( o->numUses );

                            if( useFraction >= tr->targetMinUseFraction ) {
                                TransRecord newTransD = newTrans;
                                
                                if( tr->reverseUseTarget ) {

                                    // this use dummy auto-decays
                                    // back to previous use dummy

                                    // OR it is getting reverse-used
                                    // back to the previous use dummy 
                                    newTransD.target = o->useDummyIDs[ u ];
                                    newTransD.newTarget =
                                        o->useDummyIDs[ u + 1 ];
                                    }
                                else {
                                    newTransD.target = o->useDummyIDs[ u + 1 ];
                                    newTransD.newTarget = o->useDummyIDs[ u ];
                                    }
                                
                                transToAdd.push_back( newTransD );
                                }
                            }

                        
                        float useFraction = 
                            (float)( o->numUses - 2 ) / (float)( o->numUses );
                        
                        if( useFraction >= tr->targetMinUseFraction ) {
                            
                            if( tr->reverseUseTarget ) {        
                                newTrans.target =
                                    o->useDummyIDs[ o->numUses - 2 ];
                                newTrans.newTarget = oID;
                                
                                // this trans no longer applies
                                // directly to target
                                transToDelete.push_back( tr );
                                }
                            else {
                                newTrans.target = oID;
                                newTrans.newTarget = 
                                    o->useDummyIDs[ o->numUses - 2 ];
                                }
                            
                            transToAdd.push_back( newTrans );
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
                                    
                                    // propagate used status to new actor
                                    int usesLeft = 
                                        lrint( useFraction * 
                                               newActorObj->numUses );

                                    // if num uses in actor and new actor align
                                    // AND newActor has a last-use trans
                                    // defined, then count this as a use
                                    // Checking that newActor has a last
                                    // use prevents double-decrement
                                    // (example:  shovel picks up dung,
                                    //  no use decrement, but then shovel
                                    //  drops dung triggering one use decrement)
                                    if( newActorObj->numUses == o->numUses &&
                                        getTrans( tr->newActor, -1, true ) !=
                                        NULL ) {
                                        
                                        // decrement (count as a use)

                                        usesLeft--;
                                        
                                        if( usesLeft < 0 ) {
                                            // substitute last-use
                                            // transition in place
                                            TransRecord *lastUseTR =
                                                getTrans( tr->newActor,
                                                          -1, true );

                                            if( lastUseTR != NULL ) {
                                                newTrans.newActor =
                                                    lastUseTR->newActor;
                                                }
                                            else {
                                                usesLeft = 0;
                                                }
                                            }

                                        if( u == o->numUses - 2 ) {
                                            // do this once in here
                                            // so we don't need to 
                                            // replicate all the 
                                            // case-detection logic
                                            // outside the loop

                                            // we need to enter into
                                            // the use dummies of
                                            // newActor
                                                
                                            TransRecord startRecord =
                                                *tr;
                                            startRecord.newActor =
                                                newActorObj->useDummyIDs[ 
                                                    o->numUses - 2 ];
                                                
                                            transToDelete.push_back( tr );
                                            transToAdd.push_back( 
                                                startRecord );
                                            }
                                        }
                                    
                                    if( usesLeft >= 0 ) {
                                        newTrans.newActor =
                                            newActorObj->useDummyIDs[ 
                                                usesLeft ];
                                        }
                                    }
                                else if( tr->newTarget > 0 &&
                                         tr->target > 0 &&
                                         tr->target != tr->newTarget ) {
                                    
                                    ObjectRecord *targetObj =
                                        getObject( tr->target );
                                    ObjectRecord *newTargetObj =
                                        getObject( tr->newTarget );
                                    
                                    if( ! targetObj->isUseDummy &&
                                        ! newTargetObj->isUseDummy &&
                                        targetObj->numUses == 1 &&
                                        newTargetObj->numUses == o->numUses ) {
                                        // propagate used status to new target
                                    
                                        int usesLeft = 
                                            lrint( useFraction * 
                                                   newTargetObj->numUses );
                                    
                                        newTrans.newTarget =
                                            newTargetObj->useDummyIDs[ 
                                                usesLeft ];
                                        }
                                    }
                                
                                transToAdd.push_back( newTrans );
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
                                    
                                    // propagate used status to new target
                                    
                                    int usesLeft = 
                                        lrint( useFraction * 
                                               newTargetObj->numUses );
                                    
                                    newTrans.newTarget =
                                        newTargetObj->useDummyIDs[ usesLeft ];
                                    }
                                else if( tr->actor >= 0 && 
                                         tr->newActor > 0 ) {
                                    
                                    char actorUseDummy = false;
                                    int actorNumUses = 1;
                                    if( tr->actor > 0 ) {
                                        ObjectRecord *actorObj =
                                            getObject( tr->actor );
                                        
                                        actorUseDummy = actorObj->isUseDummy;
                                        actorNumUses = actorObj->numUses;
                                        }
                                    
                                    ObjectRecord *newActorObj =
                                        getObject( tr->newActor );
                                    
                                    if( ! actorUseDummy &&
                                        ! newActorObj->isUseDummy &&
                                        actorNumUses == 1 &&
                                        newActorObj->numUses == o->numUses ) {
                                        // propagate used status to new actor
                                    
                                        int usesLeft = 
                                            lrint( useFraction * 
                                                   newActorObj->numUses );
                                        
                                        // decrement (count as a use)

                                        usesLeft--;
                                        
                                        if( usesLeft < 0 ) {
                                            // substitute last-use
                                            // transition in place
                                            TransRecord *lastUseTR =
                                                getTrans( tr->newActor,
                                                          -1, true );

                                            if( lastUseTR != NULL ) {
                                                newTrans.newActor =
                                                    lastUseTR->newActor;
                                                }
                                            else {
                                                usesLeft = 0;
                                                }
                                            }

                                        if( usesLeft >= 0 ) {
                                            newTrans.newActor =
                                                newActorObj->useDummyIDs[ 
                                                    usesLeft ];

                                            if( u == o->numUses - 2 ) {
                                                // do this once in here
                                                // so we don't need to 
                                                // replicate all the 
                                                // case-detection logic
                                                // outside the loop

                                                // we need to enter into
                                                // the use dummies of
                                                // newActor
                                                
                                                TransRecord startRecord =
                                                    *tr;
                                                startRecord.newActor =
                                                    newActorObj->useDummyIDs[ 
                                                        o->numUses - 2 ];
                                                
                                                transToDelete.push_back( tr );
                                                transToAdd.push_back( 
                                                    startRecord );
                                                }
                                            }
                                        }
                                    }
                                
                                transToAdd.push_back( newTrans );
                                }
                            }
                        
                        }
                    }
                
                
                SimpleVector<TransRecord*> *prodTrans = getAllProduces( oID );
                
                int numProdTrans = 0;
                
                if( prodTrans != NULL ) {
                    numProdTrans = prodTrans->size();
                    }
                
                for( int t=0; t<numProdTrans; t++ ) {
                    
                    TransRecord *tr = prodTrans->getElementDirect( t );
                    
                    TransRecord newTrans = *tr;
                    newTrans.lastUseActor = false;
                    newTrans.lastUseTarget = false;
                    newTrans.reverseUseActor = false;
                    newTrans.reverseUseTarget = false;
                    newTrans.actorMinUseFraction = 0.0f;
                    newTrans.targetMinUseFraction = 0.0f;
                    
                    if( tr->actor != oID &&
                        tr->newActor == oID &&
                        tr->reverseUseActor ) {
                    
                        newTrans.newActor = o->useDummyIDs[ 0 ];
                        
                        transToAdd.push_back( newTrans );
                        }
                    else if( tr->target != oID && 
                             tr->newTarget == oID &&
                             tr->reverseUseTarget ) {
                        
                        newTrans.newTarget = o->useDummyIDs[ 0 ];
                        
                        transToAdd.push_back( newTrans );
                        }
                    }
                }
            }
        
                
                
        for( int t=0; t<transToDelete.size(); t++ ) {
            TransRecord *tr = transToDelete.getElementDirect( t );
                    
            deleteTransFromBank( tr->actor, tr->target,
                                 tr->lastUseActor,
                                 tr->lastUseTarget,
                                 true );
            numRemoved++;
            }
                
        for( int t=0; t<transToAdd.size(); t++ ) {
            TransRecord *newTrans = transToAdd.getElement( t );
                    
            addTrans( newTrans->actor,
                      newTrans->target,
                      newTrans->newActor,
                      newTrans->newTarget,
                      newTrans->lastUseActor,
                      newTrans->lastUseTarget,
                      newTrans->reverseUseActor,
                      newTrans->reverseUseTarget,
                      newTrans->autoDecaySeconds,
                      newTrans->actorMinUseFraction,
                      newTrans->targetMinUseFraction,
                      newTrans->move,
                      newTrans->desiredMoveDist,
                      true );
            numGenerated++;
            }
        
        
        delete [] objects;

        printf( "Auto-generated %d transitions based on used objects, "
                "removing %d transitions in the process\n", 
                numGenerated, numRemoved );
        
        numRecords = records.size();
        }
    
    
    


    if( autoGenerateVariableTransitions ) {
        
        SimpleVector<TransRecord> transToAdd;

        for( int i=0; i<records.size(); i++ ) {
            TransRecord *t = records.getElementDirect( i );
            
            ObjectRecord *actor = NULL;
            ObjectRecord *target = NULL;
            ObjectRecord *newActor = NULL;
            ObjectRecord *newTarget = NULL;
            
            if( t->actor > 0 ) {
                actor = getObject( t->actor );
                }
            if( t->target > 0 ) {
                target = getObject( t->target );
                }
            if( t->newActor > 0 ) {
                newActor = getObject( t->newActor );
                }
            if( t->newTarget > 0 ) {
                newTarget = getObject( t->newTarget );
                }
            
            ObjectRecord *leadObject = NULL;
            ObjectRecord *followObject = NULL;
            
            if( actor != NULL && actor->numVariableDummyIDs > 0 ) {
                leadObject = actor;
                }
            else if( target != NULL && target->numVariableDummyIDs > 0 ) {
                leadObject = target;
                }
            if( newTarget != NULL && newTarget->numVariableDummyIDs > 0 ) {
                followObject = newTarget;
                }
            else if( newActor != NULL && newActor->numVariableDummyIDs > 0 ) {
                followObject = newActor;
                }
            
            if( leadObject != NULL && leadObject != followObject ) {
                int num = leadObject->numVariableDummyIDs;
                
                for( int k=0; k<num; k++ ) {
                    TransRecord newTrans = *t;

                    if( actor != NULL && actor->numVariableDummyIDs == num ) {
                        newTrans.actor = actor->variableDummyIDs[k];
                        }

                    if( target != NULL && target->numVariableDummyIDs == num ) {
                        newTrans.target = target->variableDummyIDs[k];
                        }

                    if( newActor != NULL && 
                        newActor->numVariableDummyIDs == num ) {
                        
                        newTrans.newActor = newActor->variableDummyIDs[k];
                        }

                    if( newTarget != NULL && 
                        newTarget->numVariableDummyIDs == num ) {
                        
                        newTrans.newTarget = newTarget->variableDummyIDs[k];
                        }
                    transToAdd.push_back( newTrans );
                    }
                }
            else if( leadObject == NULL && followObject != NULL ) {
                // mapping some other object to variable
                // a single in-point into variable objects
                
                // this will actually replace the transition when we re-add
                // it below

                TransRecord newTrans = *t;
                
                if( actor != NULL && actor->numVariableDummyIDs > 0 ) {
                    newTrans.actor = actor->variableDummyIDs[0];
                    }
                if( target != NULL && target->numVariableDummyIDs > 0 ) {
                    newTrans.target = target->variableDummyIDs[0];
                    }
                if( newActor != NULL && newActor->numVariableDummyIDs > 0 ) {
                    newTrans.newActor = newActor->variableDummyIDs[0];
                    }
                if( newTarget != NULL && newTarget->numVariableDummyIDs > 0 ) {
                    newTrans.newTarget = newTarget->variableDummyIDs[0];
                    }
                
                transToAdd.push_back( newTrans );
                }
            else if( leadObject != NULL && leadObject == followObject ) {
                // map through subsequent variable objects
                int num = leadObject->numVariableDummyIDs;
                                
                for( int k=0; k<num-1; k++ ) {
                    TransRecord newTrans = *t;

                    if( actor != NULL && actor->numVariableDummyIDs == num ) {
                        newTrans.actor = actor->variableDummyIDs[k];
                        }
                    
                    if( target != NULL && target->numVariableDummyIDs == num ) {
                        newTrans.target = target->variableDummyIDs[k];
                        }

                    if( newActor != NULL && 
                        newActor->numVariableDummyIDs == num ) {
                        
                        newTrans.newActor = newActor->variableDummyIDs[k+1];
                        }

                    if( newTarget != NULL && 
                        newTarget->numVariableDummyIDs == num ) {
                        
                        newTrans.newTarget = newTarget->variableDummyIDs[k+1];
                        }
                    transToAdd.push_back( newTrans );
                    }
                }
            }
        
        int numGenerated = 0;
        
        for( int t=0; t<transToAdd.size(); t++ ) {
            TransRecord *newTrans = transToAdd.getElement( t );
                    
            addTrans( newTrans->actor,
                      newTrans->target,
                      newTrans->newActor,
                      newTrans->newTarget,
                      newTrans->lastUseActor,
                      newTrans->lastUseTarget,
                      newTrans->reverseUseActor,
                      newTrans->reverseUseTarget,
                      newTrans->autoDecaySeconds,
                      newTrans->actorMinUseFraction,
                      newTrans->targetMinUseFraction,
                      newTrans->move,
                      newTrans->desiredMoveDist,
                      true );
            numGenerated++;
            }
        
        printf( "Auto-generated %d transitions based on variable objects.\n", 
                numGenerated );
        }




    regenerateDepthMap();
    regenerateHumanMadeMap();
    }



void freeTransBank() {
    for( int i=0; i<records.size(); i++ ) {
        delete records.getElementDirect(i);
        }
    
    records.deleteAll();
    
    delete [] usesMap;
    delete [] producesMap;
    
    if( depthMap != NULL ) {
        delete [] depthMap;
        depthMap = NULL;
        }
    depthMapSize = 0;

    if( humanMadeMap != NULL ) {
        delete [] humanMadeMap;
        humanMadeMap = NULL;
        }
    humanMadeMapSize = 0;
    
    }



void regenerateDepthMap() {
    
    if( depthMap != NULL ) {    
        delete [] depthMap;
        depthMap = NULL;
        }
    
    depthMapSize = getMaxObjectID() + 1;
        
    depthMap = new int[ depthMapSize ];
        
    // all start unreachable
    for( int i=0; i<depthMapSize; i++ ) {
        depthMap[i] = UNREACHABLE;
        }
        

    int numObjects;
        
    ObjectRecord **objects = getAllObjects( &numObjects );
        
    SimpleVector<int> treeHorizon;

    for( int i=0; i<numObjects; i++ ) {
        ObjectRecord *o = objects[i];
            
        if( o->id >= depthMapSize ) {
            continue;
            }
            
        if( o->mapChance > 0 ) {
            // a natural object... a starting point
            depthMap[ o->id ] = 0;
                
            treeHorizon.push_back( o->id );
            }    
        }
    delete [] objects;
        
    int index = 0;
        
    while( index < treeHorizon.size() ) {
        int nextID = treeHorizon.getElementDirect( index );
            
        SimpleVector<TransRecord*> *uses = getAllUses( nextID );

        if( uses != NULL )
        for( int i=0; i<uses->size(); i++ ) {
                
            TransRecord *tr = uses->getElementDirect( i );
                
            // we need to know the depth of all ingredients
            // depth of offspring object is max of ingredient depth
            int nextDepth = UNREACHABLE;
                
            if( tr->actor == nextID && tr->target <= 0 ) {
                // this actor used by itself
                nextDepth = depthMap[ nextID ] + 1;
                }
            else if( tr->target == nextID && tr->actor <=0 ) {
                // this target used by itself
                nextDepth = depthMap[ nextID ] + 1;
                }
            else if( tr->actor == nextID && tr->actor == tr->target ) {
                // object used on itself
                nextDepth = depthMap[ nextID ] + 1;
                }
            else if( tr->actor > 0 && tr->target > 0 ) {
                    
                nextDepth = depthMap[ tr->actor ];

                if( nextDepth < depthMap[ tr->target ] ) {
                    nextDepth = depthMap[ tr->target ];
                    }
                nextDepth += 1;
                }
                
            if( nextDepth < UNREACHABLE ) {
                    
                if( tr->newActor > 0 ) {
                    if( depthMap[ tr->newActor ] == UNREACHABLE ) {
                        depthMap[ tr->newActor ] = nextDepth;
                        treeHorizon.push_back( tr->newActor );
                        }
                    }
                if( tr->newTarget > 0 ) {
                    if( depthMap[ tr->newTarget ] == UNREACHABLE ) {
                        depthMap[ tr->newTarget ] = nextDepth;
                        treeHorizon.push_back( tr->newTarget );
                        }
                    }
                }
            // else one of ingredients has unknown depth
            // we'll reach it later
            }

        index ++;
        }
    
    }



void regenerateHumanMadeMap() {
    
    if( humanMadeMap != NULL ) {    
        delete [] humanMadeMap;
        humanMadeMap = NULL;
        }
    
    humanMadeMapSize = getMaxObjectID() + 1;
        
    humanMadeMap = new char[ humanMadeMapSize ];

    // temporary, those that we KNOW are natural-made
    char *naturalMap = new char[ humanMadeMapSize ];
    char *unreachableMap = new char[ humanMadeMapSize ];
        
    // all start unknown
    for( int i=0; i<humanMadeMapSize; i++ ) {
        humanMadeMap[i] = false;
        naturalMap[i] = false;
        unreachableMap[i] = false;
        }
    

    int numObjects;
        
    ObjectRecord **objects = getAllObjects( &numObjects );

    for( int i=0; i<numObjects; i++ ) {
        ObjectRecord *o = objects[i];

        int oID = o->id;

        if( ! o->permanent ) {
            // these can be moved around by people
            humanMadeMap[oID] = true;
            continue;
            }
        if( o->mapChance > 0 ) {
            // natural object
            naturalMap[oID] = true;
            continue;
            }
        if( o->person ) {
            humanMadeMap[oID] = true;
            continue;
            }
        if( o->deathMarker ) {
            humanMadeMap[oID] = true;
            continue;
            }
        
        SimpleVector<TransRecord*> *prodTrans = getAllProduces( o->id );
        
        int numTrans = 0;
        
        if( prodTrans != NULL ) {
            numTrans = prodTrans->size();
            }
        
        if( numTrans == 0 ) {
            unreachableMap[oID] = true;
            continue;
            }
        
        // check for those immediately made by people
        for( int t=0; t<numTrans; t++ ) {
            TransRecord *trans = prodTrans->getElementDirect( t );
            
            if( trans->actor >= 0 || trans->actor == -2 ) {
                // bare hand or tool use produces this
                // or default human action
                humanMadeMap[ oID ] = true;
                break;
                }
            }
        }
    

    // now step for unknown-origin objects and see if we find known objects
    char anyUnknown = true;    

    // if non are assigned and some are still unknown, we have an orphaned
    // loop
    char anyAssigned = true;
    
    while( anyUnknown && anyAssigned ) {
        anyUnknown = false;
        anyAssigned = false;
        
        for( int i=0; i<numObjects; i++ ) {
            ObjectRecord *o = objects[i];
            
            int oID = o->id;

            if( ! humanMadeMap[ oID ] && 
                ! naturalMap[ oID ] &&
                ! unreachableMap[ oID ] ) {
                
                
                char assignedThisObject = false;
                
                
                SimpleVector<TransRecord*> *prodTrans = getAllProduces( oID );
                
                if( prodTrans != NULL ) {
                    
                    int numTrans = prodTrans->size();
                
                    
                    for( int t=0; t<numTrans; t++ ) {
                        TransRecord *trans = prodTrans->getElementDirect( t );
                        
                        int targetID = trans->target;
                        if( trans->actor == -1 && targetID > 0 ) {
                            // auto-decay
                            
                            if( humanMadeMap[ targetID ] ) {
                                humanMadeMap[ oID ] = true;
                                assignedThisObject = true;
                                anyAssigned = true;
                                break;
                                }
                            if( naturalMap[ targetID ] ) {
                                naturalMap[ oID ] = true;
                                assignedThisObject = true;
                                anyAssigned = true;
                                break;
                                }
                            }
                        }
                    }
                

                if( ! assignedThisObject ) {
                    anyUnknown = true;
                    }
                }
            }
        }
   

    /*
    // print a report    
    for( int i=0; i<numObjects; i++ ) {
        ObjectRecord *o = objects[i];
        
        if( humanMadeMap[ o->id ] ) {
            printf( "HUMAN-MADE:   " );
            }
        else if( naturalMap[ o->id ] ) {
            printf( "NATURAL:      " );
            }
        else {
            printf( "UNREACHABLE:  " );
            }
        
        printf( "%s\n", o->description );
        }
    */

    delete [] objects;
    delete [] naturalMap;
    delete [] unreachableMap;
    }





TransRecord *getTrans( int inActor, int inTarget, char inLastUseActor,
                       char inLastUseTarget ) {
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
            r->lastUseActor == inLastUseActor &&
            r->lastUseTarget == inLastUseTarget ) {
            return r;
            }
        }
    
    return NULL;
    }



TransRecord *getTransProducing( int inNewActor, int inNewTarget ) {
    int mapIndex = inNewTarget;
    
    if( mapIndex < 0 ) {
        mapIndex = inNewActor;
        }
    
    if( mapIndex < 0 ) {
        return NULL;
        }

    if( mapIndex >= mapSize ) {
        return NULL;
        }
    
    int numRecords = producesMap[mapIndex].size();
    
    for( int i=0; i<numRecords; i++ ) {
        
        TransRecord *r = producesMap[mapIndex].getElementDirect(i);
        
        if( r->newActor == inNewActor && r->newTarget == inNewTarget ) {
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



SimpleVector<TransRecord*> *getAllProduces( int inProducesID ) {
    if( inProducesID >= mapSize ) {
        return NULL;
        }

    return &( producesMap[inProducesID] );
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
        
        if( r->autoDecaySeconds != 0 ) {
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





static char *getFileName( int inActor, int inTarget, 
                          char inLastUseActor, char inLastUseTarget ) {
    const char *lastUseString = "";
    
    if( inLastUseActor && ! inLastUseTarget ) {
        lastUseString = "_LA";
        }
    else if( ! inLastUseActor && inLastUseTarget ) {
        lastUseString = "_LT";
        }
    else if( inLastUseActor && inLastUseTarget ) {
        lastUseString = "_LA_LT";
        }
    
    return autoSprintf( "%d_%d%s.txt", inActor, inTarget, lastUseString );
    }


static char *getOldFileName( int inActor, int inTarget, 
                             char inLastUseActor, char inLastUseTarget ) {
    const char *lastUseString = "";
    
    if( inLastUseActor && ! inLastUseTarget ) {
        lastUseString = "_LA";
        }
    else if( ! inLastUseActor && inLastUseTarget ) {
        lastUseString = "_L";
        }
    else if( inLastUseActor && inLastUseTarget ) {
        lastUseString = "_LA_LT";
        }
    
    return autoSprintf( "%d_%d%s.txt", inActor, inTarget, lastUseString );
    }



void addTrans( int inActor, int inTarget,
               int inNewActor, int inNewTarget,
               char inLastUseActor,
               char inLastUseTarget,
               char inReverseUseActor,
               char inReverseUseTarget,
               int inAutoDecaySeconds,
               float inActorMinUseFraction,
               float inTargetMinUseFraction,
               int inMove,
               int inDesiredMoveDist,
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
    TransRecord *t = getTrans( inActor, inTarget, 
                               inLastUseActor, inLastUseTarget );
    
    char writeToFile = false;
    

    if( t == NULL ) {
        t = new TransRecord;

        t->actor = inActor;
        t->target = inTarget;
        t->newActor = inNewActor;
        t->newTarget = inNewTarget;
        t->autoDecaySeconds = inAutoDecaySeconds;
        t->epochAutoDecay = 0;
        if( inAutoDecaySeconds < 0 ) {
            t->epochAutoDecay = -inAutoDecaySeconds;
            }
        
        t->lastUseActor = inLastUseActor;
        t->lastUseTarget = inLastUseTarget;

        t->reverseUseActor = inReverseUseActor;
        t->reverseUseTarget = inReverseUseTarget;
        
        t->actorMinUseFraction = inActorMinUseFraction;
        t->targetMinUseFraction = inTargetMinUseFraction;
        
        t->move = inMove;
        t->desiredMoveDist = inMove;
        
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
            t->targetMinUseFraction == inTargetMinUseFraction &&
            t->reverseUseActor == inReverseUseActor &&
            t->reverseUseTarget == inReverseUseTarget &&
            t->move == inMove &&
            t->desiredMoveDist == inDesiredMoveDist ) {
            
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
            t->epochAutoDecay = 0;
            if( t->autoDecaySeconds < 0 ) {
                t->epochAutoDecay = -inAutoDecaySeconds;
                }

            t->reverseUseActor = inReverseUseActor;
            t->reverseUseTarget = inReverseUseTarget;
            
            t->actorMinUseFraction = inActorMinUseFraction;
            t->targetMinUseFraction = inTargetMinUseFraction;
            
            t->move = inMove;
            t->desiredMoveDist = inDesiredMoveDist;
            
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

            char *fileName = getFileName( inActor, inTarget, 
                                          inLastUseActor, inLastUseTarget );
            

            File *transFile = transDir.getChildFile( fileName );
            


            // clear old-style file, which may be lingering
            char *oldFileName = getOldFileName( inActor, inTarget, 
                                             inLastUseActor, inLastUseTarget );
            

            File *oldTransFile = transDir.getChildFile( oldFileName );
            
            oldTransFile->remove();
            delete oldTransFile;
            delete [] oldFileName;
            

            int reverseUseActorFlag = 0;
            int reverseUseTargetFlag = 0;
            
            if( inReverseUseActor ) {
                reverseUseActorFlag = 1;
                }
            if( inReverseUseTarget ) {
                reverseUseTargetFlag = 1;
                }

            char *fileContents = autoSprintf( "%d %d %d %f %f %d %d %d %d", 
                                              inNewActor, inNewTarget,
                                              inAutoDecaySeconds,
                                              inActorMinUseFraction,
                                              inTargetMinUseFraction,
                                              reverseUseActorFlag,
                                              reverseUseTargetFlag,
                                              inMove,
                                              inDesiredMoveDist );

        
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
                          char inLastUseActor,
                          char inLastUseTarget,
                          char inNoWriteToFile ) {
    
    // one exists?
    TransRecord *t = getTrans( inActor, inTarget, 
                               inLastUseActor, inLastUseTarget );
    
    if( t != NULL ) {
        
        if( ! inNoWriteToFile ) {    
            File transDir( NULL, "transitions" );
            
            if( transDir.exists() && transDir.isDirectory() ) {
                
                char *fileName = getFileName( inActor, inTarget, 
                                              inLastUseActor,
                                              inLastUseTarget );
                
                char *oldFileName = getOldFileName( inActor, inTarget, 
                                                    inLastUseActor,
                                                    inLastUseTarget );
                
                File *transFile = transDir.getChildFile( fileName );
                File *oldTransFile = transDir.getChildFile( oldFileName );
                
                
                File *cacheFile = transDir.getChildFile( "cache.fcz" );
                
                cacheFile->remove();
                
                delete cacheFile;

                
                transFile->remove();
                delete transFile;
                delete [] fileName;
                

                oldTransFile->remove();
                delete oldTransFile;
                delete [] oldFileName;
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
    
    if( inTrans->lastUseActor ) {
        printf( " (lastUseActor)" );
        }
    if( inTrans->lastUseTarget ) {
        printf( " (lastUseTarget)" );
        }
    if( inTrans->reverseUseActor ) {
        printf( " (reverseUseActor)" );
        }
    if( inTrans->reverseUseTarget ) {
        printf( " (reverseUseTarget)" );
        }
    if( inTrans->move > 0 ) {
        const char *moveName = "none";
        
        switch( inTrans->move ) {
            case 1:
                moveName = "chase";
                break;
            case 2:
                moveName = "flee";
                break;
            case 3:
                moveName = "rand";
                break;
            case 4:
                moveName = "nor";
                break;
            case 5:
                moveName = "sou";
                break;
            case 6:
                moveName = "est";
                break;
            case 7:
                moveName = "wst";
                break;
            }
        

        printf( " (move=%s,dist=%d)", moveName, inTrans->desiredMoveDist );
        }
    
    printf( "\n" );
    }



int getObjectDepth( int inObjectID ) {
    if( inObjectID >= depthMapSize ) {
        return UNREACHABLE;
        }
    else {
        return depthMap[ inObjectID ];
        }
    }



char isHumanMade( int inObjectID ) {
    if( inObjectID >= humanMadeMapSize ) {
        return false;
        }
    else {
        return humanMadeMap[ inObjectID ];
        }
    }



void setTransitionEpoch( int inEpocSeconds ) {
    for( int i=0; i<records.size(); i++ ) {
        TransRecord *tr = records.getElementDirect(i);

        if( tr->epochAutoDecay > 0 ) {
            tr->autoDecaySeconds = tr->epochAutoDecay * inEpocSeconds;
            }
        }
    }
