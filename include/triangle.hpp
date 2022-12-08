#pragma once
#include "PnRT.h"
#include "bound.hpp"

struct Triangle {
	int indices[3] = { -1, -1, -1 };
	int materialId = -1;
	Bound bound;
	glm::vec3 boundCenter = glm::vec3(0);
};

inline bool TriangleIntersect(const Triangle& tri, const Ray& ray,
	const std::vector<Vertex>& vertices, Interaction* isect) {
	glm::vec3 p0 = vertices[tri.indices[0]].position;
	glm::vec3 p1 = vertices[tri.indices[1]].position;
	glm::vec3 p2 = vertices[tri.indices[2]].position;

	// ������������������
	glm::vec3 e01 = p0 - p1;
	glm::vec3 e02 = p0 - p2;
	glm::vec3 normal = normalize(cross(e01, e02));

	// �����������α���
	if (glm::dot(normal, ray.dir) > 0) {
		normal = -normal;
	}

	// ����߻�������������ƽ��ĵ�
	float nDotd = glm::dot(normal, ray.dir);
	if (glm::abs(nDotd) < 0.000001f) return false;
	float time = glm::dot(normal, p0 - ray.origin) / nDotd;
	if (time >= ray.tMax || time < 0) return false; // ����ʱ����ڹ��ߵ�tMax��˵�����߸����������һ��������
	glm::vec3 position = ray.origin + time * ray.dir;

	// �жϸõ��Ƿ����������ڲ�
	glm::vec3 pv0 = p0 - position;
	glm::vec3 pv1 = p1 - position;
	glm::vec3 pv2 = p2 - position;
	glm::vec3 c01 = cross(pv0, pv1);
	glm::vec3 c12 = cross(pv1, pv2);
	glm::vec3 c20 = cross(pv2, pv0);
	if (glm::dot(c01, c12) <= 0 || glm::dot(c12, c20) <= 0) return false;
	ray.tMax = time; // ���е�ǰ�����Σ����¹���tMax

	isect->position = position;
	isect->normal = normal;
	isect->materialId = tri.materialId;
	isect->time = time;
	return true;
}

inline bool TriangleIntersect2(const Triangle& tri, const Ray& ray,
	const std::vector<Vertex>& vertices, Interaction* isect) {
	glm::vec3 p0 = vertices[tri.indices[0]].position;
	glm::vec3 p1 = vertices[tri.indices[1]].position;
	glm::vec3 p2 = vertices[tri.indices[2]].position;

	// �任����ϵ������ԭ����(0, 0, 0), ����ָ��+z��
	glm::vec3 P0 = p0 - ray.origin;
	glm::vec3 P1 = p1 - ray.origin;
	glm::vec3 P2 = p2 - ray.origin;
	glm::vec3 rd = ray.dir;
	// ����z����Ϊ0��ѡȡ���������������н���
	if (rd.z == 0) {
		if (std::abs(rd.x) > std::abs(rd.y)) {
			std::swap(P0.x, P0.z);
			std::swap(P1.x, P1.z);
			std::swap(P2.x, P2.z);
			std::swap(rd.x, rd.z);
		}
		else {
			std::swap(P0.y, P0.z);
			std::swap(P1.y, P1.z);
			std::swap(P2.y, P2.z);
			std::swap(rd.y, rd.z);
		}
	}
	// ���б任�����߷�����+z�����
	float invDz = 1.f / rd.z;
	P0.x = P0.x - P0.z * rd.x * invDz;
	P0.y = P0.y - P0.z * rd.y * invDz;
	P0.z *= invDz;
	P1.x = P1.x - P1.z * rd.x * invDz;
	P1.y = P1.y - P1.z * rd.y * invDz;
	P1.z *= invDz;
	P2.x = P2.x - P2.z * rd.x * invDz;
	P2.y = P2.y - P2.z * rd.y * invDz;
	P2.z *= invDz;

	float e0 = P1.x * P2.y - P1.y * P2.x;
	float e1 = P2.x * P0.y - P2.y * P0.x;
	float e2 = P0.x * P1.y - P0.y * P1.x;

	double p2txp1ty = (double)P2.x * (double)P2.y;
	double p2typ1tx = (double)P2.y * (double)P1.x;
	e0 = (float)(p2typ1tx - p2txp1ty);
	double p0txp2ty = (double)P0.x * (double)P2.y;
	double p0typ2tx = (double)P0.y * (double)P2.x;
	e1 = (float)(p0typ2tx - p0txp2ty);
	double p1txp0ty = (double)P1.x * (double)P0.y;
	double p1typ0tx = (double)P1.y * (double)P0.x;
	e2 = (float)(p1typ0tx - p1txp0ty);

	// ����������������
	if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0)) return false;

	// ���������㹲��
	float det = e0 + e1 + e2;
	if (det == 0) return false;

	float tScaled = e0 * P0.z + e1 * P1.z + e2 * P2.z;
	if (det > 0 && (tScaled <= 0 || tScaled > ray.tMax * det)) return false;
	if (det < 0 && (tScaled >= 0 || tScaled < ray.tMax * det)) return false;

	float invDet = 1.f / det;
	float t = tScaled * invDet;
	float b0 = e0 * invDet;
	float b1 = e1 * invDet;
	float b2 = e2 * invDet;
	
	glm::vec3 pHit = b0 * p0 + b1 * p1 + b2 * p2;
	*isect = { pHit, glm::vec3(0), tri.materialId, t};
	return true;
}