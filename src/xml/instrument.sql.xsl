<?xml version="1.0"?>
<xsl:transform xmlns:xs="http://www.w3.org/2001/XMLSchema"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:fn="http://www.w3.org/2005/02/xpath-functions"
               version="1.0">
					  
<xsl:output method="text" indent="no"/>
<xsl:strip-space elements="*"/>

<xsl:template match="Namespace">
<![CDATA[
--
-- THIS IS generated code - do not modify. To make changes, modify appropriate
-- xml/xsl files!
--

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
]]>

<xsl:apply-templates select="." mode="tables"/>

<![CDATA[
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;
/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
]]>

</xsl:template>


<!-- Database related code generation -->
<!-- -->
<xsl:template match="Namespace/SQLDefs/Tables" mode="tables">
<xsl:for-each select="*">
    <xsl:text>&#10;--</xsl:text>
    <xsl:text>&#10;-- Creating TABLE `</xsl:text><xsl:value-of select="name()"/><xsl:text>`</xsl:text>
    <xsl:text>&#10;--</xsl:text>
    <xsl:text>&#10;DROP TABLE IF EXISTS `</xsl:text><xsl:value-of select="name()"/><xsl:text>`;</xsl:text>
<![CDATA[
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
]]>
    <xsl:text>&#10;CREATE TABLE `</xsl:text><xsl:value-of select="name()"/><xsl:text>` (</xsl:text>
    <xsl:apply-templates select="Fields/*" mode="tableFields">
        <xsl:with-param name="table" select="name()" />
    </xsl:apply-templates>
    <xsl:text>&#10;&#9;`timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP</xsl:text>
    <xsl:apply-templates select="PrimaryKeys/*" mode="tablePrimaryKeys">
        <xsl:with-param name="table" select="name()" />
    </xsl:apply-templates>
    <xsl:apply-templates select="Keys/*" mode="tableKeys">
        <xsl:with-param name="table" select="name()" />
    </xsl:apply-templates>
    <xsl:text>&#10;) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=latin1;</xsl:text>
<![CDATA[
/*!40101 SET character_set_client = @saved_cs_client */;
]]>

</xsl:for-each>
</xsl:template>

<xsl:template match="*" mode="tablePrimaryKeys">
    <xsl:param name="table"/>
    <xsl:text>,&#10;&#9;PRIMARY KEY </xsl:text>
    <xsl:text>(`</xsl:text>
    <xsl:value-of select="name()"/>
    <xsl:text>`)</xsl:text>
</xsl:template>

<xsl:template match="*" mode="tableKeys">
    <xsl:param name="table"/>
    <xsl:text>,</xsl:text>
    <xsl:if test="position()=1">     
        <xsl:text>&#10;&#9;UNIQUE KEY `</xsl:text>
        <xsl:value-of select="$table"/>
        <xsl:text>_unique_key` (</xsl:text>
    </xsl:if>
    <xsl:text>`</xsl:text>
    <xsl:value-of select="name()"/>
    <xsl:text>`(</xsl:text>
    <xsl:value-of select="@length"/>
    <xsl:text>)</xsl:text>
    <xsl:if test="position()=last()">
        <xsl:text>)</xsl:text>
    </xsl:if>
</xsl:template>

<xsl:template match="*" mode="tableFields">
    <xsl:param name="table"/>
    <xsl:call-template name="tableFields">
        <xsl:with-param name="table" select="$table" />
        <xsl:with-param name="tableField" select="name()" />
    </xsl:call-template>
</xsl:template>

<xsl:template name="tableFields">
     <xsl:param name="table"/>
     <xsl:param name="tableField"/>
     <xsl:for-each select="/Namespace/InstrumentsDefs/*">
        <xsl:if test="$tableField = name()">
            <xsl:for-each select="*[not(@serializable='false')]">
                <xsl:text>&#10;&#9;`</xsl:text>
                <xsl:value-of select="name()"/>
                <xsl:text>` </xsl:text>
                <xsl:call-template name="getColumnType">
                    <xsl:with-param name="table" select="$table" />
                    <xsl:with-param name="tableField" select="$tableField" />
                    <xsl:with-param name="item" select="name()" />
                </xsl:call-template>
                <xsl:call-template name="isTableKey">
                    <xsl:with-param name="table" select="$table" />
                    <xsl:with-param name="tableField" select="$tableField" />
                    <xsl:with-param name="item" select="name()" />
                </xsl:call-template>
                <xsl:call-template name="isRequired">
                    <xsl:with-param name="table" select="$table" />
                    <xsl:with-param name="tableField" select="$tableField" />
                    <xsl:with-param name="item" select="name()" />
                </xsl:call-template>
                <xsl:text>,</xsl:text>
            </xsl:for-each>
        </xsl:if>
    </xsl:for-each>        
</xsl:template>

<xsl:template name="getColumnType">
    <xsl:param name="table"/>
    <xsl:param name="tableField"/>
    <xsl:param name="item"/>
    
    <xsl:for-each select="/Namespace/SQLDefs/Tables/*">
        <xsl:if test="$table=name()">
            <xsl:choose>
                <xsl:when test="PrimaryKeys/*[name()=$item]/@auto_increment='true'">
                    <xsl:text>int(11)</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>tinytext</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:if>
    </xsl:for-each>
</xsl:template>

<xsl:template name="isTableKey">
    <xsl:param name="table"/>
    <xsl:param name="tableField"/>
    <xsl:param name="item"/>
    
    <xsl:for-each select="/Namespace/SQLDefs/Tables/*">
        <xsl:if test="$table=name()">
            <xsl:if test="count(Fields/*[name()=$tableField]) != 0">
                <xsl:if test="count(Keys/*[name()=$item]) != 0">
                    <xsl:text> NOT NULL</xsl:text>
                </xsl:if>
                <xsl:if test="count(PrimaryKeys/*[name()=$item]) != 0">
                    <xsl:text> NOT NULL</xsl:text>
                    <xsl:if test="PrimaryKeys/*[name()=$item]/@auto_increment='true'">
                        <xsl:text> AUTO_INCREMENT</xsl:text>
                    </xsl:if>
                </xsl:if>
            </xsl:if>
        </xsl:if>
    </xsl:for-each>
</xsl:template>

<xsl:template name="isRequired">
    <xsl:param name="table"/>
    <xsl:param name="tableField"/>
    <xsl:param name="item"/>
    
    <xsl:for-each select="/Namespace/SQLDefs/Tables/*">
        <xsl:if test="$table=name()">
            <xsl:if test="count(Fields/*[name()=$tableField]) != 0">
                <xsl:if test="count(Required/*[name()=$item]) != 0">
                    <xsl:text> NOT NULL</xsl:text>                    
                </xsl:if>
            </xsl:if>
        </xsl:if>
    </xsl:for-each>
</xsl:template>

</xsl:transform>

