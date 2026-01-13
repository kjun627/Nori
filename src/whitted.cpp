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
        Intersection its; // Intersection 정보 담을 변수
        Color3f Le = (0.0f); // 직접 발광체에서 나오는 광량계산용
        if(!scene->rayIntersect(ray,its)) return Color3f(0.0f); 
        if(its.mesh->isEmitter()) { //ray랑 메쉬가 부딛혔으면 해당 메쉬가 광원인지 확인
            Le = its.mesh->getEmitter()->eval(its); // 광원이면 해당 radiance 대입
        }else{
            Le = Color3f(0.0f); // 그게 아니면 발광 없음
        }
        
        std::vector<Mesh*> lights; // light 정보 담을 벡터

        const std::vector<Mesh*>& meshes = scene->getMeshes(); // 일단 Scene 내에 모든 메쉬정보를 받고
        for (Mesh* mesh : meshes){ // 그 중에서 광원인 메쉬만 선택적으로 light vector에 삽입
            if(mesh->isEmitter()){
                lights.push_back(mesh);
            }
        }

        if(lights.empty()){
            return Le; //light가 없는 경우 그냥 진행
        }

        float randomSample = sampler->next1D(); // sample [0,1] 하나 받아오기 1D
        int lightIndex = std::min((int)(randomSample * lights.size()), (int)lights.size()-1); // 랜덤하게 light 하나 선택
        Mesh* selectedLight = lights[lightIndex]; 
        float pdfLight = 1.0f / (float)lights.size(); // pdf 는 해당 광원의 1/광원개수
        
        Point2f sample2D = sampler->next2D(); // sample [0,1] 두개 받아오기 2D
        Point3f y; 
        Normal3f ny;
        float pdfArea;

        // 그 선택된 광원에서의 barycentric 좌표, normal, pdf 등등 값 받기
        selectedLight->sampleSurface(sample2D, y, ny, pdfArea);
                
        Vector3f dir = (y - its.p).normalized(); //p->y 방향 벡터 설정
        float dist = (y - its.p).norm();//p->y 거리
        
        float cosinX = std::abs(its.shFrame.n.dot(dir)); // 방향벡터와 충돌지점에서의 cosin X 값
        float cosinY = std::abs(ny.dot(-dir));//샘플 포인트의 노말과 방향벡터의 cosin값
        float G = cosinX * cosinY / (dist * dist); // 거리 제곱 상쇄
        
        
        Ray3f shadowRay(its.p, dir); // shadow ray 준비
        shadowRay.mint = Epsilon; 
        shadowRay.maxt = dist - Epsilon;
        
        if(scene->rayIntersect(shadowRay)){
            return Le; // 가려졌으면 광원에 대한 radiance만 반환 
        } 
        // 입사 방향, 출사 방향 BSDF 
        BSDFQueryRecord bRec(its.toLocal(dir), its.toLocal(-ray.d), ESolidAngle);
        Color3f fr = its.mesh->getBSDF()->eval(bRec); //반사율 계싼
        
        Color3f LeY = selectedLight->getEmitter()->eval(its); // 선택된 광원에서 오는 광량
        
        float pdf_light = pdfLight;
        float pdf_area = pdfArea;
        float pdf_total = pdf_light * pdf_area;
        
        Color3f result = fr * G * LeY / pdf_total; // BRDF * Geo Term * radiance / pdf
        result += Le; // 해당 물체가 직접 조명을 내는 광원체라면 그 광량까지 반영
        
        return result;  
    }

    std::string toString() const override {
        return "WhittedIntegrator[]";
    }
};

NORI_REGISTER_CLASS(WhittedIntegrator, "whitted");
NORI_NAMESPACE_END
