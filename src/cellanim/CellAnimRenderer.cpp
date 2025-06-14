#include "CellAnimRenderer.hpp"

#include <cstddef>

#include <cmath>

#include <array>

#include <algorithm>

#include <string_view>

#include "glInclude.hpp"

#include "Logging.hpp"

#include "Macro.hpp"

#if defined(IMGUI_IMPL_OPENGL_ES2)
    #define GLSL_VERSION_STR "#version 100"
#elif defined(__APPLE__)
    #define GLSL_VERSION_STR "#version 150"
#else
    #define GLSL_VERSION_STR "#version 130"
#endif // defined(__APPLE__), defined(IMGUI_IMPL_OPENGL_ES2)

/*
    We use a custom shader for drawing parts since we need to pass in
    fore & back colors that need to get applied when rendering.
*/

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
    "\n"
    "uniform vec3 Fore_Color_A;\n" // For the part.
    "uniform vec3 Back_Color_A;\n"
    "\n"
    "uniform vec3 Fore_Color_B;\n" // For the key.
    "uniform vec3 Back_Color_B;\n"
    "\n"
    "in vec2 Frag_UV;\n"
    "in vec4 Frag_Color;\n"
    "\n"
    "out vec4 Out_Color;\n"
    "\n"
    "void main() {\n"
    "    vec4 texColor = texture(Texture, Frag_UV.st);\n"
    "\n"
    "    vec3 multipliedColorA = texColor.xyz * Fore_Color_A;\n"
    "    vec3 passA = vec3(1.0) - (vec3(1.0) - multipliedColorA) * (vec3(1.0) - Back_Color_A);\n"
    "\n"
    "    vec3 multipliedColorB = passA * Fore_Color_B;\n"
    "    vec3 passB = vec3(1.0) - (vec3(1.0) - multipliedColorB) * (vec3(1.0) - Back_Color_B);\n"
    "\n"
    "    Out_Color = vec4(passB, texColor.a) * Frag_Color;\n"
    "}\n";

GLuint CellAnimRenderer::sShaderProgram { 0 };

GLint CellAnimRenderer::sTextureUniform { -1 };
GLint CellAnimRenderer::sForeColorAUniform { -1 };
GLint CellAnimRenderer::sBackColorAUniform { -1 };
GLint CellAnimRenderer::sForeColorBUniform { -1 };
GLint CellAnimRenderer::sBackColorBUniform { -1 };
GLint CellAnimRenderer::sProjMtxUniform { -1 };

GLint CellAnimRenderer::sPositionAttrib { -1 };
GLint CellAnimRenderer::sUvAttrib { -1 };
GLint CellAnimRenderer::sColorAttrib { -1 };

GLuint CellAnimRenderer::sTexDrawFramebuffer { 0 };

static void checkShaderError(GLuint shader, GLenum flag, bool isProgram, std::string_view errorMessage) {
    GLint success { 0 };
    GLchar error[1024] { '\0' };

    if (isProgram) {
        glGetProgramiv(shader, flag, &success);
        if (success == GL_FALSE) {
            glGetProgramInfoLog(shader, sizeof(error), NULL, error);
            Logging::err << errorMessage << ":\n" << error << std::endl;
        }
    }
    else {
        glGetShaderiv(shader, flag, &success);
        if (success == GL_FALSE) {
            glGetShaderInfoLog(shader, sizeof(error), NULL, error);
            Logging::err << errorMessage << ":\n" << error << std::endl;
        }
    }
}

void CellAnimRenderer::InitShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    checkShaderError(
        vertexShader, GL_COMPILE_STATUS, false,
        "[CellAnimRenderer::InitShader] Vertex shader compilation failed"
    );

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    checkShaderError(
        fragmentShader, GL_COMPILE_STATUS, false,
        "[CellAnimRenderer::InitShader] Fragment shader compilation failed"
    );

    sShaderProgram = glCreateProgram();
    glAttachShader(sShaderProgram, vertexShader);
    glAttachShader(sShaderProgram, fragmentShader);
    glLinkProgram(sShaderProgram);

    checkShaderError(
        sShaderProgram, GL_LINK_STATUS, true,
        "[CellAnimRenderer::InitShader] Shader program link failed"
    );

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Looking up the uniform & attrib locations in advance avoids stuttering issues

    sTextureUniform = glGetUniformLocation(sShaderProgram, "Texture");

    sForeColorAUniform = glGetUniformLocation(sShaderProgram, "Fore_Color_A");
    sBackColorAUniform = glGetUniformLocation(sShaderProgram, "Back_Color_A");
    sForeColorBUniform = glGetUniformLocation(sShaderProgram, "Fore_Color_B");
    sBackColorBUniform = glGetUniformLocation(sShaderProgram, "Back_Color_B");

    sProjMtxUniform = glGetUniformLocation(sShaderProgram, "ProjMtx");

    sPositionAttrib = glGetAttribLocation(sShaderProgram, "Position");
    sUvAttrib = glGetAttribLocation(sShaderProgram, "UV");
    sColorAttrib = glGetAttribLocation(sShaderProgram, "Color");

    glGenFramebuffers(1, &sTexDrawFramebuffer);

    Logging::info << "[CellAnimRenderer::InitShader] Successfully initialized shader." << std::endl;
}

