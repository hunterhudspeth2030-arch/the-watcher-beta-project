#include <windows.h>
#include <cmath>
#include <string>
#include <vector>
#include <queue>
#include <ctime>

const int WINDOW_W = 1280, WINDOW_H = 720;
const int MAP_SIZE = 20;
const float FOV = 0.8f, MAX_DEPTH = 20.0f;

std::string gameMap = 
    "####################"
    "#S.................#"
    "#.###.####.###.###.#"
    "#.#......#.#.....#.#"
    "#.#.####.#.#.###.#.#"
    "#.#.#......#...#.#.#"
    "#.#.####.#.###.#.#.#"
    "#..................#"
    "####.#########.#####"
    "#......#...........#"
    "#.####.#.#######.###"
    "#.#....#.#.....#...#"
    "#.#.####.#.###.###.#"
    "#.#......#.#.....#.#"
    "#.########.#####.#.#"
    "#..................#"
    "#.###.####.#######.#"
    "#.#......#.......#.#"
    "#.########.#######.#"
    "####################";

enum AIState { PATROL, CHASE, SEARCH };
AIState watcherState = PATROL;

float pX = 1.5f, pY = 1.5f, pA = 0.0f;
float wX = 18.5f, wY = 18.5f; 
float lastKnownX = 1.5f, lastKnownY = 1.5f;
float searchTimer = 0.0f;
POINT patrolTarget = {1, 1};
int currentState = 0; 

bool IsWall(int x, int y) {
    if (x < 0 || x >= MAP_SIZE || y < 0 || y >= MAP_SIZE) return true;
    return gameMap[y * MAP_SIZE + x] == '#';
}

bool HasLineOfSight() {
    float dx = pX - wX, dy = pY - wY;
    float dist = sqrt(dx*dx + dy*dy);
    if (dist > 12.0f) return false;
    for (float i = 0; i < dist; i += 0.2f) {
        if (IsWall((int)(wX + (dx/dist)*i), (int)(wY + (dy/dist)*i))) return false;
    }
    return true;
}

