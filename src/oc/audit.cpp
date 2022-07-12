#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hocdec.h>
#include "hocassrt.h"


static int doaudit;
static FILE* faudit;
static FILE* audit_pipe;

typedef struct RetrieveAudit {
    int mode;
    int id;
    FILE* pipe;
} RetrieveAudit;

static RetrieveAudit retrieve_audit;

static void pipesend(int type, const char* s);

/*
Notes:

The goal is to support easy reconstruction of a simulation while realizing
that the user may not know whether to save the information til she sees
something that she feels is worth saving. If he does want to save then
a saveaudit() command is issued.

RCS checking of files
In order to not delay the main process while managing rcs files (deciding
whether a file needs to be checked in with rcsdiff and ci) we merely
send the file name through a pipe to another process which will asynchronously
maintain a list of xopen statements with the proper rcs version number.
*/

#define AUDIT_SCRIPT_DIR "$NEURONHOME/lib/auditscripts"
#define AUDIT_DIR        "AUDIT"


static void hoc_audit_init(void) {
#if !OCSMALL
    if (retrieve_audit.mode) {
        /* clean up. there must have been an execerror */
        retrieve_audit.mode = 0;
        retrieve_audit.id = 0;
        if (retrieve_audit.pipe) {
            pclose(retrieve_audit.pipe);
            retrieve_audit.pipe = (FILE*) 0;
        }
    }
#endif
}

void hoc_audit_from_hoc_main1(int argc, const char** argv, const char** envp) {
#if !OCSMALL
    /*ARGSUSED*/
    int i;
    char buf[200];

    hoc_on_init_register(hoc_audit_init);

#if 0
	if (getenv("HOCAUDIT")) {
		doaudit = 1;
		printf("auditing\n");
	}else{
		doaudit = 0;
		printf("no auditing\n");
	}
#endif

    if (!doaudit) {
        return;
    }
    /* since file open for entire session will have to make the name unique*/
    sprintf(buf, "if [ ! -d %s ] ; then mkdir %s ; fi", AUDIT_DIR, AUDIT_DIR);
    nrn_assert(system(buf) >= 0);
    sprintf(buf, "mkdir %s/%d", AUDIT_DIR, hoc_pid());
    nrn_assert(system(buf) >= 0);
    sprintf(buf, "%s/hocaudit.sh %d %s", AUDIT_SCRIPT_DIR, hoc_pid(), AUDIT_DIR);
    if ((audit_pipe = popen(buf, "w")) == (FILE*) 0) {
        hoc_warning("Could not connect to hocaudit.sh via pipe:", buf);
        doaudit = 0;
        return;
    }
    if (hoc_saveaudit() == 0) {
        return;
    }
    fprintf(faudit, "/*\n");
    for (i = 0; i < argc; ++i) {
        fprintf(faudit, " %s", argv[i]);
    }
    fprintf(faudit, "\n*/\n");
    fflush(faudit);
    for (i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            fprintf(faudit, "xopen(\"%s\")\n", argv[i]);
            hoc_audit_from_xopen1(argv[i], (char*) 0);
        }
    }
    fprintf(faudit, "\n");
#endif
}

#if !OCSMALL
static void pipesend(int type, const char* s) {
    int err;
    if (audit_pipe) {
        err = fprintf(audit_pipe, "%d %s\n", type, s);
        if (err == EOF) {
            hoc_warning("auditing failed in pipesend", "turning off");
            doaudit = 0;
            audit_pipe = 0;
            return;
        }
        fflush(audit_pipe);
    }
}
#endif
void hoc_audit_command(const char* buf) {
#if !OCSMALL
    if (doaudit) {
        fprintf(faudit, "%s", buf);
    }
#endif
}

void hoc_audit_from_xopen1(const char* fname, const char* rcs) {
#if !OCSMALL
    if (!hoc_retrieving_audit() && doaudit && !rcs) {
        pipesend(1, fname);
    }
#endif
}

void hoc_audit_from_final_exit(void) {
#if !OCSMALL
    if (faudit) {
        fclose(faudit);
        faudit = 0;
    }
    if (audit_pipe) {
        pclose(audit_pipe);
        audit_pipe = 0;
    }
    doaudit = 0;
#endif
}

void hoc_Saveaudit(void) {
    int err;
#if !OCSMALL
    err = hoc_saveaudit();
#endif
    hoc_ret();
    hoc_pushx((double) err);
}

int hoc_saveaudit(void) {
#if !OCSMALL
    static int n = 0;
    char buf[200];
    if (hoc_retrieving_audit() || !doaudit) {
        return 0;
    }
    if (faudit) {
        fclose(faudit);
        faudit = 0;
        sprintf(buf, "hocaudit%d", n);
        pipesend(3, buf);
        ++n;
    }
    sprintf(buf, "%s/%d/hocaudit%d", AUDIT_DIR, hoc_pid(), n);
    if ((faudit = fopen(buf, "w")) == (FILE*) 0) {
        hoc_warning("NO audit. fopen failed for:", buf);
        doaudit = 0;
        return 0;
    }
#endif
    return 1;
}

int hoc_retrieving_audit(void) {
#if !OCSMALL
    return retrieve_audit.mode;
#else
    return 0;
#endif
}

void hoc_Retrieveaudit(void) {
    int err, id;
#if !OCSMALL
    if (ifarg(1)) {
        id = (int) chkarg(1, 0., 1e7);
    } else {
        id = 0;
    }
#endif
    err = hoc_retrieve_audit(id);
    hoc_ret();
    hoc_pushx((double) err);
}

static void xopen_audit(void) {
#if !OCSMALL
    char buf[200], *bp;
    constexpr auto rm_str = "rm ";
    strcpy(buf, rm_str);
    bp = buf + strlen(buf);
    /* get the temporary file name */
    nrn_assert(fgets(bp, 200 - strlen(rm_str), retrieve_audit.pipe));
    /*printf("xopen_audit: %s", bp);*/
    bp[strlen(bp) - 1] = '\0';
    hoc_xopen1(bp, "");
#if 1
    nrn_assert(system(buf) >= 0);
#endif
#endif
}

#ifdef NeXT
int hoc_retrieve_audit(int id) /* I have no idea why... CMC */
#else
int hoc_retrieve_audit(int id)

#endif
{
#if !OCSMALL
    RetrieveAudit save;
    char buf[200];
    char retdir[200];
    save = retrieve_audit;
    /*printf("retrieve audit id=%d\n", id);*/
    retrieve_audit.mode = 1;
    retrieve_audit.id = id;

    sprintf(buf, "%s/retrieve.sh %d %s", AUDIT_SCRIPT_DIR, id, AUDIT_DIR);
    if ((retrieve_audit.pipe = popen(buf, "r")) == (FILE*) 0) {
        hoc_execerror("Could not connect via pipe:", buf);
    }
    nrn_assert(fgets(retdir, 200, retrieve_audit.pipe));
    xopen_audit();
    nrn_assert(!fgets(buf, 200, retrieve_audit.pipe));
    /*	pclose(retrieve_audit.pipe);*/
    retrieve_audit = save;
    fprintf(stderr, "should now delete %s", retdir);
#endif
    return 1;
}

void hoc_xopen_from_audit(const char* fname) {
#if !OCSMALL
    char buf[200];
    /* check the synchronization */
    nrn_assert(fgets(buf, 200, retrieve_audit.pipe));
    buf[strlen(buf) - 1] = '\0';
    if (strncmp(buf, fname, strlen(fname)) != 0) {
        fprintf(stderr, "Warning: xopen_from_audit files have different names %s %s\n", fname, buf);
    }
    xopen_audit();
#endif
}
