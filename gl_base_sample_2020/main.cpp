#include <GLAD/glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>
#include "shader.h"
#include "camera.h"
#include "LiteMath.h"
#include <iostream>
using namespace LiteMath;
#define STB_IMAGE_IMPLEMENTATION
#define GLFW_DLL
#define PI 3.14159265
#include "stb_image.h"
#include "common.h"


void OnKeyboardPressed(GLFWwindow* window, int key, int scancode, int action, int mode);
void OnMouseMove(GLFWwindow* window, double xpos, double ypos);
void OnMouseButtonClicked(GLFWwindow* window, int button, int action, int mods);
void OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset);
void doCameraMovement(Camera& camera, GLfloat deltaTime);
unsigned int loadTexture(const char* path, bool gammaCorrection);

unsigned int SCR_WIDTH = 600;
unsigned int SCR_HEIGHT = 600;
static int filling = 0;
static bool keys[1024];
static bool g_captureMouse = true;
static bool g_capturedMouseJustNow = false;

bool bloom = true;
bool bloomKeyPressed = false;
float exposure = 1.0f;

Camera camera(glm::vec3(0.0f, 5.0f, 5.0f));
float lastX = 400;
float lastY = 300;
bool firstMouse = true;

float timer = 0.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLsizei CreateSphere(float radius, int numberSlices, GLuint& vao)
{
    int i, j;

    int numberParallels = numberSlices;
    int numberVertices = (numberParallels + 1) * (numberSlices + 1);
    int numberIndices = numberParallels * numberSlices * 3;

    float angleStep = (2.0f * 3.14159265358979323846f) / ((float)numberSlices);

    std::vector<float> pos(numberVertices * 4, 0.0f);
    std::vector<float> norm(numberVertices * 4, 0.0f);
    std::vector<float> texcoords(numberVertices * 2, 0.0f);

    std::vector<int> indices(numberIndices, -1);

    for (i = 0; i < numberParallels + 1; i++)
    {
        for (j = 0; j < numberSlices + 1; j++)
        {
            int vertexIndex = (i * (numberSlices + 1) + j) * 4;
            int normalIndex = (i * (numberSlices + 1) + j) * 4;
            int texCoordsIndex = (i * (numberSlices + 1) + j) * 2;

            pos.at(vertexIndex + 0) = radius * sinf(angleStep * (float)i) * sinf(angleStep * (float)j);
            pos.at(vertexIndex + 1) = radius * cosf(angleStep * (float)i);
            pos.at(vertexIndex + 2) = radius * sinf(angleStep * (float)i) * cosf(angleStep * (float)j);
            pos.at(vertexIndex + 3) = 1.0f;

            norm.at(normalIndex + 0) = pos.at(vertexIndex + 0) / radius;
            norm.at(normalIndex + 1) = pos.at(vertexIndex + 1) / radius;
            norm.at(normalIndex + 2) = pos.at(vertexIndex + 2) / radius;
            norm.at(normalIndex + 3) = 1.0f;

            texcoords.at(texCoordsIndex + 0) = (float)j / (float)numberSlices;
            texcoords.at(texCoordsIndex + 1) = (1.0f - (float)i) / (float)(numberParallels - 1);
        }
    }

    int* indexBuf = &indices[0];

    for (i = 0; i < numberParallels; i++)
    {
        for (j = 0; j < numberSlices; j++)
        {
            *indexBuf++ = i * (numberSlices + 1) + j;
            *indexBuf++ = (i + 1) * (numberSlices + 1) + j;
            *indexBuf++ = (i + 1) * (numberSlices + 1) + (j + 1);

            *indexBuf++ = i * (numberSlices + 1) + j;
            *indexBuf++ = (i + 1) * (numberSlices + 1) + (j + 1);
            *indexBuf++ = i * (numberSlices + 1) + (j + 1);

            int diff = int(indexBuf - &indices[0]);
            if (diff >= numberIndices)
                break;
        }
        int diff = int(indexBuf - &indices[0]);
        if (diff >= numberIndices)
            break;
    }

    GLuint vboVertices, vboIndices, vboNormals, vboTexCoords;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);

    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(GLfloat), &pos[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, norm.size() * sizeof(GLfloat), &norm[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &vboTexCoords);
    glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords);
    glBufferData(GL_ARRAY_BUFFER, texcoords.size() * sizeof(GLfloat), &texcoords[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);

    glBindVertexArray(0);

    return indices.size();
}



