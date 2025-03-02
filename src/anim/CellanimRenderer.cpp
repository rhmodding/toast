#include "CellanimRenderer.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

#include <array>

#include <algorithm>

#include "../common.hpp"

#if defined(IMGUI_IMPL_OPENGL_ES2)
    #define GLSL_VERSION_STR "#version 100"
#elif defined(__APPLE__)
    #define GLSL_VERSION_STR "#version 150"
#else
    #define GLSL_VERSION_STR "#version 130"
#endif // defined(__APPLE__), defined(IMGUI_IMPL_OPENGL_ES2)

static const char* vertexShaderSource =
    GLSL_VERSION_STR "\n\n"
    "uniform mat4 ProjMtx;\n"
    "\n"
    "in vec2 Position;\n"
    "in vec2 UV;\n"
    "in vec4 Color;\n"
    "\n"
    "out vec2 Frag_UV;\n"
    "out vec4 Frag_Color;\n"
    "\n"
    "void main() {\n"
    "    Frag_UV = UV;\n"
    "    Frag_Color = Color;\n"
    "    gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
    "}\n";

static const char* fragmentShaderSource =
    GLSL_VERSION_STR "\n\n"
    "uniform sampler2D Texture;\n"
    "uniform vec3 Fore_Color;\n"
    "uniform vec3 Back_Color;\n"
    "\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "\n"
    "out vec4 Out_Color;\n"
    "\n"
    "void main() {\n"
    "    vec4 texColor = texture(Texture, Frag_UV.st);\n"
    "\n"
    "    vec3 multipliedColor = vec3(texColor) * Fore_Color;\n"
    "    vec3 finalColor = vec3(1.0) - (vec3(1.0) - multipliedColor) * (vec3(1.0) - Back_Color);\n"
    "\n"
    "    Out_Color = vec4(finalColor, texColor.a) * Frag_Color;\n"
    "}\n";

GLuint CellanimRenderer::shaderProgram { 0 };

GLint CellanimRenderer::foreColorUniform { 0 };
GLint CellanimRenderer::backColorUniform { 0 };
GLint CellanimRenderer::projMtxUniform { 0 };

static void checkShaderError(GLuint shader, GLenum flag, bool isProgram, const std::string& errorMessage) {
    GLint success { 0 };
    GLchar error[1024] { '\0' };

    if (isProgram) {
        glGetProgramiv(shader, flag, &success);
        if (success == GL_FALSE) {
            glGetProgramInfoLog(shader, sizeof(error), NULL, error);
            std::cerr << errorMessage << ":\n" << error << '\n';
        }
    }
    else {
        glGetShaderiv(shader, flag, &success);
        if (success == GL_FALSE) {
            glGetShaderInfoLog(shader, sizeof(error), NULL, error);
            std::cerr << errorMessage << ": " << error << '\n';
        }
    }
}

void CellanimRenderer::InitShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    checkShaderError(
        vertexShader, GL_COMPILE_STATUS, false,
        "[CellanimRenderer::InitShader] Vertex shader compilation failed"
    );

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    checkShaderError(
        fragmentShader, GL_COMPILE_STATUS, false,
        "[CellanimRenderer::InitShader] Fragment shader compilation failed"
    );

    CellanimRenderer::shaderProgram = glCreateProgram();
    glAttachShader(CellanimRenderer::shaderProgram, vertexShader);
    glAttachShader(CellanimRenderer::shaderProgram, fragmentShader);
    glLinkProgram(CellanimRenderer::shaderProgram);

    checkShaderError(
        CellanimRenderer::shaderProgram, GL_LINK_STATUS, true,
        "[CellanimRenderer::InitShader] Shader program link failed"
    );

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Looking up the uniform locations in advance avoids stuttering issues.

    CellanimRenderer::foreColorUniform = glGetUniformLocation(
        CellanimRenderer::shaderProgram, "Fore_Color"
    );
    CellanimRenderer::backColorUniform = glGetUniformLocation(
        CellanimRenderer::shaderProgram, "Back_Color"
    );

    CellanimRenderer::projMtxUniform = glGetUniformLocation(
        CellanimRenderer::shaderProgram, "ProjMtx"
    );
}

void CellanimRenderer::DestroyShader() {
    glDeleteProgram(CellanimRenderer::shaderProgram);
}

