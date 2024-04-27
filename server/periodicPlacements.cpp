#include "periodicPlacements.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/system/Time.h"

#include "minorGems/system/Time.h"

#include "../gameSource/settingsToggle.h"



typedef struct PeriodicPlacementRecord {
        int objectID;
        int populationRadius;
        double lastPlacementTime;
        double placementIntervalSeconds;
    
        char *globalMessage;

        char live;        
        
    } PeriodicPlacementRecord;


static char enabled = false;


static SimpleVector <PeriodicPlacementRecord> records;



static double lastSettingReloadTime = 0;

// every 30 seconds
static double settingReloadInterval = 30;

// updates and marks live, or creates new
static void updateRecord( int inID, int inRadius, double inInterval,
                          char *inMessage ) {
    
    for( int i=0; i<records.size(); i++ ) {
        PeriodicPlacementRecord *r = records.getElement(i);
        
        if( r->objectID == inID ) {
            
            r->populationRadius = inRadius;
            r->placementIntervalSeconds = inInterval;
            
            delete [] r->globalMessage;
            
            r->globalMessage = inMessage;

            r->live = true;
            
            return;
            }
        }
    
    // not found
    // insert new
    PeriodicPlacementRecord r = { inID, inRadius, Time::getCurrentTime(),
                                  inInterval, inMessage, true };
    records.push_back( r );
    }




static void clearSettings() {
    for( int i=0; i<records.size(); i++ ) {
        delete [] records.getElement(i)->globalMessage;
        }

    records.deleteAll();
    }



static void reloadSettings() {
    enabled = 
        SettingsManager::getIntSetting( "allowPeriodicPlacements", 0 );
    
    // mark all existing ones as not live
    for( int i=0; i<records.size(); i++ ) {
        records.getElement(i)->live = false;
        }


    // now look at settings file again

    useContentSettings();
    
    char *cont = 
        SettingsManager::getSettingContents( "periodicPlacements", "" );
    
    useMainSettings();

    if( strcmp( cont, "" ) == 0 ) {
        delete [] cont;
        
        clearSettings();
        
        return;    
        }

    int numParts;
    char **parts = split( cont, "\n", &numParts );
    
    delete [] cont;
    
    for( int i=0; i<numParts; i++ ) {
        if( strcmp( parts[i], "" ) != 0 ) {
            /*
            39600.0 4998 2000 ORBITAL ALIGNMENT WITH ANOTHER PLANET DETECTED.**ROCKET CONSTRUCTION LOCATION IDENTIFIED.
            */

            double seconds;
            int id;
            int radius;
            
            int numRead = 
                sscanf( parts[i], "%lf %d %d", &seconds, &id, &radius );

            if( numRead == 3 ) {
                
                char *poundLocation = strstr( parts[i], "#" );
                
                char *message;
                
                if( poundLocation != NULL ) {
                    message = stringDuplicate( & poundLocation[1] );
                    }
                else {
                    message = stringDuplicate( "" );
                    }
                
                updateRecord( id, radius, seconds, message );
                
                }
            }

        delete [] parts[i];
        }
    delete [] parts;
    

    for( int i=0; i<records.size(); i++ ) {
        
        if( ! records.getElement(i)->live ) {
            // stale record not in settings anymore
            delete [] records.getElement(i)->globalMessage;
            records.deleteElement( i );
            i--;
            }
        }


    lastSettingReloadTime = Time::getCurrentTime();
    }


void initPeriodicPlacements() {
    reloadSettings();
    }



void freePeriodicPlacements() {
    clearSettings();
    }



PeriodicPlacementAction *stepPeriodicPlacements() {

    double curTime = Time::getCurrentTime();
    
    if( curTime - settingReloadInterval > 
        lastSettingReloadTime ){
    
        reloadSettings();
        }

    if( ! enabled ) {
        return NULL;
        }
    
    for( int i=0; i<records.size(); i++ ) {
        PeriodicPlacementRecord *r = records.getElement(i);
        
        if( curTime - r->lastPlacementTime > r->placementIntervalSeconds ) {
            
            // found one
            
            PeriodicPlacementAction *a = new PeriodicPlacementAction;
            
            a->objectID = r->objectID;
            a->populationRadius = r->populationRadius;
            a->globalMessage = stringDuplicate( r->globalMessage );
            
            r->lastPlacementTime = curTime;

            return a;
            }
        }
    

    return NULL;
    }

