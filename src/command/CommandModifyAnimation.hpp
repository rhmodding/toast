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
        unsigned cellanimIndex, unsigned animationIndex,
        RvlCellAnim::Animation newAnimation
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex),
        newAnimation(newAnimation)
    {
        this->oldAnimation = this->getAnimation();
    }
    ~CommandModifyAnimation() = default;

    void Execute() override {
        this->getAnimation() = this->newAnimation;

        GET_APP_STATE;

        if (!appState.getArrangementMode())
            PlayerManager::getInstance().clampCurrentKeyIndex();
        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

    void Rollback() override {
        this->getAnimation() = this->oldAnimation;

        GET_APP_STATE;

        if (!appState.getArrangementMode())
            PlayerManager::getInstance().clampCurrentKeyIndex();
        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;

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
