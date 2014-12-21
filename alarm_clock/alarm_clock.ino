#include <Time.h>  

#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 

#include <LiquidCrystal.h>
//LiquidCrystal(rs, enable, d4, d5, d6, d7) 
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void processSyncMessage(){
	// if time sync available from the serial port
	// update time and return true
	while(Serial.available() >=  TIME_MSG_LEN ){  // time message consists of header & 10 ASCII digits
		char c = Serial.read() ; 
		Serial.print(c);  
		if( c == TIME_HEADER ) {       
			time_t pctime = 0;
			for(int i=0; i < TIME_MSG_LEN -1; i++){   
				c = Serial.read();          
				if( c >= '0' && c <= '9'){   
				pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
				}
			}   
		setTime(pctime);   // Sync Arduino clock to the time received on the serial port
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

void setup(){
	Serial.begin(9600);
	lcd.begin(16, 2);
	
	lcd.print("Waiting on time");
	lcd.setCursor(0,1);
	lcd.print("sync from serial");
	syncTime();
}

void loop(){
	lcd.clear();
	lcd.print("Time is:" + getTime());
	delay(100);
	//lcd.setCursor(0, 1);
	//lcd.print(millis()/1000);
	
}
