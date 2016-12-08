#include "audio.h"
#include "media.h"
#include "landmark_collect.h"
#include <fstream>
using namespace std;

void init();
void run_from_json(const char *file_name);
void process_raw_landmarks();

int main() {
    init();
    //run_from_json("data/config.json");
    process_raw_landmarks();

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

void process_raw_landmarks() {
    Collector::find_all_data("processed_back", "land");
    Collector::pca();
}