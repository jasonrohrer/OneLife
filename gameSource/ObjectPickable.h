#ifndef OBJECT_PICKABLE_INCLUDED
#define OBJECT_PICKABLE_INCLUDED


#include "Pickable.h"


#include "objectBank.h"
#include "spriteBank.h"
#include "transitionBank.h"
#include "categoryBank.h"



#ifndef OHOL_NON_EDITOR

#include "EditorTransitionPage.h"

extern EditorTransitionPage *transPage;

#include "EditorAnimationPage.h"

extern EditorAnimationPage *animPage;


#include "EditorCategoryPage.h"

extern EditorCategoryPage *categoryPage;

static char deletePossible = true;
#else 

static char deletePossible = false;

#endif



static float getObjectFieldValue( ObjectRecord *inO,
                                  const char *inFieldName ) {
    if( strcmp( inFieldName, "mapp" ) == 0 ) {
        return inO->mapChance;
        }
    if( strcmp( inFieldName, "heat" ) == 0 ) {
        return inO->heatValue;
        }
    if( strcmp( inFieldName, "r" ) == 0 ) {
        return inO->rValue;
        }
    if( strcmp( inFieldName, "food" ) == 0 ) {
        return inO->foodValue;
        }
    if( strcmp( inFieldName, "speed" ) == 0 ) {
        return inO->speedMult;
        }
    if( strcmp( inFieldName, "slots" ) == 0 ) {
        return inO->numSlots;
        }
    if( strcmp( inFieldName, "slotsize" ) == 0 ) {
        return inO->slotSize;
        }
    if( strcmp( inFieldName, "locked" ) == 0 ) {
        return inO->slotsLocked;
        }
    if( strcmp( inFieldName, "tmstrch" ) == 0 ) {
        return inO->slotTimeStretch;
        }
    if( strcmp( inFieldName, "usedist" ) == 0 ) {
        return inO->useDistance;
        }
    if( strcmp( inFieldName, "deadlydist" ) == 0 ) {
        return inO->deadlyDistance;
        }
    if( strcmp( inFieldName, "pickupage" ) == 0 ) {
        return inO->minPickupAge;
        }
    if( strcmp( inFieldName, "bottom" ) == 0 ) {
        return inO->clothing == 'b';
        }
    if( strcmp( inFieldName, "backpack" ) == 0 ) {
        return inO->clothing == 'p';
        }
    if( strcmp( inFieldName, "shoe" ) == 0 ) {
        return inO->clothing == 's';
        }
    if( strcmp( inFieldName, "tunic" ) == 0 ) {
        return inO->clothing == 't';
        }
    if( strcmp( inFieldName, "hat" ) == 0 ) {
        return inO->clothing == 'h';
        }
    if( strcmp( inFieldName, "#use" ) == 0 ) {
        return inO->numUses;
        }
    if( strcmp( inFieldName, "handheld" ) == 0 ) {
        return inO->heldInHand;
        }
    if( strcmp( inFieldName, "rideable" ) == 0 ) {
        return inO->rideable;
        }
    if( strcmp( inFieldName, "blocking" ) == 0 ) {
        return inO->blocksWalking;
        }
    if( strcmp( inFieldName, "home" ) == 0 ) {
        return inO->homeMarker;
        }
    if( strcmp( inFieldName, "containable" ) == 0 ) {
        return inO->containable;
        }
    if( strcmp( inFieldName, "containsize" ) == 0 ) {
        return inO->containSize;
        }
    if( strcmp( inFieldName, "permanent" ) == 0 ) {
        return inO->permanent;
        }
    if( strcmp( inFieldName, "person" ) == 0 ) {
        return inO->person;
        }
    if( strcmp( inFieldName, "race" ) == 0 ) {
        return inO->race;
        }
    if( strcmp( inFieldName, "floor" ) == 0 ) {
        return inO->floor;
        }
    if( strcmp( inFieldName, "death" ) == 0 ) {
        return inO->deathMarker;
        }
    if( strcmp( inFieldName, "sideaccess" ) == 0 ) {
        return inO->sideAccess;
        }
    if( strcmp( inFieldName, "noflip" ) == 0 ) {
        return inO->noFlip;
        }
    if( strcmp( inFieldName, "nospawn" ) == 0 ) {
        return inO->personNoSpawn;
        }
    if( strcmp( inFieldName, "male" ) == 0 ) {
        return inO->male;
        }
    if( strcmp( inFieldName, "behind" ) == 0 ) {
        return inO->drawBehindPlayer;
        }
    if( strcmp( inFieldName, "hugFloor" ) == 0 ) {
        return inO->floorHugging;
        }
    return 0;
    }





class ObjectPickable : public Pickable {
        
    public:
        virtual void **search( const char *inSearch, 
                               int inNumToSkip, 
                               int inNumToGet, 
                               int *outNumResults, int *outNumRemaining ) {
            
            if( strcmp( inSearch, "." ) == 0 ) {
                int *stackResults = 
                    getStackRange( inNumToSkip, inNumToGet,
                                   outNumResults, outNumRemaining );
                void **results = new void *[ *outNumResults ];
                
                for( int i=0; i< *outNumResults; i++ ) {
                    results[i] = (void*)getObject( stackResults[i] );
                    }                

                delete [] stackResults;
                return results;
                }
            else {
                return (void**)searchObjects( inSearch, inNumToSkip, inNumToGet,
                                              outNumResults, outNumRemaining );
                }
            }
        
        


