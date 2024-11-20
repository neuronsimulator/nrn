.. include:: ../rst_substitutions.txt

.. _porting-mechanisms-to-cpp:

Adapting MOD files for C++ with |neuron_with_cpp_mechanisms|
============================================================

.. attention::
  This guide only applies if you have MOD files with VERBATIM blocks that fail
  to compile with |neuron_with_cpp_mechanisms| but work with older versions of
  NEURON. If this is not your case, you can skip this guide.

In older versions of NEURON, MOD files containing NMODL code were translated
into C code before being compiled and executed by NEURON.
Starting with |neuron_with_cpp_mechanisms|, NMODL code is translated into C++
code instead.

In most cases, this does not present any issues, as simple C code is typically
valid C++, and no changes are required.
However, C and C++ are not the same language, and there are cases in which MOD
files containing ``VERBATIM`` blocks need to be modified in order to build with
|neuron_with_cpp_mechanisms|.

Before you start, you should decide if you need your MOD files to be
compatible simultaneously with |neuron_with_cpp_mechanisms| **and** older
versions, or if you can safely stop supporting older versions.
Supporting both is generally possible, but it may be more cumbersome than
committing to using C++ features.
Considering NEURON has maintained strong backward compatibility and its
internal numerical methods haven't changed with migration to C++, it is likely
to be sufficient to adapt MOD files to C++ only and use
|neuron_with_cpp_mechanisms|.
If you do decide to preserve compatibility across versions, the preprocessor
macros described in :ref:`verbatim` may prove useful.

.. note::
  If you have a model that stopped compiling when you upgraded to or beyond
  |neuron_with_cpp_mechanisms|, the first thing that you should check is
  whether the relevant MOD files have already been updated in ModelDB or in the
  GitHub repository of that model. You can check the repository name with the
  model accession number under `<https://github.com/ModelDBRepository>`_.
  An updated version may already be available!

  The following models were updated in ModelDB in preparation for
  |neuron_with_cpp_mechanisms| and may serve as useful references. You can see
  updated MOD files in the `Changed Mod Files` column. You can just apply the changes
  to your MOD files, or you can use the updated MOD files directly from the GitHub repository.

