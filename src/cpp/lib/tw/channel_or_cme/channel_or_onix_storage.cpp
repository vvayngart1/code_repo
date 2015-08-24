#include <tw/channel_or_cme/channel_or_onix_storage.h>
#include <tw/channel_or/channel_or_storage.h>

#include <boost/date_time/gregorian/gregorian.hpp>

namespace tw {
namespace channel_or_cme {
    
// Limit memory cache to 10000 messages
//
const uint32_t ChannelOrOnixStorage::MAX_OUTBOUND_CACHE_SIZE = 10000;
    
ChannelOrOnixStorage::ChannelOrOnixStorage() {
    clear();    
}

ChannelOrOnixStorage::~ChannelOrOnixStorage() {
}

void ChannelOrOnixStorage::clear() {
    _state.clear();
    _msgInbound.clear();
    _msgOutbound.clear();
    _sessionCreationTime = OnixS::FIX::Timestamp::getUtc();
    
    _seqNums.clear();
    _outboundMsgs.clear();
}
    
bool ChannelOrOnixStorage::init(const TSessionSettingsPtr& settings) {
    try {
        // TODO: right now using local time (which happened to be CME time), if
        // this code to be executed in a different timezone from CST, need to
        // reimplement
        //        
        boost::gregorian::date now = boost::gregorian::day_clock::local_day();
        
        _state._year = now.year();
        _state._week = now.week_number();
        _state._senderCompID = settings->_senderCompId;
        _state._targetCompID = settings->_targetCompId;
        
        if ( !tw::channel_or::ChannelOrStorage::instance().getFixSessionCMEStatesForSession(_state) )
            return false;
        
        _msgInbound._year = now.year();
        _msgInbound._week = now.week_number();
        _msgInbound._senderCompID = settings->_senderCompId;
        _msgInbound._targetCompID = settings->_targetCompId;
        _msgInbound._direction = tw::channel_or::eDirection::kInbound;
        
        _msgOutbound._year = now.year();
        _msgOutbound._week = now.week_number();
        _msgOutbound._senderCompID = settings->_senderCompId;
        _msgOutbound._targetCompID = settings->_targetCompId;
        _msgOutbound._direction = tw::channel_or::eDirection::kOutbound;
        
        if ( !_state._sessionCreationTimeStr.empty() )
            _sessionCreationTime = OnixS::FIX::Timestamp::fromString(_state._sessionCreationTimeStr);
        
        LOGGER_INFO << "SessionCreationTime: " << _sessionCreationTime.toString() << "\n";
                
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
        return false;
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
        return false;
    }
    
    return true;
}

void ChannelOrOnixStorage::onStateChange(const std::string& currState,
                                         const std::string& prevState) {
    try {
        _state._prevState = prevState;
        _state._currState = currState;
        
        if ( !tw::channel_or::ChannelOrStorage::instance().persist(_state) )
            LOGGER_ERRO << "Can't persist state: "  << _state.toString() << "\n";                
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelOrOnixStorage::getOutbound(int beginSeqNum,
                                       int endSeqNum,
                                       OnixS::FIX::ISessionStorage::ISessionStorageListener* listener) {
    try {
        if ( !listener )
            return;
        
        for (int i = beginSeqNum; i <= endSeqNum; ++i) {
            TOutboundMsgs::iterator iter = _outboundMsgs.find(i);
            if ( iter != _outboundMsgs.end() ) {
                listener->onReplayedMessage(ISessionStorage::RawMessagePointer(&iter->second[0], iter->second.size()));
            }
        }
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";        
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";
    }
}

void ChannelOrOnixStorage::persistMessage(int seqNum, const OnixS::FIX::ISessionStorage::RawMessagePointer& p, tw::channel_or::FixSessionCMEMsg& v) {
    try {
        v._seqNum = seqNum;
        v._message.assign(reinterpret_cast<const char*>(p.buffer_), p.length_);
        
        // Filter out all heartbeat messages
        //
        OnixS::FIX::Message m(v._message, OnixS::FIX::MessageValidationFlags::None);
        if ( m.getType() == "0")
            return;
        
        if ( !tw::channel_or::ChannelOrStorage::instance().persist(v) )
            LOGGER_ERRO << "Can't persist message: "  << v.toString() << "\n";
        
        if ( tw::channel_or::eDirection::kInbound == v._direction )
            return;
        
        if ( _seqNums.size() == MAX_OUTBOUND_CACHE_SIZE ) {
            int seqNumToDelete = _seqNums.front();
            _seqNums.pop_front();
            TOutboundMsgs::iterator iter = _outboundMsgs.find(seqNumToDelete);
            if ( iter != _outboundMsgs.end() )
                _outboundMsgs.erase(iter);
        }
        
        _seqNums.push_back(seqNum);
        _outboundMsgs.insert(TOutboundMsgs::value_type(seqNum, TRawMessage(p.buffer_, p.buffer_+ p.length_)));
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
    }
}

void ChannelOrOnixStorage::storeInbound(int seqNum,
                                        const OnixS::FIX::ISessionStorage::RawMessagePointer& pointer) {
    try {
        _state._inSeqNum = seqNum;
        
        if ( !tw::channel_or::ChannelOrStorage::instance().persist(_state) )
            LOGGER_ERRO << "Can't persist state: "  << _state.toString() << "\n";
        
        persistMessage(seqNum, pointer, _msgInbound);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
    }
}

void ChannelOrOnixStorage::storeOutbound(int seqNum,
                                         const OnixS::FIX::ISessionStorage::RawMessagePointer& pointer) {
    try {
        _state._outSeqNum = seqNum;
        
        if ( !tw::channel_or::ChannelOrStorage::instance().persist(_state) )
            LOGGER_ERRO << "Can't persist state: "  << _state.toString() << "\n";
        
        persistMessage(seqNum, pointer, _msgOutbound);
        
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
    }
}

void ChannelOrOnixStorage::sessionCreationTime(OnixS::FIX::Timestamp timestamp) {
    try {
        _sessionCreationTime = timestamp;
        _state._sessionCreationTimeStr = _sessionCreationTime.toString();
        
        LOGGER_INFO << "Setting SessionCreationTime: " << _sessionCreationTime.toString() << "::" << _state.toString() << "\n";
    } catch(const std::exception& e) {
        LOGGER_ERRO << "Exception: "  << e.what() << "\n" << "\n";
    } catch(...) {
        LOGGER_ERRO << "Exception: UNKNOWN" << "\n" << "\n";        
    }
}

OnixS::FIX::Timestamp ChannelOrOnixStorage::sessionCreationTime() {
    return _sessionCreationTime;
}

} // namespace channel_or_cme
} // namespace tw