GLsizei CreateConus(GLuint& vao, float4 point, float height, int grani)
{
    float angle = 360 / grani; //в градусах
    float radius = 1.0f;
    std::vector<float> positions =
    {
        point.x,  point.y,  point.z,  point.w, //центр
        point.x, point.y + height ,  point.z  , point.w, //вершина
    };

    std::vector<float> normals =
    {
        0.0f, 0.0f, 0.1f, 1.0f,
        0.0f, 1.0f, 1.0f, 1.0f,
    };

    std::vector<uint32_t> indices;

    for (int i = 1; i <= grani + 1; i++)
    {
        positions.push_back(cos(angle * i * PI / 180));  // x
        positions.push_back(0.0f); // y
        positions.push_back(sin(angle * i * PI / 180));  // z
        positions.push_back(1.0f);

        normals.push_back(positions.at(i + 0) / grani);
        normals.push_back(positions.at(i + 1) / grani);
        normals.push_back(positions.at(i + 2) / grani);
        normals.push_back(1.0f);

        // основание
        indices.push_back(0);
        indices.push_back(i);
        if ((i - 1) == grani) {
            indices.push_back(2);
        }
        else {
            indices.push_back(i + 1);
        }

        // шапка
        indices.push_back(1);
        indices.push_back(i);
        if ((i - 1) == grani) {
            indices.push_back(2);
        }
        else {
            indices.push_back(i + 1);
        }
    }
    GLuint vboVertices, vboIndices, vboNormals;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboIndices);
    glBindVertexArray(vao);
    glGenBuffers(1, &vboVertices);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vboNormals);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    return indices.size();
}

unsigned int cubeVAO;
unsigned int cubeVBO;
void Parallel()
{
    float vertices[] = {

       -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
        1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
        1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
       -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
       -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,


       -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
        1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
        1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
       -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
       -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

       -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
       -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
       -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
       -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
       -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
       -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

        1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,

       -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
        1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
        1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
       -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
       -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

       -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
        1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
        1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
        1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
       -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
       -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f
    };
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);


    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}


