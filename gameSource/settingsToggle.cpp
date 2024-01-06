#include "settingsToggle.h"

#include "minorGems/util/SettingsManager.h"


const char *mainSettingsDir = "settings";
const char *contentSettingsDir = "contentSettings";



void useContentSettings() {
    SettingsManager::setDirectoryName( contentSettingsDir );
    }



void useMainSettings() {
    SettingsManager::setDirectoryName( mainSettingsDir );
    }
