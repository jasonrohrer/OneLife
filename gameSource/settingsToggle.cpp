#include "settingsToggle.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/io/file/File.h"


const char *mainSettingsDir = "settings";
const char *contentSettingsDir = "contentSettings";

// this is used by server ONLY if it exists
// if not, regular "settings" folder is used
const char *serverSettingsDir = "serverSettings";


static char useServerSettings = false;



void useContentSettings() {
    SettingsManager::setDirectoryName( contentSettingsDir );
    }



void useMainSettings() {
    if( useServerSettings ) {
        SettingsManager::setDirectoryName( serverSettingsDir );
        }
    else {
        SettingsManager::setDirectoryName( mainSettingsDir );
        }
    }


void setUseServerSettings() {
    File testFile( NULL, serverSettingsDir );
    if( testFile.exists() && testFile.isDirectory() ) {
        useServerSettings = true;
        }
    else {
        useServerSettings = false;
        }
    }
