#ifndef COMMANDMODIFYANIMATIONNAME_HPP
#define COMMANDMODIFYANIMATIONNAME_HPP

#include "BaseCommand.hpp"

#include <string_view>

#include "../SessionManager.hpp"

class CommandModifyAnimationName : public BaseCommand {
public:
    // Constructor: Modify animation name from cellanimIndex, animationIndex and name.
    CommandModifyAnimationName(
        unsigned cellanimIndex, unsigned animationIndex,
        std::string_view name
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex),
        newName(name)
    {
        this->oldName = this->getAnimation().name;
    }
    ~CommandModifyAnimationName() = default;

    void Execute() override {
        this->getAnimation().name = this->newName;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        this->getAnimation().name = this->oldName;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;

    std::string oldName;
    std::string newName;

    CellAnim::Animation& getAnimation() {
        return SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(cellanimIndex).object
            ->animations.at(animationIndex);
    }
};

#endif // COMMANDMODIFYANIMATIONNAME_HPP
