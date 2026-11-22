// Oddlands (오드랜즈) - 2026-2 컴퓨터 그래픽스 최종 프로젝트
// 뼈대(ShaderUtil, Mesh3D, DrawBox, Window)는 실습 코드와 같은 구조다.
// 1단계 김민우: 캐릭터 이동·점프(Player), 3인칭 카메라(Camera).
// 2단계 김민우: 적 스폰·추적(Enemy), 투사체 공격(Attack), 충돌(Collision), 경험치·레벨업(Growth, Gem).
//              충돌 효과(Effect: 파편·흔들림·번쩍임), 넓은 임시 맵.

#define NOMINMAX
#include <Windows.h>

#include <gl/glew.h>   // glew.h는 반드시 glfw3.h보다 먼저 include
#include <gl/glfw3.h>
#include <gl/glm/glm.hpp>
#include <gl/glm/ext.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iterator>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <filesystem>
using namespace std;

// ================================================== 셰이더 (실습과 같음) ==================================================
namespace ShaderUtil
{
    string ReadTextFile(const char* fileName)
    {
        ifstream file(fileName);
        if (!file.is_open())
        {
            cerr << "Failed to open shader file: " << fileName << endl;
            return "";
        }

        return string(istreambuf_iterator<char>(file), istreambuf_iterator<char>());
    }

    GLuint CompileShader(GLenum type, const char* fileName)
    {
        string sourceText = ReadTextFile(fileName);
        if (sourceText.empty())
            return 0;

        const char* source = sourceText.c_str();
        GLuint shader = glCreateShader(type);

        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            GLchar log[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, log);
            cerr << "Shader compilation failed: " << fileName << endl;
            cerr << log << endl;
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    GLuint CreateProgram(const char* vertexFile, const char* fragmentFile)
    {
        GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexFile);
        GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentFile);

        if (vertexShader == 0 || fragmentShader == 0)
        {
            if (vertexShader != 0) glDeleteShader(vertexShader);
            if (fragmentShader != 0) glDeleteShader(fragmentShader);
            return 0;
        }

        GLuint program = glCreateProgram();

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        if (!success)
        {
            GLchar log[1024];
            glGetProgramInfoLog(program, 1024, nullptr, log);
            cerr << "Shader program link failed" << endl;
            cerr << log << endl;
            glDeleteProgram(program);
            return 0;
        }

        return program;
    }
}

// ================================================== 도형 (실습과 같음, 지금 쓰는 것만) ==================================================
namespace Mesh3D
{
    struct Vertex
    {
        float x, y, z;
        float r, g, b;
    };

    void Push(vector<Vertex>& v, const glm::vec3& p, const glm::vec3& c)
    {
        v.push_back({ p.x, p.y, p.z, c.r, c.g, c.b });
    }

    void AddTriangle(vector<Vertex>& v, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& color)
    {
        Push(v, a, color);
        Push(v, b, color);
        Push(v, c, color);
    }

    void AddCube(vector<Vertex>& v, float h, const glm::vec3 faceColors[6])
    {
        glm::vec3 p[8] =
        {
            {-h,-h,-h},{ h,-h,-h},{ h, h,-h},{-h, h,-h},
            {-h,-h, h},{ h,-h, h},{ h, h, h},{-h, h, h}
        };

        int t[12][3] =
        {
            {4,5,6},{4,6,7},{1,0,3},{1,3,2},
            {0,4,7},{0,7,3},{5,1,2},{5,2,6},
            {3,7,6},{3,6,2},{0,1,5},{0,5,4}
        };

        for (int i = 0; i < 12; i++)
            AddTriangle(v, p[t[i][0]], p[t[i][1]], p[t[i][2]], faceColors[i / 2]);
    }

    void InitBuffer(GLuint& vao, GLuint& vbo)
    {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
        glEnableVertexAttribArray(1);
    }

    void Draw(GLuint vao, GLuint vbo, const vector<Vertex>& vertices, GLenum mode)
    {
        if (vertices.empty())
            return;

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(mode, 0, static_cast<GLsizei>(vertices.size()));
    }

    void SendMatrix(GLint location, const glm::mat4& m)
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
    }
}

GLuint shaderProgram = 0, VAO = 0, VBO = 0;
GLint modelLocation = -1, viewLocation = -1, projectionLocation = -1;

void DrawMesh(const vector<Mesh3D::Vertex>& v, const glm::mat4& model, GLenum mode = GL_TRIANGLES)
{
    Mesh3D::SendMatrix(modelLocation, model);
    Mesh3D::Draw(VAO, VBO, v, mode);
}

void DrawBox(const glm::vec3& size, const glm::vec3& position, const glm::vec3& color, const glm::mat4& parent = glm::mat4(1.0f))
{
    glm::vec3 faces[6] = { color, color, color, color, color, color };
    vector<Mesh3D::Vertex> v;
    Mesh3D::AddCube(v, 0.5f, faces);
    glm::mat4 m = glm::translate(parent, position);
    m = glm::scale(m, size);
    DrawMesh(v, m);
}

void BeginFrame(const glm::vec3& clearColor, const glm::mat4& view, const glm::mat4& projection)
{
    glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Mesh3D::SendMatrix(viewLocation, view);
    Mesh3D::SendMatrix(projectionLocation, projection);
}

// ================================================== 창 (실습과 같음, [추가] 표시만 다름) ==================================================
namespace Window
{
    void Close(GLFWwindow* window)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLFWwindow* Open(int width, int height, const char* title)
    {
        if (!glfwInit()) return nullptr;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // [추가] 창 크기 고정 → 화면 비율을 상수로 써도 된다

        GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window) { glfwTerminate(); return nullptr; }

        glfwMakeContextCurrent(window);
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) { Close(window); return nullptr; }

        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);

        // [추가] 셰이더를 shaders 폴더에 둔다 (빌드할 때 실행 폴더로 같이 복사됨)
        shaderProgram = ShaderUtil::CreateProgram("shaders/vertex.glsl", "shaders/fragment.glsl");
        if (!shaderProgram) { Close(window); return nullptr; }
        glUseProgram(shaderProgram);

        modelLocation = glGetUniformLocation(shaderProgram, "modelTransform");
        viewLocation = glGetUniformLocation(shaderProgram, "viewTransform");
        projectionLocation = glGetUniformLocation(shaderProgram, "projectionTransform");

        Mesh3D::InitBuffer(VAO, VBO);
        return window;
    }

    void Run(GLFWwindow* window, void (*update)(float), void (*draw)())
    {
        double lastTime = glfwGetTime();

        while (!glfwWindowShouldClose(window))
        {
            double now = glfwGetTime();
            float dt = static_cast<float>(now - lastTime);
            lastTime = now;
            // [추가] 창을 끌면 그동안 루프가 멈춰 dt가 커지고 캐릭터가 순간이동한다. 최대 0.05초로 자른다.
            if (dt > 0.05f) dt = 0.05f;

            if (update) update(dt);
            draw();

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
        glDeleteProgram(shaderProgram);
        Close(window);
    }
}

