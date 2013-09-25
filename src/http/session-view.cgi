<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<!--
Copyright 2003-2013 Jeffrey K. Hollingsworth

This file is part of Active Harmony.

Active Harmony is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Active Harmony is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Active Harmony.  If not, see <http://www.gnu.org/licenses/>.
-->

<html>
  <head>
    <title>Active Harmony Web Interface</title>
    <script type="text/javascript" src="jquery.min.js"></script>
    <script type="text/javascript" src="jquery.flot.min.js"></script>
    <script type="text/javascript" src="jquery.flot.time.min.js"></script>
    <script type="text/javascript" src="jquery.flot.resize.min.js"></script>
    <script type="text/javascript" src="jquery.flot.selection.min.js"></script>
    <!--[if lte IE 8]>
    <script type="text/javascript" src="excanvas.min.js"></script>
    <![endif]-->
    <script type="text/javascript" src="session-view.js"></script>
    <link rel="stylesheet" type="text/css" href="activeharmony.css" />
  </head>

  <body>
    <div id="server_status" style="clear:both">
      <div style="float:left; width:150px">
        Server Status:
      </div>
      <div id="appName" style="float:left">
      </div>
      <div id="app_div" style="float:right">
        <ul style="list-style-type:none; margin:0; padding:0">
          <li style="display:inline">Refresh Interval:
            <select id="interval">
              <option value="1000">1</option>
              <option value="5000" selected>5</option>
              <option value="10000">10</option>
              <option value="30000">30</option>
              <option value="60000">60</option>
            </select>
          </li>

          <li id="svr_time" style="display:inline">
          </li>
        </ul>
      </div>

    <div style="clear:both">
      <div style="float:left">
        <ul style="list-style-type:none; margin:0; padding:0">
          <li style="display:inline">
            View:
            <select id="view_list" onchange="drawChart()">
              <option>Overview</option>
            </select>
          </li>

          <li style="display:inline">
            Table Length:
            <select id="table_len" onchange="updateDataTable()">
              <option>5</option>
              <option selected>10</option>
              <option>25</option>
              <option>50</option>
            </select>
          </li>
          <li style="display:inline">
            Chart Size:
            <select id="chart_size" onchange="updatePlotSize()">
              <option>640x480</option>
              <option>800x600</option>
              <option>1024x768</option>
              <option>1200x1024</option>
            </select>
          </li>

        </ul>
      </div>
    </div>
    <div style="clear:both">
      <hr />
      <div id="plot_area">
      </div>
      <div id="table_area">
        <table id="data_table">
          <thead>
            <tr>
              <td style="border:none"></td>
            </tr>
          </thead>

          <tbody>
            <tr id="data_table_row_best">
              <td>Best</td>
            </tr>
          </tbody>
        </table>
      </div>

    </div>
  </body>
</html>
