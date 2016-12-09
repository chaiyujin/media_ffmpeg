#include "audio.h"
#include "media.h"
#include "collector.h"
#include <fstream>
#define X_STEP 3.3
#define Y_STEP 1.0
using namespace std;

void init();
void run_from_json(const char *file_name);
void process_raw_landmarks(string source, string target, int k_x, int k_y);

int main() {
    init();
    //run_from_json("data/config.json");
    process_raw_landmarks("processed_back", "train", 11, 5);

    system("pause");
    return 0;
}

void init() {
    Media::initialize();
    Media::set_parameter("opencv", "true");
    //Media::set_parameter("warp", "true");
}

void run_from_json(const char *file_name) {
   
    for (int i = 0; i < 2; ++i) {
        char media[100];
        sprintf(media, "data/%d.mp4", i);
        Media::load_media(media);
        Media::process();
    }
    //Audio::play_pcm("processed/data0.pcm");
   
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
                    if (fin >> x) {
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
    }
}