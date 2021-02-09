#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

#define SMART_HOME_RX_PIN 13
#define SMART_HOME_TX_PIN 15

SoftwareSerial smartHomeSerial(SMART_HOME_RX_PIN, SMART_HOME_TX_PIN);
WiFiServer server(80);
WiFiClient client;

char gasValue[] = "0000";
char photoCellSensorValue[] = "0000";
char pirMotionSensorValue[] = "0000";
char steamSensorValue[] = "0000";
char soilHumiditySensorValue[] = "0000";

const char* ssid = "";
const char* password = "";
String state;
int sendCount = 0;

void setup() {
	// シリアルの初期化
	Serial.begin(9600);
	smartHomeSerial.begin(115200);

	// Wi-fiの初期化
	initWifi();
}

void initWifi() {
	Serial.println();
	// SSIDの表示
	Serial.print("Connecting to ");
	Serial.println(ssid);

	// Wi-fiに接続
	WiFi.begin(ssid, password);

	// 判定
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("");
	Serial.println("WiFi connected");

	// サーバーの開始
	server.begin();
	Serial.println("Server started");

	// IPアドレスの表示
	Serial.println(WiFi.localIP());
}
void parentRecvSendProcess() {
	// クライアント生成
	String req;

	client = server.available();
	delay(1);
	while(client) {
		if(client.available()){
			req = client.readStringUntil('\n');
			Serial.println(req);
			if (req.indexOf("GET /") >= 0){
				// リクエスト処理
				state = "";
				Serial.println("-----from Browser FirstTime HTTP Request---------");
				for(int i = 5; i < 15; i++) {
					state += req[i];
				}
				Serial.println(state);
				while(req.indexOf("\r") != 0){
					req = client.readStringUntil('\n');
					Serial.println(req);
				}
				req = "";

				// 待機時間
				delay(10);

			 	// レスポンス処理
				client.print(F("HTTP/1.1 200 OK\r\n"));
				client.print(F("Content-Type:text/html\r\n"));
				client.print(F("Connection:close\r\n\r\n"));
				client.print(gasValue);
				client.print(F(","));
				client.print(photoCellSensorValue);
				client.print(F(","));
				client.print(pirMotionSensorValue);
				client.print(F(","));
				client.print(steamSensorValue);
				client.print(F(","));
				client.print(soilHumiditySensorValue);
				
				// 終了処理
				delay(10);
				client.stop();
				delay(10);
				Serial.println("\nGET HTTP client stop--------------------");
				req = "";
			}
		}
		smartHomeRecvSendProcess();
		delay(1000);
	}
	smartHomeRecvSendProcess();
	delay(1000);
}

void smartHomeRecvSendProcess() {
	smartHomeSerial.listen();
	if (smartHomeSerial.isListening()) {
		if (state == "") {
			state = "0000000000";
		}

		if(sendCount == 0) {
			smartHomeSerial.print(state);
			sendCount = 1;
		} else {
			sendCount--;
		}
	}

	if (smartHomeSerial.available()) {
		char test = (char) smartHomeSerial.read();
		switch (test)
		{
		case 'A':
			gasValue[0] = (char) smartHomeSerial.read();
			gasValue[1] = (char) smartHomeSerial.read();
			gasValue[2] = (char) smartHomeSerial.read();
			gasValue[3] = (char) smartHomeSerial.read();
			smartHomeSerial.read();
		case 'B':
			photoCellSensorValue[0] = (char) smartHomeSerial.read();
			photoCellSensorValue[1] = (char) smartHomeSerial.read();
			photoCellSensorValue[2] = (char) smartHomeSerial.read();
			photoCellSensorValue[3] = (char) smartHomeSerial.read();
			smartHomeSerial.read();
		case 'C':
			pirMotionSensorValue[0] = (char) smartHomeSerial.read();
			pirMotionSensorValue[1] = (char) smartHomeSerial.read();
			pirMotionSensorValue[2] = (char) smartHomeSerial.read();
			pirMotionSensorValue[3] = (char) smartHomeSerial.read();
			smartHomeSerial.read();
		case 'D':
			steamSensorValue[0] = (char) smartHomeSerial.read();
			steamSensorValue[1] = (char) smartHomeSerial.read();
			steamSensorValue[2] = (char) smartHomeSerial.read();
			steamSensorValue[3] = (char) smartHomeSerial.read();
			smartHomeSerial.read();
		case 'E':
			soilHumiditySensorValue[0] = (char) smartHomeSerial.read();
			soilHumiditySensorValue[1] = (char) smartHomeSerial.read();
			soilHumiditySensorValue[2] = (char) smartHomeSerial.read();
			soilHumiditySensorValue[3] = (char) smartHomeSerial.read();
		default:
			break;
		}
	}
}
