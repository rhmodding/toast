#ifndef COMMANDDELETEANIMATION_HPP
#define COMMANDDELETEANIMATION_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

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

        PlayerManager::getInstance().clampCurrentKeyIndex();
        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

    void Rollback() override {
        auto it = this->getAnimations().begin() + this->animationIndex;
        this->getAnimations().insert(it, this->animation);

        PlayerManager::getInstance().clampCurrentKeyIndex();
        AppState::getInstance().correctSelectedParts();

        SessionManager::getInstance().getCurrentSessionModified() = true;
    }

private:
    uint16_t cellanimIndex;
    uint16_t animationIndex;

    RvlCellAnim::Animation animation;

    std::vector<RvlCellAnim::Animation>& getAnimations() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations;
    }

    RvlCellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDDELETEANIMATION_HPP
