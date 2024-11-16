#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "Singleton.hpp"

#include "anim/Animatable.hpp"
#include "anim/RvlCellAnim.hpp"

#include "SessionManager.hpp"
#include "ConfigManager.hpp"

#include <algorithm>

#include <cstdint>
#include <cstring>

#include <imgui.h>
#include <imgui_internal.h>

#include "common.hpp"

// Stores instance of AppState in local appState.
#define GET_APP_STATE AppState& appState = AppState::getInstance()

// Stores globalAnimatable refpointer in local globalAnimatable.
#define GET_ANIMATABLE Animatable& globalAnimatable = AppState::getInstance().globalAnimatable

#define BEGIN_GLOBAL_POPUP ImGui::PushOverrideID(AppState::getInstance().globalPopupID)
#define END_GLOBAL_POPUP ImGui::PopID()

class AppState : public Singleton<AppState> {
    friend class Singleton<AppState>; // Allow access to base class constructor

private:
    std::array<float, 4> windowClearColor;
public:
    inline bool getDarkThemeEnabled() {
        return ConfigManager::getInstance().getConfig().theme == ThemeChoice_Dark;
    }

    void applyTheming() {
        if (this->getDarkThemeEnabled()) {
            ImGui::StyleColorsDark();
            this->windowClearColor = std::array<float, 4>({ 24 / 255.f, 24 / 255.f, 24 / 255.f, 1.f });
        }
        else {
            ImGui::StyleColorsLight();
            this->windowClearColor = std::array<float, 4>({ 248 / 255.f, 248 / 255.f, 248 / 255.f, 1.f });
        }

        ImGui::GetStyle().Colors[ImGuiCol_TabSelectedOverline] = ImVec4();
        ImGui::GetStyle().Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4();
    }

    inline const std::array<float, 4>& getWindowClearColor() {
        return this->windowClearColor;
    }

    inline unsigned getUpdateRate() {
        return ConfigManager::getInstance().getConfig().updateRate;
    }

    ImGuiID globalPopupID { 0xBEEFAB1E };
    inline void OpenGlobalPopup(const char* popupId) {
        ImGui::PushOverrideID(this->globalPopupID);
        ImGui::OpenPopup(popupId);
        ImGui::PopID();
    }

    struct Fonts {
        ImFont* normal;
        ImFont* large;
        ImFont* giant;
        ImFont* icon;
    } fonts;

    bool getArrangementMode() {
        if (SessionManager::getInstance().currentSession >= 0)
            return SessionManager::getInstance().getCurrentSession()->arrangementMode;
        else
            return false;
    }

    // This key object is meant for the arrangement mode to control the state of the key.
    RvlCellAnim::AnimationKey controlKey;

    unsigned selectedAnimation { 0 };
    //int selectedPart { -1 };

    struct SelectedPart {
        unsigned index;
        int selectionOrder;
    };

    std::vector<SelectedPart> selectedParts;
    int spSelectionOrder { 0 };

    void clearSelectedParts() {
        this->selectedParts.clear();
        this->spSelectionOrder = 0;
    }

    inline bool multiplePartsSelected() const {
        return this->selectedParts.size() > 1;
    }
    inline bool anyPartsSelected() const {
        return !this->selectedParts.empty();
    }
    inline bool singlePartSelected() const {
        return this->selectedParts.size() == 1;
    }

    bool isPartSelected(unsigned partIndex) const {
        return std::any_of(
            selectedParts.begin(), selectedParts.end(),
            [partIndex](const SelectedPart& sp) { return sp.index == partIndex; }
        );
    }
    void setPartSelected(unsigned partIndex, bool selected) {
        auto it = std::find_if(
            selectedParts.begin(),
            selectedParts.end(),
            [partIndex](const SelectedPart& sp) { return sp.index == partIndex; }
        );

        if (selected) {
            if (it == selectedParts.end())
                selectedParts.push_back({ partIndex, spSelectionOrder++ });
        } else if (it != selectedParts.end()) {
            selectedParts.erase(it);
            resetSelectionOrder();
        }
    }

    void setBatchPartSelection(unsigned partIndex, bool selected, int sizeBeforehand) {
        std::sort(selectedParts.begin(), selectedParts.end(), [](const SelectedPart& a, const SelectedPart& b) {
            return a.index < b.index;
        });

        auto it = std::lower_bound(
            selectedParts.begin(), selectedParts.end(),
            partIndex, [](const SelectedPart& sp, int partIndex) { return sp.index < partIndex; }
        );

        bool isContained = (it != selectedParts.end() && it->index == partIndex);

        if (selected && !isContained)
            selectedParts.push_back({ partIndex, spSelectionOrder++ });
        else if (!selected && isContained) {
            selectedParts.erase(it);
            resetSelectionOrder();
        }
    }

