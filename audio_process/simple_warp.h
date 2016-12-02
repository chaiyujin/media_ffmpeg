#pragma once
#ifndef __WARP_H__
#define __WARP_H__

#define MOUTH_COUNT 30

#include "dde_tracker.h"
#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;

extern const int mouth_lm_index[];

class SimpleWarp {
private:
    Mat image;
    float cx, cy;
    float w, h;
    float old_lm[150];
    float new_lm[150];
    DDETracker *dde;
public:
    SimpleWarp(DDETracker *dde_tracker, string file_path) {
        image = imread(file_path);
        //h = image.rows;
        //w = image.cols;
        for (int i = 0; i < 20; ++i) dde_tracker->run(image);
        int trans_n, land_n, expression_n, identity_n;
        float *land = dde_tracker->get("landmarks",   &land_n);
        cx = land[49 * 2];
        cy = land[49 * 2 + 1];
        float bh, bw, hmin = INFINITY, hmax = -INFINITY, wmin = INFINITY, wmax = -INFINITY;
        for (int i = 0, idx= 0; i < land_n; i += 2) {
            old_lm[i] = land[i];
            old_lm[i + 1] = land[i + 1];
            if (dde_tracker->is_mouth_landmark(i / 2)) {
                if (land[i] - cx < hmin) hmin = land[i] - cx;
                if (land[i] - cx > hmax) hmax = land[i] - cx;
                if (land[i + 1] - cy < wmin) wmin = land[i + 1] - cy;
                if (land[i + 1] - cy > wmax) wmax = land[i + 1] - cy;
            }
        }
        h = hmax - hmin;
        w = wmax - wmin;
        dde = dde_tracker;
    }

    void simple_warp(float *shift) {
        Mat img(image.rows, image.cols, image.type());
        image.copyTo(img);
        for (int i = 0; i < MOUTH_COUNT; ++i) {
            shift[i * 2] *= h;
            shift[i * 2 + 1] *= w;
        }
        for (int i = 0, idx = 0; i < 150; i += 2) {
            new_lm[i] = old_lm[i];
            new_lm[i + 1] = old_lm[i + 1];
            if (dde->is_mouth_landmark(i / 2)) {
                new_lm[i]     = cx + shift[idx++];
                new_lm[i + 1] = cy + shift[idx++];
            }
            circle(img, Point(new_lm[i], new_lm[i + 1]), 1, Scalar(255, 0, 0), -1);
            //circle(img, Point(old_lm[i], old_lm[i + 1]), 1, Scalar(255, 0, 0), -1);
        }
        int ch = img.channels();
        for (int r = 0; r < img.rows; ++r) {
            uint8_t* data = img.ptr<uint8_t>(r);
            for (int c = 0; c < img.cols; c++) {
                for (int i = 0; i < 47; ++i) {
                    if (in_triangle(c, r, mouth_triangles[i * 3],
                        mouth_triangles[i * 3 + 1],
                        mouth_triangles[i * 3 + 2])) {
                        Scalar color(0, 0, 0);
                        if (i >= 33 && i < 39) {
                        }
                        else {
                        color = calc_origin_color(image, c, r, mouth_triangles[i * 3],
                            mouth_triangles[i * 3 + 1],
                            mouth_triangles[i * 3 + 2]);
                        }
                        data[c * ch] = color[0];
                        data[c * ch + 1] = color[1];
                        data[c * ch + 2] = color[2];
                        break;
                    }
                }
            }
        }
        imshow("simple", img);
    }

    bool in_triangle(int x, int y, int pi0, int pi1, int pi2);
    Scalar calc_origin_color( Mat &img, int x, int y, int pi0, int pi1, int pi2);
};
#endif