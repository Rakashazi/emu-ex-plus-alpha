#include "../src/bridge/Bridge.hh"
#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
using namespace SnesBridge;
template <class F> void rejects(F f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::exception &) {
        rejected = true;
    }
    assert(rejected);
}
int main() {
    Bridge b;
    std::array<uint8_t, WramSize> ram{};
    uint8_t rom[] = {1, 2, 3};
    rejects([&] { b.read(false, 0, 1); });
    b.open(rom, 3);
    rejects([&] { b.read(false, 0, 1); });
    assert(b.read(true, 2, 1).bytes[0] == 3);
    ram[WramSize - 1] = 42;
    b.publish(ram.data());
    auto s = b.read(false, WramSize - 1, 1);
    assert(s.bytes[0] == 42 && s.frame == 1);
    rejects([&] { b.read(false, -1, 1); });
    rejects([&] { b.read(false, 0x1ffff, 2); });
    rejects([&] { b.read(true, 3, 1); });
    rejects([&] { b.read(false, 0, 0); });
    b.queue({{0x80, 2}, {0x100, 1}, {0, 1}});
    assert(b.next() == 0x80);
    assert(b.next() == 0x80);
    b.pause(true);
    assert(b.next() == 0);
    b.pause(false);
    assert(b.next() == 0x100);
    assert(b.next() == 0);
    assert(b.next() == 0);
    b.queue({{0x8000, 10}});
    b.manualInput(0, 0x100);
    assert(b.next() == 0);
    rejects([&] { b.queue({{0x8000, 1}}); });
    b.manualInput(0, 0);
    b.queue({{0x8000, 1}});
    b.cancel();
    assert(b.next() == 0);
    rejects([&] { b.queue({{1, 1}}); });
    rejects([&] { b.queue({{0, 0}}); });
    rejects([&] { b.queue({{0, 36001}}); });
    b.queue({{0x8000, 10}});
    b.close();
    assert(b.next() == 0);
    rejects([&] { b.read(true, 0, 1); });
    b.open(rom, 3);
    b.publish(ram.data());
    assert(b.read(false, 0, 1).generation > s.generation);
    b.discontinuity();
    rejects([&] { b.read(false, 0, 1); });
    b.publish(ram.data());
    b.queue({{0x8000, 10}});
    std::this_thread::sleep_for(std::chrono::milliseconds(2050));
    assert(b.next() == 0);
    assert(b.info()[0] == 1 && b.info()[2] == 3);
    std::atomic<bool> done = false;
    std::thread reader([&] {
        while (!done) {
            auto x = b.read(false, 0, WramSize);
            assert(x.bytes.front() == x.bytes[1]);
        }
    });
    for (int n = 0; n < 100; ++n) {
        ram.fill(uint8_t(n));
        b.publish(ram.data());
    }
    done = true;
    reader.join();
    std::cout << "bridge tests passed\n";
}
