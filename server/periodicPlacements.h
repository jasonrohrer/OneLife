

void initPeriodicPlacements();


void freePeriodicPlacements();


typedef struct PeriodicPlacementAction {
        int objectID;
        int populationRadius;
        char *globalMessage;
    } PeriodicPlacementAction;


// NULL if nothing needs to be placed
// struct, and internal globalMessage char *, destroyed by caller
PeriodicPlacementAction *stepPeriodicPlacements();
