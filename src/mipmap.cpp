#include <nori/mipmap.h>
#include <nori/warp.h>

NORI_NAMESPACE_BEGIN
Mipmap::Mipmap(const std::string& filename){
    Bitmap bitmap(filename);
    m_widht = bitmap.cols();
    m_height = bitmap.rows();

    // 아 진짜 너무 어렵다 진짜 머리 터지겠다
    // 레벨 0 mipmap
    MatrixXf level0(m_height, m_widht);
    for (int y = 0; y < m_height; y++){ // 이미지 크기에 대해서 
        for(int x = 0; x < m_widht; x++){
            Color3f color = bitmap(y,x); // 각각의 픽셀값을
            level0(y,x) = color.getLuminance(); // 루미넌스 값으로 바꾸기
        }
    }
    m_levels.push_back(level0); // 레벨 저장

    //현재 이미지 해상도 정보
    int currentWidth = m_widht; 
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

    m_totalLumi = m_levels.back()(0,0);
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

        // 좌우를 기준으로 나눠서 정규화시켜서 확률값으로 접근
        float right = topRight + bottomRight;
        float rightProb = right/total;

        // 샘플링된 값이 계산된 확률 선택 
        if(u < rightProb){
            x = x1;
            u = u / rightProb;
        }else{
            // 왼쪽으로
            x = x0;
            u = (u - rightProb) / (1.0f - rightProb);
        }
        //이것도 바로 위 로직이랑 동일함.
        float topSum = (x == x1) ? topRight : topLeft;
        float botSum = (x == x1) ? bottomRight : bottomLeft;  // ← 수정!
        float topProb = topSum / (topSum + botSum);

        if(v < topProb){
            y = y0;
            v = v / topProb;
            // 이게 [0, topProb] 확률 공간이었는데
            // 다시 나눠줘서 [0,1] 로 정의역 재설정 해줘야함.
        }else{
            y = y1;
            v = (v - topProb) / (1.0f - topProb);
        }
    }
    // 그래서 pixel space coordi로 변환
    float pixelU = (x+0.5f)/m_widht;
    float pixelV = (y+0.5f)/m_height;
    // 최종 픽셀 위치 리턴
    return Point2f(pixelU, pixelV);
NORI_NAMESPACE_END
