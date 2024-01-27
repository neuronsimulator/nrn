#include "test_multicore.h"

#include "code.h"
#include "hocdec.h"
#include "multicore.h"
#include "nrn_ansi.h"
#include "ocfunc.h"

#include <catch2/catch.hpp>

#include <algorithm>
#include <iostream>
#include <vector>

/* @brief
 *  Test multicore implementation:
 *  * parallel mode (std::threads)
 *  * parallel mode with busywait
 *  * serial mode
 *  * performance
 *      * NOTE: GitHub runners don't have enough capabilities for performance KPIs
 */


TEST_CASE("Multicore unit and performance testing", "[NEURON][multicore]") {
    static const auto nof_threads_range{nrn::test::make_available_threads_range()};
    SECTION("Simulation set-up", "[NEURON][multicore][setup]") {
        WHEN("We create a parallel context") {
            THEN("we make sure we have a parallel context") {
                REQUIRE(hoc_oc("objref pc\n"
                               "pc = new ParallelContext()") == 0);
            }
            THEN("we assert nrn_thread is equal to 1") {
                REQUIRE(nrn_nthread == 1);
            }
            THEN("we check we have no worker threads") {
                REQUIRE(nof_worker_threads() == 0);
            }
        }
        WHEN("We setup the cells for the simulation") {
            THEN("we create 1000 passive membrane cells for the simulation") {
                REQUIRE(hoc_oc(pass_cell_template) == 0);
                REQUIRE(hoc_oc(prun) == 0);
                std::string cells{1000_pas_cells};
                REQUIRE(hoc_oc(cells.c_str()) == 0);
            }
        }
    }

    SECTION("Test parallel mode", "[NEURON][multicore][parallel]") {
        static std::vector<double> cache_sim_times;
        GIVEN("we do prun() over each nof_threads{nof_threads_range}") {
            auto nof_threads = GENERATE_COPY(from_range(nof_threads_range));
            THEN("we run the simulation with " + std::to_string(nof_threads) + " threads") {
                nrn_threads_create(nof_threads, 1);
                REQUIRE(nrn_nthread == nof_threads);
                REQUIRE(nof_worker_threads() == (nof_threads > 1 ? nof_threads : 0));
                auto start = std::chrono::high_resolution_clock::now();
                REQUIRE(hoc_oc("prun()") == 0);
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                cache_sim_times.push_back(duration.count());
                REQUIRE(nof_worker_threads() == (nof_threads > 1 ? nof_threads : 0));
            }
        }
        THEN("we assert all simulations ran") {
            REQUIRE(cache_sim_times.size() == nof_threads_range.size());
        }
        THEN("we print the results") {
            std::cout << "[parallel][simulation times] : " << std::endl;
            std::cout << "nt"
                      << "\t"
                      << "cache=0"
                      << "\t\t"
                      << "cache=1" << std::endl;
            for (auto i = 0; i < cache_sim_times.size(); ++i) {
                std::cout << nof_threads_range[i] << "\t" << cache_sim_times[i] << std::endl;
            }
        }
        THEN("we check that the more threads we have the faster the simulation runs") {
            if (nof_threads_range.size() > 2) {
                REQUIRE(std::is_sorted(cache_sim_times.rbegin(), cache_sim_times.rend()));
                THEN(
                    "we check that the standard deviaton is above 25% from the mean for simulation "
                    "vectors") {
                    const auto cache_mean =
                        std::accumulate(cache_sim_times.begin(), cache_sim_times.end(), 0.0) /
                        cache_sim_times.size();
                    const auto cache_std_dev = std::sqrt(
                        std::accumulate(cache_sim_times.begin(),
                                        cache_sim_times.end(),
                                        0.0,
                                        [cache_mean](double a, double b) {
                                            return a + (b - cache_mean) * (b - cache_mean);
                                        }) /
                        cache_sim_times.size());
                    REQUIRE(cache_std_dev / cache_mean > 0.2);
                    // print the standard deviations
                    std::cout << "[parallel][cache][standard deviation] : " << cache_std_dev
                              << std::endl;
                }
            } else {
                WARN("Not enough threads to test parallel performance KPI");
            }
        }
    }

    SECTION("Test serial mode", "[NEURON][multicore][serial]") {
        static std::vector<double> sim_times;
        GIVEN("we do prun() over each nof_threads{nof_threads_range} with serial mode on") {
            auto nof_threads = GENERATE_COPY(from_range(nof_threads_range));
            THEN("we run the serial simulation with " << nof_threads << " threads") {
                nrn_threads_create(nof_threads, 0);
                REQUIRE(nrn_nthread == nof_threads);
                REQUIRE(nof_worker_threads() == 0);
                auto start = std::chrono::high_resolution_clock::now();
                REQUIRE(hoc_oc("prun()") == 0);
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                sim_times.push_back(duration.count());
                REQUIRE(nof_worker_threads() == 0);
            }
        }
        THEN("we assert all simulations ran") {
            REQUIRE(sim_times.size() == nof_threads_range.size());
        }
        THEN("we print the results") {
            std::cout << "[serial][simulation times] : " << std::endl;
            std::cout << "nt"
                      << "\t"
                      << "cache=1" << std::endl;
            for (auto i = 0; i < sim_times.size(); ++i) {
                std::cout << nof_threads_range[i] << "\t" << sim_times[i] << std::endl;
            }
        }
        THEN("we assert sim_times have under 10% standard deviation from the mean") {
            if (nof_threads_range.size() > 2) {
                const auto mean = std::accumulate(sim_times.begin(), sim_times.end(), 0.0) /
                                  sim_times.size();
                const auto sq_sum =
                    std::inner_product(sim_times.begin(), sim_times.end(), sim_times.begin(), 0.0);
                const auto stdev = std::sqrt(sq_sum / sim_times.size() - mean * mean);

                std::cout << "[serial][standard deviation] : " << stdev << std::endl;
                REQUIRE(stdev < 0.1 * mean);
            } else {
                WARN("Not enough threads to test serial performance KPI");
            }
        }
    }

    SECTION("Test busywait parallel mode", "[NEURON][multicore][parallel][busywait]") {
        WHEN("busywait is set to 1") {
            THEN("set thread_busywait to 1") {
                REQUIRE(hoc_oc("pc.thread_busywait(1)") == 0);
            }
            static std::vector<double> sim_times;
            GIVEN("we do prun() over each nof_threads{nof_threads_range} with serial mode on") {
                auto nof_threads = GENERATE_COPY(from_range(nof_threads_range));
                THEN("we run the parallel busywait simulation with " << nof_threads << " threads") {
                    nrn_threads_create(nof_threads, 1);
                    REQUIRE(nrn_nthread == nof_threads);
                    REQUIRE(nof_worker_threads() == (nof_threads > 1 ? nof_threads : 0));
                    auto start = std::chrono::high_resolution_clock::now();
                    REQUIRE(hoc_oc("prun()") == 0);
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end -
                                                                                          start);
                    sim_times.push_back(duration.count());
                    REQUIRE(nof_worker_threads() == (nof_threads > 1 ? nof_threads : 0));
                }
            }
            THEN("we assert all simulations ran") {
                REQUIRE(sim_times.size() == nof_threads_range.size());
            }
            THEN("we print the results") {
                std::cout << "[parallel][busywait][simulation times] : " << std::endl;
                std::cout << "nt"
                          << "\t"
                          << "cache=1" << std::endl;
                for (auto i = 0; i < sim_times.size(); ++i) {
                    std::cout << nof_threads_range[i] << "\t" << sim_times[i] << std::endl;
                }
            }
            THEN("we assert sim_times have over 20% standard deviation from the mean") {
                if (nof_threads_range.size() > 2) {
                    const auto mean = std::accumulate(sim_times.begin(), sim_times.end(), 0.0) /
                                      sim_times.size();
                    const auto sq_sum = std::inner_product(sim_times.begin(),
                                                           sim_times.end(),
                                                           sim_times.begin(),
                                                           0.0);
                    const auto stdev = std::sqrt(sq_sum / sim_times.size() - mean * mean);
                    std::cout << "[parallel][busywait][standard deviation] : " << stdev
                              << std::endl;
                    // standard deviation should be less than 5% of the mean
                    REQUIRE(stdev > 0.2 * mean);
                } else {
                    WARN("Not enough threads to test busywait+parallel performance KPI");
                }
            }
        }
    }
}
