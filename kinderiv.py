#!/usr/bin/python

# read the translated mod files and construct _kinderiv.h

import os

fnames = [f.replace('.mod', '.c') for f in os.listdir('.') if f.endswith('.mod')]
deriv = []
kin = []
for fname in fnames:
  f = open(fname, "r")
  for line in f:
    word = line.split()
    if len(word) > 3:
      if word[0] == '/*' and word[1] == '_derivimplic_':
        deriv.append([word[2], word[3], fname, word[1]])
      if word[0] == '/*' and word[1] == '_kinetic_':
        kin.append([word[2], word[3], fname, word[1]])
  f.close()

fout = open("_kinderiv.h", "w")
fout.write('''
#ifndef _kinderiv_h
#define _kinderiv_h
''')

fout.write("\n/* data used to construct this file */\n")
for l in [deriv, kin]:
  for item in l:
    fout.write('/*')
    for word in item:
      fout.write(' %s' % word)
    fout.write(' */\n')

fout.write("\n/* declarations */\n")
for item in deriv:
  fout.write('#pragma acc routine seq\n')
  fout.write('extern int %s%s(_threadargsproto_);\n' % (item[0], item[1]))
  fout.write('#pragma acc routine seq\n')
  fout.write('extern int _cb_%s%s(_threadargsproto_);\n' % (item[0], item[1]))

for item in kin:
  fout.write('#pragma acc routine seq\n')
  fout.write('extern int %s%s(void*, double*, _threadargsproto_);\n' % (item[0], item[1]))

fout.write("\n/* callback indices */\n")
derivoffset = 1
kinoffset = 1
for i, item in enumerate(deriv):
  fout.write('#define _derivimplic_%s%s %d\n' % (item[0], item[1], i + derivoffset))
for i, item in enumerate(kin):
  fout.write('#define _kinetic_%s%s %d\n' % (item[0], item[1], i + kinoffset))

fout.write("\n/* switch cases */\n")
fout.write("\n#define _NRN_DERIVIMPLIC_CASES \\\n")
for item in deriv:
  fout.write("  case _derivimplic_%s%s: %s%s(_threadargs_); break; \\\n" % (item[0], item[1], item[0], item[1]))
fout.write("\n")

fout.write("\n#define _NRN_DERIVIMPLIC_CB_CASES \\\n")
for item in deriv:
  fout.write("  case _derivimplic_%s%s: _cb_%s%s(_threadargs_); break; \\\n" % (item[0], item[1], item[0], item[1]))
fout.write("\n")

fout.write("\n#define _NRN_KINETIC_CASES \\\n")
for item in kin:
  fout.write("  case _kinetic_%s%s: %s%s(so, rhs, _threadargs_); break; \\\n" % (item[0], item[1], item[0], item[1]))
fout.write("\n")

fout.write('\n#endif\n')
fout.close()
