/*
   Licensed to the Apache Software Foundation (ASF) under one or more
   contributor license agreements.  See the NOTICE file distributed with
   this work for additional information regarding copyright ownership.
   The ASF licenses this file to You under the Apache License, Version 2.0
   (the "License"); you may not use this file except in compliance with
   the License.  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
var showControllersOnly = false;
var seriesFilter = "";
var filtersOnlySampleSeries = true;

/*
 * Add header in statistics table to group metrics by category
 * format
 *
 */
function summaryTableHeader(header) {
    var newRow = header.insertRow(-1);
    newRow.className = "tablesorter-no-sort";
    var cell = document.createElement('th');
    cell.setAttribute("data-sorter", false);
    cell.colSpan = 1;
    cell.innerHTML = "Requests";
    newRow.appendChild(cell);

    cell = document.createElement('th');
    cell.setAttribute("data-sorter", false);
    cell.colSpan = 3;
    cell.innerHTML = "Executions";
    newRow.appendChild(cell);

    cell = document.createElement('th');
    cell.setAttribute("data-sorter", false);
    cell.colSpan = 7;
    cell.innerHTML = "Response Times (ms)";
    newRow.appendChild(cell);

    cell = document.createElement('th');
    cell.setAttribute("data-sorter", false);
    cell.colSpan = 1;
    cell.innerHTML = "Throughput";
    newRow.appendChild(cell);

    cell = document.createElement('th');
    cell.setAttribute("data-sorter", false);
    cell.colSpan = 2;
    cell.innerHTML = "Network (KB/sec)";
    newRow.appendChild(cell);
}

/*
 * Populates the table identified by id parameter with the specified data and
 * format
 *
 */
function createTable(table, info, formatter, defaultSorts, seriesIndex, headerCreator) {
    var tableRef = table[0];

    // Create header and populate it with data.titles array
    var header = tableRef.createTHead();

    // Call callback is available
    if(headerCreator) {
        headerCreator(header);
    }

    var newRow = header.insertRow(-1);
    for (var index = 0; index < info.titles.length; index++) {
        var cell = document.createElement('th');
        cell.innerHTML = info.titles[index];
        newRow.appendChild(cell);
    }

    var tBody;

    // Create overall body if defined
    if(info.overall){
        tBody = document.createElement('tbody');
        tBody.className = "tablesorter-no-sort";
        tableRef.appendChild(tBody);
        var newRow = tBody.insertRow(-1);
        var data = info.overall.data;
        for(var index=0;index < data.length; index++){
            var cell = newRow.insertCell(-1);
            cell.innerHTML = formatter ? formatter(index, data[index]): data[index];
        }
    }

    // Create regular body
    tBody = document.createElement('tbody');
    tableRef.appendChild(tBody);

    var regexp;
    if(seriesFilter) {
        regexp = new RegExp(seriesFilter, 'i');
    }
    // Populate body with data.items array
    for(var index=0; index < info.items.length; index++){
        var item = info.items[index];
        if((!regexp || filtersOnlySampleSeries && !info.supportsControllersDiscrimination || regexp.test(item.data[seriesIndex]))
                &&
                (!showControllersOnly || !info.supportsControllersDiscrimination || item.isController)){
            if(item.data.length > 0) {
                var newRow = tBody.insertRow(-1);
                for(var col=0; col < item.data.length; col++){
                    var cell = newRow.insertCell(-1);
                    cell.innerHTML = formatter ? formatter(col, item.data[col]) : item.data[col];
                }
            }
        }
    }

    // Add support of columns sort
    table.tablesorter({sortList : defaultSorts});
}

