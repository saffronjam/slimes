#pragma once

#include <saffron.h>

namespace slimes
{
using namespace saffron;
struct alignas(16) Agent
{
	sf::Vector2f Position;
	float Angle;
};
}
