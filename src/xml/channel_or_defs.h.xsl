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

#include &lt;tw/price/defs.h&gt;
#include &lt;tw/common/defs.h&gt;
#include &lt;tw/common/uuid.h&gt;
#include &lt;tw/common/high_res_time.h&gt;
#include &lt;tw/common_str_util/fast_stream.h&gt;
#include &lt;tw/common/command.h&gt;
#include &lt;tw/log/defs.h&gt;
#include &lt;tw/functional/delegate.hpp&gt;
#include &lt;tw/channel_db/channel_db.h&gt;

#include &lt;tw/generated/enums_common.h&gt;
#include &lt;tw/generated/instrument.h&gt;
#include &lt;tw/generated/risk_defs.h&gt;

#include &lt;boost/lexical_cast.hpp&gt;
#include &lt;boost/algorithm/string.hpp&gt;
#include &lt;boost/bind.hpp&gt;
#include &lt;boost/shared_ptr.hpp&gt;

#include &lt;string&gt;
#include &lt;vector&gt;
#include &lt;stdint.h&gt;

namespace tw {
namespace <xsl:value-of select="/Namespace/@name"/> {

typedef tw::risk::TStrategyId TStrategyId;
typedef tw::risk::TAccountId TAccountId;
typedef tw::common::TUuidBuffer TOrderId;
typedef tw::common::TUuidBuffer TFillId;

// Enums 
//
<xsl:apply-templates select="." mode="enum"/>

// Forward declaration
//
<xsl:apply-templates select="." mode="fwd_decl"/>


// Callbacks class
//
<xsl:apply-templates select="." mode="callbacks"/>

// Data structures
//
<xsl:apply-templates select="." mode="decl"/>

// Database related structures
//
<xsl:apply-templates select="." mode="db"/>


} // namespace channel_or
} // namespace tw

</xsl:template>


<xsl:template match="Namespace/OrdersDefs" mode="enum">
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

<xsl:template match="Namespace/OrdersDefs" mode="fwd_decl">
<xsl:for-each select="*">
    <xsl:choose>
        <xsl:when test="@type='struct'">
struct <xsl:value-of select="name()"/>;
            <xsl:if test="@shared_ptr='true'">
typedef boost::shared_ptr&lt;<xsl:value-of select="name()"/>&gt; T<xsl:value-of select="name()"/>Ptr;
            </xsl:if>
        </xsl:when>	
        <xsl:otherwise>
        </xsl:otherwise>
    </xsl:choose>
</xsl:for-each>
</xsl:template>

<xsl:template match="Namespace/OrdersDefs" mode="callbacks">
<xsl:for-each select="*">
    <xsl:choose>
        <xsl:when test="@type='callbacks'">
class <xsl:value-of select="name()"/> {
public:
<xsl:for-each select="*">
    typedef tw::functional::delegate<xsl:value-of select="@args_num"/>&lt;<xsl:value-of select="@return_type"/>, <xsl:value-of select="@args"/>&gt; TCB_<xsl:value-of select="name()"/>;
</xsl:for-each>
<xsl:for-each select="*">    
    template &lt;typename TClient&gt;
    static TCB_<xsl:value-of select="name()"/> createCB_<xsl:value-of select="name()"/>(TClient* client) {
        return TCB_<xsl:value-of select="name()"/>::from_method&lt;TClient, &amp;TClient::<xsl:value-of select="name()"/>&gt;(client);
    }
</xsl:for-each>
public:
    <xsl:value-of select="name()"/>() {
        clear();
    }
    
    void clear() {<xsl:for-each select="*">
        _<xsl:value-of select="name()"/>.clear();</xsl:for-each>
    }
    
public:
    template &lt;typename TClient&gt;
    bool registerClient(TClient* client) {
        clear();
        
        <xsl:for-each select="*">
        _<xsl:value-of select="name()"/> = createCB_<xsl:value-of select="name()"/>(client);</xsl:for-each>
        
        if ( !client )
            return false;            
            
        return true;
    }
    
public:
<xsl:for-each select="*">
<xsl:text>    </xsl:text><xsl:value-of select="@return_type"/><xsl:text> </xsl:text><xsl:value-of select="name()"/>(<xsl:value-of select="@args_full"/>) {
<xsl:text>        </xsl:text>if ( !_<xsl:value-of select="name()"/>.empty() )
          _<xsl:value-of select="name()"/>(<xsl:value-of select="@args_names"/>);
    }
    
</xsl:for-each>
private:
<xsl:for-each select="*">
<xsl:text>    </xsl:text>TCB_<xsl:value-of select="name()"/> _<xsl:value-of select="name()"/>;    
</xsl:for-each>    
};
        </xsl:when>	
        <xsl:otherwise>
        </xsl:otherwise>
    </xsl:choose>