        virtual void draw( void *inItem, doublePair inPos ) {
            ObjectRecord *r = (ObjectRecord*)inItem;

            
            int maxD = getMaxDiameter( r );
            
            double zoom = 1;

            if( maxD > 64 ) {    
                zoom = 64.0 / maxD;
                }
            
            doublePair c = getObjectCenterOffset( r );
            // take off contained offset, since it doesn't apply here
            c.x -= r->containOffsetX;
            c.y -= r->containOffsetY;

            inPos = sub( inPos, mult( c, zoom ) );

            drawObject( r, 2, inPos, 0, false, false, 20, 0, false, false,
                        getEmptyClothingSet(), zoom );
            }



        virtual int getID( void *inItem ) {
            ObjectRecord *r = (ObjectRecord*)inItem;
            
            return r->id;
            }
        
        
        
        virtual void *getObjectFromID( int inID ) {
            ObjectRecord *o = getObject( inID, true );
            
            return (void*) o;
            }
        


        virtual char canDelete( int inID ) {
            return 
                deletePossible &&
                ! isObjectUsed( inID ) && 
                ! isObjectUsedInCategories( inID );
            }


        virtual FloatColor getTextColor( void *inItem ) {
            FloatColor c = { 0, 0, 0, 1 };

            ObjectRecord *r = (ObjectRecord*)inItem;
            
            if( r->mapChance > 0 ) {
                c.g = 0.5;
                }
            if( r->deadlyDistance > 0 ) {
                c.r = 0.75;
                }

            return c;
            }


        
        virtual void deleteID( int inID ) {
            Pickable::deleteID( inID );

            removeObjectFromAllCategories( inID );
            deleteObjectFromBank( inID );

            #ifndef OHOL_NON_EDITOR
            transPage->clearUseOfObject( inID );
            animPage->clearUseOfObject( inID );
            categoryPage->clearUseOfObject( inID );
            #endif
            
            for( int i=0; i<endAnimType; i++ ) {
                clearAnimation( inID, (AnimType)i );
                }
            }



        virtual const char *getText( void *inItem ) {
            ObjectRecord *r = (ObjectRecord*)inItem;
            
            return r->description;
            }
        



        // for implementing field=value search, like mapp=0.2
        // inFieldName will be all lower case
        virtual char isValidField( const char *inFieldName ) {
            
            if( strcmp( inFieldName, "mapp" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "heat" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "r" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "food" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "speed" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "slots" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "slotsize" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "locked" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "tmstrch" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "usedist" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "deadlydist" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "pickupage" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "bottom" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "backpack" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "shoe" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "tunic" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "hat" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "#use" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "handheld" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "rideable" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "blocking" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "home" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "containable" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "containsize" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "permanent" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "person" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "race" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "floor" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "death" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "sideaccess" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "noflip" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "nospawn" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "male" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "behind" ) == 0 ) {
                return true;
                }
            if( strcmp( inFieldName, "hugFloor" ) == 0 ) {
                return true;
                }

            return false;
            }
        
        

        // searches for objects with field matching value
        // inLessEqualGreater is -1, 0, or 1 if we want to find
        // objects that have fields that are less than, equal to, or greater
        // than inFieldValue, respectively.
        virtual void **search( const char *inFieldName,
                               float inFieldValue,
                               int inLessEqualGreater,
                               int inNumToSkip, 
                               int inNumToGet, 
                               int *outNumResults, int *outNumRemaining ) {
            
            int numObjects;
            ObjectRecord **allObjects =
                getAllObjects( &numObjects );
            
            SimpleVector<ObjectRecord*> matches;
            
            int matchIndex = 0;
            
            int remaining = 0;

            for( int i=numObjects - 1; i>=0; i-- ) {
            
                float thisValue = 
                    getObjectFieldValue( allObjects[i], inFieldName );

                if( ( inLessEqualGreater == -1 &&
                      thisValue < inFieldValue )
                    ||
                    // close enough for equals
                    ( inLessEqualGreater == 0 &&
                      fabs( thisValue - inFieldValue ) < 0.0001 )
                    ||
                    ( inLessEqualGreater == 1 &&
                      thisValue > inFieldValue ) ) {
                    
                    if( matchIndex >= inNumToSkip ) {
                        if( matches.size() < inNumToGet ) {
                            matches.push_back( allObjects[i] );
                            }
                        else {
                            remaining++;
                            }
                        }

                    matchIndex ++;
                    }
                }
            
            delete [] allObjects;

            int numResults = matches.size();


            void **returnArray = (void**)matches.getElementArray();
            
            *outNumResults = numResults;
            *outNumRemaining = remaining;
            
            return returnArray;
            };


    protected:
        

        virtual SimpleVector<int> *getStack() {
            return &sStack;
            }

        
        static SimpleVector<int> sStack;

    };
        

#endif