.. list-table::
   :header-rows: 1

   * - ModelDB Id
     - Model Name
     - GitHub Repository
     - Pull Request
     - Changed Mod Files
   * - `2487 <https://modeldb.science/2487>`_
     - Olfactory Mitral Cell (Davison et al 2000)
     - `ModelDBRepository/2487 <https://github.com/ModelDBRepository/2487>`_
     - `pull/1 <https://github.com/ModelDBRepository/2487/pull/1/files>`_
     - cadecay.mod
   * - `2730 <https://modeldb.science/2730>`_
     - Olfactory Bulb Network (Davison et al 2003)
     - `ModelDBRepository/2730 <https://github.com/ModelDBRepository/2730>`_
     - `pull/1 <https://github.com/ModelDBRepository/2730/pull/1/files>`_
     - cadecay.mod
   * - `2733 <https://modeldb.science/2733>`_
     - Olfactory Mitral Cell (Bhalla, Bower 1993)
     - `ModelDBRepository/2733 <https://github.com/ModelDBRepository/2733>`_
     - `pull/1 <https://github.com/ModelDBRepository/2733/pull/1/files>`_
     - cadecay.mod
   * - `3658 <https://modeldb.science/3658>`_
     - Glutamate diffusion ...lar glomerulus (Saftenku 2005)
     - `ModelDBRepository/3658 <https://github.com/ModelDBRepository/3658>`_
     - `pull/1 <https://github.com/ModelDBRepository/3658/pull/1/files>`_
     - glubbfbm.mod glubes2.mod glubes23.mod glubes3.mod glubes4.mod glubes5.mod glubes6.mod glures23.mod
   * - `7399 <https://modeldb.science/7399>`_
     - Feedforward heteroas...with HH dynamics (Lytton 1998)
     - `ModelDBRepository/7399 <https://github.com/ModelDBRepository/7399>`_
     - `pull/1 <https://github.com/ModelDBRepository/7399/pull/1/files>`_
     - matrix.mod snsarr.inc vecst.mod
   * - `7400 <https://modeldb.science/7400>`_
     - Hippocampus temporo-...gram shift model (Lytton 1999)
     - `ModelDBRepository/7400 <https://github.com/ModelDBRepository/7400>`_
     - `pull/1 <https://github.com/ModelDBRepository/7400/pull/1/files>`_
     - matrix.mod vecst.mod
   * - `8284 <https://modeldb.science/8284>`_
     - Febrile seizure-indu...ations to Ih (Chen et al 2001)
     - `ModelDBRepository/8284 <https://github.com/ModelDBRepository/8284>`_
     - `pull/1 <https://github.com/ModelDBRepository/8284/pull/1/files>`_
     - hyperde1.mod hyperde2.mod hyperde3.mod hyperso.mod ichan.mod
   * - `9889 <https://modeldb.science/9889>`_
     - Thalamic quiescence ...e seizures (Lytton et al 1997)
     - `ModelDBRepository/9889 <https://github.com/ModelDBRepository/9889>`_
     - `pull/1 <https://github.com/ModelDBRepository/9889/pull/1/files>`_
     - rand.mod
   * - `12631 <https://modeldb.science/12631>`_
     - Computer model of cl...n thalamic slice (Lytton 1997)
     - `ModelDBRepository/12631 <https://github.com/ModelDBRepository/12631>`_
     - `pull/2 <https://github.com/ModelDBRepository/12631/pull/2/files>`_
     - rand.mod
   * - `26997 <https://modeldb.science/26997>`_
     - Gamma oscillations i... networks (Wang, Buzsaki 1996)
     - `ModelDBRepository/26997 <https://github.com/ModelDBRepository/26997>`_
     - `pull/1 <https://github.com/ModelDBRepository/26997/pull/1/files>`_
     - vecst.mod
   * - `35358 <https://modeldb.science/35358>`_
     - CA3 pyramidal cell: ...ub model (Pinsky, Rinzel 1994)
     - `ModelDBRepository/35358 <https://github.com/ModelDBRepository/35358>`_
     - `pull/2 <https://github.com/ModelDBRepository/35358/pull/2/files>`_
     - matrix.mod vecst.mod
   * - `37819 <https://modeldb.science/37819>`_
     - Thalamocortical augm...response (Bazhenov et al 1998)
     - `ModelDBRepository/37819 <https://github.com/ModelDBRepository/37819>`_
     - `pull/1 <https://github.com/ModelDBRepository/37819/pull/1/files>`_
     - matrix.mod pointer.inc ppsav.inc vecst.mod
   * - `51781 <https://modeldb.science/51781>`_
     - Dentate gyrus networ...model (Santhakumar et al 2005)
     - `ModelDBRepository/51781 <https://github.com/ModelDBRepository/51781>`_
     - `pull/1 <https://github.com/ModelDBRepository/51781/pull/1/files>`_
     - hyperde3.mod ichan2.mod
   * - `52034 <https://modeldb.science/52034>`_
     - Cortical network mod...leptogenesis (Bush et al 1999)
     - `ModelDBRepository/52034 <https://github.com/ModelDBRepository/52034>`_
     - `pull/1 <https://github.com/ModelDBRepository/52034/pull/1/files>`_
     - holt_rnd.mod precall.mod snsarr.inc
   * - `64229 <https://modeldb.science/64229>`_
     - Parallel network sim...h NEURON (Migliore et al 2006)
     - `ModelDBRepository/64229 <https://github.com/ModelDBRepository/64229>`_
     - `pull/1 <https://github.com/ModelDBRepository/64229/pull/1/files>`_
     - parbulbNet/cadecay.mod
   * - `64296 <https://modeldb.science/64296>`_
     - Dynamical model of o...al cell  (Rubin, Cleland 2006)
     - `ModelDBRepository/64296 <https://github.com/ModelDBRepository/64296>`_
     - `pull/1 <https://github.com/ModelDBRepository/64296/pull/1/files>`_
     - cadecay.mod
   * - `87585 <https://modeldb.science/87585>`_
     - Sodium channel mutat...eizures + (Barela et al. 2006)
     - `ModelDBRepository/87585 <https://github.com/ModelDBRepository/87585>`_
     - `pull/1 <https://github.com/ModelDBRepository/87585/pull/1/files>`_
     - ichanR859C1.mod ichanWT2005.mod
   * - `93321 <https://modeldb.science/93321>`_
     - Activity dependent c...euron model  (Liu et al. 1998)
     - `ModelDBRepository/93321 <https://github.com/ModelDBRepository/93321>`_
     - `pull/1 <https://github.com/ModelDBRepository/93321/pull/1/files>`_
     - cadecay.mod
   * - `97868 <https://modeldb.science/97868>`_
     - NEURON interfaces to...gorithm (Neymotin et al. 2008)
     - `ModelDBRepository/97868 <https://github.com/ModelDBRepository/97868>`_
     - `pull/2 <https://github.com/ModelDBRepository/97868/pull/2/files>`_
     - MySQL.mod spud.mod vecst.mod
   * - `97874 <https://modeldb.science/97874>`_
     - Neural Query System ...NEURON Simulator (Lytton 2006)
     - `ModelDBRepository/97874 <https://github.com/ModelDBRepository/97874>`_
     - `pull/2 <https://github.com/ModelDBRepository/97874/pull/2/files>`_
     - NQS/vecst.mod modeldb/vecst.mod
   * - `97917 <https://modeldb.science/97917>`_
     - Cell splitting in ne...ng scaling (Hines et al. 2008)
     - `ModelDBRepository/97917 <https://github.com/ModelDBRepository/97917>`_
     - `pull/2 <https://github.com/ModelDBRepository/97917/pull/2/files>`_
     - nrntraub/mod/rand.mod nrntraub/mod/ri.mod pardentategyrus/hyperde3.mod pardentategyrus/ichan2.mod
   * - `105507 <https://modeldb.science/105507>`_
     - Tonic-clonic transit...tion (Lytton and Omurtag 2007)
     - `ModelDBRepository/105507 <https://github.com/ModelDBRepository/105507>`_
     - `pull/2 <https://github.com/ModelDBRepository/105507/pull/2/files>`_
     - intf\_.mod misc.mod stats.mod vecst.mod
   * - `106891 <https://modeldb.science/106891>`_
     - JitCon: Just in time... networks (Lytton et al. 2008)
     - `ModelDBRepository/106891 <https://github.com/ModelDBRepository/106891>`_
     - `pull/3 <https://github.com/ModelDBRepository/106891/pull/3/files>`_
     - jitcon.mod misc.h misc.mod stats.mod vecst.mod
   * - `113732 <https://modeldb.science/113732>`_
     - MEG of Somatosensory Neocortex (Jones et al. 2007)
     - `ModelDBRepository/113732 <https://github.com/ModelDBRepository/113732>`_
     - `pull/1 <https://github.com/ModelDBRepository/113732/pull/1/files>`_
     - precall.mod presyn.inc
   * - `116094 <https://modeldb.science/116094>`_
     - Lateral dendrodendit...ctory Bulb (David et al. 2008)
     - `ModelDBRepository/116094 <https://github.com/ModelDBRepository/116094>`_
     - `pull/1 <https://github.com/ModelDBRepository/116094/pull/1/files>`_
     - LongDendrite/cadecay.mod ShortDendrite/cadecay.mod
   * - `116830 <https://modeldb.science/116830>`_
     - Broadening of activi...tructures (Lytton et al. 2008)
     - `ModelDBRepository/116830 <https://github.com/ModelDBRepository/116830>`_
     - `pull/1 <https://github.com/ModelDBRepository/116830/pull/1/files>`_
     - intf.mod misc.mod stats.mod vecst.mod
   * - `116838 <https://modeldb.science/116838>`_
     - The virtual slice setup (Lytton et al. 2008)
     - `ModelDBRepository/116838 <https://github.com/ModelDBRepository/116838>`_
     - `pull/1 <https://github.com/ModelDBRepository/116838/pull/1/files>`_
     - intf\_.mod misc.h misc.mod stats.mod vecst.mod
   * - `116862 <https://modeldb.science/116862>`_
     - Thalamic interneuron...rtment model (Zhu et al. 1999)
     - `ModelDBRepository/116862 <https://github.com/ModelDBRepository/116862>`_
     - `pull/1 <https://github.com/ModelDBRepository/116862/pull/1/files>`_
     - clampex.mod misc.h vecst.mod
   * - `123815 <https://modeldb.science/123815>`_
     - Encoding and retriev...rcuit (Cutsuridis et al. 2009)
     - `ModelDBRepository/123815 <https://github.com/ModelDBRepository/123815>`_
     - `pull/1 <https://github.com/ModelDBRepository/123815/pull/1/files>`_
     - burststim2.mod ichan2.mod regn_stim.mod
   * - `136095 <https://modeldb.science/136095>`_
     - Synaptic information...columns (Neymotin et al. 2010)
     - `ModelDBRepository/136095 <https://github.com/ModelDBRepository/136095>`_
     - `pull/1 <https://github.com/ModelDBRepository/136095/pull/1/files>`_
     - clampex.mod field.mod infot.mod intf\_.mod intfsw.mod misc.h misc.mod netrand.inc pregencv.mod stats.mod updown.mod vecst.mod
   * - `136310 <https://modeldb.science/136310>`_
     - Modeling local field...otentials (Bedard et al. 2004)
     - `ModelDBRepository/136310 <https://github.com/ModelDBRepository/136310>`_
     - `pull/1 <https://github.com/ModelDBRepository/136310/pull/1/files>`_
     - ImpedanceFM.mod
   * - `137845 <https://modeldb.science/137845>`_
     - Spike exchange metho...rcomputer (Hines et al., 2011)
     - `ModelDBRepository/137845 <https://github.com/ModelDBRepository/137845>`_
     - `pull/1 <https://github.com/ModelDBRepository/137845/pull/1/files>`_
     - invlfire.mod
   * - `138379 <https://modeldb.science/138379>`_
     - Emergence of physiol...lations (Neymotin et al. 2011)
     - `ModelDBRepository/138379 <https://github.com/ModelDBRepository/138379>`_
     - `pull/1 <https://github.com/ModelDBRepository/138379/pull/1/files>`_
     - intf6\_.mod misc.h misc.mod stats.mod vecst.mod
   * - `139421 <https://modeldb.science/139421>`_
     - Ketamine disrupts th...pocampus (Neymotin et al 2011)
     - `ModelDBRepository/139421 <https://github.com/ModelDBRepository/139421>`_
     - `pull/1 <https://github.com/ModelDBRepository/139421/pull/1/files>`_
     - misc.h misc.mod stats.mod vecst.mod wrap.mod
   * - `140881 <https://modeldb.science/140881>`_
     - Computational Surgery (Lytton et al. 2011)
     - `ModelDBRepository/140881 <https://github.com/ModelDBRepository/140881>`_
     - `pull/1 <https://github.com/ModelDBRepository/140881/pull/1/files>`_
     - intf6\_.mod misc.h misc.mod stats.mod vecst.mod
   * - `141505 <https://modeldb.science/141505>`_
     - Prosthetic electrost...ortical simulation (Kerr 2012)
     - `ModelDBRepository/141505 <https://github.com/ModelDBRepository/141505>`_
     - `pull/1 <https://github.com/ModelDBRepository/141505/pull/1/files>`_
     - infot.mod intf6\_.mod intfsw.mod misc.h misc.mod staley.mod stats.mod vecst.mod
   * - `144538 <https://modeldb.science/144538>`_
     - Reinforcement learni...ement (Chadderdon et al. 2012)
     - `ModelDBRepository/144538 <https://github.com/ModelDBRepository/144538>`_
     - `pull/1 <https://github.com/ModelDBRepository/144538/pull/1/files>`_
     - infot.mod intf6\_.mod intfsw.mod misc.h misc.mod stats.mod updown.mod vecst.mod
   * - `144549 <https://modeldb.science/144549>`_
     - Hopfield and Brody m...d, Brody 2000) (NEURON+python)
     - `ModelDBRepository/144549 <https://github.com/ModelDBRepository/144549>`_
     - `pull/1 <https://github.com/ModelDBRepository/144549/pull/1/files>`_
     - misc.h misc.mod stats.mod vecst.mod
   * - `144586 <https://modeldb.science/144586>`_
     - Boolean network-base...sis network (Mai and Liu 2009)
     - `ModelDBRepository/144586 <https://github.com/ModelDBRepository/144586>`_
     - `pull/1 <https://github.com/ModelDBRepository/144586/pull/1/files>`_
     - bnet.mod misc.h misc.mod stats.mod vecst.mod
   * - `146949 <https://modeldb.science/146949>`_
     - Motor cortex microci...pping (Chadderdon et al. 2014)
     - `ModelDBRepository/146949 <https://github.com/ModelDBRepository/146949>`_
     - `pull/1 <https://github.com/ModelDBRepository/146949/pull/1/files>`_
     - infot.mod intf6.mod intfsw.mod matrix.mod misc.h misc.mod staley.mod stats.mod vecst.mod
   * - `149000 <https://modeldb.science/149000>`_
     - Using Strahler`s ana...c models (Marasco et al, 2013)
     - `ModelDBRepository/149000 <https://github.com/ModelDBRepository/149000>`_
     - `pull/1 <https://github.com/ModelDBRepository/149000/pull/1/files>`_
     - pj.mod
   * - `149739 <https://modeldb.science/149739>`_
     - A two-layer biophysi...dulation (Li and Cleland 2013)
     - `ModelDBRepository/149739 <https://github.com/ModelDBRepository/149739>`_
     - `pull/1 <https://github.com/ModelDBRepository/149739/pull/1/files>`_
     - cadecay.mod cadecay2.mod
   * - `150240 <https://modeldb.science/150240>`_
     - A Model Circuit of T...vergence (Behuret et al. 2013)
     - `ModelDBRepository/150240 <https://github.com/ModelDBRepository/150240>`_
     - `pull/1 <https://github.com/ModelDBRepository/150240/pull/1/files>`_
     - RandomGenerator.mod
   * - `150245 <https://modeldb.science/150245>`_
     - Sensorimotor cortex ...eaching (Neymotin et al. 2013)
     - `ModelDBRepository/150245 <https://github.com/ModelDBRepository/150245>`_
     - `pull/1 <https://github.com/ModelDBRepository/150245/pull/1/files>`_
     - infot.mod intf6\_.mod misc.h misc.mod stats.mod vecst.mod
   * - `150551 <https://modeldb.science/150551>`_
     - Calcium waves and mG...rons (Ashhad & Narayanan 2013)
     - `ModelDBRepository/150551 <https://github.com/ModelDBRepository/150551>`_
     - `pull/1 <https://github.com/ModelDBRepository/150551/pull/1/files>`_
     - Calamp.mod
   * - `150556 <https://modeldb.science/150556>`_
     - Single compartment D...MPA (Biddell and Johnson 2013)
     - `ModelDBRepository/150556 <https://github.com/ModelDBRepository/150556>`_
     - `pull/1 <https://github.com/ModelDBRepository/150556/pull/1/files>`_
     - KBNetStim.mod
   * - `150691 <https://modeldb.science/150691>`_
     - Model of arrhythmias...twork (Casaleggio et al. 2014)
     - `ModelDBRepository/150691 <https://github.com/ModelDBRepository/150691>`_
     - `pull/1 <https://github.com/ModelDBRepository/150691/pull/1/files>`_
     - Readme halfgapm1.mod halfgapspk.mod
   * - `151126 <https://modeldb.science/151126>`_
     - Effects of increasin... network (Bianchi et al. 2014)
     - `ModelDBRepository/151126 <https://github.com/ModelDBRepository/151126>`_
     - `pull/1 <https://github.com/ModelDBRepository/151126/pull/1/files>`_
     - burststim2.mod ichan2.mod regn_stim.mod
   * - `151282 <https://modeldb.science/151282>`_
     - Ih tunes oscillation...3 model (Neymotin et al. 2013)
     - `ModelDBRepository/151282 <https://github.com/ModelDBRepository/151282>`_
     - `pull/1 <https://github.com/ModelDBRepository/151282/pull/1/files>`_
     - misc.h misc.mod stats.mod vecst.mod
   * - `153280 <https://modeldb.science/153280>`_
     - Parvalbumin-positive...amidal cells (Lee et al. 2014)
     - `ModelDBRepository/153280 <https://github.com/ModelDBRepository/153280>`_
     - `pull/1 <https://github.com/ModelDBRepository/153280/pull/1/files>`_
     - ch_Kdrfast.mod mynetstim.mod repeatconn.mod
   * - `154732 <https://modeldb.science/154732>`_
     - Spine head calcium i...ell model (Graham et al. 2014)
     - `ModelDBRepository/154732 <https://github.com/ModelDBRepository/154732>`_
     - `pull/1 <https://github.com/ModelDBRepository/154732/pull/1/files>`_
     - burststim2.mod
   * - `155568 <https://modeldb.science/155568>`_
     - Dentate gyrus network model (Tejada et al 2014)
     - `ModelDBRepository/155568 <https://github.com/ModelDBRepository/155568>`_
     - `pull/1 <https://github.com/ModelDBRepository/155568/pull/1/files>`_
     - hyperde3.mod ichan2.mod
   * - `155601 <https://modeldb.science/155601>`_
     - Basket cell extrasyn...tions (Proddutur et al., 2013)
     - `ModelDBRepository/155601 <https://github.com/ModelDBRepository/155601>`_
     - `pull/1 <https://github.com/ModelDBRepository/155601/pull/1/files>`_
     - hyperde3.mod ichan2.mod
   * - `155602 <https://modeldb.science/155602>`_
     - Status epilepticus a...c inhibition (Yu J et al 2013)
     - `ModelDBRepository/155602 <https://github.com/ModelDBRepository/155602>`_
     - `pull/1 <https://github.com/ModelDBRepository/155602/pull/1/files>`_
     - hyperde3.mod ichan2.mod
   * - `156780 <https://modeldb.science/156780>`_
     - Microcircuits of L5 ...midal cells (Hay & Segev 2015)
     - `ModelDBRepository/156780 <https://github.com/ModelDBRepository/156780>`_
     - `pull/1 <https://github.com/ModelDBRepository/156780/pull/1/files>`_
     - ProbAMPANMDA2.mod ProbUDFsyn2.mod
   * - `157157 <https://modeldb.science/157157>`_
     - CA1 pyramidal neuron...cles (Saudargiene et al. 2015)
     - `ModelDBRepository/157157 <https://github.com/ModelDBRepository/157157>`_
     - `pull/1 <https://github.com/ModelDBRepository/157157/pull/1/files>`_
     - burststim2.mod regn_stim.mod
   * - `168874 <https://modeldb.science/168874>`_
     - Neuronal dendrite ca...e model (Neymotin et al, 2015)
     - `ModelDBRepository/168874 <https://github.com/ModelDBRepository/168874>`_
     - `pull/4 <https://github.com/ModelDBRepository/168874/pull/4/files>`_
     - misc.h misc.mod stats.mod vecst.mod
   * - `181967 <https://modeldb.science/181967>`_
     - Long time windows fr...op (Cutsuridis & Poirazi 2015)
     - `ModelDBRepository/181967 <https://github.com/ModelDBRepository/181967>`_
     - `pull/1 <https://github.com/ModelDBRepository/181967/pull/1/files>`_
     - burststim.mod hyperde3.mod ichan2.mod regn_stim.mod
   * - `182129 <https://modeldb.science/182129>`_
     - Computational modeli...signaling (Lupascu et al 2020)
     - `ModelDBRepository/182129 <https://github.com/ModelDBRepository/182129>`_
     - `pull/1 <https://github.com/ModelDBRepository/182129/pull/1/files>`_
     - ProbGABAAB_EMS_GEPH_g.mod
   * - `183300 <https://modeldb.science/183300>`_
     - Mitral cell activity...ory bulb NN (Short et al 2016)
     - `ModelDBRepository/183300 <https://github.com/ModelDBRepository/183300>`_
     - `pull/1 <https://github.com/ModelDBRepository/183300/pull/1/files>`_
     - cadecay.mod cadecay2.mod thetastim.mod vecstim.mod
   * - `185355 <https://modeldb.science/185355>`_
     - Dentate gyrus networ...g in epilepsy (Yim et al 2015)
     - `ModelDBRepository/185355 <https://github.com/ModelDBRepository/185355>`_
     - `pull/2 <https://github.com/ModelDBRepository/185355/pull/2/files>`_
     - HCN.mod ichan2.mod netstim125.mod netstimbox.mod
   * - `185858 <https://modeldb.science/185858>`_
     - Ca+/HCN channel-depe...eocortex (Neymotin et al 2016)
     - `ModelDBRepository/185858 <https://github.com/ModelDBRepository/185858>`_
     - `pull/1 <https://github.com/ModelDBRepository/185858/pull/1/files>`_
     - misc.h misc.mod vecst.mod
   * - `186768 <https://modeldb.science/186768>`_
     - CA3 Network Model of...Activity (Sanjay et. al, 2015)
     - `ModelDBRepository/186768 <https://github.com/ModelDBRepository/186768>`_
     - `pull/1 <https://github.com/ModelDBRepository/186768/pull/1/files>`_
     - misc.h misc.mod stats.mod vecst.mod wrap.mod
   * - `187604 <https://modeldb.science/187604>`_
     - Hippocampal CA1 NN w...ork clamp (Bezaire et al 2016)
     - `ModelDBRepository/187604 <https://github.com/ModelDBRepository/187604>`_
     - `pull/2 <https://github.com/ModelDBRepository/187604/pull/2/files>`_
     - fastconn.mod mynetstim.mod repeatconn.mod sgate.mod
   * - `189154 <https://modeldb.science/189154>`_
     - Multitarget pharmaco...ia in M1 (Neymotin et al 2016)
     - `ModelDBRepository/189154 <https://github.com/ModelDBRepository/189154>`_
     - `pull/1 <https://github.com/ModelDBRepository/189154/pull/1/files>`_
     - misc.h misc.mod stats.mod vecst.mod
   * - `194897 <https://modeldb.science/194897>`_
     - Motor system model w...l arm (Dura-Bernal et al 2017)
     - `ModelDBRepository/194897 <https://github.com/ModelDBRepository/194897>`_
     - `pull/2 <https://github.com/ModelDBRepository/194897/pull/2/files>`_
     - izhi2007.mod nsloc.mod vecevent.mod
   * - `195615 <https://modeldb.science/195615>`_
     - Computer models of c...ynamics (Neymotin et al. 2017)
     - `ModelDBRepository/195615 <https://github.com/ModelDBRepository/195615>`_
     - `pull/1 <https://github.com/ModelDBRepository/195615/pull/1/files>`_
     - misc.h misc.mod vecst.mod
   * - `223031 <https://modeldb.science/223031>`_
     - Interneuron Specific...l (Guet-McCreight et al, 2016)
     - `ModelDBRepository/223031 <https://github.com/ModelDBRepository/223031>`_
     - `pull/1 <https://github.com/ModelDBRepository/223031/pull/1/files>`_
     - S1/ingauss.mod S2/ingauss.mod SD/ingauss.mod SDprox1/ingauss.mod SDprox2/ingauss.mod
   * - `225080 <https://modeldb.science/225080>`_
     - CA1 pyr cell: Inhibi...ssion (Grienberger et al 2017)
     - `ModelDBRepository/225080 <https://github.com/ModelDBRepository/225080>`_
     - `pull/1 <https://github.com/ModelDBRepository/225080/pull/1/files>`_
     - pr.mod vecevent.mod
   * - `231427 <https://modeldb.science/231427>`_
     - Shaping NMDA spikes ...on on L5PC (Doron et al. 2017)
     - `ModelDBRepository/231427 <https://github.com/ModelDBRepository/231427>`_
     - `pull/2 <https://github.com/ModelDBRepository/231427/pull/2/files>`_
     - ProbAMPA.mod ProbUDFsyn2_lark.mod
   * - `232097 <https://modeldb.science/232097>`_
     - 2D model of olfactor...llations (Li and Cleland 2017)
     - `ModelDBRepository/232097 <https://github.com/ModelDBRepository/232097>`_
     - `pull/1 <https://github.com/ModelDBRepository/232097/pull/1/files>`_
     - cadecay.mod cadecay2.mod
   * - `239177 <https://modeldb.science/239177>`_
     - Phase response theor...(Tikidji-Hamburyan et al 2019)
     - `ModelDBRepository/239177 <https://github.com/ModelDBRepository/239177>`_
     - `pull/1 <https://github.com/ModelDBRepository/239177/pull/1/files>`_
     - PIR-Inetwork/innp.mod PIR-Inetwork/vecevent.mod
   * - `241165 <https://modeldb.science/241165>`_
     - Biophysically realis...imulation (Aberra et al. 2018)
     - `ModelDBRepository/241165 <https://github.com/ModelDBRepository/241165>`_
     - `pull/1 <https://github.com/ModelDBRepository/241165/pull/1/files>`_
     - mechanisms/ProbAMPANMDA_EMS.mod mechanisms/ProbGABAAB_EMS.mod
   * - `241240 <https://modeldb.science/241240>`_
     - Role of afferent-hai...regularity (Holmes et al 2017)
     - `ModelDBRepository/241240 <https://github.com/ModelDBRepository/241240>`_
     - `pull/1 <https://github.com/ModelDBRepository/241240/pull/1/files>`_
     - afhcsyn_i0.mod hc1.mod random.mod ribbon1.mod
   * - `244262 <https://modeldb.science/244262>`_
     - Deconstruction of co...ic DBS (Kumaravelu et al 2018)
     - `ModelDBRepository/244262 <https://github.com/ModelDBRepository/244262>`_
     - `pull/1 <https://github.com/ModelDBRepository/244262/pull/1/files>`_
     - rand.mod ri.mod
   * - `244848 <https://modeldb.science/244848>`_
     - Active dendrites sha...urons (Basak & Narayanan 2018)
     - `ModelDBRepository/244848 <https://github.com/ModelDBRepository/244848>`_
     - `pull/2 <https://github.com/ModelDBRepository/244848/pull/2/files>`_
     - apamp.mod
   * - `247968 <https://modeldb.science/247968>`_
     - Gamma genesis in the...ral amygdala (Feng et al 2019)
     - `ModelDBRepository/247968 <https://github.com/ModelDBRepository/247968>`_
     - `pull/1 <https://github.com/ModelDBRepository/247968/pull/1/files>`_
     - Gfluct_new_exc.mod Gfluct_new_inh.mod vecevent.mod
   * - `249463 <https://modeldb.science/249463>`_
     - Layer V pyramidal ce...cs (Mäki-Marttunen et al 2019)
     - `ModelDBRepository/249463 <https://github.com/ModelDBRepository/249463>`_
     - `pull/1 <https://github.com/ModelDBRepository/249463/pull/1/files>`_
     - almog/ProbAMPANMDA2.mod almog/ProbUDFsyn2.mod hay/ProbAMPANMDA2.mod hay/ProbUDFsyn2.mod
   * - `256388 <https://modeldb.science/256388>`_
     - The APP in C-termina...n firing (Pousinha et al 2019)
     - `ModelDBRepository/256388 <https://github.com/ModelDBRepository/256388>`_
     - `pull/1 <https://github.com/ModelDBRepository/256388/pull/1/files>`_
     - burststim2.mod ichan2.mod regn_stim.mod
   * - `259366 <https://modeldb.science/259366>`_
     - Cycle skipping in IN...dji-Hamburyan & Canavier 2020)
     - `ModelDBRepository/259366 <https://github.com/ModelDBRepository/259366>`_
     - `pull/1 <https://github.com/ModelDBRepository/259366/pull/1/files>`_
     - innp.mod

