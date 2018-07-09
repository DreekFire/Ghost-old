#pragma once
#include <string>
#include <chrono>
#include "intrin.h" // not cross-platform, replace later
#include "Listener.h"
#include "Player.h"

namespace config {
    //optional optimizations
    bool compressEmiss = false;
    bool compressTrans = true; // "false" not implemented yet
    bool compressTransStorage = true; // storage not implemented yet
    //internal config
    const double trainThresh = 0.1;
    const double gaussianSigma = 0.64;
    const int numEmiss = 12;
    const int windowFront = 4;
    const int windowRear = 8;
    const int windowLength = windowFront + windowRear;
}

class Matcher { //todo: make sure Matcher correctly includes ghost states
public:
    Matcher(std::vector<double> transitionMat, std::vector<double> emissionMat, std::vector<note> score);
    ~Matcher();
    void update(note o);
private:
    Listener input;
    Player output;
    std::vector<double> delta;
    std::vector<double> transMat;
    std::vector<double> emissMat;
    std::vector<double> pi;
    std::vector<double> gaussian; //gaussian kernel folded in half
    std::vector<int> psi;
    std::vector<note> sequence;
    std::vector<note> score;
    std::string saveFile;
    std::chrono::steady_clock timer;
    int numStates;
    int numObs;
    int currentPos;
    int noteStartPos;
    double mu;
    double msPerTick;
    std::chrono::time_point<std::chrono::steady_clock> noteStartTime;
    void train(std::vector<note> obs);
    void save(std::string file, std::vector<double> pi, std::vector<double> tMat, std::vector<double> eMat);
    void loadText(std::string file);
    void loadMidi(std::string file);
    int viterbi();
    double tProb(int state1, int state2);
    double eProb(int state, int emiss);
    double calcMu(std::vector<double> tMat, int N);
};