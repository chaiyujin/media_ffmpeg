#include "audio.h"
#include "media.h"
using namespace std;

int main() {
    Media::initialize();
    Media::load_media("test.mp4");
    Media::set_parameters("pcm", "test.pcm");
    Media::process();
    Audio::play_pcm("test.pcm");
    
    system("pause");
    return 0;
}