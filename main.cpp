#include "PnRT.hpp"
#include "bound.hpp"
#include "triangle.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "shader.hpp"
#include "light.hpp"
#include "BSDF.hpp"

void APIENTRY glDebugOutput(GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void* userParam) {
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return; // ignore these non-significant error codes

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source) {
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

GLFWwindow* window;
void GLFWInit() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(-1);
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		exit(-1);
	}

	int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // makes sure errors are displayed synchronously
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
	srand(time(0));
}

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

void PrefixSum() {
	ComputeShader cs("./shaders/prefix_sum.comp");
	cs.use();
	float input_data[1024];
	float output_data[1024];
	for (int i = 0; i < 1024; ++i) input_data[i] = rand() % 10, output_data[i] = 0;
	for (int i = 0; i < 20; ++i) {
		std::cout << input_data[i] << " ";
	}
	std::cout << std::endl;
	unsigned int buffers[2];
	glCreateBuffers(2, buffers);

	glNamedBufferStorage(buffers[0], sizeof(input_data), input_data, GL_MAP_READ_BIT | GL_DYNAMIC_STORAGE_BIT);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[0]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[0]);

	glNamedBufferStorage(buffers[1], sizeof(output_data), output_data, GL_MAP_READ_BIT | GL_DYNAMIC_STORAGE_BIT);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[1]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1]);

	glDispatchCompute(1, 1, 1);
	float* out_data = (float*)glMapNamedBufferRange(buffers[1], 0, sizeof(output_data), GL_MAP_READ_BIT);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[1]);
	for (int i = 0; i < 20; ++i)
		std::cout << out_data[i] << " ";
}

