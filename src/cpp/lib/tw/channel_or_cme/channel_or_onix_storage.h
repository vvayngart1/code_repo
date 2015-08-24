#pragma once

#include <tw/channel_or_cme/settings.h>
#include <tw/generated/channel_or_defs.h>

#include <OnixS/FIXEngine.h>

#include <deque>
#include <map>
#include <vector>

namespace tw {
namespace channel_or_cme {
    
class ChannelOrOnixStorage : public OnixS::FIX::ISessionStorage {
    static const uint32_t MAX_OUTBOUND_CACHE_SIZE;
    
public:
    ChannelOrOnixStorage();
    ~ChannelOrOnixStorage();
    
    void clear();
    
public:
    bool init(const TSessionSettingsPtr& settings);
    void onStateChange(const std::string& currState, const std::string& prevState);
    
public:
    // OnixS::FIX::ISessionStorage interface
    //        
    virtual int inSeqNum() {
        return _state._inSeqNum;
    }
    
    virtual void inSeqNum (int seqNum) {
        _state._inSeqNum = seqNum;
    }
    
    virtual int outSeqNum() {
        return _state._outSeqNum;
    }
    
    virtual void outSeqNum (int seqNum) {
        _state._outSeqNum = seqNum;
    }
    
    virtual void setSessionTerminationFlag(bool terminated) {        
    }
    
    virtual void close(bool keepSequenceNumbers, bool doBackup) {        
    }
    
    virtual void getOutbound(int beginSeqNum, int endSeqNum, OnixS::FIX::ISessionStorage::ISessionStorageListener* listener);
    virtual void storeInbound(int seqNum, const OnixS::FIX::ISessionStorage::RawMessagePointer& pointer);
    virtual void storeOutbound(int seqNum, const OnixS::FIX::ISessionStorage::RawMessagePointer& pointer);
    virtual void sessionCreationTime (OnixS::FIX::Timestamp timestamp);
    virtual OnixS::FIX::Timestamp sessionCreationTime();
    
private:
    void persistMessage(int seqNum, const OnixS::FIX::ISessionStorage::RawMessagePointer& p, tw::channel_or::FixSessionCMEMsg& v);
    
private:
    typedef std::deque<int> TSeqNums;
    
    typedef std::vector<byte> TRawMessage;
    typedef std::map<int, TRawMessage> TOutboundMsgs;    
    
    tw::channel_or::FixSessionCMEState _state;
    tw::channel_or::FixSessionCMEMsg _msgInbound;
    tw::channel_or::FixSessionCMEMsg _msgOutbound;
    OnixS::FIX::Timestamp _sessionCreationTime;
    
    TSeqNums _seqNums;
    TOutboundMsgs _outboundMsgs;
};

    
} // namespace channel_or_cme
} // namespace tw
