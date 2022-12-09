#pragma once
#include "PnRT.hpp"

struct Camera {
	Camera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, 
		float fov, int width, int height)
		: eye(eye), center(center), up(up), fov(fov), aspect((float)width / height),
		  width(width), height(height), nChannels(3) {
		data = new unsigned char[width * height * nChannels];
		float theta = glm::radians(fov);
		// 设定屏幕在z轴的-1处，平行于xy平面
		float halfHeight = glm::tan(theta * 0.5);
		float halfWidth = aspect * halfHeight;

		w = glm::normalize(eye - center);
		u = glm::normalize(glm::cross(up, w));
		v = glm::cross(w, u);

		lowerLeftCorner = eye - halfWidth * u - halfHeight * v - w;
		horizontal = 2 * halfWidth * u;
		vertical = 2 * halfHeight * v;
	}


	void Output(const std::string& path) const {
		stbi_write_png(path.c_str(), width, height, nChannels, data, 0);
	}

	// 均处于世界坐标
	glm::vec3 eye;
	glm::vec3 center;
	glm::vec3 up;
	glm::vec3 lowerLeftCorner;
	glm::vec3 u, v, w;
	glm::vec3 horizontal, vertical;

	float fov; // indegree 
	float aspect; // width : height

	int width;
	int height;
	int nChannels;
	unsigned char* data;
};

inline Ray CameraGetRay(const Camera& camera, float s, float t) {
	return Ray{ camera.eye, camera.lowerLeftCorner + s * camera.horizontal + 
		t * camera.vertical - camera.eye, FLOAT_MAX };
}