var sensorsApp = angular.module('sensorsApp', []);

var unitConverter = {
	"1":"C",
	"2":"%",
	"3":"V",
	"4":"OPEN/CLOSE" //not used ... switch hard coded in the data treatment
};

var getChart = undefined;

//AmCharts.ready(
	getChart = function (chartData) {
	    // SERIAL CHART
	    var chart = new AmCharts.AmSerialChart();
	    chart.pathToImages = "./amcharts/images/";
	    chart.dataProvider = chartData;
	    chart.categoryField = "datetime";
		chart.addListener("dataUpdated", zoomChart);
		chart.dataDateFormat = "YYYY-MM-DD";

		// GRAPH Humidity
		var graphHum = new AmCharts.AmGraph();
		graphHum.valueField = "humidity";
		graphHum.type = "line";
		graphHum.fillColors = "#01CBCE";
		graphHum.fillAlphas = 1;
		graphHum.lineColor = "#01CBCE";
		graphHum.columnWidth = 20;
		graphHum.valueAxis = "Humidity";
		graphHum.bullet = "round";
		graphHum.balloonText = "[[category]]<br><b><span style='font-size:14px;'>value: [[value]]</span></b>";
	    
		graphHum.hideBulletsCount = 50; // this makes the chart to hide bullets when there are more than 50 series in selection
		chart.addGraph(graphHum);
		
		// GRAPH Temperature
		var graph = new AmCharts.AmGraph();
		graph.valueField = "temperature";
		graph.type = "smoothedLine";
		graph.valueAxis = "Temperature";
		graph.bullet = "round";
		graph.balloonText = "[[category]]<br><b><span style='font-size:14px;'>value: [[value]]</span></b>";
	    
		graph.hideBulletsCount = 50; // this makes the chart to hide bullets when there are more than 50 series in selection
		chart.addGraph(graph);
		
		// AXES
	    // category
		
	    var categoryAxis = chart.categoryAxis;
	    categoryAxis.parseDates = true; // as our data is date-based, we set parseDates to true
	    categoryAxis.minPeriod = "ss"; //
		categoryAxis.maxSeries = 50;
		categoryAxis.groupToPeriods = ["mm", "10mm", "30mm", "hh", "DD", "WW", "MM", "YYYY"];
		
		//Value Axes
		var tempAxis = new AmCharts.ValueAxis();
		tempAxis.minimum = 10;
		tempAxis.maximum = 30;
		tempAxis.position = "left";
		tempAxis.title = "Temperature";
		tempAxis.id = "Temperature";
		tempAxis.unit = " °C";
		chart.addValueAxis(tempAxis);
		
		//Value Axes
		var humAxis = new AmCharts.ValueAxis();
		humAxis.minimum = 0;
		humAxis.maximum = 100;
		humAxis.position = "right";
		humAxis.title = "Humidité";
		humAxis.id = "Humidity";
		humAxis.unit = "%";
		chart.addValueAxis(humAxis);
		// CURSOR
	    chartCursor = new AmCharts.ChartCursor();
	    chartCursor.cursorPosition = "mouse";
		chartCursor.categoryBalloonDateFormat = "JJ:NN:SS";
	    chart.addChartCursor(chartCursor);
		
		// PERIOD
		/*
		var periodSelector = new AmCharts.PeriodSelector();
		periodSelector.periods = [
		   {period:"DD", count:1, label:"1 day"}, 
		   {period:"DD", selected:true, count:5, label:"5 days"},
		   {period:"MM", count:1, label:"1 month"},
		   {period:"YYYY", count:1, label:"1 year"},
		   {period:"YTD", label:"YTD"},
		   {period:"MAX", label:"MAX"}
		];
		chart.periodSelector = periodSelector;
		*/

	    // SCROLLBAR
	    var chartScrollbar = new AmCharts.ChartScrollbar();
	    chartScrollbar.graph = graph;
	    chartScrollbar.scrollbarHeight = 40;
	    chartScrollbar.color = "#FFFFFF";
	    chartScrollbar.autoGridCount = true;
	    chart.addChartScrollbar(chartScrollbar);
		
		// WRITE
	    //chart.write(chartId);
		chart.addListener("rendered", zoomChart);

		zoomChart();
		function zoomChart(){
			chart.zoomToIndexes(/*chart.dataProvider.length - 100*/0, chart.dataProvider.length - 1);
		}

		return chart;
	}	
//);


//Encapsulate Socket in Angular
sensorsApp.factory('socket', function ($rootScope) {
  var socket = io.connect('http://127.0.0.1:8080', {'connect timeout': 1000});
  return {
    on: function (eventName, callback) {
      socket.on(eventName, function () {  
        var args = arguments;
        $rootScope.$apply(function () {
          callback.apply(socket, args);
        });
      });
    },
    emit: function (eventName, data, callback) {
      socket.emit(eventName, data, function () {
        var args = arguments;
        $rootScope.$apply(function () {
          if (callback) {
            callback.apply(socket, args);
          }
        });
      })
    }
  };
});

