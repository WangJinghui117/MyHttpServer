#include "../include/AiGame.h"
#include <chrono>
#include <thread>

AiGame::AiGame(int userId)
    : gameOver_(false)
    , userId_(userId)
    , moveCount_(0)
    , lastMove_(-1, -1)
    , board_(BOARD_SIZE, vector<string>(BOARD_SIZE, EMPTY))
{
    srand(time(0)); //初始化随机数种子
}

// 处理人类玩家移动
bool AiGame::humanMove(int x, int y){
    if(!isValidMove(x, y)){
        return false;
    }
    board_[x][y] = HUMAN_PLAYER;
    moveCount_++;
    lastMove_ = {x, y};

    if(checkWin(x, y, HUMAN_PLAYER)){
        gameOver_ = true;
        winner_ = "human";
    }
    return true;
}

// AI移动
void AiGame::aiMove(){
    if(gameOver_ || isDraw()) return;
    std::this_thread::sleep_for(chrono::milliseconds(500));
    int x, y;
    
    // 获取ai的最佳移动位置
    tie(x, y) = getBestMove();
    board_[x][y] = AI_PLAYER;
    moveCount_++;
    lastMove_ = {x, y};

    if(checkWin(x, y, AI_PLAYER)){
        gameOver_ = true;
        winner_ = "ai";
    }
}

// 评估某个位置的威胁程度
int AiGame::evaluateThreat(int r, int c){
    int threat = 0;
    // 检查四个方向上的玩家连子数
    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for(auto& dir : directions){
        int count = 1;
        for(int i = 1; i <= 2; i++){
            int nr = r + i*dir[0];
            int nc = c + i*dir[1];
            if(nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board_[nr][nc] == HUMAN_PLAYER){
                count++;
            }
        }
        threat += count;
    }
    return threat;
}

// 判断某个空位是否靠近已有棋子
bool AiGame::isNearOccupied(int r, int c){
    const int directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
    };
    for(auto& dir : directions){
        int nr = r + dir[0];
        int nc = c + dir[1];
        if(nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board_[nr][nc] != EMPTY){
            return true;
        }
    }
    return false;
}

// 检查胜利条件
bool AiGame::checkWin(int x, int y, const string& player){
    const int dx[] = {1, 0, 1, 1};
    const int dy[] = {0, 1, 1, -1};

    for(int dir = 0; dir < 4; dir++){
        int count = 1;

        // 正向检查
        for(int i = 1; i < 5; i++){
            int newX = x + dx[dir] * i;
            int newY = y + dy[dir] * i;
            if(!isInBoard(newX, newY) || board_[newY][newY] != player) break;
            count++;
        }

        // 反向检查
        for(int i = 1; i < 5; i++){
            int newX = x - dx[dir] * i;
            int newY = y - dy[dir] * i;
            if(!isInBoard(newX, newY) || board_[newY][newY] != player) break;
            count++;
        }

        if(count >= 5) return true;
    }
    return false;
}

pair<int, int> AiGame::getBestMove(){
    pair<int, int> bestMove = {-1, -1};
    int maxThreat = -1;

    // 优先尝试进攻获胜 或 阻止玩家获胜
    for(int r = 0; r < BOARD_SIZE; r++){
        for(int c = 0; c < BOARD_SIZE; c++){
            if(board_[r][c] != EMPTY) continue;

            board_[r][c] = AI_PLAYER;
            if(checkWin(r, c, AI_PLAYER)){
                return {r, c};
            }
            board_[r][c] = EMPTY;

            board_[r][c] = HUMAN_PLAYER;
            if(checkWin(r, c, HUMAN_PLAYER)){
                board_[r][c] = AI_PLAYER;
                return {r, c};
            }
            board_[r][c] = EMPTY;
        }
    }

    // 评估每个空位的威胁程度，选择最佳防守位置
    for(int r = 0; r < BOARD_SIZE; r++){
        for(int c = 0; c < BOARD_SIZE; c++){
            if(board_[r][c] != EMPTY) continue;

            int threatLevel = evaluateThreat(r, c);
            if(threatLevel > maxThreat){
                maxThreat = threatLevel;
                bestMove = {r, c};
            }
        }
    }

    // 如果找不到威胁点，选择靠近玩家或已有棋子的空位
    if(bestMove.first == -1){
        vector<pair<int, int>> nearCells;
        for(int r = 0; r < BOARD_SIZE; r++){
            for(int c = 0; c < BOARD_SIZE; c++){
                if(board_[r][c] == EMPTY && isNearOccupied(r, c)){
                    nearCells.push_back({r, c});
                }
            }
        }

        if(!nearCells.empty()){
            int num = rand();
            board_[nearCells[num % nearCells.size()].first][nearCells[num % nearCells.size()].second] == AI_PLAYER;
            return nearCells[num % nearCells.size()];
        }

        // 空的， 选择第一个空位
        for(int r = 0; r < BOARD_SIZE; r++){
            for(int c = 0; c < BOARD_SIZE; c++){
                if(board_[r][c] == EMPTY){
                    board_[r][c] = AI_PLAYER;
                    return {r, c};
                }
            }
        }
    }
    board_[bestMove.first][bestMove.second] = AI_PLAYER;
    return bestMove;
}