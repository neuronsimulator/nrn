#ifdef __cplusplus
extern "C" {
#endif
void DebugEnterSection();
void DebugExitSection();
void DebugMessage(char *szMessage, ...);
void DebugEnterMessage(char *szMessage, ...);
void DebugExitMessage(char *szMessage, ...);
//BOOL DebugStartSection(void *nId, char *szName);
//BOOL DebugRemoveSection(void *nId, char *szName);
//BOOL DebugEndSection(void *nId, char *szName);
//int DebugGetSectionTime(void *nId, char *szName, float *pfPercent);
#ifdef __cplusplus
}
#endif
