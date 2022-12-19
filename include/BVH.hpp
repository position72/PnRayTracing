#pragma once
#include "PnRT.hpp"
#include "bound.hpp"
#include "triangle.hpp"

struct BVHNode {
	Bound bound;
	int axis;
	int rightChild;
	int startIndex;
	int endIndex;
};

class BVH {
public:
	BVH(const std::vector<Vertex>& vertices, const std::vector<Triangle>& triangles)
		: vertices(vertices), triangles(triangles) {
		BuildBVH(0, triangles.size());
	}

	bool Intersect(const Ray& r, Interaction* isect) const {
		// �ѽ�Ҫ���ʵĽڵ�ѹ��ջ��
		int nodeStack[128], top = 0;
		nodeStack[top++] = 0;
		// �жϻ��б�־
		bool hit = false;
		while (top) {
			const int curId = nodeStack[--top];
			const BVHNode& node = bvh[curId];
			// δ���иõ��Χ��������
			if (!BoundIntersect(node.bound, r)) continue;
			if (node.rightChild == -1) { // Ҷ�ӽڵ�
				for (int i = node.startIndex; i < node.endIndex; ++i) { // ÿ����������
					if (TriangleIntersect(triangles[i], r, vertices, isect)) {
						hit = true;
					}
				}
			} else { // ��Ҷ�ӽڵ�
				// �ڻ������Ϲ��߷���Ϊ����������Ҫ�ȷ����Ҷ���
				if (r.dir[node.axis] < 0) {
					nodeStack[top++] = curId + 1;
					// �ж��Ƿ������һ�����ӵİ�Χ��
					const BVHNode& rc = bvh[node.rightChild];
					if (BoundIntersect(rc.bound, r)) nodeStack[top++] = node.rightChild;
				} else {
					nodeStack[top++] = node.rightChild;
					const BVHNode& lc = bvh[curId + 1];
					if (BoundIntersect(lc.bound, r)) nodeStack[top++] = curId + 1;
				}
			}
		}
		return hit;
	}

	// ֻ�����ཻ���ԣ�������Interaction��Ϣ
	bool IntersectP(const Ray& r) const {
		// �ѽ�Ҫ���ʵĽڵ�ѹ��ջ��
		int nodeStack[128], top = 0;
		nodeStack[top++] = 0;
		while (top) {
			const int curId = nodeStack[--top];
			const BVHNode& node = bvh[curId];
			// δ���иõ��Χ��������
			if (!BoundIntersect(node.bound, r)) continue;
			if (node.rightChild == -1) { // Ҷ�ӽڵ�
				for (int i = node.startIndex; i < node.endIndex; ++i) { // ÿ����������
					if (TriangleIntersectP(triangles[i], r, vertices))
						return true;
				}
			} else { // ��Ҷ�ӽڵ�
				// �ڻ������Ϲ��߷���Ϊ����������Ҫ�ȷ����Ҷ���
				if (r.dir[node.axis] < 0) {
					nodeStack[top++] = curId + 1;
					// �ж��Ƿ������һ�����ӵİ�Χ��
					const BVHNode& rc = bvh[node.rightChild];
					if (BoundIntersect(rc.bound, r)) nodeStack[top++] = node.rightChild;
				} else {
					nodeStack[top++] = node.rightChild;
					const BVHNode& lc = bvh[curId + 1];
					if (BoundIntersect(lc.bound, r)) nodeStack[top++] = curId + 1;
				}
			}
		}
		return false;
	}
private:
	struct Bucket {
		int nTriangles = 0;
		Bound bound;
	};

