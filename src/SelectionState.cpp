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
int SelectionState::getMatchingRegionPartIndex(
    const CellAnim::ArrangementPart& part,
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

void SelectionState::resetSelectionOrder() {
    this->nextSelectionOrder = 0;
    for (auto& sp : this->selectedParts)
        sp.selectionOrder = this->nextSelectionOrder++;
}

void SelectionState::applyRangeSelectionOrder(const ImGuiSelectionRequest& req) {
    if (req.RangeDirection > 0) {
        for (int i = req.RangeLastItem; i <= req.RangeFirstItem; i++) {
            this->setBatchPartSelection(i, req.Selected);
            //selectionOrder++;
        }
    }
    else { //if (req.RangeDirection < 0) {
        for (int i = req.RangeFirstItem; i >= req.RangeLastItem; i--) {
            this->setBatchPartSelection(i, req.Selected);
            //selectionOrder--;
        }
    }
}

void SelectionState::setPartSelected(unsigned partIndex, bool selected) {
    auto it = std::find_if(
        this->selectedParts.begin(), this->selectedParts.end(),
        [partIndex](const SelectedPart& sp) {
            return sp.index == partIndex;
        }
    );

    if (selected) {
        if (it == this->selectedParts.end())
            this->selectedParts.push_back({ partIndex, this->nextSelectionOrder++ });
    }
    else if (it != this->selectedParts.end()) {
        this->selectedParts.erase(it);
        this->resetSelectionOrder();
    }
}

void SelectionState::setBatchPartSelection(unsigned partIndex, bool selected) {
    std::sort(
        this->selectedParts.begin(), this->selectedParts.end(),
        [](const SelectedPart& a, const SelectedPart& b) {
            return a.index < b.index;
        }
    );

    auto it = std::lower_bound(
        this->selectedParts.begin(), this->selectedParts.end(),
        partIndex, [](const SelectedPart& sp, int partIndex) {
            return static_cast<int>(sp.index) < partIndex;
        }
    );

    bool isContained = (it != selectedParts.end() && it->index == partIndex);

    if (selected && !isContained)
        this->selectedParts.push_back({ partIndex, this->nextSelectionOrder++ });
    else if (!selected && isContained) {
        this->selectedParts.erase(it);
        this->resetSelectionOrder();
    }
}

void SelectionState::processMultiSelectRequests(ImGuiMultiSelectIO* msIo) {
    for (const ImGuiSelectionRequest& req : msIo->Requests) {
        if (req.Type == ImGuiSelectionRequestType_SetAll) {
            this->clearSelectedParts();
            if (req.Selected) {
                this->selectedParts.reserve(msIo->ItemsCount);
                for (unsigned i = 0; i < static_cast<unsigned>(msIo->ItemsCount); i++, nextSelectionOrder++) // TODO: fnuy
                    this->setBatchPartSelection(i, req.Selected);
            }
        }
        else if (req.Type == ImGuiSelectionRequestType_SetRange) {
            const size_t selectionChanges = req.RangeLastItem - req.RangeFirstItem + 1;

            if (
                selectionChanges == 1 ||
                selectionChanges < (this->selectedParts.size() / 100)
            ) {
                for (int i = req.RangeFirstItem; i <= req.RangeLastItem; i++)
                    this->setPartSelected(i, req.Selected);
            }
            else
                this->applyRangeSelectionOrder(req);
        }
    }
}

void SelectionState::deleteSelectedParts(
    ImGuiMultiSelectIO* msIo,
    std::vector<CellAnim::ArrangementPart>& parts,
    int itemCurrentIndexToSelect
) {
    std::vector<CellAnim::ArrangementPart> newParts;
    newParts.reserve(parts.size() - this->selectedParts.size());

    int itemNextIndexToSelect = -1;
    for (unsigned i = 0; i < parts.size(); i++) {
        if (!this->isPartSelected(i))
            newParts.push_back(parts[i]);
        if (itemCurrentIndexToSelect == static_cast<int>(i))
            itemNextIndexToSelect = newParts.size() - 1;
    }
    parts.swap(newParts);

    this->clearSelectedParts();
    if (itemNextIndexToSelect != -1 && msIo->NavIdSelected)
        this->setPartSelected(itemNextIndexToSelect, true);
}

int SelectionState::getNextPartIndexAfterDeletion(ImGuiMultiSelectIO* msIo, unsigned partCount) {
    if (this->selectedParts.empty())
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

void SelectionState::correctSelectedParts() {
    unsigned size = PlayerManager::getInstance().getArrangement().parts.size();

    // Create local clone of selectedParts since setPartSelected
    // will mutate the original
    std::vector<SelectedPart> newSelected;
    newSelected.reserve(this->selectedParts.size());

    for (const auto& sp : this->selectedParts) {
        if (sp.index < size)
            newSelected.push_back(sp);
    }
    this->selectedParts = newSelected;

    this->resetSelectionOrder();
}
