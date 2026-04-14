#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <HTTPClient.h>
#include <time.h>
#include <Preferences.h>
#include <SPI.h>

String current_ssid = "YOUR_SSID"; 
String current_password = "YOUR_PASSWORD";

int wifiSetupState = 0; 
String tempSSID = "";

#define BOTtoken "YOUR_BOT_TOKEN"
#define CHAT_ID_BF "YOUR_CHAT_ID"
String CHAT_ID_GF = "";

String owm_key = "WEATHER_API_KEY"; 

#define TOUCH_PIN 4
#define SCK_PIN   5
#define MOSI_PIN  6
#define CS_PIN    7
#define DC_PIN    8
#define RES_PIN   9
#define BUSY_PIN  10

//I'M USING WEACT STUDIO WHITE-BLACK-RED 4.2 E-INK DISPLAY FROM ALIEXPRESS
//FOR HARDWARE SETUP TROUBLESHOOTING PLEASE REFER TO THEIR GITHUB
GxEPD2_3C<GxEPD2_420c_GDEY042Z98, GxEPD2_420c_GDEY042Z98::HEIGHT> display(GxEPD2_420c_GDEY042Z98(CS_PIN, DC_PIN, RES_PIN, BUSY_PIN));
U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
Preferences settings; 

// DEEP SLEEP CONFIG
unsigned long lastActivityTime = 0; 
const unsigned long SLEEP_TIMEOUT = 60000;
const uint64_t WAKEUP_INTERVAL_MINUTES = 30;

unsigned long lastTimeBotRan;
unsigned long lastMessage;
const unsigned long botCheckDelay = 1000; 

unsigned long lastTouchTime = 0; 
const unsigned long touchCooldown = 30000; 

//DEFAULT VALUES - PLEASE CHANGE ACCORDINGLY
String lastNormalMessage = "Czekam na wiadomosc...";
int annivYear = 2000; 
int annivMonth = 10; 
int annivDay = 10;
int bdayMonth = 10;    
int bdayDay = 10;   

// WEATHER AND WIDGET SETUP
bool showWeather1 = true;
// 0=WEATHER, 1=CALENDAR, 2=COUNTDOWN, 3=EMPTY
int bottomWidget = 0;
//DEFAULT VALUES - PLEASE CHANGE ACCORDINGLY
String city1_name = "Cieplewo";
String city1_query = "Cieplewo,PL";
String city2_name = "Gdynia";
String city2_query = "Gdynia,PL";

//DEFAULT VALUES - PLEASE CHANGE ACCORDINGLY
int countYear = 2026; 
int countMonth = 1; 
int countDay = 1;
String countDesc = "Wydarzenie";

//TIME AND DATE VARIABLES
String currentTimeStr = "--:--";
String currentDateStr = "--.--.----";
long daysTogether = 0; 
long daysLeftCountdown = 0;
int currentYear = 2024;
int currentHour = 12;  
int currentMonth = 1;  
int currentDay = 1;    

struct WeatherData { 
  String temp; 
  String desc; 
  String icon; 
};

//AUX FUNCTIONS
String removePolishChars(String text) {
  text.replace("ą", "a"); 
  text.replace("Ą", "A"); 
  text.replace("ć", "c"); 
  text.replace("Ć", "C");
  text.replace("ę", "e"); 
  text.replace("Ę", "E"); 
  text.replace("ł", "l"); 
  text.replace("Ł", "L");
  text.replace("ń", "n"); 
  text.replace("Ń", "N"); 
  text.replace("ó", "o"); 
  text.replace("Ó", "O");
  text.replace("ś", "s"); 
  text.replace("Ś", "S"); 
  text.replace("ź", "z"); 
  text.replace("Ź", "Z");
  text.replace("ż", "z"); 
  text.replace("Ż", "Z");
  return text;
}

int getDaysInMonth(int m, int y) {
  if (m == 2) return ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) ? 29 : 28;
  if (m == 4 || m == 6 || m == 9 || m == 11) return 30;
  return 31;
}

//GREETING SECTION
struct GreetingPair { 
  String line1; 
  String line2; 
};

//YOU MAY ADD YOUR GREETINGS - THESE ARE ONLY THE EXAMPLES - AI GENERATED
GreetingPair morningPool[] = {
  {"Dzień dobry,", "Kochanie! :)"}, {"Witaj z rana,", "Misia! <3"},
  {"Miłego dnia,", "Kiciu! (=^w^=)"}, {"Pobudka,", "Śpioszku! <3"},
  {"Czas wstać,", "Piękności! :)"}, {"Uśmiechnij się,", "Skarbie! :*"},
  {"Kawa zrobiona,", "Kochanie?"}, {"Dobrego dnia,", "Moja Miłości! <3"},
  {"Poranne buziaki", "dla Ciebie! :*"}, {"Zacznij dzień", "z uśmiechem! :)"},
  {"Meow meow meow","meow meow! (=^o^=)"}, {"A kto rano wstaje","temu kot mruczy (=^w^=)"},
  {"Elo","żelo!"}
};

