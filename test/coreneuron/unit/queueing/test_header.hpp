/*
 *  * Miniapp - test_header.hpp, Copyright (c), 2016,
 *  * Kai Langen - Swiss Federal Institute of technology in Lausanne,
 *  * kai.langen@epfl.ch,
 *  * All rights reserved.
 *  *
 *  * This library is free software; you can redistribute it and/or
 *  * modify it under the terms of the GNU Lesser General Public
 *  * License as published by the Free Software Foundation; either
 *  * version 3.0 of the License, or (at your option) any later version.
 *  *
 *  * This library is distributed in the hope that it will be useful,
 *  * but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  * Lesser General Public License for more details.
 *  *
 *  * You should have received a copy of the GNU Lesser General Public
 *  * License along with this library.
 *  */

#ifndef test_header_hpp
#define test_header_hpp

#include <boost/mpl/list.hpp>
#include "coreneuron/nrniv/sptbinq.h"

template<container C>
struct data{
	static const container cont = C;
};

typedef boost::mpl::list<
						data<queueing::spltree>,
						data<queueing::pq_que>
						> full_test_types;

#endif
