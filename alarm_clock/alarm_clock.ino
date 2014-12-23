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

String pad(int value){
	return value < 10 ? "0" + String(value) : String(value);
}

String formatTime(time_t t){
	return pad(hour(t)) + ":" + pad(minute(t)) + ":" + pad(second(t)); 
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

void syncTimeFromSerial(){
	time_t time;
	while (timeStatus() == timeNotSet){ 
		time = processTimeMessage(TIME_HEADER);
		if (time != NULL){
			setTime(time);
		}
		delay(100);
	}
}

time_t getAlarmFromSerial(){
	time_t alarm = NULL;
	while (alarm == NULL){
		alarm = processTimeMessage(ALARM_HEADER);
		delay(100);
	}
	return alarm;
}

void soundAlarm();

static int snooze_pin_state = 1;
static int cancel_pin_state = 1;

bool buttonReleased(int* previous_state, int pin){
	int new_state = digitalRead(pin);
	bool released = *previous_state == 1 && new_state == 0;
	*previous_state = new_state;
	return released;
}

bool snoozeButtonReleased(){
	return buttonReleased(&snooze_pin_state, SNOOZE_PIN);
}

bool cancelButtonReleased(){
	return buttonReleased(&cancel_pin_state, CANCEL_PIN);
}

bool snoozed(){
	bool snoozeReleased = snoozeButtonReleased();
	if (snoozeReleased){
		Alarm.timerOnce(SNOOZE_SECONDS, soundAlarm);
		display("Snoozed! Shh for:\n" + String(SNOOZE_SECONDS) + " seconds");
		delay(1500);
	}
	return snoozeReleased;
}

bool cancelled(){
	bool cancelReleased = cancelButtonReleased();
	if (cancelReleased){
		display("Cancelled!");
		delay(1000);
	}
	return cancelReleased;
}

void beep(int freq, int period, int silence){
	analogWrite(ALARM_PIN, freq); 
	delay(period);
	analogWrite(ALARM_PIN, 0);
	delay(silence);
}

void soundAlarm(){
	display("Beep! Beep!\nSNOOZE or CANCEL");
	delay(250);
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


time_t edit(time_t time, String msg){
	tmElements_t tm;
	breakTime(time,tm);
	
	uint8_t* current = &tm.Hour;
	int scale = 24;
	String timeS;
	
	int blink_period = 400;
	int blink_counter = 0;
	
	while (!cancelled()){
		if (snoozeButtonReleased()){
			//advance to next edit or finish
			if (current == &tm.Second) {
				break;
			}
			current = current == &tm.Minute ? &tm.Second : current;	
			current = current == &tm.Hour ? &tm.Minute : current;
			scale = 60;
		}
		
		*current = int(float(1024-analogRead(A5))/1024*scale);
		timeS = formatTime(makeTime(tm));
	
		if (blink_counter >= blink_period/2){
			if (current == &tm.Hour){
				timeS[0] = ' ';
				timeS[1] = ' ';
			}
			if (current == &tm.Minute){
				timeS[3] = ' ';
				timeS[4] = ' ';
			}
			if (current == &tm.Second){
				timeS[6] = ' ';
				timeS[7] = ' ';
			}
		}
		blink_counter = (blink_counter + 50) % blink_period;
		
		display(msg + ":\n" + timeS);
		delay(50);
	}
	return makeTime(tm);
}

int getMode(){
	return !snoozeButtonReleased()*2 + !cancelButtonReleased();
}


void setup(){
	Serial.begin(9600);
	lcd.begin(LCD_CHARS_PER_ROW, LCD_ROWS);
	pinMode(ALARM_PIN, OUTPUT);
	pinMode(CANCEL_PIN, INPUT_PULLUP);
	pinMode(SNOOZE_PIN, INPUT_PULLUP);
	setupAlarm(now()+10); 
}

void loop(){
	switch(getMode()){
		case 3: display("T: " + formatTime(now())+ "\nA: " + formatTime(getNextAlarm())); 
				break;
		
		case 2: setupAlarm(edit(getNextAlarm(), "Change alarm")); 
				break;
		
		case 1: setTime(edit(now(), "Change time")); 
				break;
		
		case 0: display("Waiting on time\nsync from serial...");
				syncTimeFromSerial();
				display("Waiting on alarm\nfrom serial...");
				setupAlarm(getAlarmFromSerial());
	}
	Alarm.delay(100);
}
