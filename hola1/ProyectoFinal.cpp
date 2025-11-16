#include <iostream>
#include <string>
#include <cstdlib> // Para rand()
#include <ctime>   // Para time()

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Shaders, Modelos y Texturas
#include "Shader.h"
#include "Model.h"

// Implementación de STB_IMAGE para cargar texturas
// Se define aquí para que se compile en este archivo .cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // (El comentario en la línea 207 estaba mal escrito)


// Prototipos de funciones
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void Animacion();


// --- Definición de la Clase Camera ---
// (Movida desde Camera.h para facilitar la edición en un solo archivo)

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// Valores por defecto de la cámara
const GLfloat YAW = -90.0f;
const GLfloat PITCH = 0.0f;
const GLfloat SPEED = 20.0f; // Velocidad de movimiento
const GLfloat SENSITIVITY = 0.25f;
const GLfloat ZOOM = 45.0f;

class Camera
{
public:
    // Atributos
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // Ángulos de Euler
    GLfloat Yaw;
    GLfloat Pitch;
    // Opciones
    GLfloat MovementSpeed;
    GLfloat MouseSensitivity;
    GLfloat Zoom;

    // Constructor
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), GLfloat yaw = YAW, GLfloat pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        this->Position = position;
        this->WorldUp = up;
        this->Yaw = yaw;
        this->Pitch = pitch;
        this->updateCameraVectors();
    }

    // Devuelve la matriz de vista (View Matrix)
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(this->Position, this->Position + this->Front, this->Up);
    }

    // Devuelve el valor del zoom
    GLfloat GetZoom() {
        return this->Zoom;
    }

    // Procesa el movimiento del teclado
    void ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime)
    {
        GLfloat velocity = this->MovementSpeed * deltaTime;
        if (direction == FORWARD)
            this->Position += this->Front * velocity;
        if (direction == BACKWARD)
            this->Position -= this->Front * velocity;
        if (direction == LEFT)
            this->Position -= this->Right * velocity;
        if (direction == RIGHT)
            this->Position += this->Right * velocity;
        if (direction == UP)
            this->Position += this->WorldUp * velocity;
        if (direction == DOWN)
            this->Position -= this->WorldUp * velocity;
    }

    // Procesa el movimiento del mouse
    void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= this->MouseSensitivity;
        yoffset *= this->MouseSensitivity;

        this->Yaw += xoffset;
        this->Pitch += yoffset;

        // Limita el ángulo de Pitch (para no dar la vuelta)
        if (constrainPitch)
        {
            if (this->Pitch > 89.0f)
                this->Pitch = 89.0f;
            if (this->Pitch < -89.0f)
                this->Pitch = -89.0f;
        }

        this->updateCameraVectors();
    }

private:
    // Recalcula los vectores de la cámara (Front, Right, Up)
    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
        front.y = sin(glm::radians(this->Pitch));
        front.z = sin(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
        this->Front = glm::normalize(front);

        this->Right = glm::normalize(glm::cross(this->Front, this->WorldUp));
        this->Up = glm::normalize(glm::cross(this->Right, this->Front));
    }
};

// --- Variables Globales ---

// Dimensiones de la ventana
const GLint WIDTH = 1200, HEIGHT = 800;
int screenWidth, screenHeight; // Se obtendrán del framebuffer

// Cámara
Camera camera(glm::vec3(0.0f, 15.0f, 55.0f)); // Posición inicial
bool keys[1024];
GLfloat lastX = WIDTH / 2.0f;
GLfloat lastY = HEIGHT / 2.0f;
bool firstMouse = true;

// Control de Tiempo (DeltaTime)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Ciclo Día/Tarde/Noche
glm::vec3 dayColor(0.5f, 0.8f, 1.0f);
glm::vec3 nightColor(0.196f, 0.196f, 0.274f);
glm::vec3 sunsetColor(1.0f, 0.686f, 0.0f);
glm::vec3 currentColor = dayColor;
glm::vec3 targetColor = dayColor;
glm::vec3 startTransitionColor = dayColor;
bool isTransitioning = false;
bool isNight = false;
bool isSunset = false;
float transitionFactor = 0.0f;
float transitionSpeed = 0.5f; // Velocidad de la transición

// Animación de Ho-oh
glm::vec3 hoohPos = glm::vec3(40.0f, 30.0f, 0.0f);
glm::vec3 hoohTargetPos = glm::vec3(40.0f, 30.0f, 0.0f);
float hoohSpeed = 10.0f;
float hoohMinY = 60.0f; // Altura mínima de vuelo (aumentada)
glm::vec3 hoohDirection = glm::vec3(0.0f, 0.0f, 1.0f);
float hoohCurveSpeed = 0.5f;
float hoohCurveAmount = 1.0f;


// --- Variables de Animación de MEW ---
glm::vec3 mewPos = glm::vec3(0.0f, 2.0f, 0.0f); // Posición inicial de Mew
glm::vec3 mewTargetPos = glm::vec3(0.0f, 2.0f, 0.0f); // Objetivo actual de Mew
glm::vec3 mewDirection = glm::vec3(0.0f, 0.0f, 0.0f); // Dirección actual de Mew

GLfloat mewSpeed = 2.70f; // Velocidad de movimiento de Mew
GLfloat mewCurveSpeed = 2.70f; // Velocidad de la curva (más alto = más errático)
GLfloat mewCurveAmount = 2.70f; // Cantidad de la curva (más alto = curvas más cerradas)
GLfloat mewMinY = 0.5f; // Altura mínima de vuelo

