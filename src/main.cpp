// Piloto IA para Geometry Dash (v3).
// Cada frame prueba varios "planes" (cuándo apretar o soltar el botón y por
// cuánto tiempo) en una simulación y elige uno que sobreviva.
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
#include <vector>

using namespace geode::prelude;

// 1 = la simulación también usa orbes. Si Actions marca error por culpa de
// las orbes, cámbialo a 0 y vuelve a compilar.
#define IA_ORBES 1

namespace ia {
    // Cuántos pasos hacia adelante simula cada plan. Si el juego va muy
    // lento en tu celular, bájalo a 120.
    constexpr int HORIZON = 150;

    // Un plan cambia el botón en el paso t1 y lo mantiene cambiado por dur
    // pasos (dur = 0 significa hasta el final).
    struct Plan {
        int t1;
        int dur;
    };

    constexpr int TIMES[] = {0, 4, 9, 16, 28};
    constexpr int DURS[] = {8, 24, 0};

    struct Simulation {
        PlayerObject* sim = nullptr;
        bool simulating = false;
        bool simDead = false;
        bool pressed = false;   // estado real del botón del jugador 1
        float dt = 1.f / 60.f;
        int lastScore = 0;      // cuántos pasos sobrevivió el mejor plan

#if IA_ORBES
        // Las orbes que toca la simulación se marcan como "usadas"; aquí
        // guardamos su estado real para devolverlo después de cada plan.
        struct RingSave {
            RingObject* ring;
            bool a;
            bool b;
        };
        std::vector<RingSave> ringSaves;

        void saveRing(RingObject* r) {
            for (auto& s : ringSaves) {
                if (s.ring == r) return;
            }
            ringSaves.push_back({r, r->m_hasBeenActivated, r->m_hasBeenActivatedP2});
        }

        void restoreRings() {
            for (auto& s : ringSaves) {
                s.ring->m_hasBeenActivated = s.a;
                s.ring->m_hasBeenActivatedP2 = s.b;
            }
            ringSaves.clear();
        }
#else
        void saveRing(RingObject*) {}
        void restoreRings() {}
#endif

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

        // Simula un plan. Devuelve cuántos pasos sobrevive (máximo HORIZON).
        int runPlan(PlayerObject* base, Plan plan) {
            auto pl = PlayLayer::get();
            simDead = false;
            sim->setVisible(false);
            sim->copyAttributes(base);
            sim->m_gravityMod = base->m_gravityMod;
            sim->m_isOnGround = base->m_isOnGround;

            bool simPressed = pressed;
            if (simPressed) sim->pushButton(PlayerButton::Jump);

            int result = HORIZON;
            for (int i = 0; i < HORIZON; i++) {
                resetCollisionLog();
                pl->checkCollisions(sim, dt, false);
                if (simDead) {
                    result = i;
                    break;
                }

                bool inSwitch = i >= plan.t1 && (plan.dur == 0 || i < plan.t1 + plan.dur);
                bool want = inSwitch ? !pressed : pressed;
                if (want != simPressed) {
                    simPressed = want;
                    want ? sim->pushButton(PlayerButton::Jump) : sim->releaseButton(PlayerButton::Jump);
                }
                sim->update(dt);
            }

            restoreRings();
            return result;
        }

        // Devuelve true si el botón debe quedar apretado en este frame.
        bool decide(PlayerObject* base) {
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
                if (score >= HORIZON) done = true;
            };

            probe(Plan{HORIZON + 1, 0});   // no cambiar nada
            for (int t : TIMES) {
                for (int d : DURS) {
                    if (done) break;
                    probe(Plan{t, d});
                }
                if (done) break;
            }

            simulating = false;
            lastScore = best;

            bool cambiaAhora = bestPlan.t1 == 0;
            return cambiaAhora ? !pressed : pressed;
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

        // Texto de estado arriba de la pantalla, para ver si el mod está vivo.
        auto label = CCLabelBMFont::create("Piloto IA: cargado", "bigFont.fnt");
        label->setScale(0.4f);
        label->setID("ia-status"_spr);
        auto win = CCDirector::get()->getWinSize();
        label->setPosition({win.width / 2.f, win.height - 15.f});
        m_uiLayer->addChild(label, 100);
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
        if (ia::S().simulating) {
#if IA_ORBES
            // El jugador falso sí puede usar orbes; el real no se entera.
            if (this == ia::S().sim) {
                ia::S().saveRing(p0);
                PlayerObject::ringJump(p0, p1);
            }
#endif
            return;
        }
        PlayerObject::ringJump(p0, p1);
    }
};

class $modify(IAGameLayer, GJBaseGameLayer) {
    bool canBeActivatedByPlayer(PlayerObject* p0, EffectGameObject* p1) {
        if (ia::S().simulating) {
#if IA_ORBES
            if (p0 == ia::S().sim && typeinfo_cast<RingObject*>(p1)) {
                return GJBaseGameLayer::canBeActivatedByPlayer(p0, p1);
            }
#endif
            return false;
        }
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

    // Durante la simulación solo cuenta lo que puede matar, bloquear o (con
    // IA_ORBES) las orbes.
    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* vec, int objectsCount, float dt) {
        if (ia::S().simulating) {
            gd::vector<GameObject*> extra;
            for (int i = 0; i < objectsCount; i++) {
                GameObject* obj = vec->at(i);
                bool keep = obj->m_objectType == GameObjectType::Solid ||
                            obj->m_objectType == GameObjectType::Hazard ||
                            obj->m_objectType == GameObjectType::AnimatedHazard ||
                            obj->m_objectType == GameObjectType::Slope;
#if IA_ORBES
                if (!keep && typeinfo_cast<RingObject*>(obj)) keep = true;
#endif
                if (keep) extra.push_back(obj);
            }
            GJBaseGameLayer::collisionCheckObjects(player, &extra, extra.size(), dt);
            return;
        }
        GJBaseGameLayer::collisionCheckObjects(player, vec, objectsCount, dt);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        if (ia::S().simulating) {
#if IA_ORBES
            if (player == ia::S().sim) {
                ia::S().saveRing(ring);
                GJBaseGameLayer::playerTouchedRing(player, ring);
            }
#endif
            return;
        }
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }

    // Cada frame: prueba los planes y aplica la primera acción del mejor.
    void updateCamera(float dt) {
        auto pl = PlayLayer::get();
        auto& s = ia::S();
        if (pl && s.sim && !s.simulating) {
            std::string estado = "Piloto IA: pon modo practica";
            if (pl->m_isPracticeMode && !pl->m_player1->m_isDead) {
                bool want = s.decide(pl->m_player1);
                estado = fmt::format("Piloto IA | vive {}/{} | {}",
                                     s.lastScore, ia::HORIZON, want ? "TOCA" : "suelta");
                if (want != s.pressed) {
                    this->handleButton(want, 1, true);
                }
            }
            if (auto l = typeinfo_cast<CCLabelBMFont*>(pl->m_uiLayer->getChildByID("ia-status"_spr))) {
                l->setString(estado.c_str());
            }
        }
        GJBaseGameLayer::updateCamera(dt);
    }
};
