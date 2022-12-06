#pragma once
#include "PnRT.h"

struct Mesh {
	Mesh(const std::vector<Vertex>& vertices, const std::vector<int>& indices)
		:vertices(vertices), indices(indices) { }
	std::vector<Vertex> vertices;
	std::vector<int> indices;
};

class Model {
public:
	Model(const std::string& path, const glm::mat4& modelMatrix, unsigned int materialId)
		: modelMatrix(modelMatrix), materialId(materialId) {
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}		
		processNode(scene->mRootNode, scene);
	}
	std::vector<Mesh> meshes;
	glm::mat4 modelMatrix;
	int materialId;
private:
	void processNode(const aiNode* node, const aiScene* scene) {
		for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}
		for (unsigned int i = 0; i < node->mNumChildren; ++i) {
			processNode(node->mChildren[i], scene);
		}
	}
	Mesh processMesh(const aiMesh* mesh, const aiScene* scene) {
		std::vector<Vertex> vertices;
		std::vector<int> indices;

		for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
			glm::vec3 position(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
			glm::vec3 normal(0), tangent(0), bitangent(0);
			if (mesh->mNormals) normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			if (mesh->mTangents) tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
			if (mesh->mBitangents) bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
			glm::vec2 texcoord(0);
			if (mesh->mTextureCoords[0]) texcoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			vertices.push_back({ position, normal, tangent, bitangent, texcoord });
		}

		for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
			const aiFace& face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; ++j) {
				indices.push_back(face.mIndices[j]);
			}
		}
		return Mesh(std::move(vertices), std::move(indices));
	}
};

inline void ModelOutput(const std::vector<Model>& models, std::vector<Vertex>& vertices, std::vector<Triangle>& triangles) {
	int countVertices = 0;
	for (const auto& model : models) {
		glm::mat4 normalMatrix = glm::transpose(glm::inverse(model.modelMatrix));
		for (const auto& mesh : model.meshes) {
			for (const auto& vertex : mesh.vertices) {
				Vertex nVertex;
				nVertex.position = glm::vec3(model.modelMatrix * glm::vec4(vertex.position, 1.0));
				nVertex.normal = glm::vec3(normalMatrix * glm::vec4(vertex.normal, 1.0));
				nVertex.tangent = glm::vec3(model.modelMatrix * glm::vec4(vertex.tangent, 1.0));
				nVertex.bitangent = glm::vec3(model.modelMatrix * glm::vec4(vertex.bitangent, 1.0));
				nVertex.texcoord = vertex.texcoord;
				vertices.push_back(nVertex);
			}
			for (int i = 0; i < mesh.indices.size(); i += 3) {
				Triangle triangle;
				triangle.indices[0] = mesh.indices[i] + countVertices;
				triangle.indices[1] = mesh.indices[i + 1] + countVertices;
				triangle.indices[2] = mesh.indices[i + 2] + countVertices;
				triangle.materialId = model.materialId;
				triangle.bound.Union(vertices[triangle.indices[0]].position);
				triangle.bound.Union(vertices[triangle.indices[1]].position);
				triangle.bound.Union(vertices[triangle.indices[2]].position);
				triangle.boundCenter = (triangle.bound.pMax + triangle.bound.pMin) * .5f;
				triangles.push_back(triangle);
			}
			countVertices += mesh.vertices.size();
		}
	}
}