int main() {
    glfwInit();

    // Configuración de la ventana (OpenGL 3.3 Core)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Proyecto Final", nullptr, nullptr);
    if (nullptr == window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwGetFramebufferSize(window, &screenWidth, &screenHeight);
    glfwMakeContextCurrent(window);

    // Configurar callbacks de Teclado y Mouse
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);

    // Capturar el cursor del mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Inicializar GLEW
    glewExperimental = GL_TRUE;
    if (GLEW_OK != glewInit()) {
        std::cout << "Failed to initialise GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    // Configuración del Viewport y OpenGL
    glViewport(0, 0, screenWidth, screenHeight);
    glEnable(GL_DEPTH_TEST); // Activar pruebas de profundidad
    glEnable(GL_BLEND);      // Activar transparencias

    // Inicializar la semilla para números aleatorios
    srand(static_cast<unsigned int>(time(NULL)));

    // --- Compilar Shaders ---
    Shader ourShader("Shader/core.vs", "Shader/core.frag");
    Shader lampShader("Shader/lamp1.vs", "Shader/lamp1.frag");
    Shader modelShader("Shader/modelLoading.vs", "Shader/modelLoading.frag");


    // --- Cargar Textura de Césped (grassTextureID) ---
    GLuint grassTextureID;
    glGenTextures(1, &grassTextureID);
    glBindTexture(GL_TEXTURE_2D, grassTextureID);

    // Configurar wrapping (repetir)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Configurar filtrado (pixelado)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Cargar la imagen
    int textureWidth, textureHeight, nrChannels;
    unsigned char* image = stbi_load("images/pasto.png", &textureWidth, &textureHeight, &nrChannels, 0);

    if (image)
    {
        // Detectar formato (RGB o RGBA)
        GLenum format;
        if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA; // Soporta transparencia
        else
            format = GL_RGB; // Por defecto

        glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture: images/pasto.png" << std::endl;
    }
    stbi_image_free(image); // Liberar memoria de la imagen
    glBindTexture(GL_TEXTURE_2D, 0); // Desenlazar


    // --- Cargar Textura de Agua (waterTextureID) ---
    GLuint waterTextureID;
    GLuint waterTextureID_2;
    GLuint hojasTextureID;
    GLuint tejadoTextureID;
    glGenTextures(1, &waterTextureID);
    glBindTexture(GL_TEXTURE_2D, waterTextureID);

    // --- Cargar Textura de Agua 2 (waterTextureID_2) ---
    glGenTextures(1, &waterTextureID_2); // <- CAMBIO AQUÍ
    glBindTexture(GL_TEXTURE_2D, waterTextureID_2); // <- CAMBIO AQUÍ

    // Configurar wrapping (repetir)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Configurar filtrado (pixelado)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Cargar la imagen
    unsigned char* agua2 = stbi_load("images/agua2.png", &textureWidth, &textureHeight, &nrChannels, 0); // <- CAMBIO AQUÍ

    if (agua2) // <- CAMBIO AQUÍ
    {
        // Detectar formato
        GLenum format;
        if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else
            format = GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, agua2); // <- CAMBIO AQUÍ
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture: images/agua2.png" << std::endl; // <- CAMBIO AQUÍ
    }
    stbi_image_free(agua2); // <- CAMBIO AQUÍ
    glBindTexture(GL_TEXTURE_2D, 0); // Desenlazar

    glBindTexture(GL_TEXTURE_2D, waterTextureID);
    // Configurar wrapping (repetir)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Configurar filtrado (pixelado)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Cargar la imagen
    unsigned char* agua = stbi_load("images/agua.png", &textureWidth, &textureHeight, &nrChannels, 0);

    if (agua)
    {
        // Detectar formato
        GLenum format;
        if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else
            format = GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, agua);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture: images/agua.png" << std::endl;
    }
    stbi_image_free(agua); // Liberar memoria de la imagen
    glBindTexture(GL_TEXTURE_2D, 0); // Desenlazar

    // --- Cargar Textura de Hojas (hojasTextureID) ---
    glGenTextures(1, &hojasTextureID);
    glBindTexture(GL_TEXTURE_2D, hojasTextureID);
    // Configurar wrapping (repetir)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Configurar filtrado (pixelado)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Cargar la imagen
    image = stbi_load("images/hojas.jpg", &textureWidth, &textureHeight, &nrChannels, 0);
    if (image)
    {
        // Detectar formato
        GLenum format;
        if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else
            format = GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture: images/hojas.jpg" << std::endl;
    }
    stbi_image_free(image); // Liberar memoria de la imagen
    glBindTexture(GL_TEXTURE_2D, 0); // Desenlazar

    // --- Cargar Textura de Tejado (tejadoTextureID) ---
    glGenTextures(1, &tejadoTextureID);
    glBindTexture(GL_TEXTURE_2D, tejadoTextureID);
    // Configurar wrapping (repetir)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Configurar filtrado (pixelado)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Cargar la imagen
    image = stbi_load("images/tejado.png", &textureWidth, &textureHeight, &nrChannels, 0);
    if (image)
    {
        // Detectar formato
        GLenum format;
        if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else
            format = GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture: images/tejado.png" << std::endl;
    }
    stbi_image_free(image); // Liberar memoria de la imagen
    glBindTexture(GL_TEXTURE_2D, 0); // Desenlazar

    // --- Definición de Vértices ---

    // Vértices del Cubo (Posición + Coords. de Textura)
    // Usado para casas, muebles, árboles, pasto, etc.
    float vertices[] = {
        // Posición           // Normales            // Coords. Textura
        // Cara frontal (Z+)
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
        // Cara trasera (Z-)
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
        // Cara derecha (X+)
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         // Cara izquierda (X-)
        -0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        // Cara inferior (Y-)
       -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
       -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
       -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
       // Cara superior (Y+)
      -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
       0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
       0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
       0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
      -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
      -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f
    };

    // Vértices del Agua (Cubo con Coords. de Textura 2x2 en la cara superior)
    float vertices_water[] = {
        // (Caras frontal, trasera, izq, der, inferior... con normales)
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f, -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f, -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f, 0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f, 0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f, 0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f, -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f, -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f, -0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f, -0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, -0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f, -0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,  0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,  0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,  0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f, -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f, -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        // Cara superior (AGUA) - Coordenadas 2x2
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 2.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   2.0f, 2.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,   2.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,   2.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   0.0f, 2.0f
    };

    // Vértices del Triángulo (para rellenar el hueco del techo)
    float roof_gap_vertices[] = {
        // Posición            // Normales            // Coords. Textura
         0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.5f, 1.0f,
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f
    };

    // --- Configuración de VAO y VBO para el Cubo (VAO) ---
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Atributo de Posición (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Atributo de Normal (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // Atributo de Coordenada de Textura (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- Configuración de VAO y VBO para el Agua (VAO_water) ---
    GLuint VBO_water, VAO_water;
    glGenVertexArrays(1, &VAO_water);
    glGenBuffers(1, &VBO_water);

    glBindVertexArray(VAO_water);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_water);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_water), vertices_water, GL_STATIC_DRAW);

    // Atributo de Posición (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // Atributo de Normal (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // Atributo de Coordenada de Textura (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- Configuración de VAO y VBO para el Triángulo del Techo (VAO_gap) ---
    GLuint VBO_gap, VAO_gap;
    glGenVertexArrays(1, &VAO_gap);
    glGenBuffers(1, &VBO_gap);

    glBindVertexArray(VAO_gap);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_gap);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roof_gap_vertices), roof_gap_vertices, GL_STATIC_DRAW);

    // Atributos (igual que el cubo)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- Cargar Modelos 3D ---
    Model mewModel((char*)"Models/Mew.obj");
    Model hoohModel("Models/ho-oh/Ho-Oh/hooh.dae");

    // --- Paleta de Colores ---
    glm::vec3 floorColor(0.85f, 0.75f, 0.5f);
    glm::vec3 greenRugColor(0.2f, 0.6f, 0.3f);
    glm::vec3 whiteColor(1.0f, 1.0f, 1.0f);
    glm::vec3 redRugColor(0.8f, 0.2f, 0.2f);
    glm::vec3 tableColor(1.0f, 0.9f, 0.3f);
    glm::vec3 tableTopColor(0.2f, 0.5f, 0.8f);
    glm::vec3 chairColor(0.5f, 0.7f, 0.9f);
    glm::vec3 potColor(0.8f, 0.5f, 0.2f);
    glm::vec3 plantColor(0.1f, 0.7f, 0.2f);
    glm::vec3 counterColor(0.9f, 0.9f, 0.9f);
    glm::vec3 stairsColor(0.9f, 0.8f, 0.6f);
    glm::vec3 wallColor(0.9f, 0.85f, 0.7f);
    glm::vec3 bedBlanketColor(0.2f, 0.8f, 0.4f);
    glm::vec3 deskColor(0.8f, 0.6f, 0.4f);
    glm::vec3 bookshelfColor(0.3f, 0.7f, 0.6f);
    glm::vec3 electronicsColor(0.5f, 0.5f, 0.5f);
    glm::vec3 pcColor(0.85f, 0.85f, 0.8f);
    glm::vec3 facadeColor(0.9f, 0.9f, 0.95f);
    glm::vec3 roofColor(0.8f, 0.3f, 0.2f);
    glm::vec3 doorColor(0.6f, 0.4f, 0.2f);
    glm::vec3 windowColor(0.5f, 0.7f, 0.9f);
    glm::vec3 grassColor(0.4f, 0.75f, 0.4f);
    glm::vec3 treeTrunkColor(0.5f, 0.35f, 0.05f);
    glm::vec3 treeLeavesColor(0.1f, 0.5f, 0.1f);
    glm::vec3 waterColor(0.3f, 0.5f, 0.95f);
    glm::vec3 labWallColor(0.9f, 0.88f, 0.7f);
    glm::vec3 labRoofColor(0.5f, 0.5f, 0.6f);
    glm::vec3 labAccentColor(0.8f, 0.4f, 0.3f);
    glm::vec3 blackColor(0.1f, 0.1f, 0.1f);
    glm::vec3 lightGreyColor(0.75f, 0.75f, 0.75f);
    glm::vec3 postColor(0.2f, 0.2f, 0.2f);
    glm::vec3 lightColor(1.0f, 1.0f, 0.7f);

    // --- Bucle principal de renderizado ---
    while (!glfwWindowShouldClose(window))
    {
        // Calcular delta time (tiempo entre frames)
        GLfloat currentFrame = (GLfloat)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Revisar eventos (teclado, mouse)
        glfwPollEvents();
        DoMovement(); // Procesar movimiento de teclado
        Animacion();  // Actualizar animación de Ho-oh

        // ===============================================================
        //     PASO 1: CONFIGURAR LA ILUMINACIÓN GLOBAL
        // ===============================================================

        // Activa el shader de iluminación principal (modelShader)
        modelShader.Use();

        // --- Matrices de Cámara ---
        glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)screenWidth / (GLfloat)screenHeight, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(modelShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(modelShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // --- Posición del Espectador (Cámara) ---
        glUniform3fv(glGetUniformLocation(modelShader.Program, "viewPos"), 1, &camera.Position[0]);

        // --- ☀️ EL SOL (Luz Direccional) ---
        glUniform3f(glGetUniformLocation(modelShader.Program, "dirLight.direction"), -0.707f, -0.707f, 0.0f);
        glUniform3f(glGetUniformLocation(modelShader.Program, "dirLight.ambient"), 0.2f, 0.2f, 0.2f);
        glUniform3f(glGetUniformLocation(modelShader.Program, "dirLight.diffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(modelShader.Program, "dirLight.specular"), 1.0f, 1.0f, 1.0f);

        // --- 💡 LUCES DE POSTE (Punto) ---
        // Poste 1 (Derecha)
        glUniform3f(glGetUniformLocation(modelShader.Program, "pointLights[0].position"), 30.0f, 6.5f, 2.9f);
        glUniform3f(glGetUniformLocation(modelShader.Program, "pointLights[0].ambient"), 0.05f, 0.05f, 0.0f);
        glUniform3f(glGetUniformLocation(modelShader.Program, "pointLights[0].diffuse"), 0.8f, 0.8f, 0.6f); // Luz amarilla
        glUniform3f(glGetUniformLocation(modelShader.Program, "pointLights[0].specular"), 1.0f, 1.0f, 0.8f);
        glUniform1f(glGetUniformLocation(modelShader.Program, "pointLights[0].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(modelShader.Program, "pointLights[0].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(modelShader.Program, "pointLights[0].quadratic"), 0.032f);

        // Poste 2 (Izquierda)
        glUniform3f(glGetUniformLocation(modelShader.Program, "pointLights[1].position"), -9.5f, 6.5f, 2.9f);
        glUniform3f(glGetUniformLocation(modelShader.Program, "pointLights[1].ambient"), 0.05f, 0.05f, 0.0f);
        glUniform3f(glGetUniformLocation(modelShader.Program, "pointLights[1].diffuse"), 0.8f, 0.8f, 0.6f); // Luz amarilla
        glUniform3f(glGetUniformLocation(modelShader.Program, "pointLights[1].specular"), 1.0f, 1.0f, 0.8f);
        glUniform1f(glGetUniformLocation(modelShader.Program, "pointLights[1].constant"), 1.0f);
        glUniform1f(glGetUniformLocation(modelShader.Program, "pointLights[1].linear"), 0.09f);
        glUniform1f(glGetUniformLocation(modelShader.Program, "pointLights[1].quadratic"), 0.032f);


        // --- Lógica de transición de color (Día/Noche) ---
        if (isTransitioning)
        {
            transitionFactor += transitionSpeed * deltaTime;
            transitionFactor = glm::clamp(transitionFactor, 0.0f, 1.0f);
            currentColor = glm::mix(startTransitionColor, targetColor, transitionFactor);

            if (transitionFactor >= 1.0f)
            {
                isTransitioning = false;
                // Actualiza el estado actual
                if (targetColor == dayColor) {
                    isNight = false;
                    isSunset = false;
                }
                else if (targetColor == nightColor) {
                    isNight = true;
                    isSunset = false;
                }
                else if (targetColor == sunsetColor) {
                    isNight = false;
                    isSunset = true;
                }
            }
        }

        // --- Limpiar pantalla ---
        glClearColor(currentColor.r, currentColor.g, currentColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // ===============================================================
        //     INICIO DEL DIBUJO DE OBJETOS SÓLIDOS (CASAS, ÁRBOLES, ETC.)
        // ===============================================================

        // Declaramos 'model' y 'modelLoc' UNA SOLA VEZ para todo el bucle de dibujo
        glm::mat4 model;
        GLint modelLoc = glGetUniformLocation(modelShader.Program, "model");

        // Configura el shader para objetos sólidos (SIN textura)
        glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 0); // 0 = NO usar textura
        glUniform3f(glGetUniformLocation(modelShader.Program, "material.specular"), 0.5f, 0.5f, 0.5f); // Brillo estándar
        glUniform1f(glGetUniformLocation(modelShader.Program, "material.shininess"), 32.0f);

        glBindVertexArray(VAO); // Enlaza el VAO del Cubo

        // ===============================================================
                //						ESCENARIO EXTERIOR (SIN CÉSPED)
                // ===============================================================

                // Árboles (Borde exterior)
        float treeSpacing = 5.0f;
        for (int i = -9; i <= 9; i++) {
            // Filas horizontales (arriba y abajo)
            for (float zPos : {-48.0f, 48.0f}) {
                bool skip = false;
                float currentX = i * treeSpacing;
                if (currentX > 30.0f && zPos == -48.0f) skip = true; // Omitir esquina sup-der
                if (currentX > -37.5f && currentX < -22.5f && zPos == 48.0f) skip = true; // Omitir zona estanque

                if (!skip) {
                    glm::mat4 treeModel = glm::translate(glm::mat4(1.0f), glm::vec3(i * treeSpacing, 0.0f, zPos));
                    // Tronco
                    model = glm::translate(treeModel, glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(1.0f, 3.0f, 1.0f));
                    glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(treeTrunkColor));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    // --- Hojas (AHORA CON TEXTURA) ---
                    // 1. Activar modo textura
                    glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 1);
                    // 2. Vincular la textura de hojas
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, hojasTextureID);
                    glUniform1i(glGetUniformLocation(modelShader.Program, "texture_diffuse1"), 0);

                    // 3. Dibujar
                    model = glm::translate(treeModel, glm::vec3(0.0f, 4.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
                    // (Ya no se usa glUniform3fv... para el color)
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // 4. VOLVER a modo color sólido para el siguiente tronco
                    glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 0);
                }
            }
        }
        for (int i = -9; i <= 9; i++) {
            // Filas verticales (izquierda y derecha)
            for (float xPos : {-48.0f, 48.0f}) {
                bool skip = false;
                float currentZ = i * treeSpacing;
                if (xPos == 48.0f && currentZ < -40.0f) skip = true; // Omitir esquina sup-der
                if (xPos == -48.0f && currentZ > 26.25f) skip = true; // Omitir zona estanque

                if (!skip) {
                    glm::mat4 treeModel = glm::translate(glm::mat4(1.0f), glm::vec3(xPos, 0.0f, i * treeSpacing));
                    // Tronco
                    model = glm::translate(treeModel, glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(1.0f, 3.0f, 1.0f));
                    glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(treeTrunkColor));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    // --- Hojas (AHORA CON TEXTURA) ---
                    // 1. Activar modo textura
                    glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 1);
                    // 2. Vincular la textura de hojas
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, hojasTextureID);
                    glUniform1i(glGetUniformLocation(modelShader.Program, "texture_diffuse1"), 0);

                    // 3. Dibujar
                    model = glm::translate(treeModel, glm::vec3(0.0f, 4.0f, 0.0f));
                    model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
                    // (Ya no se usa glUniform3fv... para el color)
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                    glDrawArrays(GL_TRIANGLES, 0, 36);

                    // 4. VOLVER a modo color sólido para el siguiente tronco
                    glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 0);
                }
            }
        }
        float plantPositions[2][2] = { {-8.0f, 8.0f}, {8.0f, 8.0f} };
        float chairPositions[4][3] = { {2.5f, 0.0f, 0.0f}, {-2.5f, 0.0f, 0.0f}, {0.0f, 0.0f, 2.0f}, {0.0f, 0.0f, -2.0f} };
        float chairRotations[4] = { -90.0f, 90.0f, 180.0f, 0.0f };

        // ===============================================================
        //						CASA DERECHA
        // ===============================================================
        {
            glm::mat4 houseBaseModel = glm::translate(glm::mat4(1.0f), glm::vec3(20.0f, 0.0f, -10.0f));
            // --- Piso ---
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(20.0f, 0.1f, 20.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(floorColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // --- Alfombra Verde (borde blanco) ---
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, -0.45f, 0.0f));
            model = glm::scale(model, glm::vec3(12.5f, 0.1f, 8.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(whiteColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, -0.4f, 0.0f));
            model = glm::scale(model, glm::vec3(12.0f, 0.1f, 8.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(greenRugColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // --- Alfombra Roja (con marco negro) ---
            {
                float centerX = 0.0f, centerY = -0.4f, centerZ = 8.0f;
                float totalWidth = 5.0f, totalDepth = 3.0f, frameThick = 0.1f;
                float frameY = centerY + 0.01f; // Evitar Z-fighting
                // 1. Relleno Rojo
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(centerX, centerY, centerZ));
                model = glm::scale(model, glm::vec3(totalWidth - (frameThick * 2), 0.1f, totalDepth - (frameThick * 2)));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(redRugColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // 2. Marco Negro (4 piezas)
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(centerX, frameY, centerZ + (totalDepth / 2.0f) - (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(totalWidth, 0.1f, frameThick));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(centerX, frameY, centerZ - (totalDepth / 2.0f) + (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(totalWidth, 0.1f, frameThick));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(centerX - (totalWidth / 2.0f) + (frameThick / 2.0f), frameY, centerZ));
                model = glm::scale(model, glm::vec3(frameThick, 0.1f, totalDepth - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(centerX + (totalWidth / 2.0f) - (frameThick / 2.0f), frameY, centerZ));
                model = glm::scale(model, glm::vec3(frameThick, 0.1f, totalDepth - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // --- Mesa (con marco negro) ---
            {
                float frameThick = 0.1f;
                // 1. Base Amarilla
                float yellow_X = 0.0f, yellow_Y = 0.5f, yellow_Z = 0.0f;
                float yellow_W = 4.0f, yellow_H = 0.2f, yellow_D = 3.0f;
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(yellow_X, yellow_Y, yellow_Z));
                model = glm::scale(model, glm::vec3(yellow_W - (frameThick * 2), yellow_H, yellow_D - (frameThick * 2)));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(tableColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // Marco Negro (Base)
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(yellow_X, yellow_Y, yellow_Z + (yellow_D / 2.0f) - (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(yellow_W, yellow_H, frameThick));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(yellow_X, yellow_Y, yellow_Z - (yellow_D / 2.0f) + (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(yellow_W, yellow_H, frameThick));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(yellow_X - (yellow_W / 2.0f) + (frameThick / 2.0f), yellow_Y, yellow_Z));
                model = glm::scale(model, glm::vec3(frameThick, yellow_H, yellow_D - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(yellow_X + (yellow_W / 2.0f) - (frameThick / 2.0f), yellow_Y, yellow_Z));
                model = glm::scale(model, glm::vec3(frameThick, yellow_H, yellow_D - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // 2. Mantel Azul
                float blue_X = 0.0f, blue_Y = 0.61f, blue_Z = 0.0f;
                float blue_W = 3.5f, blue_H = 0.02f, blue_D = 2.5f;
                float frameY_Blue = blue_Y + 0.01f;
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(blue_X, blue_Y, blue_Z));
                model = glm::scale(model, glm::vec3(blue_W - (frameThick * 2), blue_H, blue_D - (frameThick * 2)));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(tableTopColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // Marco Negro (Mantel)
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(blue_X, frameY_Blue, blue_Z + (blue_D / 2.0f) - (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(blue_W, blue_H, frameThick));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(blue_X, frameY_Blue, blue_Z - (blue_D / 2.0f) + (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(blue_W, blue_H, frameThick));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(blue_X - (blue_W / 2.0f) + (frameThick / 2.0f), frameY_Blue, blue_Z));
                model = glm::scale(model, glm::vec3(frameThick, blue_H, blue_D - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(blue_X + (blue_W / 2.0f) - (frameThick / 2.0f), frameY_Blue, blue_Z));
                model = glm::scale(model, glm::vec3(frameThick, blue_H, blue_D - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // Patas de la Mesa
            float tableLegX = 1.8f, tableLegZ = 1.3f;
            glm::vec3 tableLegPos[] = {
                glm::vec3(tableLegX, 0.0f, tableLegZ), glm::vec3(tableLegX, 0.0f, -tableLegZ),
                glm::vec3(-tableLegX, 0.0f, tableLegZ), glm::vec3(-tableLegX, 0.0f, -tableLegZ)
            };
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(tableColor));
            for (int i = 0; i < 4; i++) {
                model = houseBaseModel;
                model = glm::translate(model, tableLegPos[i]);
                model = glm::scale(model, glm::vec3(0.2f, 0.8f, 0.2f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // Sillas (4)
            float chairPositions[4][3] = { {2.5f, 0.0f, 0.0f}, {-2.5f, 0.0f, 0.0f}, {0.0f, 0.0f, 2.0f}, {0.0f, 0.0f, -2.0f} };
            float chairRotations[4] = { -90.0f, 90.0f, 180.0f, 0.0f };
            for (int i = 0; i < 4; i++) {
                glm::mat4 chairBase = houseBaseModel;
                chairBase = glm::translate(chairBase, glm::vec3(chairPositions[i][0], 0.0f, chairPositions[i][2]));
                chairBase = glm::rotate(chairBase, glm::radians(chairRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
                // Asiento
                model = chairBase;
                model = glm::translate(model, glm::vec3(0.0f, 0.2f, 0.0f));
                model = glm::scale(model, glm::vec3(1.0f, 0.2f, 1.0f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(chairColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // Respaldo
                model = chairBase;
                model = glm::translate(model, glm::vec3(0.0f, 0.75f, -0.4f));
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 0.2f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // Patas de la Silla
                float chairLegX = 0.4f, chairLegZ = 0.4f, legHeight = 0.5f, legCenterY = -0.15f;
                glm::vec3 chairLegPos[] = {
                    glm::vec3(chairLegX, legCenterY, chairLegZ), glm::vec3(chairLegX, legCenterY, -chairLegZ),
                    glm::vec3(-chairLegX, legCenterY, chairLegZ), glm::vec3(-chairLegX, legCenterY, -chairLegZ)
                };
                for (int j = 0; j < 4; j++) {
                    model = chairBase;
                    model = glm::translate(model, chairLegPos[j]);
                    model = glm::scale(model, glm::vec3(0.1f, legHeight, 0.1f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
            // Plantas
            
            for (int i = 0; i < 2; i++) {
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(plantPositions[i][0], -0.325f, plantPositions[i][1]));
                model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(potColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(plantPositions[i][0], 0.05f, plantPositions[i][1]));
                model = glm::scale(model, glm::vec3(0.50f, 0.50f, 0.50f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(plantColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // TV Planta Baja
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, -0.05f, -8.5f));
            model = glm::scale(model, glm::vec3(4.0f, 0.8f, 2.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(windowColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, 1.2f, -8.5f));
            model = glm::scale(model, glm::vec3(4.0f, 1.7f, 2.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, 1.2f, -7.49f));
            model = glm::scale(model, glm::vec3(3.6f, 1.5f, 0.02f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(lightGreyColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Alacena 
            {
                float cabinetBaseX = -4.0f, cabinetBaseZ = -8.5f, cabinetDepth = 2.0f;
                float frameThick = 0.1f, totalWidth = 3.0f;
                float base_H = 1.0f, mid_H = 0.1f, glass_H = 1.0f;
                float total_H = base_H + mid_H + glass_H + frameThick;
                float floorY = -0.45f;
                float base_Y = floorY + (base_H / 2.0f);
                float mid_Y = floorY + base_H + (mid_H / 2.0f);
                float glass_Y = floorY + base_H + mid_H + (glass_H / 2.0f);
                float pillar_Y = floorY + (total_H / 2.0f);
                // 1. Base Marrón
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, base_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth - (frameThick * 2), base_H, cabinetDepth));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(deskColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // 2. Vidrio Azul
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, glass_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth - (frameThick * 2), glass_H, cabinetDepth));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(windowColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // 3. Marco Negro (Pilares)
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX - (totalWidth / 2.0f) + (frameThick / 2.0f), pillar_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, total_H, cabinetDepth + 0.01f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX + (totalWidth / 2.0f) - (frameThick / 2.0f), pillar_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, total_H, cabinetDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // 4. Marco Café (Vigas Horizontales)
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, floorY + total_H - (frameThick / 2.0f), cabinetBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth, frameThick, cabinetDepth + 0.01f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(deskColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, mid_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth, mid_H, cabinetDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // 5. Marco Negro (Vigas Verticales)
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, glass_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, glass_H, cabinetDepth + 0.02f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, base_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, base_H, cabinetDepth + 0.02f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // Lavabo (Estilo Pokémon)
            {
                float sinkBaseX = -8.0f, sinkBaseZ = -8.5f, sinkDepth = 2.0f;
                float frameThick = 0.1f, totalWidth = 4.0f;
                float bottom_H = 1.0f, mid_H = 0.1f, top_H = 0.5f, total_H = bottom_H + mid_H + top_H;
                float floorY = -0.45f;
                float bottom_Y = floorY + (bottom_H / 2.0f), mid_Y = floorY + bottom_H + (mid_H / 2.0f);
                float top_Y = floorY + bottom_H + mid_H + (top_H / 2.0f), pillar_Y = floorY + (total_H / 2.0f);
                float left_W = totalWidth * 0.3f, right_W = totalWidth * 0.7f;
                float left_X = sinkBaseX - (totalWidth / 2.0f) + (left_W / 2.0f);
                float right_X = sinkBaseX + (totalWidth / 2.0f) - (right_W / 2.0f);
                float divider_X = sinkBaseX - (totalWidth / 2.0f) + left_W;
                // 1. Cubierta Gris
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(sinkBaseX, top_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth - (frameThick * 2), top_H, sinkDepth));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(lightGreyColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // 2. Base Azul
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(left_X, bottom_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(left_W - (frameThick / 2.0f), bottom_H, sinkDepth));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(windowColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(right_X, bottom_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(right_W - (frameThick / 2.0f), bottom_H, sinkDepth));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // 3. Marco Negro
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(sinkBaseX - (totalWidth / 2.0f) + (frameThick / 2.0f), pillar_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, total_H, sinkDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(sinkBaseX + (totalWidth / 2.0f) - (frameThick / 2.0f), pillar_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, total_H, sinkDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(sinkBaseX, floorY + total_H - (frameThick / 2.0f), sinkBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth, frameThick, sinkDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(sinkBaseX, mid_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth, mid_H, sinkDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(divider_X, bottom_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, bottom_H, sinkDepth + 0.02f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // Escaleras
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(stairsColor));
            for (int i = 0; i < 7; i++) {
                model = houseBaseModel;
                float stepY = -0.2f + i * 0.25f;
                float stepX = 5.0f + i * 0.75f;
                model = glm::translate(model, glm::vec3(stepX, stepY, -8.0f));
                model = glm::scale(model, glm::vec3(0.75f, 0.25f, 2.5f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // --- SEGUNDO PISO ---
            float secondFloorY = 3.0f;
            float houseHeight = 8.0f + secondFloorY;
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY, 0.0f));
            model = glm::scale(model, glm::vec3(20.0f, 0.1f, 20.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(floorColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY + 4.0f, -10.0f));
            model = glm::scale(model, glm::vec3(20.0f, 8.0f, 0.1f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(wallColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-10.0f, secondFloorY + 4.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 8.0f, 20.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Cama
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 0.5f, 2.0f));
            model = glm::scale(model, glm::vec3(3.0f, 1.0f, 5.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(whiteColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 1.05f, 2.0f));
            model = glm::scale(model, glm::vec3(3.0f, 0.1f, 3.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(bedBlanketColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 1.2f, 2.0f - 2.0f));
            model = glm::scale(model, glm::vec3(2.5f, 0.4f, 1.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(whiteColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 2.0f, 2.0f - 2.6f));
            model = glm::scale(model, glm::vec3(3.0f, 2.0f, 0.2f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(deskColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Escritorio
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-1.0f, secondFloorY + 1.5f, -9.0f));
            model = glm::scale(model, glm::vec3(18.0f, 0.2f, 2.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(deskColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-1.0f - 8.8f, secondFloorY + 0.75f, -9.0f));
            model = glm::scale(model, glm::vec3(0.2f, 1.5f, 2.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-1.0f + 8.8f, secondFloorY + 0.75f, -9.0f));
            model = glm::scale(model, glm::vec3(0.2f, 1.5f, 2.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Estantería
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(2.0f, secondFloorY + 2.6f, -9.2f));
            model = glm::scale(model, glm::vec3(4.0f, 2.0f, 1.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(bookshelfColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Libros
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(1.5f, secondFloorY + 2.1f, -9.1f));
            model = glm::scale(model, glm::vec3(0.2f, 1.0f, 1.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(redRugColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(1.8f, secondFloorY + 2.1f, -9.1f));
            model = glm::scale(model, glm::vec3(0.2f, 1.0f, 1.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(plantColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(2.1f, secondFloorY + 2.1f, -9.1f));
            model = glm::scale(model, glm::vec3(0.2f, 1.0f, 1.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(chairColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // PC
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 2.3f, -9.0f));
            model = glm::scale(model, glm::vec3(1.5f, 1.5f, 0.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(pcColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-9.5f, secondFloorY + 2.2f, -9.0f));
            model = glm::scale(model, glm::vec3(1.0f, 1.4f, 1.8f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 1.6f, -9.0f + 0.6f));
            model = glm::scale(model, glm::vec3(1.2f, 0.05f, 0.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-7.2f, secondFloorY + 1.6f, -9.0f + 0.6f));
            model = glm::scale(model, glm::vec3(0.25f, 0.05f, 0.4f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            //pantalla de la PC
            model = houseBaseModel;
            // Posicionada ligeramente al frente del monitor
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 2.3f, -8.74f));
            model = glm::scale(model, glm::vec3(1.4f, 1.4f, 0.05f)); // Plana
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // --- Silla de la pc ---
            glm::mat4 chairBaseModel = houseBaseModel;
            // Mover al centro de la silla y al nivel del piso 2
            chairBaseModel = glm::translate(chairBaseModel, glm::vec3(-8.0f, secondFloorY, -7.5f));
            // ¡ROTAR 180 GRADOS para que mire al escritorio!
            chairBaseModel = glm::rotate(chairBaseModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            // 2. Dibujar piezas relativas a 'chairBaseModel'

            // Base de la silla (relativa a chairBaseModel)
            model = chairBaseModel; // Empezar desde la base de la silla
            model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f)); // Posición Y relativa
            model = glm::scale(model, glm::vec3(0.2f, 1.0f, 0.2f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Asiento (relativo a chairBaseModel)
            model = chairBaseModel; // Empezar desde la base de la silla
            model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // Posición Y relativa
            model = glm::scale(model, glm::vec3(0.8f, 0.2f, 0.8f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(pcColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Respaldo (relativo a chairBaseModel)
            model = chairBaseModel; // Empezar desde la base de la silla
            // Se dibuja en Z -0.3f (hacia "atrás" de la silla)
            model = glm::translate(model, glm::vec3(0.0f, 1.5f, -0.3f));
            model = glm::scale(model, glm::vec3(0.8f, 1.0f, 0.2f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(pcColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // TV (Segunda Planta)
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY + 0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(3.5f, 1.0f, 2.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(electronicsColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY + 1.8f, 0.0f));
            model = glm::scale(model, glm::vec3(2.5f, 1.5f, 0.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(lightGreyColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY + 1.8f, 0.0f + 0.26f));
            model = glm::scale(model, glm::vec3(2.2f, 1.3f, 0.05f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY + 1.1f, 0.8f));
            model = glm::scale(model, glm::vec3(1.5f, 0.2f, 1.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(lightGreyColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-1.25f, secondFloorY + 0.5f, 0.0f + 0.9f));
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(windowColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(1.25f, secondFloorY + 0.5f, 0.0f + 0.9f));
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Fachada y Techo
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, houseHeight / 2.0f - 0.5f, -10.0f));
            model = glm::scale(model, glm::vec3(20.0f, houseHeight, 0.1f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(facadeColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-10.0f, houseHeight / 2.0f - 0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, houseHeight, 20.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(10.0f, houseHeight / 2.0f - 0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, houseHeight, 20.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, houseHeight / 2.0f - 0.5f, 10.0f));
            model = glm::scale(model, glm::vec3(20.0f, houseHeight, 0.1f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Relleno del Hueco del Techo (Triángulo)
            {
                glBindVertexArray(VAO_gap); // <-- Usar el VAO del triángulo
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(facadeColor));
                float gapHeight = 6.46f;
                float gapCenterY = 10.5f + (gapHeight / 2.0f);
                float houseWidth = 20.0f, wallDepth = 0.1f;
                // Relleno frontal
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(0.0f, gapCenterY, 10.0f));
                model = glm::scale(model, glm::vec3(houseWidth, gapHeight, wallDepth));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 3); // Solo 3 vértices
                // Relleno trasero
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(0.0f, gapCenterY, -10.0f));
                model = glm::scale(model, glm::vec3(houseWidth, gapHeight, wallDepth));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 3); // Solo 3 vértices
                glBindVertexArray(VAO); // <-- Volver al VAO del cubo
            }
            // Techo
             // --- ACTIVAR TEXTURA DE TEJADO ---
            glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tejadoTextureID);
            glUniform1i(glGetUniformLocation(modelShader.Program, "texture_diffuse1"), 0);

            float roofBaseY = houseHeight - 0.5f;
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-5.2f, roofBaseY + 3.25f, 0.0f));
            model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(12.5f, 0.2f, 22.0f));
            // La línea de glUniform3fv(roofColor) se elimina
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(5.2f, roofBaseY + 3.25f, 0.0f));
            model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(12.5f, 0.2f, 22.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // --- VOLVER A MODO COLOR SÓLIDO ---
            // (Importante para que la puerta y ventanas se dibujen bien)
            glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 0);
            // Puerta y Ventanas
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-3.0f, 0.5f, 10.05f));
            model = glm::scale(model, glm::vec3(2.5f, 3.0f, 0.1f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(doorColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            float windowZ = 10.05f;
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(4.5f, 1.5f, windowZ));
            model = glm::scale(model, glm::vec3(3.5f, 2.5f, 0.1f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(windowColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-4.5f, secondFloorY + 2.0f, windowZ));
            model = glm::scale(model, glm::vec3(3.5f, 2.5f, 0.1f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(4.5f, secondFloorY + 2.0f, windowZ));
            model = glm::scale(model, glm::vec3(3.5f, 2.5f, 0.1f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ===============================================================
        //						CASA IZQUIERDA
        // ===============================================================
        {
            // Este bloque es una copia idéntica del de la Casa Derecha,
            // solo cambia la 'houseBaseModel'
            glm::mat4 houseBaseModel = glm::translate(glm::mat4(1.0f), glm::vec3(-20.0f, 0.0f, -10.0f));
            // --- Piso ---
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(20.0f, 0.1f, 20.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(floorColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // --- Alfombra Verde (borde blanco) ---
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, -0.45f, 0.0f));
            model = glm::scale(model, glm::vec3(12.5f, 0.1f, 8.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(whiteColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, -0.4f, 0.0f));
            model = glm::scale(model, glm::vec3(12.0f, 0.1f, 8.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(greenRugColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // --- Alfombra Roja (con marco negro) ---
            {
                float centerX = 0.0f, centerY = -0.4f, centerZ = 8.0f;
                float totalWidth = 5.0f, totalDepth = 3.0f, frameThick = 0.1f;
                float frameY = centerY + 0.01f;
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(centerX, centerY, centerZ));
                model = glm::scale(model, glm::vec3(totalWidth - (frameThick * 2), 0.1f, totalDepth - (frameThick * 2)));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(redRugColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(centerX, frameY, centerZ + (totalDepth / 2.0f) - (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(totalWidth, 0.1f, frameThick));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(centerX, frameY, centerZ - (totalDepth / 2.0f) + (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(totalWidth, 0.1f, frameThick));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(centerX - (totalWidth / 2.0f) + (frameThick / 2.0f), frameY, centerZ));
                model = glm::scale(model, glm::vec3(frameThick, 0.1f, totalDepth - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(centerX + (totalWidth / 2.0f) - (frameThick / 2.0f), frameY, centerZ));
                model = glm::scale(model, glm::vec3(frameThick, 0.1f, totalDepth - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // --- Mesa (con marco negro) ---
            {
                float frameThick = 0.1f;
                float yellow_X = 0.0f, yellow_Y = 0.5f, yellow_Z = 0.0f;
                float yellow_W = 4.0f, yellow_H = 0.2f, yellow_D = 3.0f;
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(yellow_X, yellow_Y, yellow_Z));
                model = glm::scale(model, glm::vec3(yellow_W - (frameThick * 2), yellow_H, yellow_D - (frameThick * 2)));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(tableColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(yellow_X, yellow_Y, yellow_Z + (yellow_D / 2.0f) - (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(yellow_W, yellow_H, frameThick));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(yellow_X, yellow_Y, yellow_Z - (yellow_D / 2.0f) + (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(yellow_W, yellow_H, frameThick));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(yellow_X - (yellow_W / 2.0f) + (frameThick / 2.0f), yellow_Y, yellow_Z));
                model = glm::scale(model, glm::vec3(frameThick, yellow_H, yellow_D - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(yellow_X + (yellow_W / 2.0f) - (frameThick / 2.0f), yellow_Y, yellow_Z));
                model = glm::scale(model, glm::vec3(frameThick, yellow_H, yellow_D - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                float blue_X = 0.0f, blue_Y = 0.61f, blue_Z = 0.0f;
                float blue_W = 3.5f, blue_H = 0.02f, blue_D = 2.5f;
                float frameY_Blue = blue_Y + 0.01f;
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(blue_X, blue_Y, blue_Z));
                model = glm::scale(model, glm::vec3(blue_W - (frameThick * 2), blue_H, blue_D - (frameThick * 2)));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(tableTopColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(blue_X, frameY_Blue, blue_Z + (blue_D / 2.0f) - (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(blue_W, blue_H, frameThick));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(blue_X, frameY_Blue, blue_Z - (blue_D / 2.0f) + (frameThick / 2.0f)));
                model = glm::scale(model, glm::vec3(blue_W, blue_H, frameThick));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(blue_X - (blue_W / 2.0f) + (frameThick / 2.0f), frameY_Blue, blue_Z));
                model = glm::scale(model, glm::vec3(frameThick, blue_H, blue_D - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(blue_X + (blue_W / 2.0f) - (frameThick / 2.0f), frameY_Blue, blue_Z));
                model = glm::scale(model, glm::vec3(frameThick, blue_H, blue_D - (frameThick * 2)));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // Patas de la Mesa
            float tableLegX = 1.8f, tableLegZ = 1.3f;
            glm::vec3 tableLegPos[] = {
                glm::vec3(tableLegX, 0.0f, tableLegZ), glm::vec3(tableLegX, 0.0f, -tableLegZ),
                glm::vec3(-tableLegX, 0.0f, tableLegZ), glm::vec3(-tableLegX, 0.0f, -tableLegZ)
            };
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(tableColor));
            for (int i = 0; i < 4; i++) {
                model = houseBaseModel;
                model = glm::translate(model, tableLegPos[i]);
                model = glm::scale(model, glm::vec3(0.2f, 0.8f, 0.2f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // Sillas (4)
            for (int i = 0; i < 4; i++) {
                glm::mat4 chairBase = houseBaseModel;
                chairBase = glm::translate(chairBase, glm::vec3(chairPositions[i][0], 0.0f, chairPositions[i][2]));
                chairBase = glm::rotate(chairBase, glm::radians(chairRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
                model = chairBase;
                model = glm::translate(model, glm::vec3(0.0f, 0.2f, 0.0f));
                model = glm::scale(model, glm::vec3(1.0f, 0.2f, 1.0f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(chairColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = chairBase;
                model = glm::translate(model, glm::vec3(0.0f, 0.75f, -0.4f));
                model = glm::scale(model, glm::vec3(1.0f, 1.0f, 0.2f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                float chairLegX = 0.4f, chairLegZ = 0.4f, legHeight = 0.5f, legCenterY = -0.15f;
                glm::vec3 chairLegPos[] = {
                    glm::vec3(chairLegX, legCenterY, chairLegZ), glm::vec3(chairLegX, legCenterY, -chairLegZ),
                    glm::vec3(-chairLegX, legCenterY, chairLegZ), glm::vec3(-chairLegX, legCenterY, -chairLegZ)
                };
                for (int j = 0; j < 4; j++) {
                    model = chairBase;
                    model = glm::translate(model, chairLegPos[j]);
                    model = glm::scale(model, glm::vec3(0.1f, legHeight, 0.1f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
            // Plantas
            for (int i = 0; i < 2; i++) {
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(plantPositions[i][0], -0.325f, plantPositions[i][1]));
                model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(potColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(plantPositions[i][0], 0.05f, plantPositions[i][1]));
                model = glm::scale(model, glm::vec3(0.50f, 0.50f, 0.50f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(plantColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // TV Planta Baja
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, -0.05f, -8.5f));
            model = glm::scale(model, glm::vec3(4.0f, 0.8f, 2.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(windowColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, 1.2f, -8.5f));
            model = glm::scale(model, glm::vec3(4.0f, 1.7f, 2.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, 1.2f, -7.49f));
            model = glm::scale(model, glm::vec3(3.6f, 1.5f, 0.02f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(lightGreyColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Alacena (Estilo Pokémon)
            {
                float cabinetBaseX = -4.0f, cabinetBaseZ = -8.5f, cabinetDepth = 2.0f;
                float frameThick = 0.1f, totalWidth = 3.0f;
                float base_H = 1.0f, mid_H = 0.1f, glass_H = 1.0f;
                float total_H = base_H + mid_H + glass_H + frameThick;
                float floorY = -0.45f;
                float base_Y = floorY + (base_H / 2.0f);
                float mid_Y = floorY + base_H + (mid_H / 2.0f);
                float glass_Y = floorY + base_H + mid_H + (glass_H / 2.0f);
                float pillar_Y = floorY + (total_H / 2.0f);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, base_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth - (frameThick * 2), base_H, cabinetDepth));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(deskColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, glass_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth - (frameThick * 2), glass_H, cabinetDepth));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(windowColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX - (totalWidth / 2.0f) + (frameThick / 2.0f), pillar_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, total_H, cabinetDepth + 0.01f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX + (totalWidth / 2.0f) - (frameThick / 2.0f), pillar_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, total_H, cabinetDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, floorY + total_H - (frameThick / 2.0f), cabinetBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth, frameThick, cabinetDepth + 0.01f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(deskColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, mid_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth, mid_H, cabinetDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, glass_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, glass_H, cabinetDepth + 0.02f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(cabinetBaseX, base_Y, cabinetBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, base_H, cabinetDepth + 0.02f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // Lavabo (Estilo Pokémon)
            {
                float sinkBaseX = -8.0f, sinkBaseZ = -8.5f, sinkDepth = 2.0f;
                float frameThick = 0.1f, totalWidth = 4.0f;
                float bottom_H = 1.0f, mid_H = 0.1f, top_H = 0.5f, total_H = bottom_H + mid_H + top_H;
                float floorY = -0.45f;
                float bottom_Y = floorY + (bottom_H / 2.0f), mid_Y = floorY + bottom_H + (mid_H / 2.0f);
                float top_Y = floorY + bottom_H + mid_H + (top_H / 2.0f), pillar_Y = floorY + (total_H / 2.0f);
                float left_W = totalWidth * 0.3f, right_W = totalWidth * 0.7f;
                float left_X = sinkBaseX - (totalWidth / 2.0f) + (left_W / 2.0f);
                float right_X = sinkBaseX + (totalWidth / 2.0f) - (right_W / 2.0f);
                float divider_X = sinkBaseX - (totalWidth / 2.0f) + left_W;
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(sinkBaseX, top_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth - (frameThick * 2), top_H, sinkDepth));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(lightGreyColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(left_X, bottom_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(left_W - (frameThick / 2.0f), bottom_H, sinkDepth));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(windowColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(right_X, bottom_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(right_W - (frameThick / 2.0f), bottom_H, sinkDepth));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(sinkBaseX - (totalWidth / 2.0f) + (frameThick / 2.0f), pillar_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, total_H, sinkDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(sinkBaseX + (totalWidth / 2.0f) - (frameThick / 2.0f), pillar_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, total_H, sinkDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(sinkBaseX, floorY + total_H - (frameThick / 2.0f), sinkBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth, frameThick, sinkDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(sinkBaseX, mid_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(totalWidth, mid_H, sinkDepth + 0.01f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(divider_X, bottom_Y, sinkBaseZ));
                model = glm::scale(model, glm::vec3(frameThick, bottom_H, sinkDepth + 0.02f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // Escaleras
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(stairsColor));
            for (int i = 0; i < 7; i++) {
                model = houseBaseModel;
                float stepY = -0.2f + i * 0.25f;
                float stepX = 5.0f + i * 0.75f;
                model = glm::translate(model, glm::vec3(stepX, stepY, -8.0f));
                model = glm::scale(model, glm::vec3(0.75f, 0.25f, 2.5f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
            // --- SEGUNDO PISO ---
            float secondFloorY = 3.0f;
            float houseHeight = 8.0f + secondFloorY;
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY, 0.0f));
            model = glm::scale(model, glm::vec3(20.0f, 0.1f, 20.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(floorColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY + 4.0f, -10.0f));
            model = glm::scale(model, glm::vec3(20.0f, 8.0f, 0.1f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(wallColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-10.0f, secondFloorY + 4.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, 8.0f, 20.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Cama
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 0.5f, 2.0f));
            model = glm::scale(model, glm::vec3(3.0f, 1.0f, 5.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(whiteColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 1.05f, 2.0f));
            model = glm::scale(model, glm::vec3(3.0f, 0.1f, 3.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(bedBlanketColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 1.2f, 2.0f - 2.0f));
            model = glm::scale(model, glm::vec3(2.5f, 0.4f, 1.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(whiteColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 2.0f, 2.0f - 2.6f));
            model = glm::scale(model, glm::vec3(3.0f, 2.0f, 0.2f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(deskColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Escritorio
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-1.0f, secondFloorY + 1.5f, -9.0f));
            model = glm::scale(model, glm::vec3(18.0f, 0.2f, 2.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(deskColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-1.0f - 8.8f, secondFloorY + 0.75f, -9.0f));
            model = glm::scale(model, glm::vec3(0.2f, 1.5f, 2.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-1.0f + 8.8f, secondFloorY + 0.75f, -9.0f));
            model = glm::scale(model, glm::vec3(0.2f, 1.5f, 2.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Estantería
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(2.0f, secondFloorY + 2.6f, -9.2f));
            model = glm::scale(model, glm::vec3(4.0f, 2.0f, 1.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(bookshelfColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Libros
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(1.5f, secondFloorY + 2.1f, -9.1f));
            model = glm::scale(model, glm::vec3(0.2f, 1.0f, 1.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(redRugColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(1.8f, secondFloorY + 2.1f, -9.1f));
            model = glm::scale(model, glm::vec3(0.2f, 1.0f, 1.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(plantColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(2.1f, secondFloorY + 2.1f, -9.1f));
            model = glm::scale(model, glm::vec3(0.2f, 1.0f, 1.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(chairColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // PC
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 2.3f, -9.0f));
            model = glm::scale(model, glm::vec3(1.5f, 1.5f, 0.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(pcColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-9.5f, secondFloorY + 2.2f, -9.0f));
            model = glm::scale(model, glm::vec3(1.0f, 1.4f, 1.8f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 1.6f, -9.0f + 0.6f));
            model = glm::scale(model, glm::vec3(1.2f, 0.05f, 0.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-7.2f, secondFloorY + 1.6f, -9.0f + 0.6f));
            model = glm::scale(model, glm::vec3(0.25f, 0.05f, 0.4f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            //pantalla de la PC
            model = houseBaseModel;
            // Posicionada ligeramente al frente del monitor
            model = glm::translate(model, glm::vec3(-8.0f, secondFloorY + 2.3f, -8.74f));
            model = glm::scale(model, glm::vec3(1.4f, 1.4f, 0.05f)); // Plana
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // --- Silla de la pc ---
            glm::mat4 chairBaseModel = houseBaseModel;
            // Mover al centro de la silla y al nivel del piso 2
            chairBaseModel = glm::translate(chairBaseModel, glm::vec3(-8.0f, secondFloorY, -7.5f));
            // ¡ROTAR 180 GRADOS para que mire al escritorio!
            chairBaseModel = glm::rotate(chairBaseModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            // 2. Dibujar piezas relativas a 'chairBaseModel'

            // Base de la silla (relativa a chairBaseModel)
            model = chairBaseModel; // Empezar desde la base de la silla
            model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f)); // Posición Y relativa
            model = glm::scale(model, glm::vec3(0.2f, 1.0f, 0.2f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Asiento
            model = chairBaseModel; // Empezar desde la base de la silla
            model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f)); // Posición Y relativa
            model = glm::scale(model, glm::vec3(0.8f, 0.2f, 0.8f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(pcColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // Respaldo (relativo a chairBaseModel)
            model = chairBaseModel; // Empezar desde la base de la silla
            // Se dibuja en Z -0.3f (hacia "atrás" de la silla)
            model = glm::translate(model, glm::vec3(0.0f, 1.5f, -0.3f));
            model = glm::scale(model, glm::vec3(0.8f, 1.0f, 0.2f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(pcColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // TV (Segunda Planta)
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY + 0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(3.5f, 1.0f, 2.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(electronicsColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY + 1.8f, 0.0f));
            model = glm::scale(model, glm::vec3(2.5f, 1.5f, 0.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(lightGreyColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY + 1.8f, 0.0f + 0.26f));
            model = glm::scale(model, glm::vec3(2.2f, 1.3f, 0.05f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(blackColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, secondFloorY + 1.1f, 0.8f));
            model = glm::scale(model, glm::vec3(1.5f, 0.2f, 1.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(lightGreyColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-1.25f, secondFloorY + 0.5f, 0.0f + 0.9f));
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(windowColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(1.25f, secondFloorY + 0.5f, 0.0f + 0.9f));
            model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Fachada y Techo
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, houseHeight / 2.0f - 0.5f, -10.0f));
            model = glm::scale(model, glm::vec3(20.0f, houseHeight, 0.1f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(facadeColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-10.0f, houseHeight / 2.0f - 0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, houseHeight, 20.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(10.0f, houseHeight / 2.0f - 0.5f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f, houseHeight, 20.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, houseHeight / 2.0f - 0.5f, 10.0f));
            model = glm::scale(model, glm::vec3(20.0f, houseHeight, 0.1f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Relleno del Hueco del Techo (Triángulo)
            {
                glBindVertexArray(VAO_gap); // <-- Usar el VAO del triángulo
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(facadeColor));
                float gapHeight = 6.46f;
                float gapCenterY = 10.5f + (gapHeight / 2.0f);
                float houseWidth = 20.0f, wallDepth = 0.1f;
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(0.0f, gapCenterY, 10.0f));
                model = glm::scale(model, glm::vec3(houseWidth, gapHeight, wallDepth));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 3);
                model = houseBaseModel;
                model = glm::translate(model, glm::vec3(0.0f, gapCenterY, -10.0f));
                model = glm::scale(model, glm::vec3(houseWidth, gapHeight, wallDepth));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glBindVertexArray(VAO); // <-- Volver al VAO del cubo
            }
            // Techo
            // --- ACTIVAR TEXTURA DE TEJADO ---
            glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tejadoTextureID);
            glUniform1i(glGetUniformLocation(modelShader.Program, "texture_diffuse1"), 0);

            float roofBaseY = houseHeight - 0.5f;
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-5.2f, roofBaseY + 3.25f, 0.0f));
            model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(12.5f, 0.2f, 22.0f));
            // La línea de glUniform3fv(roofColor) se elimina
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(5.2f, roofBaseY + 3.25f, 0.0f));
            model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(12.5f, 0.2f, 22.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            // --- VOLVER A MODO COLOR SÓLIDO ---
            // (Importante para que la puerta y ventanas se dibujen bien)
            glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 0);
            // Puerta y Ventanas
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-3.0f, 0.5f, 10.05f));
            model = glm::scale(model, glm::vec3(2.5f, 3.0f, 0.1f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(doorColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            float windowZ = 10.05f;
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(4.5f, 1.5f, windowZ));
            model = glm::scale(model, glm::vec3(3.5f, 2.5f, 0.1f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(windowColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(-4.5f, secondFloorY + 2.0f, windowZ));
            model = glm::scale(model, glm::vec3(3.5f, 2.5f, 0.1f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            model = houseBaseModel;
            model = glm::translate(model, glm::vec3(4.5f, secondFloorY + 2.0f, windowZ));
            model = glm::scale(model, glm::vec3(3.5f, 2.5f, 0.1f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // --- Laboratorio ---
        {
            glm::mat4 labBaseModel = glm::translate(glm::mat4(1.0f), glm::vec3(15.0f, 0.0f, 25.0f));
            // Base principal
            model = labBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
            model = glm::scale(model, glm::vec3(30.0f, 5.0f, 15.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(labWallColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Techo
            model = labBaseModel;
            model = glm::translate(model, glm::vec3(0.0f, 4.5f, 0.0f));
            model = glm::scale(model, glm::vec3(32.0f, 0.5f, 16.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(labRoofColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
            // Estructura roja lateral
            model = labBaseModel;
            model = glm::translate(model, glm::vec3(12.0f, 6.0f, 0.0f));
            model = glm::scale(model, glm::vec3(4.0f, 3.0f, 8.0f));
            glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(labAccentColor));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // ===============================================================
        //						POSTES DE LUZ
        // ===============================================================
        {
            glm::vec3 postPositions[] = {
                glm::vec3(30.0f, 0.0f, 2.0f),  // Poste derecha
                glm::vec3(-9.5f, 0.0f, 2.0f)   // Poste izquierda
            };

            for (int i = 0; i < 2; i++)
            {
                glm::mat4 basePoste = glm::translate(glm::mat4(1.0f), postPositions[i]);
                // 1. Poste vertical
                model = basePoste;
                model = glm::translate(model, glm::vec3(0.0f, 3.5f, 0.0f));
                model = glm::scale(model, glm::vec3(0.35f, 7.0f, 0.35f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(postColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // 2. Brazo horizontal
                model = basePoste;
                model = glm::translate(model, glm::vec3(0.0f, 6.8f, 0.5f));
                model = glm::scale(model, glm::vec3(0.3f, 0.3f, 1.0f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
                // 3. "Luz" (bombilla)
                model = basePoste;
                model = glm::translate(model, glm::vec3(0.010f, 6.5f, 0.9f));
                model = glm::scale(model, glm::vec3(0.9f, 0.9f, 0.9f));
                glUniform3fv(glGetUniformLocation(modelShader.Program, "material.diffuse"), 1, glm::value_ptr(lightColor));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }

        // ===============================================================
        //      PASO 2: DIBUJAR EL CÉSPED Y AGUA (CON TEXTURA)
        // ===============================================================

        // Configura el shader para objetos CON textura
        glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 1); // 1 = SÍ usar textura
        glUniform3f(glGetUniformLocation(modelShader.Program, "material.specular"), 0.1f, 0.1f, 0.1f); // Poco brillo
        glUniform1f(glGetUniformLocation(modelShader.Program, "material.shininess"), 16.0f);

        // **NOTA**: Ya no declaramos 'modelLoc' aquí, usamos la que definimos
        // en la sección de objetos sólidos (línea 622).

        // --- Dibujar Césped ---
        glBindVertexArray(VAO); // VAO del Cubo
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTextureID);
        glUniform1i(glGetUniformLocation(modelShader.Program, "texture_diffuse1"), 0);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(100.0f, 0.1f, 100.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);


        // --- Dibujar Estanque de Agua ---
        glBindVertexArray(VAO_water); // VAO del Agua (con coords. 2x2)
        glActiveTexture(GL_TEXTURE0);
        // --- Lógica de Animación del Agua ---
    // (Reemplaza la línea 'glBindTexture(GL_TEXTURE_2D, waterTextureID);')

    // fmod(tiempo, 1.0) crea un ciclo que va de 0.0 a 1.0 cada segundo
        float waterFrameTime = fmod(glfwGetTime(), 1.0f);

        if (waterFrameTime < 0.5f)
        {
            // Primera mitad del segundo: usa la textura 1
            glBindTexture(GL_TEXTURE_2D, waterTextureID);
        }
        else
        {
            // Segunda mitad del segundo: usa la textura 2
            glBindTexture(GL_TEXTURE_2D, waterTextureID_2);
        }
        glUniform1i(glGetUniformLocation(modelShader.Program, "texture_diffuse1"), 0);

        
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-30.0f, -0.9f, 38.0f));
        model = glm::scale(model, glm::vec3(15.0f, 0.1f, 23.5f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 36);


        // ===============================================================
        //      PASO 3: DIBUJAR LOS MODELOS 3D CARGADOS
        // ===============================================================

        // (modelShader ya está activo y la cámara configurada)

        // Configuración de material para los modelos (brillantes)
        glUniform1i(glGetUniformLocation(modelShader.Program, "useTexture"), 1); // SÍ usan textura
        glUniform3f(glGetUniformLocation(modelShader.Program, "material.specular"), 1.0f, 1.0f, 1.0f); // Muy brillante
        glUniform1f(glGetUniformLocation(modelShader.Program, "material.shininess"), 64.0f);

        // --- Dibujar Mew ---
        glm::mat4 modelMew = glm::mat4(1.0f);

        // 1. Traslación (Mover a Mew a su posición)
        modelMew = glm::translate(modelMew, mewPos);

        // 2. Rotación (Hacer que Mew mire hacia 'mewDirection')
        //    (Evita un error matemático si la dirección es (0,0,0))
        if (glm::length(mewDirection) > 0.0001f)
        {
            // Esta matriz 'lookAt' calcula la rotación necesaria
            glm::mat4 rotationMatrixMew = glm::inverse(glm::lookAt(glm::vec3(0.0f), -mewDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
            modelMew = modelMew * rotationMatrixMew;
        }

        // 3. Escala (Hacer el modelo más pequeño)
        modelMew = glm::scale(modelMew, glm::vec3(0.15f, 0.15f, 0.15f));

        // 4. Enviar la matriz final al shader y dibujar
        glUniformMatrix4fv(glGetUniformLocation(modelShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(modelMew));
        mewModel.Draw(modelShader);

        // --- Dibujar Ho-oh ---
        glm::mat4 modelHoOh = glm::mat4(1.0f);
        // 1. Traslación
        modelHoOh = glm::translate(modelHoOh, hoohPos);
        // 2. Rotación Dinámica (hacia donde vuela)
        glm::mat4 rotationMatrix = glm::inverse(glm::lookAt(glm::vec3(0.0f), -hoohDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        modelHoOh = modelHoOh * rotationMatrix;
        // 3. Rotación Estática (inclinación)
        modelHoOh = glm::rotate(modelHoOh, glm::radians(75.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        // 4. Aleteo (Balanceo)
        float flapFactor = sin(glfwGetTime() * 4.0f); // Oscila
        modelHoOh = glm::rotate(modelHoOh, glm::radians(flapFactor * 15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // 5. Escala
        modelHoOh = glm::scale(modelHoOh, glm::vec3(0.25f, 0.25f, 0.25f));
        glUniformMatrix4fv(glGetUniformLocation(modelShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(modelHoOh));
        hoohModel.Draw(modelShader);

        // ===============================================================
        //      PASO 5: DIBUJAR EL SOL VISUAL (SIN LUZ)
        // ===============================================================

        // Usamos el shader de color simple (ourShader)
        ourShader.Use();

        // Pasamos las matrices (este shader sí las necesita)
        glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // Desactivamos la prueba de profundidad (para que se vea siempre)
        //glDisable(GL_DEPTH_TEST);

        glm::mat4 modelSun = glm::mat4(1.0f);
        modelSun = glm::translate(modelSun, glm::vec3(80.0f, 80.0f, 0.0f)); // Posición lejana
        modelSun = glm::scale(modelSun, glm::vec3(10.0f, 10.0f, 10.0f));
        glUniformMatrix4fv(glGetUniformLocation(ourShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(modelSun));

        // Color naranja
        if (isNight)
        {
            // Es de noche: Dibuja una Luna (azul/blanca)
            glUniform3f(ourShader.uniformColor, 0.8f, 0.9f, 1.0f);
        }
        else
        {
            // Es de día o atardecer: Dibuja un Sol (naranja)
            glUniform3f(ourShader.uniformColor, 1.0f, 0.5f, 0.0f);
        }

        glBindVertexArray(VAO); // Usamos el VAO del cubo
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Reactivamos la prueba de profundidad
        glEnable(GL_DEPTH_TEST);

        // --- Terminar el frame ---
        glBindVertexArray(0); // Desenlaza el VAO
        glfwSwapBuffers(window);
    }
    // --- Fin del bucle principal (while) ---


    // --- Limpieza de Recursos ---
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO_water);
    glDeleteBuffers(1, &VBO_water);
    glDeleteVertexArrays(1, &VAO_gap);
    glDeleteBuffers(1, &VBO_gap);

    glfwTerminate();
    return EXIT_SUCCESS;
}


// --- Definición de Funciones de Control ---

void DoMovement() {
    // Movimiento de Cámara
    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (keys[GLFW_KEY_SPACE])
        camera.ProcessKeyboard(UP, deltaTime);
    if (keys[GLFW_KEY_C])
        camera.ProcessKeyboard(DOWN, deltaTime);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    // Cerrar ventana
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Transición a NOCHE (Tecla N)
    if (key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        if (!isTransitioning)
        {
            isTransitioning = true;
            transitionFactor = 0.0f;
            startTransitionColor = currentColor;
            targetColor = isNight ? dayColor : nightColor; // Alterna
            isSunset = false;
        }
    }

    // Transición a ATARDECER (Tecla M)
    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        if (!isTransitioning)
        {
            isTransitioning = true;
            transitionFactor = 0.0f;
            startTransitionColor = currentColor;
            targetColor = isSunset ? dayColor : sunsetColor; // Alterna
            isNight = false;
        }
    }

    // Registro de teclas presionadas
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos) {
    if (firstMouse) {
        lastX = (GLfloat)xPos;
        lastY = (GLfloat)yPos;
        firstMouse = false;
    }

    GLfloat xOffset = (GLfloat)xPos - lastX;
    GLfloat yOffset = lastY - (GLfloat)yPos; // Invertido

    lastX = (GLfloat)xPos;
    lastY = (GLfloat)yPos;

    camera.ProcessMouseMovement(xOffset, yOffset);
}

void Animacion()
{
    // 1. Comprobar si Ho-oh llegó al objetivo
    float dist = glm::distance(hoohPos, hoohTargetPos);
    if (dist < 2.0f)
    {
        // 2. Generar nuevo objetivo aleatorio
        float targetX = (rand() % 120) - 60.0f; // Rango -60 a 60
        float targetZ = (rand() % 120) - 60.0f; // Rango -60 a 60
        float targetY = (rand() % 20) + hoohMinY; // Rango [hoohMinY] a [hoohMinY + 20]
        hoohTargetPos = glm::vec3(targetX, targetY, targetZ);
    }

    // 3. Calcular dirección de vuelo (con curvas)
    glm::vec3 linearDirection = glm::normalize(hoohTargetPos - hoohPos);
    glm::vec3 rightVector = glm::normalize(glm::cross(linearDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
    float curveFactor = sin(glfwGetTime() * hoohCurveSpeed);

    // 4. Actualizar dirección y posición
    hoohDirection = glm::normalize(linearDirection + (rightVector * curveFactor * hoohCurveAmount));
    hoohPos += hoohDirection * hoohSpeed * deltaTime;

    // --- ANIMACIÓN DE MEW (AGREGAR ESTO) ---
    // 1. Comprobar si Mew llegó al objetivo
    float distMew = glm::distance(mewPos, mewTargetPos);
    if (distMew < 1.0f) // Radio más pequeño para cambiar de objetivo con más frecuencia
    {
        // 2. Generar nuevo objetivo aleatorio
        float targetX = (rand() % 40) - 20.0f; // Rango -20 a 20 (más centrado)
        float targetZ = (rand() % 40) - 20.0f; // Rango -20 a 20
        float targetY = (rand() % 3) + mewMinY; // Rango [mewMinY] a [mewMinY + 3]
        mewTargetPos = glm::vec3(targetX, targetY, targetZ);
    }

    // 3. Calcular dirección de vuelo (con curvas pronunciadas)
    glm::vec3 linearDirectionMew = glm::normalize(mewTargetPos - mewPos);
    glm::vec3 rightVectorMew = glm::normalize(glm::cross(linearDirectionMew, glm::vec3(0.0f, 1.0f, 0.0f)));
    float curveFactorMew = sin(glfwGetTime() * mewCurveSpeed); // Oscilación de la curva

    // 4. Actualizar dirección y posición
    mewDirection = glm::normalize(linearDirectionMew + (rightVectorMew * curveFactorMew * mewCurveAmount));
    mewPos += mewDirection * mewSpeed * deltaTime;
}

