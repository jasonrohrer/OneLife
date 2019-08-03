

void freeArcReport();


void reportArcEnd();

void stepArcReport();


// destroyed internally
char *getArcName();

void resetArcName();


double getArcRunningSeconds();


// returns -1 if there's no new milestone
int getArcYearsToReport( double inSecondsPerYear,
                         int inMilestoneYears = 100 );