    void processMultiSelectRequests(ImGuiMultiSelectIO* msIo) {
        for (ImGuiSelectionRequest& req : msIo->Requests) {
            if (req.Type == ImGuiSelectionRequestType_SetAll) {
                clearSelectedParts();
                if (req.Selected) {
                    selectedParts.reserve(msIo->ItemsCount);
                    for (int idx = 0; idx < msIo->ItemsCount; idx++, spSelectionOrder++) {
                        this->setBatchPartSelection(idx, req.Selected, selectedParts.size());
                    }
                }
            }
            else if (req.Type == ImGuiSelectionRequestType_SetRange) {
                const int selectionChanges = (int)req.RangeLastItem - (int)req.RangeFirstItem + 1;

                if (selectionChanges == 1 || selectionChanges < selectedParts.size() / 100) {
                    for (int idx = (int)req.RangeFirstItem; idx <= (int)req.RangeLastItem; idx++)
                        this->setPartSelected(idx, req.Selected);
                } else
                    this->applyRangeSelectionOrder(req);
            }
        }
    }

    void deleteSelectedParts(
        ImGuiMultiSelectIO* msIo,
        std::vector<RvlCellAnim::ArrangementPart>& parts,
        int itemCurrentIndexToSelect
    ) {
        std::vector<RvlCellAnim::ArrangementPart> newParts;
        newParts.reserve(parts.size() - selectedParts.size());

        int itemNextIndexToSelect = -1;
        for (unsigned index = 0; index < parts.size(); index++) {
            if (!isPartSelected(index))
                newParts.push_back(parts[index]);
            if (itemCurrentIndexToSelect == index)
                itemNextIndexToSelect = newParts.size() - 1;
        }
        parts.swap(newParts);

        clearSelectedParts();
        if (itemNextIndexToSelect != -1 && msIo->NavIdSelected) {
            setPartSelected(itemNextIndexToSelect, true);
        }
    }

    int getNextPartIndexAfterDeletion(ImGuiMultiSelectIO* msIo, unsigned partCount) {
        if (selectedParts.empty())
            return -1;

        int focusedIndex = (int)msIo->NavIdItem;

        if (!msIo->NavIdSelected) {
            msIo->RangeSrcReset = true;
            return focusedIndex;
        }

        for (int index = focusedIndex + 1; index < partCount; index++) {
            if (!this->isPartSelected(index))
                return index;
        }

        for (int index = std::min(focusedIndex, (int)partCount) - 1; index >= 0; index--) {
            if (!this->isPartSelected(index))
                return index;
        }

        return -1;
    }

    int getMatchingNamePartIndex(
        const RvlCellAnim::ArrangementPart& part,
        const RvlCellAnim::Arrangement& arrangement
    ) {
        if (part.editorName[0] == '\0')
            return -1;

        for (unsigned i = 0; i < arrangement.parts.size(); i++) {
            const auto& lPart = arrangement.parts[i];
            if (
                lPart.editorName[0] != '\0' &&
                (strncmp(lPart.editorName, part.editorName, 32) == 0)
            )
                return i;
        }

        return -1;
    }

    int getMatchingRegionPartIndex(
        RvlCellAnim::ArrangementPart& part,
        const RvlCellAnim::Arrangement& arrangement
    ) {
        for (unsigned i = 0; i < arrangement.parts.size(); i++) {
            const auto& lPart = arrangement.parts[i];
            if (
                lPart.regionX == part.regionX &&
                lPart.regionY == part.regionY &&
                lPart.regionW == part.regionW &&
                lPart.regionH == part.regionH
            )
                return i;
        }

        return -1;
    }

    inline void correctSelectedParts() {
        unsigned size = this->globalAnimatable.getCurrentArrangement()->parts.size();

        // Create local clone of selectedParts since
        // setPartSelected will mutate the original
        auto sp = this->selectedParts;

        for (auto& part : sp) {
            if (part.index >= size)
                this->setPartSelected(part.index, false);
        }
    }

    bool focusOnSelectedPart { false };

    Animatable globalAnimatable {};

    struct OnionSkinState {
        bool enabled { false };
        bool drawUnder { false };
        bool rollOver { false };

        unsigned backCount { 3 };
        unsigned frontCount { 2 };

        uint8_t opacity { 50 };
    } onionSkinState;
private:
    void resetSelectionOrder() {
        int order { 0 };
        for (auto& part : selectedParts)
            part.selectionOrder = order++;

        spSelectionOrder = order;
    }

    void applyRangeSelectionOrder(const ImGuiSelectionRequest& req) {
        if (req.RangeDirection > 0) {
            for (int index = req.RangeLastItem; index <= req.RangeFirstItem; index++) {
                this->setBatchPartSelection(index, req.Selected, selectedParts.size());
                //selectionOrder++;
            }
        }
        else { //if (req.RangeDirection < 0) {
            for (int index = req.RangeFirstItem; index >= req.RangeLastItem; index--) {
                this->setBatchPartSelection(index, req.Selected, selectedParts.size());
                //selectionOrder--;
            }
        }
    }

private:
    AppState() {} // Private constructor to prevent instantiation
    ~AppState() {}
};

#endif // APPSTATE_HPP
