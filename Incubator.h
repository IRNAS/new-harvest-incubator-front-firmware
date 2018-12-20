#pragma region README
/* INCUBATOR CLASS
CONSTRUCTION: -> constructed with the Incubator() constructor. Initialise variables, clases and pinouts.
INITIALISATION: 
	* Init() method needs to be caled before operation, with display address passed as argument.
	* Two logfiles, one for data logging and one for error logging are initialised.
	* Stored data is read, using preferences class.
	* Screen is initialised.
	* setupWiFi() function is called -> tries to preform autoconnect with stored data, if it fails, or first-time login is 
	performed, it guides user trough wifi setup.
	* Time is configured.
OPERATION: 
	* Each loop, switchState() function is called from the main sketch. Based on the incubator state, operation is controlled. 
	* INITIAL:
		- On restart operation starts in innitial state. Proceed to OPERATION.
	* OPERATION: main state.
		- call switchButtons() to get user input trough buttons. It will adjust the screen and screenState and reset screen timer,
		to retur to main screen. It will also adjust set temperature and co2 level if applicable. 
		- call updateScreen() to update data on the screen. Based on the screen state:
			* MAIN: update current temperature and co2 % if changed. 
			* SET_TEMP/SET_CO2 if timer is more than 
		- call regulate() to regulate main enviromental parameters. 
	* STANDBY: do nothing
	* OFF: do nothing
	* ALERT: handle major buggs.
DATA LOGGING: every 10 s, call logData() function that logs time, sensor and heater temperature, set temperature, sensor CO2 value,
and set CO2 value. 
	


*/

#pragma endregion

#ifndef Incubator_h
#define Incubator_h

#include <WiFi.h>     
#include <DNSServer.h>
#include <ESP32WebServer.h>
#include "WiFiManager.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_SHT31.h"
#include <DallasTemperature.h>
#include <OneWire.h>
#include "AutoPID.h"
#include <HardwareSerial.h>
#include "FS.h"
#include "SD.h"
#include <time.h>
#include <stdarg.h>


//Define error logging level
#define DEBUG 5
#pragma region Logging functions

//Alert
#if (DEBUG >= 0)
	#define ALERT_PRINTLN(x){ Serial.println (x); errorfile.println (x);}
	#define ALERT_PRINT(x){ Serial.print (x); errorfile.print (x);}
#else
	#define ALERT_PRINTLN(x)
	#define ALERT_PRINT(x)
#endif

//Critical
#if (DEBUG >= 1)
	#define CRIT_PRINTLN(x){ Serial.println (x); errorfile.println (x);}
	#define CRIT_PRINT(x){ Serial.print (x); errorfile.print (x);}
#else
	#define CRIT_PRINTLN(x)
	#define CRIT_PRINT(x)
#endif

//Error
#if (DEBUG >= 2)
	#define ERR_PRINTLN(x){ Serial.println (x); errorfile.println (x);}
	#define ERR_PRINT(x){ Serial.print (x); errorfile.print (x);}
#else
	#define ERR_PRINTLN(x)
	#define ERR_PRINT(x)
#endif

//Warning
#if (DEBUG >= 3)
	#define WARN_PRINTLN(x){ Serial.println (x); errorfile.println (x);}
	#define WARN_PRINT(x){ Serial.print (x); errorfile.print (x);}
#else
	#define WARN_PRINTLN(x)
	#define WARN_PRINT(x)
#endif

//Informational
#if (DEBUG >= 4)
	#define INFO_PRINTLN(x){ Serial.println (x); errorfile.println (x);}
	#define INFO_PRINT(x){ Serial.print (x); errorfile.print (x);}
#else
	#define INFO_PRINTLN(x)
	#define INFO_PRINT(x)
#endif

//Debug
#if (DEBUG >= 5)
	#define DEBUG_PRINTLN(x){ Serial.println (x); errorfile.println (x);}
	#define DEBUG_PRINT(x){ Serial.print (x); errorfile.print (x);}
#else
	#define DEBUG_PRINTLN(x)
	#define DEBUG_PRINT(x)
#endif

#pragma endregion

//LED screen parameters
#define TEXT_OFFSET 4
#define OLED_RESET 4
#define HEIGHT_SMALL 9
#define HEIGHT_LARGE 20
#define HEIGHT_IP 10
#define HEIGHT_TEXT 14
#define HEIGHT_TEMP 30
#define HEIGHT_SET 27
#define HEIGHT_BUTTONS 54
#define VERTICAL_OFFSET 3
#define HORIZONTAL_OFFSET 6
#define SCREEN_TIMER 5000 //Timer for screen update in millis

