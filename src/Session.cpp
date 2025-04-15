#include "Session.hpp"

#include "PlayerManager.hpp"

#include "Logging.hpp"

void Session::addCommand(std::shared_ptr<BaseCommand> command) {
    command->Execute();
    if (this->undoQueue.size() == COMMANDS_MAX)
        this->undoQueue.pop_front();

    this->undoQueue.push_back(command);
    this->redoQueue.clear();
}

void Session::undo() {
    if (this->undoQueue.empty())
        return;

    auto command = undoQueue.back();
    this->undoQueue.pop_back();

    command->Rollback();

    this->redoQueue.push_back(command);

    this->modified = true;
}

void Session::redo() {
    if (this->redoQueue.empty())
        return;

    auto command = redoQueue.back();
    this->redoQueue.pop_back();

    command->Execute();

    if (this->undoQueue.size() == COMMANDS_MAX)
        this->undoQueue.pop_front();

    this->undoQueue.push_back(command);

    this->modified = true;
}

void Session::setCurrentCellanimIndex(unsigned index) {
    if (index >= this->cellanims.size())
        return;

    this->currentCellanim = index;
    this->sheets->setBaseTextureIndex(
        this->cellanims.at(index).object->sheetIndex
    );

    PlayerManager::getInstance().correctState();

    const std::string& cellanimName = this->cellanims.at(this->currentCellanim).name;

    Logging::info <<
        "[Session::setCurrentCellanimIndex] Selected cellanim no. " << index+1 << " (\"" << cellanimName << "\")." << std::endl;
}