GLsizei CreateTriangle(GLuint& vao)
{

    std::vector<GLfloat> vertices_vec = { -0.5f, -0.5f, +0.0f,
                                         +0.5f, -0.5f, +0.0f,
                                         +0.0f, +0.5f, +0.0f };

    std::vector<GLfloat> normals_vec = { 0.0f, 0.0f, 1.0f,
                                        0.0f, 0.0f, 1.0f,
                                        0.0f, 0.0f, 1.0f };

    std::vector<GLfloat> texcoords_vec = { 0.0f, 0.0f,
                                          1.0f, 0.0f,
                                          0.5f, 0.5f };

    std::vector<GLuint> indices_vec = { 0, 1, 2 };


    GLuint vboVertices, vboIndices, vboNormals, vboTexCoords;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vboVertices);
    glGenBuffers(1, &vboIndices);
    glGenBuffers(1, &vboNormals);
    glGenBuffers(1, &vboTexCoords);

    glBindVertexArray(vao);
    {
        //передаем в шейдерную программу атрибут координат вершин
        glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
        glBufferData(GL_ARRAY_BUFFER, vertices_vec.size() * sizeof(GLfloat), &vertices_vec[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
        glEnableVertexAttribArray(0);

        //передаем в шейдерную программу атрибут нормалей
        glBindBuffer(GL_ARRAY_BUFFER, vboNormals);
        glBufferData(GL_ARRAY_BUFFER, normals_vec.size() * sizeof(GLfloat), &normals_vec[0], GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
        glEnableVertexAttribArray(1);

        //передаем в шейдерную программу атрибут текстурных координат
        glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords);
        glBufferData(GL_ARRAY_BUFFER, texcoords_vec.size() * sizeof(GLfloat), &texcoords_vec[0], GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
        glEnableVertexAttribArray(2);

        //передаем в шейдерную программу индексы
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_vec.size() * sizeof(GLuint), &indices_vec[0], GL_STATIC_DRAW);

    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    return indices_vec.size();
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, GL_FALSE);

   

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    

   
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL basic sample", nullptr, nullptr);
    

    glfwMakeContextCurrent(window);


    glfwSetKeyCallback(window, OnKeyboardPressed);
    glfwSetCursorPosCallback(window, OnMouseMove);
    glfwSetMouseButtonCallback(window, OnMouseButtonClicked);
    glfwSetScrollCallback(window, OnMouseScroll);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    glEnable(GL_DEPTH_TEST);

    Shader shader("../shader.vs", "../shader.fs");
    Shader skyboxShader("../skybox.vs", "../skybox.fs");
    Shader shaderLight("../shader.vs", "../light.fs");

  



    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    std::vector<std::string> faces
    {
       "../posx.jpg", "../negx.jpg", "../posy.jpg", "../negy.jpg", "../posz.jpg", "../negz.jpg"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    unsigned int samTexture = loadTexture("../samolet.jpg", true);
    unsigned int elkaTexture = loadTexture("../elka.jpg", true);

    std::vector<glm::vec3> lightPositions;
    lightPositions.push_back(glm::vec3(1.5f, 5.5f, 3.0f));
    lightPositions.push_back(glm::vec3(8.0f, 5.0f, 2.5f));
    lightPositions.push_back(glm::vec3(-4.5f, 6.5f, -1.5f));
    lightPositions.push_back(glm::vec3(5.0f, 10.0f, 0.0f));

    std::vector<glm::vec3> lightColors;
    lightColors.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
    lightColors.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
    lightColors.push_back(glm::vec3(1.0f, 1.0f, 1.0f));
    lightColors.push_back(glm::vec3(1.0f, 1.0f, 1.0f));


    shader.use();
    shader.setInt("material.diffuse", 0);
    shader.setInt("material.specular", 1);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);



    GLuint vaoConus;
    GLsizei coneIndices = CreateConus(vaoConus, float4(0.0f, 3.0f, 0.0f, 1.0f), 3.0f, 8);


    GLuint samolet;
    GLsizei samolet__ = CreateSphere(0.246f, 50, samolet);



    GLuint vaoSphereLight1;
    GLsizei SphereLight1Indices = CreateSphere(0.03f, 30, vaoSphereLight1);

    GLuint vaoSphereLight2;
    GLsizei SphereLight2Indices = CreateSphere(0.06f, 30, vaoSphereLight2);

    GLuint vaoSphereLight3;
    GLsizei SphereLight3Indices = CreateSphere(0.05f, 30, vaoSphereLight3);

    GLuint vaoSphereLight4;
    GLsizei SphereLight4Indices = CreateSphere(0.07f, 30, vaoSphereLight4);


   


    while (!glfwWindowShouldClose(window))
    {
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        doCameraMovement(camera, deltaTime);

        glClearColor(0.4f, 0.25f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        shader.use();
        shader.setVec3("viewPos", camera.Position);
        shader.setFloat("material.shininess", 64.0f);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);


        shader.setVec3("dirLight.direction", 0.0f, -5.0f, 0.0f);
        shader.setVec3("dirLight.ambient", 0.6f, 0.6f, 0.6f);
        shader.setVec3("dirLight.color", 1.0f, 1.0f, 1.0f);
        shader.setVec3("dirLight.specular", 1.0f, 1.0f, 1.0f);

        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            shader.setVec3("lights[" + std::to_string(i) + "].Position", lightPositions[i]);
            shader.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
            shader.setVec3("lights[" + std::to_string(i) + "].ambient", 0.5f, 0.5f, 0.5f);
            shader.setFloat("lights[" + std::to_string(i) + "].constant", 1.0f);
            shader.setFloat("lights[" + std::to_string(i) + "].linear", 0.09);
            shader.setFloat("lights[" + std::to_string(i) + "].quadratic", 0.032);
        }
        shader.setVec3("viewPos", camera.Position);


      
        glBindTexture(GL_TEXTURE_2D, elkaTexture);
        glBindVertexArray(vaoConus);
        {
            for (int i = 1; i < 50; i++) {
                
                model = glm::rotate(glm::mat4(1.0f), (i * 6 * 2.5f) * LiteMath::DEG_TO_RAD, glm::vec3(0.0, 1.0, 0.0));
                model = glm::translate(model, glm::vec3((i * 1.0f), -5.0f, +10.0f)); // пропорции
                model = glm::scale(model, glm::vec3(1.0f, 0.2f, 0.5f));
                shader.setMat4("model", model);
                               
                glDrawElements(GL_TRIANGLE_STRIP, coneIndices, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
            }
        }

      
        glBindTexture(GL_TEXTURE_2D, samTexture);
        glBindVertexArray(samolet);
        {
           
             

           model = glm::mat4(1.0f);
            model = glm::rotate(glm::mat4(1.0f), timer * 0.54f * ( float(PI) / 180.0f), glm::vec3(0.0, 1.0, 0.0));
            model = glm::translate(model, glm::vec3(15.5f, 20.0f, 0.0f));
            model = glm::scale(model, glm::vec3(5 * 1.0f, 5 * 1.0f, 5 * 5.0f));
            shader.setMat4("model", model);
            glDrawElements(GL_TRIANGLE_STRIP, samolet__, GL_UNSIGNED_INT, nullptr);



            model = glm::mat4(1.0f);
            model = glm::rotate(glm::mat4(1.0f), timer * 0.54f * (float(PI) / 180.0f), glm::vec3(0.0, 1.0, 0.0));
            model = glm::translate(model, glm::vec3(15.5f, 20.0f, 0.0f));
            model = glm::scale(model, glm::vec3(5 * 4.0f, 5 * 0.5f, 5 * 1.0f));
            shader.setMat4("model", model);
            glDrawElements(GL_TRIANGLE_STRIP, samolet__, GL_UNSIGNED_INT, nullptr);

           /* model = transpose(mul(rotate_X_4x4(timer), (mul(translate4x4(float3(3.0f + sin(timer), 95.0f + cos(timer / 2), 2.0f)), scale4x4(float3(5 * 4.0f, 5 * 0.5f, 5 * 1.0f))))));
            lambert.SetUniform("model", model); GL_CHECK_ERRORS;
            glDrawElements(GL_TRIANGLE_STRIP, samolet__, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;



            model = transpose(mul(rotate_X_4x4(timer), (mul(translate4x4(float3(1.0f + sin(timer), 95.0f + cos(timer / 2), 2.0f)), scale4x4(float3(5 * 4.0f, 5 * 0.5f, 5 * 1.0f))))));
            lambert.SetUniform("model", model); GL_CHECK_ERRORS;
            glDrawElements(GL_TRIANGLE_STRIP, samolet__, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;*/




        }




      

        shaderLight.use();
        shaderLight.setMat4("projection", projection);
        shaderLight.setMat4("view", view);
        glBindVertexArray(vaoSphereLight1);
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(lightPositions[0]));
            shaderLight.setMat4("model", model);
            shaderLight.setVec3("lightColor", lightColors[0]);
            glDrawElements(GL_TRIANGLE_STRIP, SphereLight1Indices, GL_UNSIGNED_INT, nullptr);
        }

        glBindVertexArray(vaoSphereLight2);
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(lightPositions[1]));
            shaderLight.setMat4("model", model);
            shaderLight.setVec3("lightColor", lightColors[0]);
            glDrawElements(GL_TRIANGLE_STRIP, SphereLight2Indices, GL_UNSIGNED_INT, nullptr);
        }

        glBindVertexArray(vaoSphereLight3);
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(lightPositions[2]));
            shaderLight.setMat4("model", model);
            shaderLight.setVec3("lightColor", lightColors[2]);
            glDrawElements(GL_TRIANGLE_STRIP, SphereLight3Indices, GL_UNSIGNED_INT, nullptr);
        }
        glBindVertexArray(vaoSphereLight4);
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(lightPositions[3]));
            shaderLight.setMat4("model", model);
            shaderLight.setVec3("lightColor", lightColors[3]);
            glDrawElements(GL_TRIANGLE_STRIP, SphereLight4Indices, GL_UNSIGNED_INT, nullptr);
        }


        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);


        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        timer += 0.3;
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteVertexArrays(1, &vaoConus);
   

    glfwTerminate();
    return 0;
}


