#pragma once
#include <Geode/Geode.hpp>
#include <vector>

using namespace geode::prelude;

struct TrajectoryPlayerData {
    cocos2d::CCPoint position;
    cocos2d::CCRect hitbox;
    float rotation;
    bool isDead;
};

struct PredictionConfig {
    int m_maxLength = 500; // Cuántos frames simular
    double m_overridenTPS = 240.0; // Ticks por segundo
};

class BotTrajectory {
public:
    static bool iterate(GJBaseGameLayer* pl, PlayerObject* player, std::vector<TrajectoryPlayerData>& cached, const PredictionConfig& config);
    static void drawTrajectory(cocos2d::CCDrawNode* drawNode, const std::vector<TrajectoryPlayerData>& cached);
};
