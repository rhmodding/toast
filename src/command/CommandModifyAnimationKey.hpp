#ifndef COMMANDMODIFYANIMATIONKEY_HPP
#define COMMANDMODIFYANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

class CommandModifyAnimationKey : public BaseCommand {
public:
    // Constructor: Replace key from cellanimIndex, animationIndex and keyIndex by newKey.
    CommandModifyAnimationKey(
        unsigned cellanimIndex, unsigned animationIndex, unsigned keyIndex,
        RvlCellAnim::AnimationKey newKey
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex), keyIndex(keyIndex),
        newKey(newKey)
    {
        this->oldKey = this->getKey();
    }
    ~CommandModifyAnimationKey() = default;

    void Execute() override {
        this->getKey() = this->newKey;

        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

    void Rollback() override {
        this->getKey() = this->oldKey;

        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;
    unsigned keyIndex;

    RvlCellAnim::AnimationKey oldKey;
    RvlCellAnim::AnimationKey newKey;

    RvlCellAnim::AnimationKey& getKey() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex)
            .keys.at(this->keyIndex);
    }

    RvlCellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->getKey().arrangementIndex);
    }
};

#endif // COMMANDMODIFYANIMATIONKEY_HPP
