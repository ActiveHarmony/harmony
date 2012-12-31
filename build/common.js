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
var viewData = new Array();
var viewChart;
var UTCoffset;
var appName;

// Effectively, the main() routine for this program.
$(document).ready(function(){
    if (window.XMLHttpRequest) {
        // code for IE7+, Firefox, Chrome, Opera, Safari
        xmlhttp = new XMLHttpRequest();
    } else {
        // code for IE6, IE5
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
    }

    viewData.push(new Array());
    var d = new Date();
    UTCoff = -d.getTimezoneOffset()*60*1000;

    appName = alert(document.URL);
    updatePlotSize();
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

var best_coord = new Array();
var coords = new Array();
var chartDataIdx;
var varList;
var tableColMap;

var refreshInterval = 5000;
function refresh() {
    xmlhttp.open("GET", "data-refresh?"+coords.length, false);
    xmlhttp.send();

    var pairs = xmlhttp.responseText.split("|");
    var newCoords = 0;

    for (var i = 0; i < pairs.length; ++i) {
        var sep = pairs[i].search(":");
        if (sep == -1) {
            continue;
        }
        var key = pairs[i].substr(0, sep);
        var val = pairs[i].substr(++sep);

        switch (key) {
        case "time":
            document.getElementById("svr_time").innerHTML = date2Str(val);
            break;
        case "app":
            document.getElementById("appName").innerHTML = val;
            break;
        case "var":
            updateVarList(val);
            break;
        case "coord":
            var c_arr = val.split(",");

            if (c_arr[0] == "best") {
                best_coord = c_arr;
                var row = document.getElementById("data_table_row_best");
                for (var i = 1; i < best_coord.length; ++i) {
                    if (row.cells[i].innerHTML != best_coord[i]) {
                        row.cells[i].style.background = "GreenYellow";
                        row.cells[i].innerHTML = best_coord[i];
                    } else {
                        row.cells[i].style.backgroundColor = "inherit";
                    }
                }

            } else {
                coords.push(c_arr);
                updateChartData(c_arr);
                newCoords = 1;
            }
            break;

        default:
            alert("Unknown key in data pair: [ " + pairs[i] + "]");
        }
    }

    if (newCoords) {
        updateDataTable();
        drawChart();
    }

    var i_select = document.getElementById("interval");
    setTimeout("refresh()", i_select[ i_select.selectedIndex ].value);
}

function updateChartData(arr) {
    var timestamp = parseInt(arr[0]) + UTCoff;
    var perf = parseFloat(arr.slice(-1));
    viewData[0].push([timestamp, perf]);

    for (var i = 2; i < arr.length - 1; ++i) {
        var sep = arr[i].search(":");
        if (sep == -1) {
            continue;
        }
        var key = arr[i].substr(0, sep);
        var val = arr[i].substr(++sep);
        var idx = tableColMap[key];

        viewData[idx].push([parseFloat(val), perf]);
        chartDataIdx[idx][viewData[idx].length - 1]= coords.length - 1;
    }
}

function updateDataTableDimensions() {
    var l_select = document.getElementById("table_len");
    var numRows = parseInt(l_select[ l_select.selectedIndex ].text) + 1;
    var numCols = currVars.split(",").length + 2;
    var tableBody = document.getElementById("data_table").tBodies[0];

    if (tableBody.rows.length == numRows &&
        tableBody.rows[0].cells.length == numCols) {
        return;
    }

    for (var i = 0; i < numRows; ++i) {
        if (tableBody.rows.length <= i) {
            tableBody.appendChild( document.createElement("tr") );
        }

        var row = tableBody.rows[i];
        if (row.cells.length > numCols)
            row.cells.length = numCols;

        while (row.cells.length < numCols) {
            row.appendChild( document.createElement("td") );
        }
        while (row.cells.length > numCols) {
            row.removeChild( row.lastChild );
        }
    }
    while (tableBody.rows.length > numRows) {
        tableBody.removeChild( tableBody.lastChild );
    }
}

function updateDataTable() {
    var tableBody = document.getElementById("data_table").tBodies[0];
    var cols = currVars.split(",").length + 2;
    updateDataTableDimensions();

    var idx = coords.length - 1;
    for (var i = 1; i < tableBody.rows.length; ++i) {
        if (idx >= 0) {
            var coord = coords[idx];

            tableBody.rows[i].cells[0].innerHTML = coord[1];
            for (var j = 1; j < cols-1; ++j) {
                tableBody.rows[i].cells[j].innerHTML = "";
            }
            tableBody.rows[i].cells[cols-1].innerHTML = coord.slice(-1);

            for (var j = 2; j < coord.length; ++j) {
                var sep = coord[j].search(":");
                if (sep == -1) {
                    continue;
                }
                var key = coord[j].substr(0, sep);
                var val = coord[j].substr(++sep);
                var col = tableColMap[key];

                tableBody.rows[i].cells[col].innerHTML = val;
            }
            --idx;

        } else {
            tableBody.rows[i].cells[0].innerHTML = "&lt;N/A&gt;";
            for (var j = 1; j < cols; ++j) {
                tableBody.rows[i].cells[j].innerHTML = "";
            }
        }
    }
}

var currVars = "";
function updateVarList(vars) {
    if (vars == currVars)
        return;

    var v_select = document.getElementById("view_list");
    var tableHead = document.getElementById("data_table").tHead;
    v_select.length = 1;

    var v_arr = vars.split(",").sort();
    tableColMap = new Object();
    chartDataIdx = new Array(v_arr.length + 1);

    for (var i = 0; i < v_arr.length; ++i) {
        var col = document.createElement("td");
        col.innerHTML = v_arr[i].substr(2);
        tableHead.rows[0].appendChild(col);

        var option = document.createElement("option");
        option.text = v_arr[i].substr(2);
        v_select.add(option, null);

        viewData.push(new Array());
        chartDataIdx[i] = new Array();
        tableColMap[v_arr[i].substr(2)] = i + 1;
    }

    var col = document.createElement("td");
    col.innerHTML = "Performance";
    tableHead.rows[0].appendChild(col);
    chartDataIdx[v_arr.length] = new Array();

    currVars = vars;
    updateDataTableDimensions();
}

function updatePlotSize() {
    var c_select = document.getElementById("chart_size");
    var dim = c_select[ c_select.selectedIndex ].value.split("x");
    var p_div = document.getElementById("plot_area");

    p_div.style.width = dim[0] + "px";
    p_div.style.height = dim[1] + "px";
}

function reloadServer() {
    document.open("text/html", "replace");
    document.write("<html><body><h1>Reloading server</h1></body></html>");
    document.close();

    xmlhttp.open("GET", "hup", false);
    xmlhttp.send();
    if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
        document.open("text/html", "replace");
        document.write("<html><body><h1>Success!</h1></body></html>");
        document.close();

        window.location.reload(true);
    }
}

var prevPoint = null;
function drawChart() {
    var i = document.getElementById("view_list").selectedIndex;
    var x_mode = null;
    if (i == 0) {
        x_mode = "time";
    }

    $.plot($("#plot_area"), [viewData[i]], { xaxis:  {mode:x_mode},
                                             grid:   {hoverable:true},
                                             points: {show:true}});
    $("#plot_area").bind("plothover", function (event, pos, item) {
        if (item) {
            if (prevPoint != item.dataIndex) {
                if (prevPoint != null) {
                    $("#tooltip").remove();
                }
                prevPoint = item.dataIndex;

                $('<div id="tooltip">' + labelString(item.dataIndex) + '</div>').css( {
                    position: 'absolute',
                    display: 'none',
                    top: item.pageY + 5,
                    left: item.pageX - 150,
                    border: '1px solid #fdd',
                    padding: '2px',
                    'background-color': '#fee',
                    opacity: 0.80
                }).appendTo("body").fadeIn(100);
            }
        } else {
            $("#tooltip").remove();
            prevPoint = null;
        }
    });
}

function labelString(idx) {
    var chartNum = document.getElementById("view_list").selectedIndex;
    var coord;
    if (chartNum == 0) {
        coord = coords[idx];
    } else {
        coord = coords[ chartDataIdx[chartNum][idx] ];
    }

    var retval = "Node:";
    for (var i = 1; i < coord.length - 1; ++i) {
        retval += coord[i] + "<br>";
    }
    retval += "Performance:" + coord.slice(-1);

    return retval;
}
