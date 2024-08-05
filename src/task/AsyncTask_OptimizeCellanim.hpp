#ifndef ASYNCTASK_OPTIMIZECELLANIM_HPP
#define ASYNCTASK_OPTIMIZECELLANIM_HPP

#include "AsyncTask.hpp"

#include <algorithm>

#include <unordered_set>

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "../SessionManager.hpp"
#include "../MtCommandManager.hpp"

#include "../AppState.hpp"

struct OptimizeCellanimOptions {
    bool removeAnimationNames{ false };
    bool removeHiddenPartsEditor{ true };
    bool removeInvisibleParts{ true };
    bool removeUnusedArrangements{ true };

    enum DownscaleOption: int {
        DownscaleOption_None,
        DownscaleOption_0_875x,
        DownscaleOption_0_75x,
        DownscaleOption_0_5x
    } downscaleSpritesheet{ DownscaleOption_None };
};

class OptimizeCellanimTask : public AsyncTask {
public:
    OptimizeCellanimTask(
        uint32_t id,
        SessionManager::Session* session, OptimizeCellanimOptions options
    ) :
        AsyncTask(id, "Optimizing cellanim .."),

        session(session), options(options)
    {}

protected:
    void Run() override {
        auto cellanim = this->session->getCellanimObject();

        GET_MT_COMMAND_MANAGER;
        GET_SESSION_MANAGER;

        if (this->options.removeAnimationNames) {
            auto& animationNames = this->session->getAnimationNames();
            std::future<void> future = mtCommandManager.enqueueCommand([&animationNames](){
               animationNames.clear();
            });

            future.get();
        }

        if (
            this->options.removeHiddenPartsEditor ||
            this->options.removeInvisibleParts
        ) {
            std::vector<std::pair<uint32_t, uint32_t>> toErase;

            auto& arrangements = cellanim->arrangements;

            for (uint32_t i = 0; i < arrangements.size(); i++) {
                auto& arrangement = arrangements.at(i);
                for (uint32_t j = 0; j < arrangement.parts.size(); j++) {
                    const auto& part = arrangement.parts.at(j);

                    // Skip if it might be a templating part
                    if (part.regionW < 16 || part.regionH < 16)
                        continue;

                    if (
                        this->options.removeHiddenPartsEditor ?
                            !part.editorVisible : false ||
                        this->options.removeInvisibleParts ? (
                            part.opacity == 0 ||
                            (part.scaleX == 0.f && part.scaleY == 0.f)
                        ) : false
                    ) {
                        toErase.push_back({ i, j });
                        continue;
                    }
                }
            }

            std::sort(toErase.rbegin(), toErase.rend(), [](const auto& a, const auto& b) {
                return a.second < b.second;
            });

            std::future<void> future = mtCommandManager.enqueueCommand(
            [&toErase, &arrangements]() {
                for (const auto& pair : toErase) {
                    auto& parts = arrangements.at(pair.first).parts;
                    parts.erase(parts.begin() + pair.second);
                }

                AppState::getInstance().correctSelectedPart();
            });

            future.get();
        }

        if (this->options.removeUnusedArrangements) {
            std::unordered_set<uint32_t> usedIndices;
            std::vector<uint32_t> toErase;

            for (const auto& anim : cellanim->animations) {
                for (const auto& key : anim.keys)
                    usedIndices.insert(key.arrangementIndex);
            }

            for (uint32_t i = 0; i < cellanim->arrangements.size(); i++) {
                if (usedIndices.find(i) == usedIndices.end())
                    toErase.push_back(i);
            }

            auto& animations = cellanim->animations;
            auto& arrangements = cellanim->arrangements;

            std::future<void> future = mtCommandManager.enqueueCommand(
            [&arrangements, &toErase, &animations]() {
                for (uint32_t i = 0; i < toErase.size(); i++) {
                    uint32_t index = toErase.at(i);

                    auto it = arrangements.begin() + index;
                    arrangements.erase(it);

                    for (uint32_t j = i; j < toErase.size(); j++) {
                        if (toErase.at(j) > index)
                            toErase.at(j)--;
                    }

                    for (auto& animation : animations) {
                        for (auto& key : animation.keys) {
                            if (key.arrangementIndex > index)
                                key.arrangementIndex--;
                        }
                    }
                }
            });

            future.get();
        }

        if (this->options.downscaleSpritesheet) {
            auto sheet = this->session->getCellanimSheet();

            uint16_t newWidth = sheet->width;
            uint16_t newHeight = sheet->height;

            switch (this->options.downscaleSpritesheet) {
                case OptimizeCellanimOptions::DownscaleOption_0_875x:
                    newWidth *= .875f;
                    newHeight *= .875f;
                    break;
                case OptimizeCellanimOptions::DownscaleOption_0_75x:
                    newWidth *= .75f;
                    newHeight *= .75f;
                    break;
                case OptimizeCellanimOptions::DownscaleOption_0_5x:
                    newWidth *= .5f;
                    newHeight *= .5f;
                    break;

                default:
                    break;
            }

            if (newWidth & 1) newWidth++;
            if (newHeight & 1) newHeight++;

            // RGBA
            auto bilinearInterpolation = [](
                const unsigned char* src, uint32_t srcWidth, uint32_t srcHeight,
                unsigned char* dst, uint32_t dstWidth, uint32_t dstHeight
            ) {
                float x_ratio = static_cast<float>(srcWidth - 1) / dstWidth;
                float y_ratio = static_cast<float>(srcHeight - 1) / dstHeight;
                float x_diff, y_diff, blue, red, green, alpha;
                uint32_t x, y, index;

                for (uint32_t i = 0; i < dstHeight; i++) {
                    for (uint32_t j = 0; j < dstWidth; j++) {
                        x = static_cast<uint32_t>(x_ratio * j);
                        y = static_cast<uint32_t>(y_ratio * i);
                        x_diff = (x_ratio * j) - x;
                        y_diff = (y_ratio * i) - y;
                        index = (y * srcWidth + x) * 4;

                        for (uint32_t c = 0; c < 4; c++) {
                            blue = src[index + c] * (1 - x_diff) * (1 - y_diff) +
                                    src[index + 4 + c] * x_diff * (1 - y_diff) +
                                    src[(y + 1) * srcWidth * 4 + x * 4 + c] * (1 - x_diff) * y_diff +
                                    src[(y + 1) * srcWidth * 4 + (x + 1) * 4 + c] * x_diff * y_diff;

                            dst[(i * dstWidth + j) * 4 + c] = static_cast<unsigned char>(blue);
                        }
                    }
                }
            };

            std::vector<unsigned char> originalPixels(sheet->width * sheet->height * 4);

            std::future<void> futureA = mtCommandManager.enqueueCommand([&sheet, &originalPixels]() {
                glBindTexture(GL_TEXTURE_2D, sheet->texture);
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, originalPixels.data());
                glBindTexture(GL_TEXTURE_2D, 0);
            });

            futureA.get();

            std::vector<unsigned char> downscaledPixels(newWidth * newHeight * 4);
            bilinearInterpolation(
                originalPixels.data(), sheet->width, sheet->height,
                downscaledPixels.data(), newWidth, newHeight
            );

            GLuint newTexture{ 0 };

            std::future<void> futureB = mtCommandManager.enqueueCommand([&sheet, &newTexture, &downscaledPixels, newWidth, newHeight]() {
                sheet->FreeTexture();

                glGenTextures(1, &newTexture);
                glBindTexture(GL_TEXTURE_2D, newTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, newWidth, newHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, downscaledPixels.data());

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

                sheet->texture = newTexture;

                sheet->width = newWidth;
                sheet->height = newHeight;
            });

            futureB.get();
        }
    }

    void Effect() override {
        SessionManager::getInstance().getCurrentSession()->clearUndoRedoStack();
        SessionManager::getInstance().SessionChanged();
    }

private:
    SessionManager::Session* session;
    OptimizeCellanimOptions options;
};

#endif // ASYNCTASK_OPTIMIZECELLANIM_HPP