..
  Does this need some more qualification? Are there non-VERBATIM
  incompatibilities?

Legacy patterns that are invalid C++
------------------------------------

This section aims to list some patterns that the NEURON developers have found
in pre-|neuron_with_cpp_mechanisms| models that need to be modified to be valid
C++.

Implicit pointer type conversions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
C++ has stricter rules governing pointer type conversions than C. For example

.. code-block:: cpp

  double* x = (void*)0;   // valid C, invalid C++
  double* x = nullptr;    // valid C++, invalid C
  double* x = (double*)0; // valid C and C++ (C-style casts discouraged in C++)

Similarly, in C one can pass a ``void*`` argument to a function that expects a
``double*``. In C++ this is forbidden.

The same issue may manifest itself in code such as

.. code-block:: cpp

  double* x = malloc(7 * sizeof(double));          // valid C, invalid C++
  double* x = new double[7];                       // valid C++, invalid C
  double* x = (double*)malloc(7 * sizeof(double)); // valid C and C++ (C-style casts discouraged in C++)

If you choose to move from using C ``malloc`` and ``free`` to using C++ ``new``
and ``delete`` then remember that you cannot mix and match ``new`` with ``free``
and so on.

.. note::
  Explicit memory management with ``new`` and ``delete`` is discouraged in C++
  (`R.11: Avoid calling new and delete explicitly <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-newdelete>`_).
  If you do not need to support older versions of NEURON, you may be able to
  use standard library containers such as ``std::vector<T>``.

