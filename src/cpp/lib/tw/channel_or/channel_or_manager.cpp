#include <tw/channel_or/channel_or_manager.h>

#include <tw/channel_or_cme/channel_or_cme_manager.h>

namespace tw {
namespace channel_or {
    
ChannelOrManager::ChannelOrManager() {
    clear();
}

ChannelOrManager::~ChannelOrManager() {
    
}

void ChannelOrManager::clear() {
    
}

// TODO: for now uses only channel_or_cme::ChannelOrManager - once
// more exhcnages are implemented, need to integrate!!!
//
bool ChannelOrManager::init(const tw::common::Settings& settings) {
    return tw::channel_or_cme::ChannelOrManager::instance().init(settings);
}

bool ChannelOrManager::ChannelOrManager::start() {
    return tw::channel_or_cme::ChannelOrManager::instance().start();
}

bool ChannelOrManager::stop() {
    return tw::channel_or_cme::ChannelOrManager::instance().stop();
}

// ProcessorOut interface
//
bool ChannelOrManager::sendNew(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    return tw::channel_or_cme::ChannelOrManager::instance().sendNew(order, rej);
}

bool ChannelOrManager::sendMod(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    return tw::channel_or_cme::ChannelOrManager::instance().sendMod(order, rej);
}

bool ChannelOrManager::sendCxl(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    return tw::channel_or_cme::ChannelOrManager::instance().sendCxl(order, rej);
}

void ChannelOrManager::onCommand(const tw::common::Command& command) {
    tw::channel_or_cme::ChannelOrManager::instance().onCommand(command);
}

bool ChannelOrManager::rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    return tw::channel_or_cme::ChannelOrManager::instance().rebuildOrder(order, rej);
}

void ChannelOrManager::recordFill(const tw::channel_or::Fill& fill) {
    tw::channel_or_cme::ChannelOrManager::instance().recordFill(fill);
}

void ChannelOrManager::rebuildPos(const tw::channel_or::PosUpdate& update) {
    tw::channel_or_cme::ChannelOrManager::instance().rebuildPos(update);
}

void ChannelOrManager::onRebuildOrderRej(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej) {
    // Not implemented - nothing to do
    //
}

void ChannelOrManager::onNewRej(const TOrderPtr& order, const Reject& rej) {
    tw::channel_or_cme::ChannelOrManager::instance().onNewRej(order, rej);
}

void ChannelOrManager::onModRej(const TOrderPtr& order, const Reject& rej) {
    tw::channel_or_cme::ChannelOrManager::instance().onModRej(order, rej);
}

void ChannelOrManager::onCxlRej(const TOrderPtr& order, const Reject& rej) {
    tw::channel_or_cme::ChannelOrManager::instance().onCxlRej(order, rej);
}
    
} // namespace channel_or
} // namespace tw
