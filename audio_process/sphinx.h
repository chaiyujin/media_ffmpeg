#pragma once
#ifndef __YUKI_SPHINX_H__
#define __YUKI_SPHINX_H__

#define MODEL_DIR "D:/software/dev/cmusphinx/model/"

#include <pocketsphinx.h>
#include <pocketsphinx_internal.h>
#include <cmd_ln.h>

class Sphinx {
private:
    static Sphinx   *instance;

    ps_decoder_t    *ps;
    cmd_ln_t        *config;
    FILE            *fh;
    char const      *hyp, *uttid;
    int16           buf[512];
    int             rv;
    int32           score;
    
public:
    static void initialize();
    static void run_pcm(const char *file_name,
                        const char *output_name);
};

#endif
