#ifndef COMMANDMODIFYANIMATIONNAME_HPP
#define COMMANDMODIFYANIMATIONNAME_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"

class CommandModifyAnimationName : public BaseCommand {
public:
    // Constructor: Modify animation name from cellanimIndex, animationIndex and name.
    CommandModifyAnimationName(
        uint16_t cellanimIndex, uint16_t animationIndex,
        char* name
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex),
        newName(name)
    {
        if (this->getAnimationNames().find(animationIndex) != this->getAnimationNames().end()) {
            this->oldName = this->getAnimationNames()[animationIndex];
        }
    }

    void Execute() override {
        if (
            this->newName.empty() &&
            this->getAnimationNames().find(animationIndex) != this->getAnimationNames().end()
        ) {
            this->getAnimationNames().erase(animationIndex);
            return;
        }

        this->getAnimationNames()[animationIndex] = this->newName;
    }

    void Rollback() override {
        if (
            this->oldName.empty() &&
            this->getAnimationNames().find(animationIndex) != this->getAnimationNames().end()
        ) {
            this->getAnimationNames().erase(animationIndex);
            return;
        }

        this->getAnimationNames()[animationIndex] = this->oldName;
    }

private:
    uint16_t cellanimIndex;
    uint16_t animationIndex;

    std::string oldName;
    std::string newName;

    std::unordered_map<uint16_t, std::string>& getAnimationNames() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).
            animNames;
    }
};

#endif // COMMANDMODIFYANIMATIONNAME_HPP
