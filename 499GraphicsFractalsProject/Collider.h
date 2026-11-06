#pragma once
#include <variant>
#include <glm/ext/vector_float3.hpp>

struct SphereCollider { float radius{ 0.5f }; };
struct BoxCollider { glm::vec3 halfExtents{ 0.5f }; };

using Collider = std::variant<SphereCollider, BoxCollider>;