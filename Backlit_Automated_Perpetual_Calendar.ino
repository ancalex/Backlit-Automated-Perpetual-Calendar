/*
 * * ESP8266 template with phone config web page
 * based on ESP 8266 Arduino IDE WebConfig by John Lassen and
 * BVB_WebConfig_OTA_V7 from Andreas Spiess https://github.com/SensorsIot/Internet-of-Things-with-ESP8266
 *
 */
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#include "FastLED.h"
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define LED_PIN 2
#define NUM_LEDS  72
CRGB leds[NUM_LEDS];
uint8 BRIGHTNESS = 100;
byte calendar_leds[] = {0, 1, 2, 3, 4, 5, 6,
		19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33,
		46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
		71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59};
byte calendar_months[] = {57, 58, 67, 66, 65, 64, 63, 62, 61, 60, 59, 56};
byte warning_led = 69; // used only for no wifi
byte holiday_led = 68;
byte anniversary_led = 55;
CRGB weekday_color = CRGB(0,255,64);
CRGB actualday_color = CRGB(0,148,255);
CRGB weekend_color = CRGB(128,0,0);
CRGB month_color = CRGB(255,216,0);
byte holidays[] = {1, 1, 7, 4, 12, 25};
byte anniversaries[] = {3, 2, 5, 26, 8, 15};
int temp_hour = 0;
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <EEPROM.h>
#include "global.h"
#include "NTP.h"

// Include STYLE and Script "Pages"
#include "Page_Script.js.h"
#include "Page_Style.css.h"

// Include HTML "Pages"
#include "Page_Admin.h"
#include "Page_NTPSettings.h"
#include "Page_Information.h"
#include "Page_NetworkConfiguration.h"
#include "Page_SetTime.h"

extern "C" {
#include "user_interface.h"
}

void CalendarDisplay(int y, byte m, byte d) {
	//erase all leds
	fill_solid( leds, NUM_LEDS, CRGB(0,0,0));
	byte mdays = daysInMonth(y, m); // number of days from the current month
	int i = 0;
	for (int k = 1; k <= mdays; k++) {
		int z = DayOfTheWeek(y, m, k); // sunday-0 saturday-6
		Serial.print(z);Serial.print("--");Serial.print(k);Serial.print("--");Serial.print(i);Serial.print("--");Serial.println(calendar_leds[i]);
		if (z == 6) {
			if (d == k) {
				leds[calendar_leds[i]] = actualday_color;
			}
			else {
				leds[calendar_leds[i]] = weekend_color;
			}
			if (config.FirstWeekDay == "Monday") {
				i = i + 1 ;
			}
			else {
				i = i + 7; //because saturday is the last day in the week
			}
		}
		else if (z == 0) {
			if (d == k) {
				leds[calendar_leds[i]] = actualday_color;
			}
			else {
				leds[calendar_leds[i]] = weekend_color;
			}
			if (config.FirstWeekDay == "Monday") {
				i = i + 7 ; //because sunday is the last day in the week
			}
			else {
				i = i + 1;
			}
		}
		else {
			if (d == k) {
				leds[calendar_leds[i]] = actualday_color;
			}
			else {
				leds[calendar_leds[i]] = weekday_color;
			}
			i = i + 1;
		}
		if (i == d) {
			leds[calendar_leds[i]] = actualday_color;
		}
	}
	leds[calendar_months[m - 1]] = month_color;
	// check important days
	int numHolidays = (sizeof(holidays) / sizeof(holidays[0]));
	for (int i = 0; i < numHolidays; i += 2) {
		if (holidays[i] == m && holidays[i+1] == d) {
			leds[holiday_led] = weekday_color;
		}
	}
	int numAnniversaries = (sizeof(anniversaries) / sizeof(anniversaries[0]));
	for (int i = 0; i < numAnniversaries; i += 2) {
		if (anniversaries[i] == m && anniversaries[i+1] == d) {
			leds[anniversary_led] = actualday_color;
		}
	}
}

void pride()
{
	static uint16_t sPseudotime = 0;
	static uint16_t sLastMillis = 0;
	static uint16_t sHue16 = 0;

	uint8_t sat8 = beatsin88( 87, 220, 250);
	uint8_t brightdepth = beatsin88( 341, 96, 224);
	uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
	uint8_t msmultiplier = beatsin88(147, 23, 60);

	uint16_t hue16 = sHue16;//gHue * 256;
	uint16_t hueinc16 = beatsin88(113, 1, 3000);

	uint16_t ms = millis();
	uint16_t deltams = ms - sLastMillis ;
	sLastMillis  = ms;
	sPseudotime += deltams * msmultiplier;
	sHue16 += deltams * beatsin88( 400, 5,9);
	uint16_t brightnesstheta16 = sPseudotime;

	for( uint16_t i = 0 ; i < NUM_LEDS; i++) {
		hue16 += hueinc16;
		uint8_t hue8 = hue16 / 256;

		brightnesstheta16  += brightnessthetainc16;
		uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

		uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
		uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
		bri8 += (255 - brightdepth);

		CRGB newcolor = CHSV( hue8, sat8, bri8);

		uint16_t pixelnumber = i;
		pixelnumber = (NUM_LEDS-1) - pixelnumber;

		nblend( leds[pixelnumber], newcolor, 64);
	}
}

