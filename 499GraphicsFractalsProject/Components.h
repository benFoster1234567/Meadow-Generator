#pragma once
#include <cstdint>

using ComponentId = size_t;

enum class ComponentType : uint8_t
{
	Empty,
	Rigidbody
};

struct Component
{
	ComponentType type;
	ComponentId id;
};

class ComponentManager
{

};