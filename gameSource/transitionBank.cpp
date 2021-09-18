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
                int noUseActorFlag = 0;
                int noUseTargetFlag = 0;
                
                int move = 0;
                int desiredMoveDist = 1;
                
                sscanf( contents, "%d %d %d %f %f %d %d %d %d %d %d", 
                        &newActor, &newTarget, &autoDecaySeconds,
                        &actorMinUseFraction, &targetMinUseFraction,
                        &reverseUseActorFlag,
                        &reverseUseTargetFlag,
                        &move,
                        &desiredMoveDist,
                        &noUseActorFlag,
                        &noUseTargetFlag );
                
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

                r->actorChangeChance = 1.0f;
                r->targetChangeChance = 1.0f;
                r->newActorNoChange = -1;
                r->newTargetNoChange = -1;
                
                r->reverseUseActor = false;
                if( reverseUseActorFlag == 1 ) {
                    r->reverseUseActor = true;
                    }

                r->reverseUseTarget = false;
                if( reverseUseTargetFlag == 1 ) {
                    r->reverseUseTarget = true;
                    }


                r->noUseActor = false;
                if( noUseActorFlag == 1 ) {
                    r->noUseActor = true;
                    }

                r->noUseTarget = false;
                if( noUseTargetFlag == 1 ) {
                    r->noUseTarget = true;
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



typedef struct TransIDPair {
        int fromID;
        int toID;
        int noChangeID;
    } TransIDPair;
    


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

                CategoryRecord *parentCat = getCategory( parentID );
                
                if( parentCat->isPattern || parentCat->isProbabilitySet ) {
                    // generate pattern transitions later
                    // don't generate andy transitions for prob sets
                    continue;
                    }
                
                SimpleVector<TransRecord*> *parentTransOrig = 
                    getAllUses( parentID );

                if( parentTransOrig == NULL ) {
                    continue;
                    }
                
                // make copy of it
                // we add transitions below, and it may cause 
                // parentTrans to be reallocated internally
                SimpleVector<TransRecord *> parentTrans;
                
                parentTrans.push_back_other( parentTransOrig );
                
                
                int numParentTrans = parentTrans.size(); 
                for( int t=0; t<numParentTrans; t++ ) {
                    
                    TransRecord *tr = parentTrans.getElementDirect( t );
                    
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
                        if( newActor == parentID ) {
                            newActor = oID;
                            }
                        else if( newTarget == parentID ) {
                            // cross-case
                            // actor left on ground in place of target
                            newTarget = oID;
                            }
                        
                        // don't write to disk
                        addTrans( actor, target, newActor, newTarget,
                                  tr->lastUseActor,
                                  tr->lastUseTarget,
                                  tr->reverseUseActor,
                                  tr->reverseUseTarget,
                                  tr->noUseActor,
                                  tr->noUseTarget,
                                  tr->autoDecaySeconds,
                                  tr->actorMinUseFraction,
                                  tr->targetMinUseFraction, 
                                  tr->move,
                                  tr->desiredMoveDist,
                                  tr->actorChangeChance,
                                  tr->targetChangeChance,
                                  tr->newActorNoChange,
                                  tr->newTargetNoChange,
                                  true );
                        }

                    actor = tr->actor;
                    target = tr->target;
                    newActor = tr->newActor;
                    newTarget = tr->newTarget;
                    
                    if( target == parentID ) {
                        target = oID;
                        if( newTarget == parentID ) {
                            newTarget = oID;
                            }
                        else if( newActor == parentID ) {
                            // cross-case
                            // target ends up in hand
                            newActor = oID;
                            }
                        
                        // don't write to disk
                        addTrans( actor, target, newActor, newTarget,
                                  tr->lastUseActor,
                                  tr->lastUseTarget,
                                  tr->reverseUseActor,
                                  tr->reverseUseTarget,
                                  tr->noUseActor,
                                  tr->noUseTarget,
                                  tr->autoDecaySeconds,
                                  tr->actorMinUseFraction,
                                  tr->targetMinUseFraction, 
                                  tr->move,
                                  tr->desiredMoveDist,
                                  tr->actorChangeChance,
                                  tr->targetChangeChance,
                                  tr->newActorNoChange,
                                  tr->newTargetNoChange,
                                  true );
                        }

                    actor = tr->actor;
                    target = tr->target;
                    newActor = tr->newActor;
                    newTarget = tr->newTarget;
                    
                    if( actor == parentID && target == parentID ) {
                        actor = oID;
                        if( newActor == parentID ) {
                            newActor = oID;
                            }
                        target = oID;
                        if( newTarget == parentID ) {
                            newTarget = oID;
                            }
                        
                        // don't write to disk
                        addTrans( actor, target, newActor, newTarget,
                                  tr->lastUseActor,
                                  tr->lastUseTarget,
                                  tr->reverseUseActor,
                                  tr->reverseUseTarget,
                                  tr->noUseActor,
                                  tr->noUseTarget,
                                  tr->autoDecaySeconds,
                                  tr->actorMinUseFraction,
                                  tr->targetMinUseFraction, 
                                  tr->move,
                                  tr->desiredMoveDist,
                                  tr->actorChangeChance,
                                  tr->targetChangeChance,
                                  tr->newActorNoChange,
                                  tr->newTargetNoChange,
                                  true );
                        }
                    }
                }
            }
        
        delete [] objects;

        printf( "Auto-generated %d transitions based on categories\n", 
                records.size() - numRecords );
        
        numRecords = records.size();



        for( int i=0; i<numRecords; i++ ) {
            TransRecord *tr = records.getElementDirect( i );

            int transIDs[4] = { tr->actor, tr->target, 
                                tr->newActor, tr->newTarget };
            
            CategoryRecord *transCats[4] = { NULL, NULL, NULL, NULL };
            
            int patternSize = -1;
            int numPatternsInTrans = 0;
            
            for( int n=0; n<4; n++ ) {
                
                if( transIDs[n] > 0 ) {
                    transCats[n] = getCategory( transIDs[n] );
                    if( transCats[n] != NULL ) {
                        if( ! transCats[n]->isPattern ) {
                            transCats[n] = NULL;
                            }
                        if( transCats[n] != NULL && patternSize != -1 &&
                            transCats[n]->objectIDSet.size() != patternSize ) {
                            // doesn't match what's already set
                            transCats[n] = NULL;
                            }
                        else if( transCats[n] != NULL && patternSize == -1 ) {
                            // set it
                            patternSize = transCats[n]->objectIDSet.size();
                            }
                        if( transCats[n] != NULL ) {
                            numPatternsInTrans++;
                            }
                        }
                    }
                }
            
            // pattern only applies if at least one of actor or target
            // has pattern apply.
            // (don't fill out pattern if only newActor or newTarget have
            //  pattern apply, because this will just replace the master
            //  transition with final element of the pattern list)
            
            // is there any reason to require numPatternsInTrans >=2 ?
            // That is what the original code did, but I'm not sure.
            // As long as actor or target are a pattern, I think we're
            // okay applying one-pattern transitions.

            // for example, if a pink rose bush turns into a non-pattern object
            // (like a dead rose bush), then this SHOULD apply to all 
            // rose bushes that are part of the pattern.

            // Not requiring numPatternsInTrans >=2 also fixes the problem
            // with decay-to-nothing for dead dogs and puppies
            // (That German Shepherd transition only has one pattern object
            //  in it, because "nothing" is not a pattern.)
            
            if( numPatternsInTrans > 0 && 
                ( transCats[0] != NULL || transCats[1] != NULL ) ) {
                
                for( int p=0; p<patternSize; p++ ) {
                    int newTransIDs[4] = { tr->actor, tr->target, 
                                           tr->newActor, tr->newTarget };
                    
                    for( int n=0; n<4; n++ ) {
                        if( transCats[n] != NULL ) {
                            newTransIDs[n] = 
                                transCats[n]->objectIDSet.getElementDirect( p );
                            }
                        }

                    // don't replace explicitly-authored trans with an auto-
                    // generated one based on a pattern
                    // the authored trans trumps the pattern
                    TransRecord *existingTrans = getTrans( newTransIDs[0],
                                                           newTransIDs[1],
                                                           tr->lastUseActor,
                                                           tr->lastUseTarget );
                    if( existingTrans == NULL ) {    
                        // no authored trans exists

                        addTrans( newTransIDs[0],
                                  newTransIDs[1],
                                  newTransIDs[2],
                                  newTransIDs[3],
                                  tr->lastUseActor,
                                  tr->lastUseTarget,
                                  tr->reverseUseActor,
                                  tr->reverseUseTarget,
                                  tr->noUseActor,
                                  tr->noUseTarget,
                                  tr->autoDecaySeconds,
                                  tr->actorMinUseFraction,
                                  tr->targetMinUseFraction, 
                                  tr->move,
                                  tr->desiredMoveDist,
                                  tr->actorChangeChance,
                                  tr->targetChangeChance,
                                  tr->newActorNoChange,
                                  tr->newTargetNoChange,
                                  true );
                        }
                    
                    }
                }
            }
        

        
        printf( "Auto-generated %d transitions based on pattern categories\n", 
                records.size() - numRecords );
        
        numRecords = records.size();
        }



    
    // run twice
    // this generates both sides of transitions that have two generic uses
    // occurring
    for( int r=0; r<2; r++ )
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

                
                TransRecord *genericTrans[2];

                // use and last use
                genericTrans[0] = getTrans( oID, -1, false );
                genericTrans[1] = getTrans( oID, -1, true );
                
                
                for( int g=0; g<2; g++ )
                if( genericTrans[g] != NULL && 
                    genericTrans[g]->newTarget == 0 ) {
                    
                    char lastUseActor = genericTrans[g]->lastUseActor;
                    
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

                                newTrans.newActor = genericTrans[g]->newActor;
                                
                                transToAdd.push_back( newTrans );

                                numAdded++;
                                }
                            else {
                                // replace in-place with generic use result
                                tr->newActor = genericTrans[g]->newActor;
                                
                                numChanged ++;
                                }
                            }
                        else if( tr->target == oID && tr->newTarget == oID ) {
                            
                            // self-to-self target

                            // note that Generic Use transitions are
                            // always defined with actor and newActor
                            
                            // even though they can apply in situations
                            // where a target is experiencing generic use

                            if( lastUseActor ) {
                                // add a new one

                                TransRecord newTrans = *tr;
                                
                                newTrans.lastUseTarget = true;

                                newTrans.newTarget = genericTrans[g]->newActor;
                                
                                transToAdd.push_back( newTrans );

                                numAdded++;
                                }
                            else {
                                // replace in-place with generic use result
                                tr->newTarget = genericTrans[g]->newActor;
                                
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
                      tr.noUseActor,
                      tr.noUseTarget,
                      tr.autoDecaySeconds,
                      tr.actorMinUseFraction,
                      tr.targetMinUseFraction,
                      tr.move,
                      tr.desiredMoveDist,
                      tr.actorChangeChance,
                      tr.targetChangeChance,
                      tr.newActorNoChange,
                      tr.newTargetNoChange,
                      true );
            }
        

        printf( "Run %d:  "
                "Auto-modified %d transitions based generic use transitions "
                "and auto-added %d last use generic transitions "
                "(%d objects had generic use transitions defined).\n", 
                r, numChanged, numAdded, numGenerics );
        }
    



    if( autoGenerateUsedObjectTransitions ) {
        // new attempt:
        // process transitions not objects

        int numGenerated = 0;
        int numRemoved = 0;
        
        SimpleVector<TransRecord*> transToDelete;
        SimpleVector<TransRecord> transToAdd;
    
        for( int i=0; i<records.size(); i++ ) {
            TransRecord *tr = records.getElementDirect( i );
            ObjectRecord *actor = NULL;
            ObjectRecord *target = NULL;
            ObjectRecord *newActor = NULL;
            ObjectRecord *newTarget = NULL;
            
            float actorUseChance = 1.0f;
            float targetUseChance = 1.0f;
            
            if( tr->actor > 0 ) {
                actor = getObject( tr->actor );
                if( ! tr->noUseActor ) {
                    actorUseChance = actor->useChance;
                    }
                }
            if( tr->target > 0 ) {
                target = getObject( tr->target );
                if( ! tr->noUseTarget ) {
                    targetUseChance = target->useChance;
                    }
                }
            if( tr->newActor > 0 ) {
                newActor = getObject( tr->newActor );
                }
            if( tr->newTarget > 0 ) {
                newTarget = getObject( tr->newTarget );
                }

            TransRecord newTrans = *tr;
            newTrans.lastUseActor = false;
            newTrans.lastUseTarget = false;
            newTrans.reverseUseActor = false;
            newTrans.reverseUseTarget = false;
            newTrans.noUseActor = false;
            newTrans.noUseTarget = false;
            newTrans.actorMinUseFraction = 0.0f;
            newTrans.targetMinUseFraction = 0.0f;
            
            char processed = false;
            
            if( ! tr->lastUseTarget && ! tr->lastUseActor ) {

                char actorDecrement = false;
                SimpleVector<TransIDPair> actorSteps;

                if( actor != NULL && newActor != NULL 
                    &&
                    actor->numUses > 1 &&
                    actor->numUses == newActor->numUses ) {
                    
                    actorDecrement = true;
                    
                    int dir = -1;
                    if( tr->reverseUseActor ) {
                        dir = 1;
                        }
                    if( tr->noUseActor ) {
                        dir = 0;
                        TransIDPair tp = { actor->id, newActor->id };
                        actorSteps.push_back( tp );
                        }

                    for( int u=0; u<actor->numUses; u++ ) {
                        TransIDPair tp = { -1, -1 };
                        int uTo = u + dir;
                        
                        if( u < actor->numUses - 1 ) {
                            tp.fromID = actor->useDummyIDs[u];
                            }
                        else if( dir == -1 ) {
                            tp.fromID = actor->id;
                            }

                        if( uTo < actor->numUses - 1 &&
                            uTo >= 0 ) {
                            tp.toID = newActor->useDummyIDs[uTo];
                            if( u < actor->numUses - 1 ) {
                                tp.noChangeID = newActor->useDummyIDs[u];
                                }
                            else {
                                tp.noChangeID = newActor->id;
                                }
                            }
                        else if( uTo < 0 ) {
                            }
                        else if( uTo >= actor->numUses - 1 ) {
                            tp.toID = newActor->id;
                            tp.noChangeID = newActor->id;
                            }
                        
                        if( tp.fromID != -1 && tp.toID != -1 ) {
                            actorSteps.push_back( tp );
                            }
                        }
                    }
                else {
                    // default, no decrement
                 
                    if( actor != NULL &&
                        tr->reverseUseActor && 
                        newActor != NULL && newActor->numUses > 1 ) {

                        TransIDPair tp = { actor->id, 
                                           newActor->useDummyIDs[0] };
                        actorSteps.push_back( tp );
                        }
                    else {
                        // at least one
                        TransIDPair tp = 
                            { tr->actor, tr->newActor, tr->newActor };
                        actorSteps.push_back( tp );
                        
                        if( actor != NULL && actor->numUses > 1 ) {
                            // apply to all actor dummies
                            for( int u=0; u<actor->numUses-1; u++ ) {
                                float useFraction = 
                                    (float)( u+1 ) / (float)( actor->numUses );
                            
                                if( useFraction >= tr->actorMinUseFraction ) {
                                    
                                    tp.fromID = actor->useDummyIDs[u];
                                    actorSteps.push_back( tp );
                                    }
                                }
                            }
                        }
                    }
                
                char targetDecrement = false;
                SimpleVector<TransIDPair> targetSteps;

                if( target != NULL && newTarget != NULL 
                    &&
                    target->numUses > 1 &&
                    target->numUses == newTarget->numUses ) {
                    
                    targetDecrement = true;
                    
                    int dir = -1;
                    if( tr->reverseUseTarget ) {
                        dir = 1;
                        }
                    if( tr->noUseTarget ) {
                        dir = 0;
                        TransIDPair tp = { target->id, newTarget->id };
                        targetSteps.push_back( tp );
                        }
                    
                    for( int u=0; u<target->numUses; u++ ) {
                        TransIDPair tp = { -1, -1 };
                        int uTo = u + dir;
                        
                        if( u < target->numUses - 1 ) {
                            tp.fromID = target->useDummyIDs[u];
                            }
                        else if( dir == -1 ) {
                            tp.fromID = target->id;
                            }

                        if( uTo < target->numUses - 1 &&
                            uTo >= 0 ) {
                            tp.toID = newTarget->useDummyIDs[uTo];
                            if( u < target->numUses - 1 ) {
                                tp.noChangeID = newTarget->useDummyIDs[u];
                                }
                            else {
                                tp.noChangeID = newTarget->id;
                                }
                            }
                        else if( uTo < 0 ) {
                            }
                        else if( uTo >= target->numUses - 1 ) {
                            tp.toID = newTarget->id;
                            if( dir == 1 && tp.fromID != -1 ) {
                                // transition back to parent object
                                // if we have a use chance, it should
                                // apply here too, leaving us at last use dummy
                                // object if use chance doesn't happen.
                                tp.noChangeID = tp.fromID;
                                }
                            else {
                                tp.noChangeID = newTarget->id;
                                }
                            }
                        
                        if( tp.fromID != -1 && tp.toID != -1 ) {
                            targetSteps.push_back( tp );
                            }
                        }
                    }
                else {
                    // default

                    if( target != NULL &&
                        tr->reverseUseTarget && 
                        newTarget != NULL && newTarget->numUses > 1 ) {

                        TransIDPair tp = { target->id, 
                                           newTarget->useDummyIDs[0] };
                        targetSteps.push_back( tp );
                        }
                    else {
                        // at least one
                        TransIDPair tp = { tr->target, tr->newTarget, 
                                           tr->newTarget };
                        targetSteps.push_back( tp );
                        
                        if( target != NULL && target->numUses > 1 ) {
                            // apply to all target dummies
                            for( int u=0; u<target->numUses-1; u++ ) {
                                float useFraction = 
                                    (float)( u+1 ) / (float)( target->numUses );
                                
                                if( useFraction >= tr->targetMinUseFraction ) {
                                    tp.fromID = target->useDummyIDs[u];
                                    targetSteps.push_back( tp );
                                    }
                                }
                            }
                        }
                    }
                
                if( actorDecrement || targetDecrement ) {
                    
                    for( int as=0; as < actorSteps.size(); as++ ) {
                        TransIDPair ap = actorSteps.getElementDirect( as );
                        
                        for( int ts=0; ts < targetSteps.size(); ts++ ) {
                            TransIDPair tp = targetSteps.getElementDirect( ts );
                            
                            newTrans.actor = ap.fromID;
                            newTrans.newActor = ap.toID;
                            
                            newTrans.target = tp.fromID;
                            newTrans.newTarget = tp.toID;
                            
                            newTrans.actorChangeChance = actorUseChance;
                            newTrans.targetChangeChance = targetUseChance;
                            
                            newTrans.newActorNoChange = ap.noChangeID;
                            newTrans.newTargetNoChange = tp.noChangeID;

                            transToAdd.push_back( newTrans );
                            }
                        }
                    
                    if( ( actorDecrement && tr->reverseUseActor )
                        ||
                        ( targetDecrement && tr->reverseUseTarget ) ) {
                        // transition replaced
                        transToDelete.push_back( tr );
                        }
                    
                    processed = true;
                    }
                else {
                    // consider cross pass-through
                    
                    if( target != NULL && newActor != NULL 
                        &&
                        target->numUses > 1 &&
                        target->numUses == newActor->numUses ) {
                        // use preservation between target and new actor
                        
                        // generate one for each use dummy
                        for( int u=0; u<target->numUses-1; u++ ) {
                            newTrans.target = target->useDummyIDs[u];
                            newTrans.newActor = newActor->useDummyIDs[u];
                            
                            transToAdd.push_back( newTrans );
                            }
                        processed = true;
                        }
                    else if( actor != NULL && newTarget != NULL 
                        &&
                        actor->numUses > 1 &&
                        actor->numUses == newTarget->numUses ) {
                        // use preservation between actor and new target
                        
                        // generate one for each use dummy
                        for( int u=0; u<actor->numUses-1; u++ ) {
                            newTrans.actor = actor->useDummyIDs[u];
                            newTrans.newTarget = newTarget->useDummyIDs[u];
                            
                            transToAdd.push_back( newTrans );
                            }
                        processed = true;
                        }
                    // consider target straight pass through
                    // we preserve fraction of uses remaining
                    // (partially-picked carrot row seeds into that
                    //  many carrot flowers)
                    else if( target != NULL && newTarget != NULL &&
                             target->numUses > 1 && newTarget->numUses > 1 ) {
                        
                        // generate one for each use dummy
                        for( int u=0; u<target->numUses-1; u++ ) {
                            newTrans.target = target->useDummyIDs[u];

                            float useFraction = 
                                (float)( u ) / (float)( target->numUses );
                            // propagate used status to new target
                            int usesLeft = 
                                lrint( useFraction * newTarget->numUses );

                            if( usesLeft > newTarget->numUses - 2 ) {
                                usesLeft = newTarget->numUses - 2;
                                }
                            
                            newTrans.newTarget = 
                                newTarget->useDummyIDs[usesLeft];
                            
                            transToAdd.push_back( newTrans );
                            }
                        processed = true;
                        }
                    }    
                }
            
            
            if( ! processed ) {
                if( tr->lastUseActor || tr->lastUseTarget ) {
                                    
                    if( tr->lastUseActor && 
                        actor != NULL && 
                        actor->numUses > 1 ) {
                        
                        if( ! tr->reverseUseActor ) {
                            newTrans.actor = actor->useDummyIDs[0];
                            }
                        
                        transToAdd.push_back( newTrans );
                        
                        if( target != NULL && target->numUses > 1 ) {
                            // applies to every use dummy of target
                            
                            char mapNewTarget = false;
                            if( newTarget != NULL && 
                                target->numUses == newTarget->numUses ) {
                                mapNewTarget = true;
                                }

                            for( int u=0; u<target->numUses-1; u++ ) {
                                float useFraction = 
                                    (float)( u+1 ) / (float)( target->numUses );
                            
                                if( useFraction >= tr->targetMinUseFraction ) {

                                    newTrans.target = target->useDummyIDs[u];
                                
                                    if( mapNewTarget ) {
                                        // pass through
                                        newTrans.newTarget = 
                                            newTarget->useDummyIDs[u];
                                        }
                                    
                                    transToAdd.push_back( newTrans );
                                    }
                                }
                            }
                        }
                    if( tr->lastUseTarget && 
                        target != NULL && 
                        target->numUses > 1 ) {
                            
                        if( ! tr->reverseUseTarget ) {
                            newTrans.target = target->useDummyIDs[0];
                            }

                        transToAdd.push_back( newTrans );
                        
                        if( actor != NULL && actor->numUses > 1 ) {
                            // applies to every use dummy of actor
                            
                            char mapNewActor = false;
                            if( newActor != NULL && 
                                actor->numUses == newActor->numUses ) {
                                mapNewActor = true;
                                }
                            
                            for( int u=0; u<actor->numUses-1; u++ ) {
                                float useFraction = 
                                    (float)( u+1 ) / (float)( actor->numUses );
                            
                                if( useFraction >= tr->actorMinUseFraction ) {

                                    newTrans.actor = actor->useDummyIDs[u];

                                    if( mapNewActor ) {
                                        // pass through
                                        newTrans.newActor = 
                                            newActor->useDummyIDs[u];
                                        }
                                    
                                    transToAdd.push_back( newTrans );
                                    }
                                }
                            }
                        }
                    }
                else if( tr->reverseUseActor && 
                         newActor != NULL && newActor->numUses > 1 ) {
                    newTrans.newActor = newActor->useDummyIDs[0];
                    transToAdd.push_back( newTrans );
                    if( target != NULL && target->numUses > 1 ) {
                        // applies to every use dummy of target

                        char mapNewTarget = false;
                        if( newTarget != NULL && 
                            target->numUses == newTarget->numUses ) {
                            mapNewTarget = true;
                            }
                        
                        for( int u=0; u<target->numUses-1; u++ ) {
                            float useFraction = 
                                (float)( u+1 ) / (float)( target->numUses );
                            
                            if( useFraction >= tr->targetMinUseFraction ) {
                                newTrans.target = target->useDummyIDs[u];
                            
                                if( mapNewTarget ) {
                                    // pass through
                                    newTrans.newTarget = 
                                        newTarget->useDummyIDs[u];
                                    }
                                
                                transToAdd.push_back( newTrans );
                                }
                            }
                        }
                    }
                else if( tr->reverseUseTarget && 
                         newTarget != NULL && newTarget->numUses > 1 ) {
                    newTrans.newTarget = newTarget->useDummyIDs[0];
                    transToAdd.push_back( newTrans );
                    if( actor != NULL && actor->numUses > 1 ) {
                        // applies to every use dummy of actor

                        char mapNewActor = false;
                        if( newActor != NULL && 
                            actor->numUses == newActor->numUses ) {
                            mapNewActor = true;
                            }
                            
                        for( int u=0; u<actor->numUses-1; u++ ) {
                            float useFraction = 
                                (float)( u+1 ) / (float)( actor->numUses );
                            
                            if( useFraction >= tr->actorMinUseFraction ) {
                                newTrans.actor = actor->useDummyIDs[u];
                                
                                if( mapNewActor ) {
                                    // pass through
                                    newTrans.newActor = 
                                        newActor->useDummyIDs[u];
                                    }
                                
                                transToAdd.push_back( newTrans );
                                }
                            }
                        }
                    }
                else {
                    // consider trans that simply apply to
                    // all use dummies without using them further
                    // Example:  extracting a bowl full of berries
                    //           from a bush that is > 0.5 full, and bush
                    //           becomes empty.
                    
                    SimpleVector<int> actorDummies;
                    SimpleVector<int> targetDummies;
                    
                    if( actor != NULL && actor->numUses > 1 ) {
                        
                        for( int u=0; u<actor->numUses-1; u++ ) {
                            float useFraction = 
                                (float)( u+1 ) / (float)( actor->numUses );
                            
                            if( useFraction >= tr->actorMinUseFraction ) {
                                actorDummies.push_back( actor->useDummyIDs[u] );
                                }
                            }
                        }
                    else {
                        // default one
                        actorDummies.push_back( tr->actor );
                        }

                    if( target != NULL && target->numUses > 1 ) {
                        
                        for( int u=0; u<target->numUses-1; u++ ) {
                            float useFraction = 
                                (float)( u+1 ) / (float)( target->numUses );
                            
                            if( useFraction >= tr->targetMinUseFraction ) {
                                targetDummies.push_back( 
                                    target->useDummyIDs[u] );
                                }
                            }
                        }
                    else {
                        // default one
                        targetDummies.push_back( tr->target );
                        }

                    if( actorDummies.size() > 1 || targetDummies.size() > 1 ) {
                        for( int ad=0; ad<actorDummies.size(); ad++ ) {
                            newTrans.actor = 
                                actorDummies.getElementDirect( ad );
                            for( int td=0; td<targetDummies.size(); td++ ) {
                                newTrans.target = 
                                    targetDummies.getElementDirect( td );
                                transToAdd.push_back( newTrans );
                                }
                            }
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
                      newTrans->noUseActor,
                      newTrans->noUseTarget,
                      newTrans->autoDecaySeconds,
                      newTrans->actorMinUseFraction,
                      newTrans->targetMinUseFraction,
                      newTrans->move,
                      newTrans->desiredMoveDist,
                      newTrans->actorChangeChance,
                      newTrans->targetChangeChance,
                      newTrans->newActorNoChange,
                      newTrans->newTargetNoChange,
                      true );
            numGenerated++;
            }

        printf( "Auto-generated %d transitions based on used objects, "
                "%d removed in the process.\n", 
                numGenerated, numRemoved );
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
                      newTrans->noUseActor,
                      newTrans->noUseTarget,
                      newTrans->autoDecaySeconds,
                      newTrans->actorMinUseFraction,
                      newTrans->targetMinUseFraction,
                      newTrans->move,
                      newTrans->desiredMoveDist,
                      newTrans->actorChangeChance,
                      newTrans->targetChangeChance,
                      newTrans->newActorNoChange,
                      newTrans->newTargetNoChange,
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


#include "objectMetadata.h"


#define NUM_CHANCE_RECORDS 100
static int nextChanceRecord;
static TransRecord chanceRecords[NUM_CHANCE_RECORDS];


#include "minorGems/util/random/CustomRandomSource.h"
static CustomRandomSource randSource;


TransRecord *getPTrans( int inActor, int inTarget, 
                        char inLastUseActor,
                        char inLastUseTarget ) {
    
    int actorMeta = extractMetadataID( inActor );
    int targetMeta = extractMetadataID( inTarget );

    if( actorMeta != 0 )
        inActor = extractObjectID( inActor );

    if( targetMeta != 0 )
        inTarget = extractObjectID( inTarget );


    TransRecord *r = getTrans( inActor, inTarget, 
                               inLastUseActor, inLastUseTarget );
    
    if( r == NULL ) {
        return r;
        }
    
    char actorProbSet = isProbabilitySet( r->newActor );
    char targetProbSet = isProbabilitySet( r->newTarget );

    if( r->actorChangeChance == 1.0f && r->targetChangeChance == 1.0f &&
        actorMeta == 0 && targetMeta == 0 
        && ! actorProbSet && ! targetProbSet ) {
        return r;
        }
    
    TransRecord *rStatic = &( chanceRecords[ nextChanceRecord ] );
    
    nextChanceRecord++;
    if( nextChanceRecord >= NUM_CHANCE_RECORDS ) {
        nextChanceRecord = 0;
        }
    
    *rStatic = *r;
    
    if( r->actorChangeChance != 1.0f ) {
        if( randSource.getRandomBoundedDouble( 0, 1.0 ) >
            r->actorChangeChance ) {
            // no change to actor
            rStatic->newActor = rStatic->newActorNoChange;
            }
        }
    if( r->targetChangeChance != 1.0f ) {
        if( randSource.getRandomBoundedDouble( 0, 1.0 ) >
            r->targetChangeChance ) {
            // no change to target
            rStatic->newTarget = rStatic->newTargetNoChange;
            }
        }

    if( actorProbSet ) {
        rStatic->newActor = pickFromProbSet( rStatic->newActor );
		
		ObjectRecord *actor = getObject(rStatic->actor);
		ObjectRecord *newActor = getObject(rStatic->newActor);
		if (actor != NULL && actor->isUseDummy && newActor != NULL && newActor->numUses > 1) {
			ObjectRecord *actorParent = getObject(actor->useDummyParent);
			if (actorParent != NULL && actorParent->numUses == newActor->numUses) {
				rStatic->newActor = newActor->useDummyIDs[actor->thisUseDummyIndex];
				}
			}
        
        }
    if( targetProbSet ) {
        rStatic->newTarget = pickFromProbSet( rStatic->newTarget );
		
		ObjectRecord *target = getObject(rStatic->target);
		ObjectRecord *newTarget = getObject(rStatic->newTarget);
		if (target != NULL && target->isUseDummy && newTarget != NULL && newTarget->numUses > 1) {
			ObjectRecord *targetParent = getObject(target->useDummyParent);
			if (targetParent != NULL && targetParent->numUses == newTarget->numUses) {
				rStatic->newTarget = newTarget->useDummyIDs[target->thisUseDummyIndex];
				}
			}
		
        }
    
    
    int passThroughMeta = actorMeta;
    
    if( actorMeta == 0 ) {
        passThroughMeta = targetMeta;
        }

    if( passThroughMeta != 0 ) {
        if( rStatic->newActor > 0 &&
            getObject( rStatic->newActor )->mayHaveMetadata ) {
            rStatic->newActor = packMetadataID( rStatic->newActor, 
                                                passThroughMeta );
            }
        else if( rStatic->newTarget > 0 &&
                 getObject( rStatic->newTarget )->mayHaveMetadata ) {
            rStatic->newTarget = packMetadataID( rStatic->newTarget, 
                                                 passThroughMeta );
            }
        }
    
    return rStatic;
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
        *outNumResults = 0;
        *outNumRemaining = 0;
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



// returns true if inCatID is NOT a pattern, too
static char isTransPartOfPattern( int inCatID, TransRecord *inTrans ) {
    CategoryRecord *catRec = getCategory( inCatID );
    
    if( catRec == NULL ) {
        return true;
        }
    if( ! catRec->isPattern ) {
        return true;
        }

    int patternSize = catRec->objectIDSet.size();
    
    CategoryRecord *actorCat = NULL;
    CategoryRecord *targetCat = NULL;
    
    if( inTrans->actor > 0 ) {
        actorCat = getCategory( inTrans->actor );
        }
    if( inTrans->target > 0 ) {
        targetCat = getCategory( inTrans->target );
        }
    
    char actorInPattern = false;
    char targetInPattern = false;
    
    if( actorCat != NULL &&
        actorCat->isPattern &&
        actorCat->objectIDSet.size() == patternSize ) {
        actorInPattern = true;
        }
    else if( targetCat != NULL &&
             targetCat->isPattern &&
             targetCat->objectIDSet.size() 
             == patternSize ) {
        targetInPattern = true;
        }
    
    if( !actorInPattern && !targetInPattern ) {
        return false;
        }
    return true;
    }



static TransRecord **searchWithCategories( 
    SimpleVector<TransRecord *> inMapToSearch[],
    int inID, 
    int inNumToSkip, 
    int inNumToGet, 
    int *outNumResults, int *outNumRemaining ) {


    // don't completely block results if we are outside bounds of map
    // map is created based on objects that have transitions
    // if we have an high-ID object that has no transitions itself
    // but is part of a category, it may be off the map
    int numRecords = 0;
    if( inID < mapSize ) {
        numRecords = inMapToSearch[inID].size();
        }


    ReverseCategoryRecord *catRec = getReverseCategory( inID );

    int extraRecords = 0;

    if( catRec != NULL ) {
        for( int i=0; i< catRec->categoryIDSet.size(); i++ ) {
            int catID = catRec->categoryIDSet.getElementDirect( i );
            if( catID < mapSize ) {
                
                for( int j=0; j<inMapToSearch[ catID ].size(); j++ ) {
                    if( isTransPartOfPattern( 
                            catID, 
                            inMapToSearch[ catID ].getElementDirect( j ) ) ) {
                        
                        extraRecords ++;
                        }
                    }
                }
            }
        }
    
    
    TransRecord **initialResult = 
        search( inMapToSearch, inID, inNumToSkip, inNumToGet,
                outNumResults, outNumRemaining );
    if( inNumToGet == *outNumResults || extraRecords == 0 ) {
        *outNumRemaining += extraRecords;
        return initialResult;
        }
    else if( extraRecords > 0 ) {
        // ran out of main results, need to go into category results

        
        SimpleVector<TransRecord *> results;
        for( int r=0; r<*outNumResults; r++ ) {
            results.push_back( initialResult[r] );
            }
        
        if( initialResult != NULL ) {
            delete [] initialResult;
            }
        
        inNumToSkip -= numRecords;
        if( inNumToSkip < 0 ) {
            inNumToSkip = 0;
            }
        
        int numRemaining = extraRecords - inNumToSkip;
        int i = 0;
        
        while( i < catRec->categoryIDSet.size() &&
               *outNumResults < inNumToGet ) {
            int numLeftToGet = inNumToGet - *outNumResults;

            int catID = catRec->categoryIDSet.getElementDirect( i );
            int catTransSize = 0;
            if( catID < mapSize ) {
                
                catTransSize = inMapToSearch[ catID ].size();
                }
            
            int catNumResults = 0;
            int catNumRemaining = 0;
            
            TransRecord **catResult = 
                search( inMapToSearch, catID, 
                        inNumToSkip, numLeftToGet,
                        &catNumResults, &catNumRemaining );
            if( catResult != NULL ) {
                for( int r=0; r<catNumResults; r++ ) {
                    
                    // if cat is a pattern...
                    // make sure at least one of actor or target 
                    // is also part of pattern, else pattern does not apply
                    if( ! isTransPartOfPattern( catID, catResult[r] ) ) {
                        ( *outNumResults ) --;
                        continue;
                        }
                    
                    results.push_back( catResult[r] );
                    }
                delete [] catResult;
                *outNumResults += catNumResults;

                numRemaining -= catNumResults;                
                }
            
            inNumToSkip -= catTransSize;
            if( inNumToSkip < 0 ) {
                inNumToSkip = 0;
                }
            
            i++;
            }
        *outNumResults = results.size();
        *outNumRemaining = numRemaining;
        return results.getElementArray();
        }
    else {
        return NULL;
        }




    }



TransRecord **searchUses( int inUsesID, 
                          int inNumToSkip, 
                          int inNumToGet, 
                          int *outNumResults, int *outNumRemaining ) {

    if( autoGenerateCategoryTransitions ) {
        return search( usesMap, inUsesID, inNumToSkip, inNumToGet,
                       outNumResults, outNumRemaining );
        }
    else {
        return searchWithCategories( usesMap, inUsesID, inNumToSkip, inNumToGet,
                                     outNumResults, outNumRemaining );
        }
    }



TransRecord **searchProduces( int inProducesID, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining ) {
    
    if( autoGenerateCategoryTransitions ) {        
        return search( producesMap, inProducesID, inNumToSkip, inNumToGet,
                       outNumResults, outNumRemaining );
        }
    else {
        return searchWithCategories(
            producesMap, inProducesID, inNumToSkip, inNumToGet,
            outNumResults, outNumRemaining );
        }
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
               char inNoUseActor,
               char inNoUseTarget,
               int inAutoDecaySeconds,
               float inActorMinUseFraction,
               float inTargetMinUseFraction,
               int inMove,
               int inDesiredMoveDist,
               float inActorChangeChance,
               float inTargetChangeChance,
               int inNewActorNoChange,
               int inNewTargetNoChange,
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
        
        // instead of adding just one new map element at a time
        // we know that more are coming, because they are added in batches
        // But doubling each time will waste a lot of space at the end
        // Can speed it up by a factor of X by adding X each time instead of 1
        // speed it up by 100x
        // This will waste at most space for 100 elements in last expansion.
        int newMapSize = max + 100;
        
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

        t->noUseActor = inNoUseActor;
        t->noUseTarget = inNoUseTarget;
        
        t->actorMinUseFraction = inActorMinUseFraction;
        t->targetMinUseFraction = inTargetMinUseFraction;
        
        t->move = inMove;
        t->desiredMoveDist = inDesiredMoveDist;
        
        t->actorChangeChance = inActorChangeChance;
        t->targetChangeChance = inTargetChangeChance;

        t->newActorNoChange = inNewActorNoChange;
        t->newTargetNoChange = inNewTargetNoChange;
        

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
            t->noUseActor == inNoUseActor &&
            t->noUseTarget == inNoUseTarget &&
            t->move == inMove &&
            t->desiredMoveDist == inDesiredMoveDist &&
            t->actorChangeChance == inActorChangeChance &&
            t->targetChangeChance == inTargetChangeChance &&
            t->newActorNoChange == inNewActorNoChange &&
            t->newTargetNoChange == inNewTargetNoChange ) {
            
            // no change to produces map either... 

            // don't need to change record at all, done.
            }
        else {
            
            // remove record from producesMaps
            
            if( t->newActor != 0 &&
                t->newActor != inNewActor ) {
                producesMap[t->newActor].deleteElementEqualTo( t );
                }
            if( t->newTarget != 0 &&
                t->newTarget != inNewTarget ) {                
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

            t->noUseActor = inNoUseActor;
            t->noUseTarget = inNoUseTarget;
            
            t->actorMinUseFraction = inActorMinUseFraction;
            t->targetMinUseFraction = inTargetMinUseFraction;
            
            t->move = inMove;
            t->desiredMoveDist = inDesiredMoveDist;
            
            t->actorChangeChance = inActorChangeChance;
            t->targetChangeChance = inTargetChangeChance;

            t->newActorNoChange = inNewActorNoChange;
            t->newTargetNoChange = inNewTargetNoChange;

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

            int noUseActorFlag = 0;
            int noUseTargetFlag = 0;
            
            if( inNoUseActor ) {
                noUseActorFlag = 1;
                }
            if( inNoUseTarget ) {
                noUseTargetFlag = 1;
                }

            
            // don't save change chance to file
            // it's only for auto-generated transitions

            char *fileContents = autoSprintf( "%d %d %d %f %f %d %d "
                                              "%d %d %d %d", 
                                              inNewActor, inNewTarget,
                                              inAutoDecaySeconds,
                                              inActorMinUseFraction,
                                              inTargetMinUseFraction,
                                              reverseUseActorFlag,
                                              reverseUseTargetFlag,
                                              inMove,
                                              inDesiredMoveDist,
                                              noUseActorFlag,
                                              noUseTargetFlag );

        
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
    
    // first, see if this decays into a grave in one step

    TransRecord *trans = getTrans( -1, inObjectID );
    
    if( trans != NULL && trans->newTarget > 0 ) {
        
        ObjectRecord *nextR = getObject( trans->newTarget );

        if( nextR != NULL && nextR->deathMarker ) {
            return true;
            }
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
    if( inTrans->noUseActor ) {
        printf( " (noUseActor)" );
        }
    if( inTrans->noUseTarget ) {
        printf( " (noUseTarget)" );
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
            case 8:
                moveName = "find";
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
