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
    createTable($("#statisticsTable"), {"supportsControllersDiscrimination": true, "overall": {"data": ["Total", 3840, 0, 0.0, 2.5052083333333366, 0, 44, 2.0, 3.0, 4.0, 10.0, 37.205697122371866, 173.1204696432032, 8.586956357184382], "isController": false}, "titles": ["Label", "#Samples", "FAIL", "Error %", "Average", "Min", "Max", "Median", "90th pct", "95th pct", "99th pct", "Transactions/s", "Received", "Sent"], "items": [{"data": ["LT-S02_bid_history", 300, 0, 0.0, 2.033333333333333, 1, 14, 2.0, 3.0, 5.0, 11.980000000000018, 33.04328670558432, 16.553912187465578, 7.002337124132613], "isController": false}, {"data": ["L1_auction_list_20u", 100, 0, 0.0, 2.82, 2, 5, 3.0, 4.0, 4.0, 5.0, 20.999580008399832, 170.45752834943303, 3.855391642167157], "isController": false}, {"data": ["L3_auction_list_100u", 500, 0, 0.0, 2.972, 1, 30, 2.0, 4.0, 6.0, 15.0, 33.63832077502691, 273.04855691603876, 6.1757854547900966], "isController": false}, {"data": ["LT-S03_notifications", 200, 0, 0.0, 2.444999999999999, 1, 4, 2.0, 3.0, 4.0, 4.0, 18.409425625920473, 8.054123711340205, 7.388939386966126], "isController": false}, {"data": ["LT-S04_submit_bid", 90, 0, 0.0, 3.98888888888889, 2, 44, 3.5, 4.0, 5.0, 44.0, 9.310023792283024, 5.191429282610945, 4.2458799912072], "isController": false}, {"data": ["LT-S05_bid_history", 200, 0, 0.0, 1.8850000000000013, 1, 12, 2.0, 2.0, 3.9499999999999886, 10.0, 10.094891984655765, 6.200866267918433, 2.1392495709670905], "isController": false}, {"data": ["LT-S05_notifications", 200, 0, 0.0, 2.4850000000000008, 1, 18, 2.0, 3.0, 4.949999999999989, 15.930000000000064, 10.094891984655765, 7.028962876034726, 4.051758403997577], "isController": false}, {"data": ["L2_auction_list_50u", 250, 0, 0.0, 2.3600000000000017, 1, 5, 2.0, 3.0, 3.0, 4.0, 25.47640884540915, 206.79678742484458, 4.677309436461836], "isController": false}, {"data": ["LT-S02_auction_detail", 300, 0, 0.0, 2.7333333333333343, 1, 40, 2.0, 3.0, 6.0, 26.920000000000073, 33.03237172428981, 32.51624091609777, 6.161311522792336], "isController": false}, {"data": ["L4_auction_list_200u", 1000, 0, 0.0, 2.3430000000000053, 1, 6, 2.0, 3.0, 3.0, 4.0, 50.22349455075084, 407.67352217367284, 9.220719702676911], "isController": false}, {"data": ["LT-S05_submit_bid", 200, 0, 0.0, 3.5799999999999996, 2, 18, 3.0, 4.0, 5.0, 17.90000000000009, 10.094891984655765, 5.62908527660004, 4.603822809408439], "isController": false}, {"data": ["LT-S05_list", 200, 0, 0.0, 2.840000000000001, 1, 16, 3.0, 3.0, 4.0, 13.960000000000036, 10.093873019077419, 81.9338598970425, 1.853171999596245], "isController": false}, {"data": ["LT-S05_detail", 200, 0, 0.0, 1.8649999999999995, 1, 9, 2.0, 2.0, 3.9499999999999886, 9.0, 10.095401544596436, 9.937660895462116, 1.8830289990409368], "isController": false}, {"data": ["S00_healthz_baseline", 100, 0, 0.0, 1.1000000000000003, 0, 26, 1.0, 1.0, 2.0, 25.759999999999877, 22.696323195642304, 16.734105481162054, 4.056081196096232], "isController": false}]}, function(index, item){
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
