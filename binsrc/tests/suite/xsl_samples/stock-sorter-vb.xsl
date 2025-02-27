<?xml version="1.0"?>
<!--
 -  
 -  $Id: stock-sorter-vb.xsl,v 1.4.10.1 2013/01/02 16:16:13 source Exp $
 -
 -  This file is part of the OpenLink Software Virtuoso Open-Source (VOS)
 -  project.
 -  
 -  Copyright (C) 1998-2022 OpenLink Software
 -  
 -  This project is free software; you can redistribute it and/or modify it
 -  under the terms of the GNU General Public License as published by the
 -  Free Software Foundation; only version 2 of the License, dated June 1991.
 -  
 -  This program is distributed in the hope that it will be useful, but
 -  WITHOUT ANY WARRANTY; without even the implied warranty of
 -  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 -  General Public License for more details.
 -  
 -  You should have received a copy of the GNU General Public License along
 -  with this program; if not, write to the Free Software Foundation, Inc.,
 -  51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 -  
 -  
-->

<!-- Note that this style sheet relies on the XMLDocument and XSLDocument properties and thus
     only works when browsing XML -->
     
<!DOCTYPE xsl:stylesheet [
  <!ENTITY externalScript SYSTEM "externalScript.xml">
]>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl" language="VBScript">
  &externalScript;
  
  <xsl:template match="/">
    <HTML>
      <HEAD>
        <STYLE>
          BODY {margin:0}
          .bg {font:8pt Verdana; background-color:purple; color:white}
          H1 {font:bold 14pt Verdana; width:100%; margin-top:1em}
          .row {font:8pt Verdana; border-bottom:1px solid #CC88CC}
          .header {font:bold 9pt Verdana; cursor:hand; padding:2px; border:2px outset gray}
          .up {background-color:#DDFFDD;}
          .down {background-color:#FFDDDD;}
        </STYLE>
      </HEAD>
      
      <SCRIPT LANGUAGE="VBScript"><xsl:comment><![CDATA[
        Dim stylesheet, source, sortField
        Function sort(field)
          sortField.value = field
          listing.innerHTML = source.documentElement.transformNode(stylesheet)
        End Function
      ]]></xsl:comment></SCRIPT>

      <SCRIPT LANGUAGE="VBScript" for="window" event="onload"><xsl:comment><![CDATA[
        Set stylesheet = document.XSLDocument
        Set source = document.XMLDocument
        Set sortField = document.XSLDocument.selectSingleNode("//@order-by")
      ]]></xsl:comment></SCRIPT>
      
      <BODY>
        <TABLE width="100%" cellspacing="0">
          <TR>
            <TD class="bg"/>
            <TD class="bg">
              <H1><xsl:value-of select="portfolio/description"/>
                for <xsl:apply-templates select="portfolio/date"/></H1>
              <DIV>Average change: <B><xsl:eval>averageChange(me)</xsl:eval></B></DIV>
              <DIV>Total volume: <B><xsl:eval>totalVolume(me)</xsl:eval></B></DIV>
            </TD>
          </TR>
          <TR>
            <TD class="bg" width="120" valign="top">
              <P>Click on the column headers to sort by that field.</P>
              <P>Demonstration of custom formatting of data typed values and
              local reapplication of the style sheet.</P>
              <P>Stocks losing more than 5% indicated in red.  Stocks gaining
              value indicated in green.</P>
            </TD>
            <TD class="bg" valign="top">
              <DIV id="listing"><xsl:apply-templates select="portfolio"/></DIV>
            </TD>
          </TR>
        </TABLE>    
      </BODY>
    </HTML>
  </xsl:template>
  
  <xsl:template match="portfolio">
    <TABLE STYLE="background-color:white">
      <THEAD>
        <TD width="200"><DIV class="header" onClick="sort('name')">Company</DIV></TD>
        <TD width="80"><DIV class="header" onClick="sort('symbol')">Symbol</DIV></TD>
        <TD width="80"><DIV class="header" onClick="sort('price')">Price</DIV></TD>
        <TD width="80"><DIV class="header" onClick="sort('change')">Change</DIV></TD>
        <TD width="80"><DIV class="header" onClick="sort('percent')">%Change</DIV></TD>
        <TD width="80"><DIV class="header" onClick="sort('volume')">Volume</DIV></TD>
      </THEAD>
      <xsl:for-each select="stock"><xsl:sort select="symbol" />
        <TR>
          <xsl:for-each select="change">
            <xsl:if expr="me.nodeTypedValue &gt; 0">
              <xsl:attribute name="class">up</xsl:attribute>
            </xsl:if>
          </xsl:for-each>
          <xsl:for-each select="percent">
            <xsl:if expr="me.nodeTypedValue &lt; -5">
              <xsl:attribute name="class">down</xsl:attribute>
            </xsl:if>
          </xsl:for-each>
          <TD><DIV class="row"><xsl:value-of select="name"/></DIV></TD>
          <TD><DIV class="row"><xsl:value-of select="symbol"/></DIV></TD>
          <TD><DIV class="row" STYLE="text-align:right"><xsl:apply-templates select="price"/></DIV></TD>
          <TD><DIV class="row" STYLE="text-align:right"><xsl:apply-templates select="change"/></DIV></TD>
          <TD><DIV class="row" STYLE="text-align:right"><xsl:apply-templates select="percent"/></DIV></TD>
          <TD><DIV class="row" STYLE="text-align:right"><xsl:apply-templates select="volume"/></DIV></TD>
        </TR>
      </xsl:for-each>
    </TABLE>
  </xsl:template>

  <xsl:template match="date" language="JScript">
    <xsl:eval>formatDate(this.nodeTypedValue, "MMMM dd',' yyyy")</xsl:eval>
              at <xsl:eval>formatTime(this.nodeTypedValue, "hh:mm tt")</xsl:eval>
  </xsl:template>
  
  <xsl:template match="price | change">
    <xsl:eval language="JScript">formatNumber(this.nodeTypedValue, "$0.00")</xsl:eval>
  </xsl:template>
  
  <xsl:template match="percent">
    <xsl:if expr="this.nodeTypedValue &gt; 0" language="JScript">+</xsl:if>
    <xsl:eval language="JScript">formatNumber(this.nodeTypedValue, "0.0")</xsl:eval>%
  </xsl:template>
  
  <xsl:template match="volume">
    <xsl:eval language="JScript">formatNumber(this.nodeTypedValue * 1000000, "#,###,###")</xsl:eval>
  </xsl:template>
  
</xsl:stylesheet>
