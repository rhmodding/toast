#ifndef WINDOW_CANVAS_HPP
#define WINDOW_CANVAS_HPP

#include "BaseWindow.hpp"

#include <imgui.h>

#include <cstdint>

#include <stdexcept>

#include "CanvasState.hpp"

#include "cellanim/CellAnimRenderer.hpp"

#include "manager/SessionManager.hpp"

class WindowCanvas : public BaseWindow {
private:
    static WindowCanvas *sInstance;
public:
    WindowCanvas() { sInstance = this; }
    ~WindowCanvas() { sInstance = nullptr; }

    void update() override;

    static uint32_t getBackgroundColor() {
        if (useCustomBGColor()) {
            ImVec4 bgColor = getCustomBGColor();
            return IM_COL32(bgColor.x * 255, bgColor.y * 255, bgColor.z * 255, bgColor.w * 255);
        }
        else {
            if (sInstance == nullptr) {
                return 0x00000000;
            }

            switch (sInstance->mState.gridType) {
            case CanvasGridType::Dark:
                return IM_COL32(50, 50, 50, 255);
            case CanvasGridType::Light:
                return IM_COL32(255, 255, 255, 255);
            case CanvasGridType::None:
            default:
                return IM_COL32_BLACK_TRANS;
            }
        }
    }

private:
    void Menubar();

    void DrawCanvasText();

private:
    static bool useCustomBGColor() {
        Session *session = SessionManager::getInstance().getCurrentSession();
        return (session != nullptr) && session->getCurrentCellAnim().useBackgroundColor;
    }
    static void setUseCustomBGColor(bool use) {
        Session *session = SessionManager::getInstance().getCurrentSession();
        if (session != nullptr) {
            session->getCurrentCellAnim().useBackgroundColor = use;
        }
    }

    static ImVec4 getCustomBGColor() {
        Session *session = SessionManager::getInstance().getCurrentSession();
        if (session != nullptr) {
            float *bgColor = session->getCurrentCellAnim().backgroundColor;
            return ImVec4 { bgColor[0], bgColor[1], bgColor[2], 1.0f };
        }
        else {
            return ImVec4 { 0.0f, 0.0f, 0.0f, 0.0f };
        }
    }
    static void setCustomBGColor(ImVec4 color) {
        Session *session = SessionManager::getInstance().getCurrentSession();
        if (session != nullptr) {
            float *bgColor = session->getCurrentCellAnim().backgroundColor;
            bgColor[0] = color.x;
            bgColor[1] = color.y;
            bgColor[2] = color.z;
        }
    }

public:
    CanvasState mState;

private:
    CellAnimRenderer mCellAnimRenderer;

    ImVec2 mCanvasTopLeft;
    ImVec2 mCanvasSize;

    enum PartHandle : int {
        PartHandle_None = -1,
        PartHandle_Top = 0,
        PartHandle_Right = 1,
        PartHandle_Bottom = 2,
        PartHandle_Left = 3,
        PartHandle_TopLeft = 4,
        PartHandle_TopRight = 5,
        PartHandle_BottomRight = 6,
        PartHandle_BottomLeft = 7,
        PartHandle_Whole = 8
    } mHoveredPartHandle { PartHandle_None };
};

#endif // WINDOW_CANVAS_HPP
