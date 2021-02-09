// https://blog.cfm-art.net/archives/1077
// https://otomizu.work/2020/07/25/arduino-zero-pad/#toc_id_1

#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "ESP8266.h"

#define PIR_MOTION_SENSOR_PIN     2
#define BUZZER_PIN                3
#define BUTTON_ONE_PIN            4
#define LED_YELLOW_PIN            5
#define FAN_REVERSE_PIN           6
#define FAN_NORMAL_PIN            7
#define BUTTON_TWO_PIN            8
#define SERVO_DOOR_PIN            9
#define SERVO_WINDOW_PIN         10
#define RELAY_PIN                12
#define LED_WHITE_PIN            13
#define MQ2_SENSOR_PIN           A0
#define PHOTOCELL_SENSOR_PIN     A1
#define SOIL_HUMIDITY_SENSOR_PIN A2
#define STEAM_SENSOR             A3

#define GAS_DANGER_VALUE         700
#define PHOTOCELL_NIGHT_VALUE    300
#define PIR_MOTION_MOVE_VALUE    1
#define STEAM_RAIN_VALUE         1000
#define SOIL_DROUGHT_VALUE       1000

struct ExtInfo {
	int gasValue;
	int photoCellSensorValue;
	int pirMotionSensorValue;
	int steamSensorValue;
	int soilHumiditySensorValue;
};

struct State {
	bool gasDanger;
	bool nightMonitoring;
	bool rain;
	bool soilDrought;
	bool windowOpen;
	bool doorOpen;
	bool relayState;
	bool whiteLEDState;
	bool yellowLEDState;
	char fanState;
};

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servoWindow;
Servo servoDoor;

const String doorPassword = ".--.-.";
int sendCount = 0;
int recvCount = 0;
String password;

State stateInfo;
ExtInfo extInfo;

void setup() {
	// シリアルポートの設定
	Serial.begin(115200);

	// LECディスプレイの設定
	lcd.init();
	lcd.backlight();

	// サーボモーターの設定
	// 窓側のサーボモータ
	servoWindow.attach(SERVO_WINDOW_PIN);
	servoWindow.write(0);
	// ドア側のサーボモータ
	servoDoor.attach(SERVO_DOOR_PIN);
	servoDoor.write(0);

	// PIN設定
	// ファン設定
	pinMode(FAN_REVERSE_PIN, OUTPUT);
	pinMode(FAN_NORMAL_PIN, OUTPUT);
	stopFan();

	// ボタン1設定
	pinMode(BUTTON_ONE_PIN, INPUT);
	// ボタン2設定
	pinMode(BUTTON_TWO_PIN, INPUT);
	// PIRモーションセンサー設定
	pinMode(PIR_MOTION_SENSOR_PIN, INPUT);
	// ブザー設定
	pinMode(BUZZER_PIN, OUTPUT);
	// ガスセンサー設定
	pinMode(MQ2_SENSOR_PIN, INPUT);
	// 光電セルセンサー設定
	pinMode(PHOTOCELL_SENSOR_PIN, INPUT);
	// 白LED設定
	pinMode(LED_WHITE_PIN, OUTPUT);
	// 水蒸気センサ設定
	pinMode(STEAM_SENSOR, INPUT);
	// 土壌湿度センサ設定
	pinMode(SOIL_HUMIDITY_SENSOR_PIN, INPUT);
	// リレー設定
	pinMode(RELAY_PIN, OUTPUT);
	// 黄LED設定
	pinMode(LED_YELLOW_PIN, OUTPUT);

	// 状態の初期化
	stateInfo.gasDanger       = false;
	stateInfo.nightMonitoring = false;
	stateInfo.rain            = false;
	stateInfo.soilDrought     = false;
	stateInfo.windowOpen      = false;
	stateInfo.doorOpen        = false;
	stateInfo.relayState      = false;
	stateInfo.whiteLEDState   = false;
	stateInfo.yellowLEDState  = false;
	stateInfo.fanState        = '0';
}

void loop() {
	// 各種センサーから値と読み取り
	extInfo = checkExternalInformation();

	// Wi-Fiで親機にデータ送信&受信
	sendWifiData(extInfo);
	recvWifiData();

	// 緊急プロセス
	emergencyProcess(extInfo, stateInfo);
	// 通常プロセス
	normalProcess(extInfo, stateInfo);
	// 0.1秒待機
	delay(100);
}

