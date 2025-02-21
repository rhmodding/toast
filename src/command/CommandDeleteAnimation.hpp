#ifndef COMMANDDELETEANIMATION_HPP
#define COMMANDDELETEANIMATION_HPP

#include "BaseCommand.hpp"

#include "../anim/CellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

class CommandDeleteAnimation : public BaseCommand {
public:
    // Constructor: Delete animation by cellanimIndex and animationIndex.
    CommandDeleteAnimation(
        unsigned cellanimIndex, unsigned animationIndex
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex)
    {
        this->animation = this->getAnimation();
    }
    ~CommandDeleteAnimation() = default;

    void Execute() override {
        auto it = this->getAnimations().begin() + this->animationIndex;
        this->getAnimations().erase(it);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        auto it = this->getAnimations().begin() + this->animationIndex;
        this->getAnimations().insert(it, this->animation);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;

    CellAnim::Animation animation;

    std::vector<CellAnim::Animation>& getAnimations() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations;
    }

    CellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDDELETEANIMATION_HPP
