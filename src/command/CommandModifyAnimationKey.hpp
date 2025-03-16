#ifndef COMMANDMODIFYANIMATIONKEY_HPP
#define COMMANDMODIFYANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "../cellanim/CellAnim.hpp"

#include "../SessionManager.hpp"

#include "../AppState.hpp"

class CommandModifyAnimationKey : public BaseCommand {
public:
    // Constructor: Replace key from cellanimIndex, animationIndex and keyIndex by newKey.
    CommandModifyAnimationKey(
        unsigned cellanimIndex, unsigned animationIndex, unsigned keyIndex,
        CellAnim::AnimationKey newKey
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex), keyIndex(keyIndex),
        newKey(newKey)
    {
        this->oldKey = this->getKey();
    }
    ~CommandModifyAnimationKey() = default;

    void Execute() override {
        this->getKey() = this->newKey;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        this->getKey() = this->oldKey;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;
    unsigned keyIndex;

    CellAnim::AnimationKey oldKey;
    CellAnim::AnimationKey newKey;

    CellAnim::AnimationKey& getKey() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex)
            .keys.at(this->keyIndex);
    }

    CellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->getKey().arrangementIndex);
    }
};

#endif // COMMANDMODIFYANIMATIONKEY_HPP
