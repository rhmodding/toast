#ifndef COMMANDINSERTANIMATION_HPP
#define COMMANDINSERTANIMATION_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../cellanim/CellAnim.hpp"

class CommandInsertAnimation : public BaseCommand {
public:
    // Constructor: Insert animation by cellanimIndex and animationIndex from animation.
    CommandInsertAnimation(
        unsigned cellanimIndex,
        unsigned animationIndex,
        CellAnim::Animation animation
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex),
        animation(animation)
    {}
    ~CommandInsertAnimation() = default;

    void Execute() override {
        auto& animations = this->getCellAnim()->animations;

        auto it = animations.begin() + this->animationIndex;
        animations.insert(it, this->animation);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        auto& animations = this->getCellAnim()->animations;

        auto it = animations.begin() + this->animationIndex;
        animations.erase(it);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;
    CellAnim::Animation animation;

    std::shared_ptr<CellAnim::CellAnimObject> getCellAnim() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object;
    }
};

#endif // COMMANDINSERTANIMATION_HPP
