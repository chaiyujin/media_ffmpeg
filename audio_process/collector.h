#pragma once
#ifndef __COLLECTOR_H__
#define __COLLECTOR_H__

#include <armadillo>
#include <vector>
#include <map>
#include "directory.h"
#include "dde_tracker.h"
#include "log.h"

using namespace std;

class Collector {
private:
    static vector<vector<float> > data_list;
    static vector<string> file_paths;
    static vector<string> phoneme;
    static map<string, int> phoneme_id;
    static arma::mat pca_v;
    static arma::vec mean;
    static int data_index;

    static vector<string> get_data_of_index(int index);

public:
    Collector() {}

    static bool find_all_data(string path);
    static void pca_landmarks();
    static vector<float> project_lm(const vector<float> &origin);
    static vector<float> back_project_lm(const vector<float> &projected);
    static bool get_first_data(vector<string> &files);
    static bool get_next_data (vector<string> &files);
    static int  get_id_of_phoneme(string phone);
};


#endif