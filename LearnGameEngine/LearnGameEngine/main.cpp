#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>

#include "time.hpp"
#include "events.hpp"
#include "input.hpp"

#include "debug.hpp"
#include "debug_system.hpp"

/*=====GENERAL NAMING CONVENTIONS=====
    VARIABLES :
        - m_foo -> member variable (non-static)
        - g_foo -> global variable
        - s_foo -> static variable (member or function-local)
        - kFoo  -> constant value

    FUNCTION / Types  :
        - names in pamel case [GetThing() or PlayerController]
*/
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{
    // initializing glfw
	glfwInit(); // setting up internal stuff to interface with windows
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // telling glfw that i want v3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // specifically v3.3
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // forces VAOs,shaders,etc

    // creating the window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Triangle", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // loading opengl functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    // viewport and resize
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // triangle vertex data
    float vertices[] = //in clip space
    {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    // VBO -> Vertex Buffer Object (stores vertex data)
    // VAO -> Vertex Array Object (data to intepret vertex data)
    // sending data to gpu
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO); // get id from opengl
    glGenBuffers(1, &VBO); // get a buffer id from opengl

    glBindVertexArray(VAO); // set active array to this

    glBindBuffer(GL_ARRAY_BUFFER, VBO); // set the active buffer to the current
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // copy vertex data into gpu

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); // describes the layout of the vertex data to the shader
    glEnableVertexAttribArray(0); // turns the attribute on

    // ============SHADER===========
    // vertex shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() 
        { 
            gl_Position = vec4(aPos, 1.0); 
        }
    )";

    // fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 uColor;
        void main() 
        { 
            FragColor = vec4(uColor,1.0); 
        }
    )";

    //compiling and linking shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    //links the shader
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    //=======================================

    //getting location id for uniforms
    int colorLoc = glGetUniformLocation(shaderProgram, "uColor"); //has to be same signature as in shaders

    //initializing global systems
    Engine::Time       g_Time; 
    Engine::TimeConfig g_TimeConfig;
    Engine::EventBus   g_EventBus; //create a global event bus

    double startTime = glfwGetTime();
    Engine::TimeSystem::Initialize(g_Time, startTime, g_TimeConfig);
    Engine::InputSystem::Initialize(window, &g_EventBus);

    // plug in debug frame listener
    Engine::DebugFrameListener g_DebugFrameListener(g_EventBus);

    //subscribing to the key pressed event
    g_EventBus.Subscribe<Engine::KeyPressedEvent>([](const Engine::KeyPressedEvent& e) {
            if (e.key == GLFW_KEY_ESCAPE) 
            {
                printf("ESCAPE\n");
            }
        });

    // engine loop
    while (!glfwWindowShouldClose(window))
    {
        double now = glfwGetTime();

        //begin system frame
        Engine::TimeSystem::BeginFrame(g_Time, now);
        Engine::InputSystem::BeginFrame();
        glfwPollEvents(); // keyboard/mouse/window events

        g_EventBus.Emit(Engine::FrameStartEvent{ g_Time });

        //=====FIXED UPDATE=====
        int stepIndex = 0;
        while (Engine::TimeSystem::StepFixed(g_Time))
        {
            g_EventBus.Emit(Engine::FixedUpdateEvent
                {
                    g_Time,
                    g_Time.fixedDeltaTime,
                    stepIndex++
                });
            // PhysicsSystem::FixedUpdate(g_Time.fixedDeltaTime);
        }

        //=====UPDATE=====
        g_EventBus.Emit(Engine::UpdateEvent{ g_Time });

        //manually checking input
        if (Engine::InputSystem::WasKeyPressed(GLFW_KEY_SPACE)) 
        { 
            printf("IM PRESSING SPACE\n");
        }



        //=====RENDER=====
        float alpha = 0.0f;
        if (g_Time.fixedDeltaTime > 0.0f)
        {
            alpha = std::clamp(
                g_Time.accumulator / g_Time.fixedDeltaTime,
                0.0f,
                1.0f
            );
        }

        g_EventBus.Emit(Engine::RenderEvent{ g_Time,alpha });

        // clears the buffer and sets it to this colour
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // runs our shader program
        glUseProgram(shaderProgram);

        //binding uniforms
        glUniform3f(colorLoc, 1.0f, 0.5f, 0.2f);


        glBindVertexArray(VAO); // bind our vertex setup
        glDrawArrays(GL_TRIANGLES, 0, 3); //draw the triangle


        //=====FRAME END=====
        g_EventBus.Emit(Engine::FrameEndEvent{ g_Time });

        glfwSwapBuffers(window); // show rendered frame
    }

    //cleanup and terminate window
    glfwTerminate();
    return 0;
}