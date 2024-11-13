/**
 * @file batches.cpp
 * @author Dario Loi
 * @brief n-body problem simulation
 * @date 2024-11-04
 *
 * @copyright MIT License
 *
 * @example nbody.cpp
 */

#include "glad.h"
#include "staplegl.hpp"

#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/vec2.hpp>
#include <iostream>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "external/assets/screen_quad.h"

// #define CENTER_ATTRACTOR

constexpr float GRAVITY_CONSTANT = 1e-7;
constexpr float BODY_MASS = 2.5e4;
constexpr float TIME_STEP = 1 / 120.0F;
constexpr float SIM_BOUNDARY = 100.0F;
constexpr float SOFTENING = 1e0F;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// OpenGL debug callback
void GLAPIENTRY
MessageCallback(GLenum source [[maybe_unused]],
    GLenum type,
    GLuint id [[maybe_unused]],
    GLenum severity,
    GLsizei length [[maybe_unused]],
    const GLchar* message,
    const void* userParam [[maybe_unused]])
{

    // skip non-errors
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION || type == GL_DEBUG_TYPE_PERFORMANCE || type == GL_DEBUG_TYPE_OTHER) {
        return;
    }

    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x,\nmessage = %s\n", // NOLINT
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
        type, severity, message);

    // print location of error
    fprintf(stderr, "source = 0x%x, id = %d\n", source, id); // NOLINT
}

// initial window size
constexpr int32_t SCR_WIDTH = 1600;
constexpr int32_t SCR_HEIGHT = 900;