.. _local-non-prototype-function-declaration:

Local non-prototype function declarations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In C, the function declaration

.. code-block:: c

  void some_function();

is a `non-prototype function declaration
<https://en.cppreference.com/w/c/language/function_declaration>`_: it declares
that ``some_function`` exists, but it does not specify the number of arguments
that it takes, or their types.
In C++, the same code declares that ``some_function`` takes zero arguments (the
C equivalent of this is ``void some_function(void)``).

If such a declaration occurs in a top-level ``VERBATIM`` block then it is
likely to be harmless: it will add a zero-parameter overload of the function,
which will never be called.
It **will**, however, cause a problem if the declaration is included **inside**
an NMODL construct

.. code-block:: c

  PROCEDURE procedure() {
  VERBATIM
    void some_method_taking_an_int(); // problematic in C++
    some_method_taking_an_int(42);
  ENDVERBATIM
  }

because in this case the local declaration hides the real declaration, which is
something like

.. code-block:: c

  void some_method_taking_an_int(int);

in a NEURON header that is included when the translated MOD file is compiled.
In this case, the problematic local declaration can simply be removed.

.. _function-decls-with-incorrect-types:

Function declarations with incorrect types
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In older MOD files and versions of NEURON, API methods were often accessed by
declaring them in the MOD file and not by including a correct declaration from
NEURON itself.
In NEURON 8.2+, more declarations are implicitly included when MOD files are
compiled.
This can lead to problems if the declaration in the MOD file did not specify
the correct argument and return types.

