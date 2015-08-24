#include <tw/price/defs.h>
#include <tw/price/ticks_converter.h>

#include <tw/generated/instrument.h>
#include <tw/instr/fwdDefs.h>
#include <tw/instr/instrument_manager.h>

typedef tw::instr::Instrument TInstrument;
typedef tw::instr::InstrumentPtr TInstrumentPtr;

typedef tw::price::TicksConverter TTickConverter;
typedef tw::price::Ticks TTicks;

struct InstrHelper {
    static TInstrumentPtr get(const std::string& name) {
        return tw::instr::InstrumentManager::instance().getByDisplayName(name);
    }
    
    static TInstrumentPtr getNQH2() {
        TInstrumentPtr instrument = get("NQH2");
        if ( instrument )
            return instrument;        
        
        instrument.reset(new TInstrument());
        std::string instrData = "6,Future,Equity,Cash,USD,20,1,4,2010-12-17 14:30:00.000,2012-03-16 13:30:00.000,2012-03-16 13:30:00.000,NQ,NQ,NQH2,,CME,CME,CME Globex,8870,8870,8870,8870,15,,0.01,5000,0,,0,,0,,0,,0,,,,,,,,,";
        
        instrument->fromString(instrData);
        
        tw::instr::InstrumentManager::setTickConverter(instrument);
        tw::instr::InstrumentManager::setPrecision(instrument);
        
        instrument->_precision = 2;
        
        tw::instr::InstrumentManager::instance().addInstrument(instrument);
        
        return instrument;
    }
    
    static TInstrumentPtr getNQU2() {
        TInstrumentPtr instrument = get("NQU2");
        if ( instrument )
            return instrument;        
        
        instrument.reset(new TInstrument());
        std::string instrData = "7,Future,Equity,Cash,USD,20,1,4,2010-12-17 14:30:00.000,2012-03-16 13:30:00.000,2012-03-16 13:30:00.000,NQ,NQ,NQU2,,CME,CME,CME Globex,8871,8871,8871,8871,15,,0.01,5000,0,,0,,0,,0,,0,,,,,,,,,";
        instrument->fromString(instrData);
        
        tw::instr::InstrumentManager::setTickConverter(instrument);
        tw::instr::InstrumentManager::setPrecision(instrument);
        
        tw::instr::InstrumentManager::instance().addInstrument(instrument);
        
        return instrument;
    }
    
    static TInstrumentPtr getNQM2() {
        TInstrumentPtr instrument = get("NQM2");
        if ( instrument )
            return instrument;        
        
        instrument.reset(new TInstrument());
        std::string instrData = "30,Future,Equity,Cash,USD,20,25,1,2011-03-18 13:30:00.000,2012-06-15 13:30:00.000,2012-06-15 13:30:00.000,NQ,NQ,NQM2,,CME,CME,CME Globex,20924,20924,20924,20924,15,,0.01,5,0,,0,,0,,0,,0,20120404,NetChangePreliminary,273650,";
        instrument->fromString(instrData);
        instrument->_feeExLiqAdd = -0.05;
        instrument->_feeExLiqRem = 0.06;
        instrument->_feeExClearing = 0.07;
        instrument->_feeBrokerage = 0.08;
        instrument->_feePerTrade = 0.03;
        
        tw::instr::InstrumentManager::setTickConverter(instrument);
        tw::instr::InstrumentManager::setPrecision(instrument);
        
        tw::instr::InstrumentManager::instance().addInstrument(instrument);
        
        return instrument;
    }
    
    static TInstrumentPtr getSIM2() {
        TInstrumentPtr instrument = get("SIM2");
        if ( instrument )
            return instrument;        
        
        instrument.reset(new TInstrument());
        std::string instrData = "60,Future,Metal,Cash,USD,5000,5,1,2012-03-29 21:30:00.000,2012-06-27 17:25:00.000,2012-06-27 17:25:00.000,SI,SI,SIM2,,CME,CEC,CME Globex,37726,37726,37726,37726,33,,0.001,25,0,,0,,0,,0,,0,20120404,NetChangePreliminary,31073,";
        
        instrument->fromString(instrData);
        instrument->_feeExLiqAdd = 0;
        instrument->_feeExLiqRem = 0;
        instrument->_feeExClearing = 0;
        instrument->_feeBrokerage = 0;
        instrument->_feePerTrade = 0;
        
        tw::instr::InstrumentManager::setTickConverter(instrument);
        tw::instr::InstrumentManager::setPrecision(instrument);
        
        tw::instr::InstrumentManager::instance().addInstrument(instrument);
        
        return instrument;
    }
    
    static TInstrumentPtr getZFM2() {
        TInstrumentPtr instrument = get("ZFM2");
        if ( instrument )
            return instrument;        
        
        instrument.reset(new TInstrument());
        std::string instrData = "38,Future,InterestRates,Cash,USD,100000,1,128,2011-03-31 21:30:00.000,2012-06-29 17:01:00.000,2012-06-29 17:01:00.000,ZB,ZF,ZFM2,,CME,CBOT,CME Globex,465961,465961,465961,465961,115,,1,7.8125,0,,0,,0,,0,,0,20120404,NetChangePreliminary,122.508,";
        
        
        instrument->fromString(instrData);
        instrument->_feeExLiqAdd = 0;
        instrument->_feeExLiqRem = 0;
        instrument->_feeExClearing = 0;
        instrument->_feeBrokerage = 0;
        instrument->_feePerTrade = 0;
        
        tw::instr::InstrumentManager::setTickConverter(instrument);
        tw::instr::InstrumentManager::setPrecision(instrument);
        
        tw::instr::InstrumentManager::instance().addInstrument(instrument);
        
        return instrument;
    }
    
