#include <iostream>
#include <vector>
#include <set>
#include <random>
#include <cassert>
#include <chrono>
#include <string>
#include <ctime>
#include <fstream>
#include <algorithm>

// #define OPT 1000000.0
#define RUNS 100
#define INSTANCES 100

using namespace std;

long double OPT;
int N, K;
vector<int> S;
vector<long double> f, vs, gs;
int vCount, gCount;

long double greedRatio = 0.63;
int greedingChance = 75;

long double greedy_with_oph() {
    int size_S = 0;
    double prevGreedy = OPT;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> coin(1, 100);
    f.push_back(0);
    
    long double greedy;
    long double val;
    int greedyGreed;
    while(size_S < K){
        greedy = 0.0;
        val = 0.0;
        greedyGreed = 1;
        while(greedy + (OPT - f[size_S])/K + f[size_S] < OPT && greedy + (OPT - f[size_S])/K <= prevGreedy){
            greedy += (OPT - f[size_S])/K;
            if(coin(gen) < 15){
                greedy += min(min(OPT/100, prevGreedy - greedy), OPT - greedy - f[size_S]);
            }
            if(coin(gen) < greedingChance/greedyGreed)
                break;
            // greedyGreed <<= 1;
        }
        val = min((long double)OPT/K , greedy);

        assert(val + greedy > 0);

        if(coin(gen)  <= 100 * (val/(val + greedy))){
            f.push_back(f[size_S] + val);
            S.push_back(1);
            vCount++;
        }else{
            f.push_back(f[size_S] + greedy);
            S.push_back(0);
            gCount++;
        }

        assert(greedy <= prevGreedy);

        prevGreedy = greedy;

        vs.push_back(val);
        gs.push_back(greedy);

        size_S++;
    }


    return f[K];
}

pair<long double, long double> mod_greedy() {
    ifstream fin;

    vector<long double> bins (K, OPT/K);
    vector<long double> os, Gs;
    vector<long double> greedy_factors;
    long double GreedyVal = 0.0;

    fin.open("greedy_factors.in");

    for(int i = 0; i < K; i++) {
        double x;
        fin >> x;
        greedy_factors.push_back(x);

        GreedyVal += x;
    }

    fin.close();

    long double answer = 0.0;

    for(int i = 0; i < K; i++) {

        // for(int j = 0; j < K; j++) {
        //     cout << bins[j] << " ";
        // }
        // cout << endl;

        os.clear();
        Gs.clear();

        //Calculate optimals

        for(int j = 0; j < K; j++) {
            if(bins[j] > 0){
                os.push_back(j);
            }
        }
        
        // Calculate greedys

        for(int j = 0; j < K; j++) {
            Gs.push_back(0);
            for (int m = 0; m < K; m++){
                Gs.back() += min(bins[m], greedy_factors[j]/K);
            }
        }

        // Choose arg_max

        int arg_max = -1;
        long double max_val = -1;

        for(int j = 0; j < K; j++) {
            if(Gs[j] > max_val) {
                max_val = Gs[j];
                arg_max = j;
            }
        }

        for(int j = 0; j < os.size(); j++) {
            if(bins[os[j]] > max_val) {
                max_val = bins[os[j]];
                arg_max = os[j] + K;
            }
        }

        // Get prediction

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> coin(0, (int)os.size() - 1);
        std::uniform_int_distribution<> pcoin(1, 100);

        int p = coin(gen);
        int g = (arg_max >= K) ? arg_max - K : arg_max;
        long double deltaP = bins[p];
        long double deltaG = max_val;

        // cout << "deltaP: " << deltaP << " os.size(): " << os.size() << ", deltaG: " << deltaG << endl;
        assert(deltaP + deltaG > 0);

        long double Beta = deltaP / (deltaP + deltaG);

        if(pcoin(gen) < Beta * 100){
            answer += deltaP;
            bins[p] = 0;

            // cout << "Optimal selected via prediction: " << p << " with deltaP = " << deltaP << endl;
        }else {
            answer += deltaG;

            if(arg_max < K) {
                for(int j = 0; j < K; j++) {
                    bins[j] = max((long double)0, bins[j] - greedy_factors[g]/K);
                }
                greedy_factors[g] = 0;
                // cout << "Greedy selected: " << g << " with deltaG = " << deltaG << endl;
            }else {
                bins[arg_max - K] = 0;
                // cout << "Optimal selected via Greedy: " << arg_max - K << " with deltaG = " << deltaG << endl;
            }
        }

        // cout << "Current bins: " << endl;
    }
    // cout << "Current answer2: " << answer/OPT << " * OPT"<< endl;

    return make_pair(answer, GreedyVal);

    // for(int j = 0; j < K; j++) {
    //     cout << bins[j] << " ";
    // }
    // cout << endl;

    // cout << "Final answer: " << answer/OPT << " * OPT"<< endl;
    // cout << "Greedy value: " << GreedyVal/OPT << " * OPT"<< endl;
}   