// ================================================== 게임 ==================================================
const int windowWidth = 1280, windowHeight = 720;
const float mapHalf = 60.0f;   // 바닥 절반 크기(120 × 120). 바닥 그리기와 이동 제한이 같이 쓴다.

float playTime;    // 이번 판에서 흐른 시간(초). 레벨업 선택 중에는 멈춘다
int killCount;     // 처치한 적 수

// a ~ b 사이의 무작위 실수
float RandomRange(float a, float b)
{
    return a + (b - a) * (rand() / (float)RAND_MAX);
}

// ---------- 맵 장애물 (임시) ----------
// 맵·장애물 배치는 홍범기 담당. 지금은 넓은 바닥에 장애물을 무작위로 뿌린 임시 맵이다.
// 진짜 맵이 들어오면 Reset()에서 obstacles 목록만 채우면 충돌은 그대로 동작한다.
namespace Map
{
    enum Kind { Crate, Rock, Pillar, Tree };

    struct Box
    {
        glm::vec3 center;   // 상자 가운데 (충돌 범위)
        glm::vec3 size;     // 전체 크기 (가로, 높이, 세로)
        glm::vec3 color;
        Kind kind;          // Tree면 충돌 상자(줄기) 위에 잎을 더 그린다
    };

    vector<Box> obstacles;
    vector<Mesh3D::Vertex> floorVertices;   // 체크무늬 바닥. 한 번만 만들어 두고 매 프레임 그린다

    const float tileSize = 4.0f;
    const float wallHeight = 2.0f;

    // [추가] 맵 전용 난수. 시드가 고정이라 실행할 때마다 같은 맵이 나온다 (rand()의 스폰·업그레이드와 따로 논다)
    unsigned int mapSeed;
    float MapRandom(float a, float b)
    {
        mapSeed = mapSeed * 1103515245u + 12345u;
        return a + (b - a) * ((mapSeed >> 8) & 0xFFFF) / 65535.0f;
    }

    void BuildFloor()
    {
        floorVertices.clear();
        int n = (int)(mapHalf * 2.0f / tileSize);
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                float x0 = -mapHalf + i * tileSize, x1 = x0 + tileSize;
                float z0 = -mapHalf + j * tileSize, z1 = z0 + tileSize;
                glm::vec3 c = ((i + j) % 2 == 0) ? glm::vec3(0.35f, 0.6f, 0.3f) : glm::vec3(0.31f, 0.55f, 0.27f);
                // 윗면만, 위에서 봤을 때 반시계 순서 (AddCube 윗면과 같은 순서)
                Mesh3D::AddTriangle(floorVertices, { x0, 0, z0 }, { x0, 0, z1 }, { x1, 0, z1 }, c);
                Mesh3D::AddTriangle(floorVertices, { x0, 0, z0 }, { x1, 0, z1 }, { x1, 0, z0 }, c);
            }
        }
    }

    void Reset()
    {
        if (floorVertices.empty())
            BuildFloor();

        obstacles.clear();
        mapSeed = 20261121u;

        // 점프 최고 높이는 약 2.5. 상자·바위는 넘을 수 있고, 기둥·나무는 돌아가야 한다.
        const int count = 140;
        for (int tries = 0; tries < 2000 && (int)obstacles.size() < count; tries++)
        {
            glm::vec3 p(MapRandom(-mapHalf + 3.0f, mapHalf - 3.0f), 0.0f, MapRandom(-mapHalf + 3.0f, mapHalf - 3.0f));

            // 시작 지점(가운데)은 비워 둔다
            if (glm::length(p) < 8.0f)
                continue;

            // 다른 장애물과 너무 붙지 않게 (적이 사이에 끼지 않도록)
            bool tooClose = false;
            for (const auto& o : obstacles)
            {
                if (glm::length(o.center - glm::vec3(p.x, o.center.y, p.z)) < 4.5f)
                {
                    tooClose = true;
                    break;
                }
            }
            if (tooClose)
                continue;

            float r = MapRandom(0.0f, 1.0f);
            Box b;
            if (r < 0.35f)
            {
                float s = MapRandom(0.9f, 1.5f);
                b = { {}, { s, s * 0.8f, s }, { 0.6f, 0.42f, 0.25f }, Crate };
            }
            else if (r < 0.6f)
            {
                b = { {}, { MapRandom(2.0f, 3.5f), MapRandom(0.5f, 1.0f), MapRandom(2.0f, 3.5f) }, { 0.52f, 0.52f, 0.5f }, Rock };
            }
            else if (r < 0.8f)
            {
                float w = MapRandom(1.4f, 2.6f);
                float g = MapRandom(0.45f, 0.6f);
                b = { {}, { w, MapRandom(3.5f, 6.0f), w }, { g, g, g + 0.08f }, Pillar };
            }
            else
            {
                b = { {}, { 0.5f, 3.0f, 0.5f }, { 0.45f, 0.3f, 0.18f }, Tree };
            }
            b.center = glm::vec3(p.x, b.size.y * 0.5f, p.z);
            obstacles.push_back(b);
        }
    }

    void Draw()
    {
        DrawMesh(floorVertices, glm::mat4(1.0f));

        // 맵 끝 벽 (보이기만 하고, 막는 건 각 Update의 clamp가 한다)
        glm::vec3 wallColor(0.4f, 0.36f, 0.32f);
        float len = mapHalf * 2.0f + 1.0f;
        DrawBox({ len, wallHeight, 0.5f }, { 0.0f, wallHeight * 0.5f, -mapHalf - 0.25f }, wallColor);
        DrawBox({ len, wallHeight, 0.5f }, { 0.0f, wallHeight * 0.5f,  mapHalf + 0.25f }, wallColor);
        DrawBox({ 0.5f, wallHeight, len }, { -mapHalf - 0.25f, wallHeight * 0.5f, 0.0f }, wallColor);
        DrawBox({ 0.5f, wallHeight, len }, {  mapHalf + 0.25f, wallHeight * 0.5f, 0.0f }, wallColor);

        for (const auto& box : obstacles)
        {
            DrawBox(box.size, box.center, box.color);

            if (box.kind == Tree)   // 잎: 충돌 없음, 줄기 위에 큰 상자 두 개
            {
                float top = box.size.y;
                DrawBox({ 2.4f, 1.4f, 2.4f }, { box.center.x, top + 0.4f, box.center.z }, { 0.2f, 0.5f, 0.22f });
                DrawBox({ 1.5f, 1.0f, 1.5f }, { box.center.x, top + 1.5f, box.center.z }, { 0.24f, 0.58f, 0.26f });
            }
            else if (box.kind == Crate)   // 나무 상자 띠
            {
                DrawBox({ box.size.x + 0.02f, box.size.y * 0.15f, box.size.z + 0.02f }, box.center, { 0.42f, 0.28f, 0.15f });
            }
        }
    }
}

