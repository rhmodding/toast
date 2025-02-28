#ifndef COMMANDMODIFYANIMATION_HPP
#define COMMANDMODIFYANIMATION_HPP

#include "BaseCommand.hpp"

#include "../anim/CellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../AppState.hpp"

class CommandModifyAnimation : public BaseCommand {
public:
    // Constructor: Replace animation from cellanimIndex and animationIndex by newAnimation.
    CommandModifyAnimation(
        unsigned cellanimIndex, unsigned animationIndex,
        CellAnim::Animation newAnimation
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex),
        newAnimation(newAnimation)
    {
        this->oldAnimation = this->getAnimation();
    }
    ~CommandModifyAnimation() = default;

    void Execute() override {
        this->getAnimation() = this->newAnimation;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        this->getAnimation() = this->oldAnimation;

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;

    CellAnim::Animation oldAnimation;
    CellAnim::Animation newAnimation;

    CellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDMODIFYANIMATION_HPP
