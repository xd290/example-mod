// IA para Geometry Dash (v2): simula el futuro y decide si tocar o no.
// La técnica de simulación (jugador falso + checkCollisions + update) está
// basada en el Show Trajectory de Eclipse Menu (EPL-2.0).
// Solo actúa en MODO PRÁCTICA.

#include <Geode/Geode.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace ia {
    // Cuántos frames hacia adelante simula en cada rama. Si el juego va lento
    // en tu celular, bájalo (por ejemplo a 40).
    constexpr int HORIZON = 60;

    struct Simulation {
        PlayerObject* sim = nullptr;
        bool simulating = false;
        bool simDead = false;
        bool pressed = false;   // estado real del botón del jugador 1
        float dt = 1.f / 60.f;

        void init() {
            auto pl = PlayLayer::get();
            sim = PlayerObject::create(1, 1, pl, pl, true);
            sim->setPosition({0, 105});
            sim->setVisible(false);
            sim->setID("ia-sim-player"_spr);
            pl->m_objectLayer->addChild(sim);
        }

        void quit() { sim = nullptr; }

        bool isSimDead(PlayerObject* p) {
            if (simulating && p == sim) {
                simDead = true;
                return true;
            }
            return false;
        }

        void resetCollisionLog() {
            sim->m_collisionLogTop->removeAllObjects();
            sim->m_collisionLogBottom->removeAllObjects();
            sim->m_collisionLogLeft->removeAllObjects();
            sim->m_collisionLogRight->removeAllObjects();
        }

        // Simula una rama (botón abajo o arriba). Devuelve cuántos frames sobrevive.
        int run(PlayerObject* base, bool down) {
            auto pl = PlayLayer::get();
            simDead = false;
            sim->setVisible(false);
            sim->copyAttributes(base);
            sim->m_gravityMod = base->m_gravityMod;
            sim->m_isOnGround = base->m_isOnGround;

            if (pressed) {
                down ? sim->pushButton(PlayerButton::Jump) : sim->releaseButton(PlayerButton::Jump);
            }

            bool acted = false;
            for (int i = 0; i < HORIZON; i++) {
                resetCollisionLog();
                pl->checkCollisions(sim, dt, false);
                if (simDead) return i;

                if (!acted && !pressed) {
                    acted = true;
                    down ? sim->pushButton(PlayerButton::Jump) : sim->releaseButton(PlayerButton::Jump);
                }
                sim->update(dt);
            }
            return HORIZON;
        }

        // true = tocar, false = soltar.
        bool decide(PlayerObject* base) {
            simulating = true;
            int tocando = run(base, true);
            int soltando = run(base, false);
            simulating = false;
            return tocando > soltando; // en empate, no toca
        }
    };

    inline Simulation& S() {
        static Simulation s;
        return s;
    }
}

class $modify(IAPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        ia::S().init();
        return true;
    }

    // Si muere el jugador falso, no destruye el real.
    void destroyPlayer(PlayerObject* player, GameObject* gameObject) override {
        if (gameObject != m_anticheatSpike && ia::S().isSimDead(player)) return;
        PlayLayer::destroyPlayer(player, gameObject);
    }

    void playEndAnimationToPos(cocos2d::CCPoint p0) {
        if (ia::S().simulating) return;
        PlayLayer::playEndAnimationToPos(p0);
    }

    void onQuit() {
        PlayLayer::onQuit();
        ia::S().quit();
    }
};

// Lo siguiente evita que la simulación tenga efectos en el juego real.
class $modify(IAHardStreak, HardStreak) {
    void addPoint(cocos2d::CCPoint p0) {
        if (ia::S().simulating) return;
        HardStreak::addPoint(p0);
    }
};

class $modify(IAEffectObject, EffectGameObject) {
    void triggerObject(GJBaseGameLayer* p0, int p1, const gd::vector<int>* p2) override {
        if (ia::S().simulating) return;
        EffectGameObject::triggerObject(p0, p1, p2);
    }
};

class $modify(IAGameObject, GameObject) {
    void playShineEffect() {
        if (ia::S().simulating) return;
        GameObject::playShineEffect();
    }
};

class $modify(IAPlayerObject, PlayerObject) {
    void playSpiderDashEffect(cocos2d::CCPoint from, cocos2d::CCPoint to) {
        if (ia::S().simulating) return;
        PlayerObject::playSpiderDashEffect(from, to);
    }

    void incrementJumps() {
        if (ia::S().simulating) return;
        PlayerObject::incrementJumps();
    }

    void update(float dt) override {
        PlayerObject::update(dt);
        auto pl = PlayLayer::get();
        if (pl && !ia::S().simulating) {
            ia::S().dt = dt / pl->m_gameState.m_timeWarp;
        }
    }

    void ringJump(RingObject* p0, bool p1) {
        if (ia::S().simulating) return;
        PlayerObject::ringJump(p0, p1);
    }
};

class $modify(IAGameLayer, GJBaseGameLayer) {
    bool canBeActivatedByPlayer(PlayerObject* p0, EffectGameObject* p1) {
        if (ia::S().simulating) return false;
        return GJBaseGameLayer::canBeActivatedByPlayer(p0, p1);
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        if (button == 1 && isPlayer1) ia::S().pressed = down;
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void flipGravity(PlayerObject* p0, bool p1, bool p2) {
        if (ia::S().simulating) return;
        GJBaseGameLayer::flipGravity(p0, p1, p2);
    }

    // Durante la simulación solo cuenta lo que puede matar o bloquear.
    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* vec, int objectsCount, float dt) {
        if (ia::S().simulating) {
            gd::vector<GameObject*> extra;
            for (int i = 0; i < objectsCount; i++) {
                GameObject* obj = vec->at(i);
                if (obj->m_objectType == GameObjectType::Solid ||
                    obj->m_objectType == GameObjectType::Hazard ||
                    obj->m_objectType == GameObjectType::AnimatedHazard ||
                    obj->m_objectType == GameObjectType::Slope) {
                    extra.push_back(obj);
                }
            }
            GJBaseGameLayer::collisionCheckObjects(player, &extra, extra.size(), dt);
            return;
        }
        GJBaseGameLayer::collisionCheckObjects(player, vec, objectsCount, dt);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (ia::S().simulating) return;
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }

    // Cada frame: simula las dos opciones y elige la que sobrevive más.
    void updateCamera(float dt) {
        auto pl = PlayLayer::get();
        auto& s = ia::S();
        if (pl && s.sim && pl->m_isPracticeMode && !s.simulating && !pl->m_player1->m_isDead) {
            bool want = s.decide(pl->m_player1);
            if (want != s.pressed) {
                this->handleButton(want, 1, true);
            }
        }
        GJBaseGameLayer::updateCamera(dt);
    }
};
