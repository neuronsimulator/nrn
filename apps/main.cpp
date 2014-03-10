extern int main1(int argc, char** argv, char** env);
extern "C" {extern void modl_reg(void);}

int main(int argc, char** argv, char** env) {
  return main1(argc, argv, env);
}

void modl_reg() {}
