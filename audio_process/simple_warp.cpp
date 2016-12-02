#include "simple_warp.h"

const int mouth_lm_index[] = {
    3, 4, 5, 6, 7, 8, 9, 10, 11,
    38, 39, 40,
    46, 47, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 58, 59, 60, 61, 62, 63
};

float cross(float x0, float y0, float x1, float y1) {
    return x0 * y1 - x1 * y0;
}

bool SimpleWarp::in_triangle(int x, int y, int pi0, int pi1, int pi2) {
    bool f0, f1, f2;
    float x0, y0, x1, y1;
    // 1
    x0 = new_lm[pi1 * 2]     - new_lm[pi0 * 2];
    y0 = new_lm[pi1 * 2 + 1] - new_lm[pi0 * 2 + 1];
    x1 = x - new_lm[pi0 * 2];
    y1 = y - new_lm[pi0 * 2 + 1];
    f0 = cross(x0, y0, x1, y1) > 0;
    // 2
    x0 = new_lm[pi2 * 2]     - new_lm[pi1 * 2];
    y0 = new_lm[pi2 * 2 + 1] - new_lm[pi1 * 2 + 1];
    x1 = x - new_lm[pi1 * 2];
    y1 = y - new_lm[pi1 * 2 + 1];
    f1 = cross(x0, y0, x1, y1) > 0;
    // 3
    x0 = new_lm[pi0 * 2]     - new_lm[pi2 * 2];
    y0 = new_lm[pi0 * 2 + 1] - new_lm[pi2 * 2 + 1];
    x1 = x - new_lm[pi2 * 2];
    y1 = y - new_lm[pi2 * 2 + 1];
    f2 = cross(x0, y0, x1, y1) > 0;
    if ((f0 && f1 && f2) || (!f0 && !f1 && !f2)) return true;
    return false;
}

bool solve_linear_system2x2(const float A[2][2],
    const float B[2], float *x0, float *x1) {
    float det = A[0][0] * A[1][1] - A[0][1] * A[1][0];
    if (fabsf(det) < 1e-10f) {
        return false;
    }
    *x0 = (A[1][1] * B[0] - A[0][1] * B[1]) / det;
    *x1 = (A[0][0] * B[1] - A[1][0] * B[0]) / det;
    if (isnan(*x0) || isnan(*x1)) {
        return false;
    }
    return true;
}

Scalar SimpleWarp::calc_origin_color( Mat &img, int x, int y, int pi0, int pi1, int pi2) {
    Scalar ret(255, 0, 0);
    float t0, t1;
    float A[2][2] = {
        { new_lm[pi0 * 2] - new_lm[pi2 * 2], 
          new_lm[pi1 * 2] - new_lm[pi2 * 2] },
        { new_lm[pi0 * 2 + 1] - new_lm[pi2 * 2 + 1], 
          new_lm[pi1 * 2 + 1] - new_lm[pi2 * 2 + 1] }
    };
    float B[2] = {
        x - new_lm[pi2 * 2],
        y - new_lm[pi2 * 2 + 1]
    };
    solve_linear_system2x2(A, B, &t0, &t1);
    //cout << t0 << " " << t1 << endl;
    int xx = t0 * old_lm[pi0 * 2]     + t1 * old_lm[pi1 * 2]     + (1 - t0 - t1) * old_lm[pi2 * 2];
    int yy = t0 * old_lm[pi0 * 2 + 1] + t1 * old_lm[pi1 * 2 + 1] + (1 - t0 - t1) * old_lm[pi2 * 2 + 1];
    uint8_t *data = img.ptr<uint8_t>((int)yy);
    ret[0] = data[xx * img.channels()];
    ret[1] = data[xx * img.channels() + 1];
    ret[2] = data[xx * img.channels() + 2];

    return ret;
}
