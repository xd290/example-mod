#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

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
