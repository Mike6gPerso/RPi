//this port has to be configured the same number in file /etc/nginx/sites-available/default on line "proxy_pass http://localhost:8080"
var io = require('socket.io').listen(8080);

//FileSystem watcher
var fs = require('fs');


/////////////////////////////////////////
//DB
var file ="../SQL/temp_fakeData.sl3";
var sqlite3 = require("sqlite3").verbose();
var db = new sqlite3.Database(file);

//Folders
//var dataFolder = "/home/pi/virtualWire/data/";

db.serialize(function() {
  
  db.each("SELECT data AS Temperature FROM data where unit_ID=1 order by timestamp DESC limit 1;", function(err, row) {
    console.log(row.Temperature);
  });
});




////////////////////////////////////////////


 
 //authorize handshake - make sure the request is coming from nginx, not from the outside world
//if you comment out this section, you will be able to hit this socket directly at the port it's running at, from anywhere!
//this was tested on Socket.IO v1.2.1 and will not work on older versions
io.use(function(socket, next) {
var handshakeData = socket.request;
console.log("AUTHORIZING CONNECTION FROM " + handshakeData.connection.remoteAddress + ":" + handshakeData.connection.remotePort);
if (handshakeData.connection.remoteAddress == "localhost" || handshakeData.connection.remoteAddress == "127.0.0.1")
next();
next(new Error('REJECTED IDENTITY, not coming from localhost'));
});

//var fileName = "/home/pi/virtualWire/data/20150218_220006_RF.csv";

//
function isDataFile(element) {
  return element.indexOf("20") == 0;
}


io.sockets.on('connection', function (socket) {

	// when the client emits 'sendchat', this listens and executes
	/*
	socket.on('getData', function (data) {
		var filtered = fs.readdirSync(dataFolder).filter(isDataFile).reverse();
		//console.log(filtered);
		//console.log(dataFolder + filtered[0]);
		fs.readFile(dataFolder + filtered[0], 'utf8', function (err, data) {
				if (err) throw err;
				socket.emit("tempData", data);
				//socket.emit("ok", "hello");
				//console.log(data);
				});
	});
	*/
	var sqlSensorsList = "select * from sensors where active = 1;";
	//var sensors = [];
	db.all(sqlSensorsList, function(err, rows) {
		//sensors.push.apply(sensors, rows);
		console.log("emit list of sensors");
		console.log(rows);
		socket.emit("sensorsList", JSON.stringify(rows));
	});
	
	socket.on('getDataForSensor', function (sensor) {
		//console.log(sensor);
		var obj = JSON.parse(sensor);
		var sqlSensorData = "Select distinct data, unit_ID \
		from data, units, \
		( select timestamp as last_date from data where sensor_ID = " + obj.id + " and group_ID = " + obj.group_ID +" order by timestamp desc limit 1) T1 \
		where data.unit_ID = units.id and sensor_ID = " + obj.id + " and group_ID = " + obj.group_ID +" \
		and data.timestamp = T1.last_date \
		order by timestamp desc;";
		console.log(sqlSensorData);
		db.all(sqlSensorData, function (err, data) {
			obj.data = data;
			socket.emit("sensorData", JSON.stringify(obj));
		});
	});
	
	socket.on('getAllDataForSensor', function (sensor) {
		var obj = JSON.parse(sensor);
		//console.log(obj.id);
		/*var sqlSensorData = "Select distinct data, unit \
		from data, units, \
		( select timestamp as last_date from data order by timestamp desc limit 1) T1 \
		where data.unit_ID = units.id and sensor_ID = " + obj.id + " and group_ID = " + obj.group_ID +" \
		and data.timestamp = T1.last_date \
		order by timestamp desc;";
		and data.timestamp > ((strftime('%s','now')) - (86400 *5)) \
		*/
		var sqlSensorData = "Select distinct data.timestamp as datetime, data.data as temperature, Hum.data as humidity \
		from data, units, \
			( select id, group_ID from sensors	\
			where id = " + obj.id + " and group_ID = " + obj.group_ID + " \
		) T2, data Hum  \
		where data.unit_ID = units.id and data.sensor_ID = T2.id and data.group_ID = T2.group_ID \
		and data.unit_ID = 1	\
		and Hum.unit_ID = 2 and Hum.group_ID = T2.group_ID \
		and Hum.sensor_ID = T2.id \
		and Hum.timestamp = data.timestamp \
		order by data.timestamp asc;";
		//console.log(sqlSensorData);
		db.all(sqlSensorData, function (err, data) {
			if(err)
				console.log(err);
			//console.log("got data !");
			obj.data = data;
			console.log(data);
			socket.emit("sensorAllData", JSON.stringify(obj));
		});
	});
/*
	socket.on('getTemp', function(data) {
		db.each("SELECT data AS Temperature FROM data where unit_ID=1 order by timestamp DESC limit 1;", function(err, row) {
			socket.emit("currentTemp", row.Temperature);
		});
	});
	socket.on('getHumidity', function(data) {
		db.each("SELECT data AS Humidity FROM data where unit_ID=2 order by timestamp DESC limit 1;", function(err, row) {
			socket.emit("currentHumidity", row.Humidity);
		});
	});
	*/
	
	//// fs.watch ////
	
	/*fs.watch(dataFolder, { persistent: true, recursive: true }, function( event, filename) {
		db.each("SELECT T1.timestamp as Time, T1.data as T, T2.data as H from (SELECT * FROM data where unit_ID=1 order by timestamp DESC limit 1 ) as T1, (SELECT data FROM data where unit_ID=2 order by timestamp DESC limit 1) as T2", function(err, row) {
			socket.emit("currentTemp", "{\"Time\": \"" + row.Time +"\", \"T\":\"" + row.T + "\", \"H\":\"" + row.H +"\"}");
		});
	});*/
	db.each("SELECT T1.timestamp as Time, T1.data as T, T2.data as H from (SELECT * FROM data where unit_ID=1 order by timestamp DESC limit 1 ) as T1, (SELECT data FROM data where unit_ID=2 order by timestamp DESC limit 1) as T2", function(err, row) {
			socket.emit("currentTemp", "{\"Time\": \"" + row.Time +"\", \"T\":\"" + row.T + "\", \"H\":\"" + row.H +"\"}");
	});
	
	
	
	/*
	var sqlMainQuery = "select T1.*, data.* \
from  \
( \
	select id, group_ID, name from sensors where active=1 \
) T1,  \
( \
	select timestamp as last_date from data order by timestamp desc limit 1 \
) T2, data \
where T1.id = data.sensor_ID \
and T1.group_ID = data.group_ID \
and timestamp = T2.last_date \
and unit_id = 1 \
ORDER BY data.timestamp DESC \
;";
	db.each(sqlMainQuery, function(err, row) {
		var result = { "S" : row.id, "G": row.group_ID, "N": row.name, "T" : row.data, "Time" : row.timestamp };
		socket.emit("mainPageInfo", JSON.stringify(result));
	});
	*/
	
	
});

//db.close();