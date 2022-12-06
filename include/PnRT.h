#pragma once
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif // !STB_IMAGE_IMPLEMENTATION
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include <vector>
#include <algorithm>

constexpr float FLOAT_MIN = std::numeric_limits<float>::lowest();
constexpr float FLOAT_MAX = std::numeric_limits<float>::max();
constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;

// count of float
constexpr int VERTEX_SIZE = 15;
constexpr int MATERIAL_SIZE = 18; 
constexpr int TRIANGLE_SIZE = 6;
constexpr int BVHNODE_SIZE = 12;


struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 bitangent;
	glm::vec2 texcoord;
};

struct Ray {
	glm::vec3 origin;
	glm::vec3 dir;
	mutable float tMax;
};

struct Material {
	glm::vec3 emssive = glm::vec3(0.f);
	glm::vec3 baseColor = glm::vec3(0.f);
	float subsurface = 0.0;
	float metallic = 0.0;
	float specular = 0.0;
	float specularTint = 0.0;
	float roughness = 0.0;
	float anisotropic = 0.0;
	float sheen = 0.0;
	float sheenTint = 0.0;
	float clearcoat = 0.0;
	float clearcoatGloss = 0.0;
	float IOR = 1.0;
	float transmission = 0.0;
};

struct Interaction {
	glm::vec3 position;
	glm::vec3 normal;
	int materialId;
	float time;
};

struct Camera;
struct Bound;
struct Triangle;
struct Mesh;
class Model;
class Shader;