#include <nori/mipmap.h>

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
NORI_NAMESPACE_END
