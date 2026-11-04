// SPDX-License-Identifier: GPL-3.0-or-later
#include "Bridge.hh"
namespace SnesBridge {
namespace {
Bridge bridge;
}
Bridge &instance() { return bridge; }
} // namespace SnesBridge
#ifdef __ANDROID__
#include <jni.h>
static void fail(JNIEnv *env, const char *type, const char *message) {
    env->ThrowNew(env->FindClass(type), message);
}
extern "C" JNIEXPORT jlongArray JNICALL Java_org_snes9x_bridge_NativeBridge_info(JNIEnv *env,
                                                                                 jclass) {
    auto metadata = SnesBridge::instance().info();
    jlong values[5];
    for (int i = 0; i < 5; ++i)
        values[i] = jlong(metadata[i]);
    auto result = env->NewLongArray(5);
    if (result)
        env->SetLongArrayRegion(result, 0, 5, values);
    return result;
}
extern "C" JNIEXPORT jbyteArray JNICALL Java_org_snes9x_bridge_NativeBridge_read(
    JNIEnv *env, jclass, jboolean rom, jint offset, jint length, jlongArray metadata) {
    try {
        if (!metadata || env->GetArrayLength(metadata) != 2)
            throw std::invalid_argument("metadata");
        auto snapshot = SnesBridge::instance().read(rom, offset, length);
        jlong info[] = {jlong(snapshot.generation), jlong(snapshot.frame)};
        env->SetLongArrayRegion(metadata, 0, 2, info);
        auto result = env->NewByteArray(length);
        if (result)
            env->SetByteArrayRegion(result, 0, length,
                                    reinterpret_cast<const jbyte *>(snapshot.bytes.data()));
        return result;
    } catch (const std::invalid_argument &e) {
        fail(env, "java/lang/IllegalArgumentException", e.what());
    } catch (const std::exception &e) {
        fail(env, "java/lang/IllegalStateException", e.what());
    }
    return nullptr;
}
extern "C" JNIEXPORT void JNICALL Java_org_snes9x_bridge_NativeBridge_queue(JNIEnv *env, jclass,
                                                                            jintArray masks,
                                                                            jintArray frames) {
    try {
        if (!masks || !frames)
            throw std::invalid_argument("sequence");
        int n = env->GetArrayLength(masks);
        if (n < 1 || n > 256 || env->GetArrayLength(frames) != n)
            throw std::invalid_argument("sequence");
        jint m[256], f[256];
        env->GetIntArrayRegion(masks, 0, n, m);
        env->GetIntArrayRegion(frames, 0, n, f);
        std::vector<SnesBridge::Step> steps;
        for (int i = 0; i < n; ++i) {
            if (m[i] < 0 || m[i] > 65535 || f[i] < 1)
                throw std::invalid_argument("step");
            steps.push_back({uint16_t(m[i]), uint32_t(f[i])});
        }
        SnesBridge::instance().queue(steps);
    } catch (const std::invalid_argument &e) {
        fail(env, "java/lang/IllegalArgumentException", e.what());
    } catch (const std::exception &e) {
        fail(env, "java/lang/IllegalStateException", e.what());
    }
}
extern "C" JNIEXPORT void JNICALL Java_org_snes9x_bridge_NativeBridge_command(JNIEnv *, jclass,
                                                                              jint command) {
    auto &bridge = SnesBridge::instance();
    switch (command) {
    case 0:
        bridge.cancel();
        break;
    case 1:
        bridge.pause(true);
        break;
    case 2:
        bridge.pause(false);
        break;
    case 3:
        bridge.heartbeat();
        break;
    }
}
#endif
