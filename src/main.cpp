// Piloto IA para Geometry Dash (v3).
// Cada frame prueba varios "planes" (cuándo apretar o soltar el botón y por
// cuánto tiempo) en una simulación y elige uno que sobreviva.
// La técnica de simulación (jugador falso + checkCollisions + update) está
// basada en el Show Trajectory de Eclipse Menu (EPL-2.0).
// Solo actúa en MODO PRÁCTICA.

#include <Geode/Geode.hpp>
#include <Geode/modify/EffectGameObject.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <vector>

using namespace geode::prelude;

// En esta versión del bindings de Geode, RingObject no expone los campos
// m_hasBeenActivated / m_hasBeenActivatedP2, así que la simulación de orbes
// queda desactivada para que el proyecto compile.
#define IA_ORBES 0
