#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/CCParticleSystem.hpp>
#include "trajectory.hpp"

using namespace geode::prelude;

static bool g_ocultarParticulas = false;
static bool g_ocultarDeco = false;

static bool activo(char const* clave) {
    auto mod = Mod::get();
    return mod->getSettingValue<bool>("modo-extremo") ||
           mod->getSettingValue<bool>(clave);
}

// ==========================================
// TU CÓDIGO DE FPSBOOST (INTACTO)
// ==========================================
class $modify(FPSBoostPlayer, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);

        if (activo("sin-particulas")) {
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

        if (activo("sin-estelas")) {
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

        g_ocultarParticulas = activo("sin-particulas-nivel");
        g_ocultarDeco = activo("ocultar-decoracion");

        if (activo("ocultar-fondo")) {
            if (m_background) m_background->setVisible(false);
            if (m_middleground) m_middleground->setVisible(false);
        }

        if (activo("ocultar-suelo")) {
            if (m_groundLayer) m_groundLayer->setVisible(false);
        }
    }

    void onQuit() {
        PlayLayer::onQuit();
        g_ocultarParticulas = false;
        g_ocultarDeco = false;
    }
};

class $modify(FPSBoostObject, GameObject) {
    void activateObject() {
        GameObject::activateObject();
        if (g_ocultarDeco && m_objectType == GameObjectType::Decoration) {
            this->setVisible(false);
        }
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

// ==========================================
// NUEVO CÓDIGO DEL BOT
// ==========================================
class $modify(BotLayer, PlayLayer) {
    struct Fields {
        std::vector<TrajectoryPlayerData> m_cachedTrajectory;
        cocos2d::CCDrawNode* m_drawNode = nullptr;
        PredictionConfig m_config;
        bool m_botEnabled = true; // Cambiar a true para probar el bot
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        
        m_fields->m_drawNode = cocos2d::CCDrawNode::create();
        this->addChild(m_fields->m_drawNode, 100); 
        return true;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        
        if (!m_fields->m_botEnabled || !m_player1) return;

        // 1. Limpiar y calcular la nueva trayectoria
        m_fields->m_cachedTrajectory.clear();
        for (int i = 0; i < m_fields->m_config.m_maxLength; ++i) {
            if (!BotTrajectory::iterate(this, m_player1, m_fields->m_cachedTrajectory, m_fields->m_config)) {
                break; 
            }
        }

        // 2. Dibujar la línea
        BotTrajectory::drawTrajectory(m_fields->m_drawNode, m_fields->m_cachedTrajectory);
    }
};
