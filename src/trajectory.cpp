#include "trajectory.hpp"

bool BotTrajectory::iterate(GJBaseGameLayer* pl, PlayerObject* player, std::vector<TrajectoryPlayerData>& cached, const PredictionConfig& config) {
    if (!pl || !player) return false;

    // Si la lista está vacía, empezamos en la posición actual del jugador
    if (cached.empty()) {
        cached.push_back({
            player->getPosition(),
            player->getObjectRect(),
            player->getRotation(),
            false 
        });
    } else {
        // Si ya hay puntos, creamos un movimiento falso hacia la derecha
        // Solo para que veas la línea en pantalla
        cocos2d::CCPoint lastPos = cached.back().position;
        
        float xOffset = 10.0f; // Avanzar a la derecha
        float yOffset = 0.0f;

        // Hacemos un arco falso simulando un salto
        int frame = cached.size();
        if (frame > 10 && frame < 30) {
            yOffset = 8.0f; // Sube
        } else if (frame >= 30 && frame < 50) {
            yOffset = -8.0f; // Baja
        }

        cocos2d::CCPoint newPos = cocos2d::CCPoint(lastPos.x + xOffset, lastPos.y + yOffset);
        
        cached.push_back({
            newPos,
            player->getObjectRect(),
            player->getRotation(),
            false 
        });
    }

    return true;
}

void BotTrajectory::drawTrajectory(cocos2d::CCDrawNode* drawNode, const std::vector<TrajectoryPlayerData>& cached) {
    if (!drawNode || cached.empty()) return;
    
    drawNode->clear();

    // Dibujar una línea roja
    for (size_t i = 0; i < cached.size() - 1; ++i) {
        drawNode->drawSegment(cached[i].position, cached[i+1].position, 2.0f, cocos2d::ccColor4F{1.0f, 0.0f, 0.0f, 1.0f});
    }
}
