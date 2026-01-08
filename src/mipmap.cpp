#include <nori/mipmap.h>
#include <nori/warp.h>

NORI_NAMESPACE_BEGIN
Mipmap::Mipmap(const std::string& filename){
    Bitmap bitmap(filename);
    m_width = bitmap.cols();
    m_height = bitmap.rows();

    // 레벨 0 mipmap - 원본 luminance 값 저장
    MatrixXf level0(m_height, m_width);
    m_totalLumi = 0.0f;
    for (int y = 0; y < m_height; y++){
        for(int x = 0; x < m_width; x++){
            Color3f color = bitmap(y,x);
            float lumi = color.getLuminance();
            // 부동소수점 오차로 인한 음수 값을 0으로 클램핑
            lumi = std::max(0.0f, lumi);
            level0(y,x) = lumi;
            m_totalLumi += lumi;  // 전체 luminance 합 계산
        }
    }

    // 정규화: 각 픽셀을 total luminance로 나눔
    if (m_totalLumi > 0.0f) {
        level0 /= m_totalLumi;
    }
    m_levels.push_back(level0);

    //현재 이미지 해상도 정보
    int currentWidth = m_width;
    int currentHeight = m_height;

    // 크기가 픽셀보다 클 경우
    while(currentHeight > 1 || currentWidth > 1){
         // 해상도를 가로세로 2분의 1씩 줄임
        int nextWidth = std::max(1,currentWidth/2);
        int nextHeight = std::max(1, currentHeight/2);

        // 그리고 그 영역만큼의 행렬을 만들고
        MatrixXf nextLevel(nextHeight, nextWidth);
        // 이전 레벨이 몇이었는지를 받아냄
        const MatrixXf& prevLevel = m_levels.back();

        // 새로운 크기의 해상도에 대해서 
        for(int y = 0; y < nextHeight; y++){
            for (int x = 0; x < nextWidth; x++){
                float sum = 0.0f; // 2×2 그리드를 기준으로 계산할거임
                for(int dy = 0; dy < 2; dy++){        
                    for(int dx = 0; dx < 2; dx++){ // 하나의 레벨이 이전 레벨에서의 4개 픽셀 값 먹음
                        int py = std::min(2*y+dy, currentHeight-1);  // 겹치는 부분이 없기 때문에 
                        int px = std::min(2*x+dx, currentWidth-1);   // 2칸씩 점프 해야됨.
                        sum += prevLevel(py,px); // 그 값을 전부 다 보으기
                    }
                }
                nextLevel(y,x) = sum; // 그리고 이번 mipmap의 픽셀에다가 할당
            }
        }
        m_levels.push_back(nextLevel); // mipmap 넣기
        currentWidth = nextWidth; // 레벨 해상도 다음 루프 준비
        currentHeight = nextHeight;
    }
}

Point2f Mipmap::sample(const Point2f& sample)const{
    // uniform distribution 에서 샘플링된 두 좌표 받아옴
    float u = sample.x();
    float v = sample.y();
    int x = 0, y = 0;

    // 모든 밉맵 순회할거임 맨 마지막 레벨에서 초기 이미지까지 순회 
    for (int level = m_levels.size()-1; level > 0; level --){
        // 다음 레벨의 밉맵 받아오고
        const MatrixXf& nextLevel = m_levels[level-1];
        
        // 좌표 계산하는거임 현재의 픽셀이 상위 레벨에서 어떤 픽셀인지
        int x0 = 2*x, x1 = std::min<int>(2*x+1, nextLevel.cols() - 1);
        int y0 = 2*y, y1 = std::min<int>(2*y+1, nextLevel.rows() - 1);
        
        // 하위 레벨에서의 루미넌스 값 얻어오기
        float topLeft = nextLevel(y0, x0);
        float topRight = nextLevel(y0, x1);
        float bottomLeft = nextLevel(y1, x0);
        float bottomRight = nextLevel(y1, x1);
        float total = topLeft + topRight + bottomLeft + bottomRight;

        if (total < 1e-8f) {
            // 모든 픽셀이 0이면 균등 분포로
            x = (u < 0.5f) ? x0 : x1;
            y = (v < 0.5f) ? y0 : y1;
            u = 2.0f * (u - (u < 0.5f ? 0.0f : 0.5f));
            v = 2.0f * (v - (v < 0.5f ? 0.0f : 0.5f));
            continue;
        }

        // 좌우를 기준으로 나눠서 정규화
        float left = topLeft + bottomLeft;
        float leftProb = left / total;

        // 샘플링된 값으로 좌우 선택
        if(u < leftProb){
            x = x0;
            u = (leftProb > 1e-8f) ? (u / leftProb) : 0.5f;
        }else{
            x = x1;
            float rightProb = 1.0f - leftProb;
            u = (rightProb > 1e-8f) ? ((u - leftProb) / rightProb) : 0.5f;
        }
        
        // 상하 선택 (선택된 열 내에서)
        float topSum = (x == x0) ? topLeft : topRight;
        float botSum = (x == x0) ? bottomLeft : bottomRight;
        float colTotal = topSum + botSum;
        
        if (colTotal < 1e-8f) {
            // 선택된 열이 0이면 균등 분포
            y = (v < 0.5f) ? y0 : y1;
            v = 2.0f * (v - (v < 0.5f ? 0.0f : 0.5f));
        } else {
            float topProb = topSum / colTotal;
            
            if(v < topProb){
                y = y0;
                v = (topProb > 1e-8f) ? (v / topProb) : 0.5f;
            }else{
                y = y1;
                float botProb = 1.0f - topProb;
                v = (botProb > 1e-8f) ? ((v - topProb) / botProb) : 0.5f;
            }
        }
    }
    
    // 최종 픽셀 위치 + 픽셀 내 offset (u,v는 [0,1] 범위)
    float pixelU = (x + u) / m_width;
    float pixelV = (y + v) / m_height;
    
    // OpenGL 텍스처는 bottom-up이므로 Y를 반전
    pixelV = 1.0f - pixelV;
    
    return Point2f(pixelU, pixelV);
}
float Mipmap::pdf(float u, float v) const{
    // OpenGL 텍스처 좌표는 bottom-up이므로 Y를 반전
    v = 1.0f - v;
    
    // [0,1] -> pixel space 값으로 바꾸기
    int x = std::min(std::max(0, (int)(u * m_width)), m_width - 1);
    int y = std::min(std::max(0, (int)(v * m_height)), m_height - 1);

    // Level 0은 이미 전체 합으로 정규화되어 있음
    // 따라서 픽셀값은 해당 픽셀의 확률을 나타냄
    float normalizedLumi = m_levels[0](y, x);

    if (m_totalLumi <= 0.0f) return 0.0f;

    // PDF = (정규화된 픽셀 밝기) * (픽셀 개수)
    // 이유: [0,1]^2 연속 공간의 적분이 1이 되려면 density를 픽셀 개수로 스케일해야 함
    float pdf = normalizedLumi * (m_width * m_height);
    return pdf;
}
NORI_NAMESPACE_END
