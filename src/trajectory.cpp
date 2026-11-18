#include "src/trajectory.hpp"

bool BotTrajectory::iterate(GJBaseGameLayer* pl, PlayerObject* player, std::vector<TrajectoryPlayerData>& cached, const PredictionConfig& config) {
    if (!pl || !player) return false;

    // 1. Avanzar el tiempo del juego y del jugador un "tick" simulado
    float physicsDt = 1.0f / config.m_overridenTPS;
    pl->m_gameState.m_totalTime += physicsDt;
    player->m_totalTime += physicsDt;

    // 2. Guardamos la posición en la lista para dibujar la línea
    // (En un bot real, aquí calcularías la gravedad y las colisiones)
    cached.push_back({
        player->getPosition(),
        player->getBoundingBox(),
        player->getRotation(),
        false // isDead (falso porque aún no choca)
    });

    return true;
}

void BotTrajectory::drawTrajectory(cocos2d::CCDrawNode* drawNode, const std::vector<TrajectoryPlayerData>& cached) {
    if (!drawNode || cached.empty()) return;
    
    // Limpiamos la línea anterior
    drawNode->clear();

    // Dibujar una línea roja conectando los puntos simulados
    for (size_t i = 0; i < cached.size() - 1; ++i) {
        drawNode->drawSegment(cached[i].position, cached[i+1].position, 1.5f, cocos2d::ccColor4F{1.0f, 0.0f, 0.0f, 0.8f});
    }
}
