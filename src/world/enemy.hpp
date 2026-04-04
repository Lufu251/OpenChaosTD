#pragma once

#include <raylib.h>
#include <string>

class Enemy{
public:
    std::string m_name;
    Vector2 m_position;
    float m_maxhealth;
    float m_health;
    float m_speed;
};