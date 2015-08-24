#pragma once

#include <tw/common/command.h>
#include <tw/common/pool.h>
#include <tw/common/singleton.h>
#include <tw/common/settings.h>
#include <tw/generated/channel_or_defs.h>
#include <tw/channel_or_cme/channel_or_onix.h>

namespace tw {
namespace channel_or_cme {
    
class ChannelOrManager : public tw::common::Singleton<ChannelOrManager> {
public:
    typedef tw::channel_or::TOrderPtr TOrderPtr;
    typedef tw::channel_or::Reject Reject;
    typedef tw::channel_or::Fill Fill;
    typedef tw::channel_or::PosUpdate PosUpdate;
    typedef tw::channel_or::Alert Alert;

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
    
    void onNewRej(const TOrderPtr& order, const Reject& rej);
    void onModRej(const TOrderPtr& order, const Reject& rej);
    void onCxlRej(const TOrderPtr& order, const Reject& rej);
        
private:
    // TODO: for now just have one session to CME - implement multiple
    // once needed!!!
    //
    typedef boost::shared_ptr<ChannelOrOnix> TChannelOrOnixPtr;
    TChannelOrOnixPtr _channel;
};

    
} // namespace channel_or_cme
} // namespace tw