POINT GetNextTile(int startX, int startY, int targetX, int targetY) {
    std::queue<POINT> q;
    q.push({targetX, targetY});
    std::vector<int> dist(MAP_SIZE * MAP_SIZE, -1);
    dist[targetY * MAP_SIZE + targetX] = 0;
    int dx[] = {0, 0, 1, -1}, dy[] = {1, -1, 0, 0};
    while (!q.empty()) {
        POINT curr = q.front(); q.pop();
        if (curr.x == startX && curr.y == startY) break;
        for (int i = 0; i < 4; i++) {
            int nx = curr.x + dx[i], ny = curr.y + dy[i];
            if (!IsWall(nx, ny) && dist[ny * MAP_SIZE + nx] == -1) {
                dist[ny * MAP_SIZE + nx] = dist[curr.y * MAP_SIZE + curr.x] + 1;
                q.push({nx, ny});
            }
        }
    }
    POINT best = {startX, startY};
    int minDist = 9999;
    for (int i = 0; i < 4; i++) {
        int nx = startX + dx[i], ny = startY + dy[i];
        if (!IsWall(nx, ny) && dist[ny * MAP_SIZE + nx] != -1 && dist[ny * MAP_SIZE + nx] < minDist) {
            minDist = dist[ny * MAP_SIZE + nx];
            best = {nx, ny};
        }
    }
    return best;
}

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd; (void)nShow; 
    srand((unsigned int)time(NULL));

    WNDCLASS wc = {}; 
    wc.lpfnWndProc = WndProc; 
    wc.hInstance = hInst; 
    wc.lpszClassName = "WatcherV4_PatrolFix";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    HWND hwnd = CreateWindow("WatcherV4_PatrolFix", "THE WATCHER - STEALTH AI", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, WINDOW_W, WINDOW_H, 0, 0, hInst, 0);
    HDC hdc = GetDC(hwnd); HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBm = CreateCompatibleBitmap(hdc, WINDOW_W, WINDOW_H); SelectObject(memDC, memBm);
    
    DWORD lastTick = GetTickCount();
    while (true) {
        DWORD now = GetTickCount();
        float elapsed = (now - lastTick) / 1000.0f;
        if (elapsed > 0.1f) elapsed = 0.1f;
        lastTick = now;

        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            TranslateMessage(&msg); DispatchMessage(&msg);
        }

        RECT bg = {0, 0, WINDOW_W, WINDOW_H};
        FillRect(memDC, &bg, (HBRUSH)GetStockObject(BLACK_BRUSH));

        if (currentState == 0) {
            SetTextColor(memDC, RGB(255, 255, 255)); SetBkMode(memDC, TRANSPARENT);
            TextOut(memDC, WINDOW_W/2-100, WINDOW_H/2, "PRESS ENTER TO START", 20);
            if (GetAsyncKeyState(VK_RETURN)) { currentState = 1; ShowCursor(0); }
        } else if (currentState == 1) {
            POINT m; GetCursorPos(&m); ScreenToClient(hwnd, &m);
            pA += (m.x - WINDOW_W/2) * 0.002f;
            POINT center = {WINDOW_W/2, WINDOW_H/2}; ClientToScreen(hwnd, &center); SetCursorPos(center.x, center.y);

            float nx = 0, ny = 0;
            if (GetAsyncKeyState('W')) { nx += sinf(pA) * 3.0f * elapsed; ny += cosf(pA) * 3.0f * elapsed; }
            if (GetAsyncKeyState('S')) { nx -= sinf(pA) * 3.0f * elapsed; ny -= cosf(pA) * 3.0f * elapsed; }
            if (!IsWall((int)(pX + nx * 1.5f), (int)pY)) pX += nx;
            if (!IsWall((int)pX, (int)(pY + ny * 1.5f))) pY += ny;

            bool canSee = HasLineOfSight();
            if (canSee) { watcherState = CHASE; lastKnownX = pX; lastKnownY = pY; }
            else if (watcherState == CHASE) { watcherState = SEARCH; searchTimer = 1.5f; }

            POINT next; float speed = 2.1f;
            if (watcherState == CHASE) { 
                next = GetNextTile((int)wX, (int)wY, (int)pX, (int)pY); speed = 2.8f; 
            }
            else if (watcherState == SEARCH) {
                next = GetNextTile((int)wX, (int)wY, (int)lastKnownX, (int)lastKnownY);
                // Check if reached search location
                if (sqrt(pow(wX-(lastKnownX+0.5f),2)+pow(wY-(lastKnownY+0.5f),2)) < 0.6f) {
                    searchTimer -= elapsed; 
                    if (searchTimer <= 0) {
                        watcherState = PATROL;
                        do { patrolTarget = {rand() % 18 + 1, rand() % 18 + 1}; } while (IsWall(patrolTarget.x, patrolTarget.y));
                    }
                }
            } else { // PATROL
                next = GetNextTile((int)wX, (int)wY, patrolTarget.x, patrolTarget.y);
                if (sqrt(pow(wX-(patrolTarget.x+0.5f),2)+pow(wY-(patrolTarget.y+0.5f),2)) < 0.6f) {
                    do { patrolTarget = {rand() % 18 + 1, rand() % 18 + 1}; } while (IsWall(patrolTarget.x, patrolTarget.y));
                }
            }

            float adx = (next.x + 0.5f) - wX, ady = (next.y + 0.5f) - wY;
            float d = sqrt(adx*adx + ady*ady);
            if (d > 0.01f) { wX += (adx/d) * speed * elapsed; wY += (ady/d) * speed * elapsed; }
            if (sqrt(pow(wX-pX,2)+pow(wY-pY,2)) < 0.7f) currentState = 2;

            float zBuffer[WINDOW_W];
            for (int x=0; x<WINDOW_W; x+=2) {
                float angle = (pA - FOV/2) + ((float)x/WINDOW_W) * FOV;
                float dist = 0;
                while (dist < MAX_DEPTH) { dist += 0.05f; if (IsWall((int)(pX+sinf(angle)*dist), (int)(pY+cosf(angle)*dist))) break; }
                zBuffer[x] = zBuffer[x+1] = dist;
                int h = (int)(WINDOW_H / dist);
                int shade = (int)(180 * (1.0f - dist/MAX_DEPTH));
                HBRUSH br = CreateSolidBrush(RGB(shade, shade, shade));
                RECT col = {x, WINDOW_H/2 - h/2, x+2, WINDOW_H/2 + h/2};
                FillRect(memDC, &col, br); DeleteObject(br);
            }

            float sx = wX - pX, sy = wY - pY, distW = sqrt(sx*sx + sy*sy);
            float spriteA = atan2(sx, sy) - pA;
            while (spriteA < -3.14f) spriteA += 6.28f;
            while (spriteA > 3.14f) spriteA -= 6.28f;

            if (fabs(spriteA) < FOV && zBuffer[WINDOW_W/2] > distW) {
                int screenX = (int)((0.5f * (spriteA / (FOV/2)) + 0.5f) * WINDOW_W);
                int size = (int)(WINDOW_H / distW);
                HBRUSH rb = CreateSolidBrush(watcherState == CHASE ? RGB(255, 0, 0) : RGB(255, 165, 0));
                Ellipse(memDC, screenX - size/4, WINDOW_H/2 - size/4, screenX + size/4, WINDOW_H/2 + size/4);
                DeleteObject(rb);
            }

            int ts = 6;
            for(int y=0; y<MAP_SIZE; y++) for(int x=0; x<MAP_SIZE; x++) {
                HBRUSH b = CreateSolidBrush(IsWall(x,y) ? RGB(60,60,60) : RGB(20,20,20));
                RECT r = {20+x*ts, 20+y*ts, 20+(x+1)*ts, 20+(y+1)*ts};
                FillRect(memDC, &r, b); DeleteObject(b);
            }
            HBRUSH pB = CreateSolidBrush(RGB(0, 255, 0)); 
            RECT pr = {20+(int)(pX*ts)-1, 20+(int)(pY*ts)-1, 20+(int)(pX*ts)+1, 20+(int)(pY*ts)+1};
            FillRect(memDC, &pr, pB); DeleteObject(pB);
            HBRUSH wB = CreateSolidBrush(watcherState == CHASE ? RGB(255,0,0) : RGB(255,255,0)); 
            RECT wr = {20+(int)(wX*ts)-1, 20+(int)(wY*ts)-1, 20+(int)(wX*ts)+1, 20+(int)(wY*ts)+1};
            FillRect(memDC, &wr, wB); DeleteObject(wB);

        } else {
            SetTextColor(memDC, RGB(255, 0, 0)); TextOut(memDC, WINDOW_W/2-40, WINDOW_H/2, "CAUGHT", 6);
        }
        BitBlt(hdc, 0, 0, WINDOW_W, WINDOW_H, memDC, 0, 0, SRCCOPY);
        Sleep(5);
    }
    return 0;
}