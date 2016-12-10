#pragma once
#ifndef __YUKI_VIDEO_H__
#define __YUKI_VIDEO_H__

#include <opencv2/opencv.hpp>
#include <stdlib.h>
#include "dde_tracker.h"
#include "simple_warp.h"

using namespace cv;
using namespace std;

class Video {
private:
    static Video    *instance;
    
    DDETracker      *dde_tracker;
    SimpleWarp      *warpper;
    float           *mouth_lm_shift;
    bool            opencv_show_flag;
    bool            warp_show_flag;
    FILE            *lm_fp, *ex_fp;

    void            clear();

    Video() {}
    ~Video() { clear(); }
public:

    static void initialize();

    // must be bgra
    static void process_image(
        uint8_t *data,
        int width, int height,
        string window_name);
    static bool set_parameter(string, string);
    static SimpleWarp *get_warpper() { return instance->warpper; }
    static DDETracker *get_tracker() { return instance->dde_tracker; }
};

#endif
