
#pragma once
#include <nori/color.h>
#include <nori/vector.h>
#include <nori/bitmap.h>
NORI_NAMESPACE_BEGIN

class Mipmap{
public:
    Mipmap(const std::string& filename);
    float pdf(float u, float v) const;
    Point2f sample(const Point2f& sample) const;

private:
    //level 별 mipmap
    std::vector<MatrixXf> m_levels;
    // level 0 해상도
    int m_width, m_height;
    // 루미넌스 값
    float m_totalLumi;
    void buildMipmap();
};
NORI_NAMESPACE_END
