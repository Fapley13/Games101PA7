//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_MATERIAL_H
#define RAYTRACING_MATERIAL_H

#include "Vector.hpp"

enum MaterialType {
    DIFFUSE,
    MICRO_FACET,
    MIRROR
};

class Material{
private:
    // Compute reflection direction
    Vector3f reflect(const Vector3f &I, const Vector3f &N) const
    {
        return I - 2 * dotProduct(I, N) * N;
    }

    // Compute refraction direction using Snell's law
    //
    // We need to handle with care the two possible situations:
    //
    //    - When the ray is inside the object
    //
    //    - When the ray is outside.
    //
    // If the ray is outside, you need to make cosi positive cosi = -N.I
    //
    // If the ray is inside, you need to invert the refractive indices and negate the normal N
    Vector3f refract(const Vector3f &I, const Vector3f &N, const float &ior) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        Vector3f n = N;
        if (cosi < 0) { cosi = -cosi; } else { std::swap(etai, etat); n= -N; }
        float eta = etai / etat;
        float k = 1 - eta * eta * (1 - cosi * cosi);
        return k < 0 ? 0 : eta * I + (eta * cosi - sqrtf(k)) * n;
    }

    // Compute Fresnel equation
    //
    // \param I is the incident view direction
    //
    // \param N is the normal at the intersection point
    //
    // \param ior is the material refractive index
    //
    // \param[out] kr is the amount of light reflected
    void fresnel(const Vector3f &I, const Vector3f &N, const float &ior, float &kr) const
    {
        float cosi = clamp(-1, 1, dotProduct(I, N));
        float etai = 1, etat = ior;
        if (cosi > 0) {  std::swap(etai, etat); }
        // Compute sini using Snell's law
        float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
        // Total internal reflection
        if (sint >= 1) {
            kr = 1;
        }
        else {
            float cost = sqrtf(std::max(0.f, 1 - sint * sint));
            cosi = fabsf(cosi);
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
            kr = (Rs * Rs + Rp * Rp) / 2;
        }
        // As a consequence of the conservation of energy, transmittance is given by:
        // kt = 1 - kr;
    }

public:
    static Vector3f toWorld(const Vector3f &a, const Vector3f &N){
        Vector3f B, C;
        if (std::fabs(N.x) > std::fabs(N.y)){
            float invLen = 1.0f / std::sqrt(N.x * N.x + N.z * N.z);
            C = Vector3f(N.z * invLen, 0.0f, -N.x *invLen);
        }
        else {
            float invLen = 1.0f / std::sqrt(N.y * N.y + N.z * N.z);
            C = Vector3f(0.0f, N.z * invLen, -N.y *invLen);
        }
        B = crossProduct(C, N);
        return a.x * B + a.y * C + a.z * N;
    }

public:
    MaterialType m_type;
    //Vector3f m_color;
    Vector3f m_emission;
    float ior;
    Vector3f Kd, Ks;
    float specularExponent;
    float roughness;
    float metallic;
    //Texture tex;

    inline Material(MaterialType t=DIFFUSE, Vector3f e=Vector3f(0,0,0));
    inline MaterialType getType();
    //inline Vector3f getColor();
    inline Vector3f getColorAt(double u, double v);
    inline Vector3f getEmission();
    inline bool hasEmission();

    // sample a ray by Material properties
    inline Vector3f sample(const Vector3f &w_out, const Vector3f &N);
    // given a ray, calculate the PdF of this ray
    inline float pdf(const Vector3f &w_light, const Vector3f &w_out, const Vector3f &N);
    // given a ray, calculate the contribution of this ray
    inline Vector3f eval(const Vector3f &w_light, const Vector3f &w_out, const Vector3f &N, bool is_dir);

};

Material::Material(MaterialType t, Vector3f e){
    m_type = t;
    //m_color = c;
    m_emission = e;
}

MaterialType Material::getType(){return m_type;}
///Vector3f Material::getColor(){return m_color;}
Vector3f Material::getEmission() {return m_emission;}
bool Material::hasEmission() {
    if (m_emission.norm() > EPSILON) return true;
    else return false;
}

Vector3f Material::getColorAt(double u, double v) {
    return Vector3f();
}

inline Vector3f FyReflect(const Vector3f& a, const Vector3f& n) {
    return 2.f * n * dotProduct(n, a) - a;
}

