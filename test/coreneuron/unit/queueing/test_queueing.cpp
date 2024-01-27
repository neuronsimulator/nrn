/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/network/netcvode.hpp"
#include "coreneuron/network/tqueue.hpp"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <cstdlib>
#include <vector>
#include <iostream>

using namespace coreneuron;
// UNIT TESTS
TEST_CASE("priority_queue_nq_dq") {
    TQueue<pq_que> tq = TQueue<pq_que>();
    const int num = 8;
    int cnter = 0;
    // enqueue 8 items with increasing time
    for (int i = 0; i < num; ++i)
        tq.insert(static_cast<double>(i), NULL);

    REQUIRE(tq.pq_que_.size() == (num - 1));

    // dequeue items with time <= 5.0. Should be 6 events: from 0. to 5.
    TQItem* item = NULL;
    while ((item = tq.atomic_dq(5.0)) != NULL) {
        ++cnter;
        delete item;
    }
    REQUIRE(cnter == 6);
    REQUIRE(tq.pq_que_.size() == (num - 6 - 1));

    // dequeue the rest
    while ((item = tq.atomic_dq(8.0)) != NULL) {
        ++cnter;
        delete item;
    }

    REQUIRE(cnter == num);
    REQUIRE(tq.pq_que_.empty());
    REQUIRE(tq.least() == NULL);
}

TEST_CASE("tqueue_ordered_test") {
    TQueue<pq_que> tq = TQueue<pq_que>();
    const int num = 10;
    int cnter = 0;
    double time = double();

    // insert N items with time < N
    for (int i = 0; i < num; ++i) {
        time = static_cast<double>(rand() % num);
        tq.insert(time, NULL);
    }

    time = 0.0;
    TQItem* item = NULL;
    // dequeue all items and check that previous item time <= current item time
    while ((item = tq.atomic_dq(10.0)) != NULL) {
        REQUIRE(time <= item->t_);
        ++cnter;
        time = item->t_;
        delete item;
    }
    REQUIRE(cnter == num);
    REQUIRE(tq.pq_que_.empty());
    REQUIRE(tq.least() == NULL);
}

TEST_CASE("tqueue_move_nolock") {}

TEST_CASE("tqueue_remove") {}

TEST_CASE("threaddata_interthread_send") {
    NetCvodeThreadData nt{};
    const size_t num = 6;
    for (size_t i = 0; i < num; ++i)
        nt.interthread_send(static_cast<double>(i), NULL, NULL);

    REQUIRE(nt.inter_thread_events_.size() == num);
}
/*
TEST_CASE(threaddata_enqueue){
    NetCvode n = NetCvode();
    const int num = 6;
    for(int i = 0; i < num; ++i)
        n.p[1].interthread_send(static_cast<double>(i), NULL, NULL);

    REQUIRE(n.p[1].inter_thread_events_.size() == num);

    //enqueue the inter_thread_events_
    n.p[1].enqueue(&n, &(n.p[1]));
    REQUIRE(n.p[1].inter_thread_events_.empty());
    REQUIRE(n.p[1].tqe_->pq_que_.size() == num);

    //cleanup priority queue
    TQItem* item = NULL;
    while((item = n.p[1].tqe_->atomic_dq(6.0)) != NULL)
        delete item;
}*/
