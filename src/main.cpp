#include "Renderer.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "Sphere.hpp"
#include "Vector.hpp"
#include "global.hpp"
#include <chrono>

// In the main function of the program, we create the scene (create objects and
// lights) as well as set the options for the render (image width and height,
// maximum recursion depth, field-of-view, etc.). We then call the render
// function().
int main(int argc, char** argv)
{
    // Change the definition here to change resolution
    Scene scene(784, 784);
    float roughness_all = .33f;
    float metallic_all = .5f;
    Material* red = new Material(MICRO_FACET, Vector3f(0.0f));
    red->Kd = Vector3f(0.63f, 0.065f, 0.05f);
    red->roughness = roughness_all;
    red->metallic = metallic_all;
    Material* green = new Material(MICRO_FACET, Vector3f(0.0f));
    green->Kd = Vector3f(0.14f, 0.45f, 0.091f);
    green->roughness = roughness_all;
    green->metallic = metallic_all;
    Material* white = new Material(MICRO_FACET, Vector3f(0.0f));
    white->Kd = Vector3f(0.725f, 0.71f, 0.68f);
    white->roughness = roughness_all;
    white->metallic = metallic_all;
    Material* light = new Material(MICRO_FACET, (8.0f * Vector3f(0.747f+0.058f, 0.747f+0.258f, 0.747f) + 15.6f * Vector3f(0.740f+0.287f,0.740f+0.160f,0.740f) + 18.4f *Vector3f(0.737f+0.642f,0.737f+0.159f,0.737f)));
    light->Kd = Vector3f(0.65f);
    light->roughness = roughness_all;
    light->metallic = metallic_all;
    light->metallic = metallic_all;
    Material* m_mir = new Material(MIRROR, Vector3f(0.0f));
    m_mir->Kd = Vector3f(1.f);
    Material* m_micro_face = new Material(MICRO_FACET, Vector3f(0.f));
    m_micro_face->Kd = Vector3f(0.78f, 0.78f, 0.78f);
    m_micro_face->roughness = roughness_all;
    m_micro_face->metallic = metallic_all;

    MeshTriangle floor("./models/cornellbox/floor556.obj", white);
    MeshTriangle shortbox("./models/cornellbox/shortbox9dot9.obj", m_micro_face);
    MeshTriangle tallbox("./models/cornellbox/tallbox.obj", m_micro_face);
    MeshTriangle left("./models/cornellbox/left556.obj", red);
    MeshTriangle right("./models/cornellbox/right.obj", green);
    MeshTriangle light_("./models/cornellbox/light.obj", light);
    Sphere smooth_sph(Vector3f(174.5f, 230.f, 170.f), 60.f, m_micro_face);

    scene.Add(&floor);
    scene.Add(&shortbox);
    scene.Add(&tallbox);
    scene.Add(&left);
    scene.Add(&right);
    scene.Add(&light_);
    scene.Add(&smooth_sph);

    scene.buildBVH();
    Renderer r;
    int cur_i = argc == 2 ? atoi(argv[1]) : 8;
    int spp = argc > 1 ? (cur_i > 0 ? cur_i : 8) : 8;
    auto start = std::chrono::system_clock::now();
    r.RenderMultiThread(scene, spp);
    auto stop = std::chrono::system_clock::now();

    auto render_hours = std::chrono::duration_cast<std::chrono::hours>(stop - start).count();
    auto render_minutes = std::chrono::duration_cast<std::chrono::minutes>(stop - start).count();
    auto render_seconds = std::chrono::duration_cast<std::chrono::seconds>(stop - start).count();
    render_seconds = render_seconds - render_minutes * 60;
    render_minutes = render_minutes - render_hours * 60;

    std::cout << "Render complete: \n";
    std::cout << "Time taken: " << render_hours << " hours\n";
    std::cout << "          : " << render_minutes << " minutes\n";
    std::cout << "          : " << render_seconds << " seconds\n";

    return 0;
}