// ---------- 충돌 계산 도구 (김민우) ----------
// 모든 물체를 위에서 내려다본 원(xz 평면)으로 보고, 높이(y)는 따로 비교한다.
// 적·투사체·젬은 "중심 사이 거리", 장애물은 "상자 형태의 경계 영역"으로 판정 (제안서 계획대로).
namespace Collision
{
    // 두 원이 겹치는지. 거리 대신 거리의 제곱끼리 비교하면 sqrt가 필요 없다.
    bool CircleOverlap(const glm::vec3& a, float ra, const glm::vec3& b, float rb)
    {
        float dx = a.x - b.x;
        float dz = a.z - b.z;
        float r = ra + rb;
        return dx * dx + dz * dz < r * r;
    }

    // 원(pos, radius)이 상자 안으로 들어왔으면 밖으로 밀어낸다.
    // 발이 상자 윗면보다 높으면(점프로 넘는 중) 막지 않는다.
    void PushOutOfBox(glm::vec3& pos, float radius, const Map::Box& box)
    {
        glm::vec3 half = box.size * 0.5f;
        if (pos.y >= box.center.y + half.y)
            return;

        // 1) 상자 위에서 원 중심과 가장 가까운 점
        float nearX = glm::clamp(pos.x, box.center.x - half.x, box.center.x + half.x);
        float nearZ = glm::clamp(pos.z, box.center.z - half.z, box.center.z + half.z);
        float dx = pos.x - nearX;
        float dz = pos.z - nearZ;
        float dist = sqrt(dx * dx + dz * dz);

        if (dist >= radius)
            return;   // 안 닿음

        if (dist > 0.0001f)
        {
            // 2) 가장 가까운 점 → 원 중심 방향으로, 파고든 만큼 밀어낸다
            float push = radius - dist;
            pos.x += dx / dist * push;
            pos.z += dz / dist * push;
        }
        else
        {
            // 3) 중심이 상자 안까지 들어옴 → 가장 가까운 면 바깥으로 꺼낸다
            float left = pos.x - (box.center.x - half.x);
            float right = (box.center.x + half.x) - pos.x;
            float back = pos.z - (box.center.z - half.z);
            float front = (box.center.z + half.z) - pos.z;
            float smallest = min(min(left, right), min(back, front));

            if (smallest == left) pos.x = box.center.x - half.x - radius;
            else if (smallest == right) pos.x = box.center.x + half.x + radius;
            else if (smallest == back) pos.z = box.center.z - half.z - radius;
            else pos.z = box.center.z + half.z + radius;
        }
    }
}

// ---------- 충돌 효과: 파편 파티클·화면 흔들림 (김민우) ----------
// 무엇이 닿았을 때 "닿았다"는 느낌을 주는 연출만 모았다. 판정은 Collision이 하고 여기서는 보여 주기만 한다.
// 파편도 실습 21의 공처럼 struct + vector. 튀어나가 중력으로 떨어지고, 바닥에서 튀고, 작아지며 사라진다.
namespace Effect
{
    struct Particle
    {
        glm::vec3 pos, vel;
        glm::vec3 color;
        float size;        // 처음 크기. 남은 수명에 비례해 작아진다
        float life, maxLife;
    };

    vector<Particle> particles;
    float shake;           // 화면 흔들림이 남은 시간. Camera가 읽는다

    const float gravity = 18.0f;
    const int maxParticles = 600;
    const float shakeTime = 0.25f;

    void Reset()
    {
        particles.clear();
        shake = 0.0f;
    }

    // pos에서 파편 count개를 사방(위쪽 위주)으로 튀긴다
    void Burst(const glm::vec3& pos, const glm::vec3& color, int count, float speed, float size)
    {
        for (int i = 0; i < count && (int)particles.size() < maxParticles; i++)
        {
            float angle = RandomRange(0.0f, glm::two_pi<float>());
            float s = speed * RandomRange(0.5f, 1.0f);
            glm::vec3 vel(cos(angle) * s, RandomRange(0.4f, 1.2f) * speed, sin(angle) * s);

            // 색을 조금씩 다르게 해서 덩어리가 아니라 파편처럼 보이게
            glm::vec3 c = glm::clamp(color * RandomRange(0.8f, 1.2f), glm::vec3(0.0f), glm::vec3(1.0f));
            float life = RandomRange(0.35f, 0.6f);
            particles.push_back({ pos, vel, c, size * RandomRange(0.7f, 1.2f), life, life });
        }
    }

    void Shake(float amount)
    {
        shake = max(shake, amount);
    }

    void Update(float dt)
    {
        if (shake > 0.0f)
            shake = max(0.0f, shake - dt);

        for (auto& p : particles)
        {
            p.vel.y -= gravity * dt;
            p.pos += p.vel * dt;
            if (p.pos.y < 0.0f)   // 바닥에 닿으면 약하게 튀고 미끄러지다 멈춘다
            {
                p.pos.y = 0.0f;
                p.vel.y *= -0.35f;
                p.vel.x *= 0.6f;
                p.vel.z *= 0.6f;
            }
            p.life -= dt;
        }

        for (int i = (int)particles.size() - 1; i >= 0; i--)
        {
            if (particles[i].life <= 0.0f)
                particles.erase(particles.begin() + i);
        }
    }

    void Draw()
    {
        for (const auto& p : particles)
        {
            float k = p.life / p.maxLife;   // 1 → 0
            float age = p.maxLife - p.life;
            glm::mat4 m = glm::translate(glm::mat4(1.0f), p.pos);
            m = glm::rotate(m, age * 12.0f, glm::vec3(1.0f, 1.0f, 0.0f));   // 굴러가며 날아간다
            DrawBox(glm::vec3(p.size * k), glm::vec3(0.0f), p.color, m);
        }
    }
}

// ---------- 플레이어 (김민우) ----------
namespace Player
{
    glm::vec3 pos;         // 발바닥 위치
    float yaw;             // 바라보는 방향(도). 0이면 +z
    float velocityY;       // 위쪽 속도
    float walkTime;        // 걷는 동안만 늘어나는 시간. 팔다리 흔들기에 쓴다
    bool onGround;

    float moveSpeed;       // 초당 이동 거리. 레벨업으로 빨라지므로 const가 아니다
    float hp, maxHp;       // 체력. 0이 되면 게임오버
    float invincible;      // 맞은 뒤 무적으로 남은 시간. 적에 붙어 있어도 매 프레임 깎이지 않게
    glm::vec3 knockback;   // 맞았을 때 밀려나는 속도(xz). 매 프레임 줄어든다