GreetingPair afternoonPool[] = {
  {"Miłego", "popołudnia! :)"}, {"Jak mija dzień,", "Kochanie? <3"},
  {"Przesyłam", "buziaczki! :*"}, {"Uśmiechnij się,", "Skarbie! :)"},
  {"Tęsknię za", "Tobą! <3"}, {"Chwilka przerwy,", "Kochanie? :*"},
  {"Cudownego", "popołudnia! <3"}, {"Wracaj szybko", "do mnie! <3"},
  {"Myślę o Tobie", "bez przerwy! :*"}, {"Meow meow meow","meow meow! (=^o^=)"}
};

GreetingPair eveningPool[] = {
  {"Dobry wieczór,", "Kochanie! :)"}, {"Czas na relaks,", "Skarbie! <3"},
  {"Myślę o Tobie", "tego wieczoru..."}, {"Odpocznij już,", "Kochanie! <3"},
  {"Przytulam mocno", "na odległość! :*"}, {"Udanego", "wieczoru! :)"},
  {"Tęsknię za", "Tobą! :*"}, {"Cieplutkiego", "wieczoru! :)"},
  {"Jesteś moim", "szczęściem! <3"}, {"Meow meow meow","meow meow! (=^v^=)"}
};

GreetingPair nightPool[] = {
  {"Spokojnej nocy,", "Kochanie! :)"}, {"Słodkich snów,", "Kiciu! (=^w^=)"},
  {"Śpij dobrze,", "Skarbie! :*"}, {"Śnij o nas,", "Kochanie! <3"},
  {"Cudownych snów,", "Piękności! <3"}, {"Całusy na", "dobranoc! :*"},
  {"Niech Ci się", "przyśnię! ;)"}, {"Do jutra,", "Skarbie! <3"},
  {"Meow meow meow","meow meow! (=^_^=)"}, {"Jeśli to czytasz","to pora iść spać! :o"}
};

// COUNTING FUNCTIONS
long calculateDaysTogether(tm timeinfo) {
  struct tm anniv = {0};
  anniv.tm_year = annivYear - 1900; 
  anniv.tm_mon = annivMonth - 1; 
  anniv.tm_mday = annivDay;
  anniv.tm_hour = 0; 
  anniv.tm_min = 0; 
  anniv.tm_sec = 0;

  struct tm today = timeinfo;
  today.tm_hour = 0; 
  today.tm_min = 0; 
  today.tm_sec = 0;

  time_t now = mktime(&today); 
  time_t start = mktime(&anniv);
  
  // +0.5 FOR TIME CHANGE
  return (long)((difftime(now, start) / 86400.0) + 0.5);
}

long calculateCountdown(tm timeinfo) {
  struct tm target = {0};
  target.tm_year = countYear - 1900; 
  target.tm_mon = countMonth - 1; 
  target.tm_mday = countDay;
  target.tm_hour = 0; 
  target.tm_min = 0; 
  target.tm_sec = 0;

  struct tm today = timeinfo;
  today.tm_hour = 0; 
  today.tm_min = 0; 
  today.tm_sec = 0;

  time_t now = mktime(&today); 
  time_t future = mktime(&target);
  
  return (long)((difftime(future, now) / 86400.0) + 0.5);
}
//FETCHING TIME USING NTP
void fetchTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){ return; }
  
  char timeStringBuff[10]; 
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &timeinfo);
  currentTimeStr = String(timeStringBuff);

  char dateStringBuff[20]; 
  strftime(dateStringBuff, sizeof(dateStringBuff), "%d.%m.%Y", &timeinfo);
  currentDateStr = String(dateStringBuff);

  currentYear = timeinfo.tm_year + 1900;
  currentHour = timeinfo.tm_hour;
  currentMonth = timeinfo.tm_mon + 1;
  currentDay = timeinfo.tm_mday;
  
  daysTogether = calculateDaysTogether(timeinfo);
  daysLeftCountdown = calculateCountdown(timeinfo);
}

