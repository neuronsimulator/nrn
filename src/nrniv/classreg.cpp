#include <../../nrnconf.h>

extern void Graph_reg(), HBox_reg(), VBox_reg(), GUIMath_reg(), PWManager_reg(), GrGlyph_reg(),
    ValueFieldEditor_reg(),
#if HAVE_IV
    TextEditor_reg(),
#endif
    OcTimer_reg(), OcDeck_reg(), SymChooser_reg(), StringFunctions_reg(), OcList_reg(),
    Vector_reg(), OcPtrVector_reg(), OcFile_reg(), OcPointer_reg(),
#ifdef USEMATRIX
    Matrix_reg(),
#endif
    Random_reg(), Shape_reg(), PlotShape_reg(), PPShape_reg(), RangeVarPlot_reg(),
    SectionBrowser_reg(), MechanismStandard_reg(), MechanismType_reg(), NetCon_reg(),
    LinearMechanism_reg(), KSChan_reg(), Impedance_reg(), SaveState_reg(), BBSaveState_reg(),
    FInitializeHandler_reg(), StateTransitionEvent_reg(), nrnpython_reg(),
#if USEDASPK
    Daspk_reg(),
#endif
#if USECVODE
    Cvode_reg(),
#endif
#if USEDSP
    DSP_reg(),
#endif
#if USEBBS
    ParallelContext_reg(),
#endif
    NMODLRandom_reg();

void hoc_class_registration(void) {
    void (*register_classes[])() =
    { Graph_reg,
      HBox_reg,
      VBox_reg,
      GUIMath_reg,
      PWManager_reg,
      GrGlyph_reg,
      ValueFieldEditor_reg,
#if HAVE_IV
      TextEditor_reg,
#endif
      OcTimer_reg,
      OcDeck_reg,
      SymChooser_reg,
      StringFunctions_reg,
      OcList_reg,
      Vector_reg,
      OcPtrVector_reg,
      OcFile_reg,
      OcPointer_reg,
#ifdef USEMATRIX
      Matrix_reg,
#endif
      Random_reg,
      Shape_reg,
      PlotShape_reg,
      PPShape_reg,
      RangeVarPlot_reg,
      SectionBrowser_reg,
      MechanismStandard_reg,
      MechanismType_reg,
      NetCon_reg,
      LinearMechanism_reg,
      KSChan_reg,
      Impedance_reg,
      SaveState_reg,
      BBSaveState_reg,
      FInitializeHandler_reg,
      StateTransitionEvent_reg,
      nrnpython_reg,
      NMODLRandom_reg,
#if USEDASPK
      Daspk_reg,
#endif
#if USECVODE
      Cvode_reg,
#endif
#if USEDSP
      DSP_reg,
#endif
#if USEBBS
      ParallelContext_reg,
#endif
      nullptr };

    for (int i = 0; register_classes[i]; i++) {
        (*register_classes[i])();
    }
}
