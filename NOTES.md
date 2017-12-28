#### Generating bar charts from PerfStat visitor

These are steps followed for generating bar plot for meeting on 18th Dec 2017.

First, get mod files for BBP model (e.g. savestate branch of neurodamus). We are only interested in mod files that are used in simulation. Copy them to some temporary directory :

```
cd $HOME/workarena/repos/bbp/neurodamus/lib/modlib
git checkout sandbox/kumbhar/coreneuron_plasticity

MOD_DIR=$HOME/workarena/repos/bbp/incubator/plots_dec_21_fs_meeting/bbp_mod

mkdir -p $MOD_DIR
while read -u 10 file; do cp $file $MOD_DIR/; done 10<coreneuron_modlist.txt
```

The function/procedure inlining pass is not implemented into nocmodl yet. Get inlined mod files for performance counting:

```
cd $MOD_DIR
mkdir inlined
for file in *.mod; do
	$NOCMODLX_DIR/bin/nocmodl -f $file -a a -i -s s -n inlined/${file};
done
```

Now we can run nocmodl visitor executable:

```
mkdir -p $MOD_DIR/jsons
cd $MOD_DIR/jsons
export NOCMODL=~/workarena/repos/bbp/incubator/nocmodl/cmake-build-debug

for file in $MOD_DIR/inlined/*.mod.in; do
	$NOCMODL/bin/nocmodl_visitor --file $file;
done
```

We can generate JSON data required for plotting as:

```
cd $MOD_DIR/jsons
python ../../json_generator.py
```

Above script print JSON data required for plotting on stdout. We have to copy the output to:

```
../../html_plotting/data.js
```

We can now open the plot from `html_plotting/plots.html`. Material/Scripts for the plotting is stored in:

```
$HOME/workarena/repos/bbp/incubator/plots_dec_21_fs_meeting
```