#include "CellanimHelpers.hpp"

#include "../SessionManager.hpp"

#include "../command/CommandInsertArrangement.hpp"

uint16_t CellanimHelpers::DuplicateArrangement(uint16_t arrangementIndex) {
    GET_SESSION_MANAGER;

    auto object = sessionManager.getCurrentSession()->getCellanimObject();

    sessionManager.getCurrentSession()->executeCommand(
    std::make_shared<CommandInsertArrangement>(
        sessionManager.getCurrentSession()->currentCellanim,
        object->arrangements.size(),
        object->arrangements.at(arrangementIndex)
    ));

    sessionManager.getCurrentSessionModified() = true;

    return object->arrangements.size() - 1;
}

bool CellanimHelpers::getArrangementUnique(uint16_t arrangementIndex) {
    unsigned timesUsed{ 0 };

    for (const auto& animation : SessionManager::getInstance().getCurrentSession()->getCellanimObject()->animations) {
        for (const auto& key : animation.keys) {
            if (key.arrangementIndex == arrangementIndex)
                timesUsed++;

            if (timesUsed > 1) {
                return false;
            }
        }
    }

    return true;
}