#ifndef SELECTIONSTATE_HPP
#define SELECTIONSTATE_HPP

#include <vector>

#include "cellanim/CellAnim.hpp"

class SelectionState {
public:
    SelectionState() = default;
    ~SelectionState() = default;

    static int getMatchingNamePartIndex(
        const CellAnim::ArrangementPart& part,
        const CellAnim::Arrangement& arrangement
    );
    static int getMatchingRegionPartIndex(
        const CellAnim::ArrangementPart& part,
        const CellAnim::Arrangement& arrangement
    );

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

    std::vector<SelectedPart> selectedParts;
    int nextSelectionOrder { 0 };

private:
    void resetSelectionOrder();

    void applyRangeSelectionOrder(const ImGuiSelectionRequest& req);

public:
    void clearSelectedParts() {
        this->selectedParts.clear();
        this->nextSelectionOrder = 0;
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
};

#endif // SELECTIONSTATE_HPP
