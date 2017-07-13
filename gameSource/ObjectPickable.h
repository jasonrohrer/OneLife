#ifndef OBJECT_PICKABLE_INCLUDED
#define OBJECT_PICKABLE_INCLUDED


#include "Pickable.h"


#include "objectBank.h"
#include "spriteBank.h"
#include "transitionBank.h"
#include "categoryBank.h"


#include "EditorTransitionPage.h"

extern EditorTransitionPage *transPage;

#include "EditorAnimationPage.h"

extern EditorAnimationPage *animPage;


#include "EditorCategoryPage.h"

extern EditorCategoryPage *categoryPage;




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
            
            inPos = sub( inPos, mult( getObjectCenterOffset( r ), zoom ) );

            drawObject( r, 2, inPos, 0, false, false, 20, 0, false, false,
                        getEmptyClothingSet(), zoom );
            }



        virtual int getID( void *inObject ) {
            ObjectRecord *r = (ObjectRecord*)inObject;
            
            return r->id;
            }
        


        virtual char canDelete( int inID ) {
            return 
                ! isObjectUsed( inID ) && 
                ! isObjectUsedInCategories( inID );
            }
        

        
        virtual void deleteID( int inID ) {
            Pickable::deleteID( inID );

            removeObjectFromAllCategories( inID );
            deleteObjectFromBank( inID );

            transPage->clearUseOfObject( inID );
            animPage->clearUseOfObject( inID );
            categoryPage->clearUseOfObject( inID );

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
