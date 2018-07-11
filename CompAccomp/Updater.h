#pragma once
#include <string>
#include <fstream>
#include "curl/curl.h"

class Updater {
private:
    const std::string versionFileUrl = "";
    const std::string localVersionFile = "Versions.txt";
    bool newUpdater;
    bool newMain;
    CURL* curl;
    void updateSelf();
    void updateMain();
    static size_t writeData(void *contents, size_t size, size_t nmemb, void *userp);
public:
    Updater();
    ~Updater();
    void checkUpdates();
    
}