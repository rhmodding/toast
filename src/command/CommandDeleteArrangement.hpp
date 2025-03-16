#ifndef COMMANDDELETEARRANGEMENT_HPP
#define COMMANDDELETEARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include "../anim/CellAnim.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

class CommandDeleteArrangement : public BaseCommand {
public:
    // Constructor: Delete arrangement by cellanimIndex and arrangementIndex.
    CommandDeleteArrangement(
        unsigned cellanimIndex, unsigned arrangementIndex
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex)
    {
        this->arrangement = this->getArrangement();
    }
    ~CommandDeleteArrangement() = default;

    void Execute() override {
        auto& arrangements = this->getArrangements();

        auto it = arrangements.begin() + this->arrangementIndex;
        arrangements.erase(it);

        for (
            auto& animation :
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object->animations
        ) for (auto& key : animation.keys) {
            if (key.arrangementIndex > arrangementIndex)
                key.arrangementIndex--;
            else if (key.arrangementIndex == arrangementIndex)
                key.arrangementIndex = 0;
        }

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        auto& arrangements = this->getArrangements();

        auto it = arrangements.begin() + this->arrangementIndex;
        arrangements.insert(it, this->arrangement);

        for (
            auto& animation :
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object->animations
        ) for (auto& key : animation.keys) {
            if (key.arrangementIndex >= arrangementIndex)
                key.arrangementIndex++;
        }

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned arrangementIndex;

    CellAnim::Arrangement arrangement;

    std::vector<CellAnim::Arrangement>& getArrangements() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements;
    }

    CellAnim::Arrangement& getArrangement() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object
            ->arrangements.at(this->arrangementIndex);
    }
};

#endif // COMMANDDELETEARRANGEMENT_HPP
