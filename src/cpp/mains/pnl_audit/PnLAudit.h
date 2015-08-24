#pragma once

#include <tw/log/defs.h>
#include <tw/instr/instrument_manager.h>
#include <tw/common/defs.h>
#include <tw/common/settings.h>
#include <tw/common/filesystem.h>

#include <tw/generated/enums_common.h>
#include <tw/generated/channel_or_defs.h>

#include <fstream>

namespace tw {
namespace pnl_audit {
    class PnLAudit {
    public:
        typedef tw::channel_or::PnLAuditTrailInfo TPnLAuditTrailInfo;
        typedef std::vector<tw::channel_or::PnLAuditTrailInfo> TPnLAuditTrailInfos;
        typedef std::map<std::string, std::string> TStratPreviousInfo;
        
        typedef tw::channel_or::PnLAuditStatsInfo TPnLAuditStatsInfo;
        typedef tw::channel_or::PnLAuditStratStatsInfo TPnLAuditStratStatsInfo;
        typedef std::map<std::string, TPnLAuditStratStatsInfo> TPnLAuditStratStatsInfos;
        
    public:
        PnLAudit() {
        }
        
        void clear() {
            _timestamps.clear();
            _stratsToPnLAuditTrailInfos.clear();
            _pnLAuditStratStatsInfos.clear();
        }
        
        bool start(const tw::common::Settings& settings) {
            _dirName = "pnl_audit_" + formatTimestamp(tw::common::THighResTime::now());
            return tw::common::Filesystem::create_dir(_dirName);
        }
        
        bool process(const TPnLAuditTrailInfos& infos, const tw::common::Settings& settings, const std::string& account) {
            clear();
            
            std::string filename = getFileName(settings, account);
            std::ofstream file(filename.c_str(), std::ios_base::out | std::ios_base::trunc);
            if ( !file.good() ) {
                LOGGER_ERRO << "Failed to open file: " << filename << "\n";
                return false;
            }
            
            LOGGER_INFO << "Results will be written to file: " << filename << "\n";
            
            bool status = true;
            switch ( settings._pnl_audit_mode ) {
                case tw::common::ePnLAuditMode::kPnLAnalysis:
                    status = processAnalysis(infos, settings, file);
                    break;
                default:
                    status = processNormal(infos, settings, file);
                    break;
            }
            
            file.close();
            return status;
        }
        
