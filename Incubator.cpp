#include "Incubator.h"

//ERRORS
Errors::Errors(){}

void Errors::Init() {
	temp_sensor_1 = 0;
	temp_sensor_2 = 0;
	heater_sensor = 0;
	co2_sensor = 0;
	temp = 0;
	alert = 0;
	overheat = 0;
}

//BUTTONS

Button::Button() {

}

#pragma region void Button::Init(ButtonState state, int pin) 
/* Initialise Button class
Input: 
* ButtonState state - initial button state
* int pin - pin number of button
Output: /
Description:
* Set pin number and button state.
* Set display name with SetName().
*/
void Button::Init(ButtonState state, int pin) {
	
	Pin = pin;
	pinMode(pin, INPUT);
	buttonState = state;
	SetName();
}
#pragma endregion

#pragma region void Button::OnClick()
/* Define on-click change based on button state
Input: /
Output: /
Description:
* Based on current state, change button state when pressed.
* Adjust display name of a button.
*/
void Button::OnClick() {
	
	switch (buttonState)
	{
	case ButtonState::B_UP:
	{
		break;
	}
	case ButtonState::B_DOWN:
	{
		break;
	}
	case ButtonState::B_MAIN:
	{
		buttonState = ButtonState::B_SET_TEMP;
		break;
	}
	case ButtonState::B_SET_TEMP:
	{
		buttonState = ButtonState::B_SET_CO2;
		break;
	}
	case ButtonState::B_SET_CO2:
	{
		buttonState = ButtonState::B_MAIN;
		break;
	}
	case ButtonState::B_LIGHT_ON:
	{
		buttonState = ButtonState::B_LIGHT_OFF;
		break;
	}
	case ButtonState::B_LIGHT_OFF:
	{
		buttonState = ButtonState::B_LIGHT_ON;
		break;
	}
	}

	SetName();
}
#pragma endregion

#pragma region void Button::SetName()
/* Define button display name
Input: / 
Output: /
Description:
* Define button display name based on the state.
*/
void Button::SetName() {
	switch (buttonState)
	{
	case ButtonState::B_UP:
	{
		buttonName = "UP";
		break;
	}
	case ButtonState::B_DOWN:
	{
		buttonName = "DOWN";
		break;
	}
	case ButtonState::B_MAIN:
	{
		buttonName = "MAIN";
		break;
	}
	case ButtonState::B_SET_TEMP:
	{
		buttonName = "SET TEMP";
		break;
	}
	case ButtonState::B_SET_CO2:
	{
		buttonName = "SET CO2";
		break;
	}
	case ButtonState::B_LIGHT_ON:
	{
		buttonName = "LIGHT ON";
		break;
	}
	case ButtonState::B_LIGHT_OFF:
	{
		buttonName = "LIGHT OFF";
		break;
	}
	}
}
#pragma endregion

//INCUBATOR

Incubator::Incubator():oneWire(TEMP_PROBE_PIN) {

	incubatorState = IncubatorState::INITIAL;
	screenState = ScreenState::SETUP;
	T_regulation = true;
	CO2_regulation = true;
	temp_sensor_connected_1 = false;
	temp_sensor_connected_2 = false;
	temp_sensor_connected_3 = false;
	co2_sensor_connected = false;
	send_mail = true;
	send_push = true;
	temp_notifications = true;
	co2_notifications = true;
	door_notifications = true;
	system_notifications = true;
	last_heater_sensor_update = millis();
	last_heater_update = millis();
	last_heater_error = millis();
	last_co2_update = millis();
	last_co2_error = millis();
	screenStateTimer = millis();
	log_time = millis();
	mail = "";

	T_set = INNITIAL_T;
	T_heater_set = INNITIAL_T;
	T_max = INNITIAL_T;
	T_avg = INNITIAL_T;
	T_max_old = INNITIAL_T;
	T_avg_old = INNITIAL_T;
	T_avg_gradient = 0.0;
	T_1 = INNITIAL_T;
	T_2 = INNITIAL_T;
	T_heater = INNITIAL_T;
	T_heater_old = INNITIAL_T;
	T_heater_gradient = 0.0;
	gradient_interval = 0.0;
	CO2_set = INNITIAL_CO2;
	CO2_old = INNITIAL_CO2;
	CO2 = INNITIAL_CO2;
	CO2_avg = INNITIAL_CO2;
	CO2_moving = 0.0;
	CO2_count = 0;
	O2_set = INNITIAL_O2;
	O2 = INNITIAL_O2;

	heater_interval = HEATER_INTERVAL_INNITIAL;
	heaterON = HEATER_INTERVAL_INNITIAL;
	co2_valve_interval = CO2_INTERVAL_INNITIAL;
	co2_valveON = CO2_INTERVAL_INNITIAL;
	co2_sensor = new HardwareSerial(2);

	//temperaturePID = new AutoPID(&T_heater, &T_heater_set, &heaterON, HEATER_INTERVAL_MIN, HEATER_INTERVAL_MAX, KP, KI, KD);
	//co2PID = new AutoPID(&CO2, &CO2_set, &co2_valveON, CO2_INTERVAL_MIN, CO2_INTERVAL_MAX, KP, KI, KD);

	wifiManager = new WiFiManager();

	pinMode(OUTPUT_PIN_HEATER, OUTPUT);
	pinMode(OUTPUT_PIN_CO2, OUTPUT);
}

