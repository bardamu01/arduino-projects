#include <Time.h> // from: http://www.pjrc.com/teensy/td_libs_Time.html
#include <TimeAlarms.h> 

#define SNOOZE_SECONDS 60

#define ALARM_PIN   8
#define SNOOZE_PIN  7
#define CANCEL_PIN  6

#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER   'T'   // Header tag for serial time sync message
#define ALARM_HEADER  'A'	
#define TIME_REQUEST   7    // ASCII bell character requests a time sync message 

#include <LiquidCrystal.h>
#define LCD_CHARS_PER_ROW 16
#define LCD_ROWS 2
//LiquidCrystal(rs, enable, d4, d5, d6, d7) 
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void display(String msg){
	lcd.clear();
	int newLine = msg.indexOf("\n");
	if ( newLine == -1 ){
		lcd.print(msg);
	} else {
		lcd.print(msg.substring(0, newLine));
		lcd.setCursor(0, 1);
		lcd.print(msg.substring(newLine+1, msg.length()));
	}
}

void display(char* msg){
	display(String(msg));
}

time_t processTimeMessage(char header){
	// return time message sent from serial
	// Linux: TZ_adjust=2;  echo "T$(($(date +%s)+60*60*$TZ_adjust))" >> /dev/ttyACM0
	while(Serial.available() >=  TIME_MSG_LEN ){
		char c = Serial.read() ; 
		if( c == header ) {       
			time_t pctime = 0;
			for(int i=0; i < TIME_MSG_LEN -1; i++){   
				c = Serial.read();          
				if( c >= '0' && c <= '9'){   
					pctime = (10 * pctime) + (c - '0') ;   
				}
			}
		return pctime;
		}  
	}
	return NULL;
}

void syncTime(){
	time_t time;
	while (timeStatus() == timeNotSet){ 
		time = processTimeMessage(TIME_HEADER);
		if (time != NULL){
			setTime(time);
		}
		delay(100);
	}
}

String formatTime(time_t t){
	return String(String(hour(t)) + ":" + String(minute(t)) + ":" + String(second(t))); 
}

time_t getAlarmFromSerial(){
	time_t alarm = NULL;
	while (alarm == NULL){
		alarm = processTimeMessage(ALARM_HEADER);
		delay(100);
	}
	return alarm;
}

void beep(int freq, int period, int silence){
	analogWrite(ALARM_PIN, freq); 
	delay(period);
	analogWrite(ALARM_PIN, 0);
	delay(silence);
}

void soundAlarm();

bool snoozed(){
	bool snoozePressed = digitalRead(SNOOZE_PIN) == LOW;
	if (snoozePressed){
		Alarm.timerOnce(SNOOZE_SECONDS, soundAlarm);
		display("Snoozed! Shh for:\n" + String(SNOOZE_SECONDS) + " seconds");
		delay(1500);
	}
	return snoozePressed;
}

bool cancelled(){
	bool cancelPressed = digitalRead(CANCEL_PIN) == LOW;
	if (cancelPressed){
		display("Cancelled!");
		delay(1000);
	}
	return cancelPressed;
}

void soundAlarm(){
	display("Beep! Beep!\nSNOOZE or CANCEL");
	while (!snoozed() && !cancelled()){
		beep(200, 250, 250);
	}
}

time_t alarms[5];

void setupAlarm(time_t alarm){
	alarms[0] = alarm;
	Alarm.alarmOnce(hour(alarm), minute(alarm), second(alarm), soundAlarm);
}

time_t getNextAlarm(){
	return alarms[0];
}

void setup(){
	Serial.begin(9600);
	lcd.begin(LCD_CHARS_PER_ROW, LCD_ROWS);
	pinMode(ALARM_PIN, OUTPUT);
	pinMode(CANCEL_PIN, INPUT_PULLUP);
	pinMode(SNOOZE_PIN, INPUT_PULLUP);
	
	display("Waiting on time\nsync from serial...");
	syncTime();
	
	display("Waiting on alarm\nfrom serial...");
	setupAlarm(getAlarmFromSerial());
}

void loop(){
	display("T: " + formatTime(now())+ "\nA: " + formatTime(getNextAlarm()));
	Alarm.delay(100);
}
