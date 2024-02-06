//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"
#include <iostream>

void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here
    // 求着色点
    Intersection hit = intersect(ray);
    if (!hit.happened) return depth == 0 ? backgroundColor : Vector3f(0.f);
    if (hit.obj->hasEmit()) return hit.emit;
    Vector3f res_dir = Vector3f(0.0);
    Material* shadingPMaterial = hit.m;
    Vector3f w_out = -(ray.direction);
    Vector3f shadingPNormal = hit.normal;
    float dist_to_eye = hit.distance;

    // 判断有没有直接光照
    Intersection posL = Intersection();
    float pdf = 0.0f;
    sampleLight(posL, pdf);
    Vector3f light_dir = (posL.coords - hit.coords).normalized();
    Ray lightRay = Ray(hit.coords, light_dir);
    float dist = (posL.coords - hit.coords).norm();
    Intersection obj_occlusion = intersect(lightRay);
    // 如果光源和着色点相交，就采样直接光照
    if (dist - obj_occlusion.distance < 0.001f && pdf > 0.0f && shadingPMaterial->m_type != MIRROR) {
        Vector3f fr = shadingPMaterial->eval(light_dir, w_out, shadingPNormal, true);
        float nl = std::max(0.0f, dotProduct(shadingPNormal, light_dir));

        Vector3f light_normal = posL.normal;
        float nll = std::max(0.0f, dotProduct(light_normal, -light_dir));

        Vector3f lightEmit = posL.emit * nl * nll * fr / (pdf * dist * dist);
        res_dir += lightEmit;
    }
    // 采样间接光照
    Vector3f res_ind = Vector3f(0.f);
    float Prr = get_random_float();
    if (Prr < RussianRoulette) {
        Vector3f wl = shadingPMaterial->sample(w_out, shadingPNormal).normalized();
        float nl = dotProduct(shadingPNormal, wl);
        if (nl > EPSILON) {
            Vector3f ray_orig_ind = hit.coords + shadingPNormal * .01f;
            Vector3f castLi = castRay(Ray(ray_orig_ind, wl), depth+1);
            Vector3f weight_or_frdotnl = shadingPMaterial->eval(wl, w_out, shadingPNormal, false);
            float pdf = shadingPMaterial->pdf(wl, w_out, shadingPNormal);
            res_ind += castLi * weight_or_frdotnl / (pdf * RussianRoulette);
        }
    }
    return Vector3f::Min(Vector3f(1), Vector3f::Max(Vector3f(0), (res_dir+res_ind)));
}

Vector3f Scene::castRayDiff(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here
    // 求着色点
    Intersection hit = intersect(ray);
    if (!hit.happened) return depth == 0 ? backgroundColor : Vector3f(0.f);
    if (hit.obj->hasEmit()) return hit.emit;
    Vector3f res_dir = Vector3f(0.0);
    Vector3f diff_kd = hit.m->Kd;
    float metallic = hit.m->metallic;
    Vector3f w_out = -(ray.direction);
    Vector3f shadingPNormal = hit.normal;
    float dist_to_eye = hit.distance;

    // 判断有没有直接光照
    Intersection posL = Intersection();
    float pdf = 0.0f;
    sampleLight(posL, pdf);
    Vector3f light_dir = (posL.coords - hit.coords).normalized();
    Ray lightRay = Ray(hit.coords, light_dir);
    float dist = (posL.coords - hit.coords).norm();
    Intersection obj_occlusion = intersect(lightRay);
    // 如果光源和着色点相交，就采样直接光照
    if (dist - obj_occlusion.distance < 0.001f && pdf > 0.0f) {
        Vector3f fr = diff_kd / M_PI;
        float nl = std::max(0.0f, dotProduct(shadingPNormal, light_dir));

        Vector3f light_normal = posL.normal;
        float nll = std::max(0.0f, dotProduct(light_normal, -light_dir));

        Vector3f lightEmit = posL.emit * nl * nll * fr / (pdf * dist * dist);
        res_dir += lightEmit;
    }
    // 采样间接光照
    Vector3f res_ind = Vector3f(0.f);
    float Prr = get_random_float();
    if (Prr < RussianRoulette) {
        Vector3f wl = DiffOnly::sample(w_out, shadingPNormal).normalized();
        float nl = dotProduct(shadingPNormal, wl);
        if (nl > EPSILON) {
            Vector3f ray_orig_ind = hit.coords + shadingPNormal * .01f;
            Vector3f castLi = castRay(Ray(ray_orig_ind, wl), depth+1);
            Vector3f weight_or_frdotnl = diff_kd / M_PI;
            float pdf = .5f / M_PI;
            res_ind += castLi * weight_or_frdotnl * nl / (pdf * RussianRoulette); 
        }
    }
    return Vector3f::Min(Vector3f(1), Vector3f::Max(Vector3f(0), (res_dir+res_ind))) * (1.f-metallic);
}
