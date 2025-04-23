#ifndef COMMANDMODIFYSPRITESHEET_HPP
#define COMMANDMODIFYSPRITESHEET_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"

#include "../texture/Texture.hpp"

class CommandModifySpritesheet : public BaseCommand {
public:
    // Constructor: Replace spritesheet from sheetIndex by newSheet (shared ownership).
    CommandModifySpritesheet(
        unsigned sheetIndex, std::shared_ptr<Texture> newSheet
    ) :
        sheetIndex(sheetIndex),
        newSheet(std::move(newSheet)), oldSheet(this->getSheet())
    {}
    ~CommandModifySpritesheet() = default;

    void Execute() override {
        this->getSheet() = this->newSheet;

        Session* currentSession = SessionManager::getInstance().getCurrentSession();

        currentSession->getCurrentCellAnim().object->sheetW = this->newSheet->getWidth();
        currentSession->getCurrentCellAnim().object->sheetH = this->newSheet->getHeight();
        currentSession->modified = true;
    }

    void Rollback() override {
        this->getSheet() = this->oldSheet;

        Session* currentSession = SessionManager::getInstance().getCurrentSession();

        currentSession->getCurrentCellAnim().object->sheetW = this->oldSheet->getWidth();
        currentSession->getCurrentCellAnim().object->sheetH = this->oldSheet->getHeight();

        currentSession->modified = true;
    }

private:
    unsigned sheetIndex;

    std::shared_ptr<Texture> newSheet;
    std::shared_ptr<Texture> oldSheet;

    std::shared_ptr<Texture>& getSheet() {
        return SessionManager::getInstance().getCurrentSession()
            ->sheets->getTextureByIndex(this->sheetIndex);
    }
};


#endif // COMMANDMODIFYSPRITESHEET_HPP
