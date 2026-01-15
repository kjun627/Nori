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

#include <nori/bsdf.h>
#include <nori/frame.h>
#include <nori/warp.h>

NORI_NAMESPACE_BEGIN

class Microfacet : public BSDF {
public:
    Microfacet(const PropertyList &propList) {
        /* RMS surface roughness */
        m_alpha = propList.getFloat("alpha", 0.1f);

        /* Interior IOR (default: BK7 borosilicate optical glass) */
        m_intIOR = propList.getFloat("intIOR", 1.5046f);

        /* Exterior IOR (default: air) */
        m_extIOR = propList.getFloat("extIOR", 1.000277f);

        /* Albedo of the diffuse base material (a.k.a "kd") */
        m_kd = propList.getColor("kd", Color3f(0.5f));

        /* To ensure energy conservation, we must scale the 
           specular component by 1-kd. 

           While that is not a particularly realistic model of what 
           happens in reality, this will greatly simplify the 
           implementation. Please see the course staff if you're 
           interested in implementing a more realistic version 
           of this BRDF. */
        m_ks = 1 - m_kd.maxCoeff();
    }

    /// Evaluate the BRDF for the given pair of directions
    Color3f eval(const BSDFQueryRecord &bRec) const {
        //microfacit half vector 계산
    	Vector3f half = (bRec.wi + bRec.wo).normalized();
        // 여기서의 각도계산
        float cosThetaI = Frame::cosTheta(bRec.wi);
        float cosThetaO = Frame::cosTheta(bRec.wo);
        float cosThetaH = Frame::cosTheta(half);    

        if(cosThetaI <= 0 || cosThetaO <= 0 || cosThetaH <= 0) return Color3f(0.0f);

        Color3f diffuse = m_kd / M_PI;

        float D = Warp::squareToBeckmannPdf(half,m_alpha);
        float fr = fresnel(half.dot(bRec.wi), m_extIOR, m_intIOR);
        float g = G1(bRec.wi, half) * G1(bRec.wo, half);

        float spec = (D * fr * g) / (4.0f * cosThetaI * cosThetaO * cosThetaH);

        return diffuse + m_ks * spec;

    }

    /// Evaluate the sampling density of \ref sample() wrt. solid angles
    float pdf(const BSDFQueryRecord &bRec) const {
        if (Frame::cosTheta(bRec.wi) <= 0 || Frame::cosTheta(bRec.wo) <= 0)
            return 0.0f;
        
        // Half vector 계산
        Vector3f wh = (bRec.wi + bRec.wo).normalized();
        
        // 자코비안 Jh = 1 / (4 * (wh · wo))
        float whDotWo = wh.dot(bRec.wo);
        if (whDotWo <= 0)
            return 0.0f;
        
        float Jh = 1.0f / (4.0f * whDotWo);
        
        // PDF 계산
        float specularPdf = m_ks * Warp::squareToBeckmannPdf(wh, m_alpha) * Jh;
        float diffusePdf = (1.0f - m_ks) * Frame::cosTheta(bRec.wo) * INV_PI;
        
        return specularPdf + diffusePdf;
    }

    /// Sample the BRDF
    Color3f sample(BSDFQueryRecord &bRec, const Point2f &_sample) const {
        if (Frame::cosTheta(bRec.wi) <= 0)
            return Color3f(0.0f);
        
        bRec.measure = ESolidAngle;
        
        Point2f sample = _sample;
        
        // sampelx 과 ks를 비교하여 diffuse vs specular 결정
        if (sample.x() < m_ks) {
            // Specular reflection
            
            // sample x 을 재사용하기 위해 크기 조정 (리매핑)
            sample.x() = sample.x() / m_ks;
            
            // Beckmann 에서 half vector 샘플링
            Vector3f wh = Warp::squareToBeckmann(sample, m_alpha);
            
            // Half vector를 사용하여 입사 방향을 반사시켜 출사 방향 생성
            bRec.wo = 2.0f * wh.dot(bRec.wi) * wh - bRec.wi;
            
            // 반구 아래 예외 처리
            if (Frame::cosTheta(bRec.wo) <= 0)
                return Color3f(0.0f);
            
        } else {
            // Diffuse reflection
            
            
            sample.x() = (sample.x() - m_ks) / (1.0f - m_ks);
            
            // weight cosin sampling
            bRec.wo = Warp::squareToCosineHemisphere(sample);
        }
        
        
        bRec.eta = 1.0f;
        
        // BRDF value / pdf * cos
        float pdfValue = pdf(bRec);
        if (pdfValue <= 0)
            return Color3f(0.0f);
        
        return eval(bRec) * Frame::cosTheta(bRec.wo) / pdfValue;
    }

    bool isDiffuse() const {
        /* While microfacet BRDFs are not perfectly diffuse, they can be
           handled by sampling techniques for diffuse/non-specular materials,
           hence we return true here */
        return true;
    }

    std::string toString() const {
        return tfm::format(
            "Microfacet[\n"
            "  alpha = %f,\n"
            "  intIOR = %f,\n"
            "  extIOR = %f,\n"
            "  kd = %s,\n"
            "  ks = %f\n"
            "]",
            m_alpha,
            m_intIOR,
            m_extIOR,
            m_kd.toString(),
            m_ks
        );
    }
private:
    float m_alpha;
    float m_intIOR, m_extIOR;
    float m_ks;
    Color3f m_kd;

    float G1(const Vector3f& wv, const Vector3f& wh) const {
        
        float c = wv.dot(wh) / Frame::cosTheta(wv);
        if (c <= 0) return 0.0f;
        
        float tanThetaV = Frame::tanTheta(wv);
        if (std::isinf(tanThetaV)) return 0.0f;
        
        float b = 1.0f / (m_alpha * tanThetaV);
        
        if (b < 1.6f) {
            float b2 = b * b;
            return (3.535f * b + 2.181f * b2) / 
                   (1.0f + 2.276f * b + 2.577f * b2);
        }
        return 1.0f;
    }
};

NORI_REGISTER_CLASS(Microfacet, "microfacet");
NORI_NAMESPACE_END
