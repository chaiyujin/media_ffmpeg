#include <armadillo>
#include "collector.h"
#include <io.h>
#include <stdlib.h>
#include <direct.h>
#include <fstream>
using namespace std;
using namespace arma;
using namespace Yuki;

vector<vector<float> > Collector::data_list;
vector<string>         Collector::file_paths;
vector<string>         Collector::phoneme; 
map<string, int>       Collector::phoneme_id;
arma::mat Collector::pca_v;
arma::vec Collector::mean;
int Collector::data_index = 0;

bool Collector::find_all_data(string path) {
    if (!DirBrowser::set_directory(path)) return false;
    // landmarks
    string pattern = "*.land";
    file_paths = DirBrowser::find_files(pattern, true);

    // put all data into list
    data_list.clear();
    for (size_t i = 0; i < file_paths.size(); ++i) {
        vector<float> data;
        ifstream fin(file_paths[i]);
        bool flag = true;
        while (flag) {
            data.clear();
            for (int j = 0; j < MOUTH_LANDMARKS * 2; ++j) {
                float x;
                if (fin >> x) {
                    data.push_back(x);
                }
                else {
                    flag = false;
                    break;
                }
            }
            if (flag) data_list.push_back(data);
        }
        fin.close();
    }

    // phoneme
    pattern = "*.phone";
    file_paths = DirBrowser::find_files(pattern, true);

    phoneme.clear();
    for (size_t i = 0; i < file_paths.size(); ++i) {
        string phone;
        ifstream fin(file_paths[i]);
        while (fin >> phone) {
            bool flag = false;
            for (int j = 0; j < phoneme.size(); ++j) {
                if (phoneme[j] == phone) {
                    flag = true; break;
                }
            }
            if (!flag) {
                phoneme.push_back(phone);
            }
        }
        fin.close();
    }
    sort(phoneme.begin(), phoneme.end());
    for (int i = 0; i < phoneme.size(); ++i) {
        phoneme_id.insert(pair<string, int>(phoneme[i], i));
    }
    return true;
}

void Collector::pca_landmarks() {
    // zero center
    mean = vec(MOUTH_LANDMARKS * 2);
    mean.fill(0.0);
    for (size_t i = 0; i < data_list.size(); ++i) {
        for (size_t j = 0; j < MOUTH_LANDMARKS * 2; ++j) {
            mean(j) += data_list[i][j];
        }
    }
    for (size_t j = 0; j < MOUTH_LANDMARKS * 2; ++j) {
        mean(j) /= (double)data_list.size();
    }
    mat A(data_list.size(), MOUTH_LANDMARKS * 2);
    for (size_t i = 0; i < data_list.size(); ++i) {
        for (size_t j = 0; j < MOUTH_LANDMARKS * 2; ++j) {
            A(i, j) = data_list[i][j] - mean(j);
        }
    }

    // svd
    mat U;
    vec s;
    mat V;

    svd(U, s, V, A, "std");

    // determine number of components
    double score_sum = 0.;
    double score;
    int n_components = -1;
    for (int i = 0; i < s.n_rows; ++i) {
        score_sum += s(i, 0);
    }
    for (int i = 0; i < s.n_rows; ++i) {
        score += s(i, 0);
        if (score / score_sum >= 0.95) {
            n_components = i + 1;
            break;
        }
    }

    // get the transform matrix
    LOG::notice("There are %d components after PCA.\n", n_components);
    pca_v = mat(V.n_rows, n_components);
    for (int i = 0; i < pca_v.n_rows; ++i) {
        for (int j = 0; j < n_components; ++j) {
            pca_v(i, j) = V(i, j);
        }
    }
}

vector<float> Collector::project_lm(const vector<float> &origin) {
    CHECK(origin.size() == pca_v.n_rows);
    mat input(1, origin.size());
    for (size_t i = 0; i < origin.size(); ++i) {
        input(0, i) = origin[i] - mean(i);
    }
    mat output = input * pca_v;
    vector<float> projected;
    for (int i = 0; i < output.n_cols; ++i) {
        projected.push_back(output(0, i));
    }
    return projected;
}

vector<float> Collector::back_project_lm(const vector<float> &projected) {
    CHECK(projected.size() == pca_v.n_cols);
    mat input(1, projected.size());
    for (size_t i = 0; i < projected.size(); ++i) {
        input(0, i) = projected[i];
    }
    mat output = input * pca_v.t();
    vector<float> origin;
    for (int i = 0; i < output.n_cols; ++i) {
        origin.push_back(output(0, i) + mean(i));
    }
    return origin;
}

vector<string> Collector::get_data_of_index(int index) {
    string land_file = file_paths[index];
    int i = land_file.length() - 1;
    while (land_file[i] != '/') i--;
    string dir = land_file.substr(0, i + 1);
    string file = land_file.substr(i + 1, land_file.length() - i - 1);
    i = file.length() - 1;
    while (file[i] != '.') i--;
    file = file.substr(0, i);
    LOG::notice("Get data with name: %s\n", file.c_str());
    vector<string> res;
    res.push_back(dir + file + ".land");
    res.push_back(dir + file + ".phone");
    res.push_back(dir + file + ".expr");
    return res;
}

bool Collector::get_first_data(vector<string> &files) {
    data_index = 0;
    if (data_index < file_paths.size()) {
        files = get_data_of_index(data_index);
        return true;
    }
    else
        return false;
}

bool Collector::get_next_data(vector<string> &files) {
    ++data_index;
    if (data_index < file_paths.size()) {
        files = get_data_of_index(data_index);
        return true;
    }
    else
        return false;
}

int Collector::get_id_of_phoneme(string phone) {
    map<string, int>::iterator it;
    it = phoneme_id.find(phone);
    if (it != phoneme_id.end()) {
        return it->second;
    }
    return -1;
}
