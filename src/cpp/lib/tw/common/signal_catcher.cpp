#include <tw/common/signal_catcher.h>
#include <tw/log/defs.h>

#include <signal.h>

namespace tw {
namespace common {
    
// TODO: workaround until upgrade to new version of boost 
//
static SignalCatcher* signalCatcher = NULL;

static void handle_stop_global(int v) {
    try {
        if ( signalCatcher )
            signalCatcher->handle_stop(v);
        else
            LOGGER_WARN << "signalCatcher is NULL" << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

SignalCatcher::SignalCatcher() : _lock(),
                                 _event() {
}

SignalCatcher::~SignalCatcher() {
}

void SignalCatcher::run() {
    try {
        LOGGER_INFO << "Initializing signals catcher... " << "\n";
        
        signalCatcher = this;
        
        // Hang up on controlling terminal
        //
        signal(SIGHUP, handle_stop_global);
        
        // 'Ctrl-C'
        //
        signal(SIGINT, handle_stop_global);
        
        // 'kill'
        //
        signal(SIGTERM, handle_stop_global);
        
        // 'Ctrl-Z'
        //
        signal(SIGTSTP, handle_stop_global);
        
        LOGGER_INFO << "Running signal catcher. Enter: 'Ctrl-C', 'Ctrl-Z' or 'kill <pid>' to stop" << "\n";
        
        TLockGuard lock(_lock);
        _event.wait(lock);
        
        LOGGER_INFO << "Finished signal catcher" << "\n";
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}


void SignalCatcher::handle_stop(int32_t v) {
    try {
        LOGGER_INFO << "Caught signal: " << v << " - stopping signal catcher... " << "\n";
        
        _event.notify_one();
        
        LOGGER_INFO << "Stopped signal catcher" << "\n";
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

} // namespace common
} // namespace tw
