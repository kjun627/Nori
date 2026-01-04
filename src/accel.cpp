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

#include <nori/accel.h>
#include <Eigen/Geometry>

NORI_NAMESPACE_BEGIN
BoundingBox3f getChildBoundingBox(const BoundingBox3f& parent, int octant){
        Point3f center = parent.getCenter();
        Point3f pMin = parent.min;
        Point3f pMax = parent.max;

        Point3f chMin, chMax;

        // x 축에 대해서 위치 할당
        if (octant & 1){
            chMin.x() = center.x();
            chMax.x() = pMax.x();
        }else{
            chMin.x() = pMin.x();
            chMax.x() = center.x();
        }

        // y 축에 대해서 위치 할당
        if(octant&2){
            chMin.y() = center.y();
            chMax.y() = pMax.y();
        }else{
            chMin.y() = pMin.y();
            chMax.y() = center.y();
        }

        // z 축에 대해서 위치 할당
        if(octant&4){
            chMin.z() = center.z();
            chMax.z() = pMax.z();
        }else{
            chMin.z() = pMin.z();
            chMax.z() = center.z();
        }

        return BoundingBox3f(chMin, chMax);
}
OctreeNode* buildRecursive(const BoundingBox3f& bbox, 
    const std::vector<uint32_t>& triangles, const Mesh* mesh, uint32_t depth, uint32_t maxDepth){
    
    // 삼각혀잉 없으면 종료
    if(triangles.empty()){
        return nullptr;
    }

    // 삼각형이 적으면 리프노드 생성해서 리턴해주기
    if(triangles.size() < 10){
        OctreeNode* leaf = new OctreeNode();
        leaf->triangles = triangles;
        return leaf;
    }

    if(depth >= maxDepth){
        OctreeNode* leaf = new OctreeNode();
        leaf->triangles = triangles;
        return leaf;
    }
    // 임시로 8개의 자식 노드에 담을 삼각형들
    std::vector<uint32_t> childTriangles[8];

    BoundingBox3f childBoxes[8];
    for (int i = 0; i< 8; ++i){
        childBoxes[i] = getChildBoundingBox(bbox, i);
    }

    for (uint32_t triIdx: triangles){ // 삼각형을 인덱스로 받아와서
        BoundingBox3f triBBox = mesh->getBoundingBox(triIdx);
        // 삼각형의 바운딩 박스를 만듦.
        for (int i = 0; i < 8; ++i){
            //child box 를 만들고
            // BoundingBox3f childBox = getChildBoundingBox(bbox, i);
            
            // 그 차일드 박스랑 삼각형 만든거랑 겹치는지 확인해보고 겹치면 그 자식에게 삼각형 할당해주기.
            if(childBoxes[i].overlaps(triBBox)){
                childTriangles[i].push_back(triIdx);
            }
        }
    }

    bool canSplit = false;
    for (int i = 0; i < 8; ++i) {
        if (!childTriangles[i].empty() && 
            childTriangles[i].size() < triangles.size()) {
            canSplit = true;
            break;
        }
    }
    
    if (!canSplit) {
        // 분할해도 개선 안됨 → 강제 리프
        OctreeNode* leaf = new OctreeNode();
        leaf->triangles = triangles;
        return leaf;
    }

    OctreeNode* node = new OctreeNode();

    // 받을 삼각형이 없는 노드는 nullptr로 해주고
    for(int i = 0; i< 8; i++){
        if (childTriangles[i].empty()){
            node->children[i] = nullptr;
        } else {
            // 받을게 있다면 그 바운딩 박스에
            // BoundingBox3f childBox = getChildBoundingBox(bbox, i);
            // 그 바운딩 박스에서 더 쪼개질 여지가 있는지 확인.
            node->children[i] = buildRecursive(
                childBoxes[i],
                childTriangles[i],
                mesh,
                depth + 1,
                maxDepth);
        }
    }

    return node;
}
void Accel::addMesh(Mesh *mesh) {
    if (m_mesh)
        throw NoriException("Accel: only a single mesh is supported!");
    m_mesh = mesh;
    m_bbox = m_mesh->getBoundingBox();
}