// GETTING WEATHER WITH OPENWEATHERMAP
WeatherData fetchWeather(String queryCity) {
  WeatherData data = {"--", "Brak", "E"}; 
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    //CHANGE LANGUAGE ACCORDINGLY
    String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + queryCity + "&units=metric&lang=pl&appid=" + owm_key;
    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String payload = http.getString();
      DynamicJsonDocument doc(2048); deserializeJson(doc, payload);
      float temp = doc["main"]["temp"]; 
      data.temp = String(temp, 1) + "°C"; 
      const char* desc = doc["weather"][0]["description"]; 
      data.desc = String(desc);
      String iconStr = String(doc["weather"][0]["icon"].as<const char*>());
      if (iconStr.startsWith("01d")) data.icon = "E"; else if (iconStr.startsWith("01n")) data.icon = "A"; 
      else if (iconStr.startsWith("02") || iconStr.startsWith("03") || iconStr.startsWith("04") || iconStr.startsWith("50")) data.icon = "@"; 
      else if (iconStr.startsWith("09") || iconStr.startsWith("10") || iconStr.startsWith("13")) data.icon = "C"; 
      else if (iconStr.startsWith("11")) data.icon = "B"; else data.icon = "E";
    }
    http.end();
  }
  return data;
}
// TEXT WRAPPING TO AVOID CLIPPING
void printWordWrapped(String text, int x, int y, int maxWidth) {
  int currentY = y;
  int lineHeight = u8g2Fonts.getFontAscent() - u8g2Fonts.getFontDescent() + 4;
  int startParagraph = 0;
  while (startParagraph < text.length()) {
    int newlineIndex = text.indexOf('\n', startParagraph);
    if (newlineIndex == -1) newlineIndex = text.length();
    String paragraph = text.substring(startParagraph, newlineIndex);
    String currentLine = ""; int startIndex = 0;
    while (startIndex < paragraph.length()) {
      int spaceIndex = paragraph.indexOf(' ', startIndex);
      if (spaceIndex == -1) spaceIndex = paragraph.length();
      String word = paragraph.substring(startIndex, spaceIndex);
      String testLine = currentLine + (currentLine == "" ? "" : " ") + word;
      int lineWidth = u8g2Fonts.getUTF8Width(testLine.c_str());
      if (lineWidth > maxWidth && currentLine != "") {
        u8g2Fonts.setCursor(x, currentY); 
        u8g2Fonts.print(currentLine);
        currentLine = word; 
        currentY += lineHeight;
      } else { currentLine = testLine; }
      startIndex = spaceIndex + 1;
    }
    if (currentLine != "") { 
      u8g2Fonts.setCursor(x, currentY); 
      u8g2Fonts.print(currentLine); 
      currentY += lineHeight; 
    }
    startParagraph = newlineIndex + 1;
  }
}