	int BuildBVH(int L, int R, int depth = 0) {
		// �ڵ���
		static int nodeId = -1;
		++nodeId;
		// ��ǰ�ڵ�����������ι��ɵİ�Χ��
		Bound bound;
		for (int i = L; i < R; ++i) {
			bound.Union(triangles[i].bound);
		}
		int nTriangles = R - L;
		// Ҷ�ӽڵ�
		if (nTriangles <= 2) {
			bvh.push_back({ bound, -1, -1, L, R });
			return nodeId;
		}
		Bound centerBound;
		for (int i = L; i < R; ++i)
			centerBound.Union(triangles[i].boundCenter);
		// ȡ��Χ�жԽ������ά�Ƚ��л���
		glm::vec3 diagonal = centerBound.Diagonal();
		int d;
		if (diagonal.x >= diagonal.y && diagonal.x >= diagonal.z) d = 0;
		else if (diagonal.y >= diagonal.x && diagonal.y >= diagonal.z) d = 1;
		else d = 2;
		// ��Χ�д�СΪ0��Ҷ�ӽڵ㴦��
		if (centerBound.pMax[d] == centerBound.pMin[d]) {
			bvh.push_back({ bound, -1, -1, L, R });
			return nodeId;
		}
		// ������Ữ�ֳ����ɸ�Ͱ������ÿ�������εİ�Χ�����ķֱ�װ����Ӧ��Ͱ��
		constexpr int BUCKETSIZE = 12;
		Bucket buc[BUCKETSIZE];
		for (int i = L; i < R; ++i) {
			int pos = ((triangles[i].boundCenter[d] - centerBound.pMin[d]) / diagonal[d]) * BUCKETSIZE;
			if (pos == BUCKETSIZE) pos = BUCKETSIZE - 1;
			assert(pos >= 0 && pos < BUCKETSIZE);
			buc[pos].nTriangles++;
			buc[pos].bound.Union(triangles[i].bound);
		}

		// ȡĳ�����ƻ��ִ�����С��Ͱ���ڸ�Ͱ�ں͸�Ͱ��ߵ������λ��ֵ�����ӣ����໮�ֵ��Ҷ���
		float minCost = FLOAT_MAX;
		int midBuc = 0;
		for (int m = 0; m < BUCKETSIZE - 1; ++m) {
			Bound b0, b1;
			int c0 = 0, c1 = 0;
			for (int i = 0; i <= m; ++i) {
				c0 += buc[i].nTriangles;
				b0.Union(buc[i].bound);
			}
			for (int i = m + 1; i < BUCKETSIZE; ++i) {
				c1 += buc[i].nTriangles;
				b1.Union(buc[i].bound);
			}
			float cost = trav + (b0.SurfaceArea() * c0 + b1.SurfaceArea() * c1) / bound.SurfaceArea();
			if (cost < minCost) {
				minCost = cost;
				midBuc = m;
			}
		}

		int mid = std::partition(triangles.begin() + L, triangles.begin() + R,
			[&](const Triangle& t) -> bool {
				int pos = ((t.boundCenter[d] - centerBound.pMin[d]) / diagonal[d]) * BUCKETSIZE;
		if (pos == BUCKETSIZE) pos = BUCKETSIZE - 1;
		return pos <= midBuc;
			}) - triangles.begin();

			float leafCost = nTriangles;
			// Ҷ�ӽڵ㣺�����θ���С�����ƣ����ҷ���Ҷ�ӻ���С�ڻ��ֺ�Ļ���
			if (nTriangles <= maxTrianglesInLeaf && leafCost <= minCost || mid == L) {
				bvh.push_back({ bound, -1, -1, L, R });
				return nodeId;
			}

			bvh.push_back({ bound, d, 0, L, R });
			// ��BVH�ڵ��ų����У���Ҷ�ӽڵ����������������ұߣ�ֻ��¼�Ҷ��ӱ��
			int nowId = nodeId;
			int lc = BuildBVH(L, mid, depth + 1);
			bvh[nowId].rightChild = BuildBVH(mid, R, depth + 1);
			return nowId;
	}

	int maxTrianglesInLeaf = 255;
	float trav = 1.f;
public:
	std::vector<Vertex> vertices;
	std::vector<Triangle> triangles;
	std::vector<BVHNode> bvh;
};