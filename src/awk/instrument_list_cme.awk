#!/bin/awk -f
BEGIN{i=0; print "id,itype,category,settlType,priceCurrency,contractSize,tickNumerator,tickDenominator,marginInitial,marginMaintenance,marginCurrency,firstTradingDay,lastTradingDay,exchangeOpen,exchangeClose,expirationDate,propRollDate,propOpen,propClose,symbol,displayName,description,exchange,subexchange,mdVenue,keyNum1,keyNum2,keyStr1,keyStr2,var1,var2,displayFormat,tickValue,numberOfLegs,displayNameLeg1,ratioLeg1,displayNameLeg2,ratioLeg2,displayNameLeg3,ratioLeg3,displayNameLeg4,ratioLeg4,settlDate,settlPriceType,settlPrice,feeExLiqAdd,feeExLiqRem,feeClearing,feeBrokerage,feePerTrade";}
{
    print ++i",,,,,,,,,,,,,,,,,,,"$1","$1"H2,,CME,,,,,,,,,,,,,,,,,,,,,,,,,,,,"
    print ++i",,,,,,,,,,,,,,,,,,,"$1","$1"M2,,CME,,,,,,,,,,,,,,,,,,,,,,,,,,,,"
    print ++i",,,,,,,,,,,,,,,,,,,"$1","$1"U2,,CME,,,,,,,,,,,,,,,,,,,,,,,,,,,,"
    print ++i",,,,,,,,,,,,,,,,,,,"$1","$1"Z2,,CME,,,,,,,,,,,,,,,,,,,,,,,,,,,,"
}

# The following command can be used to get the list of all cme channels to get product definitions
#
#cat /opt/onix/pf/2.13.0.0/config/config.xml | grep -i "channel id=" | awk 'BEGIN { FS="channel id=\""} {print $2;}' | awk 'BEGIN { FS="\""; ch="";} {print $1; ch=ch sprintf("%d,",$1)} END { print ch; }'

# The following command can be used to get the list of all cme channels to get product definitions for FUTURES ONLY
#
#cat /opt/onix/pf/2.13.0.0/config/config.xml | grep -i "channel id=" | grep -i future | awk 'BEGIN { FS="channel id=\""} {print $2;}' | awk 'BEGIN { FS="\""; ch="";} {print $1; ch=ch sprintf("%d,",$1)} END { print ch; }'

# The following command can be used to generate the list of symbols
#cat ../../etc/config_tw/instruments/instruments_list.config | ./instrument_list_cme.awk
