#include "minorGems/io/file/Path.h"
#include "minorGems/util/log/AppLog.h"

#include <cstdlib>

static char *logPath;
static char *dbPath;
static char *settingsPath;

void readEnv() {
    if ( getenv("OHOL_LOG_PATH") ){
        logPath = getenv("OHOL_LOG_PATH");
        AppLog::infoF( "Using %s as log path", logPath );
        }

    if ( getenv("OHOL_DB_PATH") ){
        dbPath = getenv("OHOL_DB_PATH");
        AppLog::infoF( "Using %s as db path", dbPath );
        }

    if ( getenv("OHOL_SETTINGS_PATH") ){
        settingsPath = getenv("OHOL_SETTINGS_PATH");
        AppLog::infoF( "Using %s as settings path", settingsPath );
        }
}

Path *getEnvLogPath() {
    if ( logPath ) {
        return new Path( logPath );
        }
    else {
        return NULL;
        }
    }

Path *getEnvDBPath() {
    if ( dbPath ) {
        return new Path( dbPath );
        }
    else {
        return NULL;
        }
    }

Path *getEnvSettingsPath() {
    if ( settingsPath ) {
        return new Path( settingsPath );
        }
    else {
        return NULL;
        }
    }
