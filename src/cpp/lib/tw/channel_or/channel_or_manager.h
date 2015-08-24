#pragma once

#include <tw/common/command.h>
#include <tw/common/singleton.h>
#include <tw/common/settings.h>
#include <tw/generated/channel_or_defs.h>

namespace tw {
namespace channel_or {
    
class ChannelOrManager : public tw::common::Singleton<ChannelOrManager> {
public:
    ChannelOrManager();
    ~ChannelOrManager();
    
    void clear();
    
public:
    bool init(const tw::common::Settings& settings);
    bool start();    
    bool stop();
    
public:
    // ProcessorOut interface
    //
    bool sendNew(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);    
    bool sendMod(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);    
    bool sendCxl(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);    
    void onCommand(const tw::common::Command& command);
    bool rebuildOrder(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);
    void recordFill(const tw::channel_or::Fill& fill);    
    void rebuildPos(const tw::channel_or::PosUpdate& update);
    void onRebuildOrderRej(const tw::channel_or::TOrderPtr& order, tw::channel_or::Reject& rej);
    
    void onNewRej(const TOrderPtr& order, const Reject& rej);
    void onModRej(const TOrderPtr& order, const Reject& rej);
    void onCxlRej(const TOrderPtr& order, const Reject& rej);
};

    
} // namespace channel_or
} // namespace tw
