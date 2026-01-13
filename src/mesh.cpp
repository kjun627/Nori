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

#include <nori/mesh.h>
#include <nori/bbox.h>
#include <nori/bsdf.h>
#include <nori/emitter.h>
#include <nori/warp.h>
#include <nori/dpdf.h>
#include <Eigen/Geometry>

NORI_NAMESPACE_BEGIN

Mesh::Mesh() { }

Mesh::~Mesh() {
    delete m_bsdf;
    delete m_emitter;
}

void Mesh::activate() {
    if (!m_bsdf) {
        /* If no material was assigned, instantiate a diffuse BRDF */
        m_bsdf = static_cast<BSDF *>(
            NoriObjectFactory::createInstance("diffuse", PropertyList()));
    }
    // 삼각형의 개수만큼 버퍼할당
    m_pdf.reserve(getTriangleCount());
    m_surfaceArea = 0.0f;
    // 각각의 삼각형에 대해 크기 계산후 저장시키기
    for (uint32_t i = 0; i < m_F.cols(); i++) {
        float areaValue = surfaceArea(i);
        m_pdf.append(areaValue); // 아 여기서 일종의 CDF처럼 값이 push 되고
        m_surfaceArea += areaValue;
    }
    m_pdf.normalize(); // 그 리스트 CDF 리스트들에 대해 [0,1] 로 nornalization
}

float Mesh::surfaceArea(uint32_t index) const {
    uint32_t i0 = m_F(0, index), i1 = m_F(1, index), i2 = m_F(2, index);

    const Point3f p0 = m_V.col(i0), p1 = m_V.col(i1), p2 = m_V.col(i2);

    return 0.5f * Vector3f((p1 - p0).cross(p2 - p0)).norm();
}

bool Mesh::rayIntersect(uint32_t index, const Ray3f &ray, float &u, float &v, float &t) const {
    uint32_t i0 = m_F(0, index), i1 = m_F(1, index), i2 = m_F(2, index);
    const Point3f p0 = m_V.col(i0), p1 = m_V.col(i1), p2 = m_V.col(i2);

    /* Find vectors for two edges sharing v[0] */
    Vector3f edge1 = p1 - p0, edge2 = p2 - p0;

    /* Begin calculating determinant - also used to calculate U parameter */
    Vector3f pvec = ray.d.cross(edge2);

    /* If determinant is near zero, ray lies in plane of triangle */
    float det = edge1.dot(pvec);

    if (det > -1e-8f && det < 1e-8f)
        return false;
    float inv_det = 1.0f / det;

    /* Calculate distance from v[0] to ray origin */
    Vector3f tvec = ray.o - p0;

    /* Calculate U parameter and test bounds */
    u = tvec.dot(pvec) * inv_det;
    if (u < 0.0 || u > 1.0)
        return false;

    /* Prepare to test V parameter */
    Vector3f qvec = tvec.cross(edge1);

    /* Calculate V parameter and test bounds */
    v = ray.d.dot(qvec) * inv_det;
    if (v < 0.0 || u + v > 1.0)
        return false;

    /* Ray intersects triangle -> compute t */
    t = edge2.dot(qvec) * inv_det;

    return t >= ray.mint && t <= ray.maxt;
}

BoundingBox3f Mesh::getBoundingBox(uint32_t index) const {
    BoundingBox3f result(m_V.col(m_F(0, index)));
    result.expandBy(m_V.col(m_F(1, index)));
    result.expandBy(m_V.col(m_F(2, index)));
    return result;
}

Point3f Mesh::getCentroid(uint32_t index) const {
    return (1.0f / 3.0f) *
        (m_V.col(m_F(0, index)) +
         m_V.col(m_F(1, index)) +
         m_V.col(m_F(2, index)));
}

void Mesh::sampleSurface(const Point2f &sample, Point3f &p, Normal3f &n, float &pdf) const {
    
    float sampleX = sample.x(); // [0,1] 난수 생성
    uint32_t triangleIdx = m_pdf.sampleReuse(sampleX, pdf); // 면적이 큰 삼각형 일수록 높은 확률로 선택되고 그 인덱스를 반환(CDF기반)
    // barycentic coordi 기반 좌표 - 과제 설명에 나와있음
    Point2f bary = Point2f(1-std::sqrt(1-sampleX), sample.y() *std::sqrt(1-sampleX));

    // vertex 인덱스 받아오기
    uint32_t i0 = m_F(0, triangleIdx), i1 = m_F(1, triangleIdx), i2 = m_F(2, triangleIdx);
    Point3f p0 = m_V.col(i0); // 인덱스 기반 실제 포인트 추출 
    Point3f p1 = m_V.col(i1); // 인덱스 기반 실제 포인트 추출
    Point3f p2 = m_V.col(i2); // 인덱스 기반 실제 포인트 추출
    // sample point p 에 대한 interpolation
    p = (1-bary.x()-bary.y()) * p0 + bary.x() * p1 + bary.y() * p2;

    // normal 이 존재할경우 interpolation
    if(m_N.size() > 0){
        Normal3f n0 = m_N.col(i0), n1 = m_N.col(i1), n2 = m_N.col(i2);
        n = (1-bary.x()-bary.y()) * n0 + bary.x() * n1 + bary.y() * n2;
        n = n.normalized();
    }else{
        // 없으면 노말 직접 계산해주기
        Vector3f edge1 = p1 - p0, edge2 = p2 - p0;
        n = edge1.cross(edge2).normalized();
    }
    // 1/면적으로 pdf 계산
    pdf = 1.0f/m_surfaceArea;
}

void Mesh::addChild(NoriObject *obj) {
    switch (obj->getClassType()) {
        case EBSDF:
            if (m_bsdf)
                throw NoriException(
                    "Mesh: tried to register multiple BSDF instances!");
            m_bsdf = static_cast<BSDF *>(obj);
            break;

        case EEmitter: {
                Emitter *emitter = static_cast<Emitter *>(obj);
                if (m_emitter)
                    throw NoriException(
                        "Mesh: tried to register multiple Emitter instances!");
                m_emitter = emitter;
            }
            break;

        default:
            throw NoriException("Mesh::addChild(<%s>) is not supported!",
                                classTypeName(obj->getClassType()));
    }
}

std::string Mesh::toString() const {
    return tfm::format(
        "Mesh[\n"
        "  name = \"%s\",\n"
        "  vertexCount = %i,\n"
        "  triangleCount = %i,\n"
        "  bsdf = %s,\n"
        "  emitter = %s\n"
        "]",
        m_name,
        m_V.cols(),
        m_F.cols(),
        m_bsdf ? indent(m_bsdf->toString()) : std::string("null"),
        m_emitter ? indent(m_emitter->toString()) : std::string("null")
    );
}

std::string Intersection::toString() const {
    if (!mesh)
        return "Intersection[invalid]";

    return tfm::format(
        "Intersection[\n"
        "  p = %s,\n"
        "  t = %f,\n"
        "  uv = %s,\n"
        "  shFrame = %s,\n"
        "  geoFrame = %s,\n"
        "  mesh = %s\n"
        "]",
        p.toString(),
        t,
        uv.toString(),
        indent(shFrame.toString()),
        indent(geoFrame.toString()),
        mesh ? mesh->toString() : std::string("null")
    );
}

NORI_NAMESPACE_END
