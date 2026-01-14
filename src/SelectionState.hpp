#ifndef SELECTION_STATE_HPP
#define SELECTION_STATE_HPP

#include <vector>

#include <imgui.h>

#include "cellanim/CellAnim.hpp"

class SelectionState {
public:
    SelectionState() = default;
    ~SelectionState() = default;

private:
    void resetSelectionOrder();

    void applyRangeSelectionOrder(const ImGuiSelectionRequest &req);

public:
    void clearSelectedElements() {
        mSelected.clear();
        mNextSelectionOrder = 0;
    }

    bool multipleSelected() const {
        return mSelected.size() > 1;
    }
    bool anySelected() const {
        return !mSelected.empty();
    }
    bool singleSelected() const {
        return mSelected.size() == 1;
    }

    bool checkSelected(unsigned elementIndex) const {
        return std::any_of(
            mSelected.begin(), mSelected.end(),
            [elementIndex](const Selection &sel) {
                return sel.index == elementIndex;
            }
        );
    }
    void setSelected(unsigned elementIndex, bool selected);

    void setBatchSelection(unsigned elementIndex, bool selected);

    void processMultiSelectRequests(ImGuiMultiSelectIO *msIo);

    template <typename T>
    void deleteAllSelected(
        ImGuiMultiSelectIO *msIo,
        std::vector<T> &elements,
        int itemCurrentIndexToSelect
    ) {
        std::vector<T> newElements;
        newElements.reserve(elements.size() - mSelected.size());

        int itemNextIndexToSelect = -1;
        for (size_t i = 0; i < elements.size(); i++) {
            if (!checkSelected(i)) {
                newElements.push_back(elements[i]);
            }
            if (itemCurrentIndexToSelect == static_cast<int>(i)) {
                itemNextIndexToSelect = newElements.size() - 1;
            }
        }
        elements.swap(newElements);

        clearSelectedElements();
        if (itemNextIndexToSelect != -1 && msIo->NavIdSelected) {
            setSelected(itemNextIndexToSelect, true);
        }
    }

    int getNextElementIndexAfterDel(ImGuiMultiSelectIO *msIo, unsigned elementCount);

    void validateSelection();

    void sortDescending() {
        std::sort(
            mSelected.begin(), mSelected.end(),
            [](const Selection &a, const Selection &b) {
                return a.index > b.index;
            }
        );
    }
    void sortAscending() {
        std::sort(
            mSelected.begin(), mSelected.end(),
            [](const Selection &a, const Selection &b) {
                return a.index < b.index;
            }
        );
    }

public:
    struct Selection {
        unsigned index;
        int selectionOrder;

        bool operator==(const Selection &other) const {
            return
                index == other.index &&
                selectionOrder == other.selectionOrder;
        }
        bool operator!=(const Selection &other) const {
            return !(*this == other);
        }
    };
    std::vector<Selection> mSelected;

    int mNextSelectionOrder { 0 };
};

#endif // SELECTION_STATE_HPP