    // 최고 높이 = jumpSpeed² / (2 × gravity) = 121 / 48 ≈ 2.5 (예전 7, 20 → 약 1.2)
    // 중력도 조금 올려서 높이 뛰어도 둥둥 뜨는 느낌이 덜하게 했다.
    const float jumpSpeed = 11.0f;   // 점프 순간의 위쪽 속도
    const float gravity = 24.0f;     // 초당 줄어드는 위쪽 속도
    const float bodyHalf = 0.35f;    // 몸 절반 너비. 맵 끝·장애물·적 충돌에 쓴다
    const float invincibleTime = 0.7f;
    const float hurtFlashTime = 0.15f;   // 맞은 직후 빨갛게 번쩍이는 시간

    void Reset()
    {
        pos = glm::vec3(0.0f);
        yaw = velocityY = walkTime = 0.0f;
        onGround = true;
        moveSpeed = 5.0f;
        hp = maxHp = 100.0f;
        invincible = 0.0f;
        knockback = glm::vec3(0.0f);
    }

    // 맞은 직후 0.15초 동안 1 → 0. 몸 색과 하늘색을 빨갛게 섞는 데 쓴다
    float HurtFlash()
    {
        float t = invincibleTime - invincible;
        if (invincible <= 0.0f || t > hurtFlashTime) return 0.0f;
        return 1.0f - t / hurtFlashTime;
    }

    // from: 때린 적의 위치. 그 반대쪽으로 밀려난다
    void TakeDamage(float amount, const glm::vec3& from)
    {
        if (invincible > 0.0f) return;
        hp -= amount;
        invincible = invincibleTime;

        glm::vec3 away = pos - from;
        away.y = 0.0f;
        if (glm::length(away) > 0.001f)
            knockback = glm::normalize(away) * 9.0f;

        // [효과] 맞은 자리에서 빨간 파편 + 화면 흔들림
        Effect::Burst(pos + glm::vec3(0.0f, 0.9f, 0.0f), { 1.0f, 0.25f, 0.2f }, 12, 5.0f, 0.18f);
        Effect::Shake(Effect::shakeTime);

        cout << "피격 -" << amount << "  체력 " << hp << " / " << maxHp << endl;
    }

    void Jump()
    {
        if (!onGround) return;
        velocityY = jumpSpeed;
        onGround = false;
    }

    void Update(float dt)
    {
        // 1) 키 입력 → 월드 기준 이동 방향. 카메라가 +z 쪽 뒤에 있으므로 W(-z)가 화면 안쪽이다.
        //    실습처럼 키 콜백에서 한 칸씩 옮기지 않고, 매 프레임 "지금 눌려 있는지" 확인한다.
        GLFWwindow* window = glfwGetCurrentContext();
        glm::vec3 dir(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dir.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dir.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dir.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dir.x += 1.0f;

        // 2) 대각선에서 빨라지지 않게 길이를 1로 맞추고, 움직이는 쪽을 바라본다.
        if (glm::length(dir) > 0.0f)
        {
            dir = glm::normalize(dir);
            pos += dir * moveSpeed * dt;
            yaw = glm::degrees(atan2(dir.x, dir.z));
            walkTime += dt;
        }
        else
        {
            walkTime = 0.0f;   // 멈추면 팔다리를 바로 세운다
        }

        // 넉백: 밀려나는 속도를 더하고, 마찰처럼 빠르게 줄인다 (총 1m 남짓 밀림)
        pos += knockback * dt;
        knockback *= max(0.0f, 1.0f - 10.0f * dt);

        // 3) 맵 밖으로 못 나가게 (실습 19 MoveTank와 같은 방식)
        float limit = mapHalf - bodyHalf;
        pos.x = glm::clamp(pos.x, -limit, limit);
        pos.z = glm::clamp(pos.z, -limit, limit);

        // 4) 장애물에 막힘 (점프로 윗면보다 높이 있으면 넘어간다)
        for (const auto& box : Map::obstacles)
            Collision::PushOutOfBox(pos, bodyHalf, box);

        if (invincible > 0.0f)
            invincible -= dt;

        // 5) 중력: 속도에서 중력을 빼고, 위치에 속도를 더한다 (실습 21 공과 같은 방식)
        velocityY -= gravity * dt;
        pos.y += velocityY * dt;
        if (pos.y <= 0.0f)
        {
            pos.y = 0.0f;
            velocityY = 0.0f;
            onGround = true;
        }
    }

    // 상자로 만든 로봇. 모든 부위가 root(위치 + 바라보는 방향)를 부모로 써서 같이 움직이고 돈다.
    // 바이저·가슴 코어가 있는 +z 쪽이 앞. (실습 19 탱크와 같은 방식)
    void Draw()
    {
        // [효과] 무적 시간 동안 깜빡인다 (0.1초마다 한 번씩 안 그림). 번쩍이는 처음 0.15초는 빼고.
        float flash = HurtFlash();
        if (invincible > 0.0f && flash <= 0.0f && fmod(invincible, 0.1f) < 0.05f)
            return;

        glm::mat4 root = glm::translate(glm::mat4(1.0f), pos);
        root = glm::rotate(root, glm::radians(yaw), glm::vec3(0, 1, 0));

        // 걸을 때 팔다리가 흔들리는 각도. sin으로 -1 ~ 1을 오간다 (35: 최대 각도, 10: 빠르기)
        float swing = 35.0f * sin(walkTime * 10.0f);

        // [효과] 맞은 직후에는 모든 부위를 빨간색 쪽으로 섞는다
        glm::vec3 red(1.0f, 0.15f, 0.1f);
        glm::vec3 armor = glm::mix(glm::vec3(0.86f, 0.88f, 0.92f), red, flash);   // 흰 장갑
        glm::vec3 joint = glm::mix(glm::vec3(0.18f, 0.2f, 0.25f), red, flash);    // 어두운 관절
        glm::vec3 accent = glm::mix(glm::vec3(0.95f, 0.5f, 0.15f), red, flash);   // 주황 포인트
        glm::vec3 glow = glm::mix(glm::vec3(0.3f, 0.95f, 1.0f), red, flash);      // 빛나는 하늘색

        // 다리: 엉덩이(y 0.5)로 옮기고 → x축으로 돌리고 → 아래로 내려 그린다 (실습 19 깃대와 같은 방식)
        for (int side = -1; side <= 1; side += 2)
        {
            glm::mat4 leg = glm::translate(root, glm::vec3(side * 0.17f, 0.5f, 0.0f));
            leg = glm::rotate(leg, glm::radians(swing * side), glm::vec3(1, 0, 0));
            DrawBox({ 0.2f, 0.42f, 0.22f }, { 0.0f, -0.21f, 0.0f }, joint, leg);    // 다리
            DrawBox({ 0.26f, 0.1f, 0.34f }, { 0.0f, -0.45f, 0.05f }, accent, leg);  // 발
        }

        DrawBox({ 0.5f, 0.15f, 0.35f }, { 0.0f, 0.55f, 0.0f }, joint, root);        // 골반
        DrawBox({ 0.72f, 0.55f, 0.46f }, { 0.0f, 0.9f, 0.0f }, armor, root);        // 몸통
        DrawBox({ 0.22f, 0.22f, 0.04f }, { 0.0f, 0.95f, 0.24f }, glow, root);       // 가슴 코어
        DrawBox({ 0.5f, 0.5f, 0.18f }, { 0.0f, 0.9f, -0.31f }, accent, root);       // 등 배낭

        // 팔: 어깨(y 1.1)에 매달려 다리와 반대로 흔들림
        for (int side = -1; side <= 1; side += 2)
        {
            glm::mat4 arm = glm::translate(root, glm::vec3(side * 0.48f, 1.1f, 0.0f));
            arm = glm::rotate(arm, glm::radians(-swing * side), glm::vec3(1, 0, 0));
            DrawBox({ 0.28f, 0.2f, 0.32f }, { 0.0f, 0.02f, 0.0f }, accent, arm);    // 어깨 보호대
            DrawBox({ 0.16f, 0.5f, 0.18f }, { 0.0f, -0.25f, 0.0f }, joint, arm);    // 팔
            DrawBox({ 0.2f, 0.14f, 0.2f }, { 0.0f, -0.52f, 0.0f }, armor, arm);     // 손
        }

        DrawBox({ 0.14f, 0.1f, 0.14f }, { 0.0f, 1.22f, 0.0f }, joint, root);        // 목
        DrawBox({ 0.52f, 0.4f, 0.46f }, { 0.0f, 1.45f, 0.0f }, armor, root);        // 머리
        DrawBox({ 0.44f, 0.12f, 0.04f }, { 0.0f, 1.48f, 0.24f }, glow, root);       // 바이저
        DrawBox({ 0.04f, 0.28f, 0.04f }, { 0.16f, 1.78f, 0.0f }, joint, root);      // 안테나
        DrawBox({ 0.09f, 0.09f, 0.09f }, { 0.16f, 1.94f, 0.0f }, accent, root);     // 안테나 끝
    }
}

// ---------- 기본 무기: 앞으로 나가는 자동 공격 (김민우) ----------
// 실습 21의 공(struct Ball + vector)과 같은 방식. 일정 시간마다 하나씩 쏘고, 수명이 다하면 지운다.
namespace Attack
{
    struct Bullet
    {
        glm::vec3 pos, dir;
        float life;            // 남은 시간(초). 0이 되면 사라짐
    };

