#include <catch2/catch.hpp>

#include <hocdec.h>
#include <ocfunc.h>
#include <code.h>
#include <thread>
#include <algorithm>
#include <vector>
#include "../unit_test.h"

#ifdef NRN_ENABLE_THREADS
#include <multicore.h>
#include <nrn_ansi.h>
TEST_CASE("Test usage of std::threads and performance increase", "[NEURON][multicore]") {
    WHEN("We create a parallel context") {
        hoc_oc("objref pc");
        hoc_oc("pc = new ParallelContext()");
        THEN("we assert nrn_thread is equal to 1") {
            REQUIRE(nrn_nthread == 1);
        }
        THEN("we check we have no worker threads") {
            REQUIRE(nof_worker_threads() == 0);
        }
        const auto nof_concurrent_threads = std::thread::hardware_concurrency();
        // we generate a range starting with 1 and then doubling it until we reach the number of
        // concurrent threads or 8
        auto nof_threads_range =
            std::vector<int>(1 + static_cast<int>(std::log2(nof_concurrent_threads)), 1);
        std::generate(nof_threads_range.begin() + 1, nof_threads_range.end(), [n = 1]() mutable {
            return n *= 2;
        });
        static std::vector<double> cache_sim_times;
        static std::vector<double> no_cache_sim_times;

        GIVEN("we instantiate 1000 passive membrane 1000 Cell objects") {
            hoc_oc(pass_cell_template);
            hoc_oc(prun);
            std::string cells{1000_pass_cells};
            hoc_oc(cells.c_str());
        }
        THEN("we do prun() over each nof_threads{nof_threads_range} with cachevec{0,1}") {
            auto cache_efficient = GENERATE(0, 1);
            auto nof_threads = GENERATE_REF(from_range(nof_threads_range));
            WHEN("we run the simulation with " + std::to_string(nof_threads) + " threads") {
                std::cout << "Cache efficient: " << cache_efficient << std::endl;
                nrn_cachevec(cache_efficient);
                nrn_threads_create(nof_threads, 1);
                std::cout << "Running simulation with " << nof_threads << " threads" << std::endl;
                REQUIRE(nrn_nthread == nof_threads);
                REQUIRE(nof_worker_threads() == (nof_threads > 1 ? nof_threads : 0));
                auto start = std::chrono::high_resolution_clock::now();
                REQUIRE(hoc_oc("prun()") == 0);
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                (cache_efficient ? cache_sim_times : no_cache_sim_times)
                    .push_back(duration.count());
                REQUIRE(nof_worker_threads() == (nof_threads > 1 ? nof_threads : 0));
            }
        }
        THEN("we assert all simulations ran") {
            REQUIRE(cache_sim_times.size() == nof_threads_range.size());
            REQUIRE(no_cache_sim_times.size() == nof_threads_range.size());
        }
        THEN("we check that the more threads we have the faster the simulation runs") {
            REQUIRE(std::is_sorted(cache_sim_times.rbegin(), cache_sim_times.rend()));
            REQUIRE(std::is_sorted(no_cache_sim_times.rbegin(), no_cache_sim_times.rend()));
        }
        THEN("we check that the cachevec is more efficient") {
            REQUIRE(cache_sim_times < no_cache_sim_times);
        }
        THEN("we print the results") {
            std::cout << "nt"
                      << "\t"
                      << "cache=0"
                      << "\t\t"
                      << "cache=1" << std::endl;
            for (auto i = 0; i < cache_sim_times.size(); ++i) {
                std::cout << nof_threads_range[i] << "\t" << no_cache_sim_times[i] << "\t"
                          << cache_sim_times[i] << std::endl;
            }
        }
    }
}
#endif  // NRN_ENABLE_THREADS
