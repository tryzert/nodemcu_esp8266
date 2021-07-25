#include <ESP8266WiFi.h>
#include <U8g2lib.h>
// #include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>

// int year = 2021, month = 7, day = 24, hour, minute = 39, second, week = 6;
int year, month, day, hour, minute, second, week;
String weather, temperature;

//连接wifi
void connectWifi()
{
	//wifi名称
	const char *ssid = "ahut-edu-cn";
	//wifi密码
	const char *pw = "hellokitty123";
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, pw);
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.println("Connecting to wifi...");
		delay(1000);
	}
	Serial.println("Success!");
}

//断开wifi
void closeWifi()
{
	WiFi.disconnect();
	Serial.println("Wifi closed!");
}

//网络请求
String httpGet(String url)
{
	String payload;
	WiFiClient wifiClient;
	HTTPClient http;
	http.setTimeout(1000);
	http.begin(wifiClient, url);
	http.addHeader("Content-Type", "application/json");
	int httpCode = http.GET();
	if (httpCode == HTTP_CODE_OK)
	{
		payload = http.getString();
	}
	http.end();
	wifiClient.stop();
	return payload;
}

//获取网络北京时间api
// http://api.k780.com/?app=life.time&appkey=10003&sign=b59bc3ef6191eb9f747dd4e83c99f2a4&format=json
void getInternetTime()
{
	String payload = httpGet("http://api.k780.com/?app=life.time&appkey=10003&sign=b59bc3ef6191eb9f747dd4e83c99f2a4&format=json");
	if (!payload.isEmpty())
	{
		StaticJsonDocument<384> jsonDoc;
		DeserializationError error = deserializeJson(jsonDoc, payload);
		if (error)
		{
			Serial.println("deserializeJson: an error occured!");
			//todo
		}
		else
		{
			String datetime = jsonDoc["result"]["datetime_1"];
			year = datetime.substring(0, 4).toInt();
			month = datetime.substring(5, 7).toInt();
			day = datetime.substring(8, 10).toInt();
			hour = datetime.substring(11, 13).toInt();
			minute = datetime.substring(14, 16).toInt();
			second = datetime.substring(17, 19).toInt();
			String wk = jsonDoc["result"]["week_1"];
			week = wk.toInt();
			if (week == 0)
			{
				week = 7;
			}
		}
	}
	else
	{
		Serial.println("Can't get internet time");
	}
}

//天气api 马鞍山 cityid = 101220501
void getInternetWeather()
{
	String payload = httpGet("http://api.k780.com/?app=weather.today&cityId=101220501&appkey=10003&sign=b59bc3ef6191eb9f747dd4e83c99f2a4&format=json");
	if (!payload.isEmpty())
	{
		StaticJsonDocument<1024> jsonDoc;
		DeserializationError error = deserializeJson(jsonDoc, payload);
		if (error)
		{
			Serial.println("deserializeJson: an error occured!");
			//todo
		}
		else
		{
			weather = String(jsonDoc["result"]["weather_curr"]);
			temperature = String(jsonDoc["result"]["temperature_curr"]);
		}
	}
	else
	{
		Serial.println("Can't get internet weather");
	}
}

//开机页面
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
void drawWelcomePage()
{
	u8g2.clearBuffer();
	// u8g2.setFont(u8g2_font_ncenB08_tr);
	u8g2.setFont(u8g2_font_wqy12_t_gb2312);
	u8g2.setCursor(0, 10);
	u8g2.print("欢迎来到:");
	u8g2.setCursor(10, 25);
	u8g2.print("Node-mcu Esp8266.");
	u8g2.setCursor(0, 40);
	u8g2.print("正在连接wifi...");
	u8g2.setCursor(0, 57);
	u8g2.print("请等待信息更新.");
	u8g2.sendBuffer();
}

//绘制主界面中的日期
void drawDateText()
{
	int x = 5, y = 15;
	const char *w[] = {"日", "一", "二", "三", "四", "五", "六"};
	u8g2.setCursor(x, y);
	u8g2.setFont(u8g2_font_wqy12_t_gb2312);
	u8g2.printf("%d年%02d月%02d日 星期%s", year, month, day, w[week % 7]);
}

//绘制主界面中的时间
void drawTimeText()
{
	int x = 18, y = 40;
	u8g2.setCursor(x, y);
	u8g2.setFont(u8g2_font_ncenB18_tf);
	u8g2.printf("%02d:%02d:%02d\n", hour, minute, second);
}

//绘制主界面中的天气信息
void drawWeatherText()
{
	// u8g2.drawGlyph(x + 30, y, 0x2600); //太阳
	// u8g2.drawGlyph(x + 30, y, 0x2601); //云
	// u8g2.drawGlyph(x + 30, y, 0x2614); //雨伞
	// u8g2.drawGlyph(x + 30, y, 0x2603); //雪人
	// u8g2.drawGlyph(x + 20, y, 0x2618);
	// u8g2.drawGlyph(x + 20, y, 0x2619);
	// u8g2.drawGlyph(x + 20, y, 0x2604);
	u8g2.setFont(u8g2_font_wqy12_t_gb2312);
	u8g2.setCursor(5, 60);
	u8g2.printf("马鞍山 %s %s", weather, temperature);
}

//绘制主界面
void drawHomePage()
{
	u8g2.clearBuffer();
	drawDateText();
	drawTimeText();
	drawWeatherText();
	u8g2.sendBuffer();
}

//判断是否是闰年
bool isLeapYear(int year)
{
	if (!(year % 4) && (year % 100))
		return true;
	if (!(year % 100) && !(year % 400))
		return true;
	return false;
}

//每秒加一
void updateTime()
{
	second++;
	if (second >= 60)
	{
		second = 0;
		minute++;
	}
	if (minute >= 60)
	{
		minute = 0;
		hour++;
	}
	if (hour >= 24)
	{
		hour = 0;
		day++;
	}
	if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12)
	{
		if (day > 31)
		{
			day = 1;
			month++;
			week++;
		}
	}
	else if (month == 2)
	{
		if (isLeapYear(year))
		{
			if (day > 29)
			{
				day = 1;
				month++;
				week++;
			}
		}
		else
		{
			if (day > 28)
			{
				day = 1;
				month++;
				week++;
			}
		}
	}
	if (week > 7)
	{
		week = 1;
	}
	if (month > 12)
	{
		month = 1;
		year++;
	}
}

//清空屏幕，防止屏幕烧坏
void cleanScreen()
{
	u8g2.clear();
}

//定时器
Ticker timeTicker, cleanScreenTicker, getInternetTimeTicker, getInternetWeatherTicker;
void setup()
{
	Serial.begin(115200);
	u8g2.begin();
	u8g2.enableUTF8Print();
	drawWelcomePage();
	connectWifi();
	getInternetTime();
	delay(3000);
	getInternetWeather();
	timeTicker.attach(1, updateTime);
	cleanScreenTicker.attach(7, cleanScreen);
	getInternetTimeTicker.attach(24 * 60 * 60, getInternetTime);
	getInternetWeatherTicker.attach(30 * 60, getInternetWeather);
}

void loop()
{
	drawHomePage();
	delay(1000);
}