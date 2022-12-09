#pragma once
#include "PnRT.hpp"

struct Light {
	int index; // 在三角形数组中的索引
	float prefixArea; // 前i个三角形的面积和
};

// 输入随机数u[0, 1], 根据u和灯的表面积的一维分布选出灯光
inline int GetLightIndex(float u) {
	if (lights.empty()) return -1;
	float randomArea = u * (*lights.rend()).prefixArea;
	int L = 0, R = lights.size() - 1, ans = -1;
	while (L <= R) { 
		int mid = L + R >> 1;
		if (lights[mid].prefixArea >= u) {
			ans = mid;
			R = mid - 1;
		} else {
			L = mid + 1;
		}
	}
	return ans;
}