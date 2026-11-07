#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <string>

using namespace geode::prelude;

static float ventanaBuffer() {
    auto nivel = Mod::get()->getSettingValue<std::string>("level");
    if (nivel == "Bajo") return 0.10f;
    if (nivel == "Alto") return 0.50f;
    return 0.25f; // Medio
}

static bool modoPermitido(PlayerObject* p) {
    // Solo cubo y wave
    return p->m_isDart || !(p->m_isShip || p->m_isBird || p->m_isBall ||
                            p->m_isRobot || p->m_isSpider || p->m_isSwing);
}

class $modify(BufferLayer, GJBaseGameLayer) {
    struct Fields {
        float tiempo = 0.f;
        bool esP1 = true;
        bool sostenido = false;
        bool soltarDespues = false;
        bool interno = false;
    };

    void handleButton(bool down, int button, bool isPlayer1) {
        if (m_fields->interno || button != 1 ||
            !Mod::get()->getSettingValue<bool>("enabled")) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        if (down) {
            m_fields->sostenido = true;
            auto p = isPlayer1 ? m_player1 : m_player2;
            if (p && modoPermitido(p) && !p->m_isOnGround) {
                m_fields->tiempo = ventanaBuffer();
                m_fields->esP1 = isPlayer1;
            }
        } else {
            m_fields->sostenido = false;
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        if (m_fields->soltarDespues) {
            m_fields->soltarDespues = false;
            m_fields->interno = true;
            GJBaseGameLayer::handleButton(false, 1, m_fields->esP1);
            m_fields->interno = false;
        }

        if (m_fields->tiempo > 0.f) {
            m_fields->tiempo -= dt;
            auto p = m_fields->esP1 ? m_player1 : m_player2;
            if (p && !p->m_isDead && modoPermitido(p) && p->m_isOnGround) {
                m_fields->tiempo = 0.f;
                m_fields->interno = true;
                GJBaseGameLayer::handleButton(true, 1, m_fields->esP1);
                m_fields->interno = false;
                if (!m_fields->sostenido) m_fields->soltarDespues = true;
            }
        }
    }
};
