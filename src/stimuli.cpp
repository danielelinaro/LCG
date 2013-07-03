#include "stimuli.h"
#include "generate_trial.h"
#include "common.h"
#include "utils.h"
#include "engine.h"
using namespace lcg;

char stimulus_files[MAX_NO_STIMULI][FILENAME_MAXLEN];
double *stimuli[MAX_NO_STIMULI];
uint stimulus_length;
double trial_duration;

void free_stimuli(uint nstim)
{
        for (int i=0; i<nstim; i++) free(stimuli[i]);
}

int allocate_stimuli(uint nstim)
{
        int i, err;
        uint stimlen;

        for (i=0; i<nstim; i++) {
                err = generate_trial(stimulus_files[i], GetLoggingLevel() <= Debug,
                              0, NULL, &stimuli[i], &stimlen,
                              1.0/GetGlobalDt(), GetGlobalDt());
                if (err) {
                        Logger(Critical, "Error in generate_trial.\n");
                        break;
                }
                if (i == 0) {
                        stimulus_length = stimlen;
                        trial_duration = stimulus_length * GetGlobalDt();
                }
                else if (stimlen != stimulus_length) {
                        Logger(Critical, "Stimulus files have different durations.\n");
                        break;
                }
                Logger(Debug, "Successfully read stimulus file [%s].\n", stimulus_files[i]);
        }
        if (i == nstim) {
                return 0;
        }
        else {
                free_stimuli(i+1);
                return -1;
        }
}

