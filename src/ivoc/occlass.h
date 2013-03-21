
#if EXTERNS
	Graph_reg(),
	HBox_reg(),
	VBox_reg(),
	GUIMath_reg(),
	PWManager_reg(),
	GrGlyph_reg(),
	ValueFieldEditor_reg(),
#if HAVE_IV
	TextEditor_reg(),
#endif
#if !defined(WIN32) || defined(CYGWIN)
	OcTimer_reg(),
#endif
	OcDeck_reg(),
	SymChooser_reg(),
	StringFunctions_reg(),
	OcList_reg(),
	Vector_reg(),
	OcPtrVector_reg(),
	OcFile_reg(),
	OcPointer_reg(),
#ifdef USEMATRIX
	Matrix_reg(),
#endif
	Random_reg()
#else
	Graph_reg,
	HBox_reg,
	VBox_reg,
	GUIMath_reg,
	PWManager_reg,
	GrGlyph_reg,
	ValueFieldEditor_reg,
#if HAVE_IV
	TextEditor_reg,
#endif
#if !defined(WIN32) || defined(CYGWIN)
	OcTimer_reg,
#endif
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
	Random_reg
#endif
