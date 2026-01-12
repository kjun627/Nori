/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>
#include <nori/emitter.h>

NORI_NAMESPACE_BEGIN

/**
 * \brief Whitted-style ray tracer with direct illumination
 */
class WhittedIntegrator : public Integrator {
public:
    WhittedIntegrator(const PropertyList &props) {
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const override {
        // TODO: Distribution ray tracing 구현
        
        Intersection its;
        Color3f Le = (0.0f);
        if(!scene->rayIntersect(ray,its)) return Color3f(0.0f);
        if(its.mesh->isEmitter()) {
            Le = its.mesh->getEmitter()->eval(its);
        }else{
            Le = Color3f(0.0f);
        }
        
        std::vector<Mesh*> lights;

        const std::vector<Mesh*>& meshes = scene->getMeshes();
        for (Mesh* mesh : meshes){
            if(mesh->isEmitter()){
                lights.push_back(mesh);
            }
        }

        if(lights.empty()){
            return Le;
        }

        float randomSample = sampler->next1D();
        int lightIndex = std::min((int)(randomSample * lights.size()), (int)lights.size()-1);
        Mesh* selectedLight = lights[lightIndex];
        float pdfLight = 1.0f / (float)lights.size();
        
        Point2f sample2D = sampler->next2D();
        Point3f y;
        Normal3f ny;
        float pdfArea;

        selectedLight->sampleSurface(sample2D, y, ny, pdfArea);
                
        Vector3f dir = (y - its.p).normalized();  
        float dist = (y - its.p).norm();          
        
        float cosinX = std::abs(its.shFrame.n.dot(dir));
        float cosinY = std::abs(ny.dot(-dir));          
        float G = cosinX * cosinY / (dist * dist);
        
        
        Ray3f shadowRay(its.p, dir);
        shadowRay.mint = Epsilon;
        shadowRay.maxt = dist;
        
        if(scene->rayIntersect(shadowRay)){
            return Le;  
        }
      
        BSDFQueryRecord bRec(its.toLocal(dir), its.toLocal(-ray.d), ESolidAngle);
        Color3f fr = its.mesh->getBSDF()->eval(bRec);
        
        Color3f LeY = selectedLight->getEmitter()->eval(its);
        
        float pdf_light = pdfLight;
        float pdf_area = pdfArea;
        float pdf_total = pdf_light * pdf_area;
        
        Color3f result = fr * G * LeY / pdf_total;
        result += Le;
        
        return result;  
    }

    std::string toString() const override {
        return "WhittedIntegrator[]";
    }
};

NORI_REGISTER_CLASS(WhittedIntegrator, "whitted");
NORI_NAMESPACE_END
