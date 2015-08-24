<?xml version="1.0"?>
<xsl:transform xmlns:xs="http://www.w3.org/2001/XMLSchema"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:fn="http://www.w3.org/2005/02/xpath-functions"
               version="1.0">
					  
<xsl:output method="text" indent="no"/>
<xsl:strip-space elements="*"/>

<xsl:template match="Namespace">
// THIS IS generated code - do not modify. To make changes, modify appropriate
// xml/xsl files!
//
#pragma once

#include &lt;tw/common/defs.h&gt;
#include &lt;tw/common/uuid.h&gt;
#include &lt;tw/common_str_util/fast_stream.h&gt;
#include &lt;tw/common/command.h&gt;
#include &lt;tw/log/defs.h&gt;
#include &lt;tw/generated/channel_or_defs.h&gt;
#include &lt;tw/common_trade/bars_manager.h&gt;

#include &lt;boost/lexical_cast.hpp&gt;
#include &lt;boost/algorithm/string.hpp&gt;
#include &lt;boost/bind.hpp&gt;
#include &lt;boost/shared_ptr.hpp&gt;

#include &lt;string&gt;
#include &lt;vector&gt;
#include &lt;set&gt;
#include &lt;stdint.h&gt;

namespace tw {
namespace <xsl:value-of select="/Namespace/@name"/> {

// Enums 
//
<xsl:apply-templates select="." mode="enum"/>

// Data structures
//
<xsl:apply-templates select="." mode="decl"/>

} // namespace common_trade
} // namespace tw

</xsl:template>


<xsl:template match="Namespace/Defs" mode="enum">
<xsl:for-each select="*">
    <xsl:choose>
        <xsl:when test="@type='enum'">
struct <xsl:value-of select="name()"/> {
    enum _ENUM {
        kUnknown=0,
<xsl:for-each select="*">
        k<xsl:value-of select="name()"/>=<xsl:value-of select="position()"/><xsl:if test="position() != last()">,</xsl:if>			
</xsl:for-each>
    };
    
    <xsl:value-of select="name()"/> () {
        _enum = kUnknown;
    }
    
    <xsl:value-of select="name()"/> (_ENUM value) {
        _enum = value;
    }
    
    bool isValid() const {
        return (_enum != kUnknown);
    }
    
    operator _ENUM() const {
        return _enum;
    }

    const char* toString() const {
        switch (_enum) {
<xsl:for-each select="*">
            case k<xsl:value-of select="name()"/>: return "<xsl:value-of select="name()"/>"; </xsl:for-each>
            default: return "Unknown";
        }
    }  

    void fromString(const std::string&amp; value) {
<xsl:for-each select="*">
        if ( boost::iequals(value, "<xsl:value-of select="name()"/>") ) {
            _enum = k<xsl:value-of select="name()"/>;
            return; 
        }
</xsl:for-each>
        _enum = kUnknown;
    }
    
    friend tw::log::StreamDecorator&amp; operator&lt;&lt;(tw::log::StreamDecorator&amp; os, const <xsl:value-of select="name()"/>&amp; x) {
        return os &lt;&lt; x.toString();
    }

    friend std::ostream&amp; operator&lt;&lt;(std::ostream&amp; os, const <xsl:value-of select="name()"/>&amp; x) {
        return os &lt;&lt; x.toString();
    }
    
    friend std::istream&amp; operator&gt;&gt;(std::istream&amp; input, <xsl:value-of select="name()"/>&amp; x) {
        std::string value;
        input &gt;&gt; value;
        x.fromString(value);
        
        return input;
    } 

    _ENUM _enum;
};
        </xsl:when>
        <xsl:otherwise>
        </xsl:otherwise>
    </xsl:choose>
</xsl:for-each>
</xsl:template>

<xsl:template match="Namespace/Defs" mode="decl">
<xsl:for-each select="*">
    <xsl:choose>
        <xsl:when test="@type='struct'">
struct <xsl:value-of select="name()"/><xsl:if test="@parent!=''"> : public <xsl:value-of select="@parent"/> </xsl:if> {
<xsl:if test="@parent!=''">
    typedef <xsl:value-of select="@parent"/> TParent;
</xsl:if>
<xsl:if test="@serializable='true'">
    static const size_t FIELDS_NUM=<xsl:value-of select="count(*)"/>-<xsl:value-of select="count(*[@serializable='false'])"/><xsl:if test="@parent!=''">+TParent::FIELDS_NUM</xsl:if>;
</xsl:if>



<xsl:text>&#10;&#10;    </xsl:text>    
    <xsl:value-of select="name()"/>() {
        clear();
    }

    void clear() {
<xsl:if test="@parent!=''">
        TParent::clear();
</xsl:if>
<xsl:for-each select="*">
       _<xsl:value-of select="name()"/>=<xsl:value-of select="@type"/>(); </xsl:for-each>
    }

