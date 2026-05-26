#version 430 core

in vec3 vsPos;
in vec3 vsNormal;
in vec2 vsTex;

out vec4 color;

uniform sampler2D wallTexture, floorTexture, skyTexture, modelTex;
uniform int isModel;

uniform vec2 u_resolution; 
uniform int u_gameStatus;   // 0: Green, 1: Red, 2: GameOver, 3: GameWin, 4: Menu
uniform int u_mouseHover;   // 1: 마우스 닿음, 0: 안 닿음

// 2D SDF 기본 함수들
float sdCircle(vec2 p, float r) {
    return length(p) - r;
}

float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdEquilateralTriangle(vec2 p, float r) {
    const float k = sqrt(3.0);
    p.x = abs(p.x) - r;
    p.y = p.y + r/k;
    if( p.x+k*p.y > 0.0 ) p = vec2(p.x-k*p.y,-k*p.x-p.y)/2.0;
    p.x -= clamp( p.x, -2.0*r, 0.0 );
    return -length(p)*sign(p.y);
}

float sdHouse(vec2 p) {
    float roof = sdEquilateralTriangle(p - vec2(0.0, 0.004), 0.012);
    float body = sdBox(p - vec2(0.0, -0.006), vec2(0.009, 0.006));
    float house = min(roof, body);
    float door = sdBox(p - vec2(0.0, -0.009), vec2(0.003, 0.004));
    return max(house, -door);
}

