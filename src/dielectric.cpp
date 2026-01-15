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

NORI_NAMESPACE_BEGIN

/// Ideal dielectric BSDF
// 투명한 물질 시뮬레이션하는 BSDF
// 빛 반사와 굴절 모두 처리
class Dielectric : public BSDF {
public:
    Dielectric(const PropertyList &propList) {
        /* Interior IOR (default: BK7 borosilicate optical glass) */
        m_intIOR = propList.getFloat("intIOR", 1.5046f); // 내부 굴절율
        /* Exterior IOR (default: air) */
        m_extIOR = propList.getFloat("extIOR", 1.000277f); // 외부 굴절률
    }

    Color3f eval(const BSDFQueryRecord &) const {
        /* Discrete BRDFs always evaluate to zero in Nori */
        return Color3f(0.0f); 
    }

    float pdf(const BSDFQueryRecord &) const {
        /* Discrete BRDFs always evaluate to zero in Nori */
        return 0.0f;
    }

    Color3f sample(BSDFQueryRecord &bRec, const Point2f &sample) const {
        float wiCosTheta = Frame::cosTheta(bRec.wi); // 입사각에 대한 코사인 값
        float fr = fresnel(wiCosTheta, m_extIOR, m_intIOR); // 프레넬 방정식 기반으로 반사율 계산 (fr)

        float sampleX = sample.x(); // sample
        if(sampleX < fr){ // 샘플 x의 확률 값이 fr보다 작다면 그냥 일반 반사 처리
            bRec.wo = Vector3f(
            -bRec.wi.x(),
            -bRec.wi.y(),
            bRec.wi.z()
            );
            bRec.measure = EDiscrete;
            /* Relative index of refraction: no change */
            bRec.eta = 1.0f;
            return Color3f(1.0f);
        }else{
            bool entering = wiCosTheta > 0.0f; // 들어오는 각도 체크
            float etaI = entering ? m_extIOR : m_intIOR;  // incident에서의 굴절률
            float etaT = entering ? m_intIOR : m_extIOR;  // transmitted에서의 굴절률
            float ratio = etaI / etaT; // 굴절 각도계산할 때 사용

            float sin1Squre = 1.0f - wiCosTheta * wiCosTheta;
            float sin2Squre = ratio * ratio * sin1Squre; // 스넬 법치 기반 sin^2 계산

            if (sin2Squre >= 1.0f){ // 전반사 체크
                bRec.wo = Vector3f(-bRec.wi.x(), -bRec.wi.y(), wiCosTheta);
                bRec.measure = EDiscrete;
                bRec.eta = 1.0f;
                return Color3f(1.0f); // 전반사면 완전 반사 처리
            }
            float cosTheta = std::sqrt(1.0f - sin2Squre); // 여기는 정상 굴절일 때 코사인 확인
            if(wiCosTheta>0.0f) cosTheta = -cosTheta;

            bRec.wo = Vector3f(-ratio*bRec.wi.x(), -ratio*bRec.wi.y(), cosTheta);
            bRec.measure = EDiscrete;
            bRec.eta = ratio;
            return Color3f(1.0f);
        }
    }

    std::string toString() const {
        return tfm::format(
            "Dielectric[\n"
            "  intIOR = %f,\n"
            "  extIOR = %f\n"
            "]",
            m_intIOR, m_extIOR);
    }
private:
    float m_intIOR, m_extIOR;
};

NORI_REGISTER_CLASS(Dielectric, "dielectric");
NORI_NAMESPACE_END
