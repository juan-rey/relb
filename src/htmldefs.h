/* 
   htmldefs.h: html constanst header file.

   Copyright 2006, 2007, 2008, 2009 Juan Rey Saura

This file is part of Resolutive Easy Load Balancer.

This software is a copyrighted work licensed under the Open Software License version 3.0
Please consult the file "LICENSE.txt" for details.
*/

#ifndef htmldefs_h
#define htmldefs_h

#define HTML_STYLE_SECTION "\
<style type=\"text/css\">\n\
<!--                 \n\
.rwaBody{\n\
    font-family: Arial, Helvetica, sans-serif;\n\
    font-size: 12px;\n\
    color: #666666;\n\
}\n\
.rwaTitle {\n\
font-family: Arial, Helvetica, sans-serif;\n\
    color: #2B75B1;\n\
    font-weight: bold;\n\
}\n\
.rwaTableHeader{\n\
    font-family: Arial, Helvetica, sans-serif;\n\
    font-size: 12px;\n\
    color: #FFFFFF;\n\
 background-color: #2B75B1;\n\
}\n\
table.sample {\n\
    border-width: 1px 1px 1px 1px;\n\
    border-spacing: 2px;\n\
    border-style: outset outset outset outset;\n\
    border-color: #AAAAAA;\n\
    border-collapse: collapse;\n\
    background-color: white;\n\
}\n\
table.sample th {\n\
    border-width: 1px 1px 1px 1px;\n\
    padding: 1px 1px 1px 1px;\n\
    border-style: inset inset inset inset;\n\
    border-color: #AAAAAA;\n\
    background-color: #2B75B1;\n\
    -moz-border-radius: 0px 0px 0px 0px;\n\
}\n\
table.sample td {\n\
    border-width: 1px 1px 1px 1px;\n\
    padding: 1px 1px 1px 1px;\n\
    border-style: inset inset inset inset;\n\
    border-color: #AAAAAA;\n\
    background-color: white;\n\
    -moz-border-radius: 0px 0px 0px 0px;\n\
}\n\
</style>\n\
"

#define HTML_TABLE_START "\
<table width=\"100%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n\
<tr>\n\
<td width=\"100px\"> </td>\n\
<td>\n\
<table width=\"100%\" class=\"sample\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n\
"

#define HTML_TABLE_END "\
</table>\n\
<td width=\"100px\"> </td>\n\
</td>\n\
</tr>\n\
</table>\n\
"

#endif
