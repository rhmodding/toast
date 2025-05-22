#ifndef SESSION_HPP
#define SESSION_HPP

#include <utility>

#include <memory>

#include <string>

#include <vector>

#include <deque>

#include "SelectionState.hpp"

#include "texture/Texture.hpp"
#include "texture/TextureGroup.hpp"

#include "command/BaseCommand.hpp"

#include "cellanim/CellAnim.hpp"

class Session {
public:
    Session() = default;
    ~Session() = default;

    Session(Session&& rhs) noexcept :
        cellanims(std::move(rhs.cellanims)),
        sheets(std::move(rhs.sheets)),
        resourcePath(std::move(rhs.resourcePath)),
        arrangementMode(rhs.arrangementMode),
        modified(rhs.modified),
        type(rhs.type),
        undoQueue(std::move(rhs.undoQueue)),
        redoQueue(std::move(rhs.redoQueue)),
        currentCellAnim(rhs.currentCellAnim)
    {}

    Session& operator=(Session&& rhs) noexcept {
        if (this != &rhs) {
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
        return *this;
    }

public:
    struct CellAnimGroup {
        std::shared_ptr<CellAnim::CellAnimObject> object;
        std::string name;

        SelectionState selectionState;
    };

public:
    void addCommand(std::shared_ptr<BaseCommand> command);

    bool canUndo() const { return !undoQueue.empty(); }
    void undo();

    bool canRedo() const { return !redoQueue.empty(); }
    void redo();

    void clearUndoRedo() {
        undoQueue.clear();
        redoQueue.clear();
    };

    unsigned getCurrentCellAnimIndex() const { return currentCellAnim; }
    void setCurrentCellAnimIndex(unsigned index);

    CellAnimGroup& getCurrentCellAnim() {
        return cellanims.at(currentCellAnim);
    }

    std::shared_ptr<Texture>& getCurrentCellAnimSheet() {
        return sheets->getTextureByIndex(
            cellanims.at(currentCellAnim).object->getSheetIndex()
        );
    }

    SelectionState& getCurrentSelectionState() {
        return cellanims.at(currentCellAnim).selectionState;
    }

public:
    static constexpr unsigned int COMMANDS_MAX = 512;

    std::vector<CellAnimGroup> cellanims;
    std::shared_ptr<TextureGroup> sheets { std::make_shared<TextureGroup>() };

    std::string resourcePath;

    bool arrangementMode { false };

    bool modified { false };

    CellAnim::CellAnimType type { CellAnim::CELLANIM_TYPE_INVALID };

private:
    std::deque<std::shared_ptr<BaseCommand>> undoQueue;
    std::deque<std::shared_ptr<BaseCommand>> redoQueue;

    unsigned currentCellAnim { 0 };
};

#endif // SESSION_HPP
