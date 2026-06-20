#pragma once

#include <cstddef>
#include <vector>

namespace neuron {
struct model_sorted_token;
class InterleaveInfo;
}  // namespace neuron

namespace neuron::gpu {

/** Tracks host buffers mirrored on the device for teardown via nrn_target_delete. */
class UploadState {
  public:
    void teardown();

    [[nodiscard]] std::size_t mirror_count() const noexcept {
        return mirrors_.size();
    }

    [[nodiscard]] bool is_present(void const* host_ptr) const;

    void record(void const* host, std::size_t count, std::size_t sizeof_elem);

  private:
    struct Mirror {
        void const* host{};
        std::size_t count{};
        std::size_t sizeof_elem{};
    };

    std::vector<Mirror> mirrors_{};
};

/** Upload sorted node/mech SOA vectors and InterleaveInfo (permute 1/2) to the device. */
void upload_sorted_model(model_sorted_token const& sorted, UploadState& state);

namespace detail {
/** Test hook: upload one InterleaveInfo with CoreNEURON struct-then-patch pattern. */
void upload_interleave_info_for_testing(int permute_type,
                                        InterleaveInfo& info,
                                        int ncell,
                                        UploadState& state);
}  // namespace detail

}  // namespace neuron::gpu