

// switches back and forth between content and main settings folders

void useContentSettings();

void useMainSettings();


// called once by server at startup
// sets a preference for a "serverSettings" folder, if it exists, for
// main settings.
// if it doesn't exist, then regular "settings" folder is used for main settings
void setUseServerSettings();
