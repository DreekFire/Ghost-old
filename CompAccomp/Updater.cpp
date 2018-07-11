#include "Updater.h"

Updater::Updater() {
    curl = curl_easy_init();
}

Updater::~Updater() {
    curl_easy_cleanup(curl);
}

void Updater::updateSelf() {

}

void Updater::updateMain() {

}

void Updater::checkUpdates() {
    std::string updaterVer;
    std::string mainVer;
    fstream localVer(localVersionFile);
    getline(localVer, updaterVer);
    getline(localVer, mainVer);
    localVer.close();
    curl_easy_setopt(curl, CURLOPT_URL, "versionFileUrl");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    res = curl_easy_perform(curl);
}



static size_t Updater::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{ 
    size_t realsize = size * nmemb;
    readBuffer.append(contents, realsize);
    return realsize;
}