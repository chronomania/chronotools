<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
 <xsl:template match="/location_event_commands">
 <!--
   Params:
   -common-
       size
       [param]
       [comment]
   -specific-
     Immed
       size
       [min, max]
     ObjectNumber
     NibbleHi
     NibbleLo
     OrBitNumber
     AndBitNumber
     TableReference
       address
     Table2Reference
     Prop
     Const
     GotoForward
     GotoBackward
     ConditionalGoto
     Operator
     Blob
     DialogBegin
     DialogIndex
 -->

  <html>
   <head>
    <title><xsl:value-of select="title" /></title>
    <link rel="stylesheet" type="text/css" href="eventdata.css" />
   </head>

   <body>
   
    <h1><xsl:value-of select="title" /></h1>
    
    <xsl:variable name="url" select="compiler/url" />
    Compiled by <a href="{$url}"><xsl:value-of select="compiler/name" /></a><br/>
    With thanks to <a href="http://geigercount.net/">Michael Springer</a>
    and his Offsets Guide where most of this data originally is from.
    <p/>
    
    <p>
    Each location is prefixed by the number of objects in the scene,
    and then 16 pointers to "functions" for each object.<br/>
    When the room is loaded, each object will execute the first
    function until the first <code>[Return]</code> is found.
    After that, each object will continue from that point, each
    running a few cycles at time, virtually multitasking.
     <p/>
    In this document, all the statements given are ran by their owners.
    By default, all statements affect the state of their owners, but
    statements which have <code>[for:%0]</code> in them, will alter
    the state for another object (denoted by the number).
    </p>

    <table cellspacing="1">
     <tr>
      <th class="top">Chronotools syntax</th>
      <th class="top">Size</th>
      <th class="top" colspan="18">Encoding (memory representation)</th>
     </tr>
     <xsl:apply-templates select="sequence" />
    </table>
    
    <h3>Memory address list</h3>
    (Only memory addresses relevant for the location script are listed.)

    <table cellspacing="1">
     <tr>
      <th class="top">Memory address</th>
      <th class="top">Size</th>
      <th class="top">Indexed by</th>
      <th class="top">Name in Chronotools</th>
      <th class="top">Comments</th>
     </tr>
     <xsl:apply-templates select="memory_addresses/address" />
    </table>
    
    <hr/>
    <a href="eventdata.xml">Download as xml file (with assembly comments)</a>

   </body>
  </html>
 </xsl:template>

 <xsl:template match="sequence">
      <tr>
       <th class="syntax">
         
         <xsl:if test="param[@type='ConditionalGoto']/@size > 0">
          <b>If(</b>
         </xsl:if>         
       
         <xsl:value-of select="@cmd" />
         
         <xsl:if test="param[@type='ConditionalGoto']/@size > 0">
          <b>)</b>
         </xsl:if>         
       
         <xsl:for-each select="param[@type='Const']">
          <br/> %<xsl:value-of select="@param" /> = <b><xsl:value-of select="@value" /></b>
           <xsl:if test="@comment!=''">
            - <em><xsl:value-of select="@comment" /></em>
           </xsl:if>
         </xsl:for-each>
         
         <xsl:for-each select="param[@type='Prop']">
          <br/> %<xsl:value-of select="@param" /> = <b><xsl:value-of select="@value" /></b>[ObjNumber]
           <xsl:if test="@comment!=''">
            - <em><xsl:value-of select="@comment" /></em>
           </xsl:if>
         </xsl:for-each>
       </th>
       <td>
        <xsl:value-of select="@length" />
        <xsl:if test="param[@type='Blob']/@size > 0">+ BlobLength</xsl:if>
       </td>
       
       <xsl:variable name="seq" select="." />
       
       <xsl:for-each select="/location_event_commands/positions/value">
         <xsl:call-template name="handle_byte_position">
           <xsl:with-param name="seq" select="$seq" />
           <xsl:with-param name="value" select="." />
         </xsl:call-template>
       </xsl:for-each>

       <xsl:for-each select="/location_event_commands/positions/value">
        <xsl:if test="$seq/@length &lt; ."><td class="emptydata"></td></xsl:if>
       </xsl:for-each>

      </tr>

      <tr>
       <td colspan="20" class="comments">
        <pre><xsl:value-of select="comment[@type='analysis']" /></pre>
       </td>
      </tr>
 </xsl:template>
 
 <xsl:template name="handle_byte_position">
  <xsl:param name="seq" select="param" />
  <xsl:param name="value" />
  
  <xsl:variable name="size" select="$seq/param[@pos=$value]/@size" />

  <xsl:if test="$size > 0">
    <td colspan="{$size}" rowspan="1" class="data">
     <xsl:for-each select="$seq/param[@pos=$value]">
      <xsl:call-template name="handle_param" />
     </xsl:for-each>
    </td>
  </xsl:if>
 </xsl:template>
 
 <xsl:template name="handle_param">
   <xsl:if test="@param!=''">%<xsl:value-of select="@param" /> = </xsl:if>
   
   <xsl:choose>
     <xsl:when test="@type='Immed'">
       <xsl:choose>
        <xsl:when test="@min!=''">
          <span class="value">
           <xsl:value-of select="@min"/>
           <xsl:if test="@max!=@min">-<xsl:value-of select="@max"/></xsl:if>
          </span>
        </xsl:when>
        <xsl:otherwise>val</xsl:otherwise>
       </xsl:choose>
     </xsl:when>
     <xsl:when test="@type='TableReference'">
       Index&#160;to&#160;$<xsl:value-of select="@address"/>
     </xsl:when>
     <xsl:when test="@type='Table2Reference'">
       Index&#160;to&#160;$<xsl:value-of select="@address"/>&#160;/&#160;2
     </xsl:when>
     <xsl:when test="@type='ConditionalGoto'">
       ForwardGotoAmount
     </xsl:when>
     <xsl:when test="@type='GotoForward'">
       ForwardGotoAmount
     </xsl:when>
     <xsl:when test="@type='GotoBackward'">
       BackwardGotoAmount
     </xsl:when>
     <xsl:when test="@type='Blob'">
       BlobLength+2
     </xsl:when>
     <xsl:when test="@type='ObjectNumber'">
       Object&#160;number&#160;*&#160;2
     </xsl:when>
     <xsl:when test="@type='DialogBegin'">
       DialogBeginAddress
     </xsl:when>
     <xsl:when test="@type='DialogIndex'">
       DialogIndex
     </xsl:when>
     <xsl:when test="@type='NibbleHi'">
       high 4 bits
     </xsl:when>
     <xsl:when test="@type='NibbleLo'">
       low 4 bits
     </xsl:when>
     <xsl:when test="@type='OrBitNumber'">
       Bit number to set
     </xsl:when>
     <xsl:when test="@type='AndBitNumber'">
       Bit number to clear
     </xsl:when>
     <xsl:otherwise>
      <xsl:value-of select="@type" />
     </xsl:otherwise>
   </xsl:choose>
   <xsl:if test="@highbit=1"><br/><em>High bit of this byte must be set</em></xsl:if>

   <br/>
   <xsl:if test="@comment!=''">
    <em><xsl:value-of select="@comment" /></em>
    <br/>
   </xsl:if>
   
 </xsl:template>


 <xsl:template match="address">
  <tr>
   <th><xsl:value-of select="@addr" /></th>
   <td><xsl:value-of select="@size" /></td>
   <td>
    <xsl:choose>
     <xsl:when test="@index='sprite'">sprite*12</xsl:when>
     <xsl:when test="@index='object'">object*128</xsl:when>
     <xsl:otherwise>-</xsl:otherwise>
    </xsl:choose>
   </td>
   <td><xsl:value-of select="@name" /></td>
   <td><pre><xsl:value-of select="comment" /></pre></td>
  </tr>
 </xsl:template>
 
</xsl:stylesheet>
