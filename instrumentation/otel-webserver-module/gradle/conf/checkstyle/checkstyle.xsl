<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:template match="/">
        <html>
            <head>
                <title>AppDynamics Java CheckStyle Report</title>
            </head>
            <body bgcolor="#FFFFFF">
                <p>
                    <h1>AppDynamics Java CheckStyle Report</h1>
                </p>
                <table border="1" cellspacing="0" cellpadding="2">
                    <tr bgcolor="#e24912">
                        <th colspan="2">
                            <b>Summary</b>
                        </th>
                    </tr>
                    <tr bgcolor="#838383">
                        <td>Total files checked</td>
                        <td align="right">
                            <xsl:number level="any" value="count(descendant::file)"/>
                        </td>
                    </tr>
                    <tr bgcolor="#a2ad00">
                        <td>Files with errors or warnings</td>
                        <td align="right">
                            <xsl:number level="any" value="count(descendant::file[error])"/>
                        </td>
                    </tr>
                    <tr bgcolor="#838383">
                        <td>Total errors and warnings</td>
                        <td align="right">
                            <xsl:number level="any" value="count(descendant::error)"/>
                        </td>
                    </tr>
                    <tr bgcolor="#a2ad00">
                        <td>Average errors or warnings per file</td>
                        <td align="right">
                            <xsl:number level="any" value="count(descendant::error) div count(descendant::file)"/>
                        </td>
                    </tr>
                </table>
                <hr align="left" width="95%" size="1"/>
                <p>The following are violations of the
                    <a href="https://singularity.jira.com/wiki/display/CORE/AppDynamics+Java+Coding+Style+Guide">
                        AppDynamics Java Coding Style Guide</a>:
                </p>
                <p/>
                <xsl:apply-templates/>
            </body>
        </html>
    </xsl:template>

    <xsl:template match="file[error]">
        <table bgcolor="#e3e6b2" width="95%" border="1" cellspacing="0" cellpadding="2">
            <tr bgcolor="#a2ad00">
                <th>File</th>
                <td colspan="2">
                    <xsl:value-of select="@name"/>
                </td>
            </tr>
            <tr>
                <th>Line Number</th>
                <th>Severity</th>
                <th>Error Message</th>
            </tr>
            <xsl:apply-templates select="error"/>
        </table>
        <p/>
    </xsl:template>

    <xsl:template match="error">
        <xsl:if test="@severity='warning'">
            <tr bgcolor="#fbfbf3">
                <td>
                    <xsl:value-of select="@line"/>
                </td>
                <td>
                    <xsl:value-of select="@severity"/>
                </td>
                <td>
                    <xsl:value-of select="@message"/>
                </td>
            </tr>
        </xsl:if>
        <xsl:if test="@severity='error'">
            <tr>
                <td>
                    <xsl:value-of select="@line"/>
                </td>
                <td>
                    <xsl:value-of select="@severity"/>
                </td>
                <td>
                    <xsl:value-of select="@message"/>
                </td>
            </tr>
        </xsl:if>
    </xsl:template>
</xsl:stylesheet>