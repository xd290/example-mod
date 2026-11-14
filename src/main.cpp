// Piloto IA para Geometry Dash (v5).
// Cada frame comprueba, en una simulación, si seguir igual o seguir el plan
// anterior sobrevive; solo busca planes nuevos cuando eso falla.
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

// Comprueba al compilar si las orbes tienen los campos que marcan "ya usada".
// Si no existen con estos nombres, las orbes se apagan solas (no falla).
template <class T>
constexpr bool kHasActivatedFlags = requires(T* t) {
    t->m_hasBeenActivated;
    t->m_hasBeenActivatedP2;
};

template <class T>
struct RingSaveT {
    T* ring;
    bool a;
    bool b;
};

template <class T>
void saveFlags(T* r, std::vector<RingSaveT<T>>& saves) {
    if constexpr (kHasActivatedFlags<T>) {
        for (auto& s : saves) {
            if (s.ring == r) return;
        }
        saves.push_back({r, static_cast<bool>(r->m_hasBeenActivated),
                         static_cast<bool>(r->m_hasBeenActivatedP2)});
    }
}

template <class T>
void restoreFlags(std::vector<RingSaveT<T>>& saves) {
    if constexpr (kHasActivatedFlags<T>) {
        for (auto& s : saves) {
            s.ring->m_hasBeenActivated = s.a;
            s.ring->m_hasBeenActivatedP2 = s.b;
        }
    }
    saves.clear();
}

namespace ia {
    // true si las orbes se pueden simular en esta versión de Geode.
    constexpr bool ORBS_ON = kHasActivatedFlags<RingObject>;

    // Cuántos pasos hacia adelante mira. Un salto de cubo dura unos 90.
    // Si hay lag, bájalo a 90. Si el cubo falla, súbelo a 130.
    constexpr int HORIZON = 110;
    constexpr int NEVER = 100000;

    // Un plan: el botón empieza en c0, cambia en el paso t1 y se queda
    // cambiado por dur pasos (dur = 0 significa para siempre).
    struct Plan {
        bool c0;
        int t1;
        int dur;
    };

    inline bool stateAt(const Plan& p, int k) {
        bool changed = k >= p.t1 && (p.dur == 0 || k < p.t1 + p.dur);
        return changed ? !p.c0 : p.c0;
    }

    // Planes nuevos que prueba cuando el anterior ya no sirve.
    constexpr int TIMES[] = {0, 6, 14, 26};
    constexpr int DURS[] = {10, 26, 0};

    struct Simulation {
        PlayerObject* sim = nullptr;
        bool simulating = false;
        bool simDead = false;
        bool pressed = false;   // estado real del botón del jugador 1
        float dt = 1.f / 60.f;
        int lastScore = 0;

        Plan current{false, NEVER, 0};
        int planAge = 0;        // pasos físicos reales desde que nació el plan
        bool hasPlan = false;
        int labelTick = 0;

        std::vector<RingSaveT<RingObject>> ringSaves;

        void saveRing(RingObject* r) { saveFlags<RingObject>(r, ringSaves); }
        void restoreRings() { restoreFlags<RingObject>(ringSaves); }

        void init() {
            auto pl = PlayLayer::get();
            sim = PlayerObject::create(1, 1, pl, pl, true);
            sim->setPosition({0, 105});
            sim->setVisible(false);
            sim->setID("ia-sim-player"_spr);
            pl->m_objectLayer->addChild(sim);
            hasPlan = false;
            planAge = 0;
            ringSaves.clear();
        }

        void quit() {
            sim = nullptr;
            hasPlan = false;
            ringSaves.clear();
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

        // Simula un plan desde el estado real, empezando en el paso "offset"
        // del plan. Devuelve cuántos pasos sobrevive (máximo HORIZON).
        int runPlan(PlayerObject* base, const Plan& plan, int offset) {
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

                bool want = stateAt(plan, offset + i);
                if (want != simPressed) {
                    simPressed = want;
                    want ? sim->pushButton(PlayerButton::Jump) : sim->releaseButton(PlayerButton::Jump);
                }
                sim->update(dt);
            }

            restoreRings();   // las orbes quedan como estaban en el juego real
            return result;
        }

