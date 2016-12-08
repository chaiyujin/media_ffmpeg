#pragma once
#ifndef __LM_COLLECT_H__
#define __LM_COLLECT_H__

#include <vector>
#include <opencv2/opencv.hpp>
#include "directory.h"
#include "dde_tracker.h"

using namespace cv;
using namespace std;

class Collector {
private:
    static vector<vector<float> > data_list;
    static Mat data_mat;

public:
    Collector() {}

    static bool find_all_data(string path, string ext);
    static void pca();
};


#endif