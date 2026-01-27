#include <iostream>
#include <vector>
#include <set>
#include <random>
#include <chrono>
#include <string>
#include <ctime>
#include <fstream>

#define OPT 1000000.0


using namespace std;

int N, K;
vector<int> S;
vector<long double> f, vs, gs;
int vCount, gCount;


double greedRatio = 0.63;
int greedingChance = 75;
pair<double, double> greedy_with_oph() {
    int size_S = 0;
    long double diff_avg = -1;
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
            if(coin(gen) < greedingChance/greedyGreed)
                break;
            // greedyGreed <<= 1;
        }
        val = min((long double)OPT/K , greedy);

        if(coin(gen)  <= 100 * (val/(val + greedy))){
            f.push_back(f[size_S] + val);
            S.push_back(1);
            vCount++;
        }else{
            f.push_back(f[size_S] + greedy);
            S.push_back(0);
            gCount++;
        }

        prevGreedy = greedy;

        vs.push_back(val);
        gs.push_back(greedy);

        size_S++;
        diff_avg = max(f[size_S] - f[size_S - 1], diff_avg);
    }

    // for(auto val : S){
    //     cout << val << " ";
    // }
    // cout << endl;

    // for(auto val : f){
    //     cout << val << " ";
    // }
    // cout << endl;

    // cout << "Result : " << f[K]/OPT << endl << endl;

    diff_avg = diff_avg;

    return make_pair(f[K], diff_avg);
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
            if(bins[i] > 0){
                os.push_back(i);
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
        std::uniform_int_distribution<> coin(0, os.size());
        std::uniform_int_distribution<> pcoin(1, 100);

        int p = coin(gen);
        int g = (arg_max >= K) ? arg_max - K : arg_max;
        long double deltaP = bins[p];
        long double deltaG = max_val;

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

    }

    return make_pair(answer, GreedyVal);

    // for(int j = 0; j < K; j++) {
    //     cout << bins[j] << " ";
    // }
    // cout << endl;

    // cout << "Final answer: " << answer/OPT << " * OPT"<< endl;
    // cout << "Greedy value: " << GreedyVal/OPT << " * OPT"<< endl;
}   

void runner(){

    ofstream fout;
    ofstream history;
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
    long double mini = 1.0, maxi = -1.0, avg = 0.0;
    pair<long double, long double> results_mini, results_maxi;

    
    history.open("history.out", ios::app);

    history << "========================\n";
    history << "New Run at " << buffer << "\n";
    history << "========================\n";

    for (int i = 0; i < 100; i++){
        f.clear();
        gs.clear();
        vs.clear();
        S.clear();
        vCount = 0;
        gCount = 0;

        pair<int, int> factors = greedy_with_oph();

        if (factors.first < OPT * greedRatio){
            i--;
            continue;
        }

        fout.open("greedy_factors.in");

        history << "Run " << i << ":\n";

        for(int j = 0; j < K; j++) {
            fout << gs[j] << " ";
            history << gs[j] << " ";
        }
        fout << endl;
        history << endl;

        fout.close();

        pair<long double, long double> result = mod_greedy();


        if((result.first - result.second) < mini * OPT){
            mini = (result.first - result.second)/OPT;
            results_mini = result;
        }

        if((result.first - result.second) > maxi * OPT){
            maxi = (result.first - result.second)/OPT;
            results_maxi = result;
        }

        avg += (result.first - result.second)/OPT;

        history << "Run " << i << " \n Result: " << result.first/OPT << " * OPT \n Greedy Value: " << result.second/OPT << " * OPT" << endl << endl;
    }

    history << "Min Improvement: " << mini << " when Greedy Value = " << results_mini.second/OPT << ", and Our result = " << results_mini.first/OPT << endl;
    history << "Max Improvement: " << maxi << " when Greedy Value = " << results_maxi.second/OPT << ", and Our result = " << results_maxi.first/OPT << endl;
    history << "Avg Improvement: " << avg << endl;

    history.close();
    
    avg /= 100.0;

    cout << "Min Improvement: " << mini << " when Greedy Value = " << results_mini.second/OPT << ", and Our result = " << results_mini.first/OPT << endl;
    cout << "Max Improvement: " << maxi << " when Greedy Value = " << results_maxi.second/OPT << ", and Our result = " << results_maxi.first/OPT << endl;
    cout << "Avg Improvement: " << avg << endl;
}

void find_best() {

    int max = -1;

    vector<long double> best_gs, best_vs, best_f;
    vector<int> best_S;
    int best_gCount = 0;
    int best_vCount = 0;

    for (int i = 0; i < 100; i++){
        f.clear();
        gs.clear();
        vs.clear();
        S.clear();
        vCount = 0;
        gCount = 0;

        pair<int, int> result = greedy_with_oph();

        if (result.first < OPT * greedRatio){
            continue;
        }


        // if(result.second > max){
        //     max = result.second;
        //     best_gs = gs;
        //     best_vs = vs;
        //     best_S = S;
        //     best_f = f;
        //     best_gCount = gCount;
        //     best_vCount = vCount;
        // }
    }

    // gs = best_gs;
    // vs = best_vs;
    // S = best_S;
    // f = best_f;
    // gCount = best_gCount;
    // vCount = best_vCount;

}

int main(int argc, char * argv[]) {

    if(argc < 2){
        exit(-1);
    }else{
        N = atoi(argv[1]);
        K = atoi(argv[2]);
    }

    runner();

    return 0;
    find_best();
    
    cout << "Result : " << f[K]/OPT << endl << endl;

    cout << "Number of greedy selections: " << gCount << endl;
    
    double sum_gs = 0;

    for(auto val : gs){
        cout << val << " ";
        sum_gs += val;
    }
    cout << endl;
    cout << "Ratio of greedy: " << sum_gs/OPT << endl << endl;

    cout << "Number of oph selections: " << vCount << endl;

    double sum_vs = 0;
    for(auto val : vs){
        cout << val << " ";
        sum_vs += val;
    }
    cout << endl;
    cout << "Ratio of : " << sum_vs/OPT << endl;

    return 0;
}