void renderQuad() {
	static unsigned int quadVAO = 0, quadVBO;
	if (!quadVAO) {
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

glm::vec3 Lo(const Interaction& isect, const glm::vec3& wo, const BVH& bvhaccel) {
	const glm::vec3& p = isect.position;
	const glm::vec3& n = isect.normal;
	Material material = materials[isect.materialId];
	if (isect.textureId != -1) { // 将材质baseColor修改为纹理颜色
		material.baseColor = TextureGetColor1(isect.textureId, isect.texcoord[0], isect.texcoord[1]);
	}
	glm::vec3 wi;
	float pdf;
	// BRDF项，返回函数值，采样方向和概率
	glm::vec3 f = DiffuseBRDF(n, wo, material, &wi, &pdf); 
	// 自发光
	glm::vec3 LDirect = material.emssive;
	// 其他灯光的直接光照
	int triIndex = GetLightIndex(Rand0To1());
	if (triIndex != -1) { // 找到其中一个灯光
		// 在该三角形上采样
		const Triangle& tri = bvhaccel.triangles[triIndex];
		Interaction triangleIsect = TriangleSample(tri, glm::vec2(Rand0To1(), Rand0To1()), bvhaccel.vertices);
		// 发射阴影射线，判断灯光与当前点之间有无遮挡
		Ray r;
		r.dir = triangleIsect.position - p;
		r.tMax = 1.f - ShadowEpsilon; // 防止光线与目标物体相交
		r.origin = p + n * 0.0001f; // 防止自相交
		if (!bvhaccel.IntersectP(r)) {
			// pdf为所有灯光表面积的倒数
			float lPdf = 1.f / lights[lights.size() - 1].prefixArea;
			float dis2 = r.dir.x * r.dir.x + r.dir.y * r.dir.y + r.dir.z * r.dir.z;
			const glm::vec3& li = materials[triangleIsect.materialId].emssive;
			LDirect += f * li * std::abs(glm::dot(n, r.dir) * glm::dot(triangleIsect.normal, r.dir)) / 
				(lPdf * dis2);
		}
	}
	return LDirect;
}

int main() {
	GLFWInit();
	glm::vec3 cameraPosition(0, 2.8, 9);
	glm::vec3 cameraCenter(0, 2.8, 0);
	glm::vec3 cameraUp(0, 1, 0);
	Camera camera(cameraPosition, cameraCenter, cameraUp, 45.0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// 存储一个默认材质
	materials.push_back(Material{});
	Material m;
	m.baseColor = glm::vec3(0.65, 0.05, 0.05);
	materials.push_back(m);
	m.baseColor = glm::vec3(0.73, 0.73, 0.73);
	materials.push_back(m);
	m.baseColor = glm::vec3(0.12, 0.45, 0.15);
	materials.push_back(m);
	m.baseColor = glm::vec3(0.1);
	m.emssive = glm::vec3(2);
	materials.push_back(m);

	// Cornell Box
	//Model m0("./model/Bunny.obj", glm::translate(glm::mat4(1), glm::vec3(0.6, 1.85, 3.7)), 0);
	Model m1("./model/marry/marry.obj", glm::translate(glm::mat4(1), glm::vec3(0.1, 0, -1)), 0);
	Model f1("./model/floor/floor.obj", 
		glm::scale(glm::mat4(1), glm::vec3(0.1)), 2);
	Model f2("./model/floor/floor.obj",
		glm::translate(glm::mat4(1), glm::vec3(0, 2.75, -2.75)) *
		glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(1.0, 0.0, 0.0)) *
		glm::scale(glm::mat4(1), glm::vec3(0.1)), 2);
	Model f3("./model/floor/floor.obj",
		glm::translate(glm::mat4(1), glm::vec3(2.75, 2.75, 0)) *
		glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0.0, 0.0, 1.0)) *
		glm::scale(glm::mat4(1), glm::vec3(0.1)), 3);
	Model f4("./model/floor/floor.obj",
		glm::translate(glm::mat4(1), glm::vec3(-2.75, 2.75, 0.0)) *
		glm::rotate(glm::mat4(1), glm::radians(-90.f), glm::vec3(0.0, 0.0, 1.0)) *
		glm::scale(glm::mat4(1), glm::vec3(0.1)), 1);
	Model f5("./model/floor/floor.obj",
		glm::translate(glm::mat4(1), glm::vec3(0, 5.55, 0)) *
		glm::rotate(glm::mat4(1), glm::radians(180.f), glm::vec3(0.0, 0.0, 1.0)) *
		glm::scale(glm::mat4(1), glm::vec3(0.1)), 2);
	Model light("./model/floor/floor.obj",
		glm::translate(glm::mat4(1), glm::vec3(0, 5.54, 0)) *
		glm::rotate(glm::mat4(1), glm::radians(180.f), glm::vec3(0.0, 0.0, 1.0)) *
		glm::scale(glm::mat4(1), glm::vec3(0.02)), 4);
	std::vector<Model> models;
	models.push_back(m1);
	models.push_back(f1);
	models.push_back(f2);
	models.push_back(f3);
	models.push_back(f4);
	models.push_back(f5);
	models.push_back(light);

	ModelOutput(models); 
	std::cout << "Load " << vertices.size() << " vertices and " << triangles.size() << " triangles" << std::endl;

	// 构建bvh，获取bvh结构以及重排后的顶点和三角形数据
	auto c1 = clock();
	auto bvhaccel = std::make_shared<BVH>(std::move(vertices), std::move(triangles));
	auto c2 = clock();
	std::cout << "Build BVH completed, cost: " << c2 - c1 << " ms" << std::endl;

	// 在重排后的三角形中寻找能自发光的三角形索引，加入到lights
	for (int i = 0; i < bvhaccel->triangles.size(); ++i) {
		const Triangle& tri = bvhaccel->triangles[i];
		if (materials[tri.materialId].emssive != glm::vec3(0)) {
			lights.push_back({ i, tri.area });
			if (lights.size() > 1) {
				lights[lights.size() - 1].prefixArea += lights[lights.size() - 2].prefixArea;
			}
		}
	}
	std::cout << "Load " << lights.size() << " lights" << std::endl;

	int renderCase = 0; // 0: GPU, 1: CPU
	if (renderCase == 0) {
		ComputeShader cs("./shaders/ray_tracing.comp");
		VFShader render("./shaders/render.vert", "./shaders/render.frag");

		cs.use();
		cs.setInt("SCREEN_WIDTH", SCREEN_WIDTH);
		cs.setInt("SCREEN_HEIGHT", SCREEN_HEIGHT);
		cs.setInt("lightsSize", lights.size());
		cs.setFloat("lightsSumArea", lights[lights.size() - 1].prefixArea);

		c1 = clock();
		// 将数据存放到缓冲纹理中并传输到着色器中

		// 创建缓冲区对象和纹理对象
		unsigned int tbo[5], tex[5];
		glGenBuffers(5, tbo);
		glGenTextures(5, tex);
	
		std::vector<float> vertexBuffer; // 顶点
		vertexBuffer.resize(VERTEX_SIZE * bvhaccel->vertices.size());
		int num = 0;
		for (const auto& vertex : bvhaccel->vertices) { // 加载顺序与计算着色器中读取顺序一致
			vertexBuffer[num++] = vertex.position[0];
			vertexBuffer[num++] = vertex.position[1];
			vertexBuffer[num++] = vertex.position[2];
			vertexBuffer[num++] = vertex.normal[0];
			vertexBuffer[num++] = vertex.normal[1];
			vertexBuffer[num++] = vertex.normal[2];
			vertexBuffer[num++] = vertex.tangent[0];
			vertexBuffer[num++] = vertex.tangent[1];
			vertexBuffer[num++] = vertex.tangent[2];
			vertexBuffer[num++] = vertex.bitangent[0];
			vertexBuffer[num++] = vertex.bitangent[1];
			vertexBuffer[num++] = vertex.bitangent[2];
			vertexBuffer[num++] = vertex.texcoord[0];
			vertexBuffer[num++] = vertex.texcoord[1];
			vertexBuffer[num++] = 0.f; // 保证12字节对齐
		}
		glBindBuffer(GL_TEXTURE_BUFFER, tbo[0]); // 绑定缓冲区
		glBufferData(GL_TEXTURE_BUFFER, sizeof(float) * VERTEX_SIZE * bvhaccel->vertices.size(), &vertexBuffer[0], GL_STATIC_DRAW); // 向纹理填充数据
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_BUFFER, tex[0]); // 绑定纹理
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo[0]); // 将tbo中的数据关联到texture buffer

		std::vector<float> materialBuffer; // 材质
		materialBuffer.resize(MATERIAL_SIZE * materials.size());
		num = 0;
		for (const auto& material : materials) {
			materialBuffer[num++] = material.emssive[0];
			materialBuffer[num++] = material.emssive[1];
			materialBuffer[num++] = material.emssive[2];
			materialBuffer[num++] = material.baseColor[0];
			materialBuffer[num++] = material.baseColor[1];
			materialBuffer[num++] = material.baseColor[2];
			materialBuffer[num++] = material.subsurface;
			materialBuffer[num++] = material.metallic;
			materialBuffer[num++] = material.specular;
			materialBuffer[num++] = material.specularTint;
			materialBuffer[num++] = material.roughness;
			materialBuffer[num++] = material.anisotropic;
			materialBuffer[num++] = material.sheen;
			materialBuffer[num++] = material.sheenTint;
			materialBuffer[num++] = material.clearcoat;
			materialBuffer[num++] = material.clearcoatGloss;
			materialBuffer[num++] = material.IOR;
			materialBuffer[num++] = material.transmission;
		}
		glBindBuffer(GL_TEXTURE_BUFFER, tbo[1]);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(float) * MATERIAL_SIZE * materials.size(), &materialBuffer[0], GL_STATIC_DRAW);
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_BUFFER, tex[1]); 
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo[1]); 
		std::cout << "materialBuffer width: " << MATERIAL_SIZE * materials.size() / 3 << "\n";

		std::vector<float> triangleBuffer; // 三角形
		triangleBuffer.resize(TRIANGLE_SIZE * bvhaccel->triangles.size());
		num = 0;
		for (const auto& tri : bvhaccel->triangles) {
			triangleBuffer[num++] = (float)tri.indices[0];
			triangleBuffer[num++] = (float)tri.indices[1];
			triangleBuffer[num++] = (float)tri.indices[2];
			triangleBuffer[num++] = (float)tri.materialId;
			triangleBuffer[num++] = (float)tri.textureId;
			triangleBuffer[num++] = tri.area;
		}
		glBindBuffer(GL_TEXTURE_BUFFER, tbo[2]);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(float) * TRIANGLE_SIZE * bvhaccel->triangles.size(), &triangleBuffer[0], GL_STATIC_DRAW);
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_BUFFER, tex[2]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo[2]);
		std::cout << "triangleBuffer width: " << TRIANGLE_SIZE * bvhaccel->triangles.size() / 3 << "\n";

		std::vector<float> bvhnodeBuffer; // bvh结点
		bvhnodeBuffer.resize(BVHNODE_SIZE * bvhaccel->bvh.size());
		num = 0;
		for (const auto& node : bvhaccel->bvh) {
			bvhnodeBuffer[num++] = node.bound.pMin[0];
			bvhnodeBuffer[num++] = node.bound.pMin[1];
			bvhnodeBuffer[num++] = node.bound.pMin[2];
			bvhnodeBuffer[num++] = node.bound.pMax[0];
			bvhnodeBuffer[num++] = node.bound.pMax[1];
			bvhnodeBuffer[num++] = node.bound.pMax[2];
			bvhnodeBuffer[num++] = (float)node.axis;
			bvhnodeBuffer[num++] = (float)node.rightChild;
			bvhnodeBuffer[num++] = (float)node.startIndex;
			bvhnodeBuffer[num++] = (float)node.endIndex;
			bvhnodeBuffer[num++] = 0.f;
			bvhnodeBuffer[num++] = 0.f;
		}
		glBindBuffer(GL_TEXTURE_BUFFER, tbo[3]);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(float) * BVHNODE_SIZE * bvhaccel->bvh.size(), &bvhnodeBuffer[0], GL_STATIC_DRAW);
		glActiveTexture(GL_TEXTURE0 + 3);
		glBindTexture(GL_TEXTURE_BUFFER, tex[3]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo[3]);
		std::cout << "bvhnodeBuffer width: " << BVHNODE_SIZE * bvhaccel->bvh.size() / 3 << "\n";

		std::vector<float> lightBuffer; // 灯光
		lightBuffer.resize(LIGHT_SIZE * lights.size());
		num = 0;
		for (const auto& l : lights) {
			lightBuffer[num++] = (float)l.index;
			lightBuffer[num++] = l.prefixArea;
			lightBuffer[num++] = 0.f;
		}
		glBindBuffer(GL_TEXTURE_BUFFER, tbo[4]);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(float) * LIGHT_SIZE * lights.size(), &lightBuffer[0], GL_STATIC_DRAW);
		glActiveTexture(GL_TEXTURE0 + 4);
		glBindTexture(GL_TEXTURE_BUFFER, tex[4]);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo[4]);
		std::cout << "lightBuffer width: " << LIGHT_SIZE * lights.size() / 3 << "\n";

		// 生成并载入物体的纹理
		for (int i = 0; i < textureInfos.size(); ++i) {
			int width = textureInfos[i].width;
			int height = textureInfos[i].height;
			int nChannels = textureInfos[i].nChannels;

			GLenum format;
			if (nChannels == 1) format = GL_RED;
			if (nChannels == 3) format = GL_RGB;
			if (nChannels == 4) format = GL_RGBA;

			unsigned int texture;
			glGenTextures(1, &texture);
			glActiveTexture(GL_TEXTURE5 + i); // 0, 1, 2, 3, 4已被占用
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, textures[i]);
			glGenerateMipmap(GL_TEXTURE_2D);
			textureInfos[i].tbo = texture;
		}

		// 给计算着色器中所有的textures[]绑定位置
		for (int i = 0; i < 20; ++i) { 
			std::string name = "textures[" + std::to_string(i) + "]";
			cs.setInt(name.c_str(), i + 5);
		}

		unsigned int outputImage; // 计算着色器输出图像
		glCreateTextures(GL_TEXTURE_2D, 1, &outputImage);
		glTextureStorage2D(outputImage, 1, GL_RGBA32F, SCREEN_WIDTH, SCREEN_HEIGHT);
		glBindImageTexture(0, outputImage, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		unsigned int debugtbo; // 调试输出用缓存区
		glCreateBuffers(1, &debugtbo);
		glNamedBufferStorage(debugtbo, 1024, NULL, GL_MAP_READ_BIT);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, debugtbo);
		
		c2 = clock();
		std::cout << "Loading data to shaders costs: " << c2 - c1 << "ms" << std::endl;

		const unsigned int WORK_BLOCK_SIZE = 32;
		float lastTime = glfwGetTime(), deltaTime;
		unsigned int frameCount = 0;
		while (!glfwWindowShouldClose(window)) {
			float currentTime = glfwGetTime();
			deltaTime = currentTime - lastTime;
			lastTime = currentTime;
			frameCount += deltaTime;
			std::string fps = "COMPUTE_SHADER FPS: " + std::to_string(1.0 / deltaTime);
			glfwSetWindowTitle(window, fps.c_str());

			glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

			cs.use();
			cs.setVec3f("camera.eye", camera.eye.x, camera.eye.y, camera.eye.z);
			cs.setVec3f("camera.lowerLeftCorner", camera.lowerLeftCorner.x, 
				camera.lowerLeftCorner.y, camera.lowerLeftCorner.z);
			cs.setVec3f("camera.horizontal", camera.horizontal.x, camera.horizontal.y, camera.horizontal.z);
			cs.setVec3f("camera.vertical", camera.vertical.x, camera.vertical.y, camera.vertical.z);
			cs.setUInt("frameCount", (GLuint)frameCount); // 根据glfwGetTime更新当前帧随机数种子
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, debugtbo);
			glDispatchCompute(SCREEN_WIDTH / WORK_BLOCK_SIZE + 1, SCREEN_HEIGHT / WORK_BLOCK_SIZE + 1, 1);

			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

			render.use();
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, outputImage);
			renderQuad();

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		glfwTerminate();
		return 0;
	} else {
		c1 = clock();
		int pixelCount = 0;
		for (int j = 0; j < SCREEN_HEIGHT; ++j)  {
			for (int i = 0; i < SCREEN_WIDTH; ++i) {
				++pixelCount;
				std::cerr << (float)pixelCount / (SCREEN_HEIGHT * SCREEN_WIDTH) << std::endl;
				Ray ray = CameraGetRay(camera, (float)i / SCREEN_WIDTH, (float)j / SCREEN_HEIGHT);
				ray.dir = glm::normalize(ray.dir);
				Interaction isect;
				int dataPos = camera.nChannels * ((SCREEN_HEIGHT - j - 1) * SCREEN_WIDTH + i);
				if (bvhaccel->Intersect(ray, &isect)) {
					/*glm::vec3 color = materials[isect.materialId].baseColor * 255.f;
					if (isect.textureId != -1) {
						color = TextureGetColor255(isect.textureId, isect.texcoord[0], isect.texcoord[1]);
					} */
					if (i == 153 && j == 3) {
						std::cout << "break!" << std::endl;
					}
					const int SAMPLE_COUNT = 30;
					glm::vec3 color;
					for (int s = 0; s < SAMPLE_COUNT; ++s) {
						color += Lo(isect, -ray.dir, *bvhaccel);
					}
					color /= SAMPLE_COUNT;
					camera.data[dataPos + 0] = Clamp(color[0] * 255, 0, 255);
					camera.data[dataPos + 1] = Clamp(color[1] * 255, 0, 255);
					camera.data[dataPos + 2] = Clamp(color[2] * 255, 0, 255);
				} else {
					camera.data[dataPos + 0] = 0;
					camera.data[dataPos + 1] = 0;
					camera.data[dataPos + 2] = 0;
				}
			}
		}
		c2 = clock();
		camera.Output("./photo.png");
		std::cout << "Having generated the photo costs: " << c2 - c1 << "ms\n";
	}
	
}