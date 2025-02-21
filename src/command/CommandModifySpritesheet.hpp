#ifndef COMMANDMODIFYSPRITESHEET_HPP
#define COMMANDMODIFYSPRITESHEET_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"

#include "../texture/Texture.hpp"

/*
class CommandModifySpritesheet : public BaseCommand {
public:
    // Constructor: Replace spritesheet from sheetIndex by newImageData, newImageWidth & newImageHeight.
    // Note: newImageData is not used directly and is instead copied.
    CommandModifySpritesheet(
        unsigned sheetIndex,
        unsigned char* newImageData, unsigned newImageWidth, unsigned newImageHeight
    ) :
        sheetIndex(sheetIndex),
        newImageWidth(newImageWidth), newImageHeight(newImageHeight)
    {
        unsigned dataSize = newImageWidth * newImageHeight * 4;

        this->newImageData = std::make_unique<unsigned char[]>(dataSize);
        memcpy(this->newImageData.get(), newImageData, dataSize);

        auto& sheet = this->getSheet();

        this->oldImageWidth  = sheet->getWidth();
        this->oldImageHeight = sheet->getHeight();

        dataSize = this->oldImageWidth * this->oldImageHeight * 4;

        this->oldImageData = std::make_unique<unsigned char[]>(dataSize);
    }

    void Execute() override {
        this->getSheet()->LoadRGBA32(this->newImageData.get(), this->newImageWidth, this->newImageHeight);
    }

    void Rollback() override {
        this->getSheet()->LoadRGBA32(this->oldImageData.get(), this->oldImageWidth, this->oldImageHeight);
    }

private:
    unsigned sheetIndex;
    
    std::unique_ptr<unsigned char[]> oldImageData;
    unsigned oldImageWidth, oldImageHeight;

    std::unique_ptr<unsigned char[]> newImageData;
    unsigned newImageWidth, newImageHeight;

    std::shared_ptr<Texture>& getSheet() {
        return SessionManager::getInstance().getCurrentSession()->sheets.at(this->sheetIndex);
    }
};
*/

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

        currentSession->getCurrentCellanim().object->sheetW = this->newSheet->getWidth();
        currentSession->getCurrentCellanim().object->sheetH = this->newSheet->getHeight();
        currentSession->modified = true;
    }

    void Rollback() override {
        this->getSheet() = this->oldSheet;
        
        Session* currentSession = SessionManager::getInstance().getCurrentSession();

        currentSession->getCurrentCellanim().object->sheetW = this->oldSheet->getWidth();
        currentSession->getCurrentCellanim().object->sheetH = this->oldSheet->getHeight();

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
