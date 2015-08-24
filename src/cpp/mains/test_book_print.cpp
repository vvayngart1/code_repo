#include <tw/log/defs.h>
#include <tw/price/quote.h>

void printQuote(tw::price::Quote& quote) {
    LOGGER_INFO << quote.toString() << "\n";
    
    quote.clearStatus();
    quote.clearFlag();
}

int main(int argc, char * argv[]) {
    tw::price::Quote quote;
    
    LOGGER_INFO << quote.toString() << "\n";
    
    quote._seqNum = 12345678;
    quote._exTimestamp = tw::common::THighResTime::parseCMETime("20111206-16:41:21.225765");
    quote._timestamp1.setToNow();
    quote._timestamp2.setToNow();
    quote._timestamp3.setToNow();
    
    quote._open.set(10785);
    quote._high.set(10798);
    quote._low.set(10756);
    
    quote.setTrade(tw::price::Ticks(10785), tw::price::Size(5));
    printQuote(quote);
    
    quote.setBid(tw::price::Ticks(10785), tw::price::Size(13),0,8);
    printQuote(quote);
    quote.setBid(tw::price::Ticks(10784), tw::price::Size(23),1,7);
    printQuote(quote);
    quote.setBid(tw::price::Ticks(10783), tw::price::Size(11),2,6);
    printQuote(quote);
    quote.setBid(tw::price::Ticks(10782), tw::price::Size(18),3,9);
    printQuote(quote);
    quote.setBid(tw::price::Ticks(10781), tw::price::Size(10),4,5);
    printQuote(quote);
    
    quote.setAsk(tw::price::Ticks(10787), tw::price::Size(5),0,5);
    printQuote(quote);
    quote.setAsk(tw::price::Ticks(10788), tw::price::Size(17),1,7);
    printQuote(quote);
    quote.setAsk(tw::price::Ticks(10789), tw::price::Size(17),2,6);
    printQuote(quote);
    quote.setAsk(tw::price::Ticks(10790), tw::price::Size(31),3,10);
    printQuote(quote);
    quote.setAsk(tw::price::Ticks(10791), tw::price::Size(18),4,8);
    printQuote(quote);
        
    quote.setOpen(tw::price::Ticks(10786));
    quote.setHigh(tw::price::Ticks(10797));
    quote.setLow(tw::price::Ticks(10755));
    
    printQuote(quote);
    LOGGER_INFO << quote.statsToCommand().toString() << "\n";
    
    return 0;
}