        bool processAnalysis(const TPnLAuditTrailInfos& infos, const tw::common::Settings& settings, std::ofstream& file) {
            for ( size_t i = 0; i < infos.size(); ++i ) {
                TPnLAuditTrailInfo info = infos[i];
                if ( "ACCOUNT" != info._parentType ) {
                    TPnLAuditStratStatsInfo& stratStatsInfo = _pnLAuditStratStatsInfos[getStratName(info)];

                    stratStatsInfo._accountId = info._accountId;
                    stratStatsInfo._strategyId = info._strategyId;
                    
                    if ( NULL != info._instrument1.get() )
                        stratStatsInfo._instrument1 = info._instrument1;
                    
                    if ( NULL != info._instrument2.get() )
                        stratStatsInfo._instrument2 = info._instrument2;
                    
                    
                    if ( 0 != info._pos1.get() || 0 != info._pos2.get() ) {
                        if ( 0 == stratStatsInfo._pos1.get() && 0 == stratStatsInfo._pos2.get() ) {
                            TPnLAuditStatsInfo statsInfo;
                            statsInfo._timestamp1 = info._eventTimestamp;
                            stratStatsInfo._statsInfos.push_back(statsInfo);
                        }
                        
                        if ( 0 == stratStatsInfo._pos1.get() && 0 != info._pos1.get() )
                            stratStatsInfo._statsInfos.back()._entrySide1 = (0 < info._pos1.get()) ? tw::channel_or::eOrderSide::kBuy : tw::channel_or::eOrderSide::kSell;
                        else if ( 0 == stratStatsInfo._pos2.get() && 0 != info._pos2.get() )
                            stratStatsInfo._statsInfos.back()._entrySide2 = (0 < info._pos2.get()) ? tw::channel_or::eOrderSide::kBuy : tw::channel_or::eOrderSide::kSell;

                        stratStatsInfo._pos1 = info._pos1;
                        stratStatsInfo._pos2 = info._pos2;
                        updateStatsInfo(info, stratStatsInfo._statsInfos.back());
                    } else if ( 0 != stratStatsInfo._pos1.get() || 0 != stratStatsInfo._pos2.get() ) {
                        TPnLAuditStatsInfo& statsInfo = stratStatsInfo._statsInfos.back();
                        statsInfo._timestamp2 = info._eventTimestamp;

                        stratStatsInfo._pos1 = info._pos1;
                        stratStatsInfo._pos2 = info._pos2;

                        if ( info._instrument1.get() != NULL )
                            statsInfo._netTicks1 = (info._realizedInDollars1 - stratStatsInfo._prevPnL1) / info._instrument1->_tickValue;

                        if ( info._instrument2.get() != NULL )
                            statsInfo._netTicks2 = (info._realizedInDollars2 - stratStatsInfo._prevPnL2) / info._instrument2->_tickValue;

                        statsInfo._netPnL = info._realizedInDollars - stratStatsInfo._prevPnLTotal;

                        stratStatsInfo._prevPnL1 = info._realizedInDollars1;
                        stratStatsInfo._prevPnL2 = info._realizedInDollars2;
                        stratStatsInfo._prevPnLTotal = info._realizedInDollars;
                    }
                }
            }
            
            // Output header
            //
            file << "AccountId,StratId,StratName,Trade #,StartTime,EndTime,Time In Trade(in secs),EntrySideSym1,EntrySideSym2,NetTicksSym1,NetTicksSym2,NetPnL$,Best$UnRPnLSym1,Worst$UnRPnLSym1,Best$UnRPnLSym2,Worst$UnRPnLSym2,Best$UnRPnLSpread,Worst$UnRSpread" << "\n";
            
            // Output data
            //
            TPnLAuditStratStatsInfos::iterator iter = _pnLAuditStratStatsInfos.begin();
            TPnLAuditStratStatsInfos::iterator end = _pnLAuditStratStatsInfos.end();
            for ( ; iter != end; ++iter ) {
                const TPnLAuditStratStatsInfo& stratStatsInfo = iter->second;
                if ( !stratStatsInfo._statsInfos.empty() ) {
                    std::string name = stratStatsInfo._instrument1->_displayName;
                    if ( NULL != stratStatsInfo._instrument2.get() )
                        name += "_" + stratStatsInfo._instrument2->_displayName;

                    for ( size_t i = 0; i < stratStatsInfo._statsInfos.size(); ++i ) {
                        const TPnLAuditStatsInfo& statsInfo = stratStatsInfo._statsInfos[i];
                        file << stratStatsInfo._accountId 
                             << "," << stratStatsInfo._strategyId
                             << "," << name
                             << "," << (i+1)
                             << "," << statsInfo._timestamp1
                             << "," << statsInfo._timestamp2
                             << "," << ((1UL == statsInfo._timestamp2.getImpl().year()) ? 0 : ((statsInfo._timestamp2-statsInfo._timestamp1)/1000000))
                             << "," << statsInfo._entrySide1.toString()
                             << "," << statsInfo._entrySide2.toString()
                             << "," << statsInfo._netTicks1
                             << "," << statsInfo._netTicks2
                             << "," << statsInfo._netPnL
                             << "," << statsInfo._maxUnRPnL1
                             << "," << statsInfo._minUnRPnL1
                             << "," << statsInfo._maxUnRPnL2
                             << "," << statsInfo._minUnRPnL2
                             << "," << statsInfo._maxUnRPnLTotal
                             << "," << statsInfo._minUnRPnLTotal
                             << "\n";
                    }
                }
            }
            
            return true;
        }
        
        bool processNormal(const TPnLAuditTrailInfos& infos, const tw::common::Settings& settings, std::ofstream& file) {
            std::string t;
            std::string stratName;
            
            typedef std::map<std::string, double> TStratPeakPnL;
            TStratPeakPnL stratPeakPnl;            
            
            for ( size_t i = 0; i < infos.size(); ++i ) {
                if ( !_timestamps.empty() )
                    t = _timestamps.back();
                
                TPnLAuditTrailInfo info = infos[i];
                if ( ("ACCOUNT" == info._parentType && tw::common::ePnLAuditMode::kStrat == settings._pnl_audit_mode) ||
                     ("ACCOUNT" != info._parentType && tw::common::ePnLAuditMode::kAccount == settings._pnl_audit_mode) )
                    continue;
                
                if ( t != info._eventTimestamp.toString() )
                    _timestamps.push_back(info._eventTimestamp.toString());
                
                stratName = getStratName(info);
                TStratPeakPnL::iterator iter = stratPeakPnl.find(stratName);
                if ( iter == stratPeakPnl.end() )
                    iter = stratPeakPnl.insert(TStratPeakPnL::value_type(stratName, 0.0)).first;
                
                if ( iter->second < info._realizedInDollars )
                    iter->second = info._realizedInDollars;
                
                info._peakRealizedInDollars = iter->second;
                _stratsToPnLAuditTrailInfos[stratName][info._eventTimestamp.toString()] = info;
            }
            
            {
                TStratsToPnLAuditTrailInfos::iterator iter = _stratsToPnLAuditTrailInfos.begin();
                TStratsToPnLAuditTrailInfos::iterator end = _stratsToPnLAuditTrailInfos.end();
                std::string strats = "strats,";
                std::string header = "timestamp";
                for ( ; iter != end; ++iter ) {
                    header += ",$UnR PnL,$Real PnL,Total,Drawdown";
                    strats += iter->first;
                    
                    TTimestampToTPnLAuditTrailInfo::reverse_iterator iter2 = iter->second.rbegin();
                    if ( iter2 != iter->second.rend() ) {
                        const TPnLAuditTrailInfo& info = iter2->second;
                        if ( "ACCOUNT" != info._parentType) {
                            strats += "_" + info._displayName1;
                            if ( !info._displayName2.empty() )
                                strats += "_" + info._displayName2;
                        } else {
                            strats += "_ACCOUNT";
                        }
                    }
                    
                    strats += ",,,,";
                }
                    
                file << strats << "\n";
                file << header << "\n";
            }
            
            std::string r;
            std::string r2;
            TStratPreviousInfo stratPreviousInfo;
            for ( size_t i = 0; i < _timestamps.size(); ++i ) {
                t = r = _timestamps[i];
                TStratsToPnLAuditTrailInfos::iterator iter = _stratsToPnLAuditTrailInfos.begin();
                TStratsToPnLAuditTrailInfos::iterator end = _stratsToPnLAuditTrailInfos.end();
                for ( ; iter != end; ++iter ) {
                    stratName = iter->first;
                    TTimestampToTPnLAuditTrailInfo::iterator iter2 = iter->second.find(t);
                    if ( iter2 != iter->second.end() ) {
                        const TPnLAuditTrailInfo& info = iter2->second;
                        r2 = "," 
                           + boost::lexical_cast<std::string>(info._unrealizedInDollars) + "," 
                           + boost::lexical_cast<std::string>(info._realizedInDollars) + "," 
                           + boost::lexical_cast<std::string>(info._unrealizedInDollars + info._realizedInDollars) + "," 
                           + boost::lexical_cast<std::string>(-1*(info._peakRealizedInDollars-info._realizedInDollars)+info._unrealizedInDollars);
                        
                        r += r2;
                        stratPreviousInfo[stratName] = r2;
                     } else {
                        TStratPreviousInfo::iterator iter3 = stratPreviousInfo.find(stratName);
                        if ( iter3 != stratPreviousInfo.end() )
                            r += iter3->second;
                        else
                            r += ",0,0,0,0";
                     }
                }
                file << r << "\n";
            }
            
            return true;
        }
        
