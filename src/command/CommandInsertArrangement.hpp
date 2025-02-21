#ifndef COMMANDINSERTARRANGEMENT_HPP
#define COMMANDINSERTARRANGEMENT_HPP

#include "BaseCommand.hpp"

#include "../AppState.hpp"

#include "../SessionManager.hpp"
#include "../PlayerManager.hpp"

#include "../anim/CellAnim.hpp"

class CommandInsertArrangement : public BaseCommand {
public:
    // Constructor: Insert arrangement by cellanimIndex and arrangementIndex from arrangement.
    CommandInsertArrangement(
        unsigned cellanimIndex,
        unsigned arrangementIndex,
        CellAnim::Arrangement arrangement
    ) :
        cellanimIndex(cellanimIndex), arrangementIndex(arrangementIndex),
        arrangement(arrangement)
    {}
    ~CommandInsertArrangement() = default;

    void Execute() override {
        auto& arrangements = this->getCellanim()->arrangements;

        auto it = arrangements.begin() + this->arrangementIndex;
        arrangements.insert(it, this->arrangement);

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        auto& arrangements = this->getCellanim()->arrangements;

        auto it = arrangements.begin() + this->arrangementIndex;
        arrangements.erase(it);

        for (auto& animation : this->getCellanim()->animations)
            for (auto& key : animation.keys) {
                if (key.arrangementIndex >= arrangements.size())
                    key.arrangementIndex = 0;
            }

        PlayerManager::getInstance().correctState();

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;

    unsigned arrangementIndex;

    CellAnim::Arrangement arrangement;

    std::shared_ptr<CellAnim::CellAnimObject> getCellanim() {
        return
            SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(this->cellanimIndex).object;
    }
};

#endif // COMMANDINSERTARRANGEMENT_HPP
