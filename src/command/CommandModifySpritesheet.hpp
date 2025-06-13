#ifndef COMMANDMODIFYSPRITESHEET_HPP
#define COMMANDMODIFYSPRITESHEET_HPP

#include "BaseCommand.hpp"

#include "manager/SessionManager.hpp"

#include "texture/Texture.hpp"

class CommandModifySpritesheet : public BaseCommand {
public:
    // Constructor: Replace spritesheet from sheetIndex by newSheet (shared ownership).
    CommandModifySpritesheet(
        unsigned sheetIndex, std::shared_ptr<TextureEx> newSheet
    ) :
        mSheetIndex(sheetIndex),
        mNewSheet(std::move(newSheet)), mOldSheet(getSheet())
    {}
    ~CommandModifySpritesheet() = default;

    void Execute() override {
        getSheet() = mNewSheet;

        Session* currentSession = SessionManager::getInstance().getCurrentSession();

        currentSession->getCurrentCellAnim().object->setSheetWidth(mNewSheet->getWidth());
        currentSession->getCurrentCellAnim().object->setSheetHeight(mNewSheet->getHeight());
        currentSession->getCurrentCellAnim().object->setUsePalette(TPL::getImageFormatPaletted(
            mNewSheet->getTPLOutputFormat()
        ));

        currentSession->modified = true;
    }

    void Rollback() override {
        getSheet() = mOldSheet;

        Session* currentSession = SessionManager::getInstance().getCurrentSession();

        currentSession->getCurrentCellAnim().object->setSheetWidth(mOldSheet->getWidth());
        currentSession->getCurrentCellAnim().object->setSheetHeight(mOldSheet->getHeight());
        currentSession->getCurrentCellAnim().object->setUsePalette(TPL::getImageFormatPaletted(
            mOldSheet->getTPLOutputFormat()
        ));

        currentSession->modified = true;
    }

private:
    unsigned mSheetIndex;

    std::shared_ptr<TextureEx> mNewSheet;
    std::shared_ptr<TextureEx> mOldSheet;

    std::shared_ptr<TextureEx>& getSheet() {
        return SessionManager::getInstance().getCurrentSession()
            ->sheets->getTextureByIndex(mSheetIndex);
    }
};


#endif // COMMANDMODIFYSPRITESHEET_HPP