#pragma region void Incubator::Init(Adafruit_SSD1306* Oled)
/*Initialise Incubator class
Input:
	Adafruit_SSD1306* Oled - LED screen instance
Output:
Description:
* Define buttons.
* Initialise variables.
* Initialise LED screen
* Initialise WiFi connection.
* Sensors setup.
* Display main screen.
* Set up PID controllers.
*/
void Incubator::Init(Adafruit_SSD1306* Oled){
	
	//Initalise error states
	ERRORS.Init();

	sprintf(error_filename, "/errorLog.txt");
	sprintf(filename, "/Log.txt");

	//Start SD card and log file
	if (!SD.begin()) {
		LOG(2, "SD card Mount Failed!");
	}
	logfile = SD.open(filename);
	errorfile = SD.open(error_filename);
	if (!logfile) {
		LOG(2, "Log file does not exist, create new one.");
		logfile = SD.open(filename, FILE_WRITE);
		if (!logfile) {
			LOG(2, "Couldnt create data log file.");
		}
		if (logfile.print("TIME,")) {
			logfile.print("TEMP_S1,");
			logfile.print("TEMP_S2,");
			logfile.print("TEMP_HEATER,");
			logfile.print("TEMP_SET,");
			logfile.print("CO2_S1,");
			logfile.println("CO2_SET");
			LOG(4, "Data log file written.");
		}
		else {
			LOG(2, "Data log file write failed!");
		}
	}
	if (!errorfile) {
		LOG(2, "Log file does not exist, create new one.");
		errorfile = SD.open(error_filename, FILE_WRITE);
		if (!errorfile) {
			LOG(2, "Couldnt create error log file.");
		}
		if (errorfile.println("Start error log.")) {
			LOG(4, "Error log file written.");
		}
		else {
			LOG(2, "Error log file write failed!");
		}
	}
	
	logfile.close();
	errorfile.close();
	errorfile = SD.open(error_filename, FILE_APPEND);
	LOG(2, "Error file opened for appending!");
	
	//buttons
	button_1.Init(ButtonState::B_SET_TEMP, BUTTON_1);
	button_2.Init(ButtonState::B_LIGHT_ON, BUTTON_2);
	button_3.Init(ButtonState::B_UP, BUTTON_3);
	button_4.Init(ButtonState::B_DOWN, BUTTON_4);

	LOG(4,"Buttons initialised!");
	
	//variables
	incubatorState = IncubatorState::INITIAL;
	screenState = ScreenState::SETUP;
	temp_sensor_connected_1 = false;
	temp_sensor_connected_2 = false;
	temp_sensor_connected_3 = false;
	co2_sensor_connected = false;
	o2_sensor_connected = false;
	last_heater_sensor_update = millis();
	last_heater_update = millis();
	last_co2_update = millis();
	log_time = millis();

	//Saved values
	//Start preferences
	preferences.begin("incubator", false);
	T_set = preferences.getFloat("setT", INNITIAL_T);
	CO2_set = preferences.getFloat("setCO2", INNITIAL_CO2);
	T_regulation = preferences.getBool("Tregulation", true);
	CO2_regulation = preferences.getBool("CO2regulation", true);
	incubator_name = preferences.getString("incubatorName", "IncubatorName");
	send_mail = preferences.getBool("sendMail", true);
	send_push = preferences.getBool("sendPush", true);
	temp_notifications = preferences.getBool("sendTemp", true);
	co2_notifications = preferences.getBool("sendCO2", true);
	door_notifications = preferences.getBool("sendDoor", true);
	system_notifications = preferences.getBool("sendSystem", true);
	mail = preferences.getString("mail", "");
	preferences.end();

	//setup screen
	oled = Oled;
	oled->begin(SSD1306_SWITCHCAPVCC, 0x3c);  // initialize with the I2C addr 0x3D (for the 128x64)
	printScreenInitial();

	LOG(4, "Screen initialised!");
	LOG(4, "Saved settings:");
	LOG(4, "Set T: %.2f", T_set);
	LOG(4, "Set CO2: %.2f", CO2_set);
	LOG(4, "Incubator name: %s", incubator_name.c_str());
	LOG(4, "Mail notifications: %d", send_mail);
	LOG(4, "Mail: %s", mail.c_str());
	LOG(4, "Push notifications: %d", send_push);
	LOG(4, "Temp notifications: %d", temp_notifications);
	LOG(4, "CO2 notifications: %d", co2_notifications);
	LOG(4, "Door notifications: %d", door_notifications);
	
	//setup WiFi
	WiFi_connected = setupWiFi();

	//Setup clock and logfile
	configTime(0, 0, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");
	delay(2000);
	tmstruct.tm_year = 0;
	getLocalTime(&tmstruct, 5000);
	if (tmstruct.tm_year) {
		LOG(4, "Time synchronised: %d-%02d-%02d %02d:%02d:%02d", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
	}
	else {
		LOG(2, "Time synchronisation failed!");
	}

	//setup temperature sensors and make initial reading
	setupSensors();
  
	//Main screen
	printScreenMain(deviceIP, T_avg, CO2_avg);

	//regulation temperature PID
	//temperaturePID->setBangBang(HEATER_BANG, 0);
	//temperaturePID->setTimeStep(PID_TIMESTEP);
	//co2PID->setBangBang(CO2_BANG, 0);
	//co2PID->setTimeStep(PID_TIMESTEP);
	last_heater_error = millis();
	last_co2_error = millis();
	screenStateTimer = millis();
	gradient_interval = 0.0;
}
#pragma endregion

#pragma region void Incubator::switchState()
/* Switch between different incubator states and adjust response
Input: /
Output: /
Description:
* INNITIAL: 
	- go to OPERATION
* OPERATION: normal incubator operation - regulate temperature and co2 levels, collect clicked buttons.
	- Switch buttons:
		* B_MAIN: display main screen
		* B_SET_TEMP: display set temperature screen
		* B_SET_CO2: display set co2 screen
		* B_UP: if in SET_TEMP/SET_CO2 screen, adjust set temp/co2 level, otherwise pass.
		* B_DOWN: if in SET_TEMP/SET_CO2 screen, adjust set temp/co2 level, otherwise pass.
	- Update sensor readings
	- Regulate incubator
* STANDBY
* OFF
* Check if 
*/
void Incubator::switchState() {

	switch (incubatorState)
	{
	case IncubatorState::INITIAL:
	{
		//Setup incubator
		LOG(5, "STATE - Innitial");
		incubatorState = IncubatorState::OPERATION;
		break;
	}
	case IncubatorState::OPERATION:
	{
		LOG(5, "STATE - Operation");

		//Get buttons input and change screen
		switchButtons();

		//Update sensor readings and screen
		updateScreen();

		//Regulate
		regulate();

		break;
	}
	case IncubatorState::STANDBY:
	{
		LOG(5, "STATE - Standby");
		break;
	}
	case IncubatorState::OFF:
	{
		LOG(5, "STATE - Off");
		break;
	}
	case IncubatorState::ALERT:
	{
		LOG(0, "STATE - ALERT!");
		LOG(0, "Closing errorlog file!");
		LOG(0, "Restarting ESP!");
		errorfile.close();
		handleAlert();
		ERRORS.alert = 1;
		break;
	}
	}

	//Write to SD card
	if (millis() - log_time > 10000)
	{
		logData();
		log_time = millis();
	}
}
#pragma endregion

void Incubator::switchButtons()
{
	//Get buttons
	switch (getButtons())
	{
	case ButtonState::B_MAIN:
	{
		if (screenState != ScreenState::MAIN) {
			LOG(5, "Button SET pressed. Change to main screen.");
			printScreenMain(deviceIP, T_avg, CO2_avg);
			screenStateTimer = millis();
		}
		break;
	}
	case ButtonState::B_SET_TEMP:
	{
		if (screenState != ScreenState::SET_TEMP)
		{
			LOG(5, "Button SET pressed. Change to set temperature screen.");
			screenState = ScreenState::SET_TEMP;
			printScreenSet(deviceIP);
			screenStateTimer = millis();
		}
		return;
	}
	case ButtonState::B_SET_CO2:
	{
		if (screenState != ScreenState::SET_CO2)
		{
			LOG(5, "Button SET pressed. Change to set co2 screen.");
			screenState = ScreenState::SET_CO2;
			printScreenSet(deviceIP);
			screenStateTimer = millis();
		}
		return;
	}
	case ButtonState::B_UP:
	{
		if (screenState == ScreenState::SET_TEMP) {
			LOG(5, "Button UP pressed. Increase set temperature.");
			T_set = updateValue(T_set, T_STEP, 1.0);
			if (T_set > T_MAX)
			{
				T_set = T_MAX;
			}
			preferences.begin("incubator", false);
			preferences.putFloat("setT", T_set);
			preferences.end();
			printScreenSetValue(T_set, 1);
			screenStateTimer = millis();
		}
		else if (screenState == ScreenState::SET_CO2) {
			LOG(5, "Button UP pressed. Increase set co2.");
			CO2_set = updateValue(CO2_set, T_STEP, 1.0);
			if (CO2_set > CO2_MAX)
			{
				CO2_set = CO2_MAX;
			}
			//Start preferences
			preferences.begin("incubator", false);
			preferences.putFloat("setCO2", CO2_set);
			preferences.end();
			printScreenSetValue(CO2_set, 2);
			screenStateTimer = millis();
		}
		return;
	}
	case ButtonState::B_DOWN:
	{
		if (screenState == ScreenState::SET_TEMP) {
			LOG(5, "Button DOWN pressed. Decrease set temperature.");
			T_set = updateValue(T_set, T_STEP, -1.0);
			if (T_set < T_MIN)
			{
				T_set = T_MIN;
			}
			preferences.begin("incubator", false);
			preferences.putFloat("setT", T_set);
			preferences.end();
			printScreenSetValue(T_set, 1);
			screenStateTimer = millis();
		}
		else if (screenState == ScreenState::SET_CO2) {
			LOG(5, "Button DOWN pressed. Decrease set co2.");
			CO2_set = updateValue(CO2_set, T_STEP, -1.0);
			if (CO2_set < CO2_MIN)
			{
				CO2_set = CO2_MIN;
			}
			preferences.begin("incubator", false);
			preferences.putFloat("setCO2", CO2_set);
			preferences.end();
			printScreenSetValue(CO2_set, 2);
			screenStateTimer = millis();
		}
		return;
	}
	}
}

#pragma region ButtonState Incubator::getButtons()
/*Check if any button is pressed
Input: /
Output: ButtonState - state which was called
Description:
* Collect button input and evoke new state. 
*/
ButtonState Incubator::getButtons() {
	
	ButtonState tmpState = ButtonState::B_NULL;
	
	//Check if any button is pressed
	int tmp = digitalRead(button_1.Pin);
	if (tmp) {
		LOG(5, "Pressed nr. 1 button: %s", button_1.buttonName);

		tmpState = button_1.buttonState;
		button_1.OnClick();
	}
	tmp = digitalRead(button_2.Pin);
	if (tmp) {
		LOG(5, "Pressed nr. 2 button: %s", button_2.buttonName);

		tmpState = button_2.buttonState;
		button_2.OnClick();
	}
	tmp = digitalRead(button_3.Pin);
	if (tmp) {
		LOG(5, "Pressed nr. 3 button: %s", button_3.buttonName);

		tmpState = button_3.buttonState;
		button_3.OnClick();
	}
	tmp = digitalRead(button_4.Pin);
	if (tmp) {
		LOG(5, "Pressed nr. 4 button: %s", button_4.buttonName);

		tmpState = button_4.buttonState;
		button_4.OnClick();
	}

	return tmpState;
}
#pragma endregion

#pragma region bool Incubator::setupWiFi()
/* Setup WiFi connection
Input: /
Output: bool connected - true/false if connected. 
Description:
* set up WiFi manager.
* if possible connect to wifi, otherwise guide user trough wifi setup. 
*/
bool Incubator::setupWiFi(){
  
	LOG(4, "Begin WiFi setup.");
	bool connected = false;
	//WiFiManager wifiManager;
	wifiManager->setDebugOutput(false);
	//reset settings - for testing
	//wifiManager.resetSettings();
	//set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
	//wifiManager.setAPCallback(configModeCallback);

	int autoCon = wifiManager->autoConnect(localSSID, localPassword);
	if(autoCon == 0) {
		deviceIP = WiFi.softAPIP();
		if(wifiManager->printOffline()){
			LOG(4, "Begin offline operation.");
		printScreenOffline();
		}
    else{
		LOG(3, "Failed to connect and hit timeout. Reseting...");
		//reset and try again, or maybe put it to deep sleep
		ESP.restart();
		delay(1000);
    }
	}
	else if(autoCon == 1){
    //if you get here you have connected to the WiFi
		LOG(4, "Connected to WiFi!");
		deviceIP = WiFi.localIP();
		printScreenConnected();
		connected = true;
	}
	else{
	  // Enter setup mode
	  LOG(4, "Enter connection setup mode.");
	  printScreenSetConnection();
	  if(!wifiManager->startConfigPortal(localSSID, localPassword)){
		deviceIP = WiFi.softAPIP();
		if(wifiManager->printOffline()){
		  printScreenOffline();
		}
		else {
			LOG(3, "Failed to connect and hit timeout. Reseting...");
			//reset and try again, or maybe put it to deep sleep
			ESP.restart();
			delay(1000);
		} 
		}
		else {
			//if you get here you have connected to the WiFi
			LOG(4, "Connected to WiFi!");
			deviceIP = WiFi.localIP();
			printScreenConnected();
			connected = true;  
		}
		LOG(4, "Restarting ESP!");
		ESP.restart();
		delay(1000);
	}
	LOG(4, "Connected to: %s", WiFi.SSID().c_str());
	LOG(4, "Device IP: %s", deviceIP.toString().c_str());

	return connected;
}
#pragma endregion

#pragma region void Incubator::reset_WiFi()
/* Reset connection settings
Input: /
Output: /
Description: 
	* Stop temperature and co2 regulation.
	* Reset settings in WiFi manager.
	* Restart ESP.
*/
void Incubator::reset_WiFi(){

	LOG(4, "Reseting WiFi settings and ESP!");
	LOG(4, "Terminate temperature regulation!");
	LOG(4, "Terminate co2 regulation!");
	//turn off regulation
	digitalWrite(OUTPUT_PIN_HEATER, false);
	digitalWrite(OUTPUT_PIN_CO2, false);
	//reset settings - for testing
	wifiManager->resetSettings();
	ESP.restart();
	delay(1000);
}
#pragma endregion

#pragma region void Incubator::updateScreen()
/* Update screen values and states
Input: /
Output: /
Description:
	* If on main screen check if temperature needs to be updated with updateTemperature(). If yes, print T_avg. 
	* If on main screen check if co2 needs to be updated with updateCO2(). If yes, print CO2_avg.
	* If on the set_temp/set_co2 screen, check for inactive timout and return to main screen. 
*/
void Incubator::updateScreen()
{
	switch (screenState)
	{
	case ScreenState::MAIN:
	{
		//Update sensor values;
		if (updateTemperature())
		{
			printScreenTemperature(T_avg);
		}
		if (updateCO2())
		{
			printScreenCO2(CO2_avg);
		}
		break;
	}
	case ScreenState::SET_TEMP:
	case ScreenState::SET_CO2:
	{
		//Check if non-active screen form more than 5s
		if (millis() - screenStateTimer > SCREEN_TIMER)
		{
			LOG(5, "Inactive screen. Change to main.");
			printScreenMain(deviceIP, T_avg, CO2_avg);
			screenStateTimer = millis();
			button_1.buttonState = ButtonState::B_SET_TEMP;
		}
		break;
	}
	}
}
#pragma endregion

#pragma region void Incubator::setupSensors()
/* Setup temperature and co2 sensors
Input: / 
Output: /
Description:
	* Setup both temperature sensores. If connected set temp_sensor_connected_1/2 to true.
	* Setup heater temperature probe. If connected set temp_sensor_connected_3 to true.
	* Setup ms4 sensor.  If connected set co2_sensor_connected to true. 
	* Make temperature and co2 readings. 
*/
void Incubator::setupSensors(){

	//Sensor 1
	temp_sensor_1 = Adafruit_SHT31();
	if (!temp_sensor_1.begin(0x45)) {   // Set to 0x45 for alternate i2c addr
		LOG(1, "Couldn't find SHT31 sensor 1!");
		while (1) delay(1);
	}
	else
	{
		temp_sensor_connected_1 = true;
		float t = temp_sensor_1.readTemperature();
		LOG(4, "SHT31 sensor 1 setup done. First reading: %.2f C.", t);
	}
	//Sensor 2
	temp_sensor_2 = Adafruit_SHT31();
	if (!temp_sensor_2.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
		LOG(1, "Couldn't find SHT31 sensor 2!");
		while (1) delay(1);
	}
	else
	{
		temp_sensor_connected_2 = true;
		float t = temp_sensor_2.readTemperature();
		LOG(4, "SHT31 sensor 2 setup done. First reading: %.1f",t);
	}

	//Heater temperature sensor
	heater_sensor = DallasTemperature(&oneWire);
	heater_sensor.begin();
	if (heater_sensor.getAddress(heater_sensor_address, 0)) {
		temp_sensor_connected_3 = true;
		heater_sensor.setResolution(heater_sensor_address, TEMP_PRECISION);
		LOG(4, "Heater temperature sensor connected.");
	}
	else
	{
		LOG(0, "Couldn't connect heater temperature sensor!");
	}

	//CO2 sensor

	co2_sensor->begin(9600, SERIAL_8N1, 16, 17);
	delay(500);
	if (co2_sensor->available() > 0)
	{
		co2_sensor_connected = true;
		LOG(4, "CO2 sensor connected.");
	}
	else
	{
		LOG(0, "Couldn't connect co2 sensor!");
	}
	
	printScreenSensorSetup();
	updateTemperature();
	T_heater_old = T_heater;
	updateCO2();
	delay(2000);
}
#pragma endregion

#pragma region void Incubator::regulate()
/* Regulate incubator parameters
Input: /
Output: /
Description:
	* Regulate temperature.
	* Regulate CO2
*/
void Incubator::regulate()
{
	//Run PID
	//temperaturePID->run();
	if (heaterON > HEATER_INTERVAL_MAX)
	{
		heaterON = HEATER_INTERVAL_INNITIAL;
	}
	//co2PID->run();

	//Regulate heater
	if (T_regulation) {
		regulateHeater();
	}
	else {
		LOG(3, "Temperature not regulated!");
	}
	//Regulate co2
	if (CO2_regulation) {
		regulateCO2();
	}
	else {
		LOG(3, "CO2 not regulated!");
	}
}
#pragma endregion

#pragma region void Incubator::regulateHeater()
/* Temperature regulation
Input: /
Output: /
Description:
	* Check for heater error -> exit if error
	* Check if regulation interval has passed
	* Set heater temperature with setHeaterTemperature()
	* Calculate temperature gradient for heater and sensors
	* Determine heater interval and turn on/off the heater. There are 3 modes:
		- Overshoot: turn off
		- Max heating: temperature below set level - leave heater on 
		- Normal: check if heater is on/off and turn it off/on. Set interval based on pid and temperature gradient.
	* Send on/off command to heater.
*/
void Incubator::regulateHeater()
{
	//Check for heater error
	if (checkHeaterERROR())
	{
		//Check if heater interval is compleated
		LOG(5, "Current heater interval: %d, set heater interval: %d", millis() - last_heater_update, heater_interval);
		if (millis() - last_heater_update > heater_interval)
		{
			setHeaterTemperature();
			calculateTemperatureGradient();
			
			//Overshoot set error
			if (T_max > T_set + 0.05)
			{
				//Turn off heater for one period
				heater = false; //Turn off
				heater_interval = HEATER_INTERVAL_INNITIAL;
				//Check for significant overshoot
				/*if (T_max > T_set + 0.2)
				{
					ERRORS.temp += 1; //Raise temperature error
					
					//Check for prolonged error
					if (ERRORS.temp >= ERROR_NUMBER_LIMIT)
					{
						//Check if temperature is still increasing
						if (T_avg_gradient > 0 || T_heater_gradient > 0)
						{
							LOG(0, "Prolonged temperature overshoot and still increasing! Go to ALERT state!");
							incubatorState = IncubatorState::ALERT;
						}
						else
						{
							LOG(1, "Prolonged temperature overshoot, max temperature: %.2f C!", T_max);
						}
					}
					else
					{
						LOG(2, "Temperature overshoot, max temperature: %.2f C!", T_max);
					}
				}
				else
				{
					LOG(3, "Small temperature overshoot, max temperature: %.2f C!", T_max);
				}*/
			}
			//Heater temperature below set heater temperature
			else if (T_heater_set - T_heater > HEATER_BANG)
			{
				ERRORS.temp = 0;
				heater = true;
				heater_interval = (int)(HEATER_INTERVAL_INNITIAL + heaterON);
			}
			else
			{
				ERRORS.temp = 0;
				//Check if heater is on
				if (heater)
				{
					//Turn off for one period
					heater = false; //Turn off
					heater_interval = HEATER_INTERVAL_INNITIAL;

				}
				else
				{
					//Temperature increasing
					if (T_heater_gradient > 0.0)
					{
						//Heater temperature increasing - approaching set limit -> turn off
						if (T_heater_set - T_heater < T_HEATER_THRESHOLD)
						{
							//Leave off
							heater_interval = HEATER_INTERVAL_INNITIAL;
						}
						else
						{
							//Turn on
							heater = true;
							heater_interval = (int)(HEATER_INTERVAL_INNITIAL + heaterON);
						}
					}
					else if (T_heater_gradient == 0)
					{
						if (T_heater < T_heater_set)
						{
							//Turn on
							heater = true;
							heater_interval = (int)(HEATER_INTERVAL_INNITIAL + heaterON);
						}
						else
						{
							//Leave off
							heater_interval = HEATER_INTERVAL_INNITIAL;
						}
					}
					else
					{
						//Heater temperature decreasing, but above set threshold -> turn off
						if (T_heater - T_heater_set > T_HEATER_THRESHOLD)
						{
							//Leave off
							heater_interval = HEATER_INTERVAL_INNITIAL;
						}
						else
						{
							//Turn on
							heater = true;
							heater_interval = (int)(HEATER_INTERVAL_INNITIAL + heaterON);
						}
					}
				}
			}

			//Send data to heater
			digitalWrite(OUTPUT_PIN_HEATER, heater);
			if (heater) {
				LOG(5, "Heater ON!");
			}
			else {
				LOG(5, "Heater OFF!");
			}
			last_heater_update = millis();
		}
		else
		{
			LOG(5, "Wait for update heater interval: %d, last update: %d", heater_interval, last_heater_update);
		}
	}
	else
	{
		//Send data to heater
		heater = false;
		digitalWrite(OUTPUT_PIN_HEATER, heater);
		LOG(5, "Heater OFF!");
		last_heater_update = millis();
	}
}
#pragma endregion

#pragma region void Incubator::setHeaterTemperature()
/* Determine heater temperature
Input: /
Output: /
Description:
	* Determine heater temperature based on the difference between set temperature and current temperature. 
*/
void Incubator::setHeaterTemperature()
{
	//Set temperature of the heater
	if (T_set - T_max <= 0.0)
	{
		T_heater_set = T_set + 0.2;
	}
	else if (T_set - T_max <= 1.0)
	{
		T_heater_set = T_set + 3.0 * (T_set - T_max) + 0.5 + (T_set - T_max) * 2.5;
	}
	else
	{
		T_heater_set = T_set + 3.0 * (T_set - T_max) + 3.0;
	}
	//Check for max value
	if (T_heater_set > HEATER_MAX_T)
	{
		T_heater_set = HEATER_MAX_T;
	}
	LOG(5, "Set temperature for the heater: %.1f", T_heater_set);
}
#pragma endregion

#pragma region void Incubator::calculateTemperatureGradient()
/* Calculate temperature gradients
Input: /
Output: /
Description:
	* Update gradient calculation interval and check if new gradient needs to be calculated.
	* Calculate gradient for the heater and average incubator temperature.
	* Update values. 
*/
void Incubator::calculateTemperatureGradient()
{
	gradient_interval += heater_interval;
	if (gradient_interval > 2000)
	{
		T_avg_gradient = (T_avg - T_avg_old) / (gradient_interval)* 1000.0;
		T_heater_gradient = (T_heater - T_heater_old) / (gradient_interval)* 1000.0;
		T_avg_old = T_avg;
		T_heater_old = T_heater;
		gradient_interval = 0.0;
		LOG(5, "Heater gradient: %.1f, temperature gradient: %.1f.", T_heater_gradient, T_avg_gradient);
	}
}
#pragma endregion

#pragma region bool Incubator::checkHeaterERROR()
/* Check for heater error
Input: /
Output: bool error status - true if no error, false otherwise
Description:
	* Check if interval for error check has passed
	* If sufficient difference between set and current temperature, check if there was any change.
	* If not, assume error, turn off the heater and go to the error state. 
*/
bool Incubator::checkHeaterERROR()
{
	bool regulate = true;

	//Check sensor 1
	if (isnan(T_1) || T_1 > HEATER_MAX_T || T_1 <= 0)
	{
		LOG(2, "Recieved invalid measurment on temperature sensor 1!");
		ERRORS.temp_sensor_1 += 1;
		if (ERRORS.temp_sensor_1 >= ERROR_NUMBER_LIMIT)
		{
			LOG(1, "Temperature sensor 1 stoped working!");
		}
		regulate = false;
	}
	else
	{
		ERRORS.temp_sensor_1 = 0;
	}
	//Check sensor 2
	if (isnan(T_2) || T_2 > HEATER_MAX_T || T_2 <= 0)
	{
		LOG(2, "Recieved invalid measurment on temperature sensor 2!");
		ERRORS.temp_sensor_2 += 1;
		if (ERRORS.temp_sensor_2 >= ERROR_NUMBER_LIMIT)
		{
			LOG(1, "Temperature sensor 2 stoped working!");
			if (ERRORS.temp_sensor_1 >= ERROR_NUMBER_LIMIT)
			{
				LOG(0, "All temperature sensors not operational! Go to ALERT state!");
				incubatorState = IncubatorState::ALERT;
				return false;
			}
		}
		regulate = false;
	}
	else
	{
		ERRORS.temp_sensor_2 = 0;
	}
	//Check heater
	if (isnan(T_heater) || T_heater > 100 || T_heater <= 0)
	{
		LOG(2, "Recieved invalid measurment on temperature heater sensor!");
		ERRORS.heater_sensor += 1;
		if (ERRORS.heater_sensor >= ERROR_NUMBER_LIMIT)
		{
			LOG(0, "Heater temperature sensor not operational! Go to ALERT state!");
			incubatorState = IncubatorState::ALERT;
			return false;
		}
	}
	else
	{
		ERRORS.heater_sensor = 0;
	}
	//Check if overheated
	if (T_avg > T_set + 0.5 && T_heater_gradient > 0.0)
	{
		LOG(2, "System is overheated!");
		ERRORS.overheat += 1;
		
		//Check if drastically overheated
		if (T_avg > T_set + 2 && T_heater_gradient > 0.0)
		{
			LOG(0, "Heater temperature sensor not operational! Go to ALERT state!");
			incubatorState = IncubatorState::ALERT;
		}
		return false;
	}
	else
	{
		ERRORS.overheat = 0;
	}
}
#pragma endregion

#pragma region void Incubator::regulateCO2()
/* CO2 regulation
Input: /
Output: /
Description:
	* Check if there is co2 valve error -> exit
	* Check if regulation interval has passed
	* Determine interval time, set valve on/off
*/
void Incubator::regulateCO2()
{
	//Check for CO2 error
	if (checkCO2ERROR())
	{
		//Check if heater interval is compleated
		if (millis() - last_co2_update > co2_valve_interval)
		{
			//Chech if heater is off
			if (!co2_valve)
			{
				//Determine heater interval
				if (CO2_set - CO2 < 0.0)
				{
					co2_valve_interval = CO2_INTERVAL_INNITIAL;
				}
				else
				{
					//Long pulse
					if (CO2_set - CO2 > 1.0)
					{
						//co2_valve = true;
						//co2_valve_interval = (int)(1000 + co2_valveON);
						//co2_valve_interval = 100;
						digitalWrite(OUTPUT_PIN_CO2, true);
						LOG(5, "CO2 valve ON!");
						co2_valve = false;
						delay(500);
						co2_valve_interval = CO2_INTERVAL_INNITIAL;
					}
					//Short pulse
					else
					{
						digitalWrite(OUTPUT_PIN_CO2, true);
						LOG(5, "CO2 valve ON!");
						co2_valve = false;
						delay(100);
						co2_valve_interval = 10000;
					}
				}
			}
			else
			{
				co2_valve = false;
				co2_valve_interval = CO2_INTERVAL_INNITIAL;
			}

			//Send data to co2 valve
			digitalWrite(OUTPUT_PIN_CO2, co2_valve);
			LOG(5, "CO2 valve OFF!");
			last_co2_update = millis();
		}
	}
}
#pragma endregion

#pragma region bool Incubator::checkCO2ERROR()
/* Check for CO2 valve error
Input: /
Output: bool co2 error status - true if no error, false otherwise
Description: 
	* Check if error calculation interval has passed
	* If the difference between set and current level is big enough, check if there was any change.
	* Error if no change -> turn the valve off, go to error state.
*/
bool Incubator::checkCO2ERROR()
{
	bool regulate = true;

	//check if it is time to monitor temperature
	/*if (millis() - last_co2_error > CO2_ERROR_CHECK)
	{
		//Check if we are in the phase of heating
		if (CO2_set - CO2 > 0.1)
		{
			//Check for temperature differential
			if (CO2 - CO2_old < 0.01)
			{
				//co2_valve = false;
				//incubatorState = IncubatorState::CO2_ERROR;
				//return false;
			}
		}

		//Update parameters
		last_co2_error = millis();
		CO2_old = CO2;
	}*/

	//Check co2 values
	if (isnan(CO2) || CO2 > 20.0 || CO2 <= 0.03)
	{
		LOG(2, "Recieved invalid measurment on CO2 sensor!");
		ERRORS.co2_sensor += 1;
		if (ERRORS.co2_sensor >= ERROR_NUMBER_LIMIT)
		{
			LOG(0, "CO2 sensor not operational! Go to ALERT state!");
			incubatorState = IncubatorState::ALERT;
			return false;
		}
	}

	return regulate;
}
#pragma endregion

#pragma region bool Incubator::updateTemperature()
/* Update sensor temperature readings
Input: /
Output: bool change - true if average temperature in the incubator has changed.
Description:
	* Make reading from all three temperature sensors, provided they are connected. 
	* Update average temperature that returnes change. 
*/
bool Incubator::updateTemperature() {
	
	//Sensor 1
	if (temp_sensor_connected_1)
	{
		//Make new reading 
		T_1 = temp_sensor_1.readTemperature();
		LOG(5, "Temperature sensor 1: %.2f C.", T_1);
	}
	//Sensor 2
	if (temp_sensor_connected_2)
	{
		//Make new reading 
		T_2 = temp_sensor_2.readTemperature();
		LOG(5, "Temperature sensor 2: %.2f C.", T_2);
	}
	//Heater sensor
	if (temp_sensor_connected_3 && (millis() - last_heater_sensor_update) > TEMP_READ_DELAY)
	{
		//Make new reading
		heater_sensor.requestTemperatures();
		T_heater = heater_sensor.getTempC(heater_sensor_address);
		last_heater_sensor_update = millis();
		LOG(5, "T heater: %.2f C.", T_heater);
	}

	bool change = avgTemperature();
	return change;
}
#pragma endregion

#pragma region bool Incubator::updateCO2() 
/* Update CO2 sensor reading
Input: /
Output: bool change - true if co2 level has changed
Description:
	* If co2 sensor is connected, make reading.
	* Check if level has changed and output the change.
*/
bool Incubator::updateCO2() {
	
	bool change = false;
	if (co2_sensor_connected) {
		//Make new reading
		int newReading = 0;
		bool getStart = false;
		int timeout = millis();
		String inString = "";
		float co2 = 0.0;
		while (newReading < 2) {

			int inChar = co2_sensor->read();

			if (inChar == 'z') {
				inString = "";
				getStart = true;
			}
			if (inChar == 'Z') {
				inString = "";
			}

			if (isDigit(inChar)) {
				// convert the incoming byte to a char and add it to the string:
				inString += (char)inChar;
			}
			// if you get a newline, print the string, then the string's value:
			if (inChar == '\n' && getStart) {
				if (newReading == 0)
				{
					LOG(5, "CO2 raw reading: %d ppm.", inString.toInt() * 10);
					CO2 = inString.toFloat() / 1000.0;
				}
				else
				{
					LOG(5, "CO2 average reading: %d ppm.", inString.toInt() * 10);
					co2 = inString.toFloat() / 1000.0;
				}
				newReading += 1;
			}
		}

		//Average value
		CO2_moving += co2;
		CO2_count += 1;
		if (CO2_count == 5)
		{
			co2 = CO2_moving / 5.0;
			CO2_moving = 0.0;
			CO2_count = 0;
		}
		else
		{
			co2 = CO2_avg;
		}
		
		if (abs(CO2_avg - co2) >= 0.01)
		{
			//Update temperature
			CO2_avg = co2;
			change = true;
		}
		LOG(5, "CO2 averaged value: %.3f .", CO2_avg);
	}
	return change;
}
#pragma endregion

#pragma region bool Incubator::avgTemperature()
/* Calculate average temperature in the incubator from all temperature sensors
Input: /
Output: bool change - true if average value has changed
Description:
	* Calculate average temperature, return change.
*/
bool Incubator::avgTemperature()
{
	bool change = false;
	float avg = (T_1 + T_2) / 2.0;
	float max = T_2;
	if (T_1 > T_2)
	{
		max = T_1;
	}

	if (abs((round(T_avg * 10.0) / 10.0) - (round(avg * 10.0) / 10.0)) >= 0.1)
	{
		change = true;
	}
	T_avg = avg;
	T_max = max;

	LOG(5, "Average temperature:  %.2f C", T_avg);
	LOG(5, "Max temperature:  %.2f C", T_max);

	return change;

}
#pragma endregion

#pragma region float Incubator::updateValue(float val, float step, float sgn)
/* Update value for desired step
Input:
	* float val - original value
	* float step - desired increment
	* float sgn - desired sign of increment (+/- 1)
Output:
	* float - updated value
*/
float Incubator::updateValue(float val, float step, float sgn)
{
	return val + sgn * step; 
}
#pragma endregion

void Incubator::logData()
{
	//Update time
	getLocalTime(&tmstruct, 5000);
	//Open file
	logfile = SD.open(filename, FILE_APPEND);
	//PRINT DATA
	logfile.printf("%d-%02d-%02d %02d:%02d:%02d,", (tmstruct.tm_year) + 1900, (tmstruct.tm_mon) + 1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec);
	logfile.print(T_1);
	logfile.print(",");
	logfile.print(T_2);
	logfile.print(",");
	logfile.print(T_heater);
	logfile.print(",");
	logfile.print(T_set);
	logfile.print(",");
	logfile.print(CO2);
	logfile.print(",");
	logfile.println(CO2_set);
	logfile.close();
}

void Incubator::handleAlert() {
	//turn off regulation
	digitalWrite(OUTPUT_PIN_HEATER, false);
	digitalWrite(OUTPUT_PIN_CO2, false);
	ESP.restart();
	delay(1000);
}

//PRINT FUNCTIONS

#pragma region void Incubator::printScreenInitial()
/* Print innitial screen
Input: /
Output: / 
Description:
	* Print innitial welcome screen.
*/
void Incubator::printScreenInitial(){
  oled->clearDisplay();
  oled->setTextSize(2);
  oled->setTextColor(WHITE);
  oled->setCursor(0,0);
  oled->println("Welcome!");
  oled->setTextSize(1);
  oled->setCursor(0,20);
  oled->println("The setup state will begin shortly.");
  oled->println("Wait for the instructions...");
  oled->display();
  delay(2000);
}
#pragma endregion

#pragma region void Incubator::printScreenSetConnection()
/* Print set connection screen
Input: /
Output: /
Description:
	* Print instructions to connect to locala incubator WiFi with set password. 
*/
void Incubator::printScreenSetConnection() {
  oled->clearDisplay();
  oled->setTextSize(1);
  oled->setTextColor(WHITE);
  oled->setCursor(0,0);
  oled->print("Connect to local WiFi: ");
  oled->setCursor(0,12);
  oled->setTextSize(2);
  oled->println(localSSID);
  oled->setCursor(0,31);
  oled->setTextSize(1);
  oled->print("WiFi password: ");
  oled->setCursor(0,43);
  oled->setTextSize(2);
  oled->println(localPassword);
  //oled->print("Local IP address: "); 
  //oled->println(WiFi.softAPIP());
  oled->display();
  delay(1);
}
#pragma endregion

#pragma region void Incubator::printScreenConnected() 
/* Print connection details screen
Input: /
Output: /
Description:
* Print connection detail, once device is connected to wifi.
*/
void Incubator::printScreenConnected(){
  oled->clearDisplay();
  oled->setTextSize(1);
  oled->setTextColor(WHITE);
  oled->setCursor(0,0);
  oled->println("Connected to WiFi:");
  oled->setCursor(0,12);
  oled->setTextSize(2);
  oled->println(WiFi.SSID());
  oled->setCursor(0,31);
  oled->setTextSize(1);
  oled->println("Device IP address:");
  oled->setCursor(0,45);
  oled->println(WiFi.localIP());
  oled->display();
  delay(2000);
}
#pragma endregion

#pragma region void Incubator::printScreenOffline()
/* Print offline mode screen
Input: /
Output: /
Description:
* Print offline mode, if connection cannot be stablished or user choosed this option.
*/
void Incubator::printScreenOffline(){
  oled->clearDisplay();
  oled->setTextSize(1);
  oled->setTextColor(WHITE);
  oled->setCursor(0,0);
  oled->println("Starting offline operation mode.");
  oled->display();
  delay(1);
}
#pragma endregion

#pragma region void Incubator::printScreenSensorSetup()
/* Print sensor setup screen
Input: /
Output: /
Description:
* Print for each sensor if connected or not.
*/
void Incubator::printScreenSensorSetup(){
  oled->clearDisplay();
  oled->setTextSize(1);
  oled->setTextColor(WHITE);
  oled->setCursor(0,0);
  if(temp_sensor_connected_1){
	oled->println("T1 CONNECTED!");
  }
  else{
	oled->println("T1 NOT CONNECTED!");  
  }
  oled->setCursor(0, 15);
  if(temp_sensor_connected_2){
	oled->println("T2 CONNECTED!");
  }
  else{
	oled->println("T2 NOT CONNECTED!");
  }
  oled->setCursor(0, 30);
  if (temp_sensor_connected_3) {
	  oled->println("T3 CONNECTED!");
  }
  else {
	  oled->println("T3 NOT CONNECTED!");
  }
  oled->setCursor(0, 45);
  if (co2_sensor_connected) {
	  oled->println("CO2 CONNECTED!");
  }
  else {
	  oled->println("CO2 NOT CONNECTED!");
  }
  oled->display();
  delay(10);
}
#pragma endregion

#pragma region void Incubator::printScreenMain(IPAddress ip, float t, float co2)
/* Print main screen
Input: 
	* IPAddress ip - incubator ip address
	* float t - incubator temperature
	* float co2 - incubator co2 levels
Output: /
Description:
	* Setup the main screen.
	* Print ip, button values, temperature and co2 levels. 
*/
void Incubator::printScreenMain(IPAddress ip, float t, float co2){
	
	LOG(5, "Setting up the main sreen.");
	//screen state
	screenState = ScreenState::MAIN;

	//Background
	oled->clearDisplay();
	oled->drawLine(0, HEIGHT_IP, oled->width()-1, HEIGHT_IP, WHITE);
	oled->drawLine(0, HEIGHT_BUTTONS, oled->width()-1, HEIGHT_BUTTONS, WHITE);
	oled->drawLine(oled->width()/2, HEIGHT_IP, oled->width()/2, HEIGHT_BUTTONS, WHITE);
	oled->setTextSize(1);
	oled->setCursor(0,HEIGHT_TEXT);
	oled->println("TEMP (C)");
	oled->setCursor(oled->width()/2 + TEXT_OFFSET,HEIGHT_TEXT);
	oled->println("CO2 (%)");
	//IP
	printScreenIP(ip);
	//Button 1&2
	printScreenButtonLeft();
	printScreenButtonRight();
	//Temperature
	printScreenTemperature(t);
	//CO2
	printScreenCO2(co2);
	oled->display();
	delay(1);
}
#pragma endregion

#pragma region void Incubator::printScreenSet(IPAddress ip)
/* Print setup screen for temperature and co2 value
Input:
	* IPAddress ip - incubator ip address
Output: /
Description:
	* Setup the background screen.
	* Print ip, button values.
	* Based on the screen state either display desired temperature or co2 value. 
*/
void Incubator::printScreenSet(IPAddress ip) {

	LOG(5,"Setting up the SET screen.");
	//Background
	oled->clearDisplay();
	oled->drawLine(0, HEIGHT_IP, oled->width() - 1, HEIGHT_IP, WHITE);
	oled->drawLine(0, HEIGHT_BUTTONS, oled->width() - 1, HEIGHT_BUTTONS, WHITE);
	//IP
	printScreenIP(ip);
	//Button 1&2
	printScreenButtonLeft();
	printScreenButtonRight();
	oled->setCursor(0, HEIGHT_TEXT);
	oled->setTextSize(1);
	if (screenState == ScreenState::SET_TEMP) {
		oled->println("SET TEMPERATURE (C)");
		printScreenSetValue(T_set, 1);
	}
	else {
		oled->println("SET CO2 LEVEL (%)");
		printScreenSetValue(CO2_set, 2);
	}
	oled->display();
	delay(1);
}
#pragma endregion

#pragma region void Incubator::printScreenIP(IPAddress ip)
/* Print IP address of the incubator
Input: IPAddress
Output: /
Description:
	* Print IP address at the top of the screen. 
*/
void Incubator::printScreenIP(IPAddress ip){
  //Clear previous ip
  oled->fillRect(0, 0, oled->width()-1, HEIGHT_SMALL, BLACK);
  //Re-write
  oled->setTextSize(1);
  oled->setCursor(0,0);
  oled->print("IP: ");
  oled->println(ip);
  oled->display();
}
#pragma endregion

#pragma region void Incubator::printScreenButtonLeft()
/* Print value of the left button
Input: /
Output: /
Description: /
*/
void Incubator::printScreenButtonLeft(){
  //Clear previous button
  oled->fillRect(0, HEIGHT_BUTTONS + VERTICAL_OFFSET, oled->width()/2, HEIGHT_SMALL, BLACK);
  oled->setTextSize(1);
  oled->setCursor(0, HEIGHT_BUTTONS + VERTICAL_OFFSET);
  oled->println(button_1.buttonName);
  oled->display();
  delay(1);
}
#pragma endregion

#pragma region void Incubator::printScreenButtonRight()
/* Print value of the right button
Input: /
Output: /
Description: /
*/
void Incubator::printScreenButtonRight(){
  //Clear previous button
  oled->fillRect(oled->width()/2 + HORIZONTAL_OFFSET, HEIGHT_BUTTONS + VERTICAL_OFFSET, oled->width()/2 - HORIZONTAL_OFFSET, HEIGHT_SMALL, BLACK);
  oled->setTextSize(1);
  oled->setCursor(oled->width()/2 + HORIZONTAL_OFFSET, HEIGHT_BUTTONS + VERTICAL_OFFSET);
  oled->println(button_2.buttonName);
  oled->display();
  delay(1);
}
#pragma endregion

#pragma region void Incubator::printScreenTemperature(float temp)
/* Print temperature on the main screen
Input: float temp - temperature
Output: /
Description:
	* Print temperature, rounded to one decimal place on the left side of the main screen.
*/
void Incubator::printScreenTemperature(float temp){
  //Clear previous reading
  oled->fillRect(0,HEIGHT_TEMP, oled->width()/2-1, HEIGHT_LARGE, BLACK);
  //Write new
  oled->display();
  oled->setTextSize(2);
  oled->setCursor(0,HEIGHT_TEMP);
  oled->print(temp, 1);
  oled->display();
  delay(1);
}
#pragma endregion

#pragma region void Incubator::printScreenSetValue(float val, int precision)
/* Print set value screen
Input: 
	* float val - value of either temperature or co2
	* int precision - number of decimal places
Output: /
Description:
	* Print set value screen, with given value.
*/
void Incubator::printScreenSetValue(float val, int precision) {
	LOG(5, "Setting value....");
	//Clear previous reading
	oled->fillRect(0, HEIGHT_SET, oled->width() / 2, HEIGHT_LARGE, BLACK);
	//Write new
	oled->setTextSize(2);
	oled->setCursor(0, HEIGHT_SET);
	oled->print(val, precision);
	oled->display();
	delay(1);
}
#pragma endregion

#pragma region void Incubator::printScreenCO2(float co2)
/* Print co2 on the main screen
Input: float co2 - temperature
Output: /
Description:
* Print temperature, rounded to one decimal place on the left side of the main screen.
*/
void Incubator::printScreenCO2(float co2){
  //Clear previous reading
  oled->fillRect(oled->width()/2 + HORIZONTAL_OFFSET, HEIGHT_TEMP, oled->width()/2 - HORIZONTAL_OFFSET, HEIGHT_LARGE, BLACK);
  //Write new
  oled->display();
  oled->setTextSize(2);
  oled->setCursor(oled->width()/2 + HORIZONTAL_OFFSET,HEIGHT_TEMP);
  oled->print(co2, 2);
  oled->display();
  delay(1);
}
#pragma endregion

//GET FUNCTIONS
String Incubator::get_incubator_name() {
	return incubator_name;
};

float Incubator::get_T_heater_set() {
	return T_heater_set;
};

float Incubator::get_T_set() {
	return T_set;
};

float Incubator::get_T_1() {
	return T_1;
};

float Incubator::get_T_2() {
	return T_2;
};

float Incubator::get_T_heater() {
	return T_heater;
};

float Incubator::get_T_heater_gradient() {
	return T_heater_gradient;
};

float Incubator::get_T_avg() {
	return T_avg;
};

float Incubator::get_heater_interval() {
	return heater_interval;
};

float Incubator::get_heaterON() {
	return heaterON;
};

bool Incubator::get_heater() {
	return heater;
};

float Incubator::get_CO2_set() {
	return CO2_set;
};

float Incubator::get_CO2() {
	return CO2;
};

float Incubator::get_CO2_avg() {
	return CO2_avg;
};

float Incubator::get_co2_valve_interval() {
	return co2_valve_interval;
};

bool Incubator::get_co2_valve() {
	return co2_valve;
};

//SET functions

void Incubator::set_T(float tmpT)
{
	T_set = tmpT;
	if (T_set > T_MAX)
	{
		T_set = T_MAX;
	}
	if (T_set < T_MIN)
	{
		T_set = T_MIN;
	}
	preferences.begin("incubator", false);
	preferences.putFloat("setT", T_set);
	preferences.end();
	screenState = ScreenState::SET_TEMP;
	//button_1.buttonState = ButtonState::B_SET_CO2;
	printScreenSet(deviceIP);
	screenStateTimer = millis();
}

void Incubator::set_co2(float tmpCo2)
{
	CO2_set = tmpCo2;
	if (CO2_set > CO2_MAX)
	{
		CO2_set = CO2_MAX;
	}
	if (CO2_set < CO2_MIN)
	{
		CO2_set = CO2_MIN;
	}
	preferences.begin("incubator", false);
	preferences.putFloat("setCO2", CO2_set);
	preferences.end();
	screenState = ScreenState::SET_CO2;
	//button_1.buttonState = ButtonState::B_MAIN;
	printScreenSet(deviceIP);
	screenStateTimer = millis();
}

void Incubator::set_incubator_name(String name)
{
	incubator_name = name;
	preferences.begin("incubator", false);
	preferences.putString("incubatorName", incubator_name);
	preferences.end();
}

void Incubator::set_temp_notifications(int set)
{
	temp_notifications = (bool)set;
	preferences.begin("incubator", false);
	preferences.putBool("sendTemp", temp_notifications);
	preferences.end();
};

void Incubator::set_co2_notifications(int set)
{
	co2_notifications = (bool)set;
	preferences.begin("incubator", false);
	preferences.putBool("sendCO2", co2_notifications);
	preferences.end();
};

void Incubator::set_door_notifications(int set)
{
	door_notifications = (bool)set;
	preferences.begin("incubator", false);
	preferences.putBool("sendDoor", door_notifications);
	preferences.end();
};

void Incubator::set_mail_notifications(int set)
{
	send_mail = (bool)set;
	preferences.begin("incubator", false);
	preferences.putBool("sendMail", send_mail);
	preferences.end();
};

void Incubator::set_push_notifications(int set)
{
	send_push = (bool)set;
	preferences.begin("incubator", false);
	preferences.putBool("sendPush", send_push);
	preferences.end();
};

void Incubator::set_system_notifications(int set)
{
	system_notifications = (bool)set;
	preferences.begin("incubator", false);
	preferences.putBool("sendSystem", system_notifications);
	preferences.end();
};

void Incubator::set_mail(String name)
{
	mail = name;
	preferences.begin("incubator", false);
	preferences.putString("mail", mail);
	preferences.end();
}

void Incubator::turn_off_heater() {
	heater = false;
	digitalWrite(OUTPUT_PIN_HEATER, heater);
}

//WEB INTERFACE

String Incubator::getPage() {
	String page = "<!doctype html>";
	page += "<html lang='en'>";
	page += "  <head>";
	page += "    <!-- Required meta tags -->";
	page += "    <meta charset='utf-8'>";
	page += "    <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'> ";
	page += "    <!-- Bootstrap CSS -->";
	page += "    <link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.1.1/css/bootstrap.min.css' integrity='sha384-WskhaSGFgHYWDcbwN70/dfYBj47jz9qbsMId/iRN3ewGhXQFZCSftd1LZCfmhktB' crossorigin='anonymous'> ";
	page += "  <!-- jQuery -->";
	page += "  <script src='https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js'></script>";
	page += "  <!-- JavaScript -->";
	page += "  <script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js' integrity='sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa' crossorigin='anonymous'></script>";
	page += "    <title>Test WiFi interface</title>";
	page += "  <script>";
	page += "    function GetSensorValues()";
	page += "    {";
	page += "      var request = new XMLHttpRequest();";
	page += "      request.onreadystatechange = function(){";
	page += "        if(this.readyState == 4 && this.status == 200){";
	page += "          if(this.responseText != null){";
	page += "            var a = JSON.parse(this.responseText);";
	page += "            document.getElementById('t_avg').innerHTML = a.t_avg;";
	page += "            document.getElementById('t_set').innerHTML = a.t_set;";
	page += "            document.getElementById('t_1').innerHTML = a.t_2;";
	page += "            document.getElementById('t_2').innerHTML = a.t_1;";
	page += "            document.getElementById('t_h').innerHTML = a.t_h;";
	page += "            document.getElementById('co2_avg').innerHTML = a.co2_avg;";
	page += "            document.getElementById('co2_set').innerHTML = a.co2_set;";
	page += "            document.getElementById('co2_1').innerHTML = a.co2_1;";
	page += "            document.getElementById('co2_2').innerHTML = a.co2_2;}";
	page += "        }";
	page += "      }; ";
	page += "      request.open('GET', 'getValues', true);";
	page += "      request.send();";
	page += "      setTimeout('GetSensorValues()', 10000);";
	page += "    }";
	page += "    function SetTempreature()";
	page += "    {";
	page += "      var request = new XMLHttpRequest();";
	page += "      var t = document.getElementById('t_new').value;";
	page += "      document.getElementById('t_new').value = '';";
	page += "      request.open('GET', 'setTemp?temp='+t, true);";
	page += "      request.send();";
	page += "    }";
	page += "    function SetCO2()";
	page += "    {";
	page += "      var request = new XMLHttpRequest();";
	page += "      var c = document.getElementById('co2_new').value;";
	page += "      document.getElementById('co2_new').value = '';";
	page += "      request.open('GET', 'setCO2?co2='+c, true);";
	page += "      request.send();";
	page += "    }";
	page += "    function getSettings()";
	page += "    {";
	page += "      var request = new XMLHttpRequest();";
	page += "     request.onreadystatechange = function(){";
	page += "       if(this.readyState == 4 && this.status == 200){";
	page += "         document.getElementsByTagName('body')[0].innerHTML = this.responseText;";
	page += "       };";
	page += "     }; ";
	page += "      request.open('GET', 'settings', true);";
	page += "      request.send();";
	page += "    }";
	page += "    function settingsReturn()";
	page += "    {";
	page += "      var request = new XMLHttpRequest();";
	page += "     request.onreadystatechange = function(){";
	page += "       if(this.readyState == 4 && this.status == 200){";
	page += "         document.getElementsByTagName('body')[0].innerHTML = this.responseText;";
	page += "       };";
	page += "     }; ";
	page += "      request.open('GET', 'return', false);";
	page += "      request.send();";
	page += "    }";
	page += "    function resetWiFi()";
	page += "    {";
	page += "      var request = new XMLHttpRequest();";
	page += "     request.onreadystatechange = function(){";
	page += "       if(this.readyState == 4 && this.status == 200){";
	page += "         document.getElementsByTagName('body')[0].innerHTML = this.responseText;";
	page += "       };";
	page += "     }; ";
	page += "      request.open('GET', 'resetWiFi', false);";
	page += "      request.send();";
	page += "    }";
	page += "    function SaveSettings()";
	page += "    {";
	page += "      var request = new XMLHttpRequest();";
	page += "     request.onreadystatechange = function(){";
	page += "       if(this.readyState == 4 && this.status == 200){";
	page += "         document.getElementsByTagName('body')[0].innerHTML = this.responseText;";
	page += "       };";
	page += "     }; ";
	page += "      var name = document.getElementById('name').value;";
	page += "      document.getElementById('name').value = '';";
	page += "      request.open('GET', 'save?name='+name, true);";
	page += "      request.send();";
	page += "    }";
	page += "  </script>";
	page += "  </head>";
	page += "  <body onload='GetSensorValues()' style='width: 80%; margin-right: auto; margin-left: auto; margin-top: 1%'>";
	page += "  <div>";
	page += "    <div class='row' style='background-color:LightGray;'>";
	page += "      <div class='col-md-8' style='padding-top: 2%; padding-bottom: 1%;'>";
	page += "        <h1>INCUBATOR: ";
	page += incubator_name;
	page += "        </h1>";
	page += "      </div>";
	page += "      <div class='col-md-4'>";
	page += "        <div class='row'>";
	page += "          <div class='col-md-8' style='padding-top: 3%; padding-bottom: 2%;'>";
	page += "            <h3>Wifi: ";
	page += WiFi.SSID();
	page += "            </h3>";
	page += "            <h3>Incubator IP: ";
	page += WiFi.localIP().toString();
	page += "            </h3>";
	page += "          </div>";
	page += "          <div class='col-md-4' style='padding-top: 1%; padding-bottom: 1%;'>";
	page += "            <button type='button' onclick='getSettings()' class='btn btn-primary btn-lg btn-block'>Settings</button>";
	page += "            <button type='button' onclick='resetWiFi()' class='btn btn-danger btn-lg btn-block'>Reset Wifi</button>";
	page += "          </div>";
	page += "        </div>";
	page += "      </div>";
	page += "    </div>";
	page += "    <div class='row' style='margin-top: 1%'>";
	page += "      <div class='col-md-6' style='margin: 0; padding: 0;'>";
	page += "      <div style='background-color:rgb(240, 240, 240); width: 99%; margin-right: 1%; padding: 2%;'>";
	page += "        <div class='row'>";
	page += "          <div class='col-md-12'>";
	page += "            <h2>TEMPERATURE</h2>";
	page += "          </div>";
	page += "        </div>";
	page += "        <div class='row'>";
	page += "          <div class='col-md-6'>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12'>";
	page += "                <p>Average temperature:</p>";
	page += "              </div>";
	page += "            </div>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12'>";
	page += "                <h1><span id='t_avg'>";
	page += T_avg;
	page += "                </span>&#8451;</h1>";
	page += "              </div>";
	page += "            </div>";
	page += "          </div>";
	page += "          <div class='col-md-3'>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12'>";
	page += "                <p>Set temperature:</p>";
	page += "              </div>";
	page += "            </div>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12'>";
	page += "                <h1><span id='t_set'>";
	page += T_set;
	page += "                </span> &#8451;</h1>";
	page += "              </div>";
	page += "            </div>";
	page += "          </div>";
	page += "          <div class='col-md-3'>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12'>";
	page += "                <p>New temperature:</p>";
	page += "              </div>";
	page += "            </div>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12' style='padding-top: 2%'>";
	page += "                <input type='text' id='t_new' maxlength='4' size='4'>";
	page += "                <button type='button' onclick='SetTempreature()' class='btn btn-success'>Submit</button>";
	page += "              </div>";
	page += "            </div>";
	page += "          </div>";
	page += "        </div>";
	page += "        <div class='row'>";
	page += "          <div class='col-md-12'>";
	page += "            <h3>Sensor readings:</h3>";
	page += "          </div>";
	page += "        </div>";
	page += "        <div class='row'>";
	page += "          <div class='col-md-4'>";
	page += "            <h4>Sensor 1: <b><span id='t_1'>";
	page += T_1;
	page += "            </span> &#8451;</b></h4>";
	page += "          </div>";
	page += "          <div class='col-md-4'>";
	page += "            <h4>Sensor 2: <b><span id='t_2'>";
	page += T_2;
	page += "            </span> &#8451;</b></h4>";
	page += "          </div>";
	page += "          <div class='col-md-4'>";
	page += "            <h4>Heater: <b><span id='t_h'>";
	page += T_heater;
	page += "            </span> &#8451;</b></h4>";
	page += "          </div>";
	page += "        </div>";
	page += "      </div>";
	page += "      </div>";
	page += "      <div class='col-md-6' style='margin: 0; padding: 0;'>";
	page += "      <div style='background-color:rgb(240, 240, 240); width: 99%; margin-left: 1%; padding: 2%;'>";
	page += "        <div class='row'>";
	page += "          <div class='col-md-12'>";
	page += "            <h2>CO2 LEVEL</h2>";
	page += "          </div>";
	page += "        </div>";
	page += "        <div class='row'>";
	page += "          <div class='col-md-6'>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12'>";
	page += "                <p>Average Co2 level:</p>";
	page += "              </div>";
	page += "            </div>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12'>";
	page += "                <h1><span id='co2_avg'>";
	page += CO2_avg;
	page += "                </span> %</h1>";
	page += "              </div>";
	page += "            </div>";
	page += "          </div>";
	page += "          <div class='col-md-3'>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12'>";
	page += "                <p>Set Co2 level:</p>";
	page += "              </div>";
	page += "            </div>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12'>";
	page += "                <h1><span id='co2_set'>";
	page += CO2_set;
	page += "                </span> %</h1>";
	page += "              </div>";
	page += "            </div>";
	page += "          </div>";
	page += "          <div class='col-md-3'>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12'>";
	page += "                <p>New Co2 level:</p>";
	page += "              </div>";
	page += "            </div>";
	page += "            <div class='row'>";
	page += "              <div class='col-md-12' style='padding-top: 2%'>";
	page += "                <input type='text' id='co2_new' maxlength='4' size='4'>";
	page += "                <button type='button' onclick='SetCO2()' class='btn btn-success'>Submit</button>";
	page += "              </div>";
	page += "            </div>";
	page += "          </div>";
	page += "        </div>";
	page += "        <div class='row'>";
	page += "          <div class='col-md-12'>";
	page += "            <h3>Sensor readings:</h3>";
	page += "          </div>";
	page += "        </div>";
	page += "        <div class='row'>";
	page += "          <div class='col-md-4'>";
	page += "            <h4>Sensor 1: <b><span id='co2_1'>";
	page += CO2;
	page += "            </span> %</b></h4>";
	page += "          </div>";
	page += "          <div class='col-md-4'>";
	page += "            <h4>Sensor 2: <b><span id='co2_2'>";
	page += CO2;
	page += "            </span> %</b></h4>";
	page += "          </div>";
	page += "        </div>";
	page += "      </div>";
	page += "      </div>";
	page += "    </div>";
	page += "  </div>";
	page += "  </body>";
	page += "</html>";

	return page;
}

String Incubator::getSettings() {
	String page = "<body style='width: 80%; margin-right: auto; margin-left: auto; margin-top: 1%'>";
	page += " <div>";
	page += "   <div class='row' style='background-color:LightGray;'>";
	page += "      <div class='col-md-8' style='padding-top: 2%; padding-bottom: 1%;'>";
	page += "        <h1>INCUBATOR: ";
	page += incubator_name;
	page += "        </h1>";
	page += "      </div>";
	page += "     <div class='col-md-4'>";
	page += "       <div class='row'>";
	page += "         <div class='col-md-8' style='padding-top: 3%; padding-bottom: 2%;'>";
	page += "            <h3>Wifi: ";
	page += WiFi.SSID();
	page += "            </h3>";
	page += "            <h3>Incubator IP: ";
	page += WiFi.localIP().toString();
	page += "            </h3>";
	page += "         </div>";
	page += "         <div class='col-md-4' style='padding-top: 1%; padding-bottom: 1%;'>";
	page += "           <button type='button' onclick='settingsReturn()' class='btn btn-primary btn-lg btn-block'>Back</button>";
	page += "           <button type='button' onclick='resetWiFi()' class='btn btn-danger btn-lg btn-block'>Reset Wifi</button>";
	page += "         </div>";
	page += "       </div>";
	page += "     </div>";
	page += "   </div>";
	page += "   <div class='row' style='margin-top: 1%; background-color:rgb(240, 240, 240);'>";
	page += "     <div class='col-md-12' >";
	page += "       <form>";
	page += "         <div class='form-group'>";
	page += "           <label for='usr'>Incubator name:</label>";
	page += "           <input type='text' class='form-control' id='name'>";
	page += "         </div>";
	page += "         <div class='form-group'>";
	page += "           <label for='pwd'>Other setting</label>";
	page += "           <input type='text' class='form-control' id='pwd'>";
	page += "         </div>";
	page += "         </form>";
	page += "     </div>";
	page += "   </div>";
	page += "   <div class='row justify-content-end' style=' background-color:rgb(240, 240, 240);'>";
	page += "     <div class='col-md-2' style='padding-bottom: 1%;'>";
	page += "       <button type='button' onclick='SaveSettings()' class='btn btn-success btn-lg btn-block'>Save</button>";
	page += "     </div>";
	page += "   </div>";
	page += " </div>";
	page += "  </body>";
	return page;
}

String Incubator::getReset() {
	String page = "<body style='width: 80%; margin-right: auto; margin-left: auto; margin-top: 1%'>";
	page += "  <div>";
	page += "    <p>Reseating WiFi... Please follow the instructions on the screen.</p>";
	page += "  </div>";
	page += "</body>";

	return page;
}

#pragma region void LOG(String text, int level, bool start, bool stop)
/* Logging function
Input: 
	* text - text string to be written
	* int level - log level 0 - 5
	* bool start - start new line of the log
	* bool end - end line of the log
Output: /
Description:
	* Specifie level at the start of the new line.
*/
void Incubator::LOG(int level, const char* text, ...)
{
	//Combine text
	char msg[100];
	va_list  args;
	va_start(args, text);
	vsprintf(msg, text, args);
	va_end(args);
	
	//Write level specifier at the strat of the new line
	switch (level) {
	case 0:
	{
		ALERT_PRINT(millis());
		ALERT_PRINT("  ALERT: ");
		ALERT_PRINTLN(msg);
		return;
	}
	case 1:
	{
		CRIT_PRINT(millis());
		CRIT_PRINT("  CRITICAL: ");		
		CRIT_PRINTLN(msg);
		return;
	}
	case 2:
	{
		ERR_PRINT(millis());
		ERR_PRINT("  ERROR: ");
		ERR_PRINTLN(msg);
		return;
	}
	case 3:
	{
		WARN_PRINT(millis());
		WARN_PRINT("  WARNING: ");
		WARN_PRINTLN(msg);
		return;
	}
	case 4:
	{
		INFO_PRINT(millis());
		INFO_PRINT("  INFO: ");
		INFO_PRINTLN(msg);
		return;
	}
	case 5:
	{
		DEBUG_PRINT(millis());
		DEBUG_PRINT("  DEBUG: ");
		DEBUG_PRINTLN(msg);
		return;
	}
	}
}
#pragma endregion