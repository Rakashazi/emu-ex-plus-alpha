// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace SnesBridge {
constexpr uint32_t WramSize = 0x20000;
constexpr uint16_t ButtonMask = 0xfff0;
struct Step {
    uint16_t mask;
    uint32_t frames;
};
struct Snapshot {
    uint64_t generation, frame;
    std::vector<uint8_t> bytes;
};
// No core pointers leave the emulation thread. Binder reads immutable copies.
class Bridge {
    std::mutex mutex;
    std::array<uint8_t, WramSize> wram{};
    std::vector<uint8_t> rom;
    std::deque<Step> steps;
    uint64_t generation = 0, frame = 0;
    bool loaded = false, ready = false, paused = false;
    std::array<uint16_t, 5> manual{};
    std::chrono::steady_clock::time_point lease{};

  public:
    void open(const uint8_t *data, size_t size) {
        if (!data || size == 0 || size > 0x1000000)
            throw std::invalid_argument("ROM size");
        std::lock_guard lock(mutex);
        rom.assign(data, data + size);
        loaded = true;
        ready = false;
        ++generation;
        frame = 0;
        steps.clear();
        manual.fill(0);
        paused = false;
    }
    void close() {
        std::lock_guard lock(mutex);
        loaded = ready = false;
        rom.clear();
        steps.clear();
        manual.fill(0);
        ++generation;
    }
    std::array<uint64_t, 5> info() {
        std::lock_guard lock(mutex);
        return {uint64_t(loaded), uint64_t(ready), uint64_t(rom.size()), generation, frame};
    }
    void discontinuity() {
        std::lock_guard lock(mutex);
        ++generation;
        frame = 0;
        ready = false;
        steps.clear();
    }
    void publish(const uint8_t *data) {
        std::lock_guard lock(mutex);
        if (!loaded)
            return;
        std::copy(data, data + WramSize, wram.begin());
        ++frame;
        ready = true;
    }
    Snapshot read(bool readRom, int offset, int length) {
        std::lock_guard lock(mutex);
        if (!loaded || (!readRom && !ready))
            throw std::runtime_error("NO_CONTENT");
        size_t size = readRom ? rom.size() : WramSize;
        if (offset < 0 || length < 1 || length > 0x20000 || size_t(offset) > size ||
            size_t(length) > size - size_t(offset))
            throw std::invalid_argument("RANGE");
        const uint8_t *data = readRom ? rom.data() : wram.data();
        return {generation, frame, {data + offset, data + offset + length}};
    }
    void queue(const std::vector<Step> &sequence) {
        if (sequence.empty() || sequence.size() > 256)
            throw std::invalid_argument("SEQUENCE");
        uint64_t total = 0;
        for (auto s : sequence) {
            if ((s.mask & ~ButtonMask) || s.frames < 1 || s.frames > 36000)
                throw std::invalid_argument("STEP");
            total += s.frames;
        }
        if (total > 36000)
            throw std::invalid_argument("DURATION");
        std::lock_guard lock(mutex);
        if (!loaded)
            throw std::runtime_error("NO_CONTENT");
        for (auto bits : manual)
            if (bits)
                throw std::runtime_error("MANUAL_INPUT");
        steps.assign(sequence.begin(), sequence.end());
        paused = false;
        lease = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    }
    void heartbeat() {
        std::lock_guard lock(mutex);
        lease = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    }
    void cancel() {
        std::lock_guard lock(mutex);
        steps.clear();
    }
    void pause(bool value) {
        std::lock_guard lock(mutex);
        paused = value;
    }
    void manualInput(unsigned player, uint16_t bits) {
        std::lock_guard lock(mutex);
        if (player < manual.size())
            manual[player] = bits;
        steps.clear(); // every manual transition cancels automation; never auto-resume it
    }
    uint16_t next() {
        std::lock_guard lock(mutex);
        if (std::chrono::steady_clock::now() >= lease)
            steps.clear();
        if (paused || steps.empty() || !loaded)
            return 0;
        uint16_t result = steps.front().mask;
        if (--steps.front().frames == 0)
            steps.pop_front();
        return result;
    }
};
Bridge &instance();
} // namespace SnesBridge