    private:
        std::string formatTimestamp(const tw::common::THighResTime& t) {
            return std::string(t.toString().substr(0,8) + "T" + boost::lexical_cast<std::string>(t.hour()) + "h" + boost::lexical_cast<std::string>(t.minute()) + "m" + boost::lexical_cast<std::string>(t.seconds()) + "s");
        }
        
        std::string formatTimestamp(const std::string& timestamp) {
            std::string t = timestamp;
            boost::replace_first(t, ":", "h");
            boost::replace_first(t, ":", "m");
            return t;
        }
        
        std::string getFileName(const tw::common::Settings& settings, const std::string account) {
            std::string fileName = _dirName + "/pnl_audit_";
            fileName += formatTimestamp(settings._pnl_audit_start_time) + "_" + formatTimestamp(settings._pnl_audit_end_time);
            fileName += "_" + account + ".csv";
            return fileName;
        }
        
    private:
        std::string getStratName(TPnLAuditTrailInfo& info) {
            setInstruments(info);
            return boost::lexical_cast<std::string>(info._accountId) + "_" + boost::lexical_cast<std::string>(info._strategyId);            
        }
        
        void setInstruments(TPnLAuditTrailInfo& info) {
            if ( !info._displayName1.empty() )
                info._instrument1 = tw::instr::InstrumentManager::instance().getByDisplayName(info._displayName1);
            
            if ( !info._displayName2.empty() )
                info._instrument2 = tw::instr::InstrumentManager::instance().getByDisplayName(info._displayName2);
        }
        
        void updateStatsInfo(const TPnLAuditTrailInfo& info, TPnLAuditStatsInfo& statsInfo) {
            if ( NULL != info._instrument1.get() ) {
                if ( 0 != info._maxInTicks1 )
                    statsInfo._maxUnRPnL1 = info._maxInTicks1 * info._instrument1->_tickValue;
                
                if ( 0 != info._minInTicks1 )
                    statsInfo._minUnRPnL1 = info._minInTicks1 * info._instrument1->_tickValue;
            }

            if ( NULL != info._instrument2.get() ) {
                if ( 0 != info._maxInTicks2 )
                    statsInfo._maxUnRPnL2 = info._maxInTicks2 * info._instrument2->_tickValue;
                
                if ( 0 != info._minInTicks2 )
                    statsInfo._minUnRPnL2 = info._minInTicks2 * info._instrument2->_tickValue;
            }

            statsInfo._maxUnRPnLTotal = info._maxInDollars;
            statsInfo._minUnRPnLTotal = info._minInDollars;
        }
        
    private:
        typedef std::vector<std::string> TTimestamps;
        typedef std::map<std::string, TPnLAuditTrailInfo> TTimestampToTPnLAuditTrailInfo;
        typedef std::map<std::string, TTimestampToTPnLAuditTrailInfo> TStratsToPnLAuditTrailInfos;
        
        std::string _dirName;
        TTimestamps _timestamps;
        TStratsToPnLAuditTrailInfos _stratsToPnLAuditTrailInfos;
        TPnLAuditStratStatsInfos _pnLAuditStratStatsInfos;
    };

} // namespace scalper
} // namespace tw
