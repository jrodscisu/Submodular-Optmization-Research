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
#define RUNS 100
#define INSTANCES 100
#define BAD_TAKING 0.01
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
    long double bin_size = OPT / K;
    while(size_S < K){
        // greedy = 0.0;
        greedy = bin_size;
        val = 0.0;
        // while(greedy + (OPT - f[size_S])/K + f[size_S] <= OPT && greedy + (OPT - f[size_S])/K <= prevGreedy){
        //     greedy += (OPT - f[size_S])/K;
        //     if(coin(gen) >= greedingChance)
        //         break;
        // }

        while(true){
            if(greedy < (OPT - f[size_S])/K){
                // cout << "here" << endl;
                greedy = (OPT - f[size_S])/K;
                continue;
            }
            if(bin_size - greedy/K > greedy){
                cout << "Wrong solution" << endl;
                exit(-1);
            }
            if(greedy + (greedy - (bin_size - greedy/K))/2 > prevGreedy){
                break;
            }

            greedy += (greedy - (bin_size - greedy/K))/2;
            if(coin(gen) <= 50){
                break;
            }
        }

        val = min((long double)OPT/K , greedy);

        // if(coin(gen)  <= 100 * (val/(val + greedy))){
        if(false){
            f.push_back(f[size_S] + val);
            S.push_back(1);
            vCount++;
        }else{
            f.push_back(f[size_S] + greedy);
            S.push_back(0);
            gCount++;
        }


        if(prevGreedy < greedy){
            cout << prevGreedy << ' ' << greedy << endl;
        }
        assert(greedy <= prevGreedy);
        assert(greedy >= bin_size);
        assert(greedy >= (OPT - f[size_S])/K);

        bin_size -= greedy/K;

        greedySum += greedy;
        prevGreedy = greedy;

        vs.push_back(val);
        gs.push_back(greedy);

        size_S++;
    }


    return greedySum;
}


string history_buffer;
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
    vector<int> unused_bad_ps(K, 1);
    vector<int> element;

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
            double x = 0.0;
            for (int m = 0; m < K; m++){
                x += min(bins[m], greedy_factors[j]/K);
            }
            // history_buffer += to_string(x) + " ";
            Gs.push_back(x);
        }

        //Allow bad prediction on this step 

        if(bad_ps && coin(gen) <= bad_percentage) {
            posible_bads.clear();

            for(int j = 0;j < K; j++){
                if(unused_bad_ps[j] == 1){
                    posible_bads.push_back(j);
                }
            }
            
            std::uniform_int_distribution<> pcoin(0,  (int)posible_bads.size() - 1);

            bad_prediction.first = posible_bads[pcoin(gen)]; 
            bad_prediction.second = min(bins[bad_prediction.first], greedy_factors.back() / K); // 1/K of the smallest marginal gain of greedy
            bad_prediction.first += 2 * K; // To differentiate from other indices
        }

        // Choose arg_max

        int arg_max = -1;
        long double max_val = -1.0;
            //From greedy
        vector<double> validGs;
        for(int j = 0; j < K; j++) {
            if(Gs[j] > 0){
                validGs.push_back(j);
            }
            if(Gs[j] > max_val) {
                max_val = Gs[j];
                arg_max = j;
            }
        }
            //From optimal

        for(int j = 0; j < os.size(); j++) {
            if(bins[os[j]] > max_val) {
                max_val = bins[os[j]];
                arg_max = os[j] + K;
            }
        }
            // From bad predictions

        for(int j = 0;j < K; j++){
            if(unused_bad_ps[j] == 1){
                if(min(bins[j], greedy_factors.back() / K) > max_val){
                    max_val = min(bins[j], greedy_factors.back() / K);
                    arg_max = j + 2 * K;
                }
            }
        }

        // Get prediction

        std::uniform_int_distribution<> pcoin(0, K + (int)os.size() - 1); //[Uncomment for greedy + optimal]
        // std::uniform_int_distribution<> pcoin(0, (int)os.size() - 1); // [Uncomment for opt]


        std::uniform_int_distribution<> pcoin2(0, (int)validGs.size() - 1);

        int p = pcoin(gen);// [Uncomment for greedy + optimal]
        // int p = K + pcoin(gen); //[Uncomment for optimal]
        // int p = pcoin2(gen); //[Uncomment for greedy]

        if(p < K) {
            while(Gs[p] <= 0){
                p = validGs[pcoin2(gen)];
            }
        }

        int g = arg_max;
        long double deltaP = (p >= K ) ? bins[os[p - K]] : Gs[p];
        long double deltaG = max_val;

        // Adjust for bad prediction

        if(bad_prediction.first != -2) {

            p = bad_prediction.first;
            deltaP = bad_prediction.second;
        }

        assert(deltaP + deltaG > 0);

        long double Beta = deltaP / (deltaP + deltaG);

        // cout << "Step " << i << ": p  " << p << ", DeltaP = " << deltaP << ", DeltaG = " << deltaG << ", Beta = " << Beta << endl;

        // history_buffer += "\nBins before step " + to_string(i) + ": ";
        // for(int i = 0; i < K; i++) {
        //     history_buffer += to_string(bins[i]) + " ";
        // }
        // history_buffer += "\n";

        
        // Beta = 0; //[Uncomment for no hints]
        if(coin(gen) < Beta * 100){
            double marginal_gain = deltaP;
            double Gammai = OPT - answer;

            history_buffer += "1 ";

            element.push_back(p);

            // history_buffer += "Step " + to_string(i) + ": Prediction taken. p = " + to_string(p) + ", DeltaP = " + to_string(deltaP) + ", Beta = " + to_string(Beta) + "\n";

            decisions.push_back(make_pair(make_pair(1, Beta), make_pair(marginal_gain, Gammai)));


            rhos.push_back((((deltaP * K) / Beta) - (1.0 - Beta)) / Beta);

            answer += deltaP;

            if(p < K) {
                for(int j = 0; j < K; j++) {
                    bins[j] = max((long double)0, bins[j] - greedy_factors[p]/K);
                }
                greedy_factors[p] = 0;
            }else if(p < 2 * K) { 
                bins[p - K] = deltaP;
            }else {
                unused_bad_ps[p - 2 * K] = 0;
                bins[p - 2 * K] = max((long double)0, bins[p - 2 * K] - deltaP);
            }

        }else {
            double marginal_gain = deltaG;
            double Gammai = OPT - answer;

            decisions.push_back(make_pair(make_pair(0, Beta), make_pair(marginal_gain, Gammai)));
            element.push_back(g);

            // history_buffer += "Step " + to_string(i) + ": Greedy taken. g = " + to_string(g) + ", DeltaG = " + to_string(deltaG) + ", Beta = " + to_string(Beta) + "\n";
            history_buffer += "0 ";

            rhos.push_back(0);

            answer += deltaG;

            if(g < K) {
                for(int j = 0; j < K; j++) {
                    bins[j] = max((long double)0, bins[j] - greedy_factors[g]/K);
                }
                greedy_factors[g] = 0;
            }else if (g < 2 * K){
                bins[g - K] = 0;
            }else {
                unused_bad_ps[g - 2 * K] = 0;
                bins[g - 2 * K] = max((long double)0, bins[g - 2 * K] - deltaG);
            }
        }

    }
    history_buffer += "\n";

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

    // assert(answer >= OPT * check - 0.0001);

    // cout << "Expected bound: " << OPT * check << " Achieved: " << answer << endl;

    for(int i = 0; i < K; i++) {
        // cout << "Step " << i << ": ";
        if(decisions[i].first.first == 1) {
            history_buffer += to_string(element[i]/K) + ' ';
            // cout << "Prediction taken. ";
        }else {
            history_buffer += to_string(element[i]/K) + ' ';
            // cout << "Greedy taken. ";
        }
        // cout << ", Rho: " << rhos[i] << endl;
    }
    history_buffer += '\n';
    // cout << endl;

    return make_pair(answer, GreedyVal);
}   

