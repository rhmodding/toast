#include "SpritesheetFixUtil.hpp"

#include <cstdint>

#include <cstring>

#include <memory>

#include <vector>
#include <set>

#include <algorithm>

#include "stb/stb_rect_pack.h"

#include "cellanim/CellAnim.hpp"

#include "command/CommandModifySpritesheet.hpp"
#include "command/CommandModifyArrangements.hpp"
#include "command/CompositeCommand.hpp"

static inline bool operator==(const stbrp_rect& a, const stbrp_rect& b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}

static inline bool RectLess(const stbrp_rect& lhs, const stbrp_rect& rhs) {
    return std::tie(lhs.w, lhs.h, lhs.x, lhs.y) < std::tie(rhs.w, rhs.h, rhs.x, rhs.y);
}

bool SpritesheetFixUtil::FixRepack(Session& session, int sheetIndex) {
    constexpr int PADDING = 4;
    constexpr int PADDING_HALF = PADDING / 2;
    constexpr uint32_t BORDER_COLOR = IM_COL32_BLACK;

    if (sheetIndex < 0)
        sheetIndex = session.getCurrentCellAnim().object->getSheetIndex();

    std::shared_ptr cellanimSheet = session.sheets->getTextureByIndex(sheetIndex);
    std::shared_ptr cellanimObject = session.getCurrentCellAnim().object;

    // Copy
    std::vector<CellAnim::Arrangement> arrangements = cellanimObject->getArrangements();

    std::set<stbrp_rect, decltype(&RectLess)> uniqueRects(RectLess);

    for (const auto& arrangement : arrangements) {
        for (const auto& part : arrangement.parts) {
            stbrp_rect rect = {
                .w = static_cast<int>(part.cellSize.x) + PADDING,
                .h = static_cast<int>(part.cellSize.y) + PADDING,
                .x = static_cast<int>(part.cellOrigin.x) - PADDING_HALF,
                .y = static_cast<int>(part.cellOrigin.y) - PADDING_HALF,
            };
            uniqueRects.insert(rect);
        }
    }

    const unsigned numRects = uniqueRects.size();
    std::vector<stbrp_rect> sortedRects(uniqueRects.begin(), uniqueRects.end());
    auto originalRects = sortedRects;

    std::vector<stbrp_node> nodes(numRects);
    stbrp_context context;
    stbrp_init_target(&context, cellanimSheet->getWidth(), cellanimSheet->getHeight(), nodes.data(), numRects);

    if (!stbrp_pack_rects(&context, sortedRects.data(), numRects))
        return false;

    const unsigned pixelCount = cellanimSheet->getPixelCount();

    std::unique_ptr<unsigned char[]> newImage(new unsigned char[pixelCount * 4]);
    std::memset(newImage.get(), 0x00, (pixelCount * 4));

    std::unique_ptr<unsigned char[]> srcImage(new unsigned char[pixelCount * 4]);
    cellanimSheet->GetRGBA32(srcImage.get());

    for (unsigned i = 0; i < numRects; i++) {
        const auto& rect = sortedRects[i];
        const auto& origRect = originalRects[i];

        int ox = origRect.x;
        int oy = origRect.y;
        int nx = rect.x;
        int ny = rect.y;
        int w  = origRect.w;
        int h  = origRect.h;

        const int texW = cellanimSheet->getWidth();
        const int texH = cellanimSheet->getHeight();

        int srcX0 = std::max(0, ox);
        int srcY0 = std::max(0, oy);
        int srcX1 = std::min(texW, ox + w);
        int srcY1 = std::min(texH, oy + h);

        int dstX0 = std::max(0, nx);
        int dstY0 = std::max(0, ny);
        int dstX1 = std::min(texW, nx + w);
        int dstY1 = std::min(texH, ny + h);

        int copyW = std::min(srcX1 - srcX0, dstX1 - dstX0);
        int copyH = std::min(srcY1 - srcY0, dstY1 - dstY0);

        if (copyW > 0 && copyH > 0) {
            for (int row = 0; row < copyH; row++) {
                void* src = srcImage.get() + ((srcY0 + row) * texW + srcX0) * 4;
                void* dst = newImage.get() + ((dstY0 + row) * texW + dstX0) * 4;
                std::memcpy(dst, src, copyW * 4);
            }
        }

        uint32_t* pixelData = reinterpret_cast<uint32_t*>(newImage.get());

        for (int col = 0; col < copyW; col++) {
            if (
                (dstY0 >= 0 && dstY0 < texH) &&
                ((dstX0 + col) >= 0) && ((dstX0 + col) < texW)
            ) {
                pixelData[dstY0 * texW + dstX0 + col] = BORDER_COLOR;
            }

            if (
                ((dstY0 + copyH - 1) >= 0) && ((dstY0 + copyH - 1) < texH) &&
                ((dstX0 + col) >= 0) && ((dstX0 + col) < texW)
            ) {
                pixelData[(dstY0 + copyH - 1) * texW + dstX0 + col] = BORDER_COLOR;
            }
        }
        for (int row = 0; row < copyH; row++) {
            if (
                ((dstY0 + row) >= 0) && ((dstY0 + row) < texH) &&
                dstX0 >= 0 && dstX0 < texW
            ) {
                pixelData[(dstY0 + row) * texW + dstX0] = BORDER_COLOR;
            }

            if (
                ((dstY0 + row) >= 0) && ((dstY0 + row) < texH) &&
                ((dstX0 + copyW - 1) >= 0) && ((dstX0 + copyW - 1) < texW)
            ) {
                pixelData[(dstY0 + row) * texW + dstX0 + copyW - 1] = BORDER_COLOR;
            }
        }
    }

    auto newTexture = std::make_shared<TextureEx>();
    newTexture->LoadRGBA32(newImage.get(), cellanimSheet->getWidth(), cellanimSheet->getHeight());

    for (auto& arrangement : arrangements) {
        for (auto& part : arrangement.parts) {
            stbrp_rect rect = {
                .w = static_cast<int>(part.cellSize.x) + PADDING,
                .h = static_cast<int>(part.cellSize.y) + PADDING,
                .x = static_cast<int>(part.cellOrigin.x) - PADDING_HALF,
                .y = static_cast<int>(part.cellOrigin.y) - PADDING_HALF,
            };

            auto it = std::find(originalRects.begin(), originalRects.end(), rect);
            if (it != originalRects.end()) {
                const auto& newRect = sortedRects[std::distance(originalRects.begin(), it)];
                part.cellSize.x = newRect.w - PADDING;
                part.cellSize.y = newRect.h - PADDING;
                part.cellOrigin.x = newRect.x + PADDING_HALF;
                part.cellOrigin.y = newRect.y + PADDING_HALF;
            }
        }
    }

    newTexture->setName(cellanimSheet->getName());

    auto composite = std::make_shared<CompositeCommand>();

    composite->addCommand(std::make_shared<CommandModifySpritesheet>(
        sheetIndex, newTexture
    ));
    composite->addCommand(std::make_shared<CommandModifyArrangements>(
        session.getCurrentCellAnimIndex(), std::move(arrangements)
    ));

    session.addCommand(composite);

    return true;
}