// 各種センターの読み取り
ExtInfo checkExternalInformation() {
	ExtInfo result;
	result.gasValue                = getGasValue();
	result.photoCellSensorValue    = getPhotoCellValue();
	result.pirMotionSensorValue    = getPIRMotionSensorValue();
	result.steamSensorValue        = getSteamSensorValue();
	result.soilHumiditySensorValue = getSoilHumiditySensorValue();
	return result;
}

// Wi-fiモジュール側に送るデータの処理
void sendWifiData(ExtInfo extInfo) {
	if(Serial.availableForWrite() >= 44) {
		if(sendCount == 0) {
			Serial.print("A");
			Serial.print(strPad(extInfo.gasValue, 4));
			Serial.print("B");
			Serial.print(strPad(extInfo.photoCellSensorValue, 4));
			Serial.print("C");
			Serial.print(strPad(extInfo.pirMotionSensorValue, 4));
			Serial.print("D");
			Serial.print(strPad(extInfo.steamSensorValue, 4));
			Serial.print("E");
			Serial.print(strPad(extInfo.soilHumiditySensorValue, 4));
			sendCount = 100;
		} else {
			sendCount--;
		}
	}
}

// Wi-fiモジュール側から送られてきたデータの処理
void recvWifiData() {
	if (Serial.available()) {
		stateInfo.gasDanger       = (char)Serial.read() == 1 ? true : false;
		stateInfo.nightMonitoring = (char)Serial.read() == 1 ? true : false;
		stateInfo.rain            = (char)Serial.read() == 1 ? true : false;
		stateInfo.soilDrought     = (char)Serial.read() == 1 ? true : false;
		stateInfo.windowOpen      = (char)Serial.read() == 1 ? true : false;
		stateInfo.doorOpen        = (char)Serial.read() == 1 ? true : false;
		stateInfo.relayState      = (char)Serial.read() == 1 ? true : false;
		stateInfo.whiteLEDState   = (char)Serial.read() == 1 ? true : false;
		stateInfo.yellowLEDState  = (char)Serial.read() == 1 ? true : false;
		stateInfo.fanState        = (char)Serial.read();
	}
}

// 緊急性のあるプロセス
void emergencyProcess(ExtInfo extInfo, State stateInfo) {
	// ガス検知
	if((extInfo.gasValue > GAS_DANGER_VALUE) || stateInfo.gasDanger == true) {
		gasDangerProcess();
	}
	// 不審者探知(光電セルセンサー&人探知)
	if((extInfo.photoCellSensorValue < PHOTOCELL_NIGHT_VALUE) || stateInfo.nightMonitoring == true) {
		if((extInfo.pirMotionSensorValue == PIR_MOTION_MOVE_VALUE)) {
			onWhiteLED();
		} else {
			offWhiteLED();
		}
	}
	// 水蒸気探知
	if((extInfo.steamSensorValue > STEAM_RAIN_VALUE) || stateInfo.rain == true) {
		steamRainProcess();
	}
	// 土壌湿度探知
	if((extInfo.soilHumiditySensorValue > SOIL_DROUGHT_VALUE) || stateInfo.soilDrought == true) {
		soilDroughtProcess();
	}
}

// ガス検知時の処理
void gasDangerProcess() {
	// LEDディプレイ表示
	setLCD(0, 1, "Gas Danger!!");
	// ブザー
	tone(BUZZER_PIN, 440);
	delay(125);
	delay(100);
	noTone(BUZZER_PIN);
	delay(100);
	tone(BUZZER_PIN, 440);
	delay(125);
	delay(100);
	noTone(BUZZER_PIN);
}

// 水蒸気探知時の処理
void steamRainProcess() {
	// LEDディスプレイ表示
	setLCD(0, 1, "Rain");
	// 窓を閉める
	servoWindow.write(180);
}

// 土壌湿度探知時の処理
void soilDroughtProcess() {
	// LEDディスプレイ表示
	setLCD(0, 1, "Hydropenia!!");
	// ブザー
	tone(BUZZER_PIN, 440);
	delay(125);
	delay(100);
	noTone(BUZZER_PIN);
	delay(100);
	tone(BUZZER_PIN, 440);
	delay(125);
	delay(100);
	noTone(BUZZER_PIN);
}

