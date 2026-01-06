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

#include <nori/warp.h>
#include <nori/vector.h>
#include <nori/frame.h>

NORI_NAMESPACE_BEGIN

Point2f Warp::squareToUniformSquare(const Point2f &sample) {
    return sample;
}

float Warp::squareToUniformSquarePdf(const Point2f &sample) {
    return ((sample.array() >= 0).all() && (sample.array() <= 1).all()) ? 1.0f : 0.0f;
}

Point2f Warp::squareToTent(const Point2f &sample) {

    float t_x;
    if(sample.x()<0.5){
      t_x = std::sqrt(2*sample.x() ) -1;  
    }else{
        t_x = 1 - std::sqrt(2* (1-sample.x()));
    }
    float t_y;
    if(sample.y()<0.5){
      t_y = std::sqrt(2*sample.y() ) -1;  
    }else{
        t_y = 1 - std::sqrt(2* (1-sample.y()));
    }
        
    return Point2f(t_x, t_y);
}

float Warp::squareToTentPdf(const Point2f &p) {
    if(p.x() < -1.0f || p.x() > 1.0f ||
    p.y() < -1.0f || p.y() > 1.0f){
        return 0.0f;
    }
    float x = p.x();
    float y = p.y();

    float pdf_x, pdf_y;
    if(x < 0.0f){
        pdf_x = 1 + x;
    }else{
        pdf_x = 1 - x;
    }

    if(y < 0.0f){
        pdf_y = 1 + y;
    }else{
        pdf_y = 1 - y;
    }

    return pdf_x * pdf_y;
}

Point2f Warp::squareToUniformDisk(const Point2f &sample) {
    // r = R * x_1^{1/2}
    // theta = [0,1]~[0, 2Pi] mapping
    float r = std::sqrt(sample.x());
    float theta = sample.y() * 2 * M_PI;
    
    float x = r * std::cos(theta);
    float y = r * std::sin(theta);

    return Point2f(x, y);
}

float Warp::squareToUniformDiskPdf(const Point2f &p) {
    throw NoriException("Warp::squareToUniformDiskPdf() is not yet implemented!");
}

Vector3f Warp::squareToUniformSphere(const Point2f &sample) {
    throw NoriException("Warp::squareToUniformSphere() is not yet implemented!");
}

float Warp::squareToUniformSpherePdf(const Vector3f &v) {
    throw NoriException("Warp::squareToUniformSpherePdf() is not yet implemented!");
}

Vector3f Warp::squareToUniformHemisphere(const Point2f &sample) {
    throw NoriException("Warp::squareToUniformHemisphere() is not yet implemented!");
}

float Warp::squareToUniformHemispherePdf(const Vector3f &v) {
    throw NoriException("Warp::squareToUniformHemispherePdf() is not yet implemented!");
}

Vector3f Warp::squareToCosineHemisphere(const Point2f &sample) {
    throw NoriException("Warp::squareToCosineHemisphere() is not yet implemented!");
}

float Warp::squareToCosineHemispherePdf(const Vector3f &v) {
    throw NoriException("Warp::squareToCosineHemispherePdf() is not yet implemented!");
}

Vector3f Warp::squareToBeckmann(const Point2f &sample, float alpha) {
    throw NoriException("Warp::squareToBeckmann() is not yet implemented!");
}

float Warp::squareToBeckmannPdf(const Vector3f &m, float alpha) {
    throw NoriException("Warp::squareToBeckmannPdf() is not yet implemented!");
}

NORI_NAMESPACE_END
