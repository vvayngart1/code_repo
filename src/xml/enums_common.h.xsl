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

#include &lt;tw/log/defs.h&gt;

#include &lt;string&gt;
#include &lt;vector&gt;
#include &lt;stdint.h&gt;

#include &lt;boost/lexical_cast.hpp&gt;
#include &lt;boost/algorithm/string.hpp&gt;
#include &lt;boost/bind.hpp&gt;

namespace tw {
namespace <xsl:value-of select="/Namespace/@name"/> {

template &lt;uint32_t i&gt;
class PowOf2 {
    public:
        enum { RESULT = 2 * PowOf2&lt;i-1&gt;::RESULT };
};

template&lt;&gt;
class PowOf2&lt;0&gt; {
    public:
        enum { RESULT = 1 };
};

// Enums 
//
<xsl:apply-templates select="." mode="enum"/>

} // namespace common
} // namespace tw

</xsl:template>

<xsl:template match="Namespace/EnumDefs" mode="enum">
<xsl:for-each select="*">
    <xsl:choose>
        <xsl:when test="@type='enum'">
            <xsl:variable name="bitwise" select="@bitwise" />
struct <xsl:value-of select="name()"/> {
    enum _ENUM {
        kUnknown=0,
<xsl:for-each select="*">
        k<xsl:value-of select="name()"/>=<xsl:choose><xsl:when test="$bitwise='true'">PowOf2&lt;<xsl:value-of select="position()-1"/>&gt;::RESULT</xsl:when><xsl:otherwise><xsl:value-of select="position()"/></xsl:otherwise></xsl:choose><xsl:if test="position() != last()">,</xsl:if>
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

</xsl:transform>