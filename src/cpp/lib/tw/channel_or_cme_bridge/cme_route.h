#pragma once

#include <tw/common/settings.h>
#include <tw/common_thread/locks.h>
#include <tw/common/pool.h>
#include <tw/channel_or_cme/channel_or_onix_storage.h>
#include <tw/channel_or_cme_bridge/translator.h>
#include <tw/generated/commands_common.h>

#include <OnixS/FIXEngine.h>

#include <boost/shared_ptr.hpp>
#include <tr1/unordered_map>

namespace tw {
namespace channel_or_cme_bridge {
    
class IClient {
public:
    virtual ~IClient() {
    }
    
    virtual void onReply(Message& msg, bool remove) = 0;
    virtual const std::string& name() const = 0;
};
    
class CmeRoute : public OnixS::FIX::ISessionListener {
public:
    typedef boost::shared_ptr<OnixS::FIX::Session> TSessionPtr;
    typedef boost::shared_ptr<OnixS::FIX::Scheduling::SessionScheduler> TSessionSchedulerPtr;
    
    typedef std::pair<IClient*, TOrderCmeBridgeInfoPtr> TSessionInfo;
    typedef std::pair<int32_t, TSessionInfo> TSeqIdSessionInfo;
    typedef std::tr1::unordered_map<std::string, TSeqIdSessionInfo> TSeqIdSessionInfos;
    
public:
    static TSessionSchedulerPtr _sessionScheduler;
    
public:
    CmeRoute();
    ~CmeRoute();
    
    void clear();
    
public:
    static bool global_init(const tw::channel_or_cme::Settings& settings);
    static void global_shutdown();
    
public:
    bool init(const tw::channel_or_cme::TSessionSettingsPtr& settings);
    bool start();    
    void stop();
    
    TSessionPtr getSession() const {
        return _session;
    }
    
    std::string onCommand(const tw::common::Command& command) {
        if ( command._type != tw::common::eCommandType::kChannelOrCme || tw::common::eCommandSubType::kStatus != command._subType)
            return std::string();
        
        tw::common_commands::ChannelOrCme data;
        data._results = OnixS::FIX::Session::state2string(_state);
        data._exSessionName = _session->getSenderCompID();
            
        tw::common::Command c = data.toCommand();
        c._connectionId = command._connectionId;
        c._type = command._type;
        c._subType = command._subType;
        
        return c.toString();
    }
    
public:
    // ProcessorOut interface
    //
    bool sendNew(const Message& source, IClient* session, std::string& error);
    bool sendMod(const Message& source, IClient* session, std::string& error);
    bool sendCxl(const Message& source, IClient* session, std::string& error);
    void sendCxlAll(const IClient* const session);
    
public:
    // ISessionListener interface
    //
    virtual void onInboundApplicationMsg(const Message& msg, OnixS::FIX::Session* sn);
    virtual void onInboundSessionMsg(Message& msg, OnixS::FIX::Session* sn);
    virtual void onStateChange(OnixS::FIX::Session::State newState, OnixS::FIX::Session::State prevState, OnixS::FIX::Session* sn);
    virtual bool onResendRequest (Message& message, OnixS::FIX::Session* sn);
    virtual void onError(OnixS::FIX::ISessionListener::ErrorReason reason, const std::string& description, OnixS::FIX::Session* sn);
    virtual void onWarning(OnixS::FIX::ISessionListener::WarningReason reason, const std::string& description, OnixS::FIX::Session* sn);
   
private:
    bool isValid() {
        if ( !_session )
            return false;
        
        return true;
    }
    
    bool isLoggedOn() {
        if ( !isValid() )
            return false;
        
        return ( OnixS::FIX::Session::ACTIVE == _session->getState() );
    }
    