.. code-block:: cpp

  Rand* nrn_random_arg(int); // correct declaration from NEURON header, only in new NEURON
  /* ... */
  void* nrn_random_arg(int); // incorrect declaration in MOD file, clashes with previous line

The fix here is simply to remove the incorrect declaration from the MOD file.

If the argument types are incorrect, the situation is slightly more nuanced.
This is because C++ supports overloading functions by argument type, but not by
return type.

.. code-block:: cpp

  void sink(Thing*); // correct declaration from NEURON header, only in new NEURON
  /* ... */
  void sink(void*); // incorrect declaration in MOD file, adds an overload, not a compilation error
  /* ... */
  void* void_ptr;
  sink(void_ptr);  // probably used to work, now causes a linker error
  Thing* thing_ptr;
  sink(thing_ptr); // works with both old and new NEURON

Here the incorrect declaration ``sink(void*)`` declares a second overload of
the ``sink`` function, which is not defined anywhere.
With |neuron_with_cpp_mechanisms| the ``sink(void_ptr)`` line will select the
``void*`` second overload, based on the argument type, and this will fail during
linking because this overload is not defined (only ``sink(Thing*)`` has a
definition).

In contrast, ``sink(thing_ptr)`` will select the correct overload in
|neuron_with_cpp_mechanisms|, and it also works in older NEURON versions
because ``Thing*`` can be implicitly converted to ``void*``.

