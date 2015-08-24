<?xml version="1.0"?>
<xsl:transform xmlns:xs="http://www.w3.org/2001/XMLSchema"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:fn="http://www.w3.org/2005/02/xpath-functions"
               version="1.0">
					  
<xsl:output method="text" indent="no"/>
<xsl:strip-space elements="*"/>

<!-- Database related code generation -->
<!-- -->
<xsl:template match="FIXML/Batch">
<xsl:for-each select="*">
    <xsl:if test="(Instrmt[@SecTyp='FUT'])">
        <xsl:text>sym=</xsl:text><xsl:value-of select="Instrmt/@Sym"/>;exp=<xsl:value-of select="Instrmt/@MMY"/>
        <xsl:for-each select="Full">
            <xsl:choose>
                <xsl:when test="@Typ='6'">
                    <xsl:text>;settlPrice=</xsl:text><xsl:value-of select="@Px"/>
                </xsl:when>
                <xsl:when test="@Typ='7'">
                    <xsl:text>;high=</xsl:text><xsl:value-of select="@Px"/>
                </xsl:when>
                <xsl:when test="@Typ='8'">
                    <xsl:text>;low=</xsl:text><xsl:value-of select="@Px"/>
                </xsl:when>
            </xsl:choose>            
        </xsl:for-each>
        <xsl:text>;settlDate=</xsl:text><xsl:value-of select="@BizDt"/>
        <xsl:text>&#10;</xsl:text>
    </xsl:if>
</xsl:for-each>
</xsl:template>

</xsl:transform>

