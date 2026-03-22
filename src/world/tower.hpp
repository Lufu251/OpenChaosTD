#pragma once

#include <raylib.h>
#include <string>

class Tower{
public:
    std::string m_name;
    Vector2 m_position;
    float m_damage;
    float m_radius;
    float m_fireRate;

};