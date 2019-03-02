#include "GridPos.h"

#include <math.h>

double distance( GridPos inA, GridPos inB ) {
    double dx = (double)inA.x - (double)inB.x;
    double dy = (double)inA.y - (double)inB.y;

    return sqrt(  dx * dx + dy * dy );
    }
