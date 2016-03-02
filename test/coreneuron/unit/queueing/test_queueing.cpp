/*
 * Neuromapp - test.cpp, Copyright (c), 2015,
 * Kai Langen - Swiss Federal Institute of technology in Lausanne,
 * kai.langen@epfl.ch,
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 */
/**
 * @file neuromapp/test/queuing/test.cpp
 *  Test on the Queueing Miniapp.
 */

#define BOOST_TEST_MODULE QueueingTest
#define TYPE T::cont

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/filesystem.hpp>

#include <cstdlib>
#include <vector>
#include <iostream>
//#include "test/unit/queueing/test_header.hpp"
#include "coreneuron/nrniv/netcvode.h"
#include "coreneuron/nrniv/tqueue.h"

namespace bfs = ::boost::filesystem;

//UNIT TESTS
BOOST_AUTO_TEST_CASE(priority_queue_nq_dq){
	TQueue<pq_que> tq = TQueue<pq_que>();
	const int num = 8;
	int cnter = 0;
	//enqueue 8 items with increasing time
	for(int i = 0; i < num; ++i)
		tq.insert(static_cast<double>(i), NULL);

	BOOST_CHECK(tq.pq_que_.size() == (num - 1));

	// dequeue items with time <= 5.0. Should be 6 events: from 0. to 5.
	TQItem *item = NULL;
	while((item = tq.atomic_dq(5.0)) != NULL){
		++cnter;
		delete item;
	}
	BOOST_CHECK(cnter == 6);
	BOOST_CHECK(tq.pq_que_.size() == (num - 6 - 1));

	//dequeue the rest
	while((item = tq.atomic_dq(8.0)) != NULL){
		++cnter;
		delete item;
	}

	BOOST_CHECK(cnter == num);
	BOOST_CHECK(tq.pq_que_.size() == 0);
	BOOST_CHECK(tq.least() == NULL);
}

BOOST_AUTO_TEST_CASE(tqueue_ordered_test){
	TQueue<pq_que> tq = TQueue<pq_que>();
	const int num = 10;
	int cnter = 0;
	double time = double();

	//insert N items with time < N
	for(int i = 0; i < num; ++i){
		time = static_cast<double>(rand() % num);
		tq.insert(time, NULL);
	}

	time = 0.0;
	TQItem *item = NULL;
	//dequeue all items and check that previous item time <= current item time
	while((item = tq.atomic_dq(10.0)) != NULL){
		BOOST_CHECK(time <= item->t_);
		++cnter;
		time = item->t_;
		delete item;
	}
	BOOST_CHECK(cnter == num);
	BOOST_CHECK(tq.pq_que_.size() == 0);
	BOOST_CHECK(tq.least() == NULL);
}

BOOST_AUTO_TEST_CASE(tqueue_move_nolock){
}

BOOST_AUTO_TEST_CASE(tqueue_remove){
}

BOOST_AUTO_TEST_CASE(threaddata_interthread_send){
	NetCvodeThreadData nt = NetCvodeThreadData();
	const size_t num = 6;
	for(size_t i = 0; i < num; ++i)
		nt.interthread_send(static_cast<double>(i), NULL, NULL);

	BOOST_CHECK(nt.inter_thread_events_.size() == num);
}
/*
BOOST_AUTO_TEST_CASE(threaddata_enqueue){
	NetCvode n = NetCvode();
	const int num = 6;
	for(int i = 0; i < num; ++i)
		n.p[1].interthread_send(static_cast<double>(i), NULL, NULL);

	BOOST_CHECK(n.p[1].inter_thread_events_.size() == num);

	//enqueue the inter_thread_events_
	n.p[1].enqueue(&n, &(n.p[1]));
	BOOST_CHECK(n.p[1].inter_thread_events_.size() == 0);
	BOOST_CHECK(n.p[1].tqe_->pq_que_.size() == num);

	//cleanup priority queue
	TQItem* item = NULL;
	while((item = n.p[1].tqe_->atomic_dq(6.0)) != NULL)
		delete item;
}*/