Vector3f Material::sample(const Vector3f &w_out, const Vector3f &N) {
    switch(m_type){
        case DIFFUSE:
        {
            // uniform sample on the hemisphere
            float x_1 = get_random_float();
            float x_2 = get_random_float();
            float theta = std::acos(1.f - x_1);
            float phi = 2 * M_PI * x_2;
            float sin_theta = std::sin(theta);
            Vector3f localRay(sin_theta*std::cos(phi), sin_theta*std::sin(phi), std::cos(theta));
            return toWorld(localRay, N);
            break;
        }
        case MICRO_FACET:
        {
            float x_1 = get_random_float();
            float x_2 = get_random_float();
            float alpha2 = roughness * roughness; alpha2 *= alpha2;
            float phi = 2.f * M_PI * x_1;
            float cos_theta2 = (1.f - x_2) / ((alpha2 - 1.f)*x_2 + 1.f);
            float cos_theta = std::sqrt(cos_theta2);
            float sin_theta = std::sqrt(1.f-cos_theta2);
            Vector3f micro_normal(sin_theta*std::cos(phi), sin_theta*std::sin(phi), cos_theta);
            micro_normal = toWorld(micro_normal, N);
            return FyReflect(w_out, micro_normal);
        }
        case MIRROR:
        {
            return FyReflect(w_out, N);
        }
    }
    return Vector3f(0.0f);
}

float Material::pdf(const Vector3f &w_light, const Vector3f &w_out, const Vector3f &N){
    switch(m_type){
        case DIFFUSE:
        {
            // uniform sample probability 1 / (2 * PI)
            if (dotProduct(w_out, N) > 0.0f)
                return 0.5f / M_PI;
            else
                return 0.0f;
            break;
        }
        case MICRO_FACET:
        {
            return 1.f;
        }
        case MIRROR:
        {
            return 1.f;
        }
    }
    return 0.0f;
}

inline float GGX(float nh, float roughness) {
    float alpha2 = roughness * roughness; alpha2 *= alpha2;
    float tmp = nh * nh * (alpha2 - 1.f) + 1.f;
    float denominator = M_PI * tmp * tmp;
    return alpha2 / denominator;
}

inline float GeoOcc_(float nl, float nv, float roughness) {
    float tmp = roughness + 1.f;
    float k = tmp * tmp * .125f;
    float denominator_v = nv * (1.f-k) + k;
    float denominator_l = nl * (1.f-k) + k;
    return 1.f / (denominator_l * denominator_v);
}

inline Vector3f FyFresnel(const Vector3f& albedo, float hv, float metallic) {
    float powed = std::pow(1.f-hv, 5.f);
    Vector3f f0 = lerp(Vector3f(.04f), albedo, metallic);
    Vector3f fresnel = f0 * (1.f - powed) + Vector3f(powed);
    return fresnel;
}
inline Vector3f FyFresnelStrange(const Vector3f& albedo, float hv, float metallic) {
    float powed = std::pow(1.f-hv, 5.f);
    Vector3f x = albedo * Vector3f(1.f-powed);
    Vector3f powv = Vector3f(powed);
    Vector3f metal = x + powv;
    Vector3f nonmetal = x * .04f + albedo * powv;
    Vector3f fresnel = metal * metallic + nonmetal * (1.f - metallic);
    return fresnel;
}

Vector3f Material::eval(const Vector3f &w_light, const Vector3f &w_out, const Vector3f &N, bool is_dir){
    switch(m_type){
        case DIFFUSE:
        {
            // calculate the contribution of diffuse   model
            float cosalpha = dotProduct(N, w_light);
            if (cosalpha > 0.0f) {
                Vector3f diffuse = Kd / M_PI * cosalpha;
                return diffuse;
            }
            else
                return Vector3f(0.0f);
            break;
        }
        case MICRO_FACET:
        {
            float nl = std::max(0.f, std::min(1.0f, dotProduct(w_light, N)));
            float nv = dotProduct(w_out, N);
            Vector3f h = (w_light + w_out).normalized();
            float nh = dotProduct(N, h);
            float hv = dotProduct(h, w_out);
            float geo = GeoOcc_(nl, nv, roughness);
            Vector3f fresnel = FyFresnel(Kd, hv, metallic);
            float ggx = GGX(nh, roughness);
            float pdf = nh * .25f / nv * hv * ggx;
            Vector3f diffuse = fresnel * nl / M_PI * (1.f - metallic);
            if (is_dir) {
                return ggx * geo * fresnel * 0.25f + diffuse;
            } else return fresnel * geo * hv * nv / nh + diffuse / pdf;
        }
        case MIRROR:
        {
            Vector3f h = (w_light + w_out).normalized();
            float hv = dotProduct(h, w_out);
            Vector3f fresnel = FyFresnel(Kd, hv, metallic);
            return Kd * fresnel;
        }
    }
    return Vector3f(1.0);
}

namespace DiffOnly {
    
    inline Vector3f sample(const Vector3f &w_out, const Vector3f &N) {
        float x_1 = get_random_float();
        float x_2 = get_random_float();
        float theta = std::acos(1.f - x_1);
        float phi = 2 * M_PI * x_2;
        float sin_theta = std::sin(theta);
        Vector3f localRay(sin_theta*std::cos(phi), sin_theta*std::sin(phi), std::cos(theta));
        return Material::toWorld(localRay, N);
    }
}

#endif //RAYTRACING_MATERIAL_H
