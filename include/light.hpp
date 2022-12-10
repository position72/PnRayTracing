#pragma once
#include "PnRT.hpp"

struct Light {
	int index; // �������������е�����
	float prefixArea; // ǰi�������ε������
};

// ���������u[0, 1], ����u�͵Ƶı������һά�ֲ�ѡ���ƹ�
inline int GetLightIndex(float u) {
	if (lights.empty()) return -1;
	int L = 0, R = lights.size() - 1, ans = -1;
	float randomArea = u * lights[R].prefixArea;
	while (L <= R) { 
		int mid = L + R >> 1;
		if (lights[mid].prefixArea >= randomArea) {
			ans = mid;
			R = mid - 1;
		} else {
			L = mid + 1;
		}
	}
	return lights[ans].index;
}