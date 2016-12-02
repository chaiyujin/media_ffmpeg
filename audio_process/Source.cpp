#include "audio.h"
#include "media.h"
#include <fstream>
using namespace std;

void init();
void run_from_json(const char *file_name);

int main() {
    init();
    run_from_json("data/config.json");
    
    system("pause");
    return 0;
}

void init() {
    Media::initialize();
    Media::set_parameter("opencv", "true");
    //Media::set_parameter("warp", "true");
}

void run_from_json(const char *file_name) {
   
    for (int i = 0; i < 3; ++i) {
        char media[100];
        char pcm[100];
        char lm[100];
        char ex[100];
        char phone[100];
        sprintf(media, "data/short%d.mp4", i);
        sprintf(pcm,   "processed/short%d.pcm", i);
        sprintf(lm,    "processed/short%d.land", i);
        sprintf(ex,    "processed/short%d.expr", i);
        sprintf(phone, "processed/short%d.phone", i);
        Media::load_media(media);
        Media::set_parameter("pcm", pcm);
        Media::set_parameter("landmark_output", lm);
        Media::set_parameter("expression_output", ex);
        Media::process();
        Sphinx::run_pcm(pcm, phone);
    }
    //Audio::play_pcm("data/short0.pcm");
   
    return;

    
}