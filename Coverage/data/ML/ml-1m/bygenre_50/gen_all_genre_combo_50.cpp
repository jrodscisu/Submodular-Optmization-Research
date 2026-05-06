#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <map>
#include <vector>

using namespace std;

const string genre_names[] = {"Action", "Adventure", "Animation", "Childrens", "Comedy", "Crime", "Documentary", "Drama", "Fantasy", "Film-Noir", "Horror", "Musical", "Mystery", "Romance", "Sci-Fi", "Thriller", "War", "Western"};

map<string, char> set_genres(){

    map<string, char> genre_map;
    genre_map.insert({"Action", 0});
	genre_map.insert({"Adventure", 1});
	genre_map.insert({"Animation", 2});
	genre_map.insert({"Children's", 3});
	genre_map.insert({"Comedy", 4});
	genre_map.insert({"Crime", 5});
	genre_map.insert({"Documentary", 6});
	genre_map.insert({"Drama", 7});
	genre_map.insert({"Fantasy", 8});
	genre_map.insert({"Film-Noir", 9});
	genre_map.insert({"Horror", 10});
	genre_map.insert({"Musical", 11});
	genre_map.insert({"Mystery", 12});
	genre_map.insert({"Romance", 13});
	genre_map.insert({"Sci-Fi", 14});
	genre_map.insert({"Thriller", 15});
	genre_map.insert({"War", 16});
	genre_map.insert({"Western", 17});

    return genre_map;
}

int main(int argc, char * argv[]) {

    int min_genres = atoi(argv[1]);
    int max_genres = atoi(argv[2]);

    ifstream fin_movies("formatted_movies.txt");
    ifstream fin_ratings("../formatted_ratings.txt");
    map<string, char> genre_map = set_genres();

    map<int, set<int> > movies;

    while(!fin_movies.eof()){
        string line;
        getline(fin_movies, line);

        istringstream ss(line);
        string genre;
        string movieid;
        ss >> movieid;
        set<int> genre_ids;

        while(!ss.eof()){
            ss >> genre;
            genre_ids.insert(genre_map[genre]);
        }

        movies.insert({atoi(movieid.c_str()), genre_ids});
    }

    fin_movies.close();

    vector<pair<int, int>> ratings;
    map<int, int> movie_count;
    int u, m;

    while(!fin_ratings.eof()){

        fin_ratings >> m >> u >> ws;

        if(movie_count.find(m) != movie_count.end()){
            movie_count[m]++;
        }else{
            movie_count[m] = 1;
        }

        ratings.push_back({m, u});
    }



    for(int i = 1; i < (1 << (int)genre_map.size()); i++){
        string genres_for_filename = "";
        set<int> allowed_genres;
        int j = i, cont = 0, curr = 0;
        while(j){
            if(j & 1){
                allowed_genres.insert(curr);
                genres_for_filename += ("_" + genre_names[curr]);
                cont++;
            }
            curr++;
            j >>= 1;
        }

        if(cont < min_genres || cont > max_genres)
            continue;
        
        ofstream fout(to_string(cont) + "/ratings" + genres_for_filename + "_50.txt");

        map<int, int> m_user, m_movie;
        vector<pair<int, int> > e;
        cont = 0;
        vector<pair<int, int > > movie_count_v(movie_count.size());

        for(auto [m, u] : ratings){
            bool flag = false;
            for(auto m_genre : movies[m]){
                if(allowed_genres.find(m_genre) != allowed_genres.end()){
                    flag = true;
                    break;
                }
            }

            if(!flag)
                continue;
            
            movie_count_v.push_back({movie_count[m], m});
        }

        sort(movie_count_v.begin(), movie_count_v.end());

        set<int> underrated_movies;

        for(int i = 0; i < (int)movie_count_v.size() / 2; i++){
            underrated_movies.insert(movie_count_v[i].second);
        }

        for(auto [m, u]: ratings){

            if(underrated_movies.find(m) == underrated_movies.end())
                continue;


            if(m_user.find(u) == m_user.end()){
                m_user.insert({u, m_user.size()});
            }
            if(m_movie.find(m) == m_movie.end()){
                m_movie.insert({m, m_movie.size()});
            }

            e.push_back({m_movie[m], m_user[u]});

        }
        
        fout << m_movie.size() + m_user.size() << ' ' << e.size() << endl;

        cout << genres_for_filename << ' ' << m_user.size() << ' ' << m_movie.size() << endl;
        
        for(auto [a, b] : e){
            fout << a << ' ' << b + (int)m_movie.size() << " " << 1 << endl;
        }

        fout.close();
    }

    return 0;
}