void Accel::build() {
    /* Nothing to do here for now */
    std::vector<uint32_t> allTriangles;
    for(uint32_t i = 0; i< m_mesh->getTriangleCount(); ++i){
        allTriangles.push_back(i);
    }
    int maxDepth = 8;
    m_root = buildRecursive(m_bbox, allTriangles,m_mesh, 0, maxDepth);

    std::cout << "Octree built" << std::endl;
}
bool rayIntersectNode(const OctreeNode* node, const BoundingBox3f& nodeBBox, 
    Ray3f& ray, Intersection& its,bool shadowRay, bool& foundIntersection, 
    uint32_t& f, const Mesh* Mesh){
        // 현재 노드의 바운딩 박스와 Ray intersection 결과가 false 일 때
        if(!nodeBBox.rayIntersect(ray)){
            return false; // false 반환
        }
        // 노드에 속한 삼각형 순회
        if(!node->triangles.empty()){
            for(uint32_t idx:node->triangles){
                float u, v, t;
                if (Mesh->rayIntersect(idx, ray, u, v, t)) {
                    /* An intersection was found! Can terminate
                    immediately if this is a shadow ray query */
                    if (shadowRay)
                        return true;
                    ray.maxt = its.t = t;
                    its.uv = Point2f(u, v);
                    its.mesh = Mesh;
                    f = idx;
                    foundIntersection = true;
                }
            }
        }

        // 자식 노드가 더 있으면 그 공간 추적 (depth + 1)
        for (int i = 0; i<8; i++){
            if(node->children[i]!= nullptr){
                BoundingBox3f childBox  = getChildBoundingBox(nodeBBox,i);
                bool hit = rayIntersectNode(node->children[i],childBox, ray,  its, shadowRay, foundIntersection, f, Mesh);
                if (hit && shadowRay){
                    return true;
                }
            }
        }

        return foundIntersection;
}
bool Accel::rayIntersect(const Ray3f &ray_, Intersection &its, bool shadowRay) const {
    bool foundIntersection = false;  // Was an intersection found so far?
    uint32_t f = (uint32_t) -1;      // Triangle index of the closest intersection

    Ray3f ray(ray_); /// Make a copy of the ray (we will need to update its '.maxt' value)

    if(!m_root){
        // 만약에 Octree가 비어있다면 원래의 순회방식대로 진행. (무차별 대입)
        for (uint32_t idx = 0; idx < m_mesh->getTriangleCount(); ++idx) {
            float u, v, t;
            if (m_mesh->rayIntersect(idx, ray, u, v, t)) {
                /* An intersection was found! Can terminate
                immediately if this is a shadow ray query */
                if (shadowRay)
                    return true;
                ray.maxt = its.t = t;
                its.uv = Point2f(u, v);
                its.mesh = m_mesh;
                f = idx;
                foundIntersection = true;
            }
        }
    }else{
        // 옥트리 빌드 된거면 옥트리 순회
       rayIntersectNode(m_root, m_bbox, ray, its, shadowRay, foundIntersection, f, m_mesh);
    }
        
    if (foundIntersection) {
        /* At this point, we now know that there is an intersection,
           and we know the triangle index of the closest such intersection.

           The following computes a number of additional properties which
           characterize the intersection (normals, texture coordinates, etc..)
        */
        // 찾
        /* Find the barycentric coordinates */
        Vector3f bary;
        bary << 1-its.uv.sum(), its.uv;

        /* References to all relevant mesh buffers */
        const Mesh *mesh   = its.mesh;
        const MatrixXf &V  = mesh->getVertexPositions();
        const MatrixXf &N  = mesh->getVertexNormals();
        const MatrixXf &UV = mesh->getVertexTexCoords();
        const MatrixXu &F  = mesh->getIndices();

        /* Vertex indices of the triangle */
        uint32_t idx0 = F(0, f), idx1 = F(1, f), idx2 = F(2, f);

        Point3f p0 = V.col(idx0), p1 = V.col(idx1), p2 = V.col(idx2);

        /* Compute the intersection positon accurately
           using barycentric coordinates */
        its.p = bary.x() * p0 + bary.y() * p1 + bary.z() * p2;

        /* Compute proper texture coordinates if provided by the mesh */
        if (UV.size() > 0)
            its.uv = bary.x() * UV.col(idx0) +
                bary.y() * UV.col(idx1) +
                bary.z() * UV.col(idx2);

        /* Compute the geometry frame */
        its.geoFrame = Frame((p1-p0).cross(p2-p0).normalized());

        if (N.size() > 0) {
            /* Compute the shading frame. Note that for simplicity,
               the current implementation doesn't attempt to provide
               tangents that are continuous across the surface. That
               means that this code will need to be modified to be able
               use anisotropic BRDFs, which need tangent continuity */

            its.shFrame = Frame(
                (bary.x() * N.col(idx0) +
                 bary.y() * N.col(idx1) +
                 bary.z() * N.col(idx2)).normalized());
        } else {
            its.shFrame = its.geoFrame;
        }
    }

    return foundIntersection;
}

NORI_NAMESPACE_END

