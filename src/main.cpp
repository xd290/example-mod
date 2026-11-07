#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <vector>
#include <string>
#include <cstdlib>

using namespace geode::prelude;

class $modify(FrasesPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        bool yaEstabaMuerto = m_player1->m_isDead;
        PlayLayer::destroyPlayer(player, object);

        // Solo muestra la frase si de verdad acabas de morir
        if (yaEstabaMuerto || !m_player1->m_isDead) return;

        static const std::vector<std::string> frases = {
            "Skill issue",
            "Mi abuela pasa eso dormida",
            "Eso fue culpa del lag",
            "Ya casi, ya casi...",
            "Pff, fácil",
            "El nivel está roto, seguro",
            "Intenta de nuevo, campeón"
        };

        auto i = rand() % frases.size();
        Notification::create(frases[i], NotificationIcon::None, 1.5f)->show();
    }
};
