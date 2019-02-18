/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

// @todo : This file has been added for legacy purpose and can be removed.

#pragma once

/**
 * Original implementation of nmodl use various flags to help
 * code generation. These flags are implemented as bit masks in
 * the token whic are later checked during code printing. We are
 * using ast and hence don't need all bit masks. These are defined
 * in modl.h file of original nmodl implementation.
 *
 * \todo Remove these bit masks as we incorporate type information
 * into corresponding ast types. */

/// subtypes of the token
#define ARRAY 040
#define DEP 010
#define FUNCT 0100
#define INDEP 04
#define KEYWORD 01
#define NEGATIVE 0400
#define PARM 02
#define PROCED 0200
#define SEMI 01
#define STAT 020

#define DISCF 010000

/// usage field (variable occurs in input file)
#define EXPLICIT_DECL 01

/// external definition
#define EXTDEF 0100000

//// functions that can take array or function name as an arguments
#define EXTDEF2 01000000L

/// function that takes two extra reset arguments at beginning
#define EXTDEF3 04000000L

/// function that takes an extra NrnThread* arg at beginning
#define EXTDEF4 020000000L

//// not threadsafe
#define EXTDEF5 040000000L

/// must be cast to double in expression
#define INTGER 010000000L

/// method subtypes
#define DERF 01000
#define KINF 02000
#define LINF 0200000
#define NLINF 04000

//// constants that do not appear in .var file
#define nmodlCONST 02000000L

#define PARF 040000
#define STEP1 020000
#define UNITDEF 0400000L
