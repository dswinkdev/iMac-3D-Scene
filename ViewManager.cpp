///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// Manage the viewing of 3D objects within the viewport, including camera
// navigation, mouse control, and switching between perspective and orthographic 
// projections.
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Declaration of global variables and defines
namespace
{
    // Variables for window width and height
    const int WINDOW_WIDTH = 1000;
    const int WINDOW_HEIGHT = 800;
    const char* g_ViewName = "view";
    const char* g_ProjectionName = "projection";

    // Camera object used for viewing and interacting with the 3D scene
    Camera* g_pCamera = nullptr;

    // Camera speed
    float g_CameraSpeed = 2.5f;

    // Mouse control variables
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // Time tracking for smooth frame rate
    float gDeltaTime = 0.0f;
    float gLastFrame = 0.0f;

    // Flag for orthographic projection
    bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  Constructor for the class that initializes the camera
 *  and other necessary variables.
 ***********************************************************/
ViewManager::ViewManager(ShaderManager* pShaderManager)
{
    // Initialize member variables
    m_pShaderManager = pShaderManager;
    m_pWindow = nullptr;
    g_pCamera = new Camera();

    // Default camera view parameters
    g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
    g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
    g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
    g_pCamera->Zoom = 80.0f;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  Destructor for the class that cleans up resources.
 ***********************************************************/
ViewManager::~ViewManager()
{
    // Free up allocated memory
    m_pShaderManager = nullptr;
    m_pWindow = nullptr;

    if (g_pCamera != nullptr)
    {
        delete g_pCamera;
        g_pCamera = nullptr;
    }
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  Creates the main display window and sets up mouse callbacks.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowTitle, nullptr, nullptr);

    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    // Set the created window as the main GLFW window for OpenGL
    glfwMakeContextCurrent(window);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window, ViewManager::Window_Resize_Callback);
    glfwSetCursorPosCallback(window, ViewManager::Mouse_Position_Callback);
    glfwSetScrollCallback(window, ViewManager::Mouse_Scroll_Wheel_Callback);

    // Enable blending for transparent rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_pWindow = window;
    return window;
}

/***********************************************************
 *  Window_Resize_Callback()
 *
 *  Callback function for handling window resize events.
 ***********************************************************/
void ViewManager::Window_Resize_Callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  Callback function for mouse movement to adjust camera
 *  orientation.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
    if (gFirstMouse)
    {
        gLastX = static_cast<float>(xMousePos);
        gLastY = static_cast<float>(yMousePos);
        gFirstMouse = false;
    }

    float xOffset = static_cast<float>(xMousePos) - gLastX;
    float yOffset = gLastY - static_cast<float>(yMousePos); // Invert Y-axis
    gLastX = static_cast<float>(xMousePos);
    gLastY = static_cast<float>(yMousePos);

    if (g_pCamera)
    {
        g_pCamera->ProcessMouseMovement(xOffset, yOffset);
    }
}

/***********************************************************
 *  Mouse_Scroll_Wheel_Callback()
 *
 *  Callback function for mouse scroll wheel events.
 ***********************************************************/
void ViewManager::Mouse_Scroll_Wheel_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
    if (g_pCamera)
    {
        g_pCamera->ProcessMouseScroll(static_cast<float>(yOffset));
    }
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  Processes keyboard events to control the camera's movement
 *  and other actions (e.g., toggling projections).
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
    // Close the window if the ESC key is pressed
    if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(m_pWindow, true);
    }

    // Exit if camera is invalid
    if (g_pCamera == nullptr) return;

    // Adjust camera speed with up/down arrows
    if (glfwGetKey(m_pWindow, GLFW_KEY_UP) == GLFW_PRESS)
    {
        g_CameraSpeed += 0.5f * gDeltaTime;
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        g_CameraSpeed = std::max(0.5f, g_CameraSpeed - 0.5f * gDeltaTime);
    }

    // Process basic camera movement (W, A, S, D for forward/backward/left/right)
    if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
    }

    // Process vertical movement (Q and E for up/down)
    if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(UP, gDeltaTime);
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
    {
        g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
    }

    // Toggle between orthographic and perspective projections (P for perspective, O for orthographic)
    if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
    {
        bOrthographicProjection = false;
    }
    if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
    {
        bOrthographicProjection = true;
    }
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  Prepares the scene view by calculating the view and
 *  projection matrices based on the camera's position and
 *  the chosen projection mode (perspective or orthographic).
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
    glm::mat4 view;
    glm::mat4 projection;

    // Update frame time
    float currentFrame = glfwGetTime();
    gDeltaTime = currentFrame - gLastFrame;
    gLastFrame = currentFrame;

    // Process any keyboard events
    ProcessKeyboardEvents();

    // Get the current view matrix from the camera
    view = g_pCamera->GetViewMatrix();

    // Set the projection matrix based on the current projection mode
    if (bOrthographicProjection)
    {
        float orthoWidth = 10.0f;
        float orthoHeight = orthoWidth * (float)WINDOW_HEIGHT / (float)WINDOW_WIDTH;
        projection = glm::ortho(-orthoWidth, orthoWidth, -orthoHeight, orthoHeight, 0.1f, 100.0f);
    }
    else
    {
        projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    }

    // If the shader manager is valid, set the view and projection matrices in the shader
    if (m_pShaderManager != nullptr)
    {
        m_pShaderManager->setMat4Value(g_ViewName, view);
        m_pShaderManager->setMat4Value(g_ProjectionName, projection);
        m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
    }
}