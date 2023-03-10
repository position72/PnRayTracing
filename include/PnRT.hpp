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
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
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
#include <map>
#include <set>
#include <algorithm>
#include <random>

#pragma comment(lib, "./lib/glfw3.lib")
#pragma comment(lib, "./lib/assimp-vc143-mt.lib")
#pragma comment(lib, "Opengl32.lib")

constexpr float FLOAT_MIN = std::numeric_limits<float>::lowest();
constexpr float FLOAT_MAX = std::numeric_limits<float>::max();
constexpr float PI = 3.14159265358979323846;
constexpr float InvPI = 0.31830988618379067154;
constexpr float ShadowEpsilon = 0.0001f;
constexpr int SCREEN_WIDTH = 512;
constexpr int SCREEN_HEIGHT = 512;

// count of float
constexpr int VERTEX_SIZE = 15;
constexpr int MATERIAL_SIZE = 18; 
constexpr int TRIANGLE_SIZE = 6;
constexpr int BVHNODE_SIZE = 12;
constexpr int LIGHT_SIZE = 3;


struct Vertex {
	glm::vec3 position = glm::vec3(0.f);
	glm::vec3 normal = glm::vec3(0.f);
	glm::vec3 tangent = glm::vec3(0.f);
	glm::vec3 bitangent = glm::vec3(0.f);
	glm::vec2 texcoord = glm::vec3(0.f);
};

struct Ray {
	glm::vec3 origin = glm::vec3(0.f);
	glm::vec3 dir = glm::vec3(0.f);
	mutable float tMax = FLOAT_MAX;
};

struct Material {
	glm::vec3 emssive = glm::vec3(0.f);
	glm::vec3 baseColor = glm::vec3(0.8f);
	float subsurface = 0.0;
	float metallic = 0.0;
	float specular = 0.0;
	float specularTint = 0.0;
	float roughness = 0.5;
	float anisotropic = 0.0;
	float sheen = 0.0;
	float sheenTint = 0.0;
	float clearcoat = 0.0;
	float clearcoatGloss = 0.0;
	float IOR = 1.0;
	float transmission = 0.0;
};

struct Interaction {
	glm::vec3 position = glm::vec3(0.f);
	glm::vec3 normal = glm::vec3(0.f);
	glm::vec2 texcoord = glm::vec3(0.f);
	int textureId = 0;
	int materialId = 0;
	float time = FLOAT_MAX;
};

struct TextureInfo {
	int width;
	int height;
	int nChannels;
	unsigned int tbo;
};

struct Camera;
struct Bound;
struct Triangle;
struct Light;
struct Mesh;
class Model;
class Shader;
class ImGuiLayer;


// resource
std::vector<Vertex> vertices;
std::vector<Triangle> triangles;
std::vector<Material> materials;
std::vector<unsigned char*> textures; 
// ????????????textures??????????????????????
std::map<std::string, int> texturePathToId;
std::vector<TextureInfo> textureInfos;
std::vector<Light> lights;
std::vector<Model> models;


inline glm::vec3 TextureGetColor255(int i, float u, float v) {
	const TextureInfo& info = textureInfos[i];
	int x = static_cast<int>(info.width * u);
	int y = static_cast<int>(info.height * v);
	glm::vec3 color;
	int pos = info.nChannels * (y * info.width + x);
	color[0] = textures[i][pos + 0];
	color[1] = textures[i][pos + 1];
	color[2] = textures[i][pos + 2];
	return color;
}

inline glm::vec3 TextureGetColor1(int i, float u, float v) {
	const TextureInfo& info = textureInfos[i];
	int x = static_cast<int>(info.width * u);
	int y = static_cast<int>(info.height * v);
	glm::vec3 color;
	int pos = info.nChannels * (y * info.width + x);
	color[0] = textures[i][pos + 0];
	color[1] = textures[i][pos + 1];
	color[2] = textures[i][pos + 2];
	return color / 255.f;
}

inline float Rand0To1() {
	static std::default_random_engine e;
	static std::uniform_real_distribution<float> real(0, 1);
	return real(e);
}

inline float Clamp(float x, float L, float R) {
	if (x >= R) return R;
	if (x <= L) return L;
	return x;
}