    bool isValid() const {
<xsl:for-each select="*">
    <xsl:if test="@valid_range!=''">
        if (!(_<xsl:value-of select="name()"/><xsl:value-of select="@valid_range"/><xsl:if test="@additional_validation!=''"><xsl:value-of select="@additional_validation"/></xsl:if>)) return false;</xsl:if>
</xsl:for-each>

        return true;
    }
    
<xsl:if test="@commandable='true'">
    static <xsl:value-of select="name()"/> fromCommand(const tw::common::Command&amp; cmnd) {
        <xsl:value-of select="name()"/> x;
        <xsl:if test="@parent">
        static_cast&lt;<xsl:value-of select="@parent"/>&amp;&gt;(x) = <xsl:value-of select="@parent"/>::fromCommand(cmnd);
        </xsl:if>
<xsl:for-each select="*">
    <xsl:if test="not(@serializable='false')">
        if ( cmnd.has(<xsl:text>"</xsl:text><xsl:value-of select="name()"/><xsl:text>"</xsl:text>) )
            cmnd.get(<xsl:text>"</xsl:text><xsl:value-of select="name()"/><xsl:text>"</xsl:text>, x._<xsl:value-of select="name()"/>);
    </xsl:if>
</xsl:for-each>
        return x;
    }
    
    tw::common::Command toCommand() const {
        tw::common::Command cmnd;
        <xsl:if test="@parent">
        cmnd = static_cast&lt;const <xsl:value-of select="@parent"/>&amp;&gt;(*this).toCommand();
        </xsl:if>
<xsl:for-each select="*">
    <xsl:if test="not(@serializable='false')">
        cmnd.addParams(<xsl:text>"</xsl:text><xsl:value-of select="name()"/><xsl:text>"</xsl:text>, _<xsl:value-of select="name()"/>);
    </xsl:if>
</xsl:for-each>
        return cmnd;
    }
</xsl:if>

<xsl:if test="@serializable='true'">    
    bool fromString(const std::string&amp; line) {
        std::vector&lt;std::string&gt; values;
        boost::split(values, line, boost::is_any_of(FIELDS_DELIM));
	
        size_t s1 = values.size();
        size_t s2 = FIELDS_NUM;
        if ( s1 &lt; s2 ) {
            LOGGER_ERRO &lt;&lt; "incorrect number of fields: " &lt;&lt; s1 &lt;&lt; " :: " &lt;&lt; s2 &lt;&lt; "\n"; 
            return false;
        }
            
        std::for_each(values.begin(), values.end(),
              boost::bind(boost::algorithm::trim&lt;std::string&gt;, _1, std::locale()));
              
<xsl:if test="@parent!=''">
        if ( !TParent::fromString(line) )
            return false;
</xsl:if>
        for ( size_t counter = 0<xsl:if test="@parent!=''">+TParent::FIELDS_NUM</xsl:if>; counter &lt; FIELDS_NUM; ++counter ) {
            if ( !values[counter].empty() ) {
                switch (counter) {
<xsl:for-each select="*">
    <xsl:if test="not(@serializable='false')">
                    case <xsl:value-of select="position()-1"/><xsl:if test="../@parent!=''">+TParent::FIELDS_NUM</xsl:if>:
                        _<xsl:value-of select="name()"/> = boost::lexical_cast&lt;<xsl:value-of select="@type"/>&gt;(values[counter]);
                        break;
    </xsl:if>
</xsl:for-each>
                } 
            }
        }

        return true;
    }
    
    std::string toString() const {
        std::stringstream out;
<xsl:if test="@parent!=''">
        out &lt;&lt; TParent::toString() &lt;&lt; FIELDS_DELIM;
</xsl:if>
<xsl:text>&#10;&#9;</xsl:text>
<xsl:for-each select="*">
        <xsl:if test="not(@serializable='false')"> out &lt;&lt; _<xsl:value-of select="name()"/><xsl:if test="position()!=last()"> &lt;&lt; FIELDS_DELIM </xsl:if>;
        </xsl:if>
</xsl:for-each>
        return out.str();
    }
    
    std::string toStringVerbose() const {
        std::stringstream out;
<xsl:if test="@parent!=''">
        out &lt;&lt; TParent::toStringVerbose();
</xsl:if>
<xsl:text>&#10;&#9;</xsl:text>
<xsl:for-each select="*">
        <xsl:if test="not(@serializable='false')"> out &lt;&lt; "<xsl:value-of select="name()"/>=" &lt;&lt; _<xsl:value-of select="name()"/> &lt;&lt; "\n";
        </xsl:if>
</xsl:for-each>
        return out.str();
    }

    friend tw::log::StreamDecorator&amp; operator&lt;&lt;(tw::log::StreamDecorator&amp; os, const <xsl:value-of select="name()"/>&amp; x) {
        return os &lt;&lt; x.toString();
    }
    
    friend std::ostream&amp; operator&lt;&lt;(std::ostream&amp; os, const <xsl:value-of select="name()"/>&amp; x) {
        return os &lt;&lt; x.toString();
    }

    friend bool operator&gt;&gt;(std::istream&amp; input, <xsl:value-of select="name()"/>&amp; x) {
        try {
            std::string value;
            input &gt;&gt; value;
            x.fromString(value);
        } catch(...) {
            return false;
        }
        
        return true;
    }
</xsl:if><xsl:text>&#10;</xsl:text>

<xsl:for-each select="*">
<xsl:text>    </xsl:text><xsl:value-of select="@type"/> _<xsl:value-of select="name()"/>;
</xsl:for-each>
};
        </xsl:when>	
        <xsl:otherwise>
        </xsl:otherwise>
    </xsl:choose>
</xsl:for-each>
</xsl:template>

</xsl:transform>