void main() {
    vec3 norm = normalize(vsNormal);

    if(isModel == 0) {
        if(norm.y > 0.4) { // 바닥
            vec2 uv = vsTex; uv.x *= 4.0; uv.y *= 15.0;
            color = texture(floorTexture, uv);
            if(abs(vsPos.z - (-60.0)) < 0.3) color = vec4(0.9, 0.2, 0.4, 1.0); // 결승선
        } else if(norm.y < -0.4) { // 천장
            vec2 uv = vsTex; uv.y *= 4.0;
            color = texture(skyTexture, uv);
        } else { // 벽면
            vec2 uv = vsTex; if(abs(norm.x) > 0.5) uv.x *= 4.0;
            color = texture(wallTexture, uv);
            color.rgb *= (0.7 + 0.3 * abs(norm.z));
        }
    } else {
        vec4 tCol = texture(modelTex, vsTex);
        float d = max(dot(norm, normalize(vec3(0.3, 1.0, 0.5))), 0.5);
        color = vec4(tCol.rgb * d, 1.0);
    }

    vec2 screenUV = gl_FragCoord.xy / u_resolution;

    float vignette = screenUV.x * screenUV.y * (1.0 - screenUV.x) * (1.0 - screenUV.y);
    vignette = clamp(pow(16.0 * vignette, 0.25), 0.0, 1.0);
    color.rgb *= mix(0.95, 1.0, vignette);

    float aspect = u_resolution.x / u_resolution.y;
    vec2 p = screenUV - vec2(0.5);
    p.x *= aspect;

    vec2 lightCenter = vec2(0.0, 0.40); 
    float distToLight = length(p - lightCenter);

    if (u_gameStatus == 4) { // ------------------ [메인 메뉴 화면] ------------------
        color.rgb *= 0.25;

        float d_circle = sdCircle(p - vec2(-0.22, 0.15), 0.08);
        float d_triangle = sdEquilateralTriangle(p - vec2(0.0, 0.15), 0.08);
        float d_square = sdBox(p - vec2(0.22, 0.15), vec2(0.07));

        float thickness = 0.007; 
        float logo_c = smoothstep(0.002, 0.0, abs(d_circle) - thickness);
        float logo_t = smoothstep(0.002, 0.0, abs(d_triangle) - thickness);
        float logo_s = smoothstep(0.002, 0.0, abs(d_square) - thickness);
        float logoMask = max(max(logo_c, logo_t), logo_s);

        color.rgb = mix(color.rgb, vec3(1.0, 0.1, 0.4), logoMask);

        vec2 btnCenter = vec2(0.0, -0.12);
        float d_btn = sdBox(p - btnCenter, vec2(0.15, 0.05));
        float btnOutline = smoothstep(0.002, 0.0, abs(d_btn) - 0.002);
        float btnFill = smoothstep(0.0, -0.002, d_btn);

        if (u_mouseHover == 1) {
            color.rgb = mix(color.rgb, vec3(1.0, 0.1, 0.4), btnFill * 0.35);
            color.rgb = mix(color.rgb, vec3(1.0, 0.25, 0.55), btnOutline);

            vec2 p_play = p - btnCenter;
            vec2 p_play_rot = vec2(-p_play.y, p_play.x); 
            float d_play = sdEquilateralTriangle(p_play_rot, 0.015);
            float playMask = smoothstep(0.0, -0.002, d_play);
            color.rgb = mix(color.rgb, vec3(1.0, 1.0, 1.0), playMask);
        } else {
            color.rgb = mix(color.rgb, vec3(0.08, 0.08, 0.08), btnFill);
            color.rgb = mix(color.rgb, vec3(0.65, 0.08, 0.25), btnOutline);

            vec2 p_play = p - btnCenter;
            vec2 p_play_rot = vec2(-p_play.y, p_play.x);
            float d_play = sdEquilateralTriangle(p_play_rot, 0.015);
            float playMask = smoothstep(0.0, -0.002, d_play);
            color.rgb = mix(color.rgb, vec3(0.65, 0.08, 0.25), playMask);
        }
    } 
    else { // ------------------ [인게임 및 결과 화면] ------------------
        if (u_gameStatus == 0) { // GREEN LIGHT
            if (distToLight < 0.020) color.rgb = vec3(0.0, 1.0, 0.2); 
            else if (distToLight < 0.022) color.rgb = vec3(0.1, 0.1, 0.1); 
        } 
        else if (u_gameStatus == 1) { // RED LIGHT
            if (distToLight < 0.020) color.rgb = vec3(1.0, 0.0, 0.1); 
            else if (distToLight < 0.022) color.rgb = vec3(0.1, 0.1, 0.1);
        }
        else if (u_gameStatus == 2) { // 💀 [GAME OVER 상태] 💀
            float distFromCenter = length(screenUV - vec2(0.5, 0.5));
            float redDangerBorder = smoothstep(0.25, 0.85, distFromCenter); 
            
            vec3 deadBase = mix(color.rgb, vec3(0.25, 0.05, 0.05), 0.25);
            color.rgb = mix(deadBase, vec3(0.70, 0.15, 0.15), redDangerBorder * 0.45);

            vec2 btnCenter = vec2(0.0, -0.12);
            float d_btn = sdBox(p - btnCenter, vec2(0.15, 0.05));
            float btnOutline = smoothstep(0.002, 0.0, abs(d_btn) - 0.002);
            float btnFill = smoothstep(0.0, -0.002, d_btn);

            if (u_mouseHover == 1) {
                color.rgb = mix(color.rgb, vec3(0.9, 0.15, 0.15), btnFill * 0.3);
                color.rgb = mix(color.rgb, vec3(1.0, 0.35, 0.35), btnOutline);

                vec2 p_retry = p - btnCenter;
                float d_arc = abs(length(p_retry) - 0.015) - 0.003;
                if (p_retry.x > 0.0 && p_retry.y > 0.0) d_arc = 1e9; 

                float d_arrow = sdEquilateralTriangle(vec2(p_retry.y - 0.004, p_retry.x - 0.015), 0.005);
                float retryMask = max(smoothstep(0.001, 0.0, d_arc), smoothstep(0.002, 0.0, d_arrow));
                color.rgb = mix(color.rgb, vec3(1.0, 1.0, 1.0), retryMask);
            } 
            else {
                color.rgb = mix(color.rgb, vec3(0.06, 0.04, 0.04), btnFill);
                color.rgb = mix(color.rgb, vec3(0.65, 0.15, 0.20), btnOutline);

                vec2 p_retry = p - btnCenter;
                float d_arc = abs(length(p_retry) - 0.015) - 0.003;
                if (p_retry.x > 0.0 && p_retry.y > 0.0) d_arc = 1e9;

                float d_arrow = sdEquilateralTriangle(vec2(p_retry.y - 0.004, p_retry.x - 0.015), 0.005);
                float retryMask = max(smoothstep(0.001, 0.0, d_arc), smoothstep(0.002, 0.0, d_arrow));
                color.rgb = mix(color.rgb, vec3(0.65, 0.15, 0.20), retryMask);
            }
        }
        else if (u_gameStatus == 3) { // 🎉 [GAME WIN 상태] 🎉
            color.rgb = mix(color.rgb, vec3(1.0, 0.88, 0.60), 0.20);

            float d_circle = sdCircle(p - vec2(-0.22, 0.15), 0.08);
            float d_triangle = sdEquilateralTriangle(p - vec2(0.0, 0.15), 0.08);
            float d_square = sdBox(p - vec2(0.22, 0.15), vec2(0.07));

            float thickness = 0.007; 
            float logo_c = smoothstep(0.002, 0.0, abs(d_circle) - thickness);
            float logo_t = smoothstep(0.002, 0.0, abs(d_triangle) - thickness);
            float logo_s = smoothstep(0.002, 0.0, abs(d_square) - thickness);
            float logoMask = max(max(logo_c, logo_t), logo_s);

            color.rgb = mix(color.rgb, vec3(0.0, 1.0, 0.2), logoMask);

            vec2 btnCenter = vec2(0.0, -0.12);
            float d_btn = sdBox(p - btnCenter, vec2(0.15, 0.05));
            float btnOutline = smoothstep(0.002, 0.0, abs(d_btn) - 0.002);
            float btnFill = smoothstep(0.0, -0.002, d_btn);

            if (u_mouseHover == 1) {
                color.rgb = mix(color.rgb, vec3(0.15, 0.80, 0.30), btnFill * 0.3);
                color.rgb = mix(color.rgb, vec3(0.35, 1.00, 0.50), btnOutline);

                vec2 p_home = p - btnCenter;
                float d_house = sdHouse(p_home);
                float homeMask = smoothstep(0.0, -0.002, d_house);
                color.rgb = mix(color.rgb, vec3(1.0, 1.0, 1.0), homeMask);
            } 
            else {
                color.rgb = mix(color.rgb, vec3(0.04, 0.06, 0.04), btnFill);
                color.rgb = mix(color.rgb, vec3(0.15, 0.65, 0.20), btnOutline);

                vec2 p_home = p - btnCenter;
                float d_house = sdHouse(p_home);
                float homeMask = smoothstep(0.0, -0.002, d_house);
                color.rgb = mix(color.rgb, vec3(0.15, 0.65, 0.20), homeMask);
            }
        }
    }
}