#include "TerminateHandler.hpp"

#include <exception>

#include <typeinfo>

#include "Logging.hpp"

#include "util/CxxDemangleUtil.hpp"

#include <tinyfiledialogs.h>

static void onTerminate() {
    try {
        std::exception_ptr exceptionPointer { std::current_exception() };
        if (exceptionPointer)
            std::rethrow_exception(exceptionPointer);
        else
            Logging::error("The terminate handler was invoked with no exception.");
    }
    catch (const std::exception &ex) {
        Logging::error(
            "An exception of type {} was caught: {}",
            CxxDemangleUtil::Demangle(typeid(ex).name()),
            ex.what()
        );
    }
    catch (...) {
        Logging::error("An unknown exception was thrown.");
    }

    Logging::error("This is a fatal error; toast will now be closed.");

    tinyfd_messageBox(
        "toast has crashed",
        "toast has encountered a fatal error and cannot continue execution.\n"
        "You can check the log for more details (toast.log). toast will now close.",
        "ok", "error", 1
    );

    Logging::close();

    std::exit(1);
}

void TerminateHandler::init() {
    // In debug builds we want the exception to stay unhandled, otherwise the debugger won't catch it.
#ifdef NDEBUG
    std::set_terminate(onTerminate);
#endif
}