</xsl:for-each>
</xsl:template>

<xsl:template match="Namespace/OrdersDefs" mode="decl">
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

<xsl:if test="@header='true'">
    static const std::string&amp; header() {
        static std::string h = <xsl:if test="@parent!=''">TParent::header()+","+</xsl:if><xsl:for-each select="*"><xsl:if test="not(@serializable='false')"><xsl:if test="position()=1">"</xsl:if><xsl:if test="position()!=1">,</xsl:if><xsl:value-of select="name()"/></xsl:if></xsl:for-each>";
        return h;
    }
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

<!-- Database related code generation -->
<!-- -->
<xsl:template name="token_substitute_filter">
    <xsl:param name="string"/>
    <xsl:param name="separator"/>
    <xsl:param name="substitute"/>
    <xsl:param name="index"/>
    <xsl:param name="count"/>
    <xsl:param name="size"/>
    
    <xsl:if test="string-length($string) > 0">
        <xsl:variable name="before-separator" select="substring-before($string, $separator)"/>
        <xsl:variable name="after-separator" select="substring-after($string, $separator)"/>
        <xsl:choose>
            <!-- separator not found in string -->
            <xsl:when test="string-length($before-separator)=0 and string-length($after-separator)=0">
                <xsl:value-of select="$string"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:choose>
                    <xsl:when test="$count=$index">
                        <xsl:value-of select="$before-separator"/>
                        <xsl:text>" &lt;&lt; </xsl:text>
                        <xsl:value-of select="$substitute"/>
                        <xsl:text> &lt;&lt; "</xsl:text>
                        <xsl:if test="$index=$size">
                            <xsl:value-of select="$after-separator"/>
                        </xsl:if>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:call-template name="token_substitute_filter">
                            <xsl:with-param name="string" select="$after-separator"/>
                            <xsl:with-param name="separator" select="$separator"/>
                            <xsl:with-param name="substitute" select="$substitute"/>
                            <xsl:with-param name="index" select="$index"/>
                            <xsl:with-param name="count" select="$count+1"/>
                            <xsl:with-param name="size" select="$size"/>
                        </xsl:call-template>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:if>
</xsl:template>

<xsl:template name="paramSqlSubstitution">
    <xsl:param name="string"/>
    <xsl:param name="separator"/>
    <xsl:param name="substitute"/>
    <xsl:param name="index"/>
    <xsl:param name="size"/>
    
    <xsl:call-template name="token_substitute_filter">
        <xsl:with-param name="string" select="$string"/>
        <xsl:with-param name="separator" select="$separator"/>
        <xsl:with-param name="substitute" select="$substitute"/>
        <xsl:with-param name="index" select="$index"/>
        <xsl:with-param name="count" select="1"/>
        <xsl:with-param name="size" select="$size"/>
    </xsl:call-template>
</xsl:template>

