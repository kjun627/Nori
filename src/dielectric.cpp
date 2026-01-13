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
        float wiCosTheta = Frame::cosTheta(bRec.wi);
        float fr = fresnel(wiCosTheta, m_extIOR, m_intIOR);

        float sampleX = sample.x();
        if(sampleX < fr){
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
            bool entering = wiCosTheta > 0.0f;
            float etaI = entering ? m_extIOR : m_intIOR;  
            float etaT = entering ? m_intIOR : m_extIOR;  
            float ratio = etaI / etaT;

            float sin1Squre = 1.0f - wiCosTheta * wiCosTheta;
            float sin2Squre = ratio * ratio * sin1Squre;

            if (sin2Squre >= 1.0f){
                bRec.wo = Vector3f(-bRec.wi.x(), -bRec.wi.y(), wiCosTheta);
                bRec.measure = EDiscrete;
                bRec.eta = 1.0f;
                return Color3f(1.0f);
            }
            float cosTheta = std::sqrt(1.0f - sin2Squre);
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
