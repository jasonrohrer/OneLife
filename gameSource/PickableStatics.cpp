#include "SpritePickable.h"
#include "ObjectPickable.h"
#include "OverlayPickable.h"
#include "GroundPickable.h"

SimpleVector<int> SpritePickable::sStack;
SimpleVector<int> ObjectPickable::sStack;
SimpleVector<int> OverlayPickable::sStack;
SimpleVector<int> GroundPickable::sStack;


const char * GroundPickable::sStringNames[NUM_GROUND_STRING_NAMES] = { 
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",   
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",   
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29"
    };