struct PartRenderCallbackData {
    CellAnim::CTRColor backColor;
    CellAnim::CTRColor foreColor;
};

void CellanimRenderer::renderPartCallback(const ImDrawList* parentList, const ImDrawCmd* cmd) {
    const PartRenderCallbackData* renderData =
        reinterpret_cast<const PartRenderCallbackData*>(cmd->UserCallbackData);

    const auto& foreColor = renderData->foreColor;
    const auto& backColor = renderData->backColor;

    glUseProgram(CellanimRenderer::shaderProgram);

    glUniform3f(
        CellanimRenderer::foreColorUniform,
        foreColor.r, foreColor.g, foreColor.b
    );
    glUniform3f(
        CellanimRenderer::backColorUniform,
        backColor.r, backColor.g, backColor.b
    );

    const ImDrawData* drawData = ImGui::GetDrawData();

    float L = drawData->DisplayPos.x;
    float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float T = drawData->DisplayPos.y;
    float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

    const float orthoProjection[4][4] = {
        { 2.f / (R-L),   0.f,           0.f,  0.f },
        { 0.f,           2.f / (T-B),   0.f,  0.f },
        { 0.f,           0.f,          -1.f,  0.f },
        { (R+L) / (L-R), (T+B) / (B-T), 0.f,  1.f },
    };
    glUniformMatrix4fv(CellanimRenderer::projMtxUniform, 1, GL_FALSE, orthoProjection[0]);
}

void CellanimRenderer::Draw(ImDrawList* drawList, const CellAnim::Animation& animation, unsigned keyIndex, bool allowOpacity) {
    NONFATAL_ASSERT_RET(this->cellanim, true);
    NONFATAL_ASSERT_RET(this->textureGroup, true);

    if (!this->visible)
        return;

    this->InternDraw(drawList, animation.keys.at(keyIndex), -1, 0xFFFFFFFFu, allowOpacity);
}

void CellanimRenderer::DrawOnionSkin(ImDrawList* drawList, const CellAnim::Animation& animation, unsigned keyIndex, unsigned backCount, unsigned frontCount, bool rollOver, uint8_t opacity) {
    NONFATAL_ASSERT_RET(this->cellanim, true);
    NONFATAL_ASSERT_RET(this->textureGroup, true);

    if (!this->visible)
        return;

    const unsigned keyCount = animation.keys.size();

    auto _drawOnionSkin = [&](int startIndex, int endIndex, int step, uint32_t color) {
        int i = startIndex;
        while ((step < 0) ? (i >= endIndex) : (i <= endIndex)) {
            int wrappedIndex = i;

            if (rollOver) {
                // Roll backwards
                if (wrappedIndex < 0)
                    wrappedIndex = (keyCount + (wrappedIndex % keyCount)) % keyCount;
                // Roll forwards
                else if (wrappedIndex >= static_cast<int>(keyCount))
                    wrappedIndex = wrappedIndex % keyCount;
            }

            // Break out if rollover is disabled
            if (!rollOver && (wrappedIndex < 0 || wrappedIndex >= static_cast<int>(keyCount)))
                break;

            this->InternDraw(drawList, animation.keys.at(wrappedIndex), -1, color, true);
            i += step;
        }
    };

    if (backCount > 0)
        _drawOnionSkin(keyIndex - 1, keyIndex - backCount, -1, IM_COL32(255, 64, 64, opacity));

    if (frontCount > 0)
        _drawOnionSkin(keyIndex + 1, keyIndex + frontCount, 1, IM_COL32(64, 255, 64, opacity));
}

// Note: 'angle' is in degrees.
static ImVec2 rotateVec2(const ImVec2& v, float angle, const ImVec2& origin) {
    const float s = sinf(angle * ((float)M_PI / 180.f));
    const float c = cosf(angle * ((float)M_PI / 180.f));

    float vx = v.x - origin.x;
    float vy = v.y - origin.y;

    float x = vx * c - vy * s;
    float y = vx * s + vy * c;

    return { x + origin.x, y + origin.y };
}

