#pragma once
#include "PnRT.hpp"

struct Bound {
	Bound() {
		pMin = glm::vec3(FLOAT_MAX, FLOAT_MAX, FLOAT_MAX);
		pMax = glm::vec3(FLOAT_MIN, FLOAT_MIN, FLOAT_MIN);
	}
	// ��Χ����
	void Union(const Bound& b) {
		pMin = glm::min(pMin, b.pMin);
		pMax = glm::max(pMax, b.pMax);
	}
	void Union(const glm::vec3& p) {
		pMin = glm::min(pMin, p);
		pMax = glm::max(pMax, p);
	}
	// ��Χ�жԽ���
	glm::vec3 Diagonal() {
		return pMax - pMin;
	}
	// ��Χ�б����
	float SurfaceArea() {
		glm::vec3 d = Diagonal();
		return (d.x * d.y + d.x * d.z + d.y * d.z) * 2.f;
	}
	glm::vec3 pMin, pMax;
};


inline bool BoundIntersect(const Bound& bound, const Ray& ray, float* hit0 = nullptr, float* hit1 = nullptr) {
	float t0 = 0.f, t1 = ray.tMax;
	for (int i = 0; i < 3; ++i) {
		float invDir = 1.f / ray.dir[i];
		float tNear = (bound.pMin[i] - ray.origin[i]) * invDir;
		float tFar = (bound.pMax[i] - ray.origin[i]) * invDir;
		if (tNear > tFar) std::swap(tNear, tFar);
		// ����������������Χ�е�ʱ��Ϊ���߽����Χ�е�ʱ��
		t0 = std::max(t0, tNear);
		// �������������뿪��Χ�е�ʱ��Ϊ�����뿪��Χ�е�ʱ��
		t1 = std::min(t1, tFar);
		if (t0 > t1) return false;
	}
	if (hit0) *hit0 = t0;
	if (hit1) *hit1 = t1;
	return true;
}