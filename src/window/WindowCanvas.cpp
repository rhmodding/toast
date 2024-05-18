#include "WindowCanvas.hpp"

#include <sstream>
#include <string>

#include <math.h>

#include "../SessionManager.hpp"

#include "../ConfigManager.hpp"

bool isPointInPolygon(const ImVec2& point, const ImVec2* polygon, uint16_t numVertices) {
    double x = point.x, y = point.y;
    bool inside = false;
 
    // Store the first point in the polygon and initialize
    // the second point
    ImVec2 p1 = polygon[0];
    ImVec2 p2;
 
    // Loop through each edge in the polygon
    for (uint16_t i = 1; i <= numVertices; i++) {
        // Get the next point in the polygon
        p2 = polygon[i % numVertices];
 
        // Check if the point is above the minimum y
        // coordinate of the edge
        if (y > std::min(p1.y, p2.y)) {
            // Check if the point is below the maximum y
            // coordinate of the edge
            if (y <= std::max(p1.y, p2.y)) {
                // Check if the point is to the left of the
                // maximum x coordinate of the edge
                if (x <= std::max(p1.x, p2.x)) {
                    // Calculate the x-intersection of the
                    // line connecting the point to the edge
                    double x_intersection
                        = (y - p1.y) * (p2.x - p1.x)
                              / (p2.y - p1.y)
                          + p1.x;
 
                    // Check if the point is on the same
                    // line as the edge or to the left of
                    // the x-intersection
                    if (p1.x == p2.x
                        || x <= x_intersection) {
                        // Flip the inside flag
                        inside = !inside;
                    }
                }
            }
        }
 
        // Store the current point as the first point for
        // the next iteration
        p1 = p2;
    }
 
    // Return the value of the inside flag
    return inside;
}

const ImVec2 feverSafeAreaSize(832, 456);
const ImVec2 megamixSafeAreaSize(440, 240);

const uint8_t uint8_one = 1;

