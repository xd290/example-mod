#pragma once
#include <Geode/Geode.hpp>
#include <vector>

using namespace geode::prelude;

struct TrajectoryPlayerData {
    cocos2d::CCPoint position;
    float yVelocity;
    bool isDead;
    bool isFlipped; 
};

struct PredictionConfig {
    int m_maxLength = 200; 
    float m_gravity = -0.958199f; 
    float m_jumpForce = 11.180032f; 
    float m_shipAccel = 0.8f;
    float m_waveSpeed = 7.0f;
};

class BotTrajectory {
public:
    // Ahora le pasamos si el bot va a hacer clic o no para simular
    static bool iterate(GJBaseGameLayer* pl, PlayerObject* player, std::vector<TrajectoryPlayerData>& cached, const PredictionConfig& config, bool simulateClick, bool currentClickState);
    static void drawTrajectory(cocos2d::CCDrawNode* drawNode, const std::vector<TrajectoryPlayerData>& cached);
};
