#include <tw/channel_or_cme/channel_or_cme_manager.h>
#include <tw/channel_or_cme/settings.h>

namespace tw {
namespace channel_or_cme {
    
ChannelOrManager::ChannelOrManager() {
    clear();
}

ChannelOrManager::~ChannelOrManager() {
}

void ChannelOrManager::clear() {    
}

bool ChannelOrManager::init(const tw::common::Settings& settings) {
    bool status = true;
    try {
        tw::channel_or_cme::Settings s;
        if ( !s.parse(settings._channel_or_cme_dataSource) )
            return false;
        
        if ( !tw::channel_or_cme::ChannelOrOnix::global_init(s) )
            return false;
        
        // For now, just get the first enabled session
        //
        tw::channel_or_cme::TSessionSettingsPtr sessionSettings;
        if ( !s.getFirstEnabled(sessionSettings) ) {
            LOGGER_ERRO << "No enabled sessions in: "  << settings._channel_or_cme_dataSource << "\n";        
            return false;
        }
        
        _channel.reset(new ChannelOrOnix());
        if ( !_channel->init(sessionSettings) ) 
            return false;
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool ChannelOrManager::ChannelOrManager::start() {
    bool status = true;
    try {
        return _channel->start();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

bool ChannelOrManager::stop() {
    bool status = true;
    try {
        if ( _channel ) {
            _channel->stop();        
            _channel.reset();
        }
        
        tw::channel_or_cme::ChannelOrOnix::global_shutdown();
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        status = false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        status = false;
    }
    
    return status;
}

// ProcessorOut interface
//
bool ChannelOrManager::sendNew(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    return _channel->sendNew(order, rej);
}

bool ChannelOrManager::sendMod(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    return _channel->sendMod(order, rej);
}

bool ChannelOrManager::sendCxl(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    return _channel->sendCxl(order, rej);
}

void ChannelOrManager::onCommand(const tw::common::Command& command) {
    _channel->onCommand(command);
}

bool ChannelOrManager::rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    return _channel->rebuildOrder(order, rej);
}

void ChannelOrManager::recordFill(const tw::channel_or::Fill& fill) {
    _channel->recordFill(fill);
}

void ChannelOrManager::rebuildPos(const tw::channel_or::PosUpdate& update) {
    _channel->rebuildPos(update);
}

void ChannelOrManager::onNewRej(const TOrderPtr& order, const Reject& rej) {
    _channel->onNewRej(order, rej);
}

void ChannelOrManager::onModRej(const TOrderPtr& order, const Reject& rej) {
    _channel->onModRej(order, rej);
}

void ChannelOrManager::onCxlRej(const TOrderPtr& order, const Reject& rej) {
    _channel->onCxlRej(order, rej);
}
    
} // namespace channel_or_cme
} // namespace tw