void OnKeyboardPressed(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    switch (key)
    {
    case GLFW_KEY_ESCAPE:
        if (action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    case GLFW_KEY_SPACE:
        if (action == GLFW_PRESS)
        {
            bloom = !bloom;
            bloomKeyPressed = true;
        }
        break;
    case GLFW_KEY_1:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        break;
    case GLFW_KEY_2:
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;
    default:
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}


void OnMouseButtonClicked(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        g_captureMouse = !g_captureMouse;


    if (g_captureMouse)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        g_capturedMouseJustNow = true;
    }
    else
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}


void OnMouseMove(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = float(xpos);
        lastY = float(ypos);
        firstMouse = false;
    }

    GLfloat xoffset = float(xpos) - lastX;
    GLfloat yoffset = lastY - float(ypos);

    lastX = float(xpos);
    lastY = float(ypos);

    if (g_captureMouse)
        camera.ProcessMouseMovement(xoffset, yoffset);
}


void OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(GLfloat(yoffset));
}

void doCameraMovement(Camera& camera, GLfloat deltaTime)
{
    if (keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

//функция для загрузки тектуры
unsigned int loadTexture(char const* path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);//glGenTextures() в качестве входных данных принимает количество текстур, которые мы хотим создать, и сохраняет их в массиве типа unsigned int

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        //генерация текстуры
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}