auto main(int argc, char* argv[]) -> int
{

    if (argc != 2) {
        std::cerr << "Usage:\n"
                  << argv[0] << " <N_POINTS>" << std::endl;
        return EXIT_FAILURE;
    }

    size_t off [[maybe_unused]];
    const std::string arg_points { argv[1] };
    const int32_t points_in = std::stoi(arg_points, &off);

    if (points_in <= 0) {
        std::cerr << "Invalid number of points" << std::endl;
        return EXIT_FAILURE;
    }

    if (points_in > 1.024e3) {
        std::cerr << "Too many points, simulation is O(n^2) and will blow up :(" << std::endl;
        return EXIT_FAILURE;
    }

    const uint32_t N_POINTS = static_cast<uint32_t>(points_in);

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "N-Body Problem Simulator", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(0);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) { // NOLINT (reinterpret-cast)
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // During init, enable debug output
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, nullptr);
#ifdef STAPLEGL_DEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif // STAPLEGL_DEBUG
    // antialiasing and other nice things
    glEnable(GL_MULTISAMPLE);
    glPointSize(10.0F / (1.0F + std::log10f(N_POINTS)));

    staplegl::shader_program basic { "nbody_shader", "./shaders/nbody_shader.glsl" };
    staplegl::shader_program smooth_texture { "smooth_texture", "./shaders/smooth_texture.glsl" };
    staplegl::shader_program passthrough { "passthrough_shader",
        "./shaders/passthrough_shader.glsl" };

    staplegl::texture_2d last_frame {
        {},
        staplegl::resolution { SCR_WIDTH, SCR_HEIGHT },
        staplegl::texture_color {
            .internal_format = GL_RGB8, .format = GL_RGB, .datatype = GL_UNSIGNED_BYTE },
        staplegl::texture_filter {
            .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR, .clamping = GL_CLAMP_TO_EDGE }
    };

    staplegl::texture_2d this_frame {
        {},
        staplegl::resolution { SCR_WIDTH, SCR_HEIGHT },
        staplegl::texture_color {
            .internal_format = GL_RGB8, .format = GL_RGB, .datatype = GL_UNSIGNED_BYTE },
        staplegl::texture_filter {
            .min_filter = GL_LINEAR, .mag_filter = GL_LINEAR, .clamping = GL_CLAMP_TO_EDGE }
    };

    smooth_texture.bind();
    smooth_texture.upload_uniform1i("u_last_frame", 0);
    passthrough.bind();
    passthrough.upload_uniform1i("u_tex", 0);

    using namespace staplegl::shader_data_type;

    staplegl::vertex_buffer_layout const layout_pos {
        { u_type::vec2, "aPos" }
    };

    staplegl::vertex_buffer_layout const layout_col {
        { u_type::vec3, "aCol" }
    };

    staplegl::vertex_buffer_layout const screen_quad_layout {
        { u_type::vec3, "aPos" },
        { u_type::vec2, "aUv" }
    };

    staplegl::vertex_buffer screen_quad_vtx(
        { quadVertices, STAPLEGL_QUAD_VERTICES });

    staplegl::vertex_array screen_quad_vao {};

    screen_quad_vtx.set_layout(screen_quad_layout);
    screen_quad_vao.add_vertex_buffer(std::move(screen_quad_vtx));

    // framebuffer

    staplegl::framebuffer post_fbo {};

    post_fbo.bind();
    post_fbo.set_renderbuffer({ 1600, 900 });
    staplegl::framebuffer::unbind(); // bind defo FBO

    struct simulation {
        glm::vec2* p;
        glm::vec2* v;
        glm::vec2* a;
        float* m;
    } sim;

    sim.p = new glm::vec2[N_POINTS];
    sim.v = new glm::vec2[N_POINTS];
    sim.a = new glm::vec2[N_POINTS];
    sim.m = new float[N_POINTS];

    auto* const colors = new glm::vec3[N_POINTS];

    auto dist = std::normal_distribution<float>(0.0F, std::sqrtf(SIM_BOUNDARY) * std::log10f(N_POINTS));
    auto mass_dist = std::normal_distribution<float>(BODY_MASS, std::sqrtf(BODY_MASS));
    auto col_dist = std::uniform_real_distribution<float>(0.0F, 1.0F);
    auto rng = std::mt19937_64(std::random_device {}());

    for (uint16_t i = 0; i < N_POINTS; ++i) {
        sim.p[i] = { dist(rng), dist(rng) };
        sim.v[i] = { 0.0F, 0.0F };
        sim.a[i] = { 0.0F, 0.0F };
        sim.m[i] = mass_dist(rng);
        colors[i] = { col_dist(rng), col_dist(rng), col_dist(rng) };
    }

    basic.bind();
    basic.upload_uniform1f("uScale", SIM_BOUNDARY);

    staplegl::vertex_buffer VBO_pos({ reinterpret_cast<float*>(sim.p),
                                        N_POINTS * 2 },
        layout_pos,
        staplegl::driver_draw_hint::STREAM_DRAW);

    staplegl::vertex_buffer VBO_color(
        { reinterpret_cast<float*>(colors), N_POINTS * 3 },
        layout_col,
        staplegl::driver_draw_hint::STATIC_DRAW);

    staplegl::vertex_array VAO;

    VAO.add_vertex_buffer(std::move(VBO_pos));
    VAO.add_vertex_buffer(std::move(VBO_color));

    auto lastTime = glfwGetTime();
    auto acc = 0.0;

    while (glfwWindowShouldClose(window) == 0) {
        processInput(window);

        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);

        auto timeNow = glfwGetTime();
        auto deltaTime = timeNow - lastTime;
        lastTime = timeNow;

        acc += deltaTime;

        // STEP PHYSICAL SIMULATION

        while (acc >= TIME_STEP) {

#pragma omp parallel for collapse(2)
            for (uint32_t i = 0U; i < N_POINTS; ++i) {

                const auto last_acc = sim.a[i];
                sim.a[i] = { 0.0F, 0.0F };

                for (uint32_t j = 0; j < N_POINTS; ++j) {

                    if (i == j) {
                        continue;
                    }

                    auto const& p1 = sim.p[i];
                    auto const& p2 = sim.p[j];

                    // calculate center of mass
                    glm::vec2 com = (p1 * sim.m[i] + p2 * sim.m[j]) / (sim.m[i] + sim.m[j]);

                    const auto d = glm::distance(com, p1);
                    const auto dir = glm::normalize(com - p1);

                    sim.a[i] += dir * GRAVITY_CONSTANT * sim.m[i] * sim.m[j] / (d * d + SOFTENING);
                }

                sim.p[i] += sim.v[i] * TIME_STEP + last_acc * TIME_STEP * TIME_STEP * 0.5F;
                sim.v[i] += 0.5F * (last_acc + sim.a[i]) * TIME_STEP;

// apply slight attraction to the center, proportional to the distance
#ifdef CENTER_ATTRACTOR
                sim.a[i] += glm::normalize(-sim.p[i]) * TIME_STEP;
#endif
            }

            acc -= TIME_STEP;
        }

        post_fbo.bind();
        post_fbo.set_texture(this_frame);
        last_frame.set_unit(0);

        smooth_texture.bind();
        smooth_texture.upload_uniform1f("u_time_delta", deltaTime);

        screen_quad_vao.bind();

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        VAO.bind();
        basic.bind();

        VAO.buffers_data().front().set_data(
            { reinterpret_cast<float*>(sim.p), N_POINTS * 2 });

        glDrawArrays(GL_POINTS, 0, N_POINTS);

        passthrough.bind();
        post_fbo.set_texture(last_frame);
        this_frame.set_unit(0);

        screen_quad_vao.bind();

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        staplegl::framebuffer::unbind();

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    }
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    glViewport(0, 0, width, height);
}