// MAIN FUNCTION TO REFRESH SCREEN
void showOnScreen(String messageText) {
  fetchTime();   

  time_t now;
  time(&now);
  long lastCharge = settings.getLong("last_charge", (long)now); // Domyślnie "teraz", jeśli brak w pamięci
  int daysSinceCharge = (now - lastCharge) / 86400; // 86400 to ilość sekund w dobie

  bool needsCharging = (daysSinceCharge >= 40);
  if (needsCharging) {
     bot.sendMessage(CHAT_ID_BF, "⚠️ Mój poziom energii spada! Podłącz mnie do ładowania i wpisz /naladowano.", "");
     if (CHAT_ID_GF != "") bot.sendMessage(CHAT_ID_GF, "⚠️ Mój poziom energii spada! Podłącz mnie do ładowania i wpisz /naladowano.", "");

  } 
  
  WeatherData pogoda1; 
  WeatherData pogoda2;
  if(showWeather1) pogoda1 = fetchWeather(city1_query);
  if(bottomWidget == 0) pogoda2 = fetchWeather(city2_query);

  String greeting1 = ""; 
  String greeting2 = "";
  bool isBirthday = (bdayMonth != 0 && currentMonth == bdayMonth && currentDay == bdayDay);
  //GET RANDOM GREETING
  if (isBirthday) {
    greeting1 = "Wszystkiego"; 
    greeting2 = "najlepszego! <3";
  } else {
    if (currentHour >= 5 && currentHour < 12) {
      int poolSize = sizeof(morningPool) / sizeof(morningPool[0]); 
      int r = random(0, poolSize);
      greeting1 = morningPool[r].line1; 
      greeting2 = morningPool[r].line2;
    } else if (currentHour >= 12 && currentHour < 18) {
      int poolSize = sizeof(afternoonPool) / sizeof(afternoonPool[0]); 
      int r = random(0, poolSize);
      greeting1 = afternoonPool[r].line1; 
      greeting2 = afternoonPool[r].line2;
    } else if (currentHour >= 18 && currentHour < 23) {
      int poolSize = sizeof(eveningPool) / sizeof(eveningPool[0]); 
      int r = random(0, poolSize);
      greeting1 = eveningPool[r].line1; 
      greeting2 = eveningPool[r].line2;
    } else {
      int poolSize = sizeof(nightPool) / sizeof(nightPool[0]); 
      int r = random(0, poolSize);
      greeting1 = nightPool[r].line1; 
      greeting2 = nightPool[r].line2;
    }
  }

  Serial.println("Rozpoczynam rysowanie na E-Ink...");
  display.init(115200, true, 50, false);
  display.setRotation(0); 
  u8g2Fonts.begin(display); 
  u8g2Fonts.setFontMode(1); 
  u8g2Fonts.setFontDirection(0);
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE); 
  
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE); 
    display.fillRect(210, 10, 2, 280, GxEPD_BLACK); 
    
    // HEADER AND BIRTHDAY CAKE
    if (display.epd2.hasColor) u8g2Fonts.setForegroundColor(GxEPD_RED); 
    else u8g2Fonts.setForegroundColor(GxEPD_BLACK);
    u8g2Fonts.setFont(u8g2_font_helvB14_te); 
    u8g2Fonts.setCursor(10, 25); 
    u8g2Fonts.print(greeting1); 
    u8g2Fonts.setCursor(10, 45); 
    u8g2Fonts.print(greeting2);

    if (isBirthday) {
      int cx = 155; int cy = 40;  
      display.fillRect(cx, cy, 30, 10, GxEPD_BLACK); 
      display.fillRect(cx+5, cy-10, 20, 10, GxEPD_BLACK); 
      display.fillRect(cx+14, cy-18, 2, 8, GxEPD_BLACK);  
      if (display.epd2.hasColor) display.fillCircle(cx+15, cy-21, 3, GxEPD_RED); 
      else display.fillCircle(cx+15, cy-21, 3, GxEPD_BLACK); 
    } 
    
    // MESSAGE FROM TELEGRAM
    u8g2Fonts.setForegroundColor(GxEPD_BLACK); 
    u8g2Fonts.setFont(u8g2_font_helvR14_te); 
    printWordWrapped(messageText, 10, 65, 190);

    // ANNIVERSARY COUNTER
    if (display.epd2.hasColor) u8g2Fonts.setForegroundColor(GxEPD_RED);
    u8g2Fonts.setFont(u8g2_font_helvR14_te); 
    u8g2Fonts.setCursor(10, 205); 
    u8g2Fonts.print("Jesteśmy razem:");
    u8g2Fonts.setForegroundColor(GxEPD_BLACK); 
    u8g2Fonts.setFont(u8g2_font_helvB18_te); 
    u8g2Fonts.setCursor(10, 228); 
    u8g2Fonts.print(String(daysTogether) + " dni ");

    // TIME
    if (display.epd2.hasColor) u8g2Fonts.setForegroundColor(GxEPD_RED);
    u8g2Fonts.setFont(u8g2_font_helvR14_te); 
    u8g2Fonts.setCursor(10, 250); 
    u8g2Fonts.print("Dostarczono:");
    u8g2Fonts.setForegroundColor(GxEPD_BLACK); 
    u8g2Fonts.setFont(u8g2_font_helvB18_te); 
    u8g2Fonts.setCursor(10, 270); 
    u8g2Fonts.print(currentTimeStr);
    u8g2Fonts.setFont(u8g2_font_helvR14_te); 
    u8g2Fonts.setCursor(10, 285); 
    u8g2Fonts.print(currentDateStr);

    //TOP RIGHT SIDE - WEATHER 1
    int rightX = 225; 
    if(showWeather1) {
      if (display.epd2.hasColor) u8g2Fonts.setForegroundColor(GxEPD_RED);
      u8g2Fonts.setFont(u8g2_font_helvB14_te); 
      u8g2Fonts.setCursor(rightX, 25);
      if(city1_name == "Gdynia") u8g2Fonts.print("Pogoda u Tomka:");
      else u8g2Fonts.print(city1_name + ":"); // Dynamiczna nazwa! 
      u8g2Fonts.setForegroundColor(GxEPD_BLACK); 
      u8g2Fonts.setFont(u8g2_font_helvB24_te); 
      u8g2Fonts.setCursor(rightX, 55); 
      u8g2Fonts.print(pogoda1.temp);
      u8g2Fonts.setFont(u8g2_font_helvR14_te); 
      printWordWrapped(pogoda1.desc, rightX, 80, 170);
      u8g2Fonts.setFont(u8g2_font_open_iconic_weather_4x_t); 
      u8g2Fonts.setCursor(rightX+120, 65); 
      u8g2Fonts.print(pogoda1.icon);
    }

    //BOTTOM RIGHT SIDE - WIDGETS
    if(bottomWidget == 0) {
      // MODE: WEATHER 2
      if (display.epd2.hasColor) u8g2Fonts.setForegroundColor(GxEPD_RED);
      u8g2Fonts.setFont(u8g2_font_helvB14_te); 
      u8g2Fonts.setCursor(rightX, 120); 
      if(city2_name == "Gdynia") u8g2Fonts.print("Pogoda u Tomka:");
      else u8g2Fonts.print(city2_name + ":");
      u8g2Fonts.setForegroundColor(GxEPD_BLACK); 
      u8g2Fonts.setFont(u8g2_font_helvB24_te); 
      u8g2Fonts.setCursor(rightX, 150); 
      u8g2Fonts.print(pogoda2.temp);
      u8g2Fonts.setFont(u8g2_font_helvR14_te); 
      printWordWrapped(pogoda2.desc, rightX, 175, 170);
      u8g2Fonts.setFont(u8g2_font_open_iconic_weather_4x_t); 
      u8g2Fonts.setCursor(rightX+120, 160); 
      u8g2Fonts.print(pogoda2.icon);
      // MINIMIZED COUNTDOWN
      if (display.epd2.hasColor) u8g2Fonts.setForegroundColor(GxEPD_RED);
      u8g2Fonts.setFont(u8g2_font_helvB12_te); 
      u8g2Fonts.setCursor(rightX, 225); 
      u8g2Fonts.print(countDesc + ":");
      
      u8g2Fonts.setForegroundColor(GxEPD_BLACK); 
      u8g2Fonts.setFont(u8g2_font_helvB18_te); 
      u8g2Fonts.setCursor(rightX, 250); 
      if (daysLeftCountdown > 1) u8g2Fonts.print(String(daysLeftCountdown) + " dni");
      else if (daysLeftCountdown == 1) u8g2Fonts.print(String(daysLeftCountdown) + " dzień");
      else if (daysLeftCountdown == 0) u8g2Fonts.print("To już dzisiaj!");
      else u8g2Fonts.print("Minęło!");
    } 
    else if (bottomWidget == 1) {
      // MODE: CALENDAR
      if (display.epd2.hasColor) u8g2Fonts.setForegroundColor(GxEPD_RED);
      u8g2Fonts.setFont(u8g2_font_helvB14_te); 
      u8g2Fonts.setCursor(rightX, 120); 
      u8g2Fonts.print("Kalendarz:");
      
      u8g2Fonts.setForegroundColor(GxEPD_BLACK);
      u8g2Fonts.setFont(u8g2_font_helvB10_te);
      
      // CALCULATING FIRST DAY OF MONTH
      struct tm firstDay = {0};
      firstDay.tm_year = currentYear - 1900; 
      firstDay.tm_mon = currentMonth - 1; 
      firstDay.tm_mday = 1;
      mktime(&firstDay);
      int startWday = (firstDay.tm_wday == 0) ? 6 : firstDay.tm_wday - 1; // MONDAY IS DAY 0
      
      const char* days[] = {"Pn", "Wt", "Sr", "Cz", "Pt", "Sb", "Nd"};
      for(int i=0; i<7; i++) {
        u8g2Fonts.setCursor(rightX + (i*24), 140);
        u8g2Fonts.print(days[i]);
      }
      
      int currY = 160; int col = startWday;
      for(int d = 1; d <= getDaysInMonth(currentMonth, currentYear); d++) {
        int drawX = rightX + (col * 24);
        
        // RED BOX FOR TODAY
        if (d == currentDay) {
          if (display.epd2.hasColor) {
            display.fillRoundRect(drawX-1, currY-12, 18, 14, 2, GxEPD_RED);
            u8g2Fonts.setBackgroundColor(GxEPD_RED);
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
          } else {
            display.fillRoundRect(drawX-1, currY-12, 18, 14, 2, GxEPD_BLACK);
            u8g2Fonts.setBackgroundColor(GxEPD_BLACK);
            u8g2Fonts.setForegroundColor(GxEPD_WHITE);
          }
        } else {
          u8g2Fonts.setForegroundColor(GxEPD_BLACK);
        }
        
        if(d < 10) u8g2Fonts.setCursor(drawX + 3, currY);
        else u8g2Fonts.setCursor(drawX, currY);
        
        u8g2Fonts.print(d);
        u8g2Fonts.setBackgroundColor(GxEPD_WHITE);
        
        col++;
        if(col > 6) { col = 0; currY += 18; }
      }
      u8g2Fonts.setForegroundColor(GxEPD_BLACK);

      // COMPACT MINIMIZED COUNTDOWN
      if (display.epd2.hasColor) u8g2Fonts.setForegroundColor(GxEPD_RED);
      else u8g2Fonts.setForegroundColor(GxEPD_BLACK);
      
      u8g2Fonts.setFont(u8g2_font_helvB10_te); 
      u8g2Fonts.setCursor(rightX, 275); 
      u8g2Fonts.print(countDesc + ":");
      
      u8g2Fonts.setForegroundColor(GxEPD_BLACK);
      u8g2Fonts.setFont(u8g2_font_helvB14_te); 
      u8g2Fonts.setCursor(rightX, 292); 
      if (daysLeftCountdown > 1) u8g2Fonts.print(String(daysLeftCountdown) + " dni");
      else if (daysLeftCountdown == 1) u8g2Fonts.print(String(daysLeftCountdown) + " dzień");
      else if (daysLeftCountdown == 0) u8g2Fonts.print("To już dzisiaj!");
      else u8g2Fonts.print("Minęło!");
    }
    else if (bottomWidget == 2) {
      // MODE: FULL COUNTDOWN
      if (display.epd2.hasColor) u8g2Fonts.setForegroundColor(GxEPD_RED);
      u8g2Fonts.setFont(u8g2_font_helvB14_te); 
      u8g2Fonts.setCursor(rightX, 120); 
      u8g2Fonts.print("Odliczamy:");
      
      u8g2Fonts.setForegroundColor(GxEPD_BLACK);
      u8g2Fonts.setFont(u8g2_font_helvB24_te); 
      u8g2Fonts.setCursor(rightX, 155); 
      
      if (daysLeftCountdown > 1) u8g2Fonts.print(String(daysLeftCountdown) + " dni");
      else if (daysLeftCountdown == 1) u8g2Fonts.print(String(daysLeftCountdown) + " dzień");
      else if (daysLeftCountdown == 0) {
        u8g2Fonts.print("To już dzisiaj!");
      } else {
        u8g2Fonts.print("Minęło!");
      }

      u8g2Fonts.setFont(u8g2_font_helvR14_te); 
      printWordWrapped(countDesc, rightX, 185, 170);
    }

  } while (display.nextPage());
  
  display.hibernate();
  Serial.println("Rysowanie zakończone!");
}

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TOUCH_PIN, INPUT_PULLDOWN);

  SPI.begin(SCK_PIN, -1, MOSI_PIN, CS_PIN);

  lastActivityTime = millis();

  randomSeed(esp_random());

  settings.begin("lovebox", false);
  CHAT_ID_GF = settings.getString("gf_id", CHAT_ID_BF);
  current_ssid = settings.getString("wifi_ssid", current_ssid);
  current_password = settings.getString("wifi_pass", current_password);
  lastNormalMessage = settings.getString("ostatni_tekst", "Czekam na wiadomosc..."); 
  annivYear = settings.getInt("anniv_y", 2024); 
  annivMonth = settings.getInt("anniv_m", 10); 
  annivDay = settings.getInt("anniv_d", 9);
  bdayMonth = settings.getInt("bday_m", 5);     
  bdayDay = settings.getInt("bday_d", 7);
  
  showWeather1 = settings.getBool("showW1", true);
  bottomWidget = settings.getInt("botWid", 0);
  city1_name = settings.getString("c1_name", "Cieplewo"); 
  city1_query = settings.getString("c1_query", "Cieplewo,PL");
  city2_name = settings.getString("c2_name", "Gdańsk");   
  city2_query = settings.getString("c2_query", "Gdansk,PL");
  
  countYear = settings.getInt("cY", 2026); 
  countMonth = settings.getInt("cM", 6); 
  countDay = settings.getInt("cD", 29);
  countDesc = settings.getString("cDesc", "Wakacje!");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(current_ssid.c_str(), current_password.c_str());

  Serial.print("Łączenie z WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nPołączono! IP: " + WiFi.localIP().toString());

  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
  
  Serial.print("Czekam na czas z internetu");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo, 2000)) { Serial.print("."); }
  Serial.println("\nCzas pobrany pomyślnie!");
  
  client.setInsecure(); 
  Serial.println("Bot gotowy do pracy.");
  
  showOnScreen(lastNormalMessage); 
}

