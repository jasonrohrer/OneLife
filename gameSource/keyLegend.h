#ifndef KEY_LEGNED_INCLUDED
#define KEY_LEGNED_INCLUDED


#include "minorGems/util/SimpleVector.h"
#include "minorGems/game/doublePair.h"
#include "minorGems/game/Font.h"



typedef struct KeyLegend {
        SimpleVector <char> keys;
        
        // for keys that are not just ascii keyboard keys
        // these are NULL for most, but if coresponding key is \0,
        // then this can be non-null (example:  "Arrows" or "PgUp" "n/m").
        SimpleVector <const char *> keyClassNames;
        
        SimpleVector <const char *> descriptions;
    } KeyLegend;



void addKeyDescription( KeyLegend *inLegend,
                        char inKey,
                        const char *inDescription );


void addKeyClassDescription( KeyLegend *inLegend,
                             const char *inKeyClassName,
                             const char *inDescription );


void drawKeyLegend( KeyLegend *inLegend, doublePair inPos, 
                    TextAlignment inAlign = alignLeft );


#endif
