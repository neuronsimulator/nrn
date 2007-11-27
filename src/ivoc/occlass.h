
#if EXTERNS
	Graph_reg(),
	HBox_reg(),
	VBox_reg(),
	GUIMath_reg(),
	PWManager_reg(),
	GrGlyph_reg(),
#if HAVE_IV
	ValueFieldEditor_reg(),
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
#if HAVE_IV
	ValueFieldEditor_reg,
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
	OcFile_reg,
	OcPointer_reg,
#ifdef USEMATRIX
	Matrix_reg,
#endif
	Random_reg
#endif
