#pragma once
#include "PnRT.hpp"
#include "bound.hpp"

struct Triangle {
	int indices[3] = { -1, -1, -1 };
	// material��texture�ڸ���vector�е���������glsl��������ָ�������
	int materialId = 0;
	int textureId = -1;
	float area = 0.f;
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
	isect->textureId = tri.textureId;
	isect->time = time;
	return true;
}

inline bool TriangleIntersect2(const Triangle& tri, const Ray& ray,
	const std::vector<Vertex>& vertices, Interaction* isect) {
	const Vertex& v0 = vertices[tri.indices[0]];
	const Vertex& v1 = vertices[tri.indices[1]];
	const Vertex& v2 = vertices[tri.indices[2]];

	glm::vec3 p0 = v0.position;
	glm::vec3 p1 = v1.position;
	glm::vec3 p2 = v2.position;

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

	// �ߺ������� p0 �� p1�Լ������ĵ�����pָ�������������������
	float e0 = P1.x * P2.y - P1.y * P2.x;
	float e1 = P2.x * P0.y - P2.y * P0.x;
	float e2 = P0.x * P1.y - P0.y * P1.x;

	// ����������������
	if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0)) return false;

	// ���������㹲��
	float det = e0 + e1 + e2;
	if (det == 0) return false;

	/* 
		�������꣺bi = ei / (e0 + e1 + e2) = bi / det
		��ֵ��z = simga(bi * zi) = sigma(ei * zi) / det
		���������� �ж�z�ڹ��߷�Χ����sigma(ei * zi)��ray.tMax * det���бȽ�
	*/
	float tScaled = e0 * P0.z + e1 * P1.z + e2 * P2.z;
	if (det > 0 && (tScaled <= 0 || tScaled >= ray.tMax * det)) return false;
	if (det < 0 && (tScaled >= 0 || tScaled <= ray.tMax * det)) return false;

	// ��������
	float invDet = 1.f / det;
	float t = tScaled * invDet;
	float b0 = e0 * invDet;
	float b1 = e1 * invDet;
	float b2 = e2 * invDet;
	
	glm::vec2 uv0 = v0.texcoord;
	glm::vec2 uv1 = v1.texcoord;
	glm::vec2 uv2 = v2.texcoord;
	glm::vec2 uvHit = uv0 * b0 + uv1 * b1 + uv2 * b2;

	glm::vec3 normal0 = v0.normal;
	glm::vec3 normal1 = v1.normal;
	glm::vec3 normal2 = v2.normal;
	glm::vec3 nHit;
	
	// �������β����ڷ���
	if (normal0 == glm::vec3(0) || normal1 == glm::vec3(0) || normal2 == glm::vec3(0)) {
		nHit = glm::normalize(glm::cross(p1 - p0, p2 - p0));
	} else {
		nHit = normal0 * b0 + normal1 * b1 + normal2 * b2;
	}

	// ���߻��������α��棬��Ҫ��ת����
	if (glm::dot(nHit, ray.dir) > 0) {
		nHit = -nHit;
	}

	isect->position = b0 * p0 + b1 * p1 + b2 * p2;
	isect->normal = nHit;
	isect->texcoord = uvHit;
	isect->textureId = tri.textureId;
	isect->materialId = tri.materialId;
	isect->time = t;
	ray.tMax = t;
	return true;
}