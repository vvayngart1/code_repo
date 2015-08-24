#include <tw/common/defs.h>
#include <tw/common_thread/locks.h>
#include <tw/common/settings.h>
#include <tw/channel_pf_cme/channel_pf_onix.h>

#include <map>

class InstrumentLoader : public OnixS::CME::MarketData::SecurityDefinitionListener {
public:
    InstrumentLoader();
    ~InstrumentLoader();
    
    void clear();
    
public:
    bool load(const tw::common::Settings& settings);
    
public:
    // OnixS::CME::MarketData::SecurityDefinitionListener interface
    //
    virtual void onSecurityDefinition(const OnixS::CME::MarketData::SecurityDefinition& securityDefinition, const OnixS::CME::MarketData::ChannelId& channelId);
    virtual void onSecurityDefinitionDeleted(OnixS::CME::MarketData::SecurityId securityId, const OnixS::CME::MarketData::ChannelId& channelId);
    virtual void onSecurityDefinitionsRecoveryStarted(const OnixS::CME::MarketData::ChannelId& channelId);
    virtual void onSecurityDefinitionsRecoveryFinished(const OnixS::CME::MarketData::ChannelId& channelId);     
    
private:
    bool isDone();
    std::string getDisplayName(const std::string& securityId);
    bool loadAndUpdateInstrumentsFees(const tw::common::Settings& settings);
    
    void processSettlementFile(const std::string& fileName);
    
private:
    typedef std::map<OnixS::CME::MarketData::ChannelId, bool> TChannelsStatus;
    typedef std::map<OnixS::CME::MarketData::SecurityId, std::string> TSecurityIdToDisplayName;
    
private:
    typedef tw::common_thread::Lock TLock;
    
    TLock _lock;
    
    TChannelsStatus _channelsStatus;
    TSecurityIdToDisplayName _securityIdToDisplayName;
};
