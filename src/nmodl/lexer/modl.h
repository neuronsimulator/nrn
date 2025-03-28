/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