The fix here is, again, to remove the incorrect declaration from the MOD file.

See also the section below, :ref:`deprecated-overloads-taking-void`, for cases
where NEURON **does** provide a (deprecated) definition of the ``void*``
overload.

K&R function definitions
^^^^^^^^^^^^^^^^^^^^^^^^
C supports (`until C23
<https://en.cppreference.com/w/c/language/function_definition>`_) a legacy
("K&R") syntax for function declarations.
This is not valid C++.

There is no advantage to the legacy syntax. If you have legacy definitions such
as

.. code-block:: c

  void foo(a, b) int a, b; { /* ... */ }

then simply replace them with

.. code-block:: cpp

  void foo(int a, int b) { /* ... */ }

which is valid in both C and C++.

Legacy patterns that are considered deprecated
----------------------------------------------
As noted above (:ref:`local-non-prototype-function-declaration`), declarations
such as

.. code-block:: c

  VERBATIM
  extern void vector_resize();
  ENDVERBATIM

at the global scope in MOD files declare C++ function overloads that take no
parameters.
If such declarations appear at the global scope then they do not hide the
correct declarations, so this can be harmless, but it is not necessary.
In |neuron_with_cpp_mechanisms| the full declarations of the NEURON API methods
that can be used in MOD files are implicitly included via the ``mech_api.h``
header, so this explicit declaration of a zero-parameter overload is not needed
and can safely be removed.

