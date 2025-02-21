#include "AsyncTaskOptimizeCellanim.hpp"

AsyncTaskOptimizeCellanim::AsyncTaskOptimizeCellanim(
    AsyncTaskId id,
    Session* session, OptimizeCellanimOptions options
) :
    AsyncTask(id, "Optimizing cellanim .."),

    session(session), options(options)
{}

static void bilinearScale(
    unsigned srcWidth, unsigned srcHeight,
    unsigned dstWidth, unsigned dstHeight,
    unsigned char* srcPixels, unsigned char* dstPixels
) {
    const float ratioX = static_cast<float>(srcWidth - 1) / dstWidth;
    const float ratioY = static_cast<float>(srcHeight - 1) / dstHeight;

    for (unsigned _y = 0; _y < dstHeight; _y++) {
        const float scaledY = ratioY * _y;
        const unsigned y    = static_cast<unsigned>(scaledY);
        const float diffY   = scaledY - y;

        const unsigned yOffset     = y * srcWidth * 4;
        const unsigned yOffsetNext = (y + 1) * srcWidth * 4;

        for (unsigned _x = 0; _x < dstWidth; _x++) {
            const float scaledX = ratioX * _x;
            const unsigned x    = static_cast<unsigned>(scaledX);
            const float diffX   = scaledX - x;

            const unsigned index     = yOffset + x * 4;
            const unsigned indexNext = yOffsetNext + x * 4;

            for (unsigned c = 0; c < 4; c++) {
                float topLeft     = srcPixels[index + c];
                float topRight    = srcPixels[index + 4 + c];
                float bottomLeft  = srcPixels[indexNext + c];
                float bottomRight = srcPixels[indexNext + 4 + c];

                float channel = topLeft * (1 - diffX) * (1 - diffY) +
                                topRight * diffX * (1 - diffY) +
                                bottomLeft * (1 - diffX) * diffY +
                                bottomRight * diffX * diffY;

                dstPixels[(_y * dstWidth + _x) * 4 + c] = static_cast<unsigned char>(channel);
            }
        }
    }
}

static void removeAnimationNames(Session* session) {
    auto& animations = session->getCurrentCellanim().object->animations;

    MainThreadTaskManager::getInstance().QueueTask([&animations]() {
        for (auto& animation : animations)
            animation.name.clear();
    }).get();
}

static void removeUnusedArrangements(Session* session) {
    std::shared_ptr cellanim = session->getCurrentCellanim().object;

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
    std::shared_ptr sheet = session->getCurrentCellanimSheet();

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

    bilinearScale(sheet->getWidth(), sheet->getHeight(), newWidth, newHeight, originalPixels, downscaledPixels);

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
    SessionManager::getInstance().getCurrentSession()->clearUndoRedo();
    //SessionManager::getInstance().SessionChanged();
}
