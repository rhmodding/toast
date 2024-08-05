#ifndef COMMANDDELETEMANYANIMATIONKEYS_HPP
#define COMMANDDELETEMANYANIMATIONKEYS_HPP

#include "BaseCommand.hpp"

#include "../anim/RvlCellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

// TODO: Replace with CommandModifyAnimation

class CommandDeleteManyAnimationKeys : public BaseCommand {
public:
    // Constructor: Delete keys by cellanimIndex, animationIndex, keyIndexStart and keyIndexEnd.
    CommandDeleteManyAnimationKeys(
        uint16_t cellanimIndex, uint16_t animationIndex,
        uint32_t keyIndexStart, uint32_t keyIndexEnd
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex),
        keyIndexStart(keyIndexStart), keyIndexEnd(keyIndexEnd)
    {
        RvlCellAnim::Animation& animation = this->getAnimation();

        this->keys = std::vector<RvlCellAnim::AnimationKey>(
            animation.keys.begin() + this->keyIndexStart,
            animation.keys.begin() + this->keyIndexEnd
        );
    }

    void Execute() override {
        RvlCellAnim::Animation& animation = this->getAnimation();

        animation.keys.erase(
            animation.keys.begin() + this->keyIndexStart,
            animation.keys.begin() + this->keyIndexEnd
        );

        PlayerManager::getInstance().clampCurrentKeyIndex();
    }

    void Rollback() override {
        RvlCellAnim::Animation& animation = this->getAnimation();

        animation.keys.insert(
            animation.keys.begin() + this->keyIndexStart,
            this->keys.begin(),
            this->keys.end()
        );

        PlayerManager::getInstance().clampCurrentKeyIndex();
    }

private:
    uint16_t cellanimIndex;
    uint16_t animationIndex;

    uint32_t keyIndexStart;
    uint32_t keyIndexEnd;

    std::vector<RvlCellAnim::AnimationKey> keys;

    RvlCellAnim::Animation& getAnimation() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->animations.at(this->animationIndex);
    }
};

#endif // COMMANDDELETEMANYANIMATIONKEYS_HPP
