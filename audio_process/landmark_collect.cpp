#include "landmark_collect.h"
#include <io.h>
#include <stdlib.h>
#include <direct.h>
#include <fstream>
using namespace std;
using namespace Yuki;

vector<vector<float> > Collector::data_list;
Mat Collector::data_mat;

bool Collector::find_all_data(string path, string ext) {
    if (!DirBrowser::set_directory(path)) return false;
    string pattern = "*";
    if (ext[0] == '.') { pattern += ext; }
    else {               pattern += "." + ext; }
    vector<string> file_paths = DirBrowser::find_files(pattern, true);

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

    data_mat = Mat(data_list.size(), MOUTH_LANDMARKS * 2, CV_32FC1);
    for (int r = 0; r < data_list.size(); ++r) {
        float *data = data_mat.ptr<float>(r);
        for (int c = 0; c < MOUTH_LANDMARKS * 2; ++c) {
            data[c] = data_list[r][c];
        }
    }

    return true;
}

void Collector::pca() {
    int n_components = 30;
    PCA my_pca(data_mat, Mat(), CV_PCA_DATA_AS_ROW, n_components);
    for (int i = 0; i < n_components; ++i) {
        Mat vec = my_pca.eigenvectors.row(i);
        float *data = vec.ptr<float>(0);
        for (int c = 0; c < MOUTH_LANDMARKS * 2; ++c) {
            cout << data[c] << " ";
        }
        cout << endl;
    }
}