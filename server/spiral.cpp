#include "spiral.h"
#include "math.h"


// algorithm found here:
// https://math.stackexchange.com/a/163101
/*
// Original Pseudocode
function spiral(n)
        k=ceil((sqrt(n)-1)/2)
        t=2*k+1
        m=t^2 
        t=t-1
        if n>=m-t then return k-(m-n),-k        else m=m-t end
        if n>=m-t then return -k,-k+(m-n)       else m=m-t end
        if n>=m-t then return -k+(m-n),k else return k,k-(m-n-t) end
end
*/



GridPos getSpriralPoint( GridPos inCenter, int inSpriralIndex ) {
    int x, y;
    
    GridPos result;
    
    int n = inSpriralIndex;

    int k = ceil( ( sqrt(inSpriralIndex) - 1 ) / 2 );
    
    int t = 2 * k + 1;
    
    int m = t * t;

    t = t - 1;
    
    if( n >= m - t ) {
        x = k - ( m - n );
        y = -k;
        result.x = inCenter.x + x;
        result.y = inCenter.y + y;
        return result;
        }
    else { 
        m = m - t;
        }

    if( n >= m - t ) {
        x = -k;
        y = -k + ( m - n );
        result.x = inCenter.x + x;
        result.y = inCenter.y + y;
        return result;
        }
    else {
        m = m - t;
        }

    if( n >= m - t ) {
        x = -k + ( m - n );
        y = k;
        result.x = inCenter.x + x;
        result.y = inCenter.y + y;
        return result;
        }
    else {
        x = k;
        y = k - ( m - n - t );
        result.x = inCenter.x + x;
        result.y = inCenter.y + y;
        return result;
        }
    }

