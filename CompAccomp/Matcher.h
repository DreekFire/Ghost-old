#pragma once
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include "timer.h"

typedef uint16_t note;

namespace config {
    //optional optimizations
    bool compressEmiss = false;
    bool compressTrans = true;
    bool compressTransStorage = true;
    //internal config
    const double trainThresh = 0.1;
    const int numEmiss = 12;
    const int windowFront = 4;
    const int windowRear = 8;
    const int windowLength = windowFront + windowRear;
}

class Matcher {
public:
    Matcher(std::vector<double> transitionMat, std::vector<double> emissionMat, std::vector<note> score);
    ~Matcher();
    int viterbi();
private:
    std::vector<double> delta;
    std::vector<double> transMat;
    std::vector<double> emissMat;
    std::vector<double> pi;
    std::vector<int> psi;
    std::vector<note> sequence;
    std::vector<note> score;
    std::string saveFile;
    double mu;
    int numStates;
    int numObs;
    int currentPos;
    void train(std::vector<note> obs);
    void save(std::string file, std::vector<double> pi, std::vector<double> tMat, std::vector<double> eMat);
    double tProb(int y, int x);
};