    vector<Bullet> bullets;
    float cooldown;            // 다음 발사까지 남은 시간
    float damage;              // 한 발의 피해. 레벨업으로 오른다

    const float fireInterval = 0.5f;   // 발사 간격(초)
    const float bulletSpeed = 12.0f;   // 초당 이동 거리
    const float bulletLife = 1.5f;     // 사거리 = 속도 × 수명 = 18
    const float bulletRadius = 0.15f;  // 충돌 판정 반지름 (그리는 상자는 0.2)

    void Reset()
    {
        bullets.clear();
        cooldown = 0.0f;
        damage = 1.0f;
    }

    void Update(float dt)
    {
        // 1) 발사: 시간이 되면 플레이어가 바라보는 방향으로 하나 만든다
        cooldown -= dt;
        if (cooldown <= 0.0f)
        {
            cooldown = fireInterval;

            // 로봇을 y축으로 yaw만큼 돌렸으므로, 앞(+z)은 (sin, 0, cos) 방향이 된다
            float yawRad = glm::radians(Player::yaw);
            glm::vec3 dir(sin(yawRad), 0.0f, cos(yawRad));

            Bullet b;
            b.pos = Player::pos + glm::vec3(0.0f, 0.95f, 0.0f) + dir * 0.5f;   // 가슴 코어 앞에서 출발
            b.dir = dir;
            b.life = bulletLife;
            bullets.push_back(b);
        }

        // 2) 이동
        for (auto& b : bullets)
        {
            b.pos += b.dir * bulletSpeed * dt;
            b.life -= dt;
        }

        // 3) 수명이 다한 총알 지우기 (뒤에서부터 지워야 번호가 안 꼬인다)
        for (int i = (int)bullets.size() - 1; i >= 0; i--)
        {
            if (bullets[i].life <= 0.0f)
                bullets.erase(bullets.begin() + i);
        }
    }

    void Draw()
    {
        for (const auto& b : bullets)
            DrawBox({ 0.2f, 0.2f, 0.2f }, b.pos, { 0.3f, 0.95f, 1.0f });
    }
}

// ---------- 적 스폰·추적 (김민우) ----------
// 실습 21의 공처럼 struct + vector. 플레이어 둘레 원 위에서 생겨나 플레이어 쪽으로 걸어온다.
namespace Enemy
{
    // 종류별 능력치를 표 하나에 모았다. 밸런스를 바꿀 때는 여기만 고치면 된다.
    struct Type
    {
        float speed;       // 초당 이동 거리
        float hp;          // 체력
        float radius;      // 충돌 반지름 (몸 절반 너비)
        float height;      // 키. 플레이어가 이보다 높이 뛰면 안 닿는다
        float damage;      // 플레이어에 닿았을 때 피해
        int exp;           // 죽을 때 떨어뜨리는 젬의 경험치
        glm::vec3 color;
    };

    const Type types[3] =
    {
        //  속도   체력    반지름  키     피해    경험치  색
        { 2.2f,  3.0f, 0.45f, 1.2f, 10.0f, 1, { 0.6f, 0.3f, 0.75f } },   // 0 기본형
        { 3.8f,  2.0f, 0.35f, 0.9f,  8.0f, 1, { 0.9f, 0.3f, 0.25f } },   // 1 빠른형
        { 1.4f, 12.0f, 0.75f, 1.8f, 20.0f, 5, { 0.3f, 0.45f, 0.25f } },  // 2 탱커형
    };

    struct Monster
    {
        int type;          // types 표의 번호
        glm::vec3 pos;     // 발바닥 위치
        float yaw;         // 바라보는 방향(도). Player와 같은 기준
        float hp;
        float flash;       // [효과] 맞은 뒤 남은 번쩍임 시간. 하얗게 번쩍이며 찌그러진다
    };

    vector<Monster> monsters;
    float spawnTimer;      // 다음 스폰까지 남은 시간

    const float spawnDistance = 16.0f;   // 플레이어에서 이만큼 떨어진 원 위에 생긴다
    const int maxCount = 200;            // 동시에 있을 수 있는 최대 수
    const float flashTime = 0.12f;

    void Reset()
    {
        monsters.clear();
        spawnTimer = 0.3f;
    }