void runner(){

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> optCoin(1000, 1000000);

    ofstream fout;
    ofstream history;
    ifstream instance_r;
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
    long double meanOfAvgs = 0.0, medianOfAvgs = 0.0, avgOfMedians = 0.0;

    vector<pair<long double, pair<long double, long double>>> run_avgs;
    
    history.open("history_" + to_string(K) + ".out", ios::app);
    instance_r.open("instances_" + to_string(K) + ".out");

    history << "========================\n";
    history << "New Run at " << buffer << "\n";
    history << "========================\n";

    for (int i = 0; i < INSTANCES; i++){
        f.clear();
        gs.resize(K);
        vs.clear();
        S.clear();
        vCount = 0;
        gCount = 0;

        long double greedyValue = 0.0;
        //Generate a new instance 
        // OPT = optCoin(gen);

        // greedyValue = greedy_with_oph();
        // if (greedyValue < OPT * greedRatio){
        //     i--;
        //     continue;
        // }



        // Output greedy marginal gains to history and file

        fout.open("g_marginal_gain.in");

        instance_r >> OPT;

        history << "Instance " << i << ": OPT = " << OPT << "\n";
        // history << OPT << "\n";

        for(int j = 0; j < K; j++) {
            instance_r >> gs[j];

            fout << gs[j] << " ";
            history << gs[j] << " ";

            greedyValue += gs[j];
        }
        fout << endl;
        history << endl;

        fout.close();

        // Run modified greedy

        vector<long double> runs_results;
        pair<long double, long double> result;

        long double avg = 0.0;

        for (int j = 0; j < RUNS; j++) {
            history_buffer = "";
            result = mod_greedy((bad_percentage > 0));

            history << "Run " << j << ": Achieved Value = " << result.first << ", Greedy Value = " << result.second << "\n";
            history << history_buffer;

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

    // history << "Mean of Averages: " << meanOfAvgs << endl;
    // history << "Median of Averages: " << medianOfAvgs << endl;
    // history << "Average of Medians: " << avgOfMedians / INSTANCES << endl;
    // history << "Average Improvement: " << avg_improvement << endl;
    // history << "Averege Greedy Value: " << meanOfAvgs - avg_improvement << endl << endl;

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