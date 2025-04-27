#include "AsyncTaskOptimizeCellanim.hpp"

#include <unordered_set>

#include "../MainThreadTaskManager.hpp"

#include "../stb/stb_image_resize2.h"

AsyncTaskOptimizeCellanim::AsyncTaskOptimizeCellanim(
    AsyncTaskId id,
    Session* session, OptimizeCellanimOptions options
) :
    AsyncTask(id, "Optimizing cellanim .."),

    session(session), options(options)
{}

static void removeAnimationNames(Session* session) {
    auto& animations = session->getCurrentCellAnim().object->animations;

    MainThreadTaskManager::getInstance().QueueTask([&animations]() {
        for (auto& animation : animations)
            animation.name.clear();
    }).get();
}

static void removeUnusedArrangements(Session* session) {
    std::shared_ptr cellanim = session->getCurrentCellAnim().object;

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

    MainThreadTaskManager::getInstance().QueueTask(
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

static void downscaleSpritesheet(Session* session, const OptimizeCellanimOptions& options) {
    std::shared_ptr sheet = session->getCurrentCellAnimSheet();

    unsigned newWidth = sheet->getWidth();
    unsigned newHeight = sheet->getHeight();

    switch (options.downscaleSpritesheet) {
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

    MainThreadTaskManager::getInstance().QueueTask([&sheet, originalPixels]() {
        glBindTexture(GL_TEXTURE_2D, sheet->getTextureId());
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, originalPixels);
        glBindTexture(GL_TEXTURE_2D, 0);
    }).get();

    // RGBA image
    unsigned char* downscaledPixels = new unsigned char[sheet->getWidth() * sheet->getHeight() * 4];

    stbir_resize_uint8_linear(
        originalPixels, sheet->getWidth(), sheet->getHeight(),
        sheet->getWidth() * 4,
        downscaledPixels, newWidth, newHeight,
        newWidth * 4,
        STBIR_RGBA
    );

    delete[] originalPixels;

    sheet->LoadRGBA32(downscaledPixels, newWidth, newHeight);

    delete[] downscaledPixels;
}

void AsyncTaskOptimizeCellanim::Run() {
    if (this->options.removeAnimationNames)
        removeAnimationNames(this->session);

    if (this->options.removeUnusedArrangements)
        removeUnusedArrangements(this->session);

    if (this->options.downscaleSpritesheet)
        downscaleSpritesheet(this->session, this->options);
}

void AsyncTaskOptimizeCellanim::Effect() {
    // TODO: make this whole thing undo-able
    this->session->clearUndoRedo();
    //SessionManager::getInstance().SessionChanged();
}