sensorsApp.controller('ChartController', function($scope) {

	$scope.chartList = [];

	function isInArray(id) {
		for(var i=0; i < $scope.chartList.length; i++) {
			if($scope.chartList[i].id == id){
				return true;
			}
		}
		return false;
	}

	function addInArray(chartInfo) {
		$scope.chartList.push(chartInfo);
		$scope.chartList.sort(function(a,b) {
			if(a.id < b.id){return -1;}
			if(a.id > b.id){return 1;}
			return 0;
		});
	}

	function removeFromArray(id) {
		var idx = 0;
		while($scope.chartList[idx].id != id){
			idx++;
		}
		$scope.chartList.splice(idx,1);
	}

	function refreshAllChart() {
		for(var i=0; i < $scope.chartList.length; i++) {
			$scope.chartList[i].chart.validateNow();
		}
	}

	$scope.$on('chartUpdated', function(event, chartInfos){
		var id = chartInfos.id;
		if(!isInArray(id)) {
			chartInfos.chart = getChart(chartInfos.data);
			addInArray(chartInfos);
			
			var interval = setInterval(function() {
				chartInfos.chart.write('chart' + chartInfos.id);
				refreshAllChart();
				clearInterval(interval);
			}, 0);

		}
		else {
			removeFromArray(chartInfos.id);
			var interval = setInterval(function() {
				refreshAllChart();
				clearInterval(interval);
			}, 0);
		}
	})

	$scope.getStyle = function() {
		var width =  Math.round(100 / ($scope.chartList.length == 0 ? 1 : $scope.chartList.length));
		return {
			width : width+"%",
			display: "inline-block"
		}
	}
});

/*sensorsApp.controller('SensorGlobal', function ($scope, socket) {

	$scope.lastUpdate = "";
	$scope.tempData = "";
	$scope.humidityData = "";

	var getHumidityDisplayed = function(humidityValue) {
		var valueDisplayed = "Unknow value ...;"
		if(humidityValue) {
			valueDisplayed = humidityValue + "%";
		}
		return valueDisplayed;
	}

	var getTemperatureDisplayed = function(temperatureValue) {
		var valueDisplayed = "Unknow value ...;"
		if(temperatureValue) {
			valueDisplayed = temperatureValue + "C";
		}
		return valueDisplayed;
	}

	socket.on("currentTemp", function (data) {
		var obj = JSON.parse(data);
		var longTime = parseInt(obj.Time) * 1000;
		$scope.lastUpdate = new Date(longTime).toString();
		$scope.tempData = getTemperatureDisplayed(obj.T ? obj.T/100: undefined);
		$scope.humidityData = getHumidityDisplayed(obj.H ? obj.H/100: undefined);
	});
});*/

sensorsApp.controller('SensorListCtrl', function ($scope, socket) {
	var selectedSensor = null;
	socket.on("sensorsList", function (data) {
		var sensors_local = JSON.parse(data);
		$scope.sensors = sensors_local;
		var idx = 0;
		$.each(sensors_local, function (key, data) {
			data.idx = idx++;
			socket.emit("getDataForSensor", JSON.stringify(data));
		});
		
	});
	socket.on("sensorData", function (sensorData) {
		var sensor = JSON.parse(sensorData);

		var dataReceived = sensor.data;
		sensor.data = [];
		
		//build data
		$.each(dataReceived, function(idx, data){
			var aData = {};
			var aUnit = {};
			aData.value = data.data / 100;
			aUnit.id = data.unit_ID;
			//switch degeu... sorry mika
			if(aUnit.id == 4){
				if(aData.value == "0"){
					aUnit.value = "CLOSE";
				} else {
					aUnit.value = "OPEN";
				}
			} else {
				aUnit.value = unitConverter[aUnit.id];
			}
			aData.unit = aUnit;
			sensor.data.push(aData);
		});

		//sort data
		sensor.data.sort(function(a,b) {
			if(a.unit.id < b.unit.id) {
				return -1;
			}

			if(a.unit.id > b.unit.id) {
				return 1;
			}
			return 0;
		});

		sensor.chartDisplayed = false;

		$scope.sensors[sensor.idx] = sensor;
		$scope.lastUpdate = new Date().toString();
	});
	
	$scope.openChart = function (sensor){
		if(sensor.chartDisplayed) {
			sensor.chartDisplayed = false;
			var chart = {
                    id: sensor.id,
                    data: null
            }
            $scope.$parent.$broadcast('chartUpdated', chart);
		} else {
			sensor.chartDisplayed = true;
            socket.emit("getAllDataForSensor", JSON.stringify(sensor));
		}

	}

	$scope.selectSensor = function (sensor){
		$scope.selectedSensor = sensor;
	}
	socket.on("sensorAllData", function (sensorAllData) {
		var sensorData = JSON.parse(sensorAllData);
		//sensorData.data = sensorData.data.splice(sensorData.data.length -200, sensorData.data.length);
		$.each(sensorData.data, function (index, value) {
			value.datetime = new Date(value.datetime * 1000);
			value.temperature = value.temperature / 100;
			value.humidity = value.humidity / 100;
		});
		var chart = {
			id: sensorData.id,
			data: sensorData.data
		}
		$scope.$parent.$broadcast('chartUpdated', chart);
	});
});


