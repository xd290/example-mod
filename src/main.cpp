// Piloto IA para Geometry Dash (v3).
// Simula unas pocas acciones candidatas en modo práctica.

#include <Geode/Geode.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <vector>

using namespace geode::prelude;

// Los bindings actuales no exponen el estado necesario para simular orbes
// sin alterar las del nivel.
#define IA_ORBES 0

namespace ia {
    // Horizonte y cantidad de planes reducidos para limitar trabajo por decisión.
    constexpr int HORIZON = 90;
    constexpr int DECISION_INTERVAL = 3;

    struct Plan {
        int t1;
        int dur;
    };

    constexpr int TIMES[] = {0, 7, 16};
    constexpr int DURS[] = {12, 0};

    struct Simulation {
        PlayerObject* sim = nullptr;
        bool simulating = false;
        bool simDead = false;
        bool pressed = false;
        bool cachedWant = false;
        float dt = 1.f / 60.f;
        int lastScore = 0;
        int frameCounter = 0;

        void init() {
            auto pl = PlayLayer::get();
            if (!pl) return;
            sim = PlayerObject::create(1, 1, pl, pl, true);
            if (!sim) return;
            sim->setPosition({0, 105});
            sim->setVisible(false);
            sim->setID("ia-sim-player"_spr);
            pl->m_objectLayer->addChild(sim);
            cachedWant = pressed;
        }

        void quit() {
            sim = nullptr;
            simulating = false;
            frameCounter = 0;
        }

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

        int runPlan(PlayerObject* base, Plan plan) {
            auto pl = PlayLayer::get();
            if (!pl || !sim) return 0;

            simDead = false;
            sim->setVisible(false);
            sim->copyAttributes(base);
            sim->m_gravityMod = base->m_gravityMod;
            sim->m_isOnGround = base->m_isOnGround;

            bool simPressed = pressed;
            if (simPressed) sim->pushButton(PlayerButton::Jump);

            int result = HORIZON;
            for (int i = 0; i < HORIZON; ++i) {
                resetCollisionLog();
                pl->checkCollisions(sim, dt, false);
                if (simDead) {
                    result = i;
                    break;
                }

                bool inSwitch = i >= plan.t1 &&
                    (plan.dur == 0 || i < plan.t1 + plan.dur);
                bool want = inSwitch ? !pressed : pressed;
                if (want != simPressed) {
                    simPressed = want;
                    if (want) sim->pushButton(PlayerButton::Jump);
                    else sim->releaseButton(PlayerButton::Jump);
                }
                sim->update(dt);
            }
            return result;
        }

        bool decide(PlayerObject* base) {
            if (!sim || !base) return pressed;
            simulating = true;

            int best = -1;
            Plan bestPlan{HORIZON + 1, 0};
            bool done = false;
            auto probe = [&](Plan p) {
                int score = runPlan(base, p);
                if (score > best) {
                    best = score;
                    bestPlan = p;
                }
                // No need to test more plans if this one survives the horizon.
                if (score >= HORIZON) done = true;
            };

            probe(Plan{HORIZON + 1, 0});
            for (int t : TIMES) {
                for (int d : DURS) {
                    if (done) break;
                    probe(Plan{t, d});
                }
                if (done) break;
            }

            simulating = false;
            lastScore = best;
            bool changeNow = bestPlan.t1 == 0;
            cachedWant = changeNow ? !pressed : pressed;
            return cachedWant;
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

        auto label = CCLabelBMFont::create("Piloto IA: cargado", "bigFont.fnt");
        label->setScale(0.4f);
        label->setID("ia-status"_spr);
        auto win = CCDirector::get()->getWinSize();
        label->setPosition({win.width / 2.f, win.height - 15.f});
        m_uiLayer->addChild(label, 100);
        return true;
    }

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
        if (pl && !ia::S().simulating && pl->m_gameState.m_timeWarp != 0.f) {
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

    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* vec, int objectsCount, float dt) {
        if (ia::S().simulating) {
            gd::vector<GameObject*> relevant;
            for (int i = 0; i < objectsCount; ++i) {
                GameObject* obj = vec->at(i);
                auto type = obj->m_objectType;
                if (type == GameObjectType::Solid ||
                    type == GameObjectType::Hazard ||
                    type == GameObjectType::AnimatedHazard ||
                    type == GameObjectType::Slope) {
                    relevant.push_back(obj);
                }
            }
            GJBaseGameLayer::collisionCheckObjects(player, &relevant, relevant.size(), dt);
            return;
        }
        GJBaseGameLayer::collisionCheckObjects(player, vec, objectsCount, dt);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (ia::S().simulating) return;
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }

    void updateCamera(float dt) {
        auto pl = PlayLayer::get();
        auto& s = ia::S();
        if (pl && s.sim && !s.simulating) {
            std::string estado = "Piloto IA: pon modo practica";
            if (pl->m_isPracticeMode && !pl->m_player1->m_isDead) {
                // Ejecuta la búsqueda cada 3 frames y reutiliza la última acción
                // en los frames intermedios para evitar picos de CPU.
                if (++s.frameCounter >= ia::DECISION_INTERVAL) {
                    s.frameCounter = 0;
                    s.decide(pl->m_player1);
                }
                estado = fmt::format("Piloto IA | vive {}/{} | {}",
                                     s.lastScore, ia::HORIZON,
                                     s.cachedWant ? "TOCA" : "suelta");
                if (s.cachedWant != s.pressed) {
                    this->handleButton(s.cachedWant, 1, true);
                }
            } else {
                s.frameCounter = 0;
                s.cachedWant = s.pressed;
            }
            if (auto l = typeinfo_cast<CCLabelBMFont*>(
                    pl->m_uiLayer->getChildByID("ia-status"_spr))) {
                l->setString(estado.c_str());
            }
        }
        GJBaseGameLayer::updateCamera(dt);
    }
};
