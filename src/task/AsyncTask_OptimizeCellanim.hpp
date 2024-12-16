#ifndef ASYNCTASK_OPTIMIZECELLANIM_HPP
#define ASYNCTASK_OPTIMIZECELLANIM_HPP

#include "AsyncTask.hpp"

#include <algorithm>

#include <unordered_set>

#include "../glInclude.hpp"

#include "../SessionManager.hpp"
#include "../MtCommandManager.hpp"

#include "../AppState.hpp"

struct OptimizeCellanimOptions {
    bool removeAnimationNames { false };
    bool removeUnusedArrangements { true };

    enum DownscaleOption {
        DownscaleOption_None,
        DownscaleOption_0_875x,
        DownscaleOption_0_75x,
        DownscaleOption_0_5x
    } downscaleSpritesheet { DownscaleOption_None };
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

        if (this->options.removeAnimationNames) {
            auto& animationNames = this->session->getAnimationNames();
            mtCommandManager.enqueueCommand([&animationNames](){
               animationNames.clear();
            }).get();
        }

        if (this->options.removeUnusedArrangements) {
            std::unordered_set<unsigned> usedIndices;
            std::vector<unsigned> toErase;

            for (const auto& anim : cellanim->animations) {
                for (const auto& key : anim.keys)
                    usedIndices.insert(key.arrangementIndex);
            }

            for (unsigned i = 0; i < cellanim->arrangements.size(); i++) {
                if (usedIndices.find(i) == usedIndices.end())
                    toErase.push_back(i);
            }

            auto& animations = cellanim->animations;
            auto& arrangements = cellanim->arrangements;

            mtCommandManager.enqueueCommand(
            [&arrangements, &toErase, &animations]() {
                for (unsigned i = 0; i < toErase.size(); i++) {
                    unsigned index = toErase.at(i);

                    auto it = arrangements.begin() + index;
                    arrangements.erase(it);

                    for (unsigned j = i; j < toErase.size(); j++) {
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
            }).get();
        }

        if (this->options.downscaleSpritesheet) {
            auto sheet = this->session->getCellanimSheet();

            unsigned newWidth = sheet->getWidth();
            unsigned newHeight = sheet->getHeight();

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

            // RGBA image
            unsigned char* originalPixels = new unsigned char[sheet->getWidth() * sheet->getHeight() * 4];

            mtCommandManager.enqueueCommand([&sheet, originalPixels]() {
                glBindTexture(GL_TEXTURE_2D, sheet->getTextureId());
                glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, originalPixels);
                glBindTexture(GL_TEXTURE_2D, 0);
            }).get();

            // RGBA image
            unsigned char* downscaledPixels = new unsigned char[sheet->getWidth() * sheet->getHeight() * 4];

            // Bilinear scale
            {
                const float ratioX = static_cast<float>(sheet->getWidth() - 1) / newWidth;
                const float ratioY = static_cast<float>(sheet->getHeight() - 1) / newHeight;

                for (unsigned _y = 0; _y < newHeight; _y++) {
                    const float scaledY = ratioY * _y;
                    const unsigned y    = static_cast<unsigned>(scaledY);
                    const float diffY   = scaledY - y;

                    const unsigned yOffset     = y * sheet->getWidth() * 4;
                    const unsigned yOffsetNext = (y + 1) * sheet->getWidth() * 4;

                    for (unsigned _x = 0; _x < newWidth; _x++) {
                        const float scaledX = ratioX * _x;
                        const unsigned x    = static_cast<unsigned>(scaledX);
                        const float diffX   = scaledX - x;

                        const unsigned index     = yOffset + x * 4;
                        const unsigned indexNext = yOffsetNext + x * 4;

                        for (unsigned c = 0; c < 4; c++) {
                            float topLeft     = originalPixels[index + c];
                            float topRight    = originalPixels[index + 4 + c];
                            float bottomLeft  = originalPixels[indexNext + c];
                            float bottomRight = originalPixels[indexNext + 4 + c];

                            float channel = topLeft * (1 - diffX) * (1 - diffY) +
                                            topRight * diffX * (1 - diffY) +
                                            bottomLeft * (1 - diffX) * diffY +
                                            bottomRight * diffX * diffY;

                            downscaledPixels[(_y * newWidth + _x) * 4 + c] = static_cast<unsigned char>(channel);
                        }
                    }
                }
            }

            delete[] originalPixels;

            sheet->LoadRGBA32(downscaledPixels, newWidth, newHeight);

            delete[] downscaledPixels;
        }
    }

    void Effect() override {
        SessionManager::getInstance().getCurrentSession()->clearUndoRedo();
        SessionManager::getInstance().SessionChanged();
    }

private:
    SessionManager::Session* session;
    OptimizeCellanimOptions options;
};

#endif // ASYNCTASK_OPTIMIZECELLANIM_HPP
