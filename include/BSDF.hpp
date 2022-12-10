#pragma once
#include "PnRT.hpp"

// 在切线空间下进行单位半球采样
inline glm::vec3 UniformSampleHemisphere(const glm::vec2& u) {
    float z = u[0];
    float r = std::sqrt(std::max(0.f, 1.f - z * z));
    float phi = 2 * PI * u[1];
    return glm::vec3(r * std::cos(phi), r * std::sin(phi), z);
}

inline glm::vec3 DiffuseBRDF(const glm::vec3& n, const glm::vec3& wo, 
    const Material& m, glm::vec3 *wi, float* pdf) {
    glm::vec3 t; // 切线
    if (n.z > 0.9999995f) t = glm::vec3(1.f, 0.f, 0.f);
    else t = glm::normalize(glm::cross(n, wo));
    glm::vec3 b = glm::cross(n, t); // 次切线
    *wi = UniformSampleHemisphere(glm::vec2(Rand0To1(), Rand0To1()));
    *pdf = InvPI;
    // 从切线空间转为世界空间
    *wi = glm::vec3(t[0] * (*wi)[0] + t[1] * (*wi)[1] + t[2] * (*wi)[2],
                    b[0] * (*wi)[0] + b[1] * (*wi)[1] + b[2] * (*wi)[2],
                    n[0] * (*wi)[0] + n[1] * (*wi)[1] + n[2] * (*wi)[2]);
    return m.baseColor * InvPI;
}