void WindowCanvas::Menubar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Grid")) {
            if (ImGui::MenuItem("None", nullptr, gridType == GridType_None))
                gridType = GridType_None;

            if (ImGui::MenuItem("Dark", nullptr, gridType == GridType_Dark))
                gridType = GridType_Dark;

            if (ImGui::MenuItem("Light", nullptr, gridType == GridType_Light))
                gridType = GridType_Light;

            if (ImGui::BeginMenu("Custom")) {
                bool enabled = gridType == GridType_Custom;
                if (ImGui::Checkbox("Enabled", &enabled)) {
                    if (enabled)
                        gridType = GridType_Custom;
                    else
                        gridType = AppState::getInstance().darkTheme ? GridType_Dark : GridType_Light;
                };

                ImGui::SeparatorText("Color Picker");

                ImGui::ColorPicker4(
                    "##ColorPicker",
                    (float*)&customGridColor,
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                    nullptr
                );

                ImGui::EndMenu();
            }

            ImGui::Separator();

            ImGui::MenuItem("Grid Lines", nullptr, &this->enableGridLines, gridType != GridType_None);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset", nullptr, false)) {
                this->canvasOffset = { 0.f, 0.f };
                this->canvasZoom = 0.f;
            }

            if (ImGui::MenuItem("Reset to Safe Area", nullptr, false)) {
                this->canvasOffset = { 0.f, 0.f };

                ImVec2 rect = feverSafeAreaSize;

                float scale;
                Common::fitRectangle(rect, this->canvasSize, scale);

                this->canvasZoom = scale - 1.f - 0.1f;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Rendering")) {
            if (ImGui::BeginMenu("Draw Bounding Boxes")) {
                ImGui::Checkbox("For all parts", &drawAllBounding);

                ImGui::SeparatorText("Color Picker");

                ImGui::ColorPicker4(
                    "##ColorPicker",
                    (float*)&partBoundingDrawColor,
                    ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHex,
                    nullptr
                );

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Safe Area")) {
                ImGui::Checkbox("Enabled", &this->visualizeSafeArea);
                
                ImGui::SeparatorText("Options");

                bool pushDisable{ false };
                if (!this->visualizeSafeArea) {
                    pushDisable = true;
                    ImGui::BeginDisabled(true);
                }

                ImGui::DragScalar("Alpha", ImGuiDataType_U8, &this->safeAreaAlpha);

                if (pushDisable)
                    ImGui::EndDisabled();

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Enable part transparency", nullptr, allowOpacity))
                allowOpacity = !allowOpacity;

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void WindowCanvas::Update() {
    bool& changed = SessionManager::getInstance().getCurrentSessionModified();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_MenuBar);
    ImGui::PopStyleVar();

    // Note: ImDrawList uses screen coordinates
    this->canvasTopLeft = ImGui::GetCursorScreenPos();
    this->canvasSize = ImGui::GetContentRegionAvail();
    ImVec2 canvasBottomRight = ImVec2(
        this->canvasTopLeft.x + this->canvasSize.x,
        this->canvasTopLeft.y + this->canvasSize.y
    );

    this->Menubar();

    // Determine background color
    uint32_t backgroundColor{ 0 };
    switch (this->gridType) {
        case GridType_None:
            backgroundColor = IM_COL32_BLACK_TRANS;
            break;
        
        case GridType_Dark:
            backgroundColor = IM_COL32(50, 50, 50, 255);
            break;

        case GridType_Light:
            backgroundColor = IM_COL32(255, 255, 255, 255);
            break;

        case GridType_Custom:
            backgroundColor = IM_COL32(
                customGridColor.x * 255,
                customGridColor.y * 255,
                customGridColor.z * 255,
                customGridColor.w * 255
            );
            break;
    
        default:
            // The default value of backgroundColor is already 0.
            break;
    }

    GET_IMGUI_IO;
    GET_WINDOW_DRAWLIST;

    drawList->AddRectFilled(this->canvasTopLeft, canvasBottomRight, backgroundColor);

    // This catches interactions
    ImGui::InvisibleButton("CanvasInteractions", this->canvasSize,
        ImGuiButtonFlags_MouseButtonLeft |
        ImGuiButtonFlags_MouseButtonRight |
        ImGuiButtonFlags_MouseButtonMiddle
    );

    const bool interactionHovered = ImGui::IsItemHovered();
    const bool interactionActive = ImGui::IsItemActive(); // Held

    const ImVec2 origin(
        this->canvasTopLeft.x + this->canvasOffset.x + static_cast<int>(this->canvasSize.x / 2),
        this->canvasTopLeft.y + this->canvasOffset.y + static_cast<int>(this->canvasSize.y / 2)
    );

    // Dragging
    const float mousePanThreshold = -1.0f;
    bool draggingCanvas = interactionActive && (
        ImGui::IsMouseDragging(ImGuiMouseButton_Right, mousePanThreshold) ||
        ImGui::IsMouseDragging(ImGuiMouseButton_Middle, mousePanThreshold) ||

        ConfigManager::getInstance().config.canvasLMBPanEnabled ?
            ImGui::IsMouseDragging(ImGuiMouseButton_Left, mousePanThreshold) :
            0
    );

    static bool lastDraggingCanvas{ draggingCanvas };
    static ImVec2 dragPartOffset{ 0, 0 };
    
    static bool hoveringOverSelected{ false };

    GET_ANIMATABLE;
    GET_APP_STATE;

    RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable->getCurrentArrangement();

    {
        globalAnimatable->offset = origin;

        globalAnimatable->scaleX = this->canvasZoom + 1;
        globalAnimatable->scaleY = this->canvasZoom + 1;
    }

    // All drawing operations
    {
        drawList->PushClipRect(this->canvasTopLeft, canvasBottomRight, true);
        {
            // Draw grid
            if ((this->gridType != GridType_None) && this->enableGridLines) {
                float GRID_STEP = 64.0f * (this->canvasZoom + 1);

                for (float x = fmodf(this->canvasOffset.x + static_cast<int>(this->canvasSize.x / 2), GRID_STEP); x < this->canvasSize.x; x += GRID_STEP)
                    drawList->AddLine(
                        ImVec2(this->canvasTopLeft.x + x, this->canvasTopLeft.y),
                        ImVec2(this->canvasTopLeft.x + x, canvasBottomRight.y),
                        IM_COL32(200, 200, 200, 40)
                    );
                
                for (float y = fmodf(this->canvasOffset.y + static_cast<int>(this->canvasSize.y / 2), GRID_STEP); y < this->canvasSize.y; y += GRID_STEP)
                    drawList->AddLine(
                        ImVec2(this->canvasTopLeft.x, this->canvasTopLeft.y + y),
                        ImVec2(canvasBottomRight.x, this->canvasTopLeft.y + y),
                        IM_COL32(200, 200, 200, 40)
                    );
            }
        
            // Draw animatable
            {
                bool drawOnionSkin = appState.onionSkinState.enabled;
                bool drawUnder = appState.onionSkinState.drawUnder;

                if (drawOnionSkin && drawUnder) {
                    globalAnimatable->DrawOnionSkin(
                        drawList,
                        appState.onionSkinState.backCount,
                        appState.onionSkinState.frontCount,
                        appState.onionSkinState.opacity
                    );
                }

                globalAnimatable->Draw(drawList, allowOpacity);

                if (drawOnionSkin && !drawUnder) {
                    globalAnimatable->DrawOnionSkin(
                        drawList,
                        appState.onionSkinState.backCount,
                        appState.onionSkinState.frontCount,
                        appState.onionSkinState.opacity
                    );
                }
            }

            // Draw selected part bounds
            if (appState.focusOnSelectedPart && appState.selectedPart >= 0) {
                auto bounding = globalAnimatable->getPartWorldQuad(globalAnimatable->getCurrentKey(), appState.selectedPart);
                drawList->AddQuad(
                    bounding[0], bounding[1], bounding[2], bounding[3],

                    // Yellow color if hovering
                    hoveringOverSelected && ImGui::IsWindowHovered() ?
                        IM_COL32(255, 255, 0, 255) :
                        IM_COL32(
                            partBoundingDrawColor.x * 255,
                            partBoundingDrawColor.y * 255,
                            partBoundingDrawColor.z * 255,
                            partBoundingDrawColor.w * 255
                        )
                );
            }

            // Draw all part bounds if enabled
            if (this->drawAllBounding)
                for (uint16_t i = 0; i < arrangementPtr->parts.size(); i++) {
                    if (appState.focusOnSelectedPart && appState.selectedPart == i)
                        continue; // Skip over part if bounds are already drawn

                    uint32_t color = IM_COL32(
                        partBoundingDrawColor.x * 255,
                        partBoundingDrawColor.y * 255,
                        partBoundingDrawColor.z * 255,
                        partBoundingDrawColor.w * 255
                    );

                    auto bounding = globalAnimatable->getPartWorldQuad(globalAnimatable->getCurrentKey(), i);
                    drawList->AddQuad(bounding[0], bounding[1], bounding[2], bounding[3], color);
                }
        
            // Draw safe area if enabled
            if (this->visualizeSafeArea) {
                ImVec2 boxTopLeft = {
                    origin.x - ((feverSafeAreaSize.x * (this->canvasZoom + 1)) / 2),
                    origin.y - ((feverSafeAreaSize.y * (this->canvasZoom + 1)) / 2)
                };

                ImVec2 boxBottomRight = {
                    boxTopLeft.x + (feverSafeAreaSize.x * (this->canvasZoom + 1)),
                    boxTopLeft.y + (feverSafeAreaSize.y * (this->canvasZoom + 1))
                };

                uint32_t color = IM_COL32(0, 0, 0, this->safeAreaAlpha);

                drawList->AddRectFilled( // Top Box
                    canvasTopLeft,
                    { canvasBottomRight.x, boxTopLeft.y },
                    color
                );
                drawList->AddRectFilled( // Left Box
                    { canvasTopLeft.x, boxTopLeft.y },
                    { boxTopLeft.x, boxBottomRight.y },
                    color
                );
                drawList->AddRectFilled( // Right Box
                    { boxBottomRight.x, boxTopLeft.y },
                    { canvasBottomRight.x, boxBottomRight.y },
                    color
                );
                drawList->AddRectFilled( // Bottom Box
                    { canvasTopLeft.x, boxBottomRight.y },
                    canvasBottomRight,
                    color
                );
            }
        }
        drawList->PopClipRect();

        this->DrawCanvasText();

        // Set tooltip
        if (hoveringOverSelected && ImGui::IsWindowHovered() && !draggingCanvas)
            ImGui::SetTooltip(
                "Part no. %u\nYou can drag this part to change it's position.",
                appState.selectedPart + 1
            );
    }

    // Selected part start / stop dragging
    if ((lastDraggingCanvas != draggingCanvas) && hoveringOverSelected) {
        if (draggingCanvas) {
            dragPartOffset = ImVec2(
                arrangementPtr->parts.at(appState.selectedPart).positionX,
                arrangementPtr->parts.at(appState.selectedPart).positionY
            );
        }
        else {
            changed |= true;

            arrangementPtr->parts.at(appState.selectedPart).positionX = static_cast<int16_t>(dragPartOffset.x);
            arrangementPtr->parts.at(appState.selectedPart).positionY = static_cast<int16_t>(dragPartOffset.y);
        }
    }
    lastDraggingCanvas = draggingCanvas;

    // Determine if hovering over selected part
    if (appState.focusOnSelectedPart && appState.selectedPart >= 0) {
        auto bounding = globalAnimatable->getPartWorldQuad(globalAnimatable->getCurrentKey(), appState.selectedPart);

        ImVec2 polygon[5] = {
            bounding[0],
            bounding[1],
            bounding[2],
            bounding[3],
            bounding[0]
        };

        hoveringOverSelected = isPointInPolygon(io.MousePos, polygon, 5);
    }

    // Dragging canvas / selected part
    if (draggingCanvas) {
        if (hoveringOverSelected) {
            dragPartOffset.x += io.MouseDelta.x / ((canvasZoom) + 1.f);
            dragPartOffset.y += io.MouseDelta.y / ((canvasZoom) + 1.f);
        
            arrangementPtr->parts.at(appState.selectedPart).positionX = static_cast<int16_t>(dragPartOffset.x);
            arrangementPtr->parts.at(appState.selectedPart).positionY = static_cast<int16_t>(dragPartOffset.y);
        }
        else {
            this->canvasOffset.x += io.MouseDelta.x;
            this->canvasOffset.y += io.MouseDelta.y;
        }
    }

    // Canvas zooming
    {
        const float maxZoom = 9.f;
        const float minZoom = -0.90f;

        if (interactionHovered) {
            GET_IMGUI_IO;

            if (io.KeyShift) 
                this->canvasZoom += io.MouseWheel * CANVAS_ZOOM_SPEED_FAST;
            else
                this->canvasZoom += io.MouseWheel * CANVAS_ZOOM_SPEED;

            if (this->canvasZoom < minZoom)
                this->canvasZoom = minZoom;

            if (this->canvasZoom > maxZoom)
                this->canvasZoom = maxZoom;
        }
    }

    ImGui::End();
}

void WindowCanvas::DrawCanvasText() {
    uint32_t textColor{ 0 };
    switch (this->gridType) {
        case GridType_None:
            textColor = AppState::getInstance().darkTheme ? IM_COL32_WHITE : IM_COL32_BLACK;
            break;
        
        case GridType_Dark:
            textColor = IM_COL32_WHITE;
            break;

        case GridType_Light:
            textColor = IM_COL32_BLACK;
            break;

        case GridType_Custom: {
            float y = 0.2126f * customGridColor.x + 0.7152f * customGridColor.y + 0.0722f * customGridColor.z;

            if (y > 0.5f)
                textColor = IM_COL32_BLACK;
            else
                textColor = IM_COL32_WHITE;
        } break;
    
        default:
            break;
    }

    float textDrawHeight = this->canvasTopLeft.y + 5.f;

    GET_WINDOW_DRAWLIST;

    if ((this->canvasOffset.x != 0.f) || (this->canvasOffset.y != 0.f)) {
        std::stringstream fmtStream;
        fmtStream <<
            "Offset: " <<
            std::to_string(this->canvasOffset.x) << ", " <<
            std::to_string(this->canvasOffset.y);

        drawList->AddText(
            ImVec2(
                this->canvasTopLeft.x + 10,
                textDrawHeight
            ),
            textColor, fmtStream.str().c_str()
        );

        textDrawHeight += 3.f + ImGui::CalcTextSize(fmtStream.str().c_str()).y;
    }

    if (this->canvasZoom != 0.f) {
        std::stringstream fmtStream;
        fmtStream <<
            "Zoom: " <<
            std::to_string(static_cast<uint16_t>((this->canvasZoom + 1) * 100)) <<
            "% (hold [Shift] to zoom faster)";

        drawList->AddText(
            ImVec2(
                this->canvasTopLeft.x + 10,
                textDrawHeight
            ),
            textColor, fmtStream.str().c_str()
        );

        textDrawHeight += 3.f + ImGui::CalcTextSize(fmtStream.str().c_str()).y;
    }

    if (!AppState::getInstance().globalAnimatable->getDoesDraw(this->allowOpacity)) {
        const char* text = "Nothing to draw on this frame";

        drawList->AddText(
            ImVec2(
                this->canvasTopLeft.x + 10,
                textDrawHeight
            ),
            textColor, text
        );

        textDrawHeight += 3.f + ImGui::CalcTextSize(text).y;
    }
}