    // 처음엔 0.5초마다, 시간이 갈수록 빨라져 약 2분 뒤부터 0.15초마다 (예전: 1.2초 → 0.3초)
    float SpawnInterval()
    {
        return max(0.15f, 0.5f - playTime * 0.003f);
    }

    // 처음엔 기본형만. 1분 뒤 빠른형, 2분 뒤 탱커형이 섞인다
    int PickType()
    {
        int r = rand() % 10;
        if (playTime >= 120.0f && r < 2) return 2;   // 20%
        if (playTime >= 60.0f && r < 5) return 1;    // 30%
        return 0;
    }

    void Spawn()
    {
        // 1) 플레이어 둘레 원 위의 무작위 위치. 맵 밖이면 각도를 바꿔 다시 뽑는다.
        glm::vec3 p(0.0f);
        for (int tries = 0; tries < 10; tries++)
        {
            float angle = RandomRange(0.0f, glm::two_pi<float>());
            p = Player::pos + glm::vec3(cos(angle), 0.0f, sin(angle)) * spawnDistance;
            if (fabs(p.x) < mapHalf - 1.0f && fabs(p.z) < mapHalf - 1.0f)
                break;
        }

        // 2) 열 번 다 맵 밖이었으면 맵 끝에 붙인다
        p.x = glm::clamp(p.x, -mapHalf + 1.0f, mapHalf - 1.0f);
        p.z = glm::clamp(p.z, -mapHalf + 1.0f, mapHalf - 1.0f);
        p.y = 0.0f;

        Monster m;
        m.type = PickType();
        m.pos = p;
        m.yaw = 0.0f;
        m.hp = types[m.type].hp;
        m.flash = 0.0f;
        monsters.push_back(m);
    }

    void Update(float dt)
    {
        // 1) 스폰
        spawnTimer -= dt;
        if (spawnTimer <= 0.0f && (int)monsters.size() < maxCount)
        {
            spawnTimer = SpawnInterval();
            Spawn();
        }

        // 2) 추적: 플레이어 쪽(xz)으로 걸어가고 그쪽을 바라본다. 몸이 닿을 거리에서는 멈춘다.
        for (auto& m : monsters)
        {
            if (m.flash > 0.0f)
                m.flash -= dt;

            glm::vec3 toPlayer = Player::pos - m.pos;
            toPlayer.y = 0.0f;
            float dist = glm::length(toPlayer);
            if (dist < 0.001f)
                continue;

            glm::vec3 dir = toPlayer / dist;
            m.yaw = glm::degrees(atan2(dir.x, dir.z));   // Player와 같은 공식

            if (dist > types[m.type].radius + Player::bodyHalf)
                m.pos += dir * types[m.type].speed * dt;
        }

        // 3) 밀어내기: 겹친 두 적을 겹친 만큼 반씩 서로 반대로 민다 → 한 점에 뭉치지 않는다
        for (int i = 0; i < (int)monsters.size(); i++)
        {
            for (int j = i + 1; j < (int)monsters.size(); j++)
            {
                Monster& a = monsters[i];
                Monster& b = monsters[j];
                glm::vec3 diff = b.pos - a.pos;
                diff.y = 0.0f;
                float dist = glm::length(diff);
                float minDist = types[a.type].radius + types[b.type].radius;

                if (dist < minDist && dist > 0.0001f)
                {
                    glm::vec3 push = diff / dist * ((minDist - dist) * 0.5f);
                    a.pos -= push;
                    b.pos += push;
                }
            }
        }

        // 4) 장애물과 맵 끝
        for (auto& m : monsters)
        {
            for (const auto& box : Map::obstacles)
                Collision::PushOutOfBox(m.pos, types[m.type].radius, box);

            float limit = mapHalf - types[m.type].radius;
            m.pos.x = glm::clamp(m.pos.x, -limit, limit);
            m.pos.z = glm::clamp(m.pos.z, -limit, limit);
        }
    }

    // [임시 모양] 몸통 + 눈 두 개. 몬스터 모델·걷기 애니메이션(홍범기)이 들어오면 이 함수만 바꾸면 된다.
    // 플레이어와 같이 root(위치 + 방향)를 부모로 써서, 눈이 있는 +z 쪽이 플레이어를 향한다.
    void Draw()
    {
        for (const auto& m : monsters)
        {
            const Type& t = types[m.type];
            glm::mat4 root = glm::translate(glm::mat4(1.0f), m.pos);
            root = glm::rotate(root, glm::radians(m.yaw), glm::vec3(0, 1, 0));

            // [효과] 맞은 직후: 하얗게 번쩍이고, 위아래로 눌리고 옆으로 퍼진다 (발바닥 기준이라 바닥에 붙어 있다)
            float k = max(0.0f, m.flash / flashTime);   // 1 → 0
            root = glm::scale(root, glm::vec3(1.0f + 0.2f * k, 1.0f - 0.3f * k, 1.0f + 0.2f * k));
            glm::vec3 bodyColor = glm::mix(t.color, glm::vec3(1.0f), k * 0.85f);

            float width = t.radius * 2.0f;
            DrawBox({ width, t.height, width }, { 0.0f, t.height * 0.5f, 0.0f }, bodyColor, root);

            for (int side = -1; side <= 1; side += 2)
                DrawBox({ width * 0.2f, width * 0.2f, 0.05f },
                        { side * width * 0.22f, t.height * 0.75f, t.radius + 0.025f },
                        { 1.0f, 1.0f, 0.6f }, root);
        }
    }
}

// ---------- 경험치·레벨업 (김민우) ----------
// 경험치가 차면 게임을 멈추고 업그레이드 3개를 뽑아 보여 준다. 1·2·3 키로 고르면 적용하고 다시 진행.
// 카드 UI는 홍범기 담당 → UI는 choosing, choices[], upgradeNames[]만 읽어서 그리면 된다.
// UI가 들어오기 전까지는 콘솔 창에 선택지를 출력한다.
namespace Growth
{
    int level;
    int exp;               // 지금 레벨에서 모은 경험치
    float pickupRange;     // 젬이 끌려오기 시작하는 거리 (흡수 범위)
    bool choosing;         // 레벨업 선택 중이면 true → 게임이 멈춘다
    int choices[3];        // 이번에 뽑힌 업그레이드 번호 3개

    const int upgradeCount = 4;
    const char* upgradeNames[upgradeCount] =
    {
        "공격력 +1",          // 0
        "이동 속도 +10%",     // 1
        "최대 체력 +20",      // 2
        "흡수 범위 +1",       // 3
    };

    void Reset()
    {
        level = 1;
        exp = 0;
        pickupRange = 2.0f;
        choosing = false;
    }

    // 다음 레벨까지 필요한 경험치: 5, 10, 15, ...
    int NeedExp()
    {
        return level * 5;
    }

