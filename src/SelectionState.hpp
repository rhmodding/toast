#ifndef SELECTION_STATE_HPP
#define SELECTION_STATE_HPP

#include <vector>

#include "cellanim/CellAnim.hpp"

class SelectionState {
public:
    SelectionState() = default;
    ~SelectionState() = default;

private:
    void resetSelectionOrder();

    void applyRangeSelectionOrder(const ImGuiSelectionRequest& req);

public:
    void clearSelectedParts() {
        mSelectedParts.clear();
        mNextSelectionOrder = 0;
    }

    bool multiplePartsSelected() const {
        return mSelectedParts.size() > 1;
    }
    bool anyPartsSelected() const {
        return !mSelectedParts.empty();
    }
    bool singlePartSelected() const {
        return mSelectedParts.size() == 1;
    }

    bool isPartSelected(unsigned partIndex) const {
        return std::any_of(
            mSelectedParts.begin(), mSelectedParts.end(),
            [partIndex](const SelectedPart& sp) {
                return sp.index == partIndex;
            }
        );
    }
    void setPartSelected(unsigned partIndex, bool selected);

    void setBatchPartSelection(unsigned partIndex, bool selected);

    void processMultiSelectRequests(ImGuiMultiSelectIO* msIo);

    void deleteSelectedParts(
        ImGuiMultiSelectIO* msIo,
        std::vector<CellAnim::ArrangementPart>& parts,
        int itemCurrentIndexToSelect
    );

    int getNextPartIndexAfterDeletion(ImGuiMultiSelectIO* msIo, unsigned partCount);

    void correctSelectedParts();

public:
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
    std::vector<SelectedPart> mSelectedParts;

    int mNextSelectionOrder { 0 };
};

#endif // SELECTION_STATE_HPP
