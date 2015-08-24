<?xml version="1.0"?>
<xsl:transform xmlns:xs="http://www.w3.org/2001/XMLSchema"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:fn="http://www.w3.org/2005/02/xpath-functions"
               version="1.0">
					  
<xsl:output method="text" indent="no"/>
<xsl:strip-space elements="*"/>

<xsl:template name="OneField">
	<xsl:param name="table_name"/>
	<xsl:param name="field_name"/>
	<xsl:variable name="field_parent" select="/Namespace/*/*[name()=$field_name]/@parent"/>
	<xsl:if test="$field_parent">
		<xsl:call-template name="OneField">
			<xsl:with-param name="table_name" select="$table_name"/>
			<xsl:with-param name="field_name" select="$field_parent"/>
		</xsl:call-template>
		<xsl:text>,&#10;</xsl:text>
	</xsl:if>
 <!-- lookup to struct information for the namespace -->
 <!-- the '*' in the XPATH will break if you have more than one Namespace in the XML file -->
 <!-- for each column -->
	<xsl:for-each select="/Namespace/*/*[name()=$field_name]/*">
		<xsl:if test="not(@serializable='false')">
			<xsl:variable name="item" select="name()"/>
			<xsl:variable name="item_type" select="@type"/>
			<xsl:variable name="db_type" select="@db_type"/>
			<xsl:variable name="item_maxlen" select="@maxlen"/>
			<xsl:if test="position()>1">
				<xsl:text>,&#10;</xsl:text>
			</xsl:if>
			<xsl:text>&#9;</xsl:text>
			<xsl:text>`</xsl:text>
			<xsl:value-of select="$item"/>
			<xsl:text>`</xsl:text>
			<xsl:text> </xsl:text>
			<xsl:choose>
				<!-- a primary key must either be an integral type or a typedef to an intregal type -->
				<xsl:when test="/Namespace/SQLDefs/Tables/*[name()=$table_name]/PrimaryKeys/*[name()=$item]">
						<xsl:text>bigint</xsl:text>
				</xsl:when>
				<xsl:when test="$db_type">
						<xsl:value-of select="$db_type"/>
				</xsl:when>
				<xsl:when test="$item_type='bool'">
						<xsl:text>bool</xsl:text>
				</xsl:when>
				<xsl:when test="$item_type='int8_t'">
						<xsl:text>tinyint</xsl:text>
				</xsl:when>
				<xsl:when test="$item_type='int32_t'">
						<xsl:text>int</xsl:text>
				</xsl:when>
				<xsl:when test="$item_type='uint32_t'">
						<xsl:text>int unsigned</xsl:text>
				</xsl:when>
				<xsl:when test="$item_type='double'">
						<xsl:text>double</xsl:text>
				</xsl:when>
				<xsl:when test="$item_type='float'">
						<xsl:text>float</xsl:text>
				</xsl:when>
				<!-- THighResTime comes from Onix and is a fixed-length string -->
				<xsl:when test="$item_type='tw::common::THighResTime'">
						<xsl:text>char(24)</xsl:text>
				</xsl:when>
				<xsl:when test="$item_type='date'">
						<xsl:text>date</xsl:text>
				</xsl:when>
				<xsl:when test="$item_type='datetime'">
						<xsl:text>datetime</xsl:text>
				</xsl:when>
				<xsl:otherwise>
						<xsl:text>varchar(</xsl:text>
						<xsl:choose>
							<xsl:when test="$item_maxlen">
								<xsl:value-of select="$item_maxlen"/>
							</xsl:when>
							<xsl:otherwise>
								<!-- 127 is probably too big but at least it's better than tinytext -->
								<xsl:text>127</xsl:text>
							</xsl:otherwise>
						</xsl:choose>
						<xsl:text>)</xsl:text>
				</xsl:otherwise>
			</xsl:choose>
			<xsl:choose>
				<xsl:when test="/Namespace/SQLDefs/Tables/*[name()=$table_name]/Required/*[name()=$item]">
					<xsl:text> NOT NULL</xsl:text>
				</xsl:when>
				<xsl:when test="/Namespace/SQLDefs/Tables/*[name()=$table_name]/PrimaryKeys/*[name()=$item]">
					<xsl:text> NOT NULL</xsl:text>
				</xsl:when>
			</xsl:choose>
			<xsl:if test="/Namespace/SQLDefs/Tables/*[name()=$table_name]/PrimaryKeys/*[name()=$item]/@auto_increment='true'">
				<xsl:text> AUTO_INCREMENT</xsl:text>
			</xsl:if>
		</xsl:if>
	</xsl:for-each>
