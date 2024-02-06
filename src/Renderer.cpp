#include <fstream>
#include <iostream>
#include <string>
#include "Scene.hpp"
#include "Renderer.hpp"
#include <thread>
#include <mutex>

inline float deg2rad(const float& deg) { return deg * M_PI / 180.f; }

const float EPSILON = 0.00001f;
std::mutex framebufferMutex;

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene, int rt_spp)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);
    float scale = tan(deg2rad(scene.fov * 0.5f));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);
    int m = 0;

    // change the spp value to change sample ammount
    int spp = rt_spp;
    std::cout << "SPP: " << spp << "\n";
    
    for (int j = 0; j < scene.height; ++j) {
        for (int i = 0; i < scene.width; ++i) {
            // generate primary ray direction
            float x = (2.f * (i + 0.5f) / (float)scene.width - 1.f) *
                      imageAspectRatio * scale;
            float y = (1.f - 2.f * (j + 0.5f) / (float)scene.height) * scale;

            Vector3f dir = normalize(Vector3f(-x, y, 1.f));
            for (int k = 0; k < spp; k++){
                framebuffer[m] += scene.castRay(Ray(eye_pos, dir), 0) / (spp*1.f);  
            }
            m++;
        }
        UpdateProgress(j / (float)scene.height);
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binarySPP2.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);    
}


void Renderer::RenderMultiThread(const Scene& scene, int rt_spp)
{
    float scale = tan(deg2rad(scene.fov * 0.5f));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(278, 273, -800);

    std::vector<Vector3f> framebuffer(scene.width * scene.height);
    int spp = rt_spp;
    std::cout << "SPP: " << spp << "\n";
    int num_threads = 16;
    std::vector<std::thread> threads;
    int line_group_num = scene.height / num_threads;

    for (int thr = 0; thr < num_threads; ++thr) {
        int j_start = thr * line_group_num;
        // multi thread render
        threads.push_back(std::thread([&](int j_begin){
            int m = j_begin * scene.width;
            int j_end = j_begin + line_group_num;
            for (int j = j_begin; j < j_end; ++j) {
                for (int i = 0; i < scene.width; ++i) {
                    // generate primary ray direction
                    float x = (2.f * (i + 0.5f) / (float)scene.width - 1) *
                                imageAspectRatio * scale;
                    float y = (1.f - 2 * (j + 0.5f) / (float)scene.height) * scale;

                    Vector3f dir = normalize(Vector3f(-x, y, 1));
                    Vector3f res_col(0);
                    for (int k = 0; k < spp; k++){
                        res_col += scene.castRay(Ray(eye_pos, dir), 0);
                    }
                    // for (int k = 0; k < spp; k++){
                    //     res_col += scene.castRayDiff(Ray(eye_pos, dir), 0);
                    // }
                    framebufferMutex.lock();
                    framebuffer[m] = res_col / (spp*1.f);
                    framebufferMutex.unlock();  
                    m++;
                }
            }
        }, j_start));
    }
    std::cout << "waiting for thread end: " << spp << std::endl;
    for (int thr = 0; thr < num_threads; ++thr) {
        threads[thr].join();
        UpdateProgress(thr / (float)num_threads);
    }
    UpdateProgress(1.f);

    std::string img_file_name = "./build/SPP" + std::to_string(spp) + ".ppm";
    const char* img_const_name = img_file_name.c_str();
    std::cout << img_const_name << std::endl;
    std::cout << "writing to file " << img_const_name << ": " << spp << std::endl;
    FILE* fp = fopen(img_const_name, "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    int num_pixels = scene.height * scene.width;
    for (auto i = 0; i < num_pixels; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
        if (i % scene.width == 0) {
            UpdateProgress((float)i / num_pixels);
        }
    }
    UpdateProgress(1.f);
    fclose(fp);
}
