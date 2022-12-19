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
		// 把将要访问的节点压入栈中
		int nodeStack[128], top = 0;
		nodeStack[top++] = 0;
		// 判断击中标志
		bool hit = false;
		while (top) {
			const int curId = nodeStack[--top];
			const BVHNode& node = bvh[curId];
			// 未击中该点包围盒则跳过
			if (!BoundIntersect(node.bound, r)) continue;
			if (node.rightChild == -1) { // 叶子节点
				for (int i = node.startIndex; i < node.endIndex; ++i) { // 每个三角形求交
					if (TriangleIntersect(triangles[i], r, vertices, isect)) {
						hit = true;
					}
				}
			} else { // 非叶子节点
				// 在划分轴上光线方向为负方向，则需要先访问右儿子
				if (r.dir[node.axis] < 0) {
					nodeStack[top++] = curId + 1;
					// 判断是否击中另一个儿子的包围盒
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

	// 只进行相交测试，不返回Interaction信息
	bool IntersectP(const Ray& r) const {
		// 把将要访问的节点压入栈中
		int nodeStack[128], top = 0;
		nodeStack[top++] = 0;
		while (top) {
			const int curId = nodeStack[--top];
			const BVHNode& node = bvh[curId];
			// 未击中该点包围盒则跳过
			if (!BoundIntersect(node.bound, r)) continue;
			if (node.rightChild == -1) { // 叶子节点
				for (int i = node.startIndex; i < node.endIndex; ++i) { // 每个三角形求交
					if (TriangleIntersectP(triangles[i], r, vertices))
						return true;
				}
			} else { // 非叶子节点
				// 在划分轴上光线方向为负方向，则需要先访问右儿子
				if (r.dir[node.axis] < 0) {
					nodeStack[top++] = curId + 1;
					// 判断是否击中另一个儿子的包围盒
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
		// 节点编号
		static int nodeId = -1;
		++nodeId;
		// 求当前节点包含的三角形构成的包围盒
		Bound bound;
		for (int i = L; i < R; ++i) {
			bound.Union(triangles[i].bound);
		}
		int nTriangles = R - L;
		// 叶子节点
		if (nTriangles <= 2) {
			bvh.push_back({ bound, -1, -1, L, R });
			return nodeId;
		}
		Bound centerBound;
		for (int i = L; i < R; ++i)
			centerBound.Union(triangles[i].boundCenter);
		// 取包围盒对角线最长的维度进行划分
		glm::vec3 diagonal = centerBound.Diagonal();
		int d;
		if (diagonal.x >= diagonal.y && diagonal.x >= diagonal.z) d = 0;
		else if (diagonal.y >= diagonal.x && diagonal.y >= diagonal.z) d = 1;
		else d = 2;
		// 包围盒大小为0当叶子节点处理
		if (centerBound.pMax[d] == centerBound.pMin[d]) {
			bvh.push_back({ bound, -1, -1, L, R });
			return nodeId;
		}
		// 按最长的轴划分成若干个桶，根据每个三角形的包围盒中心分别装入相应的桶中
		constexpr int BUCKETSIZE = 12;
		Bucket buc[BUCKETSIZE];
		for (int i = L; i < R; ++i) {
			int pos = ((triangles[i].boundCenter[d] - centerBound.pMin[d]) / diagonal[d]) * BUCKETSIZE;
			if (pos == BUCKETSIZE) pos = BUCKETSIZE - 1;
			assert(pos >= 0 && pos < BUCKETSIZE);
			buc[pos].nTriangles++;
			buc[pos].bound.Union(triangles[i].bound);
		}

		// 取某个估计划分代价最小的桶，在该桶内和该桶左边的三角形划分到左儿子，其余划分到右儿子
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
			// 叶子节点：三角形个数小于限制，并且访问叶子花费小于划分后的花费
			if (nTriangles <= maxTrianglesInLeaf && leafCost <= minCost || mid == L) {
				bvh.push_back({ bound, -1, -1, L, R });
				return nodeId;
			}

			bvh.push_back({ bound, d, 0, L, R });
			// 将BVH节点排成序列，非叶子节点的左儿子在其相邻右边，只记录右儿子编号
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