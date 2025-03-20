#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "Singleton.hpp"

#include "cellanim/CellAnim.hpp"

#include "SessionManager.hpp"

#include "PlayerManager.hpp"

#include "ConfigManager.hpp"

#include <algorithm>

#include <cstdint>

#include <imgui.h>
#include <imgui_internal.h>

#define BEGIN_GLOBAL_POPUP() \
    do { \
        ImGui::PushOverrideID(AppState::getInstance().globalPopupID); \
    } while (0)

#define END_GLOBAL_POPUP() \
    do { \
        ImGui::PopID(); \
    } while (0)

#define OPEN_GLOBAL_POPUP(popupId) \
    do { \
        ImGui::PushOverrideID(AppState::getInstance().globalPopupID); \
        ImGui::OpenPopup(popupId); \
        ImGui::PopID(); \
    } while (0)

class AppState : public Singleton<AppState> {
    friend class Singleton<AppState>; // Allow access to base class constructor

private:
    AppState() = default;
public:
    ~AppState() = default;

public:
    unsigned getUpdateRate() const {
        return ConfigManager::getInstance().getConfig().updateRate;
    }

    ImGuiID globalPopupID { 0xBEEFAB1E };

    bool getArrangementMode() const {
        SessionManager& sessionManager = SessionManager::getInstance();

        if (sessionManager.getSessionAvaliable())
            return sessionManager.getCurrentSession()->arrangementMode;
        else
            return false;
    }

    struct SelectedPart {
        unsigned index;
        int selectionOrder;

        bool operator==(const SelectedPart& other) const {
            return
                index == other.index &&
                selectionOrder == other.selectionOrder;
        }
        bool operator!=(const SelectedPart& other) const {
            return !(*this == other);
        }
    };

    std::vector<SelectedPart> selectedParts;
    int spSelectionOrder { 0 };

    void clearSelectedParts() {
        this->selectedParts.clear();
        this->spSelectionOrder = 0;
    }

    bool multiplePartsSelected() const {
        return this->selectedParts.size() > 1;
    }
    bool anyPartsSelected() const {
        return !this->selectedParts.empty();
    }
    bool singlePartSelected() const {
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
        }
        else if (it != selectedParts.end()) {
            selectedParts.erase(it);
            resetSelectionOrder();
        }
    }

    void setBatchPartSelection(unsigned partIndex, bool selected) {
        std::sort(selectedParts.begin(), selectedParts.end(), [](const SelectedPart& a, const SelectedPart& b) {
            return a.index < b.index;
        });

        auto it = std::lower_bound(
            selectedParts.begin(), selectedParts.end(),
            partIndex, [](const SelectedPart& sp, int partIndex) {
                return static_cast<int>(sp.index) < partIndex;
            }
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
        for (const ImGuiSelectionRequest& req : msIo->Requests) {
            if (req.Type == ImGuiSelectionRequestType_SetAll) {
                clearSelectedParts();
                if (req.Selected) {
                    selectedParts.reserve(msIo->ItemsCount);
                    for (unsigned i = 0; i < static_cast<unsigned>(msIo->ItemsCount); i++, spSelectionOrder++)
                        this->setBatchPartSelection(i, req.Selected);
                }
            }
            else if (req.Type == ImGuiSelectionRequestType_SetRange) {
                const unsigned selectionChanges = req.RangeLastItem - req.RangeFirstItem + 1;

                if (selectionChanges == 1 || selectionChanges < unsigned(selectedParts.size() / 100)) {
                    for (int i = req.RangeFirstItem; i <= req.RangeLastItem; i++)
                        this->setPartSelected(i, req.Selected);
                }
                else
                    this->applyRangeSelectionOrder(req);
            }
        }
    }

    void deleteSelectedParts(
        ImGuiMultiSelectIO* msIo,
        std::vector<CellAnim::ArrangementPart>& parts,
        int itemCurrentIndexToSelect
    ) {
        std::vector<CellAnim::ArrangementPart> newParts;
        newParts.reserve(parts.size() - selectedParts.size());

        int itemNextIndexToSelect = -1;
        for (unsigned index = 0; index < parts.size(); index++) {
            if (!isPartSelected(index))
                newParts.push_back(parts[index]);
            if (itemCurrentIndexToSelect == static_cast<int>(index))
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

        int focusedIndex = static_cast<int>(msIo->NavIdItem);

        if (!msIo->NavIdSelected) {
            msIo->RangeSrcReset = true;
            return focusedIndex;
        }

        for (unsigned index = focusedIndex + 1; index < partCount; index++) {
            if (!this->isPartSelected(index))
                return index;
        }

        for (int index = std::min(focusedIndex, static_cast<int>(partCount)) - 1; index >= 0; index--) {
            if (!this->isPartSelected(index))
                return index;
        }

        return -1;
    }

    int getMatchingNamePartIndex(
        const CellAnim::ArrangementPart& part,
        const CellAnim::Arrangement& arrangement
    ) {
        if (part.editorName.empty())
            return -1;

        for (unsigned i = 0; i < arrangement.parts.size(); i++) {
            const auto& lPart = arrangement.parts[i];
            if (
                !lPart.editorName.empty() &&
                lPart.editorName == part.editorName
            )
                return i;
        }

        return -1;
    }

    int getMatchingRegionPartIndex(
        CellAnim::ArrangementPart& part,
        const CellAnim::Arrangement& arrangement
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

    void correctSelectedParts() {
        unsigned size = PlayerManager::getInstance().getArrangement().parts.size();

        // Create local clone of selectedParts since setPartSelected
        // will mutate the original
        std::vector<SelectedPart> newSelected;
        newSelected.reserve(this->selectedParts.size());

        for (const auto& [index, order] : this->selectedParts) {
            if (index < size)
                newSelected.push_back({ index, order });
        }

        this->selectedParts = newSelected;
        resetSelectionOrder();
    }

    bool focusOnSelectedPart { false };

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
                this->setBatchPartSelection(index, req.Selected);
                //selectionOrder++;
            }
        }
        else { //if (req.RangeDirection < 0) {
            for (int index = req.RangeFirstItem; index >= req.RangeLastItem; index--) {
                this->setBatchPartSelection(index, req.Selected);
                //selectionOrder--;
            }
        }
    }
};

#endif // APPSTATE_HPP
