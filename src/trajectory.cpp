#include "trajectory.hpp"

bool BotTrajectory::iterate(GJBaseGameLayer* pl, PlayerObject* player, std::vector<TrajectoryPlayerData>& cached, const PredictionConfig& config) {
    if (!pl || !player) return false;

    // Guardamos la posición actual (getObjectRect es el hitbox correcto)
    cached.push_back({
        player->getPosition(),
        player->getObjectRect(), 
        player->getRotation(),
        false 
    });

    return true;
}

void BotTrajectory::drawTrajectory(cocos2d::CCDrawNode* drawNode, const std::vector<TrajectoryPlayerData>& cached) {
    if (!drawNode || cached.empty()) return;
    
    drawNode->clear();

    // Dibujar una línea roja uniendo los puntos
    for (size_t i = 0; i < cached.size() - 1; ++i) {
        drawNode->drawSegment(cached[i].position, cached[i+1].position, 1.5f, cocos2d::ccColor4F{1.0f, 0.0f, 0.0f, 0.8f});
    }
}
