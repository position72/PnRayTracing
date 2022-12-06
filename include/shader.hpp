#pragma once
#include "PnRT.h"

inline unsigned int createAndCompileShader(const char* shaderSource, GLenum type) {
	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &shaderSource, NULL);
	glCompileShader(shader);
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
		std::cout << shaderSource << " " << type << std::endl;
		std::cout << "Compile shader failed: " << infoLog << std::endl;
		exit(1);
	}
	return shader;
}

class Shader {
public:
	Shader() {}

	void use() const {
		glUseProgram(program);
	}

	void setFloat(const GLchar* name, const GLfloat& num) const {
		glUniform1f(glGetUniformLocation(program, name), num);
	}

	void setInt(const GLchar* name, const GLint& num) const {
		glUniform1i(glGetUniformLocation(program, name), num);
	}

	void setVec3f(const GLchar* name, const GLfloat& v0,
		const GLfloat& v1, const GLfloat& v2) const {
		glUniform3f(glGetUniformLocation(program, name), v0, v1, v2);
	}

	void setMat4f(const GLchar* name, const GLfloat* mat) const {
		glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, mat);
	}

	void setVec2f(const GLchar* name, const GLfloat& v0, const GLfloat& v1) const {
		glUniform2f(glGetUniformLocation(program, name), v0, v1);
	}

	int program;
};

class ComputeShader : public Shader {
public:
	ComputeShader(const char* path) {		
		std::string computeShaderCode;
		try {
			std::ifstream computeFile;
			computeFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			std::stringstream ss;
			computeFile.open(path);
			ss << computeFile.rdbuf();
			computeShaderCode = ss.str();
		} catch (std::ifstream::failure e) {
			std::cout << "Cannot open compute shader file: " << path << std::endl;
		}
		unsigned int shader = createAndCompileShader(computeShaderCode.c_str(), GL_COMPUTE_SHADER);
		program = glCreateProgram();
		glAttachShader(program, shader);
		glDeleteShader(shader);
		glLinkProgram(program);
		int sucessLink;
		glGetProgramiv(program, GL_LINK_STATUS, &sucessLink);
		if (!sucessLink) {
			char infoLog[512];
			glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
			std::cout << "ComputeShader failed to link: " << infoLog << std::endl;
			exit(4);
		}
	}
};

class VFShader : public Shader {
public:
	VFShader(const char* vertexShaderPath, const char* fragmentShaderPath) {
		std::ifstream vertexFile;
		std::ifstream fragmentFile;
		vertexFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fragmentFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		std::string vertexShaderCode;
		std::string fragmentShaderCode;
		std::string geometryShaderCode;
		try {
			std::stringstream vertexss, fragmentss;
			vertexFile.open(vertexShaderPath);
			vertexss << vertexFile.rdbuf();
			fragmentFile.open(fragmentShaderPath);
			fragmentss << fragmentFile.rdbuf();
			vertexShaderCode = vertexss.str();
			fragmentShaderCode = fragmentss.str();
		} catch (std::ifstream::failure e) {
			std::cout << "Cannot open shader file." << std::endl;
		}
		unsigned int vertexShader = createAndCompileShader(vertexShaderCode.c_str(), GL_VERTEX_SHADER);
		unsigned int fragmentShader = createAndCompileShader(fragmentShaderCode.c_str(), GL_FRAGMENT_SHADER);
		program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glDeleteShader(vertexShader);
		glAttachShader(program, fragmentShader);
		glDeleteShader(fragmentShader);
		glLinkProgram(program);
		int success;
		char infoLog[512];
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(program, sizeof(infoLog), NULL, infoLog);
			std::cout << "Link program failed: " << infoLog << std::endl;
			exit(2);
		}
	}
};