std::array<ImVec2, 4> CellanimRenderer::getPartWorldQuad(const CellAnim::AnimationKey& key, unsigned partIndex) const {
    std::array<ImVec2, 4> transformedQuad;

    NONFATAL_ASSERT_RETVAL(this->cellanim, transformedQuad);

    const CellAnim::Arrangement& arrangement = this->cellanim->arrangements.at(key.arrangementIndex);
    const CellAnim::ArrangementPart& part = arrangement.parts.at(partIndex);

    const int*   tempOffset = arrangement.tempOffset;
    const float* tempScale  = arrangement.tempScale;

    ImVec2 keyCenter {
        this->offset.x,
        this->offset.y
    };

    ImVec2 topLeftOffset {
        static_cast<float>(part.transform.positionX),
        static_cast<float>(part.transform.positionY)
    };

    ImVec2 bottomRightOffset {
        (topLeftOffset.x + (part.regionW * part.transform.scaleX)),
        (topLeftOffset.y + (part.regionH * part.transform.scaleY))
    };

    topLeftOffset = {
        (topLeftOffset.x * tempScale[0]) + tempOffset[0],
        (topLeftOffset.y * tempScale[1]) + tempOffset[1]
    };
    bottomRightOffset = {
        (bottomRightOffset.x * tempScale[0]) + tempOffset[0],
        (bottomRightOffset.y * tempScale[1]) + tempOffset[1]
    };

    transformedQuad = {
        topLeftOffset,
        { bottomRightOffset.x, topLeftOffset.y },
        bottomRightOffset,
        { topLeftOffset.x, bottomRightOffset.y },
    };

    const ImVec2 center = AVERAGE_IMVEC2(topLeftOffset, bottomRightOffset);

    // Transformations
    {
        // Rotation
        float rotAngle = part.transform.angle;

        if ((tempScale[0] < 0.f) ^ (tempScale[1] < 0.f))
            rotAngle = -rotAngle;

        for (auto& point : transformedQuad)
            point = rotateVec2(point, rotAngle, center);

        // Key & renderer scale
        for (auto& point : transformedQuad) {
            point.x = (point.x * key.transform.scaleX * this->scaleX) + keyCenter.x;
            point.y = (point.y * key.transform.scaleY * this->scaleY) + keyCenter.y;
        }

        // Key rotation
        for (auto& point : transformedQuad)
            point = rotateVec2(point, key.transform.angle, keyCenter);

        // Key offset addition
        for (auto& point : transformedQuad) {
            point.x += key.transform.positionX * this->scaleX;
            point.y += key.transform.positionY * this->scaleY;
        }
    }

    return transformedQuad;
}

std::array<ImVec2, 4> CellanimRenderer::getPartWorldQuad(const CellAnim::TransformValues& keyTransform, const CellAnim::Arrangement& arrangement, unsigned partIndex) const {
    std::array<ImVec2, 4> transformedQuad;

    NONFATAL_ASSERT_RETVAL(this->cellanim, transformedQuad);

    const CellAnim::ArrangementPart& part = arrangement.parts.at(partIndex);

    const int*   tempOffset = arrangement.tempOffset;
    const float* tempScale  = arrangement.tempScale;

    ImVec2 keyCenter {
        this->offset.x,
        this->offset.y
    };

    ImVec2 topLeftOffset {
        static_cast<float>(part.transform.positionX),
        static_cast<float>(part.transform.positionY)
    };

    ImVec2 bottomRightOffset {
        (topLeftOffset.x + (part.regionW * part.transform.scaleX)),
        (topLeftOffset.y + (part.regionH * part.transform.scaleY))
    };

    topLeftOffset = {
        (topLeftOffset.x * tempScale[0]) + tempOffset[0],
        (topLeftOffset.y * tempScale[1]) + tempOffset[1]
    };
    bottomRightOffset = {
        (bottomRightOffset.x * tempScale[0]) + tempOffset[0],
        (bottomRightOffset.y * tempScale[1]) + tempOffset[1]
    };

    transformedQuad = {
        topLeftOffset,
        { bottomRightOffset.x, topLeftOffset.y },
        bottomRightOffset,
        { topLeftOffset.x, bottomRightOffset.y },
    };

    const ImVec2 center = AVERAGE_IMVEC2(topLeftOffset, bottomRightOffset);

    // Transformations
    {
        // Rotation
        float rotAngle = part.transform.angle;

        if ((tempScale[0] < 0.f) ^ (tempScale[1] < 0.f))
            rotAngle = -rotAngle;

        for (auto& point : transformedQuad)
            point = rotateVec2(point, rotAngle, center);

        // Key & renderer scale
        for (auto& point : transformedQuad) {
            point.x = (point.x * keyTransform.scaleX * this->scaleX) + keyCenter.x;
            point.y = (point.y * keyTransform.scaleY * this->scaleY) + keyCenter.y;
        }

        // Key rotation
        for (auto& point : transformedQuad)
            point = rotateVec2(point, keyTransform.angle, keyCenter);

        // Key offset addition
        for (auto& point : transformedQuad) {
            point.x += keyTransform.positionX * this->scaleX;
            point.y += keyTransform.positionY * this->scaleY;
        }
    }

    return transformedQuad;
}