bool SpritesheetFixUtil::FixAlphaBleed(Session& session, int sheetIndex) {
    if (sheetIndex < 0)
        sheetIndex = session.getCurrentCellAnim().object->getSheetIndex();

    std::shared_ptr cellanimObject = session.getCurrentCellAnim().object;
    std::shared_ptr cellanimSheet = session.sheets->getTextureByIndex(sheetIndex);

    const unsigned width = cellanimSheet->getWidth();
    const unsigned height = cellanimSheet->getHeight();
    const unsigned pixelCount = cellanimSheet->getPixelCount();

    std::unique_ptr<unsigned char[]> texture(new unsigned char[pixelCount * 4]());
    cellanimSheet->GetRGBA32(texture.get());

    // Attempt to 'fix' every pixel
    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            unsigned pixelIndex = (y * width + x) * 4;

            // Pixel is #00000000
            if (*(uint32_t*)(texture.get() + pixelIndex) == 0) {
                static const int offsets[4][2] = {
                    // left, right,  up,      down
                    {-1, 0}, {1, 0}, {0, -1}, {0, 1}
                };

                for (unsigned i = 0; i < 4; i++) {
                    int nx = x + offsets[i][0];
                    int ny = y + offsets[i][1];

                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        unsigned neighborIndex = (ny * width + nx) * 4;

                        if (texture[neighborIndex + 3] > 0) {
                            texture[pixelIndex + 0] = texture[neighborIndex + 0];
                            texture[pixelIndex + 1] = texture[neighborIndex + 1];
                            texture[pixelIndex + 2] = texture[neighborIndex + 2];
                            break;
                        }
                    }
                }
            }
        }
    }

    auto newTexture = std::make_shared<TextureEx>();
    newTexture->LoadRGBA32(texture.get(), cellanimSheet->getWidth(), cellanimSheet->getHeight());

    newTexture->setName(cellanimSheet->getName());

    session.addCommand(std::make_shared<CommandModifySpritesheet>(
        sheetIndex, newTexture
    ));

    return true;
}
