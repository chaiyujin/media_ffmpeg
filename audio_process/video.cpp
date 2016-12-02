#include "video.h"

static int mask[46] = {
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 1, 0, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1,
    1, 1, 0, 0, 0,
    0
};

static int new_mask[46] = {
    1, 1, 0, 0, 0,
    0, 0, 0, 1, 1,
    0, 0, 0, 0, 1,
    1, 1, 1, 1, 0,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 0, 1, 1, 
    1, 1, 1, 1, 0,
    0
};

Video *Video::instance = NULL;

void Video::initialize() {
    if (instance) return;
    instance = new Video();
    instance->dde_tracker = new DDETracker();
    instance->warpper = new SimpleWarp(instance->dde_tracker, "data/lm.png");
    instance->mouth_lm_shift = new float[30 * 2];
    instance->opencv_show_flag = false;
    instance->warp_show_flag = false;
    instance->lm_fp = NULL;
    instance->ex_fp = NULL;
    return;
}

void Video::clear() {
    delete[] mouth_lm_shift;
    delete   dde_tracker;
}

void Video::process_image(
    uint8_t *data,
    int width, int height,
    string window_name) {

    if (!instance) return;
    DDETracker *dde_tracker = instance->dde_tracker;
    float      *mouth_lm_shift = instance->mouth_lm_shift;


    while (!dde_tracker->run((unsigned char *)data, width, height));
    for (int i = 0; i < 5; ++i) dde_tracker->run((unsigned char *)data, width, height);
    int land_n, expr_n;
    float *land = dde_tracker->get("landmarks_ar", &land_n);
    float *expr = dde_tracker->get("expression",   &expr_n);
    /* get normalized landmarks */
    float cx = -land[49 * 3], cy = -land[49 * 3 + 1];
    float bh, bw, hmin = INFINITY, hmax = -INFINITY, wmin = INFINITY, wmax = -INFINITY;
    for (int i = 0, idx= 0; i < land_n; i += 3) {
        if (dde_tracker->is_mouth_landmark(i / 3)) {
            if (-land[i] - cx < hmin) hmin = -land[i] - cx;
            if (-land[i] - cx > hmax) hmax = -land[i] - cx;
            if (-land[i + 1] - cy < wmin) wmin = -land[i + 1] - cy;
            if (-land[i + 1] - cy > wmax) wmax = -land[i + 1] - cy;
        }
    }
    bh = hmax - hmin;
    bw = wmax - wmin;
    for (int i = 0, idx = 0; i < land_n; i += 3) {
        if (dde_tracker->is_mouth_landmark(i / 3)) {
            mouth_lm_shift[idx++] = (-land[i] - cx) / float(bh);
            mouth_lm_shift[idx++] = (-land[i + 1] - cy) / float(bw);
            if (instance->lm_fp) {
                fprintf(instance->lm_fp, "%f %f ", mouth_lm_shift[idx - 2], mouth_lm_shift[idx - 1]);
            }
        }
    }
    if (instance->lm_fp) fprintf(instance->lm_fp, "\n");

    int idx = 0;
    for (int i = 0; i < expr_n; ++i) {
        if (mask[i] && new_mask[i]) {
            if (instance->ex_fp) {
                idx ++;
                fprintf(instance->ex_fp, "%f ", expr[i]);
            }
        }
    }
    if (instance->ex_fp) fprintf(instance->ex_fp, "\n");

    if (instance->opencv_show_flag) {
        int index = 0;
        Mat image(height, width, CV_8UC3);
        for (int r = 0; r < image.rows; ++r) {
            uint8_t *img = image.ptr<uint8_t>(r);
            for (int c = 0; c < image.cols * image.channels(); c += image.channels()) {
                img[c]     = data[index++];
                img[c + 1] = data[index++];
                img[c + 2] = data[index++];
                index++;
            }
        }
        for (int i = 0, idx= 0; i < land_n; i += 3) {
            if (dde_tracker->is_mouth_landmark(i / 3)) {
                circle(image, Point(-land[i + 0] + 200, -land[i + 1] + 200), 1, Scalar(255, 0, 0), -1);
            }
        }
        if (instance->warp_show_flag) {
            instance->warpper->simple_warp(mouth_lm_shift);
        }
        imshow("test", image);
        waitKey(1);
    }

    if (instance->lm_fp) fclose(instance->lm_fp);
    if (instance->ex_fp) fclose(instance->ex_fp);
    instance->lm_fp = NULL;
    instance->ex_fp = NULL;
    return;
}

bool Video::set_parameter(string name, string value) {
    if (!instance) return false;
    if (name == "opencv") {
        if (value == "true") {
            instance->opencv_show_flag = true;
            return true;
        }
        else if (value == "false") {
            instance->opencv_show_flag = false;
            return true;
        }
        else return false;
    }
    else if (name == "warp") {
        if (value == "true") {
            instance->warp_show_flag = true;
            return true;
        }
        else if (value == "false") {
            instance->warp_show_flag = false;
            return true;
        }
        else return false;
    }
    else if (name == "landmark_output") {
        if (instance->lm_fp) fclose(instance->lm_fp);
        instance->lm_fp = fopen(value.c_str(), "w");
        return true;
    }
    else if (name == "expression_output") {
        if (instance->ex_fp) fclose(instance->ex_fp);
        instance->ex_fp = fopen(value.c_str(), "w");
        return true;
    }
    return false;
}