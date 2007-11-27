#ifdef __cplusplus
extern "C" {
#endif

void dll_init(char *argv0);
struct DLL *dll_load(char *filename);
struct DLL *dll_force_load(char *filename);
void dll_unload(struct DLL *dll);
void *dll_lookup(struct DLL *dll, char *symbol);
void dll_register(char *symbol, void *address);

#ifdef __cplusplus
}
#endif

