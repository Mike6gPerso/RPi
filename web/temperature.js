var chart;
var chartData = {};
var sensorsApp = angular.module('sensorsApp', []);

var unitConverter = {
	"1":"C",
	"2":"%"
};

sensorsApp.controller('SensorGlobal', function ($scope, socket) {

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
});

sensorsApp.controller('SensorListCtrl', function ($scope, socket) {
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
		
		$.each(dataReceived, function(idx, data){
			var aData = {};
			var aUnit = {};
			aData.value = data.data / 100;
			aUnit.id = data.unit_ID;
			aUnit.value = unitConverter[aUnit.id];
			aData.unit = aUnit;
			sensor.data.push(aData);
		});

		$scope.sensors[sensor.idx] = sensor;
		$scope.lastUpdate = new Date().toString();
	});
	
	$scope.openChart = function (sensor){
		socket.emit("getAllDataForSensor", JSON.stringify(sensor));
	}
	socket.on("sensorAllData", function (sensorAllData) {
		var sensorData = JSON.parse(sensorAllData);
		console.log(sensorData);
		//sensorData.data = sensorData.data.splice(sensorData.data.length -200, sensorData.data.length);
		$.each(sensorData.data, function (index, value) {
			value.datetime = new Date(value.datetime * 1000);
			value.temperature = value.temperature / 100;
			value.humidity = value.humidity / 100;
		});
		chart.dataProvider = sensorData.data;
		chart.validateData();
	});
});

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


AmCharts.ready(function () {
    // SERIAL CHART
    chart = new AmCharts.AmSerialChart();
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
    chart.write("chartdiv");
	chart.addListener("rendered", zoomChart);

	
	zoomChart();
	function zoomChart(){
		chart.zoomToIndexes(/*chart.dataProvider.length - 100*/0, chart.dataProvider.length - 1);
	}
	
});
