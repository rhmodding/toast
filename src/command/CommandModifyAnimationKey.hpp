#ifndef COMMANDMODIFYANIMATIONKEY_HPP
#define COMMANDMODIFYANIMATIONKEY_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../AppState.hpp"
#include "../SessionManager.hpp"

#include <algorithm>

class CommandModifyAnimationKey : public BaseCommand {
public:
    // Constructor: Replace key from cellanimIndex, animationIndex and keyIndex by newKey.
    CommandModifyAnimationKey(
        uint16_t cellanimIndex, uint16_t animationIndex, uint32_t keyIndex,
        RvlCellAnim::AnimationKey newKey
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex), keyIndex(keyIndex),
        newKey(newKey)
    {
        this->oldKey = this->getKey();
    }

    void Execute() override {
        this->getKey() = this->newKey;

        GET_APP_STATE;
        appState.selectedPart = std::clamp<int32_t>(
            appState.selectedPart,
            -1,
            this->getArrangement().parts.size() - 1
        );
    }

    void Rollback() override {
        this->getKey() = this->oldKey;
    }

private:
    uint16_t cellanimIndex;
    uint16_t animationIndex;
    uint32_t keyIndex;

    RvlCellAnim::AnimationKey oldKey;
    RvlCellAnim::AnimationKey newKey;

    RvlCellAnim::AnimationKey& getKey() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex)
            ->animations.at(this->animationIndex)
            .keys.at(this->keyIndex);
    }

    RvlCellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex)
            ->arrangements.at(this->getKey().arrangementIndex);
    }
};

#endif // COMMANDMODIFYANIMATIONKEY_HPP