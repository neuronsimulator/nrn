/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file modl.h
 * \brief Legacy macro definitions from mod2c/nocmodl implementation
 *
 * Original implementation of NMODL use various flags to help
 * code generation. These flags are implemented as bit masks
 * which are later used during code printing. We are using ast
 * and hence don't need all bit masks.
 *
 * \todo Add these bit masks as enum-flags and remove this legacy header
 */

/// bit masks for block types where integration method are used
#define DERF  01000
#define KINF  02000
#define LINF  0200000
#define NLINF 04000
