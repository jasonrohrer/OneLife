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
        
        

        virtual void draw( void *inItem, doublePair inPos ) = 0;


        virtual int getID( void *inItem ) = 0;

        // returns pointer to pickable object that matches inID
        // or NULL if no match
        virtual void *getItemFromID( int inID ) = 0;
        

        virtual char canDelete( int inID ) = 0;

        virtual char isSearchable() {
            return true;
            }
        

        virtual FloatColor getTextColor( void *inItem ) {
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
        virtual const char *getText( void *inItem ) = 0;
        


        
        // for field=value search, default implementations (where
        // such search isn't supported) are defined here.
        
        // inItem can be NULL
        // in that case, function can return an undefined value
        // But passing in NULL for inItem can still be used to check whether
        // a field name is a valid one (outFound should still be set)
        virtual float getItemFieldValue( void *inItem,
                                         const char *inFieldName,
                                         char *outFound ) {
            // no fields are valid by default
            *outFound = false;
            return 0;
            }
        


        // for implementing field=value search, like mapp=0.2
        // inFieldName will be all lower case
        virtual char isValidField( const char *inFieldName ) {
            char found = false;
            
            getItemFieldValue( NULL, inFieldName, &found );
            
            return found;
            }
        

        // return array destroyed by caller
        // 
        // default implementation, for Pickable implementations that
        // have no field search, returns an empty array
        virtual void **getAllItemsForFieldSearch( int *outNumItems ) {
            void **returnArray = new void*[0];
            *outNumItems = 0;
            
            return returnArray;
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
            
            int numItems;
            void **allItems =
                getAllItemsForFieldSearch( &numItems );
            
            SimpleVector<void*> matches;
            
            int matchIndex = 0;
            
            int remaining = 0;

            for( int i=numItems - 1; i>=0; i-- ) {
            
                char found;
                float thisValue = 
                    getItemFieldValue( allItems[i], inFieldName, &found );
                
                if( ! found ) {
                    continue;
                    }

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
                            matches.push_back( allItems[i] );
                            }
                        else {
                            remaining++;
                            }
                        }

                    matchIndex ++;
                    }
                }
            
            delete [] allItems;

            int numResults = matches.size();


            void **returnArray = matches.getElementArray();
            
            *outNumResults = numResults;
            *outNumRemaining = remaining;
            
            return returnArray;
            }

        

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
        