        // Devuelve true si el botón debe quedar apretado en este frame.
        bool decide(PlayerObject* base) {
            simulating = true;

            Plan chosen{pressed, NEVER, 0};
            int chosenAge = 0;
            int best = -1;
            bool done = false;

            // 1) No cambiar nada (lo más barato y lo más común).
            {
                int sc = runPlan(base, chosen, 0);
                best = sc;
                if (sc >= HORIZON) done = true;
            }

            // 2) Seguir con el plan anterior.
            if (!done && hasPlan) {
                int sc = runPlan(base, current, planAge);
                if (sc > best) {
                    best = sc;
                    chosen = current;
                    chosenAge = planAge;
                }
                if (sc >= HORIZON) done = true;
            }

            // 3) Buscar un plan nuevo.
            if (!done) {
                for (int t : TIMES) {
                    for (int d : DURS) {
                        Plan p{pressed, t, d};
                        int sc = runPlan(base, p, 0);
                        if (sc > best) {
                            best = sc;
                            chosen = p;
                            chosenAge = 0;
                        }
                        if (sc >= HORIZON) {
                            done = true;
                            break;
                        }
                    }
                    if (done) break;
                }
            }

            simulating = false;
            lastScore = best;
            current = chosen;
            planAge = chosenAge;
            hasPlan = true;
            return stateAt(chosen, chosenAge);
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

    // Si muere el jugador falso, no destruye el real.
    void destroyPlayer(PlayerObject* player, GameObject* gameObject) override {
        if (gameObject != m_anticheatSpike && ia::S().isSimDead(player)) return;
        PlayLayer::destroyPlayer(player, gameObject);
    }

    void playEndAnimationToPos(cocos2d::CCPoint p0) {
        if (ia::S().simulating) return;
        PlayLayer::playEndAnimationToPos(p0);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        ia::S().hasPlan = false;
        ia::S().planAge = 0;
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
        auto& s = ia::S();
        if (s.simulating) return;
        auto pl = PlayLayer::get();
        if (!pl) return;
        if (pl->m_gameState.m_timeWarp != 0.f) {
            s.dt = dt / pl->m_gameState.m_timeWarp;
        }
        // Cuenta los pasos físicos del jugador real para seguir el plan.
        if (this == pl->m_player1) s.planAge++;
    }

    void ringJump(RingObject* p0, bool p1) {
        auto& s = ia::S();
        if (s.simulating) {
            // El jugador falso sí puede usar orbes; el real no se entera.
            if (ia::ORBS_ON && this == s.sim) {
                s.saveRing(p0);
                PlayerObject::ringJump(p0, p1);
            }
            return;
        }
        PlayerObject::ringJump(p0, p1);
    }
};

class $modify(IAGameLayer, GJBaseGameLayer) {
    bool canBeActivatedByPlayer(PlayerObject* p0, EffectGameObject* p1) {
        auto& s = ia::S();
        if (s.simulating) {
            if (ia::ORBS_ON && p0 == s.sim && typeinfo_cast<RingObject*>(p1)) {
                return GJBaseGameLayer::canBeActivatedByPlayer(p0, p1);
            }
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

    // Durante la simulación solo cuenta lo que puede matar, bloquear o las
    // orbes. Reutiliza la misma lista para no crear una nueva en cada paso.
    void collisionCheckObjects(PlayerObject* player, gd::vector<GameObject*>* vec, int objectsCount, float dt) {
        if (ia::S().simulating) {
            static gd::vector<GameObject*> relevant;
            relevant.clear();
            for (int i = 0; i < objectsCount; i++) {
                GameObject* obj = vec->at(i);
                bool keep = obj->m_objectType == GameObjectType::Solid ||
                            obj->m_objectType == GameObjectType::Hazard ||
                            obj->m_objectType == GameObjectType::AnimatedHazard ||
                            obj->m_objectType == GameObjectType::Slope;
                if (!keep && ia::ORBS_ON && typeinfo_cast<RingObject*>(obj)) keep = true;
                if (keep) relevant.push_back(obj);
            }
            GJBaseGameLayer::collisionCheckObjects(player, &relevant, relevant.size(), dt);
            return;
        }
        GJBaseGameLayer::collisionCheckObjects(player, vec, objectsCount, dt);
    }

    void playerTouchedRing(PlayerObject* player, RingObject* ring) {
        auto& s = ia::S();
        if (s.simulating) {
            if (ia::ORBS_ON && player == s.sim) {
                s.saveRing(ring);
                GJBaseGameLayer::playerTouchedRing(player, ring);
            }
            return;
        }
        GJBaseGameLayer::playerTouchedRing(player, ring);
    }

    void updateCamera(float dt) {
        auto pl = PlayLayer::get();
        auto& s = ia::S();
        if (pl && s.sim && !s.simulating) {
            if (pl->m_isPracticeMode && !pl->m_player1->m_isDead) {
                bool want = s.decide(pl->m_player1);
                if (want != s.pressed) {
                    this->handleButton(want, 1, true);
                }
            } else {
                s.hasPlan = false;
            }

            // El texto se actualiza pocas veces: cambiarlo cada frame da lag.
            if (++s.labelTick >= 30) {
                s.labelTick = 0;
                std::string estado;
                if (!pl->m_isPracticeMode) {
                    estado = "Piloto IA: pon modo practica";
                } else {
                    estado = fmt::format("Piloto IA | vive {}/{} | orbes {}",
                                         s.lastScore, ia::HORIZON, ia::ORBS_ON ? "SI" : "NO");
                }
                if (auto l = typeinfo_cast<CCLabelBMFont*>(pl->m_uiLayer->getChildByID("ia-status"_spr))) {
                    l->setString(estado.c_str());
                }
            }
        }
        GJBaseGameLayer::updateCamera(dt);
    }
};
