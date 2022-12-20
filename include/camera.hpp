#pragma once
#include "PnRT.hpp"

struct Camera {
	Camera() {}
	Camera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, 
		float fov, float aspect) {
		UpdateCamera(eye, center, up, fov, aspect);
	}

	void UpdateCamera(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, 
		float fov, float aspect) {
		this->eye = eye;
		this->center = center;
		this->up = up;
		this->fov = fov;
		this->aspect = aspect;
		float theta = glm::radians(fov);
		// 设定屏幕在z轴的-1处，平行于xy平面
		float halfHeight = glm::tan(theta * 0.5);
		float halfWidth = aspect * halfHeight;

		distance = glm::distance(eye, center);
		w = glm::normalize(eye - center);
		u = glm::normalize(glm::cross(up, w));
		v = glm::cross(w, u);

		lowerLeftCorner = eye - halfWidth * u - halfHeight * v - w;
		horizontal = 2 * halfWidth * u;
		vertical = 2 * halfHeight * v;
	}

	void UpdateRotate(float phi, float theta) {
		phi *= 0.6;
		theta *= 0.6;
		phi = glm::radians(phi);
		theta = glm::radians(theta);
		glm::vec3 nv = glm::vec3(std::cos(phi) * std::cos(theta), std::sin(phi) * std::cos(theta), std::sin(theta));
		nv = w * nv.x + u * nv.y + v * nv.z;
		if (std::abs(glm::dot(up, nv)) > 0.9995f) return;
		eye = center + nv * distance;
		UpdateCamera(eye, center, up, fov, aspect);
	}

	void UpdateTranslate(float delta) {
		delta *= 0.05;
		glm::vec3 nEye = eye + delta * w;
		UpdateCamera(nEye, center, up, fov, aspect);
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
	float distance; // eye-center
};

inline Ray CameraGetRay(const Camera& camera, float s, float t) {
	return Ray{ camera.eye, camera.lowerLeftCorner + s * camera.horizontal + 
		t * camera.vertical - camera.eye, FLOAT_MAX };
}