//Innitial temperature and co2 values, tresholds
#define INNITIAL_T 37.0
#define T_MAX 50.0
#define T_MIN 25.0
#define INNITIAL_CO2 5.00
#define CO2_MAX 20.0
#define CO2_MIN 0.04
#define INNITIAL_O2 0.00
#define T_STEP 0.1
#define CO2_STEP 0.01
#define T_THRESHOLD 0.1
#define T_HEATER_THRESHOLD 1
#define CO2_THRESHOLD 0.1
#define HEATER_MAX_T 80 //Max heater temperature
#define HEATER_ERROR_CHECK 30000 //Time period for heater error check
#define CO2_ERROR_CHECK 15000 //Time period for heater error check

//Errors
#define ERROR_NUMBER_LIMIT 10 //Max number of invalid errors

//Pins
#define BUTTON_1 12
#define BUTTON_2 27
#define BUTTON_3 32
#define BUTTON_4 33
#define OUTPUT_PIN_HEATER 25
#define OUTPUT_PIN_CO2 26
#define TEMP_PROBE_PIN 14

//Sensors
#define TEMP_READ_DELAY 800 //temp probe sensor can only read digital every ~750 ms
#define TEMP_PRECISION 12 //read precision of temp probe sensor, 12b max

//PID
#define KP 1000
#define KI 3
#define KD 1
#define KP_CO2 5000
#define KI_CO2 5
#define KD_CO2 1
#define HEATER_INTERVAL_MIN 0
#define HEATER_INTERVAL_MAX 5000
#define HEATER_INTERVAL_INNITIAL 1000
#define HEATER_BANG 5
#define PID_TIMESTEP 800
#define CO2_INTERVAL_MIN 0
#define CO2_INTERVAL_MAX 1000
#define CO2_INTERVAL_INNITIAL 30000
#define CO2_BANG 0.5

enum ScreenState{  
	SETUP,
	MAIN,
	SET_TEMP,
	SET_CO2
};

enum IncubatorState {
	INITIAL,
	OPERATION,
	STANDBY,
	OFF,
	ALERT
};

enum ButtonState {
	B_UP,
	B_DOWN,
	B_LIGHT_ON,
	B_LIGHT_OFF,
	B_MAIN,
	B_SET_TEMP,
	B_SET_CO2,
	B_NULL
};

class Button {
public:

	ButtonState buttonState;
	const char* buttonName;
	int Pin;

	Button();
	void Init(ButtonState, int);
	void OnClick();

private:
	void SetName();

};

class Errors {
public:
	
	int temp_sensor_1;
	int temp_sensor_2;
	int heater_sensor;
	int co2_sensor;
	int temp;
	int alert;
	int overheat;

	Errors();
	void Init();

private:
};

class Incubator{
public:
	
	Incubator(); //Constructor
	
	void Init(Adafruit_SSD1306*); //Initialisation function
	void switchState(); //Switch operation state

	//SD card logging
	File logfile; //Log file
	File errorfile; //Error log file
	char filename[30]; //Name of the log file
	char error_filename[30]; //Name of the error file
	struct tm tmstruct; //Time structure
	int log_time;

	void LOG(int level, const char *text, ...); //Log function

	//ERRORS
	Errors ERRORS;

	//Get variables
	String get_incubator_name();
	float get_T_heater_set();
	float get_T_set();
	float get_T_1();
	float get_T_2();
	float get_T_heater();
	float get_T_heater_gradient();
	float get_T_avg();
	float get_heater_interval();
	float get_heaterON();
	bool get_heater();
	float get_CO2_set();
	float get_CO2();
	float get_CO2_avg();
	float get_co2_valve_interval();
	bool get_co2_valve();

	//Set variables
	void set_T(float);
	void set_co2(float);
	void set_incubator_name(String);
	void set_temp_notifications(int);
	void set_co2_notifications(int);
	void set_door_notifications(int);
	void set_mail_notifications(int);
	void set_push_notifications(int);
	void set_system_notifications(int);
	void set_mail(String);
	void turn_off_heater();

	//Settings
	void reset_WiFi(); //Re-set wifi

	//Variables for Blynk notifications
	bool send_mail;
	bool send_push;
	bool temp_notifications;
	bool co2_notifications;
	bool door_notifications;
	bool system_notifications;
	String mail;

