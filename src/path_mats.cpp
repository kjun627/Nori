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

/**
 * \brief Whitted-style ray tracer with direct illumination
 */
class MaterialIntegrator : public Integrator {
public:
    MaterialIntegrator(const PropertyList &props) {
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const override {  
        Color3f totalThroughPut(1.0f); // 누적 투과율
        Color3f radiacne(0.0f); // 초기에 광량은 없어서 까맘
        Ray3f currentRay = ray;
        float eta = 1.0f; // 굴절률 추적  1.0= air

        for (int depth = 0; ; depth++){
            Intersection its;
            if(!scene->rayIntersect(currentRay, its)) break;
            if(its.mesh->isEmitter()){ // 교차한 메시 중에 광원 메시 가져오는 부분
                Color3f emitterRadiacne = its.mesh->getEmitter()->eval(its); // eval() 광원의 Radiance 가져오는 부분
                radiacne += totalThroughPut * emitterRadiacne; 
            }

            const BSDF* bsdf = its.mesh->getBSDF(); // BSDF 가져오기
            BSDFQueryRecord bRec(its.toLocal(-currentRay.d)); // 광선 방향
            Color3f bsdfVal = bsdf->sample(bRec, sampler->next2D()); // BSDF 샘플링
            // BSDF(wi, wo) * cos \theta / pdf(wo)
            
            if(bsdfVal.isZero()) break;

            totalThroughPut *= bsdfVal; // update throughput
            
            if(bRec.measure == EDiscrete) eta *= bRec.eta;

            float continuePropability = std::min(totalThroughPut.maxCoeff() * eta * eta, 0.99f); // 러시안 룰렛
            if(random() > continuePropability) break;

            Vector3f newDir = its.toWorld(bRec.wo); // nextRay
            currentRay = Ray3f(its.p, newDir);
            currentRay.mint = Epsilon;
        }
        return radiacne;
    }      
        
    std::string toString() const override {
        return "MaterialIntegrator[]";
    }
};

NORI_REGISTER_CLASS(MaterialIntegrator,"path_mats");
NORI_NAMESPACE_END