    void StartChoice()
    {
        // 서로 다른 3개 뽑기: 번호 4개를 무작위로 섞고 앞에서 3개를 쓴다 → 중복이 생길 수 없다
        int order[upgradeCount] = { 0, 1, 2, 3 };
        for (int i = upgradeCount - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            swap(order[i], order[j]);
        }

        for (int i = 0; i < 3; i++)
            choices[i] = order[i];
        choosing = true;

        cout << "\n===== 레벨업! Lv " << level << " =====" << endl;
        for (int i = 0; i < 3; i++)
            cout << "  " << (i + 1) << ": " << upgradeNames[choices[i]] << endl;
    }

    // 경험치가 찼으면 레벨을 올리고 선택을 시작한다
    void CheckLevelUp()
    {
        if (choosing || exp < NeedExp())
            return;

        exp -= NeedExp();
        level++;
        StartChoice();
    }

    void AddExp(int amount)
    {
        exp += amount;
        CheckLevelUp();
    }

    // slot: 0, 1, 2 (화면의 1·2·3번 카드)
    void Choose(int slot)
    {
        if (!choosing)
            return;

        int upgrade = choices[slot];
        if (upgrade == 0)
        {
            Attack::damage += 1.0f;
        }
        else if (upgrade == 1)
        {
            Player::moveSpeed *= 1.1f;
        }
        else if (upgrade == 2)
        {
            Player::maxHp += 20.0f;
            Player::hp += 20.0f;
        }
        else
        {
            pickupRange += 1.0f;
        }

        cout << "선택: " << upgradeNames[upgrade] << endl;
        choosing = false;

        CheckLevelUp();   // 젬을 한꺼번에 많이 먹어 경험치가 또 찼으면 바로 다음 선택
    }
}

// ---------- 경험치 젬 (김민우) ----------
// 적이 죽은 자리에 생겨 돌면서 떠 있다가, 흡수 범위에 들어오면 플레이어에게 날아온다.
namespace Gem
{
    struct Item
    {
        glm::vec3 pos;
        int value;         // 경험치
        bool flying;       // 한 번 끌려오기 시작하면 범위를 벗어나도 끝까지 따라온다
    };

    vector<Item> gems;
    float spin;            // 회전 각도(도). 모든 젬이 같이 돈다

    const float flySpeed = 10.0f;
    const float collectDistance = 0.5f;

    void Reset()
    {
        gems.clear();
        spin = 0.0f;
    }

    void Drop(const glm::vec3& pos, int value)
    {
        gems.push_back({ glm::vec3(pos.x, 0.0f, pos.z), value, false });
    }

    void Update(float dt)
    {
        spin += 120.0f * dt;

        for (int i = (int)gems.size() - 1; i >= 0; i--)
        {
            Item& g = gems[i];
            glm::vec3 toPlayer = Player::pos - g.pos;
            toPlayer.y = 0.0f;
            float dist = glm::length(toPlayer);

            if (dist < Growth::pickupRange)
                g.flying = true;

            if (g.flying && dist > 0.001f)
                g.pos += toPlayer / dist * min(flySpeed * dt, dist);   // 지나치지 않게 남은 거리까지만

            if (dist < collectDistance)
            {
                Growth::AddExp(g.value);
                gems.erase(gems.begin() + i);   // 뒤에서부터 지우므로 번호가 안 꼬인다
            }
        }
    }

    void Draw()
    {
        for (const auto& g : gems)
        {
            // 위아래로 떠다니기: 위치마다 위상을 달리해서 다 같이 움직이지 않게
            float bob = 0.4f + 0.15f * sin(glm::radians(spin * 2.0f) + g.pos.x);

            // 순서: 제자리로 옮기기 → y축으로 돌리기 → 비스듬히 세워 다이아몬드처럼 보이게
            glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(g.pos.x, bob, g.pos.z));
            m = glm::rotate(m, glm::radians(spin), glm::vec3(0, 1, 0));
            m = glm::rotate(m, glm::radians(45.0f), glm::vec3(1, 0, 1));

            glm::vec3 color = (g.value >= 5) ? glm::vec3(1.0f, 0.85f, 0.2f) : glm::vec3(0.3f, 1.0f, 0.5f);
            DrawBox({ 0.25f, 0.25f, 0.25f }, { 0.0f, 0.0f, 0.0f }, color, m);
        }
    }
}

// ---------- 충돌 판정 (김민우) ----------
// 위의 Collision 도구로, 매 프레임 "누가 누구와 닿았는지" 확인하고 결과를 적용한다.
namespace Collision
{
    // 투사체 ↔ 장애물: 총알 높이보다 높은 장애물(기둥·나무·큰 상자)에 닿으면 불꽃을 튀기고 사라진다.
    void BulletsVsObstacles()
    {
        for (auto& b : Attack::bullets)
        {
            if (b.life <= 0.0f)
                continue;

            for (const auto& box : Map::obstacles)
            {
                glm::vec3 half = box.size * 0.5f;
                if (b.pos.y >= box.center.y + half.y)
                    continue;   // 낮은 장애물 위로 지나감

                float r = Attack::bulletRadius;
                if (fabs(b.pos.x - box.center.x) < half.x + r && fabs(b.pos.z - box.center.z) < half.z + r)
                {
                    Effect::Burst(b.pos - b.dir * 0.2f, { 1.0f, 0.9f, 0.5f }, 5, 3.5f, 0.1f);
                    b.life = 0.0f;
                    break;
                }
            }
        }
    }

    // 투사체 ↔ 적: 닿으면 투사체는 사라지고 적 체력이 준다. 0 이하면 적을 지우고 젬을 떨어뜨린다.
    void BulletsVsEnemies()
    {
        for (auto& b : Attack::bullets)
        {
            if (b.life <= 0.0f)
                continue;   // 이번 프레임에 장애물에 이미 막힌 총알

            for (auto& m : Enemy::monsters)
            {
                if (m.hp <= 0.0f)
                    continue;   // 이번 프레임에 이미 죽은 적

                const Enemy::Type& t = Enemy::types[m.type];
                if (CircleOverlap(b.pos, Attack::bulletRadius, m.pos, t.radius))
                {
                    m.hp -= Attack::damage;
                    b.life = 0.0f;   // 다음 Attack::Update에서 지워진다

                    // [효과] 번쩍임 + 총알 방향으로 살짝 밀림(큰 적일수록 덜) + 맞은 자리에서 작은 파편
                    m.flash = Enemy::flashTime;
                    m.pos += b.dir * (0.12f / t.radius);
                    Effect::Burst(b.pos, glm::mix(t.color, glm::vec3(1.0f), 0.4f), 5, 3.0f, 0.12f);
                    break;           // 한 발은 한 마리만
                }
            }
        }

        for (int i = (int)Enemy::monsters.size() - 1; i >= 0; i--)
        {
            const Enemy::Monster& m = Enemy::monsters[i];
            if (m.hp > 0.0f)
                continue;

            // [효과] 죽으면 몸 색 파편이 크게 터진다. 탱커형은 더 많이, 화면도 살짝 흔들림
            const Enemy::Type& t = Enemy::types[m.type];
            glm::vec3 center = m.pos + glm::vec3(0.0f, t.height * 0.5f, 0.0f);
            Effect::Burst(center, t.color, (int)(10 + t.radius * 12), 5.0f, t.radius * 0.5f);
            Effect::Burst(center, glm::vec3(1.0f), 4, 4.0f, 0.12f);
            if (m.type == 2)
                Effect::Shake(0.12f);

            Gem::Drop(m.pos, Enemy::types[m.type].exp);
            killCount++;
            Enemy::monsters.erase(Enemy::monsters.begin() + i);
        }
    }