ImRect CellanimRenderer::getKeyWorldRect(const CellAnim::AnimationKey& key) const {
    NONFATAL_ASSERT_RETVAL(this->cellanim, ImRect());

    const auto& arrangement = this->cellanim->arrangements.at(key.arrangementIndex);

    std::vector<std::array<ImVec2, 4>> quads(arrangement.parts.size());
    for (unsigned i = 0; i < arrangement.parts.size(); i++)
        quads[i] = this->getPartWorldQuad(key, i);

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (unsigned i = 0; i < arrangement.parts.size(); ++i) {
        for (const ImVec2& vertex : quads[i]) {
            if (vertex.x < minX) minX = vertex.x;
            if (vertex.y < minY) minY = vertex.y;
            if (vertex.x > maxX) maxX = vertex.x;
            if (vertex.y > maxY) maxY = vertex.y;
        }
    }

    return ImRect({ minX, minY }, { maxX, maxY });
}

void CellanimRenderer::InternDraw(
    ImDrawList* drawList,
    const CellAnim::AnimationKey& key, int partIndex,
    uint32_t colorMod,
    bool allowOpacity
) {
    NONFATAL_ASSERT_RET(this->cellanim, true);

    const CellAnim::Arrangement& arrangement = this->cellanim->arrangements.at(key.arrangementIndex);

    const float texWidth = this->cellanim->sheetW;
    const float texHeight = this->cellanim->sheetH;

    for (unsigned i = 0; i < arrangement.parts.size(); i++) {
        if (partIndex != -1 && partIndex != static_cast<int>(i))
            continue;
        
        const CellAnim::ArrangementPart& part = arrangement.parts.at(i);

        // Skip invisible parts
        if (((part.opacity == 0) && allowOpacity) || !part.editorVisible)
            continue;

        std::array<ImVec2, 4> quad = this->getPartWorldQuad(key, i);

        ImVec2 uvTopLeft = {
            part.regionX / texWidth,
            part.regionY / texHeight
        };
        ImVec2 uvBottomRight = {
            uvTopLeft.x + (part.regionW / texWidth),
            uvTopLeft.y + (part.regionH / texHeight)
        };

        std::array<ImVec2, 4> uvs = std::array<ImVec2, 4>({
            uvTopLeft,
            { uvBottomRight.x, uvTopLeft.y },
            uvBottomRight,
            { uvTopLeft.x, uvBottomRight.y }
        });

        if (part.flipX) {
            std::swap(uvs[0], uvs[1]);
            std::swap(uvs[2], uvs[3]);
        }
        if (part.flipY) {
            std::swap(uvs[0], uvs[3]);
            std::swap(uvs[1], uvs[2]);
        }

        const auto& texture = this->textureGroup->getTextureByVarying(part.textureVarying);

        unsigned baseAlpha = allowOpacity ?
            uint8_t((unsigned(part.opacity) * unsigned(key.opacity)) / 0xFFu) :
            0xFFu;
        
        unsigned vertexAlpha = (baseAlpha * ((colorMod >> 24) & 0xFF)) / 0xFF;
        uint32_t vertexColor = (colorMod & 0x00FFFFFF) | (vertexAlpha << 24);

        PartRenderCallbackData callbackData;

        callbackData.backColor = part.backColor;
        callbackData.foreColor = part.foreColor;
        
        drawList->AddCallback(CellanimRenderer::renderPartCallback, &callbackData, sizeof(callbackData));

        drawList->AddImageQuad(
            (ImTextureID)texture->getTextureId(),
            quad[0], quad[1], quad[2], quad[3],
            uvs[0], uvs[1], uvs[2], uvs[3],
            vertexColor
        );
    }

    drawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr, 0);
}
