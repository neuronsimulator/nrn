#include "neuron/gpu/device_state.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/device_assign.hpp"
#include "neuron/gpu/download.hpp"
#include "neuron/gpu/upload.hpp"

#include <atomic>
#include <mutex>
#include <stdexcept>

namespace neuron::gpu {

namespace {

struct ModelDeviceState {
    std::atomic<std::size_t> sorted_token_refs{0};
    std::atomic<std::size_t> device_token_refs{0};
    bool on_device{false};
    UploadState upload{};

    void upload_model(model_sorted_token const& sorted) {
        upload_sorted_model(sorted, upload);
        on_device = true;
    }

    void download_to_host() {
        batch_download_to_host();
    }

    void upload_to_device() {
        batch_upload_to_device();
    }

    void teardown() {
        upload.teardown();
        on_device = false;
    }
};

class DeviceStateRegistry {
  public:
    std::shared_ptr<ModelDeviceState> state() {
        std::lock_guard lock{mut_};
        if (!active_) {
            active_ = std::make_shared<ModelDeviceState>();
        }
        return active_;
    }

    void invalidate() {
        std::lock_guard lock{mut_};
        if (active_) {
            active_->teardown();
        }
        cached_ensure_token_ = nullptr;
        cached_ensure_storage_.reset();
    }

    void on_sorted_token_created() {
        auto dev_state = this->state();
        ++dev_state->sorted_token_refs;
    }

    void on_sorted_token_destroyed() {
        std::shared_ptr<ModelDeviceState> state;
        {
            std::lock_guard lock{mut_};
            state = active_;
            if (!state) {
                return;
            }
            auto const remaining = --state->sorted_token_refs;
            if (remaining > 0) {
                return;
            }
            cached_ensure_token_ = nullptr;
            cached_ensure_storage_.reset();
            if (state->device_token_refs > 0) {
                throw std::runtime_error(
                    "neuron::gpu: model_sorted_token destroyed while device_token(s) still alive");
            }
            state->teardown();
            reset_active_locked();
        }
    }

    void on_device_token_created(std::shared_ptr<ModelDeviceState> const& state) {
        ++state->device_token_refs;
    }

    void on_device_token_destroyed(std::shared_ptr<ModelDeviceState> const& state) {
        if (--state->device_token_refs > 0) {
            return;
        }
        std::lock_guard lock{mut_};
        if (active_.get() != state.get()) {
            return;
        }
        if (state->sorted_token_refs > 0) {
            return;
        }
        state->teardown();
        reset_active_locked();
    }

    device_token const& ensure_cached(model_sorted_token const& sorted) {
        {
            std::lock_guard lock{mut_};
            if (cached_ensure_token_) {
                return *cached_ensure_token_;
            }
        }
        auto fresh = std::make_unique<device_token>(sorted);
        std::lock_guard lock{mut_};
        if (!cached_ensure_token_) {
            cached_ensure_storage_ = std::move(fresh);
            cached_ensure_token_ = cached_ensure_storage_.get();
        }
        return *cached_ensure_token_;
    }

    std::size_t sorted_token_count() const {
        std::lock_guard lock{mut_};
        return active_ ? active_->sorted_token_refs.load() : 0;
    }

    bool is_on_device() const {
        std::lock_guard lock{mut_};
        return active_ && active_->on_device;
    }

    std::size_t upload_mirror_count() const {
        std::lock_guard lock{mut_};
        return active_ ? active_->upload.mirror_count() : 0;
    }

    bool upload_is_present(void const* host_ptr) const {
        std::lock_guard lock{mut_};
        return active_ && active_->upload.is_present(host_ptr);
    }

  private:
    void reset_active_locked() {
        active_.reset();
        cached_ensure_token_ = nullptr;
        cached_ensure_storage_.reset();
    }

    mutable std::recursive_mutex mut_{};
    std::shared_ptr<ModelDeviceState> active_{};
    device_token const* cached_ensure_token_{nullptr};
    std::unique_ptr<device_token> cached_ensure_storage_{};
};

DeviceStateRegistry& registry() {
    static DeviceStateRegistry instance;
    return instance;
}

}  // namespace

struct device_token::State {
    std::shared_ptr<ModelDeviceState> model_state;
};

device_token::device_token(model_sorted_token const& sorted)
    : m_state{std::make_shared<State>()} {
    auto model_state = registry().state();
    if (model_state->sorted_token_refs == 0) {
        throw std::runtime_error("neuron::gpu::device_token requires an active model_sorted_token");
    }
    m_state->model_state = std::move(model_state);
    registry().on_device_token_created(m_state->model_state);
    if (!m_state->model_state->on_device) {
#if defined(NRN_ENABLE_GPU)
        if (enabled()) {
            assign_device();
        }
#endif
        m_state->model_state->upload_model(sorted);
    }
}

device_token::device_token(device_token const& other)
    : m_state{other.m_state} {
    if (m_state) {
        registry().on_device_token_created(m_state->model_state);
    }
}

device_token::device_token(device_token&& other) noexcept
    : m_state{std::move(other.m_state)} {}

device_token::~device_token() {
    if (m_state) {
        registry().on_device_token_destroyed(m_state->model_state);
    }
}

bool device_token::is_on_device() const {
    return m_state && m_state->model_state && m_state->model_state->on_device;
}

void device_token::update_host() {
    if (m_state && m_state->model_state) {
        m_state->model_state->download_to_host();
    }
}

void device_token::update_device() {
    if (m_state && m_state->model_state) {
        m_state->model_state->upload_to_device();
    }
}

device_token const& ensure_on_device(model_sorted_token const& sorted) {
    return registry().ensure_cached(sorted);
}

void invalidate_device_state() {
    registry().invalidate();
}

namespace detail {

void on_sorted_token_created() {
    registry().on_sorted_token_created();
}

void on_sorted_token_destroyed() {
    registry().on_sorted_token_destroyed();
}

std::size_t sorted_token_count_for_testing() {
    return registry().sorted_token_count();
}

bool is_on_device_for_testing() {
    return registry().is_on_device();
}

std::size_t mirror_count_for_testing() {
    return registry().upload_mirror_count();
}

bool is_present_for_testing(void const* host_ptr) {
    return registry().upload_is_present(host_ptr);
}

}  // namespace detail

}  // namespace neuron::gpu
