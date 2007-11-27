#include <windows.h>
#include <stdio.h>
#include "debug.h"
//------------------DEBUGGING CODE-------------------------
int s_nDepth = 0;


void DebugEnterSection()
	{
	s_nDepth += 2;
	}

void DebugExitSection()
	{
	s_nDepth -= 2;
	}

void DebugMessage(char *szMessage, ...)
	{
	char szBuffer[1000];
	char *szTemp = szBuffer;
	int n = s_nDepth;
	while (n)
		{
		if (n > 0)
			{
			*szTemp = ' ';
			n--;
			}
		else if (n < 0)
			{
			*szTemp = '*';
			n++;
			}
		szTemp++;
		}
	char *pArguments = (char *) &szMessage + sizeof(char *);
	vsprintf(szTemp, szMessage, pArguments);
	FILE *f = fopen("\\debug.txt", "a");
	fprintf(f, szBuffer);
	fclose(f);
	}

void DebugEnterMessage(char *szMessage, ...)
	{
	char szBuffer[1000];
	char *szTemp = szBuffer;
	int n = s_nDepth;
	while (n)
		{
		if (n > 0)
			{
			*szTemp = ' ';
			n--;
			}
		else if (n < 0)
			{
			*szTemp = '*';
			n++;
			}
		szTemp++;
		}
	char *pArguments = (char *) &szMessage + sizeof(char *);
	vsprintf(szTemp, szMessage, pArguments);
	FILE *f = fopen("\\debug.txt", "a");
	fprintf(f, szBuffer);
	fclose(f);
	DebugEnterSection();
	}

void DebugExitMessage(char *szMessage, ...)
	{
	DebugExitSection();
	char szBuffer[1000];
	char *szTemp = szBuffer;
	int n = s_nDepth;
	while (n)
		{
		if (n > 0)
			{
			*szTemp = ' ';
			n--;
			}
		else if (n < 0)
			{
			*szTemp = '*';
			n++;
			}
		szTemp++;
		}
	char *pArguments = (char *) &szMessage + sizeof(char *);
	vsprintf(szTemp, szMessage, pArguments);
	FILE *f = fopen("\\debug.txt", "a");
	fprintf(f, szBuffer);
	fclose(f);
	}


struct TimeSection
	{
	int nStartTime, nTotalTime, nId;
	BOOL bInUse, bTiming;
	TimeSection() {bInUse = FALSE;}
	};

static TimeSection s_TimeSection[256];

#define HASH(a) ((BYTE) (((a) & 0xFF) ^ (((a) & 0xFF00) >> 8)))
//#pragma argsused
static TimeSection *GetSection(void *nId, char *szName)
	{
	int nStart = HASH(int(nId));
	int nFinish = nStart - 1;
	if (nFinish == -1) nFinish = 255;
   int i;
	for (i = nStart; i != nFinish; i++)
		{
		if (i == 256) {i = 0; if (nFinish == i) return 0;}
		if (s_TimeSection[i].nId == int(nId) && s_TimeSection[i].bInUse == TRUE)
			return s_TimeSection + i;
		else if (s_TimeSection[i].bInUse == FALSE)
			break;
		}

	s_TimeSection[i].nId = int(nId);
	s_TimeSection[i].bInUse = TRUE;
	s_TimeSection[i].nTotalTime = 0;
	s_TimeSection[i].bTiming = FALSE;
//	s_TimeSection[i].sName = sName;
	return s_TimeSection + i;
	}

BOOL DebugStartSection(void *nId, char *szName)
	{
	TimeSection *pts = GetSection(nId, szName);
	if (pts == NULL) return FALSE;
	if (pts->bTiming) return FALSE;
	pts->bTiming = TRUE;
	pts->nStartTime = timeGetTime();
	return TRUE;
	}

BOOL DebugEndSection(void *nId, char *szName)
	{
	TimeSection *pts = GetSection(nId, szName);
	if (pts == NULL) return FALSE;
	if (pts->bTiming)
		{
		pts->nTotalTime += timeGetTime() - pts->nStartTime;
		pts->bTiming = FALSE;
		return TRUE;
		}
	return FALSE;
	}

BOOL DebugRemoveSection(void *nId, char *szName)
	{
	TimeSection *pts = GetSection(nId, szName);
	if (pts == NULL) return FALSE;
	pts->bInUse = FALSE;
	return TRUE;
	}

int DebugGetSectionTime(void *nId, char *szName, float *pfPercent)
	{
	int nTotalTime = 0, nTime;
	for (int i = 0; i < 256; i++)
		if (s_TimeSection[i].bInUse == TRUE)
			nTotalTime += s_TimeSection[i].nTotalTime;
	TimeSection *pts = GetSection(nId, szName);
	if (pts == NULL) return FALSE;
	nTime = pts->nTotalTime;
	*pfPercent = float(nTime) / float(nTotalTime) * 100.0;
	return nTime;
	}

class CleanUpDebugging
	{
public:
	CleanUpDebugging();
	~CleanUpDebugging();
	};

CleanUpDebugging::CleanUpDebugging()
	{
	char szBuffer[1000];
	wsprintf(szBuffer, "-----Started debugging run on %s at %s-----\n", __DATE__, __TIME__);
	FILE *f = fopen("\\debug.txt", "w");
	fprintf(f, szBuffer);
	fclose(f);
	for (int i = 0; i < 256; i++)
		{
		s_TimeSection[i].bInUse = FALSE;
		s_TimeSection[i].nTotalTime = 0;
		}
	}
#if 0
static _USERENTRY CompFunc(const void *p1, const void *p2)
	{
	if (((TimeSection *) p1)->nTotalTime >((TimeSection *) p2)->nTotalTime)
		return -1;
	return 1;
	}
#endif

#include <stdlib.h>
CleanUpDebugging::~CleanUpDebugging()
	{
	DebugMessage("-----------------------------\n");
	DebugMessage("Totals\n");
	DebugMessage("-----------------------------\n");
	int nTotalTime = 0, nTime;
	float f;
//	qsort(s_TimeSection, 256, sizeof(TimeSection), CompFunc);
   int i;
	for (i = 0; i < 256; i++)
		if (s_TimeSection[i].bInUse)
			nTotalTime += s_TimeSection[i].nTotalTime;
	for (i = 0; i < 256; i++)
		if (s_TimeSection[i].bInUse)
			{
			nTime = s_TimeSection[i].nTotalTime;
			f = float(nTime) / float(nTotalTime) * 100.0;
			DebugMessage("%8x : nTotalTime = %7d\tpercent = %.3f\n", s_TimeSection[i].nId, nTime, f);
			}
	}

CleanUpDebugging cud;

