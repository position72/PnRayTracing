#pragma once
#include "PnRT.hpp"

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

	void setUInt(const GLchar* name, const GLuint& num) const {
		glUniform1ui(glGetUniformLocation(program, name), num);
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

inline void LoadHDRImage(const char* path, const Shader& shader) {
	shader.use();
	shader.setInt("HasHDRImage", 0);
	int width, height, comp;
	unsigned int hdrTexture;
	float* hdrImage = stbi_loadf(path, &width, &height, &comp, 0);
	if (!hdrImage) {
		std::cout << "Failed to load HDR image: " << path << std::endl;
	} else {
		glGenTextures(1, &hdrTexture);
		glActiveTexture(GL_TEXTURE29);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, hdrImage);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		std::vector<std::vector<float>> pdf;
		pdf.resize(width);
		float pdfSum = 0.0;
		for (int x = 0; x < width; ++x) {
			pdf[x].resize(height);
			for (int y = 0; y < height; ++y) {
				int pos = y * width + x;
				// R * 0.2 + G * 0.7 + B * 0.1
				float lumen = hdrImage[pos * 3 + 0] * 0.2 + hdrImage[pos * 3 + 1] * 0.7 + hdrImage[pos * 3 + 2] * 0.1;
				pdf[x][y] = lumen;
				pdfSum += lumen;
			}
		}
		// 横向位置变量x的概率边缘密度函数
		std::vector<float> cdfMarginX, pdfMarginX;
		cdfMarginX.resize(width);
		pdfMarginX.resize(width);
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				pdf[x][y] /= pdfSum; // 归一化
				pdfMarginX[x] += pdf[x][y];
			}
		}
		cdfMarginX[0] = pdfMarginX[0];
		for (int x = 1; x < width; ++x) {
			cdfMarginX[x] = cdfMarginX[x - 1] + pdfMarginX[x];
		}
		// 选定x条件下对于y变量的条件概率密度函数
		std::vector<std::vector<float>> cdfYConditionX;
		cdfYConditionX.resize(width);
		for (int x = 0; x < width; ++x) {
			cdfYConditionX[x].resize(height);
			for (int y = 0; y < height; ++y) {
				cdfYConditionX[x][y] = (y > 0 ? cdfYConditionX[x][y - 1] : 0.0)
					+ pdf[x][y] / pdfMarginX[x];
			}
		}

		float* randomHDR = new float[width * height * 3];
		// 当着色器随机出两个数r1 = i / width, r2 = j / height时，在CDF逆函数查找对应的x, y
		// randomHDR[pos]存储x, y, 
		for (int i = 0; i < width; ++i) {
			for (int j = 0; j < height; ++j) {
				float r1 = Rand0To1(), r2 = Rand0To1();
				int x = (int)(std::lower_bound(cdfMarginX.begin(), cdfMarginX.end(),
					(float)i / width) - cdfMarginX.begin());
				if (x >= width) x = width - 1;
				if (x < 0) x = 0;
				int y = (int)(std::lower_bound(cdfYConditionX[x].begin(), cdfYConditionX[x].end(),
					(float)j / height) - cdfYConditionX[x].begin());
				if (y >= height) y = height - 1;
				if (y < 0) y = 0;
				int pos = 3 * (j * width + i);
				
				randomHDR[pos + 0] = (float)x / width;
				randomHDR[pos + 1] = (float)y / height;
				randomHDR[pos + 2] = pdf[x][y];
			}
		}

		unsigned int rH;
		glGenTextures(1, &rH);
		glActiveTexture(GL_TEXTURE30);
		glBindTexture(GL_TEXTURE_2D, rH);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, randomHDR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		shader.setInt("HDRImageWidth", width);
		shader.setInt("HDRImageHeight", height);
		shader.setInt("HasHDRImage", 1);

		std::cout << "Success to load HDR Image: " << path << std::endl;

		stbi_image_free(hdrImage);
		delete[] randomHDR;
	}
}