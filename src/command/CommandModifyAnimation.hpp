#ifndef COMMANDMODIFYANIMATION_HPP
#define COMMANDMODIFYANIMATION_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../AppState.hpp"

class CommandModifyAnimation : public BaseCommand {
public:
    // Constructor: Replace animation from cellanimIndex and animationIndex by newAnimation.
    CommandModifyAnimation(
        uint16_t cellanimIndex, uint16_t animationIndex,
        RvlCellAnim::Animation newAnimation
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex),
        newAnimation(newAnimation)
    {
        this->oldAnimation = this->getAnimation();
    }

    void Execute() override {
        this->getAnimation() = this->newAnimation;

        PlayerManager::getInstance().clampCurrentKeyIndex();
        AppState::getInstance().correctSelectedPart();
    }

    void Rollback() override {
        this->getAnimation() = this->oldAnimation;

        PlayerManager::getInstance().clampCurrentKeyIndex();
        AppState::getInstance().correctSelectedPart();
    }

private:
    uint16_t cellanimIndex;
    uint16_t animationIndex;

    RvlCellAnim::Animation oldAnimation;
    RvlCellAnim::Animation newAnimation;

    RvlCellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDMODIFYANIMATION_HPP
