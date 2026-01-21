#include <nori/integrator.h>
#include <nori/scene.h>
#include <nori/bsdf.h>
#include <nori/sampler.h>
#include <nori/emitter.h>
#include <cmath>

NORI_NAMESPACE_BEGIN

class MISIntegrator : public Integrator {
public:
    MISIntegrator(const PropertyList &props) {
    }

    float miWeight(float pdfA, float pdfB) const {
        if(pdfA + pdfB == 0.0f) return 0.0f;
        return pdfA / (pdfA + pdfB);
    }

    float pdfAreaToSolidAngle(float pdfArea, float dist, float cosTheta) const {
        if(cosTheta <= 0.0f) return 0.0f;
        return pdfArea * dist * dist / cosTheta;
    }

    Color3f Li(const Scene *scene, Sampler *sampler, const Ray3f &ray) const override {
        Color3f throughput(1.0f);       // 누적 투과율
        Color3f radiance(0.0f);         // 최종 radiance
        Ray3f currentRay = ray;
        float eta = 1.0f;               // 굴절률 추적

        // 이전 바운스 정보 저장
        float prevPdfBSDF = 0.0f;           // 이전 BSDF 샘플링 PDF
        bool prevBounceSkipMIS = true;      // 이전 바운스에서 NEE를 안 했으면 MIS 스킵
        Point3f prevPoint;                  // 이전 교차점 

        // emitter 메시 수집
        std::vector<Mesh*> emitters;
        for (Mesh* mesh : scene->getMeshes()) {
            if (mesh->isEmitter()) {
                emitters.push_back(mesh);
            }
        }
        float pdfLightSelection = emitters.empty() ? 0.0f : 1.0f / emitters.size();

        for (int depth = 0; ; depth++) {
            Intersection its;
            if (!scene->rayIntersect(currentRay, its)) break;

            if (its.mesh->isEmitter()) {
                Color3f Le = its.mesh->getEmitter()->eval(its);

                if (depth == 0) {
                    radiance += throughput * Le;
                }
                else if (prevBounceSkipMIS) {
                    radiance += throughput * Le;
                }
                else {
                    Vector3f distVec = its.p - prevPoint;
                    float dist = distVec.norm();
                    float cosTheta = std::max(0.0f, its.shFrame.n.dot(-currentRay.d));

                    if (cosTheta > 0.0f) {
                        float pdfArea = 1.0f / its.mesh->getSurfaceArea();
                        float pdfLight = pdfAreaToSolidAngle(pdfArea, dist, cosTheta) * pdfLightSelection;
                        float weight = miWeight(prevPdfBSDF, pdfLight);
                        radiance += throughput * Le * weight;
                    }
                    // cosTheta <= 0 처리
                }
            }

            const BSDF* bsdf = its.mesh->getBSDF();
            BSDFQueryRecord bRec(its.toLocal(-currentRay.d));

            // 모든 non-delta BSDF에서 NEE 수행 
            if (!emitters.empty()) {
                int idx = std::min((int)(sampler->next1D() * emitters.size()),
                                   (int)emitters.size() - 1);
                Mesh* emitterMesh = emitters[idx];

                Point3f lightP;
                Normal3f lightN;
                float pdfArea;
                emitterMesh->sampleSurface(sampler->next2D(), lightP, lightN, pdfArea);

                Vector3f toLight = lightP - its.p;
                float dist = toLight.norm();
                Vector3f wo = toLight.normalized();

                Ray3f shadowRay(its.p, wo, Epsilon, dist - Epsilon);

                if (!scene->rayIntersect(shadowRay)) {
                    float cosThetaLight = std::max(0.0f, lightN.dot(-wo));
                    float cosThetaSurface = std::max(0.0f, its.shFrame.n.dot(wo));

                    if (cosThetaLight > 0 && cosThetaSurface > 0) {
                        //BSDF PDF 계산
                        BSDFQueryRecord bRecNEE(its.toLocal(-currentRay.d),
                                                its.toLocal(wo), ESolidAngle);
                        float pdfBSDF = bsdf->pdf(bRecNEE);

                        float pdfLightSolidAngle = pdfAreaToSolidAngle(pdfArea, dist, cosThetaLight) * pdfLightSelection;
                        float weight = miWeight(pdfLightSolidAngle, pdfBSDF);

                        Intersection lightIts;
                        lightIts.p = lightP;
                        lightIts.shFrame.n = lightN;

                        Color3f Le = emitterMesh->getEmitter()->eval(lightIts);
                        Color3f bsdfVal = bsdf->eval(bRecNEE);

                        float G = cosThetaLight / (dist * dist);
                        Color3f contribution = Le * bsdfVal * cosThetaSurface * G * weight / (pdfArea * pdfLightSelection);
                        radiance += throughput * contribution;
                    }
                }
            }
            Color3f bsdfVal = bsdf->sample(bRec, sampler->next2D());
            if (bsdfVal.isZero()) break;

            throughput *= bsdfVal;

            prevPdfBSDF = bsdf->pdf(bRec);
            prevBounceSkipMIS = (bRec.measure == EDiscrete);
            prevPoint = its.p;

            if (bRec.measure == EDiscrete) eta *= bRec.eta;
            if (depth >= 3) {
                float q = std::min(throughput.maxCoeff() * eta * eta, 0.99f);
                if (sampler->next1D() > q) break;
                throughput /= q;
            }

            Vector3f newDir = its.toWorld(bRec.wo);
            currentRay = Ray3f(its.p, newDir);
        }

        return radiance;
    }

    std::string toString() const override {
        return "MISIntegrator[]";
    }
};

NORI_REGISTER_CLASS(MISIntegrator, "path_mis");
NORI_NAMESPACE_END