$(document).ready(function() {

    // Customize table sorter default options
    $.extend( $.tablesorter.defaults, {
        theme: 'blue',
        cssInfoBlock: "tablesorter-no-sort",
        widthFixed: true,
        widgets: ['zebra']
    });

    var data = {"OkPercent": 100.0, "KoPercent": 0.0};
    var dataset = [
        {
            "label" : "FAIL",
            "data" : data.KoPercent,
            "color" : "#FF6347"
        },
        {
            "label" : "PASS",
            "data" : data.OkPercent,
            "color" : "#9ACD32"
        }];
    $.plot($("#flot-requests-summary"), dataset, {
        series : {
            pie : {
                show : true,
                radius : 1,
                label : {
                    show : true,
                    radius : 3 / 4,
                    formatter : function(label, series) {
                        return '<div style="font-size:8pt;text-align:center;padding:2px;color:white;">'
                            + label
                            + '<br/>'
                            + Math.round10(series.percent, -2)
                            + '%</div>';
                    },
                    background : {
                        opacity : 0.5,
                        color : '#000'
                    }
                }
            }
        },
        legend : {
            show : true
        }
    });

    // Creates APDEX table
    createTable($("#apdexTable"), {"supportsControllersDiscrimination": true, "overall": {"data": [1.0, 500, 1500, "Total"], "isController": false}, "titles": ["Apdex", "T (Toleration threshold)", "F (Frustration threshold)", "Label"], "items": [{"data": [1.0, 500, 1500, "LT-S02_bid_history"], "isController": false}, {"data": [1.0, 500, 1500, "L1_auction_list_20u"], "isController": false}, {"data": [1.0, 500, 1500, "L3_auction_list_100u"], "isController": false}, {"data": [1.0, 500, 1500, "LT-S03_notifications"], "isController": false}, {"data": [1.0, 500, 1500, "LT-S04_submit_bid"], "isController": false}, {"data": [1.0, 500, 1500, "LT-S05_bid_history"], "isController": false}, {"data": [1.0, 500, 1500, "LT-S05_notifications"], "isController": false}, {"data": [1.0, 500, 1500, "L2_auction_list_50u"], "isController": false}, {"data": [1.0, 500, 1500, "LT-S02_auction_detail"], "isController": false}, {"data": [1.0, 500, 1500, "L4_auction_list_200u"], "isController": false}, {"data": [1.0, 500, 1500, "LT-S05_submit_bid"], "isController": false}, {"data": [1.0, 500, 1500, "LT-S05_list"], "isController": false}, {"data": [1.0, 500, 1500, "LT-S05_detail"], "isController": false}, {"data": [1.0, 500, 1500, "S00_healthz_baseline"], "isController": false}]}, function(index, item){
        switch(index){
            case 0:
                item = item.toFixed(3);
                break;
            case 1:
            case 2:
                item = formatDuration(item);
                break;
        }
        return item;
    }, [[0, 0]], 3);

    // Create statistics table
    createTable($("#statisticsTable"), {"supportsControllersDiscrimination": true, "overall": {"data": ["Total", 3840, 0, 0.0, 2.0138020833333266, 0, 77, 2.0, 3.0, 3.0, 8.0, 35.79251526308431, 29.586425758493732, 8.23749606771683], "isController": false}, "titles": ["Label", "#Samples", "FAIL", "Error %", "Average", "Min", "Max", "Median", "90th pct", "95th pct", "99th pct", "Transactions/s", "Received", "Sent"], "items": [{"data": ["LT-S02_bid_history", 300, 0, 0.0, 1.3066666666666669, 1, 3, 1.0, 2.0, 2.0, 2.0, 30.28161905723226, 15.14080952861613, 6.38752901988493], "isController": false}, {"data": ["L1_auction_list_20u", 100, 0, 0.0, 2.0700000000000003, 1, 4, 2.0, 3.0, 3.0, 4.0, 13.121637580370031, 12.468118521191444, 2.40905064952106], "isController": false}, {"data": ["L3_auction_list_100u", 500, 0, 0.0, 1.604, 1, 5, 1.0, 3.0, 3.0, 3.0, 33.654169751632224, 31.978034344080232, 6.1786952278387295], "isController": false}, {"data": ["LT-S03_notifications", 200, 0, 0.0, 3.2700000000000022, 1, 38, 2.0, 5.0, 8.949999999999989, 30.940000000000055, 24.816974810770567, 10.857426479712124, 9.936484055093684], "isController": false}, {"data": ["LT-S04_submit_bid", 90, 0, 0.0, 3.877777777777779, 2, 52, 3.0, 4.0, 4.0, 52.0, 6.993006993006993, 3.885762674825175, 3.1618771853146854], "isController": false}, {"data": ["LT-S05_bid_history", 200, 0, 0.0, 1.3249999999999993, 0, 2, 1.0, 2.0, 2.0, 2.0, 10.096420818819729, 6.182085794335908, 2.1297137664697865], "isController": false}, {"data": ["LT-S05_notifications", 200, 0, 0.0, 1.7049999999999996, 1, 3, 2.0, 2.0, 2.0, 3.0, 10.096420818819729, 6.990588242717956, 4.042512241910242], "isController": false}, {"data": ["L2_auction_list_50u", 250, 0, 0.0, 1.9040000000000008, 1, 5, 2.0, 3.0, 3.0, 4.0, 25.4841997961264, 24.21496718909276, 4.678739806320081], "isController": false}, {"data": ["LT-S02_auction_detail", 300, 0, 0.0, 1.6199999999999999, 1, 4, 2.0, 3.0, 3.0, 3.0, 30.278562777553493, 29.74632241622931, 5.618092702866371], "isController": false}, {"data": ["L4_auction_list_200u", 1000, 0, 0.0, 2.4649999999999985, 0, 77, 1.0, 3.0, 5.0, 43.930000000000064, 50.233586175717086, 47.73171811423118, 9.222572461948058], "isController": false}, {"data": ["LT-S05_submit_bid", 200, 0, 0.0, 2.7950000000000004, 2, 5, 3.0, 3.0, 4.0, 5.0, 10.096420818819729, 5.610218208894946, 4.565080897571811], "isController": false}, {"data": ["LT-S05_list", 200, 0, 0.0, 1.8199999999999998, 1, 3, 2.0, 3.0, 3.0, 3.0, 10.095401544596436, 9.592603225480794, 1.853452627328252], "isController": false}, {"data": ["LT-S05_detail", 200, 0, 0.0, 1.3250000000000002, 1, 3, 1.0, 2.0, 2.0, 3.0, 10.096420818819729, 9.918944671613913, 1.8733593316169417], "isController": false}, {"data": ["S00_healthz_baseline", 100, 0, 0.0, 1.08, 0, 24, 1.0, 1.0, 2.0, 23.789999999999893, 22.619316896629723, 17.074933555756616, 4.042319328206288], "isController": false}]}, function(index, item){
        switch(index){
            // Errors pct
            case 3:
                item = item.toFixed(2) + '%';
                break;
            // Mean
            case 4:
            // Mean
            case 7:
            // Median
            case 8:
            // Percentile 1
            case 9:
            // Percentile 2
            case 10:
            // Percentile 3
            case 11:
            // Throughput
            case 12:
            // Kbytes/s
            case 13:
            // Sent Kbytes/s
                item = item.toFixed(2);
                break;
        }
        return item;
    }, [[0, 0]], 0, summaryTableHeader);

    // Create error table
    createTable($("#errorsTable"), {"supportsControllersDiscrimination": false, "titles": ["Type of error", "Number of errors", "% in errors", "% in all samples"], "items": []}, function(index, item){
        switch(index){
            case 2:
            case 3:
                item = item.toFixed(2) + '%';
                break;
        }
        return item;
    }, [[1, 1]]);

        // Create top5 errors by sampler
    createTable($("#top5ErrorsBySamplerTable"), {"supportsControllersDiscrimination": false, "overall": {"data": ["Total", 3840, 0, "", "", "", "", "", "", "", "", "", ""], "isController": false}, "titles": ["Sample", "#Samples", "#Errors", "Error", "#Errors", "Error", "#Errors", "Error", "#Errors", "Error", "#Errors", "Error", "#Errors"], "items": [{"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}, {"data": [], "isController": false}]}, function(index, item){
        return item;
    }, [[0, 0]], 0);

});
