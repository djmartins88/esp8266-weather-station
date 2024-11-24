bit of a wild goose chase but eventually got it to be plug'n'play!

I bougth this TZT ESP8266 Weather Station Kit Temperature Humidity OLED Display Module but it didn't have the instructions, duh!

I started of with this link https://www.tztstore.com/goods/show-6305.html
then after scouraging the internet found https://www.uctronics.com/download/Amazon/K0061_ESP8266_KIT.pdf?srsltid=AfmBOoqtBHqp5Zyc5du5n9BKOyb52yQWCyi1KaCf3wWV1sgF9AURfCEp which lead to https://github.com/supprot/ArduCAM_esp8266-dht-thingspeak-logger that must have been using different components because the readings were all wrong, but I had already been able to compile it and implement my specific logic, so the project kept going on from this original example code.

Next adaptations where sourced from a much better source codes examples, but yet incomplete guide on this https://github.com/sunrobotics/Ideaspark_Weather_Station_Kit/blob/master/2_Source_Code/WeatherStationDemo/WeatherStationDemo.ino repo.
After getting it all to work, I realized it was kind of dumb to be reading humidity from DHT and parsing it with low-level code copied from the repo, where we were already using Adafruit, and by this time I started to make the connection that Adafruit librares have very good sensor reading/implementation, so I upgraded the version with https://github.com/adafruit/DHT-sensor-library/blob/master/examples/DHT_Unified_Sensor/DHT_Unified_Sensor.ino

I eventually disabled Atmospheric pressure, but the skelleton code is still there, if needed in the future.

## Update in code:
```
const char* SSID = "NAUGHTY";                        > your wifi ssid
const char* PASSWORD = "VERYGOODBOY";                > your wifi password
const char* THINGSPEAK_API_KEY = "DONTFOMOONCRYPTO"; > use your API key
```