</xsl:template>

<xsl:template match="Namespace/SQLDefs/Tables">
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
<!-- for each table -->
<xsl:for-each select="/Namespace/SQLDefs/Tables/*">
	<xsl:variable name="table_name" select="name()"/>
	<xsl:text>&#10;--</xsl:text>
	<xsl:text>&#10;-- Creating TABLE `</xsl:text><xsl:value-of select="$table_name"/><xsl:text>`</xsl:text>
	<xsl:text>&#10;--</xsl:text>
	<xsl:text>&#10;DROP TABLE IF EXISTS `</xsl:text><xsl:value-of select="$table_name"/><xsl:text>`;</xsl:text>
	<xsl:text>&#10;</xsl:text>
<![CDATA[
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
]]>
    <xsl:text>&#10;CREATE TABLE `</xsl:text><xsl:value-of select="$table_name"/><xsl:text>` (&#10;</xsl:text>
		<!-- a "field" is a reference to a group of "items".  each item is a struct member/column -->
		<!-- for each field -->
		<xsl:variable name="indexed" select="./Fields/@indexed"/>
		<xsl:if test="$indexed='true'">
			<xsl:text>&#9;`index` bigint AUTO_INCREMENT PRIMARY KEY,&#10;</xsl:text>
		</xsl:if>
		<xsl:for-each select="./Fields/*">
			<xsl:if test="position()>1">
				<xsl:text>,&#10;</xsl:text>
			</xsl:if>
			<xsl:variable name="field_name" select="name()"/>
			<xsl:call-template name="OneField">
				<xsl:with-param name="table_name" select="$table_name"/>
				<xsl:with-param name="field_name" select="$field_name"/>
			</xsl:call-template>
		</xsl:for-each>
	<xsl:variable name="timestamp" select="./Fields/@timestamp"/>
	<xsl:if test="$timestamp='true'">
		<xsl:text>,&#10;&#9;`timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP</xsl:text>
	</xsl:if>
		<!-- check for Primary Key -->
		<!-- it's coded for compound keys but that will break if @autoincrement is used -->
		<xsl:if test="./PrimaryKeys">
			<xsl:text>,&#10;&#9;CONSTRAINT PRIMARY KEY (</xsl:text>
			<xsl:for-each select="./PrimaryKeys/*">
				<xsl:if test="position()>1">
					<xsl:text>, </xsl:text>
				</xsl:if>
			  <xsl:text>`</xsl:text>
				<xsl:value-of select="name()"/>
			  <xsl:text>`</xsl:text>
			</xsl:for-each>
			<xsl:text>)</xsl:text>
		</xsl:if>
		<!-- check for Unique Keys  -->
		<!-- compounds are allowed but not multiple UNIQUE keys -->
		<xsl:if test="./Keys">
			<xsl:text>,&#10;&#9;CONSTRAINT UNIQUE KEY (</xsl:text>
			<xsl:for-each select="./Keys/*">
				<xsl:if test="position()>1">
					<xsl:text>, </xsl:text>
				</xsl:if>
			  <xsl:text>`</xsl:text>
				<xsl:value-of select="name()"/>
			  <xsl:text>`</xsl:text>
			</xsl:for-each>
			<xsl:text>)</xsl:text>
		</xsl:if>
		<!-- check for Unique Keys  -->
		<!-- Indexes is kind of a hack; we should make the definition structured like Keys are -->
		<xsl:if test="./Indexes">
			<xsl:for-each select="./Indexes/*">
				<xsl:if test="position()>1">
					<xsl:text>,&#10;</xsl:text>
				</xsl:if>
				<xsl:text>,&#10;&#9;INDEX </xsl:text>
				<xsl:value-of select="@value"/>
			</xsl:for-each>
		</xsl:if>
  <xsl:text>&#10;) ENGINE=InnoDB AUTO_INCREMENT=10 DEFAULT CHARSET=latin1;</xsl:text>
<![CDATA[
/*!40101 SET character_set_client = @saved_cs_client */;
]]>
</xsl:for-each>
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
</xsl:transform>