.. _deprecated-overloads-taking-void:

Deprecated overloads taking ``void*``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As noted above (:ref:`function-decls-with-incorrect-types`),
|neuron_with_cpp_mechanisms| provides extra overloads for some API methods that
aim to smooth the transition from C to C++.
These overloads are provided for widely used methods, and where overloading on
return type would not be required.

An example is the ``vector_capacity`` function, which in
|neuron_with_cpp_mechanisms| has two overloads

.. code-block:: cpp

  int vector_capacity(IvocVect*);
  [[deprecated("non-void* overloads are preferred")]] int vector_capacity(void*);

The second one simply casts its argument to ``IvocVect*`` and calls the first
one.
The ``[[deprecated]]`` attribute means that MOD files that use the second
overload emit compilation warnings when they are compiled using ``nrnivmodl``.

If your MOD files produce these deprecation warnings, make sure that the
relevant method (``vector_capacity`` in this example) is being called with an
argument of the correct type (``IvocVect*``), and not a type that is implicitly
converted to ``void*``.

Legacy random number generators and API
---------------------------------------

Various changes have also been done in the API of NEURON functions related to random 
number generators.

First, in |neuron_with_cpp_mechanisms| parameters passed to the functions need to be
of the correct type as it was already mentioned in :ref:`function-decls-with-incorrect-types`.
The most usual consequence of that is that NEURON random API functions that were taking as
an argument a ``void*`` now need to called with a ``Rand*``. An example of the changes needed
to fix this issue is given in `182129 <https://github.com/ModelDBRepository/182129/pull/1>`_.

Another related change is with ``scop_rand()`` function that is usually used for defining a
``URAND`` ``FUNCTION`` in mod files to return a number based on a uniform distribution from 0 to 1.
This function now takes no argument anymore. An example of this change can also be found in
`182129 <https://github.com/ModelDBRepository/182129/pull/1>`_.

Finally, the preferred random number generator is ``Random123``. You can find more information
about that in :meth:`Random.Random123` and :ref:`Randomness in NEURON models`. An example of the
usage of ``Random123`` can be seen in `netstim.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/netstim.mod>`_
and its `corresponding test <https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_psolve.py#L60>`_.`
Another important aspect of ``Random123`` is that it's supported in CoreNEURON as well. For more
information about this see :ref:`Random Number Generators: Random123 vs MCellRan4`.
