#ifndef PICKABLE_INCLUDED
#define PICKABLE_INCLUDED


#include "minorGems/game/doublePair.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/util/SimpleVector.h"



// interface for a Picker plug in

class Pickable {
    public:

        virtual ~Pickable() {
            };

        
        // search for "." shows stack of recently used
        virtual void **search( const char *inSearch, 
                               int inNumToSkip, 
                               int inNumToGet, 
                               int *outNumResults, int *outNumRemaining ) = 0;
        
        

        virtual void draw( void *inObject, doublePair inPos ) = 0;


        virtual int getID( void *inObject ) = 0;

        // returns pointer to pickable object that matches inID
        // or NULL if no match
        virtual void *getObjectFromID( int inID ) = 0;
        

        virtual char canDelete( int inID ) = 0;

        virtual char isSearchable() {
            return true;
            }
        

        virtual FloatColor getTextColor( void *inObject ) {
            FloatColor c = { 0, 0, 0, 1 };
            return c;
            }
        

        // sub classed must call Pickable::deleteID()
        virtual void deleteID( int inID ) {
            getStack()->deleteElementEqualTo( inID );
            }
        
        
        
        // registers use, putting it on top of stack
        virtual void usePickable( int inID ) {
            SimpleVector<int> *stack = getStack();
            
            stack->deleteElementEqualTo( inID );
            stack->push_front( inID );
            }
        
        
        // not destroyed by caller
        virtual const char *getText( void *inObject ) = 0;
        


        
        // for field=value search, default implementations (where
        // such search isn't supported) are defined here.
        
        
        // for implementing field=value search, like mapp=0.2
        // inFieldName will be all lower case
        virtual char isValidField( const char *inFieldName ) {
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
            // note that in default implementation, we should
            // never be called, because isValidField will always
            // return false
            void **returnArray = new void *[0];
            
            *outNumResults = 0;
            *outNumRemaining = 0;
            
            return returnArray;
            };

        

    protected:
        
        // sub classes return their static stacks
        // (so that every picker shares the same stack)
        virtual SimpleVector<int> *getStack() = 0;
        
        
        virtual int *getStackRange( int inNumToSkip, int inNumToGet,
                                    int *outNumResults, int *outNumRemaining ) {
            
            SimpleVector<int> *stack = getStack();
            
            int numAvail = stack->size();
            

            if( numAvail - inNumToSkip < inNumToGet ) {
                inNumToGet = numAvail - inNumToSkip;
                *outNumRemaining = 0;
                }
            else {
                *outNumRemaining = numAvail - ( inNumToSkip + inNumToGet );
                }

            *outNumResults = inNumToGet;
            
            int *returnArray = new int[ inNumToGet ];
            
            int arrayI = 0;
            for( int i=inNumToSkip; i<inNumToSkip+inNumToGet; i++ ) {
                returnArray[ arrayI ] = stack->getElementDirect( i );
                arrayI++;
                }
            
            return returnArray;
            }
        
        
    };


#endif
        
