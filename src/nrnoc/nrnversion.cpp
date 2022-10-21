#include <../../nrnconf.h>
#include "nrn_ansi.h"
#include "nrnassrt.h"
#include "nrnconfigargs.h"
#include "nrnversion.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


extern int nrn_global_argc;
extern char** nrn_global_argv;

extern int nrn_main_launch; /* 1 if nrniv, 2 if python, 0 if unknownn */

static char* ver[10];
static char* sarg = 0;
static char configargs[] = NRN_CONFIG_ARGS;

#if !defined(GIT_BRANCH)
#define GIT_DATE      "2018-08-24"
#define GIT_BRANCH    "unknown"
#define GIT_CHANGESET "d3ead4a+"
#define GIT_DESCRIBE  "7.6.2-2-gd3ead4a+"
#endif

char* nrn_version(int i) {
    char buf[1024];
    char head[1024];
    char* cp;
    int b;
    buf[0] = '\0';
    if (strncmp(GIT_BRANCH, "Release", 7) == 0) {
        sprintf(head, "%s (%s)", GIT_BRANCH, GIT_CHANGESET);
    } else {
        sprintf(head, "VERSION %s %s (%s)", GIT_DESCRIBE, GIT_BRANCH, GIT_CHANGESET);
    }
    if (i == 0) {
        sprintf(buf, "%s", PACKAGE_VERSION);
    } else if (i == 2) {
        sprintf(buf, "%s", head);
    } else if (i == 3) {
        sprintf(buf, "%s", GIT_CHANGESET);
    } else if (i == 4) {
        sprintf(buf, "%s", GIT_DATE);
    } else if (i == 5) {
        sprintf(buf, "%s", GIT_DESCRIBE);
    } else if (i == 6) {
        return configargs;
    } else if (i == 7) {
        int j, size;
        if (!sarg) {
            char* c;
            int size = 0;
            for (j = 0; j < nrn_global_argc; ++j) {
                size += strlen(nrn_global_argv[j]) + 1;
            }
            sarg = (char*) calloc(size + 1, sizeof(char));
            c = sarg;
            for (j = 0; j < nrn_global_argc; ++j) {
                sprintf(c, "%s%s", j ? " " : "", nrn_global_argv[j]);
                c = c + strlen(c);
            }
        }
        return sarg;
    } else if (i == 8) {
        sprintf(buf, "%s", NRNHOST);
    } else if (i == 9) {
        sprintf(buf, "%d", nrn_main_launch);
    } else {
        nrn_assert(snprintf(buf, 1024, "NEURON -- %s %s", head, GIT_DATE) < 1024);
    }

    if (i > 9) {
        i = 1;
    }
    if (!ver[i]) {
        ver[i] = strdup(buf);
    }

    return ver[i];
}

std::size_t nrn_num_config_keys() {
    return neuron::config::arguments.size();
}

char* nrn_get_config_key(std::size_t i) {
    nrn_assert(i < nrn_num_config_keys());
    return const_cast<char*>(std::next(neuron::config::arguments.begin(), i)->first);
}

char* nrn_get_config_val(std::size_t i) {
    nrn_assert(i < nrn_num_config_keys());
    return const_cast<char*>(std::next(neuron::config::arguments.begin(), i)->second);
}
