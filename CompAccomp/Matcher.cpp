#include "Matcher.h"

Matcher::Matcher(std::vector<double> transitionMat, std::vector<double> emissionMat, std::vector<note> scoreMat) {
    transMat = transitionMat;
    emissMat = emissionMat;
    score = scoreMat;
}

Matcher::~Matcher() {
}

void Matcher::update(note o) {
    auto currentTime = timer.now();
    sequence.push_back(o);
    int newPos = viterbi();
    if (newPos == currentPos + 2) {
        if (score[newPos] != score[currentPos]) {
            msPerTick = 2 * std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - noteStartTime).count() / (newPos - noteStartPos);
            noteStartTime = currentTime;
        }
    }
    else {
        noteStartTime = currentTime;
    }
    noteStartPos = currentPos;
}

int Matcher::viterbi() {
    //initialization
    for (int i = 0; i < numStates; i++) {
        delta[i] = pi[i] * emissMat[i * 12 + sequence[0]];
        psi[i] = 0;
    }
    //recursion
    double max1 = 0;
    double max2 = 0;
    for (int t = 1; t < numObs; t++) {
        for (int j = 0; j < numStates; j++) {
            double val = delta[t*numStates + j] * mu;
            if(val > max2) {
                max2 = val;
            }
        }
        for (int i = 0; i < numStates; i++) {
            for (int j = currentPos - config::windowFront; j < currentPos + config::windowRear; j++) {
                double current = delta[(t - 1) * numStates + j] * tProb(j, i);
                if (current > max1) {
                    psi[t * numStates + i] = j;
                    max1 = current * emissMat[i * numStates + sequence[t]];
                }
            }
            delta[t * numStates + i] = std::max(max1,max2);
        }
    }
    //termination
    double qmax = 0;
    int q;
    for (int i = 0; i < numStates; i++) {
        double val = delta[numObs * numStates + i];
        if (val > qmax) {
            q = i;
            qmax = val;
        }
    }
    return q;
}

/*void Matcher::loadText(std::string tfile, std::string efile, std::string sfile) {
    std::ifstream scoreLoader(sfile);
    note n;
    while (scoreLoader >> n) {
        score.push_back(n);
    }
    scoreLoader.close();
    numStates = score.size();
    std::vector<double> tMat(numStates * numStates);
    std::vector<double> eMat(numStates * config::numEmiss);
    std::ifstream tin(tfile);
    std::ifstream ein(efile);
    //load transMat (A)
    if (tin.good()) {
        for (int i = 0; i < config::numEmiss * config::numEmiss; i++) {
            tin >> tMat[i];
            //todo: check if filesize is correct and data is valid
        }
    }
    else {
        std::fill(tMat.begin(), tMat.end(), 1.0 / config::numEmiss);
    }
    //load emissMat (B)
    if (ein.good()) {
        for (int i = 0; i < config::numEmiss * config::numEmiss; i++) {
            ein >> eMat[i];
            //todo: check if filesize is correct and data is valid
        }
    }
    else {
        std::fill(eMat.begin(), eMat.end(), 1.0 / config::numEmiss);
    }
    transMat = tMat;
    emissMat = eMat;
    tin.close();
    ein.close();
}*/