void loop() {
  int touchState = digitalRead(TOUCH_PIN);
  if (touchState == HIGH) {

    if (millis() - lastTouchTime > touchCooldown) {
      Serial.println("PRZYCISK DOTKNIĘTY! Rysuję ekran i nakładam blokadę na 6s.");
      lastTouchTime = millis();

      bot.sendMessage(CHAT_ID_BF, "Twoja dziewczyna właśnie o Tobie pomyślała!", "");

      showOnScreen(lastNormalMessage); 
    } else {
      Serial.println("Ignoruję dotyk (Cooldown aktywny).");
    }
  }


  if (millis() > lastTimeBotRan + botCheckDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    

    while(numNewMessages) {
      for (int i=0; i<numNewMessages; i++) {
        String chat_id = String(bot.messages[i].chat_id);
        String received_text = bot.messages[i].text;

        if (chat_id == CHAT_ID_BF){
          if (received_text.startsWith("/dodaj_id ")) {
            String newId = received_text.substring(10); newId.trim();
            CHAT_ID_GF = newId; settings.putString("gf_id", CHAT_ID_GF);
            bot.sendMessage(chat_id, "Dodano uprawnienia dla ID: " + CHAT_ID_GF, "");
            bot.sendMessage(CHAT_ID_GF, "Zostalas dodana do systemu! Mozesz teraz sterowac LoveBoxem.", "");
            continue;
          }
          else if (received_text == "/usun_id") {
            CHAT_ID_GF = ""; settings.putString("gf_id", "");
            bot.sendMessage(chat_id, "Usunieto dostep dla drugiego uzytkownika.", "");
            continue;
          }
          
        }
        
        if (chat_id == CHAT_ID_BF || (CHAT_ID_GF != "" && chat_id == CHAT_ID_GF)) {

          if (received_text == "/anuluj" && wifiSetupState != 0) {
            wifiSetupState = 0;
            bot.sendMessage(chat_id, "Zmiana WiFi anulowana.", "");
            continue;
          }

          if (wifiSetupState == 1) {
            tempSSID = received_text;
            wifiSetupState = 2;
            bot.sendMessage(chat_id, "Podaj haslo dla sieci: " + tempSSID, "");
            continue; 
          } 
          else if (wifiSetupState == 2) {
            String tempPASS = received_text;
            wifiSetupState = 0;
            bot.sendMessage(chat_id, "Probuje polaczyc z " + tempSSID + "...", "");
            
            WiFi.disconnect();
            delay(500);
            WiFi.begin(tempSSID.c_str(), tempPASS.c_str());
            
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 30) {
              delay(500);
              attempts++;
            }
            
            if (WiFi.status() == WL_CONNECTED) {
              current_ssid = tempSSID;
              current_password = tempPASS;
              settings.putString("wifi_ssid", current_ssid);
              settings.putString("wifi_pass", current_password);
              bot.sendMessage(chat_id, "Polaczono pomyslnie z nowa siecia!", "");
            } else {
              WiFi.disconnect();
              delay(500);
              WiFi.begin(current_ssid.c_str(), current_password.c_str());
              while (WiFi.status() != WL_CONNECTED) { delay(500); }
              bot.sendMessage(chat_id, "Nie udalo sie polaczyc. Wrocono do poprzedniej sieci.", "");
            }
            continue;
          }

          if (received_text == "/wifi") {
            wifiSetupState = 1;
            bot.sendMessage(chat_id, "Podaj dokladna nazwe (SSID) nowej sieci WiFi (lub wpisz /anuluj):", "");
          }

          else if (received_text == "/naladowano") {
            time_t now;
            time(&now);
            settings.putLong("last_charge", (long)now);
            bot.sendMessage(chat_id, "Licznik baterii wyzerowany! LoveBox jest w pełni naładowany lub się ładuje.", "");
            showOnScreen(lastNormalMessage);
          }
          
          else if (received_text == "/pogoda1_wylacz") {
            showWeather1 = false; 
            settings.putBool("showW1", false); 
            bot.sendMessage(chat_id, "Pogoda (Góra) wyłączona!", "");
            showOnScreen(lastNormalMessage); 
          } 
          else if (received_text == "/pogoda1_wlacz") {
            showWeather1 = true; 
            settings.putBool("showW1", true); 
            bot.sendMessage(chat_id, "Pogoda (Góra) włączona!", "");
            showOnScreen(lastNormalMessage);
          }
          else if (received_text == "/pogoda2_wylacz" || received_text == "/widget pusty") {
            bottomWidget = 3; 
            settings.putInt("botWid", 3); 
            bot.sendMessage(chat_id, "Dolny widget pusty!", "");
            showOnScreen(lastNormalMessage); 
          } 
          else if (received_text == "/pogoda2_wlacz" || received_text == "/widget pogoda") {
            bottomWidget = 0; 
            settings.putInt("botWid", 0); 
            bot.sendMessage(chat_id, "Widget Pogoda ustawiony!", "");
            showOnScreen(lastNormalMessage);
          }
          else if (received_text == "/widget kalendarz") {
            bottomWidget = 1; 
            settings.putInt("botWid", 1); 
            bot.sendMessage(chat_id, "Widget Kalendarz ustawiony!", "");
            showOnScreen(lastNormalMessage);
          }
          else if (received_text == "/widget odliczanie") {
            bottomWidget = 2; 
            settings.putInt("botWid", 2); 
            bot.sendMessage(chat_id, "Widget Odliczanie przywrócony!", "");
            showOnScreen(lastNormalMessage);
          }
          
          else if (received_text.startsWith("/pogoda1_miasto ")) {
            String newCity = received_text.substring(16); newCity.trim();
            city1_name = newCity; 
            city1_query = removePolishChars(newCity) + ",PL";
            settings.putString("c1_name", city1_name); 
            settings.putString("c1_query", city1_query);
            bot.sendMessage(chat_id, "Miasto pogody górnej zmienione na: " + city1_name, "");
            showOnScreen(lastNormalMessage);
          }
          else if (received_text.startsWith("/pogoda2_miasto ")) {
            String newCity = received_text.substring(16); newCity.trim();
            city2_name = newCity; 
            city2_query = removePolishChars(newCity) + ",PL";
            settings.putString("c2_name", city2_name); 
            settings.putString("c2_query", city2_query);
            bot.sendMessage(chat_id, "Miasto pogody dolnej zmienione na: " + city2_name, "");
            showOnScreen(lastNormalMessage);
          }

          else if (received_text.startsWith("/odliczanie ")) {
            String datePart = received_text.substring(12, 22);
            String descPart = received_text.substring(23);
            
            int y = datePart.substring(0, 4).toInt();
            int m = datePart.substring(5, 7).toInt();
            int d = datePart.substring(8, 10).toInt();
            
            if(y > 2000 && m > 0 && m <= 12 && d > 0 && d <= 31) {
              countYear = y; 
              countMonth = m; 
              countDay = d; 
              countDesc = descPart;
              bottomWidget = 2;
              
              settings.putInt("cY", y); 
              settings.putInt("cM", m); 
              settings.putInt("cD", d);
              settings.putString("cDesc", countDesc);
              settings.putInt("botWid", 2);
              
              bot.sendMessage(chat_id, "Odliczanie ustawione do: " + countDesc, "");
              showOnScreen(lastNormalMessage);
            } else {
              bot.sendMessage(chat_id, "Błąd formatu! Użyj: /odliczanie RRRR-MM-DD Opis Wydarzenia", "");
            }
          }

          else if (received_text.startsWith("/rocznica ")) {
            int y, m, d;
            if (sscanf(received_text.c_str(), "/rocznica %d-%d-%d", &y, &m, &d) == 3) {
                annivYear = y; 
                annivMonth = m; 
                annivDay = d;
                settings.putInt("anniv_y", y); 
                settings.putInt("anniv_m", m); 
                settings.putInt("anniv_d", d);
                bot.sendMessage(chat_id, "Data rocznicy ustawiona!", "");
                showOnScreen(lastNormalMessage); 
            } else { bot.sendMessage(chat_id, "Zły format! Użyj: /rocznica RRRR-MM-DD", ""); }
          }
          else if (received_text.startsWith("/urodziny ")) {
            int m, d;
            if (sscanf(received_text.c_str(), "/urodziny %d-%d", &m, &d) == 2) {
                bdayMonth = m; 
                bdayDay = d;
                settings.putInt("bday_m", m); 
                settings.putInt("bday_d", d);
                bot.sendMessage(chat_id, "Data urodzin ustawiona!", "");
                showOnScreen(lastNormalMessage); 
            } else { bot.sendMessage(chat_id, "Zły format! Użyj: /urodziny MM-DD", ""); }
          }
          else if(chat_id == CHAT_ID_BF){
            Serial.println("Od Ciebie: " + received_text);
            if(received_text == lastNormalMessage) continue;
            lastNormalMessage = received_text; 
            settings.putString("ostatni_tekst", lastNormalMessage); 
            showOnScreen(lastNormalMessage); 
            bot.sendMessage(chat_id, "Wiadomość wyświetlona na E-Ink!", "");
          }
          else if (chat_id == CHAT_ID_GF){
            bot.sendMessage(chat_id, "Tylko twój chłopak może ci coś wyświetlić na ekranie :)");
          }
          
          lastMessage = millis();
        }
        else {
          bot.sendMessage(chat_id, "Brak dostepu! Twoj Chat ID to: " + chat_id + "\n\nPodaj ten numer administratorowi urzadzenia.", "");
        }
        

      }
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  if (millis() - lastActivityTime > SLEEP_TIMEOUT) {
    Serial.println("Brak aktywnosci przez 60 sekund. Przechodze w Deep Sleep.");
    
    esp_deep_sleep_enable_gpio_wakeup(1ULL << TOUCH_PIN, ESP_GPIO_WAKEUP_GPIO_HIGH);
    
    esp_sleep_enable_timer_wakeup(WAKEUP_INTERVAL_MINUTES * 60ULL * 1000000ULL);
    
    esp_deep_sleep_start();
  }

}
