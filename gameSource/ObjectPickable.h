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
        
        


        virtual void draw( void *inObject, doublePair inPos ) {
            ObjectRecord *r = (ObjectRecord*)inObject;

            
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



        virtual int getID( void *inObject ) {
            ObjectRecord *r = (ObjectRecord*)inObject;
            
            return r->id;
            }
        


        virtual char canDelete( int inID ) {
            return 
                deletePossible &&
                ! isObjectUsed( inID ) && 
                ! isObjectUsedInCategories( inID );
            }


        virtual FloatColor getTextColor( void *inObject ) {
            FloatColor c = { 0, 0, 0, 1 };

            ObjectRecord *r = (ObjectRecord*)inObject;
            
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



        virtual const char *getText( void *inObject ) {
            ObjectRecord *r = (ObjectRecord*)inObject;
            
            return r->description;
            }
        
    protected:
        

        virtual SimpleVector<int> *getStack() {
            return &sStack;
            }

        
        static SimpleVector<int> sStack;

    };
        

#endif
