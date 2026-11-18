#include "trajectory.hpp"

bool BotTrajectory::iterate(GJBaseGameLayer* pl, PlayerObject* player, std::vector<TrajectoryPlayerData>& cached, const PredictionConfig& config, bool simulateClick, bool currentClickState) {
    if (!pl || !player) return false;

    if (cached.empty()) {
        cached.push_back({ player->getPosition(), player->m_yVelocity, false, player->m_isFlipped });
        return true;
    }

    TrajectoryPlayerData last = cached.back();
    float nextVy = last.yVelocity;
    float nextY = last.position.y;
    float nextX = last.position.x + 5.0f; // Velocidad X estándar
    bool nextFlip = last.isFlipped;
    bool isDead = false;

    // --- FÍSICA POR VEHÍCULO ---
    if (player->m_isShip) {
        // Nave: Si hace clic, sube; si no, cae.
        if (simulateClick) nextVy += config.m_shipAccel;
        else nextVy -= config.m_shipAccel;
        nextVy *= 0.95f; // Fricción del aire
        nextY += nextVy;
    } 
    else if (player->m_isBall) {
        // Bola: Al hacer clic, cambia la gravedad
        if (simulateClick) nextFlip = !nextFlip;
        float gravity = nextFlip ? config.m_gravity : -config.m_gravity;
        nextVy += gravity;
        nextY += nextVy;
        if (nextY <= 0) { nextY = 0; nextVy = 0; } // Falso suelo
    }
    else if (player->m_isWave) {
        // Wave: Se mueve en diagonal
        if (simulateClick) { nextY += config.m_waveSpeed; nextVy = config.m_waveSpeed; }
        else { nextY -= config.m_waveSpeed; nextVy = -config.m_waveSpeed; }
    }
    else if (player->m_isUFO) {
        // UFO: Salta si hace clic y está en el aire
        if (simulateClick && last.yVelocity == 0) nextVy = config.m_jumpForce * 0.8f;
        nextVy += config.m_gravity;
        nextY += nextVy;
    }
    else {
        // Cubo y Robot
        if (simulateClick && last.yVelocity == 0) nextVy = config.m_jumpForce;
        nextVy += config.m_gravity;
        nextY += nextVy;
        if (nextY < 0) { nextY = 0; nextVy = 0; } // Falso suelo
    }

    cached.push_back({ cocos2d::CCPoint(nextX, nextY), nextVy, isDead, nextFlip });
    return !isDead;
}

void BotTrajectory::drawTrajectory(cocos2d::CCDrawNode* drawNode, const std::vector<TrajectoryPlayerData>& cached) {
    if (!drawNode || cached.empty()) return;
    drawNode->clear();
    for (size_t i = 0; i < cached.size() - 1; ++i) {
        cocos2d::ccColor4F color = {0.0f, 1.0f, 0.0f, 0.7f}; // Verde
        drawNode->drawSegment(cached[i].position, cached[i+1].position, 2.0f, color);
    }
}
