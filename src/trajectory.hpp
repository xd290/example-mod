#pragma once
#include <Geode/Geode.hpp>
#include <vector>

using namespace geode::prelude;

// Estructura para guardar cada punto de la simulación
struct TrajectoryPlayerData {
    cocos2d::CCPoint position;
    cocos2d::CCRect hitbox;
    float rotation;
    bool isDead;
};

// Configuración de la simulación
struct PredictionConfig {
    int m_maxLength = 1000; // Cuántos frames simular hacia el futuro
    double m_overridenTPS = 240.0; // Ticks por segundo para la simulación
};

class BotTrajectory {
public:
    // Función que simula un paso de la física del juego
    static bool iterate(GJBaseGameLayer* pl, PlayerObject* player, std::vector<TrajectoryPlayerData>& cached, const PredictionConfig& config);
    
    // Función que dibuja la línea en la pantalla
    static void drawTrajectory(cocos2d::CCDrawNode* drawNode, const std::vector<TrajectoryPlayerData>& cached);
};