// 通常のプロセス
void normalProcess(ExtInfo extInfo, State stateInfo) {
	// ドア開け(ボタンパスワード入力)
	int button1Count = 0;
	int button2Count = 0;

	byte button1Value = getButton1();
	byte button2Value = getButton2();

	// ボタン1の入力処理
	if (button1Value == 0) {
		while (button1Value == 0 && button1Count < 10) {
			button1Value = getButton1();
			button1Count++;
			delay(100);
		}
	}
	// ボタン1に対するパスコード入力処理
	if (button1Count >= 1 && button1Count < 5) {
		// 入力「.」
		password = String(password) + String(".");
		setLCD(0, 1, password);
	} else if(button1Count >= 5) {
		// 入力「-]
		password = String(password) + String("-");
		setLCD(0, 1, password);
	}

	// ボタン2の入力処理
	if (button2Value == 0) {
		while (button2Value == 0 && button2Count < 10) {
			button2Value = getButton2();
			button2Count++;
			delay(100);
		}
	}
	// ボタン2に対するパスコード認証処理
	if (button2Count >= 1) {
		if(password == doorPassword) {
			password = "";
			setLCD(0, 0, "OPEN!");
			servoDoor.write(100);
			delay(5000);
			setLCD(0, 0, "PassWord?:");
		} else {
			password = "";
			setLCD(0, 0, "ERROR!");
			delay(5000);
			setLCD(0, 0, "PassWord?:");
		}
	}

	// ドア開け(親機入力)
	if (stateInfo.doorOpen == true) {
		servoDoor.write(90);
	} else if(stateInfo.doorOpen == false) {
		servoDoor.write(0);
	}

	// 窓開け閉め
	if (stateInfo.windowOpen == true) {
		servoWindow.write(90);
	} else if(stateInfo.windowOpen == false) {
		servoWindow.write(0);
	}

	// 黄色LEDのONOFF
	if (stateInfo.yellowLEDState == true) {
		onYellowLED();
	} else if(stateInfo.yellowLEDState == false) {
		offYellowLED();
	}
	// 白色LEDのONOFF
	if (stateInfo.whiteLEDState == true) {
		onWhiteLED();
	} else if(stateInfo.whiteLEDState == false) {
		offWhiteLED();
	}

	// リレーのONOFF
	if (stateInfo.relayState == true) {
		onRelay();
	} else if(stateInfo.relayState == false) {
		offRelay();
	}
	// ファンの調節
	if (stateInfo.fanState == '1') {
		normalFan();
	} else if(stateInfo.fanState == '2') {
		reverseFan();
	} else if(stateInfo.fanState == '0') {
		stopFan();
	}
}

// LED表示関数
void setLCD(int column, int line, String printWord) {
	lcd.clear();
	lcd.setCursor(column, line);
	lcd.print(printWord);
}

// 各LEDのONとOFF関数
void onWhiteLED() { digitalWrite(LED_WHITE_PIN, HIGH); }
void offWhiteLED() { digitalWrite(LED_WHITE_PIN, LOW); }
void onYellowLED() { digitalWrite(LED_YELLOW_PIN, HIGH); }
void offYellowLED() { digitalWrite(LED_YELLOW_PIN, LOW); }

// 各種センサーの値取得関数
int getGasValue() { return analogRead(MQ2_SENSOR_PIN); }
int getPhotoCellValue() { return analogRead(PHOTOCELL_SENSOR_PIN); }
int getSteamSensorValue() { return analogRead(STEAM_SENSOR); }
int getPIRMotionSensorValue() {return digitalRead(PIR_MOTION_SENSOR_PIN); }
int getSoilHumiditySensorValue() {return analogRead(SOIL_HUMIDITY_SENSOR_PIN); }

// ボタン1とボタン2のONOFF値取得関数
byte getButton1(){ return digitalRead(BUTTON_ONE_PIN);}
byte getButton2(){ return digitalRead(BUTTON_TWO_PIN);}

// ファンの操作
void stopFan() {
	digitalWrite(FAN_REVERSE_PIN, LOW);
	digitalWrite(FAN_NORMAL_PIN, LOW);
}

void normalFan() {
	digitalWrite(FAN_REVERSE_PIN, LOW);
	digitalWrite(FAN_NORMAL_PIN, HIGH);
} 

void reverseFan() {
	digitalWrite(FAN_REVERSE_PIN, HIGH);
	digitalWrite(FAN_NORMAL_PIN, LOW);
}

// リレーのONOFF
void offRelay() { digitalWrite(RELAY_PIN, LOW); }
void onRelay() { digitalWrite(RELAY_PIN, HIGH); }

// 0埋め
String strPad(int num,int zeroCount){
	String str = String(num);
	String returnStr = "";
	if(zeroCount <= str.length()){
		return str;
	}
	for(int i = 0;i < zeroCount - str.length();i++){
		returnStr += '0';
	}
	return returnStr + str;
}