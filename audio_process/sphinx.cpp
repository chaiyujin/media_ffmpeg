#include "sphinx.h"

Sphinx *Sphinx::instance = NULL;

void Sphinx::initialize() {
    if (instance) return;
    instance = new Sphinx();
    return;
}

void Sphinx::run_pcm(const char *file_name, const char *output_name) {
    instance->config = cmd_ln_init(NULL, ps_args(), TRUE,
        "-hmm", MODEL_DIR "en-us/en-us",
        "-allphone", MODEL_DIR "en-us/en-us-phone.lm.bin",
        "-allphone_ci", "yes",
        "-beam", "1e-20", "-pbeam", "1e-20", "-lw", "2.0",
        "-logfn", "nul",
        NULL);

    if (instance->config == NULL) {
        fprintf(stderr, "Failed to create config object, see log for details\n");
        system("pause");
        exit(1);
    }

    instance->ps = ps_init(instance->config);
    if (instance->ps == NULL) {
        fprintf(stderr, "Failed to create recognizer, see log for details\n");
        system("pause");
        exit(1);
    }

    instance->fh = fopen(file_name, "rb");
    if (instance->fh == NULL) {
        fprintf(stderr, "Unable to open input file goforward.raw\n");
        system("pause");
        exit(1);
    }

    instance->rv = ps_start_utt(instance->ps);
    while (!feof(instance->fh)) {
        size_t nsamp;
        nsamp = fread(instance->buf, 2, 512, instance->fh);
        instance->rv = ps_process_raw(instance->ps, instance->buf, nsamp, FALSE, FALSE);
    }

    FILE *phoneme = fopen(output_name, "w");
    printf("Try to recognize.\n");
    ps_seg_t *t = ps_seg_iter(instance->ps);
    do {
        //printf("%s %d %d\n", t->word, t->sf, t->ef);
        for (int i = t->sf; i <= t->ef; ++i) 
            fprintf(phoneme, "%s ", t->word);
    } while(t = ps_seg_next(t));


    fclose(phoneme);
    fclose(instance->fh);
    ps_free(instance->ps);
    cmd_ln_free_r(instance->config);

    return;
}