    // 플레이어 ↔ 적: 닿으면 피해. 점프로 적 키보다 높이 있으면 안 닿는다.
    void PlayerVsEnemies()
    {
        for (const auto& m : Enemy::monsters)
        {
            const Enemy::Type& t = Enemy::types[m.type];
            if (Player::pos.y < t.height && CircleOverlap(Player::pos, Player::bodyHalf + 0.05f, m.pos, t.radius))
                Player::TakeDamage(t.damage, m.pos);
        }
    }
}

// ---------- 3인칭 카메라 (김민우) ----------
namespace Camera
{
    glm::vec3 offset(0.0f, 6.0f, 10.0f);   // 플레이어 기준 카메라 위치 (뒤·위)

    glm::mat4 View()
    {
        // 점프할 때 화면이 같이 출렁이지 않게 높이(y)는 따라가지 않는다
        glm::vec3 target(Player::pos.x, 0.0f, Player::pos.z);

        // [효과] 흔들림: 남은 시간에 비례하는 크기로 바라보는 점을 무작위로 흔든다
        if (Effect::shake > 0.0f)
        {
            float a = 0.6f * (Effect::shake / Effect::shakeTime);
            target += glm::vec3(RandomRange(-a, a), RandomRange(-a, a) * 0.5f, RandomRange(-a, a));
        }

        return glm::lookAt(target + offset,                   // Eye: 플레이어 뒤·위
                           target + glm::vec3(0, 1.0f, 0),    // At: 발이 아니라 몸 가운데쯤
                           glm::vec3(0, 1, 0));               // Up
    }
}

// ---------- 실습과 같은 Reset / Update / DrawScene / KeyCallback ----------
void Reset()
{
    playTime = 0.0f;
    killCount = 0;
    Map::Reset();
    Player::Reset();
    Attack::Reset();
    Enemy::Reset();
    Growth::Reset();
    Gem::Reset();
    Effect::Reset();
}

// [추가] HUD(홍범기)가 들어오기 전까지 창 제목에 상태를 띄워서 확인한다. HUD가 생기면 지운다.
void ShowStatusInTitle()
{
    string title = "Oddlands  |  HP " + to_string((int)ceil(Player::hp)) + "/" + to_string((int)Player::maxHp)
        + "  |  Lv " + to_string(Growth::level) + " (" + to_string(Growth::exp) + "/" + to_string(Growth::NeedExp()) + ")"
        + "  |  " + to_string((int)playTime) + "s  |  처치 " + to_string(killCount);
    if (Growth::choosing)
        title += "  |  레벨업! 1·2·3 중 선택 (콘솔 창 참고)";

    glfwSetWindowTitle(glfwGetCurrentContext(), title.c_str());
}

void Update(float dt)
{
    ShowStatusInTitle();

    if (Growth::choosing)
        return;   // 레벨업 선택 중에는 게임 정지

    playTime += dt;

    // 순서: 움직이기 → 공격 → 닿았는지 판정 → 젬 줍기
    Player::Update(dt);
    Enemy::Update(dt);
    Attack::Update(dt);
    Collision::BulletsVsObstacles();
    Collision::BulletsVsEnemies();
    Collision::PlayerVsEnemies();
    Gem::Update(dt);
    Effect::Update(dt);

    // 게임오버 화면은 3단계에서. 지금은 기록을 출력하고 바로 다시 시작한다.
    if (Player::hp <= 0.0f)
    {
        cout << "\n===== 게임오버 =====  생존 " << (int)playTime << "초, 처치 " << killCount
             << ", Lv " << Growth::level << "\n" << endl;
        Reset();
    }
}

void DrawScene()
{
    // [효과] 플레이어가 맞은 직후에는 하늘이 붉게 번쩍인다
    glm::vec3 sky = glm::mix(glm::vec3(0.55f, 0.75f, 0.95f), glm::vec3(0.9f, 0.35f, 0.35f), Player::HurtFlash() * 0.6f);
    BeginFrame(sky, Camera::View(),
        glm::perspective(glm::radians(60.0f), (float)windowWidth / windowHeight, 0.1f, 200.0f));

    Map::Draw();
    Gem::Draw();
    Enemy::Draw();
    Player::Draw();
    Attack::Draw();
    Effect::Draw();
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }

    // 레벨업 선택 중에는 1·2·3만 받는다 (점프 등이 멈춘 사이에 들어가지 않게)
    if (Growth::choosing)
    {
        if (key == GLFW_KEY_1) Growth::Choose(0);
        else if (key == GLFW_KEY_2) Growth::Choose(1);
        else if (key == GLFW_KEY_3) Growth::Choose(2);
        return;
    }

    if (key == GLFW_KEY_SPACE) Player::Jump();
}

// [추가] 실행 파일이 있는 폴더를 작업 폴더로 바꾼다.
// Visual Studio, Rider, 탐색기 더블클릭 등 어디서 실행해도 "shaders/..." 경로가 같은 곳을 가리키게 하기 위함.
void SetWorkingDirectoryToExe()
{
    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(nullptr, path, MAX_PATH) > 0)
    {
        error_code ec;
        filesystem::current_path(filesystem::path(path).parent_path(), ec);
    }
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    SetWorkingDirectoryToExe();
    srand((unsigned)time(nullptr));   // 실행할 때마다 다른 스폰 위치·업그레이드

    GLFWwindow* window = Window::Open(windowWidth, windowHeight, "Oddlands");
    if (!window) return -1;
    cout << "OpenGL " << glGetString(GL_VERSION) << endl;
    glEnable(GL_CULL_FACE);   // 상자 뒷면은 안 그린다 (실습 21과 같음)

    // 실습 21의 RunPractice21과 같은 순서: 창 열기 → 초기화 → 콜백 등록 → 루프
    Reset();
    glfwSetKeyCallback(window, KeyCallback);
    Window::Run(window, Update, DrawScene);
    return 0;
}
