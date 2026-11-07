#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <vector>
#include <string>
#include <cstdlib>

using namespace geode::prelude;

class $modify(FrasesPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);

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