void CellAnimRenderer::DestroyShader() {
    Logging::info << "[CellAnimRenderer::DestroyShader] Destroying shader.." << std::endl;

    glDeleteProgram(sShaderProgram);

    glDeleteFramebuffers(1, &sTexDrawFramebuffer);
}

struct RenderPartCallbackData {
    CellAnim::CTRColor backColorA, backColorB;
    CellAnim::CTRColor foreColorA, foreColorB;
};

void CellAnimRenderer::renderPartCallback(const ImDrawList* parentList, const ImDrawCmd* cmd) {
    const RenderPartCallbackData* renderData =
        reinterpret_cast<const RenderPartCallbackData*>(cmd->UserCallbackData);

    const auto& foreColorA = renderData->foreColorA;
    const auto& backColorA = renderData->backColorA;

    const auto& foreColorB = renderData->foreColorB;
    const auto& backColorB = renderData->backColorB;

    glUseProgram(sShaderProgram);

    glUniform3f(sForeColorAUniform, foreColorA.r, foreColorA.g, foreColorA.b);
    glUniform3f(sBackColorAUniform, backColorA.r, backColorA.g, backColorA.b);

    glUniform3f(sForeColorBUniform, foreColorB.r, foreColorB.g, foreColorB.b);
    glUniform3f(sBackColorBUniform, backColorB.r, backColorB.g, backColorB.b);

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
    glUniformMatrix4fv(sProjMtxUniform, 1, GL_FALSE, orthoProjection[0]);
}

void CellAnimRenderer::Draw(ImDrawList* drawList, const CellAnim::Animation& animation, unsigned keyIndex, bool allowOpacity) {
    NONFATAL_ASSERT_RET(mCellAnim, true);
    NONFATAL_ASSERT_RET(mTextureGroup, true);

    if (!mVisible)
        return;

    currentDrawList = drawList;
    InternDraw(DrawMethod::DrawList, animation.keys.at(keyIndex), -1, IM_COL32_WHITE, allowOpacity);
}

void CellAnimRenderer::DrawTex(GLuint textureId, const CellAnim::Animation& animation, unsigned keyIndex, bool allowOpacity) {
    NONFATAL_ASSERT_RET(mCellAnim, true);
    NONFATAL_ASSERT_RET(mTextureGroup, true);

    if (!mVisible)
        return;

    currentDrawTex = textureId;
    InternDraw(DrawMethod::Texture, animation.keys.at(keyIndex), -1, IM_COL32_WHITE, allowOpacity);
}

