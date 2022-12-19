#include "PnRT.hpp"
#include "BVH.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "shader.hpp"
#include "light.hpp"
#include "BSDF.hpp"
#include "ImGuiLayer.hpp"

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
void WindowInit() {
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
	glfwSwapInterval(0);

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

struct BounceLi {
	glm::vec3 Li = glm::vec3(0);
	glm::vec3 f = glm::vec3(0);
	float cosTheta;
	float pdf;
	float pR;
};
const int maxBounceDepth = 4;
glm::vec3 Li(const Ray& reye, const BVH& bvhaccel) {
	Ray ray = reye;
	Interaction isect;
	glm::vec3 result(0);
	// res[i]: 光线第i次弹射时，采样得到的直接光照，所在交点的BRDF，
	// 采样得到下一次光线方向wi与法线夹角的cos值和pdf，俄罗斯轮盘赌的概率pR
	BounceLi res[maxBounceDepth];
	int bounce = 0;
	while (1) {
		if (!bvhaccel.Intersect(ray, &isect)) {
			break;
		}
		const glm::vec3& p = isect.position;
		const glm::vec3& n = isect.normal;
		const glm::vec3& wo = -ray.dir;
		Material material = materials[isect.materialId];
		if (isect.textureId != -1) { // 将材质baseColor修改为纹理颜色
			material.baseColor = TextureGetColor1(isect.textureId, isect.texcoord[0], isect.texcoord[1]);
		}
		glm::vec3 wi;
		float pdf;
		// BRDF项，返回函数值，采样方向和概率
		glm::vec3 f = DiffuseBRDF(n, wo, material, &wi, &pdf);
		// 自发光，只在第一次bounce接受物体的自发光
		glm::vec3 LDirect = bounce == 0 ? material.emssive : glm::vec3(0);
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
		
		// 俄罗斯轮盘赌决定是否计算间接光
		float pR = 0.85;
		res[bounce] = { LDirect, f, std::abs(glm::dot(n, wi)), pdf, pR };
		if (Rand0To1() <= pR && bounce + 1 < maxBounceDepth) {
			ray.origin = p + n * 0.0001f;
			ray.dir = wi;
			ray.tMax = FLOAT_MAX;
			++bounce;
		} else {
			break;
		}
	}
	if (bounce == 0) return res[0].Li;
	for (int i = bounce - 1; i >= 0; --i) {
		// 当前弹射所接受的光照等于直接光照加间接光照，下一次弹射的直接光照作为当前的间接光照
		res[i].Li += res[i].f * res[i + 1].Li * res[i].cosTheta / (res[i].pdf * (1.f - res[i].pR));
	}
	return res[0].Li;
}

int main() {
	WindowInit();
	ImGuiLayer* gui = new ImGuiLayer(window);
	glm::vec3 cameraPosition(0, 2.8, 7);
	glm::vec3 cameraCenter(0, 2.8, 0);
	glm::vec3 cameraUp(0, 1, 0);
	Camera camera(cameraPosition, cameraCenter, cameraUp, 45.0, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Cornell Box
	Material m;
	m.baseColor = glm::vec3(0.65, 0.05, 0.05);
	Model m0("./model/Bunny.obj", glm::translate(glm::mat4(1), glm::vec3(0, 0, -2)) * 
		glm::scale(glm::mat4(1), glm::vec3(8)), m, "bunny");
	Model m1("./model/marry/marry.obj", glm::translate(glm::mat4(1), glm::vec3(0.1, 0, -0.5)), m, "marry");
	m.baseColor = glm::vec3(0.73, 0.73, 0.73);
	Model f1("./model/floor/floor.obj", 
		glm::scale(glm::mat4(1), glm::vec3(0.1)), m, "floor");
	Model f2("./model/floor/floor.obj",
		glm::translate(glm::mat4(1), glm::vec3(0, 2.75, -2.75)) *
		glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(1.0, 0.0, 0.0)) *
		glm::scale(glm::mat4(1), glm::vec3(0.1)), m, "front_wall");
	m.baseColor = glm::vec3(0.12, 0.45, 0.15);
	Model f3("./model/floor/floor.obj",
		glm::translate(glm::mat4(1), glm::vec3(2.75, 2.75, 0)) *
		glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0.0, 0.0, 1.0)) *
		glm::scale(glm::mat4(1), glm::vec3(0.1)), m, "right_wall");
	m.baseColor = glm::vec3(0.65, 0.05, 0.05);
	Model f4("./model/floor/floor.obj",
		glm::translate(glm::mat4(1), glm::vec3(-2.75, 2.75, 0.0)) *
		glm::rotate(glm::mat4(1), glm::radians(-90.f), glm::vec3(0.0, 0.0, 1.0)) *
		glm::scale(glm::mat4(1), glm::vec3(0.1)), m, "left_wall");
	m.baseColor = glm::vec3(0.73, 0.73, 0.73);
	Model f5("./model/floor/floor.obj",
		glm::translate(glm::mat4(1), glm::vec3(0, 5.55, 0)) *
		glm::rotate(glm::mat4(1), glm::radians(180.f), glm::vec3(0.0, 0.0, 1.0)) *
		glm::scale(glm::mat4(1), glm::vec3(0.1)), m, "ceiling");
	m.emssive = glm::vec3(3);
	Model light("./model/floor/floor.obj",
		glm::translate(glm::mat4(1), glm::vec3(0, 5.54, 0)) *
		glm::rotate(glm::mat4(1), glm::radians(180.f), glm::vec3(0.0, 0.0, 1.0)) *
		glm::scale(glm::mat4(1), glm::vec3(0.02)), m, "ceiling_light");
	std::vector<Model> models;
	models.push_back(m0);
	models.push_back(f1);
	models.push_back(f2);
	models.push_back(f3);
	models.push_back(f4);
	models.push_back(f5);
	models.push_back(light);

	ModelOutput(models); 
	std::cout << "Load " << vertices.size() << " vertices and " << triangles.size() << " triangles" << std::endl;

	gui->updateModel(models);

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

		gui->materialTex = tex[1];
	
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
		/*glBindBuffer(GL_TEXTURE_BUFFER, tbo[1]);
		glBufferData(GL_TEXTURE_BUFFER, sizeof(float) * MATERIAL_SIZE * materials.size(), &materialBuffer[0], GL_STATIC_DRAW);*/
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_1D, tex[1]); 
		//glTexBuffer(GL_TEXTURE_1D, GL_RGB32F, tbo[1]);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F, sizeof(float) * MATERIAL_SIZE * materials.size(), 0, GL_RGB, GL_FLOAT, &materialBuffer[0]);
		glGenerateMipmap(GL_TEXTURE_1D);
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
		unsigned int frameCount = 1;
		while (!glfwWindowShouldClose(window)) {
			glClear(GL_COLOR_BUFFER_BIT);

			gui->FrameBegin();

			float currentTime = glfwGetTime();
			deltaTime = currentTime - lastTime;
			lastTime = currentTime;
			++frameCount;
			std::string fps = "COMPUTE_SHADER FPS: " + std::to_string(1.0 / deltaTime);
			glfwSetWindowTitle(window, fps.c_str());

			glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

			cs.use();

			ImGui::Begin("Settings");
			if (gui->showModelSettingCombo()) {
				cs.setInt("redraw", 1);
				frameCount = 0;
			} else {
				cs.setInt("redraw", 0);
			}
			ImGui::End();

			cs.setVec3f("camera.eye", camera.eye.x, camera.eye.y, camera.eye.z);
			cs.setVec3f("camera.lowerLeftCorner", camera.lowerLeftCorner.x, 
				camera.lowerLeftCorner.y, camera.lowerLeftCorner.z);
			cs.setVec3f("camera.horizontal", camera.horizontal.x, camera.horizontal.y, camera.horizontal.z);
			cs.setVec3f("camera.vertical", camera.vertical.x, camera.vertical.y, camera.vertical.z);
			cs.setUInt("frameCount", frameCount); // 根据glfwGetTime更新当前帧随机数种子
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, debugtbo);
			glDispatchCompute(SCREEN_WIDTH / WORK_BLOCK_SIZE + 1, SCREEN_HEIGHT / WORK_BLOCK_SIZE + 1, 1);

			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


			render.use();
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, outputImage);
			renderQuad();

			gui->FrameEnd();

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		glfwTerminate();
		return 0;
	} else {
		c1 = clock();
		int pixelCount = 0;
		for (int j = 0; j < SCREEN_HEIGHT; ++j) {
			for (int i = 0; i < SCREEN_WIDTH; ++i) {
				++pixelCount;
				std::cerr << (float)pixelCount / (SCREEN_HEIGHT * SCREEN_WIDTH) << std::endl;
				Ray ray = CameraGetRay(camera, (float)i / SCREEN_WIDTH, (float)j / SCREEN_HEIGHT);
				ray.dir = glm::normalize(ray.dir);
				Interaction isect;
				int dataPos = camera.nChannels * ((SCREEN_HEIGHT - j - 1) * SCREEN_WIDTH + i);
				if (i == 153 && j == 3) {
					std::cout << "break!" << std::endl;
				}
				const int SAMPLE_COUNT = 50;
				glm::vec3 color;
				for (int s = 0; s < SAMPLE_COUNT; ++s) {
					color += Li(ray, *bvhaccel);
				}
				color /= SAMPLE_COUNT;
				camera.data[dataPos + 0] = Clamp(color[0] * 255, 0, 255);
				camera.data[dataPos + 1] = Clamp(color[1] * 255, 0, 255);
				camera.data[dataPos + 2] = Clamp(color[2] * 255, 0, 255);
			}
		}
		c2 = clock();
		camera.Output("./photo.png");
		std::cout << "Having generated the photo costs: " << c2 - c1 << "ms\n";
	}
	
}