	//WEB interface pages
	String getPage();
	String getSettings();
	String getReset();

	
private:

	IncubatorState incubatorState;
	ScreenState screenState;
	int screenStateTimer; 

	Preferences preferences;
	String incubator_name = "IncubatorName";

	//BUTTONS
	Button button_1;
	Button button_2;
	Button button_3;
	Button button_4;

	ButtonState getButtons();
	void switchButtons();

	//TEMPERATURE
	float T_set; //Desired incubator temperature
	float T_heater_set; //Set heater temperature
	
	//Temperature sensors inside incubator
	Adafruit_SHT31 temp_sensor_1;
	Adafruit_SHT31 temp_sensor_2;

	//Temperature sensor on the heater
	OneWire oneWire; //OneWire object
	DallasTemperature heater_sensor; //Heater sensor object
	DeviceAddress heater_sensor_address; //Heater sensor address
	int last_heater_sensor_update; //Last update time
	bool T_regulation; //Temperature regulation on/off

	bool temp_sensor_connected_1; //temp sensor 1
	bool temp_sensor_connected_2; //temp sensor 2
	bool temp_sensor_connected_3; //heater sensor
	
	float T_1; //Temperature sensor 1
	float T_2; //Temperature sensor 2
	float T_heater; //Temperature sensor heater
	float T_heater_old; //Temperature sensor heater at the start of the heating interval
	float T_heater_gradient;
	float gradient_interval;
	float T_avg; //Average temperature inside incubator
	float T_max; //Max sensor temperature inside incubator
	float T_max_old;
	float T_avg_old; 
	float T_avg_gradient;

	//Heater
	int last_heater_update; //Last update time of heater
	int last_heater_error; //Last error update time of heater
	float heaterON; //Heater on interval, regulated by PID
	int heater_interval; //ON/OFF interval of heater
	bool heater; //Heater staus, ON/OFF

	//CO2
	HardwareSerial *co2_sensor;
	bool CO2_regulation; //CO2 regulation on/off
	float CO2_set; //Set CO2 level
	float CO2_old; //Old CO2 level
	bool co2_sensor_connected;
	float CO2; //CO2 level sensor
	float CO2_avg; //Average co2 level
	float CO2_moving; //Moving average
	int CO2_count;

	//CO2 valve
	int last_co2_update; //Last update of the co2 valve
	int last_co2_error; //Last error update time of heater
	float co2_valveON; //co2 interval, regulated by PID
	float co2_valve_interval; //ON/OFF interval of the co2 valve
	bool co2_valve; //CO2 valve status, ON/OFF

	//O2
	float O2_set; //Set O2 level
	bool o2_sensor_connected;
	float O2;  //O2 leveld

	//PID
	AutoPID *temperaturePID;
	AutoPID *co2PID;
	
	//Methods
	void setupSensors(); //Initialise sensors
	void updateScreen(); //Change display values
	bool updateTemperature(); //Update temperature readings
	bool updateCO2(); //Update CO2 readings
	bool avgTemperature(); //Calculate average temperature inside the incubator
	float updateValue(float, float, float); //Increase/decrease value
	void regulate(); //Regulate incubator
	void regulateHeater(); //Regulate incubator temperature
	void setHeaterTemperature(); //Set heater temperature
	void calculateTemperatureGradient(); //Calculate temperature gradient of heater and incubator
	void regulateCO2(); //Regulate incubator co2 levels
	bool checkHeaterERROR(); //Check if heater is not working
	bool checkCO2ERROR(); //Check if heater is not working
	void logData(); //Log data to SD card
	void handleAlert(); //Handle alert state
	
	//WiFi
	WiFiManager *wifiManager;
	const char* localSSID = "Incubator";
	const char* localPassword = "incubator";
	IPAddress deviceIP;
	bool WiFi_connected;

	bool setupWiFi();

	//Oled
	Adafruit_SSD1306 *oled;
	
	//Print functions
	void printScreenInitial();
	void printScreenConnected();
	void printScreenSetConnection();
	void printScreenOffline();
	void printScreenSensorSetup();
	void printScreenMain(IPAddress, float, float);
	void printScreenIP(IPAddress);
	void printScreenButtonLeft();
	void printScreenButtonRight();
	void printScreenTemperature(float);
	void printScreenCO2(float);
	void printScreenSet(IPAddress);
	void printScreenSetValue(float, int);

};

#endif