#include <Time.h> // from: http://www.pjrc.com/teensy/td_libs_Time.html
#include <TimeAlarms.h> 

#define ALARM_PIN 8
#define SNOOZE_PIN 7
#define CANCEL_PIN 6

#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

#include <LiquidCrystal.h>
#define NR_CHARS_PER_ROW 16
#define ROWS 2
//LiquidCrystal(rs, enable, d4, d5, d6, d7) 
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void display(char* msg){
	lcd.clear();
	String s = String(msg);
	int newLine = s.indexOf("\n");
	if ( newLine == -1 ){
		lcd.print(msg);
	} else {
		lcd.print(s.substring(0, newLine));
		lcd.setCursor(0, 1);
		lcd.print(s.substring(newLine+1, s.length()));
	}
}

void processSyncMessage(){
	// if time sync available from the serial port
	// update time and return true
	// Linux: TZ_adjust=2;  echo "T$(($(date +%s)+60*60*$TZ_adjust))" >> /dev/ttyACM0
	while(Serial.available() >=  TIME_MSG_LEN ){
		char c = Serial.read() ; 
		Serial.print(c);  
		if( c == TIME_HEADER ) {       
			time_t pctime = 0;
			for(int i=0; i < TIME_MSG_LEN -1; i++){   
				c = Serial.read();          
				if( c >= '0' && c <= '9'){   
					pctime = (10 * pctime) + (c - '0') ;   
				}
			}   
		setTime(pctime);
		}  
	}
}

void syncTime(){
	while (timeStatus() == timeNotSet){ 
		Serial.println("waiting for sync message");
		processSyncMessage();
		delay(1000);
	}
}

String getTime(){
	time_t t = now();
	return String(String(hour(t)) + ":" + String(minute(t)) + ":" + String(second(t))); 
}

void beep(int freq, int period, int silence){
	analogWrite(ALARM_PIN, freq); 
	delay(period);
	analogWrite(ALARM_PIN, 0);
	delay(silence);
}

void soundAlarm(){
	beep(200, 250, 250);
	beep(200, 250, 250);
	beep(255, 250, 0);
}

void alarm(){
	lcd.clear();
	display("Beep! Beep!\nSNOOZE or CANCEL");
	soundAlarm();
	if (digitalRead(SNOOZE_PIN) == HIGH){
		soundAlarm();
	}
}

void setup(){
	Serial.begin(9600);
	lcd.begin(NR_CHARS_PER_ROW, ROWS);
	pinMode(ALARM_PIN, OUTPUT);
	pinMode(SNOOZE_PIN, INPUT_PULLUP);
	
	display("Waiting on time\nsync from serial");
	syncTime();
	
	time_t t = now();
	Alarm.alarmOnce(hour(t), minute(t), second(t) + 10, alarm);
}

void loop(){
	lcd.clear();
	lcd.print("Time is:" + getTime());
	Alarm.delay(100);
}
