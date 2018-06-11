#pragma once
#include <string>
#include <vector>
#include "timer.h"

typedef uint16_t note;

namespace config {
    const double trainThresh = 0.05;
    const int numEmiss = 12;
    const int windowLen = 16;
}

class Matcher {
public:
    Matcher(std::vector<note> score, std::vector<double> transitionMat, std::vector<double> emissMat);
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
    int numStates;
    int currentPos;
    void load(std::string tfile, std::string efile, std::string sfile);
    void train(std::vector<note> obs);
};