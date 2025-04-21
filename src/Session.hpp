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

    Session(Session&& other) noexcept :
        cellanims(std::move(other.cellanims)),
        sheets(std::move(other.sheets)),
        resourcePath(std::move(other.resourcePath)),
        arrangementMode(other.arrangementMode),
        modified(other.modified),
        type(other.type),
        undoQueue(std::move(other.undoQueue)),
        redoQueue(std::move(other.redoQueue)),
        currentCellAnim(other.currentCellAnim)
    {}

    Session& operator=(Session&& other) noexcept {
        if (this != &other) {
            this->cellanims = std::move(other.cellanims);
            this->sheets = std::move(other.sheets);
            this->resourcePath = std::move(other.resourcePath);
            this->arrangementMode = other.arrangementMode;
            this->modified = other.modified;
            this->type = other.type;
            this->undoQueue = std::move(other.undoQueue);
            this->redoQueue = std::move(other.redoQueue);
            this->currentCellAnim = other.currentCellAnim;
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

    bool canUndo() const { return !this->undoQueue.empty(); }
    void undo();

    bool canRedo() const { return !this->redoQueue.empty(); }
    void redo();

    void clearUndoRedo() {
        this->undoQueue.clear();
        this->redoQueue.clear();
    };

    unsigned getCurrentCellAnimIndex() const { return this->currentCellAnim; }
    void setCurrentCellAnimIndex(unsigned index);

    CellAnimGroup& getCurrentCellAnim() {
        return this->cellanims.at(this->currentCellAnim);
    }

    std::shared_ptr<Texture>& getCurrentCellAnimSheet() {
        return this->sheets->getTextureByIndex(
            this->cellanims.at(this->currentCellAnim).object->sheetIndex
        );
    }

    SelectionState& getCurrentSelectionState() {
        return this->cellanims.at(this->currentCellAnim).selectionState;
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
