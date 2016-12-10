#include "audio.h"
#include "media.h"
#include "collector.h"
#include "directory.h"
#include <fstream>
#define X_STEP 3.3
#define Y_STEP 1.0
using namespace std;
using namespace Yuki;

void init();
void run_from_json(const char *file_name);
void process_raw_landmarks(string source, string target, int k_x, int k_y);
void predict();
void show();

int main() {
    init();
    //run_from_json("data/config.json");
    process_raw_landmarks("processed_back", "train", 11, 5);
    //train();
    predict();
    show();

    system("pause");
    return 0;
}

void init() {
    Media::initialize();
    Media::set_parameter("opencv", "true");
    //Media::set_parameter("warp", "true");
}

void run_from_json(const char *file_name) {
   
    for (int i = 1; i < 2; ++i) {
        char media[100];
        sprintf(media, "data/%d.mp4", i);
        Media::load_media(media);
        Media::process();
    }
    Audio::play_pcm("processed/data0.pcm");
   
    return;

    
}

void process_raw_landmarks(string source, string dir, int kx, int ky) {
    for (int i = 0; i < dir.length(); ++i) {
        if (dir[i] == '\\') dir[i] = '/';
    }
    if (dir[dir.length() - 1] != '/') dir += '/';
    ofstream xout(dir + "X.bin", std::ios::binary);
    ofstream yout(dir + "Y.bin", std::ios::binary);

    Collector::find_all_data(source);
    Collector::pca_landmarks();

    vector<string> file_paths;
    if (Collector::get_first_data(file_paths)) {
        int count = 0;
        int xlen = (double)kx * X_STEP;
        int ylen = (double)ky * Y_STEP;
        int ycol = -1;
        do {
            // lm
            vector<vector<float> > data_list;
            vector<float> data;
            vector<float> prjt;
            ifstream fin(file_paths[0]);
            bool flag = true;
            while (flag) {
                data.clear();
                for (int j = 0; j < MOUTH_LANDMARKS * 2; ++j) {
                    float x;
                    if (!fin.eof()) {
                        fin >> x;
                        data.push_back(x);
                    }
                    else {
                        flag = false;
                        break;
                    }
                }
                if (flag) {
                    prjt = Collector::project_lm(data);
                    data_list.push_back(prjt);
                }
            }
            fin.close();
            // phone
            vector<string> phone_list;
            fin.open(file_paths[1]);
            string phone;
            while (fin >> phone) {
                phone_list.push_back(phone);
            }
            fin.close();

            // output to binary file
            
            ycol = prjt.size();

            for (double xi = 0, yi = 0;
                 ((int)xi + xlen < phone_list.size()) &&
                 ((int)yi + ylen < data_list.size());
                 xi += X_STEP,
                 yi += Y_STEP) {

                ++count;
                for (int i = (int)xi; i < (int)xi + xlen; ++i) {
                    int x = (Collector::get_id_of_phoneme(phone_list[i]));
                    xout.write((char *)&x, sizeof(int));
                }

                for (int i = (int)yi; i < (int)yi + ylen; ++i) {
                    for (int j = 0; j < ycol; j++) {
                        yout.write((char *)&data_list[i][j], sizeof(float));
                    }
                }
            }
            
        } while (Collector::get_next_data(file_paths));
        cout << "Spawn " << count << " data.\n"
                 << "X: " << xlen << " x 1\n"
                 << "Y: " << ylen << " x " << ycol << endl;
        xout.close();
        yout.close();
    }
}

void predict() {
    ifstream fin("D:/todo/Phone/Projects/audio_process/audio_process/processed/data14.phone");
    ofstream xout("D:/todo/Phone/Projects/audio_process/audio_process/python/x_input", std::ios::binary);
    vector<string> phone_list;
  
    string phone;
    while (fin >> phone) {
        phone_list.push_back(phone);
    }
    fin.close();

    int xlen = 36;

    int count = 0;

    for (double xi = 0, yi = 0;
        ((int)xi + xlen < phone_list.size());
        xi += X_STEP) {
        ++count;
        for (int i = (int)xi; i < (int)xi + xlen; ++i) {
            int x = (Collector::get_id_of_phoneme(phone_list[i]));
            xout.write((char *)&x, sizeof(int));
        }
    }

    xout.close();
    fin.close();

    DirBrowser::set_directory("D:/todo/Phone/Projects/audio_process/audio_process/python");
    system("pwd");
    system("python predict.py");
}

void show() {
    ifstream fin("D:/todo/Phone/Projects/audio_process/audio_process/python/output.txt");
    ifstream fland("D:/todo/Phone/Projects/audio_process/audio_process/processed/data14.land");
    
    int data_size, cols;
    vector<vector<float> > data_list;
    fin >> data_size >> cols;

    for (int i = 0; i < data_size; ++i) {
        float x;
        for (int s = 0; s < 5; ++s) {
            vector<float> data;
            for (int j = 0; j < 41; ++j) {
                fin >> x;
                data.push_back(x);
            }
            data_list.push_back(data);
        }
    }
    fin.close();
    // blend
    vector<vector<float> > orig_list;
    vector<vector<float> > land_list;
    for (int i = 0; i < data_size; ++i) {
        int idx = i * 5;
        int si = idx - 4; if (si < 0) si = 0;
        int ei = idx + 1;
        vector<float> data;
        vector<float> orig;
        for (int j = 0; j < 41; ++j) data.push_back(0);
        for (idx = si; idx < ei; idx++) {
            for (int j = 0; j < 41; ++j) {
                data[j] += data_list[idx][j];
            }
        }
        for (int j = 0; j < 41; ++j) data[j] /= float(ei - si);
        orig = Collector::back_project_lm(data);
        orig_list.push_back(orig);
    }

    vector<float> data;
    bool flag = true;
    while (flag) {
        data.clear();
        for (int j = 0; j < MOUTH_LANDMARKS * 2; ++j) {
            float x;
            if (!fland.eof()) {
                fland >> x;
                data.push_back(x);
            }
            else {
                flag = false;
                break;
            }
        }
        if (flag) {
            land_list.push_back(data);
        }
    }
    fland.close();

    DDETracker * dde = Video::get_tracker();
    SimpleWarp * warpper_dt = new SimpleWarp(dde, "D:/todo/Phone/Projects/audio_process/audio_process/data/lm.png", "dt");
    SimpleWarp * warpper_or = Video::get_warpper();

    cout << orig_list.size() << " " << land_list.size() << endl;

    float shift[MOUTH_LANDMARKS * 2];
    float shift2[MOUTH_LANDMARKS * 2];
    for (int i = 0; i < orig_list.size(); ++i) {
        for (int j = 0; j < MOUTH_LANDMARKS * 2; ++j) {
            shift[j] = orig_list[i][j];
            shift2[j] = land_list[i][j];
            //shift2[j] = shift[j];
        }
        warpper_dt->simple_warp(shift);
        warpper_or->simple_warp(shift2);
        waitKey(1);
    }

}