    static TInstrumentPtr getZBM3() {
        TInstrumentPtr instrument = get("ZBM3");
        if ( instrument )
            return instrument;        
        
        instrument.reset(new TInstrument());
                
        instrument->_keyId = 471151;
        instrument->_contractSize = 100000;
        instrument->_instrType = tw::instr::eInstrType::kFuture;
        instrument->_category = tw::instr::eInstrCatg::kInterestRates;
        instrument->_settlType = tw::instr::eSettlType::kCash;
        instrument->_priceCurrency = tw::instr::eCurrency::kUSD;
        instrument->_tickNumerator = 1;
        instrument->_tickDenominator = 32;
        
        instrument->_firstTradingDay = "2012-09-19 21:30:00.000";
        instrument->_lastTradingDay = "2013-06-19 17:01:00.000";
        instrument->_expirationDate = "2013-06-19 17:01:00.000";
        
        instrument->_symbol = "ZB";
        instrument->_productCode = "ZB";
        instrument->_displayName = "ZBM3";
        instrument->_description = "";
        
        instrument->_exchange = tw::instr::eExchange::kCME;
        instrument->_subexchange = tw::instr::eSubExchange::kCBOT;
        instrument->_mdVenue = "CME Globex";
        
        instrument->_keyNum1 = 471151;
        instrument->_keyNum2 = 471151;
        instrument->_keyStr1 = "471151";
        instrument->_keyStr2 = "471151";
        instrument->_var1 = "115";
        instrument->_var2 = "";
        
        instrument->_displayFormat = 1;
        instrument->_tickValue = 31.25;
        
        instrument->_numberOfLegs = 0;
        
        instrument->_feeExLiqAdd = 0;
        instrument->_feeExLiqRem = 0;
        instrument->_feeExClearing = 0;
        instrument->_feeBrokerage = 0;
        instrument->_feePerTrade = 0;
        
        tw::instr::InstrumentManager::setTickConverter(instrument);
        tw::instr::InstrumentManager::setPrecision(instrument);
        
        tw::instr::InstrumentManager::instance().addInstrument(instrument);
        
        return instrument;
    }
    
    static TInstrumentPtr getZNM2() {
        TInstrumentPtr instrument = get("ZNM2");
        if ( instrument )
            return instrument;        
        
        instrument.reset(new TInstrument());
        std::string instrData = "42,Future,InterestRates,Cash,USD,100000,1,64,2011-03-22 21:30:00.000,2012-06-20 17:01:00.000,2012-06-20 17:01:00.000,ZB,ZN,ZNM2,,CME,CBOT,CME Globex,328264,328264,328264,328264,115,,1,15.625,0,,0,,0,,0,,0,20120404,NetChangePreliminary,129.375,";
        
        instrument->fromString(instrData);
        instrument->_feeExLiqAdd = -0.15;
        instrument->_feeExLiqRem = 0.16;
        instrument->_feeExClearing = 0.17;
        instrument->_feeBrokerage = 0.18;
        instrument->_feePerTrade = 0.13;
        
        tw::instr::InstrumentManager::setTickConverter(instrument);
        tw::instr::InstrumentManager::setPrecision(instrument);
        
        tw::instr::InstrumentManager::instance().addInstrument(instrument);
        
        return instrument;
    }
    
    static TInstrumentPtr get6CU2() {
        TInstrumentPtr instrument = get("6CU2");
        if ( instrument )
            return instrument;        
        
        instrument.reset(new TInstrument());
        std::string instrData = "15,Future,Currency,Cash,USD,100000,1,1,2011-03-15 21:49:00.000,2012-09-18 14:16:00.000,2012-09-18 14:16:00.000,6C,6C,6CU2,,CME,CME,CME Globex,67020,67020,67020,67020,60,,0.0001,10,0,,0,,0,,0,,0,20120718,Final,9876,FFCXSX,";
        
        instrument->fromString(instrData);
        instrument->_feeExLiqAdd = 0.07;
        instrument->_feeExLiqRem = 0.07;
        instrument->_feeExClearing = 0.25;
        instrument->_feeBrokerage = 0.1;
        instrument->_feePerTrade = 0.05;
        
        tw::instr::InstrumentManager::setTickConverter(instrument);
        tw::instr::InstrumentManager::setPrecision(instrument);
        
        tw::instr::InstrumentManager::instance().addInstrument(instrument);
        
        return instrument;
    }
    
    static TInstrumentPtr get6SU2() {
        TInstrumentPtr instrument = get("6SU2");
        if ( instrument )
            return instrument;        
        
        instrument.reset(new TInstrument());
        std::string instrData = "23,Future,Currency,Cash,USD,125000,1,1,2011-03-14 21:49:00.000,2012-09-17 14:16:00.000,2012-09-17 14:16:00.000,6S,6S,6SU2,,CME,CME,CME Globex,56108,56108,56108,56108,11,,0.0001,12.5,0,,0,,0,,0,,0,20120718,Final,10223,FFCXSX,";
        
        instrument->fromString(instrData);
        instrument->_feeExLiqAdd = 0.07;
        instrument->_feeExLiqRem = 0.07;
        instrument->_feeExClearing = 0.25;
        instrument->_feeBrokerage = 0.1;
        instrument->_feePerTrade = 0.05;
        
        tw::instr::InstrumentManager::setTickConverter(instrument);
        tw::instr::InstrumentManager::setPrecision(instrument);
        
        tw::instr::InstrumentManager::instance().addInstrument(instrument);
        
        return instrument;
    }
    
};
