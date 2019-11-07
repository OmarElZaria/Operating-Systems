#include <assert.h>
#include "common.h"
#include "point.h"
#include <math.h>

void
point_translate(struct point *p, double x, double y)
{
    p->x = p->x + x;
    p->y = p->y + y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
    double x, y, distance, squareroot;
    
    x = (p2->x - p1->x) * (p2->x - p1->x);
    y = (p2->y - p1->y) * (p2->y - p1->y);
    
    distance = x + y;
    
    squareroot = sqrt(distance);
    
    return squareroot;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
    double x1, x2, y1, y2, distance1, squareroot1, distance2, squareroot2;
    
    x1 = p1->x * p1->x;
    y1 = p1->y * p1->y;
    
    distance1 = x1 + y1;
    
    squareroot1 = sqrt(distance1);
    
    x2 = p2->x * p2->x;
    y2 = p2->y * p2->y;
    
    distance2 = x2 + y2;
    
    squareroot2 = sqrt(distance2);
    
    if(squareroot1 < squareroot2)
        return -1;
    if(squareroot1 > squareroot2)
        return 1;
    
    return 0;
}
