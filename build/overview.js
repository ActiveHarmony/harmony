/*
 * Copyright 2003-2011 Jeffrey K. Hollingsworth
 *
 * This file is part of Active Harmony.
 *
 * Active Harmony is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Active Harmony is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Global Variables */
var xmlhttp;

// Effectively, the main() routine for this program.
$(document).ready(function() {
    if (window.XMLHttpRequest)
        xmlhttp = new XMLHttpRequest(); // IE7+, Firefox, Chrome, Opera, Safari
    else
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP"); // IE6, IE5

    refresh();
});

function date2Str(milliseconds) {
    if (typeof d == "undefined")
        d = new Date();

    d.setTime(milliseconds);
    var retval = d.getFullYear() + "/";
    if (d.getMonth() < 9)
        retval += "0";
    retval += (d.getMonth() + 1) + "/";
    if (d.getDate() < 10)
        retval += "0";
    retval += d.getDate() + " ";

    if (d.getHours() < 10)
        retval += "0";
    retval += d.getHours() + ":";
    if (d.getMinutes() < 10)
        retval += "0";
    retval += d.getMinutes() + ":";
    if (d.getSeconds() < 10)
        retval += "0";
    retval += d.getSeconds();

    return retval;
}

function refresh()
{
    xmlhttp.open("GET", "session-list", false);
    xmlhttp.send();
    var list = xmlhttp.responseText.split("|");
    var sessions = new Array();
    for (var i = 0; i < list.length; ++i) {
        if (list[i].length > 0)
            sessions.push(list[i]);
    }

    var body = document.getElementById("data_table").tBodies[0];
    tableDimension(body, sessions.length, 6);

    for (var i = 0; i < sessions.length; ++i) {
        var field = sessions[i].split(":");
        body.rows[i].cells[0].innerHTML = "<a href='/session-view.cgi?" + field[0] + "'>" + field[0] + "</a>";
        body.rows[i].cells[1].innerHTML = date2Str( field[1] );
        body.rows[i].cells[2].innerHTML = field[2];
        body.rows[i].cells[3].innerHTML = field[3];
        body.rows[i].cells[4].innerHTML = field[4];
        body.rows[i].cells[5].innerHTML = "Pause Restart <a href='#' onclick='sessionKill(\"" + field[0] + "\"); return false;'>Kill</a>";
    }
}

function sessionKill(name)
{
    xmlhttp.open("GET", "kill?"+name, false);
    xmlhttp.send();
    refresh();
}

function tableDimension(body, numRows, numCols)
{
    if (body.rows.length == numRows &&
        body.rows[0].cells.length == numCols) {
        return;
    }

    for (var i = 0; i < numRows; ++i) {
        if (body.rows.length <= i)
            body.appendChild( document.createElement("tr") );
        var row = body.rows[i];
        if (row.cells.length > numCols)
            row.cells.length = numCols;
        while (row.cells.length < numCols)
            row.appendChild( document.createElement("td") );
        while (row.cells.length > numCols)
            row.removeChild( row.lastChild );
    }
    while (body.rows.length > numRows)
        body.removeChild( body.lastChild );

    // Zero is a special case.
    if (numRows == 0) {
        body.appendChild( document.createElement("tr") );
        while (body.rows[0].cells.length < numCols)
            body.rows[0].appendChild( document.createElement("td") );
    }
}