    void processLogout(const Message& msg);
    void sendAlert(const tw::channel_or::Alert& alert);
    void sendAlert(const std::string& text);
    
private:
    void addSeqIdSessionInfo(const Message& source, const Message& dest, IClient* session, const TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr) {
        try {            
            TSessionInfo sessionInfo(session, orderCmeBridgeInfoPtr);
            
            std::string sourceClOrdId = source.get(Tags::ClOrdID);
            std::string destClOrdId = dest.get(Tags::ClOrdID);
            
            _seqIdSessionInfos[sourceClOrdId] = TSeqIdSessionInfo(source.getInt32(Tags::MsgSeqNum), sessionInfo);
            _seqIdSessionInfos[destClOrdId] = TSeqIdSessionInfo(source.getInt32(Tags::MsgSeqNum), sessionInfo);
            
            LOGGER_INFO << "Added seqIdSessionInfo - sourceClOrdId: " << sourceClOrdId << " <==> destClOrdId: " << destClOrdId << "\n";
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
    }
    
    TSeqIdSessionInfo& getSeqIdSessionInfo(const std::string& clOrderId) {
        try {
            TSeqIdSessionInfos::iterator iter = _seqIdSessionInfos.find(clOrderId);
            if ( iter != _seqIdSessionInfos.end() )
                return iter->second;
            
            LOGGER_WARN << "Can't find TSeqIdSessionInfo for: " << clOrderId << "\n";
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
        
        return _nullSeqIdSessionInfo;
    }
    
    void remSeqIdSessionInfo(const std::string& clOrderId) {
        try {
            TSeqIdSessionInfos::iterator iter = _seqIdSessionInfos.find(clOrderId);
            if ( iter != _seqIdSessionInfos.end() ) {
                _seqIdSessionInfos.erase(iter);
                LOGGER_INFO << "Removed seqIdSessionInfo - clOrderId: " << clOrderId << " -- size: " << _seqIdSessionInfos.size() << "\n";
                return;
            }
            
            LOGGER_WARN << "Can't find TSeqIdSessionInfo for: " << clOrderId << "\n";
        } catch(const std::exception& e) {
            LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
        } catch(...) {
            LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        }
    }
    
    void removeOrderCmeBridgeInfo(const TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr) {
        // Remove all orderId
        //
        remSeqIdSessionInfo(orderCmeBridgeInfoPtr->_correlationClOrdID);
        remSeqIdSessionInfo(orderCmeBridgeInfoPtr->_correlationClOrdID_bridge);

        std::map<std::string, std::string>::iterator iter = orderCmeBridgeInfoPtr->_orderIds.begin();
        std::map<std::string, std::string>::iterator end = orderCmeBridgeInfoPtr->_orderIds.end();
        for ( ; iter != end; ++iter ) {
            remSeqIdSessionInfo(iter->first);
            remSeqIdSessionInfo(iter->second);
        }
    }
    
    bool getSeqIdSessionInfoForMsg(const Message& msg, IClient*& session, TOrderCmeBridgeInfoPtr& orderCmeBridgeInfoPtr, int32_t& seqNum) {
        TSeqIdSessionInfo info = getSeqIdSessionInfo(msg.get(Tags::OrigClOrdID));
        if ( info.first == _nullSeqIdSessionInfo.first )
            info = getSeqIdSessionInfo(msg.get(Tags::ClOrdID));
        
        if ( info.first == _nullSeqIdSessionInfo.first ) {
            LOGGER_ERRO << "Can't find TSeqIdSessionInfo for: " << msg.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
            return false;
        }

        session = info.second.first;
        orderCmeBridgeInfoPtr = info.second.second;

        if ( NULL == session || NULL == orderCmeBridgeInfoPtr.get() ) {
            LOGGER_ERRO << "TSeqIdSessionInfo is NULL for: " << msg.toString(tw::channel_or_cme::FIX_FIELDS_DELIMITER) << "\n";
            return false;
        }
        
        seqNum = info.first;
        
        return true;
    }
    
private:
    typedef tw::common_thread::Lock TLock;    
    typedef tw::common::Pool<tw::channel_or::OrderCmeBridgeInfo, TLock> TPool;
    
private:
    TLock _lock;
    
    bool _receivedLogout;
    TSeqIdSessionInfo _nullSeqIdSessionInfo;
    
    TSessionPtr _session;
    Translator _translator;
    TSeqIdSessionInfos _seqIdSessionInfos;
    tw::channel_or_cme::TSessionSettingsPtr _settings;
    tw::channel_or_cme::ChannelOrOnixStorage _storage;
    OnixS::FIX::Session::State _state;
    TPool _pool;
};

    
} // namespace channel_or_cme_bridge
} // namespace tw