void CellAnimRenderer::DrawOnionSkin(
    ImDrawList* drawList,
    const CellAnim::Animation& animation, unsigned keyIndex,
    unsigned backCount, unsigned frontCount,
    bool rollOver,
    uint8_t opacity
) {
    NONFATAL_ASSERT_RET(mCellAnim, true);
    NONFATAL_ASSERT_RET(mTextureGroup, true);

    if (!mVisible)
        return;

    const unsigned keyCount = animation.keys.size();

    auto _drawOnionSkin = [this, rollOver, keyCount, drawList, animation](int startIndex, int endIndex, int step, uint32_t color) {
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

            if (!rollOver && (wrappedIndex < 0 || wrappedIndex >= static_cast<int>(keyCount)))
                break;

            currentDrawList = drawList;
            InternDraw(DrawMethod::DrawList, animation.keys.at(wrappedIndex), -1, color, true);
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

std::array<ImVec2, 4> CellAnimRenderer::getPartWorldQuad(const CellAnim::AnimationKey& key, unsigned partIndex) const {
    std::array<ImVec2, 4> transformedQuad;

    NONFATAL_ASSERT_RETVAL(mCellAnim, transformedQuad);

    const CellAnim::Arrangement& arrangement = mCellAnim->getArrangements().at(key.arrangementIndex);
    const CellAnim::ArrangementPart& part = arrangement.parts.at(partIndex);

    const auto& tempOffset = arrangement.tempOffset;
    const auto& tempScale  = arrangement.tempScale;

    ImVec2 keyCenter = mOffset;

    ImVec2 topLeftOffset {
        static_cast<float>(part.transform.position.x),
        static_cast<float>(part.transform.position.y)
    };

    ImVec2 bottomRightOffset {
        (topLeftOffset.x + (part.regionSize.x * part.transform.scale.x)),
        (topLeftOffset.y + (part.regionSize.y * part.transform.scale.y))
    };

    topLeftOffset = {
        (topLeftOffset.x * tempScale.x) + tempOffset.x,
        (topLeftOffset.y * tempScale.y) + tempOffset.y
    };
    bottomRightOffset = {
        (bottomRightOffset.x * tempScale.x) + tempOffset.x,
        (bottomRightOffset.y * tempScale.y) + tempOffset.y
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

        if ((tempScale.x < 0.f) ^ (tempScale.y < 0.f))
            rotAngle = -rotAngle;

        for (auto& point : transformedQuad)
            point = rotateVec2(point, rotAngle, center);

        // Key & renderer scale
        for (auto& point : transformedQuad) {
            point.x = (point.x * key.transform.scale.x * mScale.x) + keyCenter.x;
            point.y = (point.y * key.transform.scale.y * mScale.y) + keyCenter.y;
        }

        // Key rotation
        for (auto& point : transformedQuad)
            point = rotateVec2(point, key.transform.angle, keyCenter);

        // Key offset addition
        for (auto& point : transformedQuad) {
            point.x += key.transform.position.x * mScale.x;
            point.y += key.transform.position.y * mScale.y;
        }
    }

    return transformedQuad;
}

std::array<ImVec2, 4> CellAnimRenderer::getPartWorldQuad(const CellAnim::TransformValues& keyTransform, const CellAnim::Arrangement& arrangement, unsigned partIndex) const {
    std::array<ImVec2, 4> transformedQuad;

    NONFATAL_ASSERT_RETVAL(mCellAnim, transformedQuad);

    const CellAnim::ArrangementPart& part = arrangement.parts.at(partIndex);

    const auto& tempOffset = arrangement.tempOffset;
    const auto& tempScale  = arrangement.tempScale;

    ImVec2 keyCenter = mOffset;

    ImVec2 topLeftOffset {
        static_cast<float>(part.transform.position.x),
        static_cast<float>(part.transform.position.y)
    };

    ImVec2 bottomRightOffset {
        (topLeftOffset.x + (part.regionSize.x * part.transform.scale.x)),
        (topLeftOffset.y + (part.regionSize.y * part.transform.scale.y))
    };

    topLeftOffset = {
        (topLeftOffset.x * tempScale.x) + tempOffset.x,
        (topLeftOffset.y * tempScale.y) + tempOffset.y
    };
    bottomRightOffset = {
        (bottomRightOffset.x * tempScale.x) + tempOffset.x,
        (bottomRightOffset.y * tempScale.y) + tempOffset.y
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

        if ((tempScale.x < 0.f) ^ (tempScale.y < 0.f))
            rotAngle = -rotAngle;

        for (auto& point : transformedQuad)
            point = rotateVec2(point, rotAngle, center);

        // Key & renderer scale
        for (auto& point : transformedQuad) {
            point.x = (point.x * keyTransform.scale.x * mScale.x) + keyCenter.x;
            point.y = (point.y * keyTransform.scale.y * mScale.y) + keyCenter.y;
        }

        // Key rotation
        for (auto& point : transformedQuad)
            point = rotateVec2(point, keyTransform.angle, keyCenter);

        // Key offset addition
        for (auto& point : transformedQuad) {
            point.x += keyTransform.position.x * mScale.x;
            point.y += keyTransform.position.y * mScale.y;
        }
    }

    return transformedQuad;
}

ImRect CellAnimRenderer::getKeyWorldRect(const CellAnim::AnimationKey& key) const {
    NONFATAL_ASSERT_RETVAL(mCellAnim, ImRect());

    const auto& arrangement = mCellAnim->getArrangement(key.arrangementIndex);

    std::vector<std::array<ImVec2, 4>> quads(arrangement.parts.size());
    for (size_t i = 0; i < arrangement.parts.size(); i++)
        quads[i] = getPartWorldQuad(key, i);

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (size_t i = 0; i < arrangement.parts.size(); ++i) {
        for (const ImVec2& vertex : quads[i]) {
            if (vertex.x < minX) minX = vertex.x;
            if (vertex.y < minY) minY = vertex.y;
            if (vertex.x > maxX) maxX = vertex.x;
            if (vertex.y > maxY) maxY = vertex.y;
        }
    }

    return ImRect({ minX, minY }, { maxX, maxY });
}

struct PartDrawCommand {
    std::array<ImVec2, 4> quad;
    std::array<ImVec2, 4> uvs;
    GLuint textureId;
    uint32_t vertexColor;
    RenderPartCallbackData callbackData;
};

template<typename T>
static std::vector<PartDrawCommand> constructDrawData(
    const CellAnimRenderer& renderer,
    const CellAnim::CellAnimObject& cellanim, TextureGroup<T>& textureGroup,
    const CellAnim::AnimationKey& key, int partIndex,
    uint32_t colorMod, bool allowOpacity
) {
    const CellAnim::Arrangement& arrangement = cellanim.getArrangement(key.arrangementIndex);

    std::vector<PartDrawCommand> drawData;
    drawData.reserve(arrangement.parts.size());

    const float texWidth = cellanim.getSheetWidth();
    const float texHeight = cellanim.getSheetHeight();

    for (size_t i = 0; i < arrangement.parts.size(); i++) {
        if (partIndex >= 0 && partIndex != static_cast<int>(i))
            continue;

        const CellAnim::ArrangementPart& part = arrangement.parts.at(i);

        // Skip invisible parts
        if (((part.opacity == 0) && allowOpacity) || !part.editorVisible)
            continue;

        PartDrawCommand command {};

        command.quad = renderer.getPartWorldQuad(key, i);

        ImVec2 uvTopLeft = {
            part.regionPos.x / texWidth,
            part.regionPos.y / texHeight
        };
        ImVec2 uvBottomRight = {
            uvTopLeft.x + (part.regionSize.x / texWidth),
            uvTopLeft.y + (part.regionSize.y / texHeight)
        };

        command.uvs = std::array<ImVec2, 4>({
            uvTopLeft,
            { uvBottomRight.x, uvTopLeft.y },
            uvBottomRight,
            { uvTopLeft.x, uvBottomRight.y }
        });

        if (part.flipX) {
            std::swap(command.uvs[0], command.uvs[1]);
            std::swap(command.uvs[2], command.uvs[3]);
        }
        if (part.flipY) {
            std::swap(command.uvs[0], command.uvs[3]);
            std::swap(command.uvs[1], command.uvs[2]);
        }

        const auto& texture = textureGroup.getTextureByVarying(part.textureVarying);

        command.textureId = texture->getTextureId();

        unsigned baseAlpha = allowOpacity ?
            ((unsigned(part.opacity) * unsigned(key.opacity)) / 0xFF) :
            0xFF;

        unsigned vertexAlpha = (baseAlpha * ((colorMod >> 24) & 0xFF)) / 0xFF;
        command.vertexColor = (colorMod & 0x00FFFFFF) | (vertexAlpha << 24);

        command.callbackData = RenderPartCallbackData {
            .backColorA = part.backColor,
            .backColorB = key.backColor,
            .foreColorA = part.foreColor,
            .foreColorB = key.foreColor
        };

        drawData.push_back(std::move(command));
    }

    return drawData;
}

static inline std::array<ImVec2, 6> quadToTriangles(const std::array<ImVec2, 4>& quad) {
    return std::array<ImVec2, 6>({
        quad[0], quad[1], quad[3],
        quad[1], quad[2], quad[3]
    });
}

void CellAnimRenderer::InternDraw(
    DrawMethod drawMethod,
    const CellAnim::AnimationKey& key, int partIndex,
    uint32_t colorMod,
    bool allowOpacity
) {
    NONFATAL_ASSERT_RET(mCellAnim, true);

    const ImVec2 prevOffset = mOffset;
    const ImVec2 prevScale = mScale;

    if (drawMethod == DrawMethod::Texture) {
        mOffset = ImVec2(0.f, 0.f);
        mScale = ImVec2(1.f, 1.f);
    }

    std::vector<PartDrawCommand> drawData = constructDrawData(
        *this, *mCellAnim, *mTextureGroup,
        key, partIndex, colorMod, allowOpacity
    );

    switch (drawMethod) {
    case DrawMethod::DrawList: {
        for (auto& cmd : drawData) {
            // ImGui will copy the userdata.
            currentDrawList->AddCallback(CellAnimRenderer::renderPartCallback, &cmd.callbackData, sizeof(cmd.callbackData));

            currentDrawList->AddImageQuad(
                static_cast<ImTextureID>(cmd.textureId),
                cmd.quad[0], cmd.quad[1], cmd.quad[2], cmd.quad[3],
                cmd.uvs[0], cmd.uvs[1], cmd.uvs[2], cmd.uvs[3],
                cmd.vertexColor
            );
        }

        currentDrawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr, 0);
    } break;

    case DrawMethod::Texture: {
        constexpr float SCALE_FACTOR = 2.f;

        const ImRect frameRect = getKeyWorldRect(key);

        mDrawTexSize = ImVec2(
            CEIL_FLOAT(frameRect.Max.x - frameRect.Min.x) * SCALE_FACTOR,
            CEIL_FLOAT(frameRect.Max.y - frameRect.Min.y) * SCALE_FACTOR
        );
        mDrawTexOffset = ImVec2(
            (frameRect.Min.x + frameRect.Max.x) * .5f * SCALE_FACTOR,
            (frameRect.Min.y + frameRect.Max.y) * .5f * SCALE_FACTOR
        );

        unsigned vertexCount = drawData.size() * 6;

        struct Vertex { ImVec2 pos, uv; ImVec4 color; };
        std::vector<Vertex> vertexData (vertexCount);

        for (size_t i = 0; i < drawData.size(); i++) {
            auto tris = quadToTriangles(drawData[i].quad);
            auto uvTris = quadToTriangles(drawData[i].uvs);
            for (unsigned j = 0; j < 6; j++) {
                vertexData[(i * 6) + j] = Vertex {
                    .pos = tris[j],
                    .uv = uvTris[j],
                    .color = ImGui::ColorConvertU32ToFloat4(drawData[i].vertexColor)
                };
            }
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        GLuint VBO, VAO;

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertexData.data(), GL_STATIC_DRAW);

        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glVertexAttribPointer(sPositionAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        glEnableVertexAttribArray(sPositionAttrib);
        glVertexAttribPointer(sUvAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexAttribArray(sUvAttrib);
        glVertexAttribPointer(sColorAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(sColorAttrib);

        glBindTexture(GL_TEXTURE_2D, currentDrawTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mDrawTexSize.x, mDrawTexSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, sTexDrawFramebuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, currentDrawTex, 0);

        GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, drawBuffers);

        // Uh-oh.
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            return;

        glViewport(0, 0, mDrawTexSize.x, mDrawTexSize.y);

        glBindVertexArray(VAO);
        glUseProgram(sShaderProgram);

        float L = frameRect.Min.x, R = L + (mDrawTexSize.x / SCALE_FACTOR);
        float T = frameRect.Min.y + (mDrawTexSize.y / SCALE_FACTOR), B = frameRect.Min.y;

        const float orthoProjection[4][4] = {
            { 2.f / (R-L),   0.f,           0.f,  0.f },
            { 0.f,           2.f / (T-B),   0.f,  0.f },
            { 0.f,           0.f,          -1.f,  0.f },
            { (R+L) / (L-R), (T+B) / (B-T), 0.f,  1.f },
        };
        glUniformMatrix4fv(sProjMtxUniform, 1, GL_FALSE, orthoProjection[0]);

        glActiveTexture(GL_TEXTURE0);
        glUniform1i(sTextureUniform, 0);

        for (size_t i = 0; i < drawData.size(); i++) {
            const auto& cmd = drawData[i];

            const auto& foreColorA = cmd.callbackData.foreColorA;
            const auto& backColorA = cmd.callbackData.backColorA;

            const auto& foreColorB = cmd.callbackData.foreColorB;
            const auto& backColorB = cmd.callbackData.backColorB;

            glUniform3f(sForeColorAUniform, foreColorA.r, foreColorA.g, foreColorA.b);
            glUniform3f(sBackColorAUniform, backColorA.r, backColorA.g, backColorA.b);
            glUniform3f(sForeColorBUniform, foreColorB.r, foreColorB.g, foreColorB.b);
            glUniform3f(sBackColorBUniform, backColorB.r, backColorB.g, backColorB.b);

            glBindTexture(GL_TEXTURE_2D, cmd.textureId);
            glDrawArrays(GL_TRIANGLES, i * 6, 6);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    } break;

    default:
        break;
    }

    mOffset = prevOffset;
    mScale = prevScale;
}
