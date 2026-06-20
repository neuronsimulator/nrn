#pragma once

#include <cstddef>
#include <memory>

namespace neuron {
struct model_sorted_token;
}

namespace neuron::gpu {

/**
 * @brief RAII handle for GPU mirrors of a sorted model layout.
 *
 * Lifetime is tied to model_sorted_token / frozen-token refcounting: GPU teardown
 * runs when the last model_sorted_token for the active layout is destroyed.
 * Multiple device_token instances may share the same upload (refcounted).
 */
class device_token {
  public:
    explicit device_token(model_sorted_token const& sorted);
    device_token(device_token const& other);
    device_token(device_token&& other) noexcept;
    device_token& operator=(device_token const&) = delete;
    device_token& operator=(device_token&&) = delete;
    ~device_token();

    [[nodiscard]] bool is_on_device() const;

    /** @brief Post-psolve selective download (stub until PR 10). */
    void update_host();

    /** @brief Pre-step host→device sync for VecPlay/HOC writes (stub until PR 10). */
    void update_device();

  private:
    struct State;
    std::shared_ptr<State> m_state;
};

/**
 * @brief Upload sorted SOA vectors to the device on first GPU step.
 *
 * Returns a device_token referencing the shared upload state for the current
 * sorted layout. Safe to call repeatedly; upload happens at most once per layout.
 */
[[nodiscard]] device_token const& ensure_on_device(model_sorted_token const& sorted);

/** @brief Discard GPU mirrors when the sorted layout is invalidated. */
void invalidate_device_state();

namespace detail {
void on_sorted_token_created();
void on_sorted_token_destroyed();

/** @brief Test hook: number of live model_sorted_token registrations. */
[[nodiscard]] std::size_t sorted_token_count_for_testing();

/** @brief Test hook: whether the active layout has been uploaded. */
[[nodiscard]] bool is_on_device_for_testing();

/** @brief Test hook: number of device mirrors recorded for the active layout. */
[[nodiscard]] std::size_t mirror_count_for_testing();

/** @brief Test hook: whether a host pointer was copyin'd for the active layout. */
[[nodiscard]] bool is_present_for_testing(void const* host_ptr);
}  // namespace detail

}  // namespace neuron::gpu
