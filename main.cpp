#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCParticleSystemQuad.hpp>
using namespace geode::prelude;

static bool g_lagMode = false;

static void applySettings() {
    auto director = CCDirector::sharedDirector();
    bool on = Mod::get()->getSettingValue<bool>("enabled");

    director->setDisplayStats(Mod::get()->getSettingValue<bool>("show-fps"));

    int64_t fps = on ? Mod::get()->getSettingValue<int64_t>("fps-limit") : 60;
    if (fps < 10) fps = 10;
    director->setAnimationInterval(1.0 / static_cast<double>(fps));

#ifdef GEODE_IS_WINDOWS
    if (on && Mod::get()->getSettingValue<bool>("disable-vsync")) {
        CCApplication::sharedApplication()->toggleVerticalSync(false);
    }
#endif
}

$on_mod(Loaded) {
    listenForSettingChanges<bool>("enabled", [](bool) { applySettings(); });
    listenForSettingChanges<int64_t>("fps-limit", [](int64_t) { applySettings(); });
    listenForSettingChanges<bool>("show-fps", [](bool) { applySettings(); });
    listenForSettingChanges<bool>("disable-vsync", [](bool) { applySettings(); });
}

class $modify(OptiMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        applySettings();
        return true;
    }
};

class $modify(OptiPlayLayer, PlayLayer) {
    struct Fields {
        float acc = 0.f;
        int frames = 0;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        g_lagMode = false;
        applySettings(); // se vuelve a aplicar al entrar a cada nivel
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        if (!Mod::get()->getSettingValue<bool>("enabled")) return;
        if (!Mod::get()->getSettingValue<bool>("auto-lag")) return;

        m_fields->acc += dt;
        m_fields->frames++;
        if (m_fields->acc < 1.f) return;

        float fps = static_cast<float>(m_fields->frames) / m_fields->acc;
        m_fields->acc = 0.f;
        m_fields->frames = 0;

        float threshold = static_cast<float>(Mod::get()->getSettingValue<int64_t>("lag-fps"));
        if (fps < threshold) g_lagMode = true;
        else if (fps > threshold * 1.4f) g_lagMode = false;
    }

    void onQuit() {
        g_lagMode = false;
        PlayLayer::onQuit();
    }
};

class $modify(OptiParticles, CCParticleSystemQuad) {
    void draw() {
        if (Mod::get()->getSettingValue<bool>("enabled") &&
            (Mod::get()->getSettingValue<bool>("no-particles") || g_lagMode)) {
            return;
        }
        CCParticleSystemQuad::draw();
    }
};
