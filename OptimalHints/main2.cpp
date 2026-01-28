#include <iostream>
#include <vector>
#include <set>
#include <random>
#include <cassert>
#include <chrono>
#include <string>
#include <ctime>
#include <fstream>
#include <cmath>
#include <algorithm>

// #define OPT 1000000.0
#define RUNS 1
#define INSTANCES 1
#define BAD_TAKING 0.1
#define e 2.71828

using namespace std;

long double OPT = 1000000.0;
int K, bad_percentage = 0;
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
    long double greedySum = 0.0;
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

        greedySum += greedy;
        prevGreedy = greedy;

        vs.push_back(val);
        gs.push_back(greedy);

        size_S++;
    }


    return greedySum;
}

pair<long double, long double> mod_greedy(bool bad_ps = false) {

    ifstream fin;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> coin(1, 100);
    vector<long double> bins (K, OPT/K);
    vector<long double> os, Gs;
    vector<long double> greedy_factors;

    // ((Prediction or not, Beta), (Marginal gain, rho))
    vector<pair<pair<int, long double>, pair<long double, long double>>> decisions;
    vector<long double> bad_ps;
    vector<long double> rhos;
    long double GreedyVal = 0.0;

    fin.open("g_marginal_gain.in");

    for(int i = 0; i < K; i++) {
        double x;
        fin >> x;
        greedy_factors.push_back(x);

        GreedyVal += x;
    }

    fin.close();

    long double answer = 0.0;
    pair<int, long double> bad_prediction;
    vector<int> posible_bads;

    for(int i = 0; i < K; i++) {

        os.clear();
        Gs.clear();
        bad_prediction = make_pair(-2, -1.0);

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

        //Allow bad prediction on this step 

        if(bad_ps && coin(gen) <= bad_percentage) {
            int cont = 0 ;
            bad_prediction = make_pair(-1, -1.0);
            posible_bads.clear();
            for(int j = 0; j < K; j++) {
                if(Gs[j] > 0) {
                    posible_bads.push_back(j);
                }
                cont++;
            }
            for(int j = 0; j < os.size(); j++) {
                posible_bads.push_back(os[j] + K);
            }

            // Make bad prediction

            std::uniform_int_distribution<> pcoin(0,  (int)posible_bads.size() - 1);
            
            bad_prediction.first = posible_bads[pcoin(gen)];
            if(bad_prediction.first < K) {
                bad_prediction.second = Gs[bad_prediction.first];
            }else {
                bad_prediction.second = bins[bad_prediction.first - K] * BAD_TAKING;
            }
        }

        // Choose arg_max

        int arg_max = -1;
        long double max_val = -1.0;

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

        std::uniform_int_distribution<> pcoin(0, (int)os.size() - 1);

        int p = K + pcoin(gen);
        int g = arg_max;
        long double deltaP = bins[os[p - K]];
        long double deltaG = max_val;

        // Adjust for bad prediction

        if(bad_prediction.first != -2) {

            assert(true);

            p = bad_prediction.first;
            deltaP = bad_prediction.second;
        }

        assert(deltaP + deltaG > 0);

        long double Beta = deltaP / (deltaP + deltaG);

        cout << "Step " << i << ": p  " << p << ", DeltaP = " << deltaP << ", DeltaG = " << deltaG << ", Beta = " << Beta << endl;
        if(coin(gen) < Beta * 100){
            double marginal_gain = deltaP;
            double Gammai = OPT - answer;

            decisions.push_back(make_pair(make_pair(1, Beta), make_pair(marginal_gain, Gammai)));


            rhos.push_back((((deltaP * K) / Beta) - (1.0 - Beta)) / Beta);

            answer += deltaP;

            if(p < K) {
                for(int j = 0; j < K; j++) {
                    bins[j] = max((long double)0, bins[j] - greedy_factors[p]/K);
                }
                greedy_factors[p] = 0;
            }else { 
                bins[p - K] = deltaP;
            }


        }else {
            double marginal_gain = deltaG;
            double Gammai = OPT - answer;

            decisions.push_back(make_pair(make_pair(0, Beta), make_pair(marginal_gain, Gammai)));

            rhos.push_back(0);

            answer += deltaG;

            if(arg_max < K) {
                for(int j = 0; j < K; j++) {
                    bins[j] = max((long double)0, bins[j] - greedy_factors[g]/K);
                }
                greedy_factors[g] = 0;
            }else {
                bins[arg_max - K] = 0;
            }
        }

    }

    //summation check

    double sum_cis = 0.0;

    for(int i = 0; i < K; i++) {
        double Beta = decisions[i].first.second;
        double alpha_i = (decisions[i].first.first == 1) ? 1.0 : 0.0;
        double Gammai = decisions[i].second.second; 
        double rho_i = rhos[i];
        double ci = (1 - Beta) + alpha_i * rho_i * Beta;
        sum_cis += ci; 
    }

    double check = (1 - exp(-sum_cis / K));

    assert(answer >= OPT * check - 0.0001);

    cout << "Expected bound: " << OPT * check << " Achieved: " << answer << endl;

    for(int i = 0; i < K; i++) {
        cout << "Step " << i << ": ";
        if(decisions[i].first.first == 1) {
            cout << "Prediction taken. ";
        }else {
            cout << "Greedy taken. ";
        }
        cout << ", Rho: " << rhos[i] << endl;
    }
    cout << endl;

    return make_pair(answer, GreedyVal);
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
        long double greedyValue = greedy_with_oph();

        if (greedyValue < OPT * greedRatio){
            i--;
            continue;
        }

        // Output greedy marginal gains to history and file

        fout.open("g_marginal_gain.in");

        history << "Instance " << i << ": OPT = " << OPT << "\n";

        for(int j = 0; j < K; j++) {
            fout << gs[j] << " ";
            history << gs[j] << " ";
        }
        fout << endl;
        history << endl;

        fout.close();

        // Run modified greedy

        vector<long double> runs_results;
        pair<long double, long double> result;

        long double avg = 0.0;

        for (int j = 0; j < RUNS; j++) {
            result = mod_greedy((bad_percentage > 0));

            avg += result.first;
            runs_results.push_back(result.first);
        }

        //Compute statistics

        sort(runs_results.begin(), runs_results.end());

        avg /= (RUNS * OPT);

        meanOfAvgs += avg;

        avgOfMedians +=  runs_results[RUNS / 2]/OPT;

        run_avgs.push_back(make_pair(avg, make_pair(greedyValue/OPT, OPT)));
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

    // Output results

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

    if(argc < 2){
        exit(-1);
    }else{
        K = atoi(argv[1]);

        if(argc > 2){
            bad_percentage = atoi(argv[2]);
        }
    }

    runner();

    return 0;
}