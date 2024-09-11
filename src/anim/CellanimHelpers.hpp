#ifndef CELLANIMHELPERS_HPP
#define CELLANIMHELPERS_HPP

#include "../SessionManager.hpp"

#include "../command/CommandInsertArrangement.hpp"

namespace CellanimHelpers {
    inline uint16_t DuplicateArrangement(uint16_t arrangementIndex) {
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
}

#endif
