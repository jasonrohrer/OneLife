#include "buttonStyle.h"


void setButtonStyle( Button *inButton ) {
    inButton->setHoverColor( 1, 1, 1, 0.5 );
    inButton->setNoHoverColor( 0, 0, 0, 1 );
    inButton->setDragOverColor( 1, 1, 0, 1 );

    inButton->setFillColor( 1, 0.75, 0.2, 1 );
    
    inButton->setDragOverFillColor( 0, 0, 0, 1 );
    
    inButton->setBracketCoverLength( 16 );
    

    inButton->setHoverBorderColor( 1, 1, 1, 1 );
    
    doublePair shift = { 0, -2 };
    
    inButton->setContentsShift( shift );
    }

