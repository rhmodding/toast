#include "SelectionState.hpp"

#include <imgui.h>

#include <cstddef>

#include <algorithm>

#include "manager/PlayerManager.hpp"

void SelectionState::resetSelectionOrder() {
    mNextSelectionOrder = 0;
    for (auto &sel : mSelected)
        sel.selectionOrder = mNextSelectionOrder++;
}

void SelectionState::applyRangeSelectionOrder(const ImGuiSelectionRequest& req) {
    if (req.RangeDirection > 0) {
        for (int i = req.RangeLastItem; i <= req.RangeFirstItem; i++) {
            setBatchSelection(i, req.Selected);
            //selectionOrder++;
        }
    }
    else { //if (req.RangeDirection < 0) {
        for (int i = req.RangeFirstItem; i >= req.RangeLastItem; i--) {
            setBatchSelection(i, req.Selected);
            //selectionOrder--;
        }
    }
}

void SelectionState::setSelected(unsigned elementIndex, bool selected) {
    auto it = std::find_if(
        mSelected.begin(), mSelected.end(),
        [elementIndex](const Selection &sel) {
            return sel.index == elementIndex;
        }
    );

    if (selected) {
        if (it == mSelected.end())
            mSelected.push_back({ elementIndex, mNextSelectionOrder++ });
    }
    else if (it != mSelected.end()) {
        mSelected.erase(it);
        resetSelectionOrder();
    }
}

void SelectionState::setBatchSelection(unsigned elementIndex, bool selected) {
    std::sort(
        mSelected.begin(), mSelected.end(),
        [](const Selection &a, const Selection &b) {
            return a.index < b.index;
        }
    );

    auto it = std::lower_bound(
        mSelected.begin(), mSelected.end(),
        elementIndex, [](const Selection &sel, int elementIndex) {
            return static_cast<int>(sel.index) < elementIndex;
        }
    );

    bool isContained = (it != mSelected.end() && it->index == elementIndex);

    if (selected && !isContained)
        mSelected.push_back({ elementIndex, mNextSelectionOrder++ });
    else if (!selected && isContained) {
        mSelected.erase(it);
        resetSelectionOrder();
    }
}

void SelectionState::processMultiSelectRequests(ImGuiMultiSelectIO* msIo) {
    for (const ImGuiSelectionRequest& req : msIo->Requests) {
        if (req.Type == ImGuiSelectionRequestType_SetAll) {
            clearSelectedElements();
            if (req.Selected) {
                mSelected.reserve(msIo->ItemsCount);
                for (unsigned i = 0; i < static_cast<unsigned>(msIo->ItemsCount); i++)
                    setBatchSelection(i, req.Selected);
            }
        }
        else if (req.Type == ImGuiSelectionRequestType_SetRange) {
            const size_t selectionChanges = req.RangeLastItem - req.RangeFirstItem + 1;

            if (
                selectionChanges == 1 ||
                selectionChanges < (mSelected.size() / 100)
            ) {
                for (int i = req.RangeFirstItem; i <= req.RangeLastItem; i++)
                    setSelected(i, req.Selected);
            }
            else
                applyRangeSelectionOrder(req);
        }
    }
}

int SelectionState::getNextElementIndexAfterDel(ImGuiMultiSelectIO* msIo, unsigned elementCount) {
    if (mSelected.empty())
        return -1;

    int focusedIndex = static_cast<int>(msIo->NavIdItem);

    if (!msIo->NavIdSelected) {
        msIo->RangeSrcReset = true;
        return focusedIndex;
    }

    for (unsigned index = focusedIndex + 1; index < elementCount; index++) {
        if (!checkSelected(index))
            return index;
    }
    for (int index = std::min(focusedIndex, static_cast<int>(elementCount)) - 1; index >= 0; index--) {
        if (!checkSelected(index))
            return index;
    }

    return -1;
}

void SelectionState::validateSelection() {
    unsigned size = PlayerManager::getInstance().getArrangement().parts.size();

    // Create local clone of selectedParts since setSelected
    // will mutate the original
    std::vector<Selection> newSelected;
    newSelected.reserve(mSelected.size());

    for (const auto& sp : mSelected) {
        if (sp.index < size)
            newSelected.push_back(sp);
    }
    mSelected = newSelected;

    resetSelectionOrder();
}