void setup() {
	Serial.begin(115200);
	//**** Network Config load
	EEPROM.begin(512); // define an EEPROM space of 512Bytes to store data
	CFG_saved = ReadConfig();

	//  Connect to WiFi acess point or start as Acess point
	if (CFG_saved)  //if no configuration yet saved, load defaults
	{
		// Connect the ESP8266 to local WIFI network in Station mode
		Serial.println("Booting");
		//printConfig();
		WiFi.mode(WIFI_STA);
		WiFi.begin(config.ssid.c_str(), config.password.c_str());
		WIFI_connected = WiFi.waitForConnectResult();
		if (WIFI_connected != WL_CONNECTED)
			Serial.println("Connection Failed! activating the AP mode...");

		Serial.print("Wifi ip:");
		Serial.println(WiFi.localIP());
	}

	if ((WIFI_connected != WL_CONNECTED) or !CFG_saved) {
		// DEFAULT CONFIG
		Serial.println("Setting AP mode default parameters");
		config.ssid = "PerpetualCalendar-" + String(ESP.getChipId(), HEX); // SSID of access point
		config.password = "";
		config.dhcp = true;
		config.IP[0] = 192;
		config.IP[1] = 168;
		config.IP[2] = 1;
		config.IP[3] = 100;
		config.Netmask[0] = 255;
		config.Netmask[1] = 255;
		config.Netmask[2] = 255;
		config.Netmask[3] = 0;
		config.Gateway[0] = 192;
		config.Gateway[1] = 168;
		config.Gateway[2] = 1;
		config.Gateway[3] = 254;
		config.DeviceName = "Perpetual Calendar";
		config.ntpServerName = "0.europe.pool.ntp.org"; // to be adjusted to PT ntp.ist.utl.pt
		config.Update_Time_Via_NTP_Every = 3;
		config.timeZone = 20;
		config.isDayLightSaving = true;
		config.FirstWeekDay = "Sunday";
		//WriteConfig();
		WiFi.mode(WIFI_AP);
		WiFi.softAP(config.ssid.c_str(),"admin1234");
		Serial.print("Wifi ip:");
		Serial.println(WiFi.softAPIP());
	}

	// Start HTTP Server for configuration
	server.on("/", []() {
		Serial.println("admin.html");
		server.send_P ( 200, "text/html", PAGE_AdminMainPage); // const char top of page
	});

	server.on("/favicon.ico", []() {
		Serial.println("favicon.ico");
		server.send( 200, "text/html", "" );
	});
	// Network config
	server.on("/config.html", send_network_configuration_html);
	// Info Page
	server.on("/info.html", []() {
		Serial.println("info.html");
		server.send_P ( 200, "text/html", PAGE_Information );
	});
	server.on("/ntp.html", send_NTP_configuration_html);
	server.on("/time.html", send_Time_Set_html);
	server.on("/style.css", []() {
		Serial.println("style.css");
		server.send_P ( 200, "text/plain", PAGE_Style_css );
	});
	server.on("/microajax.js", []() {
		Serial.println("microajax.js");
		server.send_P ( 200, "text/plain", PAGE_microajax_js );
	});
	server.on("/admin/values", send_network_configuration_values_html);
	server.on("/admin/connectionstate", send_connection_state_values_html);
	server.on("/admin/infovalues", send_information_values_html);
	server.on("/admin/ntpvalues", send_NTP_configuration_values_html);
	server.on("/admin/timevalues", send_Time_Set_values_html);
	server.onNotFound([]() {
		Serial.println("Page Not Found");
		server.send ( 400, "text/html", "Page not Found" );
	});
	server.begin();
	Serial.println("HTTP server started");

	printConfig();

	// start internal time update ISR
	tkSecond.attach(1, ISRsecondTick);

	// tell FastLED about the LED strip configuration
	FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
	FastLED.setBrightness(BRIGHTNESS);
	Serial.println("FastLed Setup done");

	// start internal time update ISR
	tkSecond.attach(1, ISRsecondTick);
}

// the loop function runs over and over again forever
void loop() {
	server.handleClient();
	if (config.Update_Time_Via_NTP_Every > 0) {
		if (cNTP_Update > 5 && firstStart) {
			getNTPtime();
			delay(1500); //wait for DateTime
			cNTP_Update = 0;
			firstStart = false;
		}
		else if (cNTP_Update > (config.Update_Time_Via_NTP_Every * 60)) {
			getNTPtime();
			cNTP_Update = 0;
		}
	}
	//  feed de DOG :)
	customWatchdog = millis();

	//============================
	if (WIFI_connected != WL_CONNECTED and manual_time_set == false) {
		config.Update_Time_Via_NTP_Every = 0;
		//display_led_no_wifi
		leds[warning_led] = CRGB(128,0,0);
		FastLED.show();
	} else if (ntp_response_ok == false and manual_time_set == false) {
		config.Update_Time_Via_NTP_Every = 1;
		//display_animation_no_ntp
		pride();
		FastLED.show();
	} else if (ntp_response_ok == true or manual_time_set == true) {
		if (temp_hour != DateTime.hour or temp_hour == 0) {
			temp_hour = DateTime.hour;
			CalendarDisplay(DateTime.year, DateTime.month, DateTime.day);
			FastLED.show();
		}
	}
}