<xsl:template match="Namespace/SQLDefs/Queries" mode="db">
<xsl:for-each select="*">
struct  <xsl:value-of select="name()"/> {
    <xsl:choose>
        <xsl:when test="Type/@value='SELECT'">
    bool execute(tw::channel_db::ChannelDb&amp; channelDb<xsl:for-each select="Params/*">, const <xsl:value-of select="@type"/>&amp; <xsl:value-of select="name()"/></xsl:for-each>)
    {
        bool status = true;
        std::stringstream sql;
        try {
            <xsl:text>&#9;sql &lt;&lt; "SELECT </xsl:text>
            <xsl:apply-templates select="Outputs/*" mode="paramFields"/>
            <xsl:text> FROM </xsl:text><xsl:value-of select="Source/@value"/>
            <xsl:if test="Filter">
                <xsl:variable name = "x"><xsl:value-of select="Filter/@value"/></xsl:variable> 
                <xsl:text> WHERE </xsl:text>
                <xsl:choose>
                    <xsl:when test="Params">
                        <xsl:variable name = "y"><xsl:value-of select="$x"/></xsl:variable>
                        <xsl:for-each select="Params/*">
                            <xsl:call-template name="paramSqlSubstitution">
                                <xsl:with-param name="string" select="$x"/>
                                <xsl:with-param name="separator" select="'?'"/>
                                <xsl:with-param name="substitute" select="name()"/>
                                <xsl:with-param name="index" select="position()"/>
                                <xsl:with-param name="size" select="last()"/>
                            </xsl:call-template>
                        </xsl:for-each>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:value-of select="$x"/>
                    </xsl:otherwise>    
                </xsl:choose>
            </xsl:if>   
            <xsl:text>";</xsl:text>                    

            <xsl:for-each select="Outputs/*">
                <xsl:text>&#10;&#9;&#9;tw::channel_or::</xsl:text>
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
        </xsl:when>
        <xsl:otherwise>
    uint32_t _count;
    std::stringstream _sql;
    
    <xsl:value-of select="name()"/>() {
        clear();
    }
    
    void clear() {
        _count = 0;
        _sql.str("");        
        <xsl:text>&#10;&#9;_sql &lt;&lt; "</xsl:text>
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
        <xsl:text>) VALUES ";</xsl:text>
    }
    
    uint32_t count() const {
        return _count;
    }
    
    void add(<xsl:apply-templates select="Params/*" mode="paramList"/>) {
        if ( ++_count > 1 )
            _sql &lt;&lt; ",";
           
        _sql &lt;&lt; "("<xsl:apply-templates select="Params/*" mode="paramFieldsSet"/>
             &lt;&lt; ")";
    }
    
    bool execute(tw::channel_db::ChannelDb::TStatementPtr statement)
    {
        bool status = true;
        try {
            if ( _count == 0 )
                return false;            
            <xsl:if test="UpdateStatement">_sql &lt;&lt; "<xsl:value-of select="UpdateStatement/@value"/>";</xsl:if>
            statement-&gt;execute(_sql.str());
        } catch(const std::exception&amp; e) {            
            status = false;
            LOGGER_ERRO &lt;&lt; "Exception: "  &lt;&lt; e.what() &lt;&lt; "\n" &lt;&lt; "\n";
        } catch(...) {
            status = false;
            LOGGER_ERRO &lt;&lt; "Exception: UNKNOWN" &lt;&lt; "\n" &lt;&lt; "\n";        
        }
        
        if ( !status ) {
            try {
                LOGGER_ERRO &lt;&lt; "Failed to execute sql: " &lt;&lt; _sql.str() &lt;&lt; "\n" &lt;&lt; "\n";
            } catch(...) {
            }
        }
        
        return status;
    }
        </xsl:otherwise>
    </xsl:choose>    
    <xsl:for-each select="Outputs/*">
       <xsl:text>std::vector&lt;tw::channel_or::</xsl:text>
       <xsl:value-of select="name()"/>
       <xsl:text>&gt; _o</xsl:text>
       <xsl:value-of select="position()"/>
       <xsl:text>;</xsl:text>
    </xsl:for-each>
};

</xsl:for-each>
</xsl:template>

<xsl:template match="*" mode="paramList">
    <xsl:text>const </xsl:text><xsl:value-of select="name()"/><xsl:text>&amp; </xsl:text>p<xsl:value-of select="position()"/><xsl:if test="position()!=last()">, </xsl:if>
</xsl:template>

<xsl:template match="*" mode="paramFields">
    <xsl:call-template name="paramFields">
        <xsl:with-param name="item" select="name()" />
    </xsl:call-template>
    <xsl:if test="position() != last()">, </xsl:if>
</xsl:template>

<xsl:template name="paramFields">
     <xsl:param name="item"/>
     
     <xsl:for-each select="/Namespace/OrdersDefs/*">        
        <xsl:if test="$item = name()">
            <xsl:if test="@parent!=''">
                <xsl:call-template name="paramFields">
                    <xsl:with-param name="item" select="@parent" />
                </xsl:call-template>
                <xsl:text>, </xsl:text>
            </xsl:if>            
            <xsl:for-each select="*[not(@serializable='false')]">
                <xsl:text>`</xsl:text><xsl:value-of select="name()"/><xsl:text>`</xsl:text><xsl:if test="position() != last()">, </xsl:if>
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
     
     <xsl:for-each select="/Namespace/OrdersDefs/*">        
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
     
     <xsl:for-each select="/Namespace/OrdersDefs/*">        
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

<xsl:template name="tokenize">
    <xsl:param name="inputString"/>
    <xsl:param name="separator"/>
    <xsl:param name="resultToken"/>
    <xsl:param name="nextResultToken"/>
    
    <xsl:variable name="token" select="substring-before($inputString, $separator)" />
    <xsl:if test="$token">
        <xsl:element name="resultToken"><xsl:value-of select="$token"/></xsl:element>
    </xsl:if>
    
    <xsl:variable name="nextToken" select="substring-after($inputString, $separator)" />
    <xsl:if test="nextToken">
        <xsl:element name="nextResultToken"><xsl:value-of select="nextToken"/></xsl:element>
    </xsl:if>
</xsl:template>

</xsl:transform>

