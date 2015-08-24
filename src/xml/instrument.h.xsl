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
#include &lt;tw/common_str_util/fast_stream.h&gt;
#include &lt;tw/price/fwdDefs.h&gt;
#include &lt;tw/price/defs.h&gt;
#include &lt;tw/instr/fwdDefs.h&gt;
#include &lt;tw/channel_db/channel_db.h&gt;
#include &lt;tw/log/defs.h&gt;

#include &lt;boost/lexical_cast.hpp&gt;
#include &lt;boost/algorithm/string.hpp&gt;
#include &lt;boost/bind.hpp&gt;

#include &lt;string&gt;
#include &lt;vector&gt;
#include &lt;stdint.h&gt;

namespace tw {
namespace <xsl:value-of select="/Namespace/@name"/> {

// Enums 
//
<xsl:apply-templates select="." mode="enum"/>

// Data structures
//
<xsl:apply-templates select="." mode="decl"/>

// Database related structures
//
<xsl:apply-templates select="." mode="db"/>

} // namespace instr
} // namespace tw

</xsl:template>


<xsl:template match="Namespace/InstrumentsDefs" mode="enum">
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

<xsl:template match="Namespace/InstrumentsDefs" mode="decl">
<xsl:for-each select="*">
    <xsl:choose>
        <xsl:when test="@type='struct'">
struct <xsl:value-of select="name()"/> {
<xsl:if test="@serializable='true'">
    static const size_t FIELDS_NUM=<xsl:value-of select="count(*)"/>-<xsl:value-of select="count(*[@serializable='false'])"/><xsl:if test="@parent!=''">+TParent::FIELDS_NUM</xsl:if>;
</xsl:if>

<xsl:if test="@header='true'">
    static const std::string&amp; header() {
        static std::string h = "<xsl:for-each select="*"><xsl:if test="not(@serializable='false')"><xsl:if test="position()!=1">,</xsl:if><xsl:value-of select="name()"/></xsl:if></xsl:for-each>";
        return h;
    }
</xsl:if>

<xsl:if test="name()='Instrument'">
    typedef <xsl:for-each select="*"><xsl:if test="name()='keyId'"><xsl:value-of select="@type"/></xsl:if></xsl:for-each> TKeyId;
</xsl:if>
<xsl:text>&#10;&#10;    </xsl:text>    
    <xsl:value-of select="name()"/>() {
        clear();
    }

    void clear() {
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
    bool isEqual(const <xsl:value-of select="name()"/>&amp; rhs) const {
<xsl:for-each select="*">
    <xsl:if test="not(@serializable='false')">
        if ( _<xsl:value-of select="name()"/> != rhs._<xsl:value-of select="name()"/> )
            return false;
    </xsl:if>
</xsl:for-each>

        return true;
    }
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

        for ( size_t counter = 0; counter &lt; FIELDS_NUM; ++counter ) {
            if ( !values[counter].empty() ) {
                switch (counter) {
<xsl:for-each select="*">
    <xsl:if test="not(@serializable='false')">
                    case <xsl:value-of select="position()-1"/>:
                        _<xsl:value-of select="name()"/> = boost::lexical_cast&lt;<xsl:value-of select="@type"/>&gt;(values[counter]);
                        break;
    </xsl:if>
</xsl:for-each>
                } 
            }
        }

        return true;
    }
    
    std::string getHeader() const {
        std::stringstream out;
<xsl:text>&#10;&#9;&#9;</xsl:text>
<xsl:for-each select="*">
        <xsl:if test="not(@serializable='false')"> out &lt;&lt; "<xsl:value-of select="name()"/>"<xsl:if test="position()!=last()"> &lt;&lt; FIELDS_DELIM </xsl:if>;
        </xsl:if>
</xsl:for-each>
        return out.str();
    }
    