void runner(){

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> optCoin(1000, 1000000);

    ofstream fout;
    ofstream history;
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
    long double meanOfAvgs = 0.0, medianOfAvgs = 0.0, avgOfMedians = 0.0;

    vector<pair<long double, pair<long double, long double>>> run_avgs;
    
    history.open("history.out", ios::app);

    history << "========================\n";
    history << "New Run at " << buffer << "\n";
    history << "========================\n";

    for (int i = 0; i < INSTANCES; i++){
        f.clear();
        gs.clear();
        vs.clear();
        S.clear();
        vCount = 0;
        gCount = 0;

        //Generate a new instance
        OPT = optCoin(gen);
        long double factors = greedy_with_oph();

        if (factors < OPT * greedRatio){
            i--;
            continue;
        }

        fout.open("greedy_factors.in");

        history << "Instance " << i << ": OPT = " << OPT << "\n";

        for(int j = 0; j < K; j++) {
            fout << gs[j] << " ";
            history << gs[j] << " ";
        }
        fout << endl;
        history << endl;

        fout.close();

        vector<long double> runs_results;
        pair<long double, long double> result;

        long double avg = 0.0;

        for (int j = 0; j < RUNS; j++) {
            result = mod_greedy();

            avg += result.first;
            runs_results.push_back(result.first);
        }

        avg /= (RUNS * OPT);

        meanOfAvgs += avg;

        avgOfMedians +=  runs_results[RUNS / 2]/OPT;

        run_avgs.push_back(make_pair(avg, make_pair(factors/OPT, OPT)));
    }

    sort(run_avgs.begin(), run_avgs.end());
    meanOfAvgs /= INSTANCES;
    medianOfAvgs = run_avgs[INSTANCES / 2].first;

    // Improvement calculations

    long double avg_improvement = 0.0;
    for (int i = 0; i < INSTANCES; i++) {
        long double improvement = (run_avgs[i].first - run_avgs[i].second.first);

        avg_improvement += improvement;
    }

    avg_improvement /= INSTANCES;

    // history << "Mean Min Improvement: " << mini << " when Greedy Value = " << results_mini.second/OPT << ", and Our result = " << results_mini.first/OPT << endl;
    // history << "Max Improvement: " << maxi << " when Greedy Value = " << results_maxi.second/OPT << ", and Our result = " << results_maxi.first/OPT << endl;
    // history << "Avg Improvement: " << avg << endl << endl;

    // cout << "Min Improvement: " << mini << " when Greedy Value = " << results_mini.second/OPT << ", and Our result = " << results_mini.first/OPT << endl;
    // cout << "Max Improvement: " << maxi << " when Greedy Value = " << results_maxi.second/OPT << ", and Our result = " << results_maxi.first/OPT << endl;
    // cout << "Avg Improvement: " << avg << endl;
    
    history << "Mean of Averages: " << meanOfAvgs << endl;
    history << "Median of Averages: " << medianOfAvgs << endl;
    history << "Average of Medians: " << avgOfMedians / INSTANCES << endl;
    history << "Average Improvement: " << avg_improvement << endl;
    history << "Averege Greedy Value: " << meanOfAvgs - avg_improvement << endl << endl;

    cout << "Mean of Averages: " << meanOfAvgs << endl;
    cout << "Median of Averages: " << medianOfAvgs << endl;
    cout << "Average of Medians: " << avgOfMedians / INSTANCES << endl;
    cout << "Average Improvement: " << avg_improvement << endl;
    cout << "Averege Greedy Value: " << meanOfAvgs - avg_improvement << endl << endl;

    history.close();
    
}

int main(int argc, char * argv[]) {

    if(argc < 1){
        exit(-1);
    }else{
        K = atoi(argv[1]);
    }

    runner();

    return 0;
}