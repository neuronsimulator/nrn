#include <../../nrnconf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <nrnversion.h>
#include <nrnconfigargs.h>

extern int nrn_global_argc;
extern char** nrn_global_argv;

extern int nrn_main_launch; /* 1 if nrniv, 2 if python, 0 if unknownn */

static char buf[256];
static char* sarg = 0;
static char configargs[] = NRN_CONFIG_ARGS;

#if defined(HG_BRANCH)
char* nrn_version(int i) {
	char head[50];
	char *cp;
	int b;
	buf[0] = '\0';
	if (strncmp(HG_TAG, "Release", 7) == 0) {
		sprintf(head, "%s (%s:%s)", HG_TAG, HG_LOCAL, HG_CHANGESET);
	}else if (strncmp(HG_BRANCH, "Release", 7) == 0) {
		sprintf(head, "%s (%s:%s)", HG_BRANCH, HG_LOCAL, HG_CHANGESET);
	}else{
		sprintf(head, "VERSION %s.%s %s%s(%s:%s)",
			NRN_MAJOR_VERSION, NRN_MINOR_VERSION,
			strcmp("trunk", HG_BRANCH) ? HG_BRANCH : "",
			strcmp("trunk", HG_BRANCH) ? " " : "",
			HG_LOCAL, HG_CHANGESET);
	}
	if (i == 0) {
		sprintf(buf, "%s.%s", NRN_MAJOR_VERSION, NRN_MINOR_VERSION);
	}else if (i == 2) {
		sprintf(buf, "%s", head);
	}else if (i == 3) {
		sprintf(buf, "%s", HG_CHANGESET);
	}else if (i == 4) {
		sprintf(buf, "%s", HG_DATE);
	}else if (i == 5) {
		sprintf(buf, "%s", HG_LOCAL);
	}else if (i == 6) {
		return configargs;
	}else if (i == 7) {
		int j, size;
		if (!sarg) {
			char* c;
			int size = 0;
			for (j=0; j < nrn_global_argc; ++j) {
				size += strlen(nrn_global_argv[j]) + 1;
			}
			sarg = (char*)calloc(size+1, sizeof(char));
			c = sarg;
			for (j=0; j < nrn_global_argc; ++j) {
				sprintf(c, "%s%s", j?" ":"", nrn_global_argv[j]);
				c = c + strlen(c);
			}
		}
		return sarg;
	}else if (i == 8) {
		sprintf(buf, "%s", NRNHOST);	
	}else if (i == 9) {
		sprintf(buf, "%d", nrn_main_launch);
	}else{
		sprintf(buf, "NEURON -- %s %s", head, HG_DATE); 
	}
	return buf;
}
#else
char* nrn_version(int i) {
	char head[50];
	char tail[50];
	char *cp;
	int b;
	buf[0] = '\0';
	if (strncmp(SVN_BRANCH, "Release", 7) == 0) {
		sprintf(head, "%s", SVN_BRANCH);
	}else{
		sprintf(head, "VERSION %s.%s%s%s",
			NRN_MAJOR_VERSION, NRN_MINOR_VERSION,
			((strcmp(SVN_BRANCH, "") == 0) ? "" : "."), SVN_BRANCH);
	}
	sprintf(tail, " %s", SVN_CHANGESET);
	b = 0;
	for (cp = tail; *cp; ++cp) {
		if (*cp == ' ' || *cp == '(' || *cp == ')' ||
		  (*cp >= '0' && *cp <= '9')) {
			;
		}else{
			b = 1;
			break;
		}
	}
	if (b == 0) {
		tail[0] = '\0';
	}
	if (i == 0) {
		sprintf(buf, "%s.%s", NRN_MAJOR_VERSION, NRN_MINOR_VERSION);
	}else if (i == 2) {
		sprintf(buf, "%s.%s", head,SVN_TREE_CHANGE);
	}else if (i == 3) {
		sprintf(buf, "%s", SVN_BASE_CHANGESET);
	}else if (i == 4) {
		sprintf(buf, "%s", SVN_DATE);
	}else if (i == 5) {
		sprintf(buf, "%s", SVN_CHANGESET);
	}else{
		sprintf(buf, "NEURON -- %s.%s (%s) %s%s", head,
			SVN_TREE_CHANGE,
			SVN_BASE_CHANGESET, SVN_DATE,
			tail); 
	}
	return buf;
}
#endif
