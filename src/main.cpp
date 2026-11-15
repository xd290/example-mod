#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCParticleSystem.hpp>

using namespace geode::prelude;

static bool g_ocultarParticulas = false;

class $modify(FPSBoostPlayer, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);

        auto mod = Mod::get();

        if (mod->getSettingValue<bool>("sin-particulas")) {
            this->apagar(m_playerGroundParticles);
            this->apagar(m_vehicleGroundParticles);
            this->apagar(m_trailingParticles);
            this->apagar(m_shipClickParticles);
            this->apagar(m_ufoClickParticles);
            this->apagar(m_robotBurstParticles);
            this->apagar(m_dashParticles);
            this->apagar(m_swingBurstParticles1);
            this->apagar(m_swingBurstParticles2);
            this->apagar(m_landParticles0);
            this->apagar(m_landParticles1);
        }

        if (mod->getSettingValue<bool>("sin-estelas")) {
            if (m_regularTrail) m_regularTrail->setVisible(false);
            if (m_waveTrail) m_waveTrail->setVisible(false);
        }
    }

    void apagar(CCParticleSystem* p) {
        if (!p) return;
        p->stopSystem();
        p->setVisible(false);
    }
};

class $modify(FPSBoostLayer, PlayLayer) {
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto mod = Mod::get();
        g_ocultarParticulas = mod->getSettingValue<bool>("sin-particulas-nivel");

        if (mod->getSettingValue<bool>("ocultar-fondo")) {
            if (m_background) m_background->setVisible(false);
            if (m_middleground) m_middleground->setVisible(false);
        }

        if (mod->getSettingValue<bool>("ocultar-suelo")) {
            if (m_groundLayer) m_groundLayer->setVisible(false);
        }
    }

    void onQuit() {
        PlayLayer::onQuit();
        g_ocultarParticulas = false;
    }
};

class $modify(FPSBoostParticles, CCParticleSystem) {
    void update(float dt) {
        CCParticleSystem::update(dt);
        if (g_ocultarParticulas) {
            this->setVisible(false);
        }
    }
};
