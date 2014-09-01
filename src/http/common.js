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
function sendRestart(comm, name, init) {
    var command = "restart?" + name;

    if (init.length > 0)
        command += "&" + init;

    comm.open("GET", encodeURI(command.trim()), false);
    comm.send();

    if (comm.responseText == "FAIL")
        alert("Error restarting session.");
}

/* Pause session */
function sendPause(comm, name) {
    comm.open("GET", "pause?" + name, false);
    comm.send();

    if (comm.responseText == "FAIL")
        alert("Error pausing session.");
}

/* Resume session */
function sendResume(comm, name) {
    comm.open("GET", "resume?" + name, false);
    comm.send();

    if (comm.responseText == "FAIL")
        alert("Error resuming session.");
}

/* Kill session */
function sendKill(comm, name) {
    comm.open("GET", "kill?" + name, false);
    comm.send();

    if (comm.responseText == "FAIL")
        alert("Error killing session.");
}
