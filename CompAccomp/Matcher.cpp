#include <iostream>
#include <fstream>
#include <algorithm>
#include "Matcher.h"

Matcher::Matcher(std::vector<note> score, std::vector<double> transitionMat, std::vector<double> emissionMat) {
    transMat = transitionMat;
    emissMat = emissionMat;
    Matcher::score = score;
}

Matcher::~Matcher()
{

}

int Matcher::viterbi()
{
    //initialization
    for (int i = 0; i < numStates; i++) {
        delta[i] = pi[i] * emissMat[i * 12 + sequence[0]];
        psi[i] = 0;
    }
    return 0;
}

void Matcher::load(std::string tfile, std::string efile, std::string sfile) {
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
}

void Matcher::train(std::vector<note> obs)
{
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
            alpha[i] = pi[i] * emissMat[numStates*i + obs[0]];
            beta[(T - 1) * numStates + i] = 1;
        }
        //calculate alpha by recursion
        for (int t = 1; t < T; t++) {
            for (int i = 0; i < numStates; i++) {
                double sum = 0;
                for (int j = 0; j < numStates; j++) {
                    sum += alpha[(t - 1) * numStates + j] * transMat[j * numStates + i];
                }
                sum *= emissMat[i * config::numEmiss + obs[t]];
                alpha[t * numStates + i] = sum;
            }
        }
        //calculate beta by recursion
        for (int t = T - 1; t > 0; t--) {
            for (int i = 0; i < numStates; i++) {
                double sum = 0;
                for (int j = 0; j < numStates; j++) {
                    sum += emissMat[j * config::numEmiss + obs[t + 1]] * transMat[i * numStates + j] * beta[(t + 1) * numStates + j];
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
                                transMat[a * numStates + b] *
                                emissMat[b * numStates + obs[t + 1]] *
                                beta[(t + 1) * numStates + j];
                        }
                    }
                    xi[t * numStates * numStates + i * numStates + j] =
                        alpha[t * numStates + i] *
                        transMat[i * numStates + j] *
                        emissMat[j * numStates + obs[t + 1]] *
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
                        transMat[i * numStates + j] *
                        emissMat[j * numStates + obs[t + 1]] *
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
            //update pi
            pi[i] = gamma[i];
            //update transMat (A)
            for (int j = 0; j < numStates; j++) {
                double num = 0;
                double den = 0;
                for (int t = 0; t < T - 1; t++) {
                    num += xi[t * numStates * numStates + i * numStates + j];
                    den += gamma[t * numStates + i];
                }
                transMat[i * numStates + j] = num / den;
            }
            //update emissMat (B)
            for (int k = 0; k < config::numEmiss; k++) {
                double num = 0;
                double den = 0;
                for (int t = 0; t < T; t++) {
                    double val = gamma[i * numStates + t];
                    num += (obs[t] == k) * val;
                    den += val;
                }
                emissMat[i * config::numEmiss + k] = num / den;
            }
        }
    } while (prevProb - currentProb > config::trainThresh);
}
