#include "PnRT.hpp"

class ImGuiLayer {
public:
	ImGuiLayer(GLFWwindow* window) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext(NULL);
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 450");
	}

	void FrameBegin() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void FrameEnd() {
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void updateModel(const std::vector<Model>& models) {
		modelNames.clear();
		materialId.clear();
		for (const auto& m : models) {
			modelNames.push_back(m.name.c_str());
			materialId.push_back(m.materialId);
		}
		modelSettingCurrentItem = modelNames[0];
		currentMaterialId = models[0].materialId;
	}

	bool showModelSettingCombo() {
		if (ImGui::BeginCombo("Model Setting", modelSettingCurrentItem)) {
			for (int n = 0; n < modelNames.size(); n++) {
				bool is_selected = modelSettingCurrentItem == modelNames[n];
				if (ImGui::Selectable(modelNames[n], is_selected)) {
					modelSettingCurrentItem = modelNames[n];
					currentMaterialId = materialId[n];
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		
		bool change = false;
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_1D, materialTex);
		int offset = MATERIAL_SIZE * currentMaterialId / 3; // 第几个像素，一组RGB组成一个像素
		change |= ImGui::ColorEdit3("basecolor", glm::value_ptr(materials[currentMaterialId].baseColor));
		change |= ImGui::SliderFloat("subsurface", &materials[currentMaterialId].subsurface, 0.0, 1.0);
		change |= ImGui::SliderFloat("metallic", &materials[currentMaterialId].metallic, 0.0, 1.0);
		change |= ImGui::SliderFloat("specular", &materials[currentMaterialId].specular, 0.0, 1.0);
		change |= ImGui::SliderFloat("specularTint", &materials[currentMaterialId].specularTint, 0.0, 1.0);
		change |= ImGui::SliderFloat("roughness", &materials[currentMaterialId].roughness, 0.0, 1.0);
		change |= ImGui::SliderFloat("anisotropic", &materials[currentMaterialId].anisotropic, 0.0, 1.0);
		change |= ImGui::SliderFloat("sheen", &materials[currentMaterialId].sheen, 0.0, 1.0);
		change |= ImGui::SliderFloat("sheenTint", &materials[currentMaterialId].sheenTint, 0.0, 1.0);
		change |= ImGui::SliderFloat("clearcoat", &materials[currentMaterialId].clearcoat, 0.0, 1.0);
		change |= ImGui::SliderFloat("clearcoatGloss", &materials[currentMaterialId].clearcoatGloss, 0.0, 1.0);
		change |= ImGui::SliderFloat("IOR", &materials[currentMaterialId].IOR, 0.0, 1.0);
		change |= ImGui::SliderFloat("transmission", &materials[currentMaterialId].transmission, 0.0, 1.0);
		
		if (change) {
			glTexSubImage1D(GL_TEXTURE_1D, 0, offset + 1, 3 * sizeof(float), GL_RGB, GL_FLOAT, glm::value_ptr(materials[currentMaterialId].baseColor));
			glTexSubImage1D(GL_TEXTURE_1D, 0, offset + 2, 3 * sizeof(float), GL_RGB, GL_FLOAT, &materials[currentMaterialId].subsurface);
			glTexSubImage1D(GL_TEXTURE_1D, 0, offset + 3, 3 * sizeof(float), GL_RGB, GL_FLOAT, &materials[currentMaterialId].specularTint);
			glTexSubImage1D(GL_TEXTURE_1D, 0, offset + 4, 3 * sizeof(float), GL_RGB, GL_FLOAT, &materials[currentMaterialId].sheen);
			glTexSubImage1D(GL_TEXTURE_1D, 0, offset + 5, 3 * sizeof(float), GL_RGB, GL_FLOAT, &materials[currentMaterialId].clearcoatGloss);
		}
		return change;
	}

	const char* modelSettingCurrentItem;
	int currentMaterialId;
	unsigned int materialTex;
	std::vector<const char*> modelNames;
	std::vector<int> materialId;
};