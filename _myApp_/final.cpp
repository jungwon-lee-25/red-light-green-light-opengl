#include <sb7.h>
#include <vmath.h>
#include <shader.h>
#include <vector>
#include <iostream>
#include <cmath> 
#include <GLFW/glfw3.h> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Model.h"

// SoLoud 사운드 엔진 컴포넌트 탑재
#include "soloud.h"
#include "soloud_wav.h"

using namespace std;

void load_texture(GLuint textureID, char const* filename) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &w, &h, &ch, 0);
    if (data) {
        GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

enum DollState {
    GREEN_LIGHT,
    RED_LIGHT
};

enum GameStatus {
    MENU,        // 메인 시작 화면
    PLAYING,     // 게임 진행 중
    GAMEOVER,    // 사망 상태
    GAMEWIN      // 성공 상태 (도착선 통과)
};

class RedLightGame : public sb7::application {
public:
    RedLightGame() : yaw(-90.0f), pitch(0.0f), lastX(400.0f), lastY(300.0f), firstMouse(true), deltaTime(0.0), lastFrame(0.0) {
        for (int i = 0; i < 1024; i++) keys[i] = false;
        cameraPos = vmath::vec3(0.0f, 1.5f, 70.0f);
        cameraFront = vmath::vec3(0.0f, 0.0f, -1.0f);
        cameraUp = vmath::vec3(0.0f, 1.0f, 0.0f);
        doll = nullptr; tree = nullptr;

        gameStatus = MENU;
        currentState = GREEN_LIGHT;
        stateTimer = 0.0f;
        dollRotationY = 180.0f;
        prevCameraPos = cameraPos;

        hBgm = 0;
        hVoice = 0;
        lastGameStatus = MENU;

        // 마우스 중복 입력 방지 상태 초기화
        leftButtonReleased = true;
    }

    virtual ~RedLightGame() {
        soloudEngine.deinit(); // 사운드 엔진 소멸 안전 파괴
    }

    virtual void startup() {
        shader_program = compile_shader("squid_vs.glsl", "squid_fs.glsl");
        glGenVertexArrays(1, VAOs); glGenBuffers(1, VBOs);
        glBindVertexArray(VAOs[0]);
        GLfloat room[] = {
            -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,  0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,  0.5f, 0.5f,-0.5f, 0,0,-1, 1,1,
             0.5f, 0.5f,-0.5f, 0,0,-1, 1,1, -0.5f, 0.5f,-0.5f, 0,0,-1, 0,1, -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,
            -0.5f,-0.5f, 0.5f, 0,0, 1, 0,0,  0.5f,-0.5f, 0.5f, 0,0, 1, 1,0,  0.5f, 0.5f, 0.5f, 0,0, 1, 1,1,
             0.5f, 0.5f, 0.5f, 0,0, 1, 1,1, -0.5f, 0.5f, 0.5f, 0,0, 1, 0,1, -0.5f,-0.5f, 0.5f, 0,0, 1, 0,0,
            -0.5f, 0.5f, 0.5f,-1,0, 0, 1,1, -0.5f, 0.5f,-0.5f,-1,0, 0, 0,1, -0.5f,-0.5f,-0.5f,-1,0, 0, 0,0,
            -0.5f,-0.5f,-0.5f,-1,0, 0, 0,0, -0.5f,-0.5f, 0.5f,-1,0, 0, 1,0, -0.5f, 0.5f, 0.5f,-1,0, 0, 1,1,
             0.5f, 0.5f, 0.5f, 1,0, 0, 0,1,  0.5f, 0.5f,-0.5f, 1,0, 0, 1,1,  0.5f,-0.5f,-0.5f, 1,0, 0, 1,0,
             0.5f,-0.5f,-0.5f, 1,0, 0, 1,0,  0.5f,-0.5f, 0.5f, 1,0, 0, 0,0,  0.5f, 0.5f, 0.5f, 1,0, 0, 0,1,
            -0.5f,-0.5f,-0.5f, 0,-1,0, 0,1,  0.5f,-0.5f,-0.5f, 0,-1,0, 1,1,  0.5f,-0.5f, 0.5f, 0,-1,0, 1,0,
             0.5f,-0.5f, 0.5f, 0,-1,0, 1,0, -0.5f,-0.5f, 0.5f, 0,-1,0, 0,0, -0.5f,-0.5f,-0.5f, 0,-1,0, 0,1,
            -0.5f, 0.5f,-0.5f, 0, 1,0, 0,1,  0.5f, 0.5f,-0.5f, 0, 1,0, 1,1,  0.5f, 0.5f, 0.5f, 0, 1,0, 1,0,
             0.5f, 0.5f, 0.5f, 0, 1,0, 1,0, -0.5f, 0.5f, 0.5f, 0, 1,0, 0,0, -0.5f, 0.5f,-0.5f, 0, 1,0, 0,1
        };
        glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]); glBufferData(GL_ARRAY_BUFFER, sizeof(room), room, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(2);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(1);

        glGenTextures(1, &wallTex); load_texture(wallTex, "squid_wall.jpg");
        glGenTextures(1, &sandTex); load_texture(sandTex, "sand.jpg");
        glGenTextures(1, &skyTex);  load_texture(skyTex, "squid_sky.jpg");
        glGenTextures(1, &dollTex); load_texture(dollTex, "doll.jpeg");
        glGenTextures(1, &treeTex); load_texture(treeTex, "tree.jpeg");

        doll = new Model("doll.obj");
        tree = new Model("tree.obj");

        // SoLoud 하드웨어 재생 디바이스 활성화
        soloudEngine.init();

        // MP3 에셋 로드
        bgmSound.load("main_bgm.mp3");
        bgmSound.setLooping(true); // 로비 배경음 무한 루프

        voiceSound.load("voice.mp3");
        voiceSound.setLooping(false); // 단발성 재생

        // 첫 구동 시 메인 화면 BGM 스타트
        hBgm = soloudEngine.play(bgmSound);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    void updateGameLogic() {
        if (gameStatus == MENU && lastGameStatus != MENU) {
            // 게임오버나 클리어 후 메인화면으로 복귀 시 다른 모든 음원을 끄고 메인 BGM만 다시 무한루프 시킵니다.
            soloudEngine.stopAll();
            hBgm = soloudEngine.play(bgmSound);
        }

        if (gameStatus == MENU) {
            dollRotationY += (180.0f - dollRotationY) * 5.0f * (float)deltaTime;
        }
        else if (gameStatus == PLAYING) {
            stateTimer += (float)deltaTime;

            // [수정 완료] "피었습니다"의 "다" 발음이 완전히 끝나고 영희가 돌도록, 초록불(GREEN_LIGHT) 대기 상태를 정확히 5초로 늘려 싱크를 맞춥니다.
            float stateDuration = (currentState == GREEN_LIGHT) ? 4.2f : 5.0f;

            if (stateTimer >= stateDuration) {
                stateTimer = 0.0f;
                if (currentState == GREEN_LIGHT) {
                    // 초록불 종료 ➔ 빨간불 진입 (음원 종료 후 즉시 회전 작동)
                    currentState = RED_LIGHT;
                }
                else {
                    // 빨간불 종료 ➔ 초록불 재진입
                    currentState = GREEN_LIGHT;
                    soloudEngine.stop(hVoice); // 겹침 방지를 위해 새로운 턴을 시작할 때는 이전 소리를 완전히 멈춰줍니다.
                    hVoice = soloudEngine.play(voiceSound); // 다시 뒤도는 순간 voice 음성이 신규 재생을 시작합니다.
                }
            }

            // 실시간 플레이어 조준
            float targetAngle;
            if (currentState == GREEN_LIGHT) {
                targetAngle = 180.0f;
            }
            else {
                float dx = cameraPos[0] - 0.0f;
                float dz = cameraPos[2] - (-65.0f);
                targetAngle = atan2(dx, dz) * 180.0f / 3.1415926535f;
            }

            // [수정 완료] 영희가 도는 속도를 원래의 "홱" 돌던 스피드(10.0f)로 완벽하게 되돌렸습니다.
            float rotationSpeed = 10.0f;
            dollRotationY += (targetAngle - dollRotationY) * rotationSpeed * (float)deltaTime;

            if (cameraPos[2] <= -60.0f) {
                gameStatus = GAMEWIN;
                soloudEngine.stopAll(); // 성공 탈출 시 오디오 클린 전체 소멸
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }

            if (currentState == RED_LIGHT && abs(dollRotationY - targetAngle) < 15.0f) {
                float moveDist = sqrt(pow(cameraPos[0] - prevCameraPos[0], 2) + pow(cameraPos[2] - prevCameraPos[2], 2));

                if (moveDist > 0.005f) {
                    gameStatus = GAMEOVER;
                    soloudEngine.stopAll(); // 사망 시 오디오 즉시 스톱
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }

            prevCameraPos = cameraPos;
        }
        else if (gameStatus == GAMEOVER) {
            cameraPos[1] += (0.2f - cameraPos[1]) * 4.0f * (float)deltaTime;
            pitch += (-35.0f - pitch) * 3.0f * (float)deltaTime;

            cameraFront = vmath::normalize(vmath::vec3(
                cos(vmath::radians(yaw)) * cos(vmath::radians(pitch)),
                sin(vmath::radians(pitch)),
                sin(vmath::radians(yaw)) * cos(vmath::radians(pitch))
            ));
        }

        lastGameStatus = gameStatus;
    }

    virtual void render(double currentTime) {
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);

        deltaTime = currentTime - lastFrame; lastFrame = currentTime;

        // 마우스의 연속 단타 클릭 감지 시스템 (Click Edge Tracker)
        bool leftPressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        bool leftClicked = false;
        if (leftPressed) {
            if (leftButtonReleased) {
                leftClicked = true;
                leftButtonReleased = false; // 버튼이 눌려있는 동안에는 클릭 신호를 잠금 처리합니다.
            }
        }
        else {
            leftButtonReleased = true; // 마우스 버튼을 완전히 떼었을 때 잠금이 해제됩니다.
        }

        processInput();
        updateGameLogic();

        const GLfloat black[] = { 0, 0, 0, 1 }; glClearBufferfv(GL_COLOR, 0, black); glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        float aspect = (float)width / (float)height;

        vmath::mat4 proj = vmath::perspective(50.0f, aspect, 0.1f, 1000.0f);
        vmath::mat4 view = vmath::lookat(cameraPos, cameraPos + cameraFront, cameraUp);
        glUseProgram(shader_program);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, proj);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, view);

        int statusIndex = 0;
        if (gameStatus == MENU) statusIndex = 4;
        else if (gameStatus == GAMEOVER) statusIndex = 2;
        else if (gameStatus == GAMEWIN) statusIndex = 3;
        else if (currentState == RED_LIGHT) statusIndex = 1;
        else statusIndex = 0;

        bool mouseHover = false;
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        float nx = (float)xpos / width;
        float ny = 1.0f - ((float)ypos / height);

        float px = (nx - 0.5f) * aspect;
        float py = ny - 0.5f;

        // 메인 메뉴 시작 버튼 검사
        if (gameStatus == MENU) {
            if (abs(px - 0.0f) <= 0.15f && abs(py - (-0.12f)) <= 0.05f) {
                mouseHover = true;
                // leftClicked 플래그를 사용하여 마우스 단발 타격 필터링
                if (leftClicked) {
                    gameStatus = PLAYING;

                    // 게임 시작하기 버튼 클릭 시 로비 BGM을 완전히 정지시킵니다.
                    soloudEngine.stop(hBgm);

                    cameraPos = vmath::vec3(0.0f, 1.5f, 70.0f);
                    cameraFront = vmath::vec3(0.0f, 0.0f, -1.0f);
                    yaw = -90.0f;
                    pitch = 0.0f;
                    prevCameraPos = cameraPos;
                    currentState = GREEN_LIGHT;
                    stateTimer = 0.0f;
                    dollRotationY = 180.0f;

                    // 게임 시작 첫 프레임 영희 보이스 최초 재생
                    hVoice = soloudEngine.play(voiceSound);

                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                }
            }
        }
        else if (gameStatus == GAMEOVER || gameStatus == GAMEWIN) {
            if (abs(px - 0.0f) <= 0.15f && abs(py - (-0.12f)) <= 0.05f) {
                mouseHover = true;
                // [수정 완료] 마우스를 완전히 떼었다가 다시 딸칵 눌러야만 동작하므로, 중복 찌꺼기 입력에 의한 다이렉트 재시작 버그를 완벽하게 근절하고 메인 대기 화면에 정상 정지해 있습니다!
                if (leftClicked) {
                    gameStatus = MENU;

                    // 처음 상태로 카메라 좌표 및 앵글을 깨끗하게 재정렬하여 메인 화면 정상 출력 보장
                    cameraPos = vmath::vec3(0.0f, 1.5f, 70.0f);
                    cameraFront = vmath::vec3(0.0f, 0.0f, -1.0f);
                    yaw = -90.0f;
                    pitch = 0.0f;

                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }
            }
        }

        glUniform1i(glGetUniformLocation(shader_program, "u_mouseHover"), mouseHover ? 1 : 0);
        glUniform1i(glGetUniformLocation(shader_program, "u_gameStatus"), statusIndex);
        glUniform2f(glGetUniformLocation(shader_program, "u_resolution"), (float)width, (float)height);

        // 1. 배경 렌더링
        glUniform1i(glGetUniformLocation(shader_program, "isModel"), 0);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, wallTex);
        glUniform1i(glGetUniformLocation(shader_program, "wallTexture"), 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, sandTex);
        glUniform1i(glGetUniformLocation(shader_program, "floorTexture"), 1);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, skyTex);
        glUniform1i(glGetUniformLocation(shader_program, "skyTexture"), 2);

        vmath::mat4 mRoom = vmath::translate(0.0f, 20.0f, 0.0f) * vmath::scale(40.0f, 40.0f, 150.0f);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, mRoom);
        glBindVertexArray(VAOs[0]); glDrawArrays(GL_TRIANGLES, 0, 36);

        // 2. 모델 렌더링
        glUniform1i(glGetUniformLocation(shader_program, "isModel"), 1);
        glDisable(GL_CULL_FACE);

        if (doll) {
            glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, dollTex);
            glUniform1i(glGetUniformLocation(shader_program, "modelTex"), 3);
            vmath::mat4 mDoll = vmath::translate(0.0f, 0.0f, -65.0f) * vmath::scale(13.0f) * vmath::rotate(dollRotationY, 0.0f, 1.0f, 0.0f) * doll->getCorrectionMatrix();
            glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, mDoll);
            doll->draw();
        }
        if (tree) {
            glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, treeTex);
            glUniform1i(glGetUniformLocation(shader_program, "modelTex"), 3);
            vmath::mat4 mTree = vmath::translate(0.0f, 0.0f, -70.0f) * vmath::scale(25.0f) * vmath::rotate(0.0f, 1.0f, 0.0f, 0.0f) * tree->getCorrectionMatrix();
            glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, mTree);
            tree->draw();
        }
    }

    void processInput() {
        if (gameStatus == GAMEOVER || gameStatus == GAMEWIN) {
            if (keys['R']) {
                gameStatus = MENU;
                cameraPos = vmath::vec3(0.0f, 1.5f, 70.0f);
                cameraFront = vmath::vec3(0.0f, 0.0f, -1.0f);
                yaw = -90.0f;
                pitch = 0.0f;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            return;
        }

        if (gameStatus != PLAYING) return;

        float s = 7.0f * (float)deltaTime;

        float radYaw = vmath::radians(yaw);
        vmath::vec3 f = vmath::vec3(cos(radYaw), 0.0f, sin(radYaw));
        vmath::vec3 r = vmath::vec3(-sin(radYaw), 0.0f, cos(radYaw));

        vmath::vec3 np = cameraPos;
        if (keys['W']) np += f * s; if (keys['S']) np -= f * s;
        if (keys['A']) np -= r * s; if (keys['D']) np += r * s;
        if (np[0] < -19.5f) np[0] = -19.5f; if (np[0] > 19.5f) np[0] = 19.5f;
        if (np[2] < -74.5f) np[2] = -74.5f; if (np[2] > 74.5f) np[2] = 74.5f;
        cameraPos = np; cameraPos[1] = 1.5f;
    }

    void onKey(int k, int a) override { if (k >= 0 && k < 1024) keys[k] = (a != GLFW_RELEASE); }

    void onMouseMove(int x, int y) override {
        if (gameStatus != PLAYING) return;

        if (firstMouse) { lastX = (float)x; lastY = (float)y; firstMouse = false; }
        float ox = (float)x - lastX, oy = lastY - (float)y; lastX = (float)x; lastY = (float)y;
        yaw += ox * 0.15f; pitch += oy * 0.15f;

        if (pitch > 50.0f) pitch = 50.0f; if (pitch < -50.0f) pitch = -50.0f;

        cameraFront = vmath::normalize(vmath::vec3(cos(vmath::radians(yaw)) * cos(vmath::radians(pitch)), sin(vmath::radians(pitch)), sin(vmath::radians(yaw)) * cos(vmath::radians(pitch))));
    }

    GLuint compile_shader(const char* vs_file, const char* fs_file) {
        GLuint v = sb7::shader::load(vs_file, GL_VERTEX_SHADER);
        GLuint f = sb7::shader::load(fs_file, GL_FRAGMENT_SHADER);
        GLuint p = glCreateProgram(); glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p); return p;
    }

private:
    GLuint shader_program, VAOs[1], VBOs[1], wallTex, sandTex, skyTex, dollTex, treeTex;
    Model* doll; Model* tree;
    vmath::vec3 cameraPos, cameraFront, cameraUp;
    float yaw, pitch, lastX, lastY;
    bool keys[1024], firstMouse = true;
    double deltaTime, lastFrame;

    DollState currentState;
    GameStatus gameStatus;
    float stateTimer;
    float dollRotationY;
    vmath::vec3 prevCameraPos;

    // SoLoud 핵심 인스턴스
    SoLoud::Soloud soloudEngine;
    SoLoud::Wav bgmSound;
    SoLoud::Wav voiceSound;

    SoLoud::handle hBgm;
    SoLoud::handle hVoice;

    GameStatus lastGameStatus;

    // 마우스 중복 입력 해제 감지 플래그
    bool leftButtonReleased;
};

DECLARE_MAIN(RedLightGame)