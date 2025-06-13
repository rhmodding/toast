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
            Logging::err << "The terminate handler was invoked with no exception." << std::endl;
    }
    catch (const std::exception& ex) {
        Logging::err <<
            "An exception of type " << CxxDemangleUtil::Demangle(typeid(ex).name()) << " was caught: " << ex.what() << std::endl;
    }
    catch (...) {
        Logging::err << "An unknown exception was thrown." << std::endl;
    }

    Logging::err << "This is a fatal error; toast will now be closed." << std::endl;

    tinyfd_messageBox( \
        "toast has crashed", \
        "toast has encountered a fatal error and cannot continue execution.\n" \
        "You can check the log for more details (toast.log). toast will now close.", \
        "ok", "error", 1 \
    );

    Logging::Close();

    std::exit(1);
}

void TerminateHandler::Init() {
    // In debug builds we want the exception to stay unhandled, otherwise the debugger won't catch it.
#ifdef NDEBUG
    std::set_terminate(onTerminate);
#endif
}