    std::string toString() const {
        std::stringstream out;
<xsl:text>&#10;&#9;&#9;</xsl:text>
<xsl:for-each select="*">
        <xsl:if test="not(@serializable='false')"> out &lt;&lt; _<xsl:value-of select="name()"/><xsl:if test="position()!=last()"> &lt;&lt; FIELDS_DELIM </xsl:if>;
        </xsl:if>
</xsl:for-each>
        out &lt;&lt; "\n";
        return out.str();
    }

    
    std::string toStringVerbose() const {
        std::stringstream out;
<xsl:text>&#10;&#9;&#9;</xsl:text>
<xsl:for-each select="*">
        <xsl:if test="not(@serializable='false')"> out &lt;&lt; "<xsl:value-of select="name()"/>=" &lt;&lt; _<xsl:value-of select="name()"/> &lt;&lt; "\n";
        </xsl:if>
</xsl:for-each>
        out &lt;&lt; "\n";
        return out.str();
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
</xsl:if><xsl:text>&#10;</xsl:text>

<xsl:if test="@addable='true'">
    void operator+=(const <xsl:value-of select="name()"/>&amp; x) {
<xsl:for-each select="*">
        _<xsl:value-of select="name()"/> += x._<xsl:value-of select="name()"/>;
</xsl:for-each>
    }
    
    void operator-=(const <xsl:value-of select="name()"/>&amp; x) {
<xsl:for-each select="*">
        _<xsl:value-of select="name()"/> -= x._<xsl:value-of select="name()"/>;
</xsl:for-each>
    }
    
    double total() const {
        double res = 0.0;        
<xsl:for-each select="*">
        res += _<xsl:value-of select="name()"/>;
</xsl:for-each>        
        return res;
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


<!-- Database related code generation -->
<!-- -->
<xsl:template match="Namespace/SQLDefs/Queries" mode="db">
<xsl:for-each select="*">
struct  <xsl:value-of select="name()"/> {
    bool execute(<xsl:choose><xsl:when test="Type/@value='SELECT'">tw::channel_db::ChannelDb&amp; channelDb<xsl:for-each select="Params/*">, const <xsl:value-of select="@type"/>&amp; <xsl:value-of select="name()"/></xsl:for-each></xsl:when><xsl:otherwise>tw::channel_db::ChannelDb::TStatementPtr statement<xsl:apply-templates select="Params/*" mode="paramList"/></xsl:otherwise></xsl:choose>)
    {
        bool status = true;
        std::stringstream sql;
        try {
        <xsl:choose>
            <xsl:when test="Type/@value='SELECT'">
                    <xsl:text>&#9;sql &lt;&lt; "SELECT </xsl:text>
                    <xsl:apply-templates select="Outputs/*" mode="paramFields"/>
                    <xsl:text> FROM </xsl:text><xsl:value-of select="Source/@value"/>
                    <xsl:if test="Filter">
                        <xsl:variable name = "x"><xsl:value-of select="Filter/@value"/></xsl:variable> 
                        <xsl:text> WHERE </xsl:text>
                        <xsl:choose>
                            <!-- TODO: right now, only one pareameter is supported, need to change to use 'tokenize' method later -->
                            <xsl:when test="Params">
                                <xsl:variable name = "y"><xsl:value-of select="$x"/></xsl:variable>
                                <xsl:for-each select="Params/*">
                                    <xsl:value-of select="substring-before($y, '?')"/><xsl:text>"</xsl:text>
                                    <xsl:text> &lt;&lt; </xsl:text><xsl:value-of select="name()"/>
                                    <xsl:text> &lt;&lt; "</xsl:text><xsl:value-of select="substring-after($y, '?')"/>
                                </xsl:for-each>
                            </xsl:when>
                            <xsl:otherwise>
                                <xsl:value-of select="$x"/>
                            </xsl:otherwise>    
                        </xsl:choose>
                    </xsl:if>   
                    <xsl:text>";</xsl:text>                    
                    
                    <xsl:for-each select="Outputs/*">
                        <xsl:text>&#10;&#9;&#9;tw::instr::</xsl:text>
                        <xsl:value-of select="name()"/>
                        <xsl:text> o</xsl:text>
                        <xsl:value-of select="position()"/>
                        <xsl:text>;</xsl:text>
                    </xsl:for-each>

                    <xsl:text>&#10;&#9;&#9;tw::channel_db::ChannelDb::TConnectionPtr connection = channelDb.getConnection();</xsl:text>
                    <xsl:text>&#10;&#9;&#9;tw::channel_db::ChannelDb::TStatementPtr statement = channelDb.getStatement(connection);</xsl:text>
                    <xsl:text>&#10;&#9;&#9;tw::channel_db::ChannelDb::TResultSetPtr res = channelDb.executeQuery(statement, sql.str());</xsl:text>
                    <xsl:text>&#10;&#10;&#9;&#9;while (res-&gt;next()) {</xsl:text>
                    <xsl:apply-templates select="Outputs/*" mode="paramFieldsGet"/>
                    <xsl:for-each select="Outputs/*">
                        <xsl:text>&#10;&#10;&#9;&#9;&#9;_o</xsl:text>            
                        <xsl:value-of select="position()"/>
                        <xsl:text>.push_back(o</xsl:text>            
                        <xsl:value-of select="position()"/>
                        <xsl:text>);</xsl:text>
                    </xsl:for-each>
                    <xsl:text>&#10;&#9;&#9;}</xsl:text>
            </xsl:when>
            <xsl:when test="(Type/@value='REPLACE' or Type/@value='INSERT')">
                    <xsl:text>&#10;&#9;&#9;sql &lt;&lt; "</xsl:text>
                    <xsl:if test="Type/@value='REPLACE'">
                        <xsl:text>REPLACE</xsl:text>
                    </xsl:if>
                    <xsl:if test="Type/@value='INSERT'">
                        <xsl:text>INSERT</xsl:text>
                    </xsl:if>
                    <xsl:text> INTO </xsl:text>
                    <xsl:value-of select="Source/@value"/>
                    <xsl:text> (</xsl:text>
                    <xsl:apply-templates select="Params/*" mode="paramFields"/>
                    <xsl:text>) VALUES ("</xsl:text>
                    <xsl:apply-templates select="Params/*" mode="paramFieldsSet"/>
                    <xsl:text>&#10;&#9;&#9;&#9; &lt;&lt;")</xsl:text>
                    <xsl:if test="UpdateStatement">
                        <xsl:text> </xsl:text><xsl:value-of select="UpdateStatement/@value"/>
                    </xsl:if>
                    <xsl:text>";</xsl:text>
                    
                    <xsl:text>&#10;&#9;&#9;statement-&gt;execute(sql.str());</xsl:text>
            </xsl:when>
            <xsl:otherwise>
            </xsl:otherwise>
	</xsl:choose>
        } catch(const std::exception&amp; e) {            
            status = false;
            LOGGER_ERRO &lt;&lt; "Exception: "  &lt;&lt; e.what() &lt;&lt; "\n" &lt;&lt; "\n";
        } catch(...) {
            status = false;
            LOGGER_ERRO &lt;&lt; "Exception: UNKNOWN" &lt;&lt; "\n" &lt;&lt; "\n";        
        }
        
        if ( !status ) {
            try {
                LOGGER_ERRO &lt;&lt; "Failed to execute sql: " &lt;&lt; sql.str() &lt;&lt; "\n" &lt;&lt; "\n";
            } catch(...) {
            }
        }
        
        return status;
    }
    <xsl:for-each select="Outputs/*">
       <xsl:text>std::vector&lt;tw::instr::</xsl:text>
       <xsl:value-of select="name()"/>
       <xsl:text>&gt; _o</xsl:text>
       <xsl:value-of select="position()"/>
       <xsl:text>;</xsl:text>
    </xsl:for-each>
};

</xsl:for-each>
</xsl:template>

<xsl:template match="*" mode="paramList">
    <xsl:text>, const </xsl:text><xsl:value-of select="name()"/><xsl:text>&amp; </xsl:text>p<xsl:value-of select="position()"/>
</xsl:template>

<xsl:template match="*" mode="paramFields">
    <xsl:call-template name="paramFields">
        <xsl:with-param name="item" select="name()" />
    </xsl:call-template>
    <xsl:if test="position() != last()">, </xsl:if>
</xsl:template>

<xsl:template name="paramFields">
     <xsl:param name="item"/>
     
     <xsl:for-each select="/Namespace/InstrumentsDefs/*">        
        <xsl:if test="$item = name()">
            <xsl:if test="@parent!=''">
                <xsl:call-template name="paramFields">
                    <xsl:with-param name="item" select="@parent" />
                </xsl:call-template>
                <xsl:text>, </xsl:text>
            </xsl:if>            
            <xsl:for-each select="*[not(@serializable='false')]">
                <xsl:value-of select="name()"/><xsl:if test="position() != last()">, </xsl:if>
            </xsl:for-each>
        </xsl:if>
    </xsl:for-each>        
</xsl:template>

<xsl:template match="*" mode="paramFieldsGet">
    <xsl:call-template name="paramFieldsGet">
        <xsl:with-param name="item" select="name()" />
        <xsl:with-param name="count" select="position()" />
    </xsl:call-template>
</xsl:template>

<xsl:template name="paramFieldsGet">
     <xsl:param name="item"/>
     <xsl:param name="count"/>
     
     <xsl:for-each select="/Namespace/InstrumentsDefs/*">        
        <xsl:if test="$item = name()">
            <xsl:if test="@parent!=''">
                <xsl:call-template name="paramFieldsGet">
                    <xsl:with-param name="item" select="@parent" />
                    <xsl:with-param name="count" select="$count" />
                </xsl:call-template>
            </xsl:if>            
            <xsl:for-each select="*[not(@serializable='false')]">
                <xsl:text>&#10;&#9;&#9;&#9;o</xsl:text>
                <xsl:value-of select="$count"/>
                <xsl:text>._</xsl:text>
                <xsl:value-of select="name()"/>
                <xsl:text>=boost::lexical_cast&lt;</xsl:text>
                <xsl:value-of select="@type"/>
                <xsl:text>&gt;(</xsl:text>
                <xsl:text>res-&gt;getString("</xsl:text>
                <xsl:value-of select="name()"/>
                <xsl:text>"));</xsl:text>
            </xsl:for-each>
        </xsl:if>
    </xsl:for-each>        
</xsl:template>


<xsl:template match="*" mode="paramFieldsSet">
    <xsl:call-template name="paramFieldsSet">
        <xsl:with-param name="item" select="name()" />
        <xsl:with-param name="count" select="position()" />
    </xsl:call-template>
    <xsl:if test="position() != last()"> &lt;&lt; "," </xsl:if>
</xsl:template>

<xsl:template name="paramFieldsSet">
     <xsl:param name="item"/>
     <xsl:param name="count"/>
     
     <xsl:for-each select="/Namespace/InstrumentsDefs/*">        
        <xsl:if test="$item = name()">
            <xsl:if test="@parent!=''">
                <xsl:call-template name="paramFieldsSet">
                    <xsl:with-param name="item" select="@parent" />
                    <xsl:with-param name="count" select="$count" />
                </xsl:call-template>
                <xsl:text> &lt;&lt; ","</xsl:text>
            </xsl:if>            
            <xsl:for-each select="*[not(@serializable='false')]">
                <xsl:choose>
                    <xsl:when test="@type='double' or @type='float'">
                        <xsl:text>&#10;&#9;&#9;&#9; &lt;&lt; "'" &lt;&lt; std::replace_all(tw::common_str_util::to_string(p</xsl:text>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:text>&#10;&#9;&#9;&#9; &lt;&lt; "'" &lt;&lt; std::replace_all(boost::lexical_cast&lt;std::string&gt;(p</xsl:text>
                    </xsl:otherwise>    
                </xsl:choose>
                <xsl:value-of select="$count"/>
                <xsl:text>._</xsl:text>
                <xsl:value-of select="name()"/>
                <xsl:text>)</xsl:text>
                <xsl:text>, "'", "\\'") &lt;&lt; "'"</xsl:text>
                <xsl:if test="position() != last()">
                    <xsl:text> &lt;&lt; ","</xsl:text>
                </xsl:if>
            </xsl:for-each>
        </xsl:if>
    </xsl:for-each>        
</xsl:template>

</xsl:transform>

