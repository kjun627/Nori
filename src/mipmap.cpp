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
        
        // 히위 레벨에서의 루미넌스 값 얻어오기
        float topLeft = nextLevel(y0, x0);
        float topRight = nextLevel(y0, x1);
        float bottomLeft = nextLevel(y1, x0);
        float bottomRight = nextLevel(y1, x1);
        float total = topLeft + topRight + bottomLeft + bottomRight;

        // Division by zero 방지
        if (total < 1e-8f) {
            // 모든 픽셀이 0이면 균등 분포로
            x = (u < 0.5f) ? x0 : x1;
            y = (v < 0.5f) ? y0 : y1;
            u = 2.0f * (u - (u < 0.5f ? 0.0f : 0.5f));
            v = 2.0f * (v - (v < 0.5f ? 0.0f : 0.5f));
            continue;
        }

        // 상하를 기준으로 나눠서 정규화시켜서 확률값으로 접근 (수직 먼저)
        float top = topLeft + topRight;
        float topProb = top/total;

        // 샘플링된 값이 계산된 확률 선택
        if(v < topProb){
            // 위쪽 선택
            y = y0;
            v = (topProb > 1e-8f) ? (v / topProb) : 0.5f;
        }else{
            // 아래쪽 선택
            y = y1;
            float bottomProb = 1.0f - topProb;
            v = (bottomProb > 1e-8f) ? ((v - topProb) / bottomProb) : 0.5f;
        }

        // 좌우 선택도 동일한 로직 (선택된 row 내에서)
        float leftSum = (y == y0) ? topLeft : bottomLeft;
        float rightSum = (y == y0) ? topRight : bottomRight;
        float rowTotal = leftSum + rightSum;

        if (rowTotal < 1e-8f) {
            // Row가 0이면 균등 분포
            x = (u < 0.5f) ? x0 : x1;
            u = 2.0f * (u - (u < 0.5f ? 0.0f : 0.5f));
            continue;
        }

        float leftProb = leftSum / rowTotal;

        if(u < leftProb){
            // 왼쪽 선택
            x = x0;
            u = (leftProb > 1e-8f) ? (u / leftProb) : 0.5f;
        }else{
            // 오른쪽 선택
            x = x1;
            float rightProb = 1.0f - leftProb;
            u = (rightProb > 1e-8f) ? ((u - leftProb) / rightProb) : 0.5f;
        }
    }
    // 그래서 pixel space coordi로 변환
    float pixelU = (x+0.5f)/m_width;
    float pixelV = (y+0.5f)/m_height;
    // 최종 픽셀 위치 리턴
    return Point2f(pixelU, pixelV);
}
float Mipmap::pdf(float u, float v) const{
    // [0,1] -> pixel space 값으로 바꾸기
    int x = std::min(std::max(0, (int)(u*m_width)), m_width-1);
    int y = std::min(std::max(0, (int)(v*m_height)), m_height-1);

    // 픽셀에서의 정규화된 루미넌스값
    float pixelLumi = m_levels[0](y,x);

    // 예외 처리
    if (m_totalLumi <= 0.0f) return 0.0f;
    if (pixelLumi < 0.0f) {
        std::cerr << "WARNING: Negative luminance at (" << x << ", " << y << "): " << pixelLumi << std::endl;
        return 0.0f;
    }

    // 정규화된 PDF 계산
    // level0은 이미 정규화되어 sum=1
    // Continuous domain [0,1]x[0,1]에서의 PDF density
    // PDF = (normalized luminance) * (total number of pixels)
    float pdf = pixelLumi * (m_width * m_height);

    if (pdf < 0.0f) {
        std::cerr << "ERROR: Negative PDF! u=" << u << ", v=" << v
                  << ", x=" << x << ", y=" << y
                  << ", pixelLumi=" << pixelLumi
                  << ", width=" << m_width << ", height=" << m_height
                  << ", pdf=" << pdf << std::endl;
        return 0.0f;
    }

    return pdf;
}
NORI_NAMESPACE_END
