extern int main1(int argc, char** argv, char** env);
extern "C" {extern void modl_reg(void);}

int main(int argc, char** argv, char** env) {
  return main1(argc, argv, env);
}

/// Declare an empty function if Neurodamus mechanisms are not used, otherwise register them in mechs/cfile/mod_func.c
#ifndef ADDITIONAL_MECHS
void modl_reg() {}
#endif
