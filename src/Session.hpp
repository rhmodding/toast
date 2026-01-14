#ifndef SESSION_HPP
#define SESSION_HPP

#include <utility>

#include <memory>

#include <string>

#include <vector>

#include <deque>

#include "SelectionState.hpp"

#include "texture/TextureEx.hpp"
#include "texture/TextureGroup.hpp"

#include "command/BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

class Session {
public:
    Session() :
        sheets(std::make_shared<TextureGroup<TextureEx>>()),
        arrangementMode(false),
        modified(false),
        type(CellAnim::CELLANIM_TYPE_INVALID),
        currentCellAnim(0)
    {}
    ~Session() = default;

    Session(const Session &) = delete;
    Session &operator=(const Session &) = delete;

    Session(Session &&rhs) noexcept {
        moveFrom(std::move(rhs));
    }

    Session &operator=(Session &&rhs) noexcept {
        if (this != &rhs) {
            moveFrom(std::move(rhs));
        }
        return *this;
    }

public:
    struct CellAnimGroup {
        std::shared_ptr<CellAnim::CellAnimObject> object;
        SelectionState partSelectionState;
        SelectionState keySelectionState;
    };

    void addCommand(std::shared_ptr<BaseCommand> command);

    bool canUndo() const { return !undoQueue.empty(); }
    void undo();

    bool canRedo() const { return !redoQueue.empty(); }
    void redo();

    void clearUndoRedo() {
        undoQueue.clear();
        redoQueue.clear();
    }

    unsigned getCurrentCellAnimIndex() const { return currentCellAnim; }
    void setCurrentCellAnimIndex(unsigned index);

    const CellAnimGroup &getCurrentCellAnim() const {
        return cellanims.at(currentCellAnim);
    }
    CellAnimGroup &getCurrentCellAnim() {
        return cellanims.at(currentCellAnim);
    }

    const std::shared_ptr<TextureEx> &getCurrentCellAnimSheet() const {
        return sheets->getTextureByIndex(
            getCurrentCellAnim().object->getSheetIndex()
        );
    }
    std::shared_ptr<TextureEx> &getCurrentCellAnimSheet() {
        return sheets->getTextureByIndex(
            getCurrentCellAnim().object->getSheetIndex()
        );
    }

    const SelectionState &getPartSelectState() const {
        return getCurrentCellAnim().partSelectionState;
    }
    SelectionState &getPartSelectState() {
        return getCurrentCellAnim().partSelectionState;
    }

    const SelectionState &getKeySelectState() const {
        return getCurrentCellAnim().keySelectionState;
    }
    SelectionState &getKeySelectState() {
        return getCurrentCellAnim().keySelectionState;
    }

private:
    void moveFrom(Session &&rhs) {
        cellanims = std::move(rhs.cellanims);
        sheets = std::move(rhs.sheets);
        resourcePath = std::move(rhs.resourcePath);
        arrangementMode = rhs.arrangementMode;
        modified = rhs.modified;
        type = rhs.type;
        undoQueue = std::move(rhs.undoQueue);
        redoQueue = std::move(rhs.redoQueue);
        currentCellAnim = rhs.currentCellAnim;
    }

public:
    static constexpr unsigned int COMMANDS_MAX = 512;

    std::vector<CellAnimGroup> cellanims;
    std::shared_ptr<TextureGroup<TextureEx>> sheets;

    std::string resourcePath;

    bool arrangementMode;
    bool modified;

    CellAnim::CellAnimType type;

private:
    unsigned currentCellAnim;

    std::deque<std::shared_ptr<BaseCommand>> undoQueue;
    std::deque<std::shared_ptr<BaseCommand>> redoQueue;
};

#endif // SESSION_HPP
