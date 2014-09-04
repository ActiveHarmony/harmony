/*
 * Copyright 2003-2013 Jeffrey K. Hollingsworth
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

function abort(message) {
    alert(message);
    throw new Error("Exiting on error.");
}

var ajax;
var ajax_error_function = abort;

function AJAXinit(func) {
    if (func)
        ajax_error_function = func;

    try {
        if (window.XMLHttpRequest)
            xmlhttp = new XMLHttpRequest(); // IE7+ and modern browsers
        else // IE6, IE5
            xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
    }
    catch (exception) {
        ajax_error_function(exception.name);
    }
}

function AJAXsend(URL) {
    try {
        xmlhttp.open("GET", URL, false);
        xmlhttp.send();
    }
    catch (exception) {
        ajax_error_function(exception.name);
    }

    if (xmlhttp.responseText == "FAIL")
        ajax_error_function("Dead");

    return xmlhttp.responseText;
}

function dateString(milliseconds) {
    var d = new Date();
    d.setTime(milliseconds);
    return d.toLocaleDateString();
}

function timeString(milliseconds) {
    var d = new Date();
    d.setTime(milliseconds);
    return d.toLocaleTimeString();
}

/* Restart session */
function sendRestart(name, init) {
    var command = "restart?" + name;

    if (init && init.length > 0)
        command += "&" + init;

    AJAXsend(encodeURI(command.trim()));
}

/* Pause session */
function sendPause(name) {
    AJAXsend("pause?" + name);
}

/* Resume session */
function sendResume(name) {
    AJAXsend("resume?" + name);
}

/* Kill session */
function sendKill(name) {
    AJAXsend("kill?" + name);
}
