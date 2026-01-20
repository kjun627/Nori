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
#include <cmath>

NORI_NAMESPACE_BEGIN

class NEEIntegrator : public Integrator {
public:
    NEEIntegrator(const PropertyList &props) {
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const override {
        Color3f totalThroughPut(1.0f); // 누적 투과율
        Color3f radiacne(0.0f); // 초기에 광량은 없어서 까맘
        Ray3f currentRay = ray;
        float eta = 1.0f; // 굴절률 추적  1.0= air
        bool lastBounceWasDelta = true; // double count 방지 flag

        std::vector<Mesh*> emitters; // 광원에 대한 메시를 여기에 누적
        for(Mesh* mesh : scene->getMeshes()){
            if(mesh->isEmitter()){
                emitters.push_back(mesh);
            }
        }

        for (int depth = 0; ; depth++){
            Intersection its;
            if(!scene->rayIntersect(currentRay, its)) break;

            if(its.mesh->isEmitter()){ // 상황 1 광원 hit 처리에 대한 내용
                if(depth == 0 || lastBounceWasDelta){
                    Color3f emitterRadiacne = its.mesh->getEmitter()->eval(its);
                    radiacne += totalThroughPut * emitterRadiacne;
                }
                
            }

            const BSDF* bsdf = its.mesh->getBSDF(); // BSDF 가져오기
            BSDFQueryRecord bRec(its.toLocal(-currentRay.d)); // 입사 방향 (wi)

            if(bsdf->isDiffuse() && !emitters.empty()){ //상황 2 camera -> diffuse surface
                int idx = sampler->next1D() * emitters.size();
                Mesh* emitterMesh = emitters[idx];
                float pdfLightSelection = 1.0f / emitters.size(); // 광원하나 샘플링 Pdf 
                Point3f lightP;
                Normal3f lightN;
                float pdfSurface;

                emitterMesh->sampleSurface(sampler->next2D(), lightP, lightN, pdfSurface); // 광원ㅇ서의 포면 점 샘플링
                Vector3f toLgiht = lightP - its.p;
                float dist = toLgiht.norm();
                Vector3f wo = toLgiht.normalized();

                Ray3f shadowRay(its.p, wo);
                shadowRay.mint = Epsilon;
                shadowRay.maxt = dist - Epsilon;
                if(!scene->rayIntersect(shadowRay)){ // 만약 shadow ray가 안부딛힘 
                    float cosThetalight = std::max(0.0f, lightN.dot(-wo));// 샘플링 표면의 light의 normal과 BSDF 나가는 방향과의 내적
                    float cosThetaSurface = std::max(0.0f, its.shFrame.n.dot(wo));

                    if(cosThetalight >0 && cosThetaSurface > 0){
                        Intersection lightIts;
                        lightIts.p = lightP;
                        lightIts.shFrame.n = lightN;   
                        Color3f Le = emitterMesh->getEmitter()->eval(lightIts);
                        
                        BSDFQueryRecord bRec(its.toLocal(-currentRay.d), its.toLocal(wo), ESolidAngle);
                        Color3f bsdfVal = bsdf->eval(bRec);

                        float G = cosThetalight / (dist * dist);
                        Color3f contribution = Le * bsdfVal * cosThetaSurface * G / (pdfSurface * pdfLightSelection);
                        // contribution 계산
                        radiacne += totalThroughPut * contribution; 
                    }
                
                } 


            }
            Color3f bsdfVal = bsdf->sample(bRec, sampler->next2D()); //다음 방향에 대한 BSDF 샘플링
            // 반환값: BSDF(wi, wo) * cos(theta) / pdf(wo)

            if(bsdfVal.isZero()) break;

            totalThroughPut *= bsdfVal; // update throughput

            lastBounceWasDelta = (bRec.measure == EDiscrete);

            if(bRec.measure == EDiscrete) eta *= bRec.eta;
            if(depth >= 3){
                float continuePropability = std::min(totalThroughPut.maxCoeff() * eta * eta, 0.99f);
                if(sampler->next1D() > continuePropability) break;
                totalThroughPut /= continuePropability;
            }


            Vector3f newDir = its.toWorld(bRec.wo); // 다음 광선 방향
            currentRay = Ray3f(its.p, newDir);
            currentRay.mint = Epsilon;
        }
        return radiacne;
    }

    std::string toString() const override {
        return "NEEIntegrator[]";
    }
};

NORI_REGISTER_CLASS(NEEIntegrator,"path_ems");
NORI_NAMESPACE_END