void Matcher::train(std::vector<note> obs) {
    std::vector<double> p = pi;
    std::vector<double> tMat(numStates * numStates);
    std::fill(tMat.begin(), tMat.end(), mu);
    for (int i = 0; i < numStates; i++) {
        std::copy(transMat.begin() + i * 5, transMat.begin() + i * 5 + 5, tMat.begin() + i * config::windowLength + i - config::windowFront);
    }
    std::vector<double> eMat = emissMat;
    double prevProb = 0;
    double currentProb = 0;
    int T = obs.size();
    std::vector<double> alpha(T*numStates); //forward probabilities
    std::vector<double> beta(T*numStates); //backward probabilities
    std::vector<double> gamma(T*numStates); //individual state probabilities
    std::vector<double> xi(T*numStates*numStates); // pairwise state probabilities
    do {
        //forward-backward algorithm
        //initialization
        for (int i = 0; i < numStates; i++) {
            alpha[i] = p[i] * eMat[numStates*i + obs[0]];
            beta[(T - 1) * numStates + i] = 1;
        }
        //calculate alpha by recursion
        for (int t = 1; t < T; t++) {
            for (int i = 0; i < numStates; i++) {
                double sum = 0;
                for (int j = 0; j < numStates; j++) {
                    sum += alpha[(t - 1) * numStates + j] * tMat[j * numStates + i];
                }
                sum *= eMat[i * config::numEmiss + obs[t]];
                alpha[t * numStates + i] = sum;
            }
        }
        //calculate beta by recursion
        for (int t = T - 1; t > 0; t--) {
            for (int i = 0; i < numStates; i++) {
                double sum = 0;
                for (int j = 0; j < numStates; j++) {
                    sum += eMat[j * config::numEmiss + obs[t + 1]] * tMat[i * numStates + j] * beta[(t + 1) * numStates + j];
                }
                beta[t*numStates + i] = sum;
            }
        }
        //calculate P(obs|lamba)
        currentProb = 0;
        for (int i = 0; i < numStates; i++) {
            currentProb += alpha[T / 2 * numStates + i] * beta[T / 2 * numStates + i];
        }
        //intermediate variables
        //calculate xi
        /*for (int t = 0; t < T; t++) {
            for (int i = 0; i < numStates; i++) {
                for (int j = 0; j < numStates; j++) {
                    double sum = 0;
                    for (int a = 0; a < numStates; a++) {
                        for (int b = 0; b < numStates; b++) {
                            sum += alpha[t * numStates + a] *
                                tMat[a * numStates + b] *
                                eMat[b * numStates + obs[t + 1]] *
                                beta[(t + 1) * numStates + j];
                        }
                    }
                    xi[t * numStates * numStates + i * numStates + j] =
                        alpha[t * numStates + i] *
                        tMat[i * numStates + j] *
                        eMat[j * numStates + obs[t + 1]] *
                        beta[(t + 1) * numStates + j] / sum;
                }
            }
        }*/
        //calculate xi, version 2
        for (int t = 0; t < T; t++) {
            for (int i = 0; i < numStates; i++) {
                for (int j = 0; j < numStates; j++) {
                    xi[t * numStates * numStates + i * numStates + j] =
                        alpha[t * numStates + i] *
                        tMat[i * numStates + j] *
                        eMat[j * numStates + obs[t + 1]] *
                        beta[(t + 1) * numStates + j] / currentProb;
                }
            }
        }
        //calculate gamma
        /*for (int t = 0; t < T; t++) {
            for (int i = 0; i < numStates; i++) {
                double sum = 0;
                for (int j = 0; j < numStates; j++) {
                    sum += alpha[t * numStates + i] * beta[t * numStates + i];
                }
                gamma[t * numStates + i] = alpha[t * numStates + i] * beta[t * numStates + i] / sum;
            }
        }*/
        //calculate gamma, version 2
        for (int t = 0; t < numStates; t++) {
            for (int i = 0; i < numStates; i++) {
                gamma[t * numStates + i] = 0;
                for (int j = 0; j < numStates; j++) {
                    gamma[t * numStates + i] += xi[t * numStates * numStates + i * numStates + j];
                }
            }
        }
        //update
        for (int i = 0; i < numStates; i++) {
            //update p
            p[i] = gamma[i];
            //update tMat (A)
            for (int j = 0; j < numStates; j++) {
                double num = 0;
                double den = 0;
                for (int t = 0; t < T - 1; t++) {
                    num += xi[t * numStates * numStates + i * numStates + j];
                    den += gamma[t * numStates + i];
                }
                tMat[i * numStates + j] = num / den;
            }
            //update eMat (B)
            for (int k = 0; k < config::numEmiss; k++) {
                double num = 0;
                double den = 0;
                for (int t = 0; t < T; t++) {
                    double val = gamma[i * numStates + t];
                    num += (obs[t] == k) * val;
                    den += val;
                }
                eMat[i * config::numEmiss + k] = num / den;
            }
        }
    } while (prevProb - currentProb > config::trainThresh);
    save(saveFile, p, tMat, eMat);
}

void Matcher::save(std::string file, std::vector<double> pi, std::vector<double> tMat, std::vector<double> eMat) {
    //todo
}

double Matcher::tProb(int state1, int state2) {
    if (state1 > state2 - config::windowFront && state1 < state2 + config::windowRear) {
        return transMat[state2 * config::windowLength + state1 - state2];
    }
    return mu;
}

double Matcher::eProb(int state, int emiss) { //WIP
    if (state % 2 == 0) {
        for (int i = 0; i < 6; i++) {
            if (score[state / 2] & (_rotl16(1, emiss + i)) || score[state / 2] & (_rotl16(1, emiss - i))) {
                return gaussian[i];
            }
        }
    }
    else {
        return 1.0 / 12;
    }
}

double Matcher::calcMu(std::vector<double> tMat, int N) {
    double sum = 0;
    int tLeft = std::min(N, config::windowFront);
    int tRight = std::min(N, config::windowRear);
    int count = (tLeft * (tLeft + 1)) / 2 + (tRight * (tRight + 1)) / 2;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            sum += tMat[i*N + j] * (j < i - config::windowFront || j > i + config::windowRear);
        }
    }
    return sum / count;
}