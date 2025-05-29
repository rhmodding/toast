#include "SelectionState.hpp"

#include <imgui.h>

#include <algorithm>

#include "PlayerManager.hpp"

int SelectionState::getMatchingNamePartIndex(
    const CellAnim::ArrangementPart& part,
    const CellAnim::Arrangement& arrangement
) {
    if (part.editorName.empty())
        return -1;

    for (size_t i = 0; i < arrangement.parts.size(); i++) {
        const auto& lPart = arrangement.parts[i];
        if (
            !lPart.editorName.empty() &&
            lPart.editorName == part.editorName
        )
            return i;
    }

    return -1;
}
int SelectionState::getMatchingRegionPartIndex(
    const CellAnim::ArrangementPart& part,
    const CellAnim::Arrangement& arrangement
) {
    for (size_t i = 0; i < arrangement.parts.size(); i++) {
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

void SelectionState::resetSelectionOrder() {
    mNextSelectionOrder = 0;
    for (auto& sp : mSelectedParts)
        sp.selectionOrder = mNextSelectionOrder++;
}

void SelectionState::applyRangeSelectionOrder(const ImGuiSelectionRequest& req) {
    if (req.RangeDirection > 0) {
        for (int i = req.RangeLastItem; i <= req.RangeFirstItem; i++) {
            setBatchPartSelection(i, req.Selected);
            //selectionOrder++;
        }
    }
    else { //if (req.RangeDirection < 0) {
        for (int i = req.RangeFirstItem; i >= req.RangeLastItem; i--) {
            setBatchPartSelection(i, req.Selected);
            //selectionOrder--;
        }
    }
}

void SelectionState::setPartSelected(unsigned partIndex, bool selected) {
    auto it = std::find_if(
        mSelectedParts.begin(), mSelectedParts.end(),
        [partIndex](const SelectedPart& sp) {
            return sp.index == partIndex;
        }
    );

    if (selected) {
        if (it == mSelectedParts.end())
            mSelectedParts.push_back({ partIndex, mNextSelectionOrder++ });
    }
    else if (it != mSelectedParts.end()) {
        mSelectedParts.erase(it);
        resetSelectionOrder();
    }
}

void SelectionState::setBatchPartSelection(unsigned partIndex, bool selected) {
    std::sort(
        mSelectedParts.begin(), mSelectedParts.end(),
        [](const SelectedPart& a, const SelectedPart& b) {
            return a.index < b.index;
        }
    );

    auto it = std::lower_bound(
        mSelectedParts.begin(), mSelectedParts.end(),
        partIndex, [](const SelectedPart& sp, int partIndex) {
            return static_cast<int>(sp.index) < partIndex;
        }
    );

    bool isContained = (it != mSelectedParts.end() && it->index == partIndex);

    if (selected && !isContained)
        mSelectedParts.push_back({ partIndex, mNextSelectionOrder++ });
    else if (!selected && isContained) {
        mSelectedParts.erase(it);
        resetSelectionOrder();
    }
}

void SelectionState::processMultiSelectRequests(ImGuiMultiSelectIO* msIo) {
    for (const ImGuiSelectionRequest& req : msIo->Requests) {
        if (req.Type == ImGuiSelectionRequestType_SetAll) {
            clearSelectedParts();
            if (req.Selected) {
                mSelectedParts.reserve(msIo->ItemsCount);
                for (unsigned i = 0; i < static_cast<unsigned>(msIo->ItemsCount); i++)
                    setBatchPartSelection(i, req.Selected);
            }
        }
        else if (req.Type == ImGuiSelectionRequestType_SetRange) {
            const size_t selectionChanges = req.RangeLastItem - req.RangeFirstItem + 1;

            if (
                selectionChanges == 1 ||
                selectionChanges < (mSelectedParts.size() / 100)
            ) {
                for (int i = req.RangeFirstItem; i <= req.RangeLastItem; i++)
                    setPartSelected(i, req.Selected);
            }
            else
                applyRangeSelectionOrder(req);
        }
    }
}

void SelectionState::deleteSelectedParts(
    ImGuiMultiSelectIO* msIo,
    std::vector<CellAnim::ArrangementPart>& parts,
    int itemCurrentIndexToSelect
) {
    std::vector<CellAnim::ArrangementPart> newParts;
    newParts.reserve(parts.size() - mSelectedParts.size());

    int itemNextIndexToSelect = -1;
    for (size_t i = 0; i < parts.size(); i++) {
        if (!isPartSelected(i))
            newParts.push_back(parts[i]);
        if (itemCurrentIndexToSelect == static_cast<int>(i))
            itemNextIndexToSelect = newParts.size() - 1;
    }
    parts.swap(newParts);

    clearSelectedParts();
    if (itemNextIndexToSelect != -1 && msIo->NavIdSelected)
        setPartSelected(itemNextIndexToSelect, true);
}

int SelectionState::getNextPartIndexAfterDeletion(ImGuiMultiSelectIO* msIo, unsigned partCount) {
    if (mSelectedParts.empty())
        return -1;

    int focusedIndex = static_cast<int>(msIo->NavIdItem);

    if (!msIo->NavIdSelected) {
        msIo->RangeSrcReset = true;
        return focusedIndex;
    }

    for (unsigned index = focusedIndex + 1; index < partCount; index++) {
        if (!isPartSelected(index))
            return index;
    }
    for (int index = std::min(focusedIndex, static_cast<int>(partCount)) - 1; index >= 0; index--) {
        if (!isPartSelected(index))
            return index;
    }

    return -1;
}

void SelectionState::correctSelectedParts() {
    unsigned size = PlayerManager::getInstance().getArrangement().parts.size();

    // Create local clone of selectedParts since setPartSelected
    // will mutate the original
    std::vector<SelectedPart> newSelected;
    newSelected.reserve(mSelectedParts.size());

    for (const auto& sp : mSelectedParts) {
        if (sp.index < size)
            newSelected.push_back(sp);
    }
    mSelectedParts = newSelected;

    resetSelectionOrder();
}
