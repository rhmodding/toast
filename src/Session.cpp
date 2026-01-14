#include "Session.hpp"

#include "manager/PlayerManager.hpp"

#include "Logging.hpp"

void Session::addCommand(std::shared_ptr<BaseCommand> command) {
    command->execute();
    if (this->undoQueue.size() >= COMMANDS_MAX)
        this->undoQueue.pop_front();

    this->undoQueue.push_back(command);
    this->redoQueue.clear();
}

void Session::undo() {
    if (this->undoQueue.empty())
        return;

    auto command = undoQueue.back();
    this->undoQueue.pop_back();

    command->rollback();

    this->redoQueue.push_back(command);

    this->modified = true;
}

void Session::redo() {
    if (this->redoQueue.empty())
        return;

    auto command = redoQueue.back();
    this->redoQueue.pop_back();

    command->execute();

    if (this->undoQueue.size() == COMMANDS_MAX)
        this->undoQueue.pop_front();

    this->undoQueue.push_back(command);

    this->modified = true;
}

void Session::setCurrentCellAnimIndex(unsigned index) {
    if (index >= this->cellanims.size())
        return;

    this->currentCellAnim = index;
    const auto& cellanim = this->cellanims.at(index);

    this->sheets->setBaseTextureIndex(cellanim.object->getSheetIndex());
    this->sheets->setVaryingEnabled(cellanim.object->getUsePalette());

    PlayerManager::getInstance().validateState();

    Logging::info(
        "[Session::setCurrentCellAnimIndex] Selected cellanim no. {} (\"{}\").",
        index+1,
        cellanim.object->getName()
    );
}
