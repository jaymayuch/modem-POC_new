/****************************************************************************************************************************
  ESP32_GSM.ino
  For ESP32 to run GSM/GPRS and WiFi simultaneously, using config portal feature
  
  Library to enable GSM/GPRS and WiFi running simultaneously , with WiFi config portal.
  Based on and modified from Blynk library v0.6.1 https://github.com/blynkkk/blynk-library/releases
  Built by Khoi Hoang https://github.com/khoih-prog/BlynkGSM_Manager
  Licensed under MIT license
  Version: 1.2.0
  
  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      17/01/2020 Initial coding. Add config portal similar to Blynk_WM library.
  1.0.1   K Hoang      27/01/2020 Change Synch XMLHttpRequest to Async (https://xhr.spec.whatwg.org/). Reduce code size
  1.0.2   K Hoang      08/02/2020 Enable GSM/GPRS and WiFi running simultaneously
  1.0.3   K Hoang      18/02/2020 Add checksum. Add clearConfigData()
  1.0.4   K Hoang      14/03/2020 Enhance Config Portal GUI. Reduce code size.
  1.0.5   K Hoang      20/03/2020 Add more modem supports. See the list in README.md
  1.0.6   K Hoang      07/04/2020 Enable adding dynamic custom parameters from sketch
  1.0.7   K Hoang      09/04/2020 SSID password maxlen is 63 now. Permit special chars # and % in input data.
  1.0.8   K Hoang      14/04/2020 Fix bug.
  1.0.9   K Hoang      31/05/2020 Update to use LittleFS for ESP8266 core 2.7.1+. Add Configurable Config Portal Title,
                                  Default Config Data and DRD. Add MultiWiFi/Blynk features for WiFi and GPRS/GSM
  1.0.10  K Hoang      26/08/2020 Use MultiWiFi. Auto format SPIFFS/LittleFS for first time usage.
                                  Fix bug and logic of USE_DEFAULT_CONFIG_DATA.
  1.1.0   K Hoang      01/01/2021 Add support to ESP32 LittleFS. Remove possible compiler warnings. Update examples. Add MRD
  1.2.0   K Hoang      01/02/2021 Add functions to control Config Portal (CP) from software or Virtual Switches
                                  Fix CP and Dynamic Params bugs. To permit autoreset after timeout if DRD/MRD or forced CP
 *****************************************************************************************************************************/

#include "defines.h"
#include "Arduino.h"

//#define USE_BLYNK_WM 0
#define UBIDOTS 1
#if USE_BLYNK_WM
  #include "Credentials.h"
  #include "dynamicParams.h"
  
  #define BLYNK_PIN_FORCED_CONFIG           V10
  #define BLYNK_PIN_FORCED_PERS_CONFIG      V20

// Use button V10 (BLYNK_PIN_FORCED_CONFIG) to forced Config Portal
BLYNK_WRITE(BLYNK_PIN_FORCED_CONFIG)
{ 
  if (param.asInt())
  {
    Serial.println( F("\nCP Button Hit. Rebooting") ); 

    // This will keep CP once, clear after reset, even you didn't enter CP at all.
    Blynk.resetAndEnterConfigPortal(); 
  }
}

// Use button V20 (BLYNK_PIN_FORCED_PERS_CONFIG) to forced Persistent Config Portal
BLYNK_WRITE(BLYNK_PIN_FORCED_PERS_CONFIG)
{ 
  if (param.asInt())
  {
    Serial.println( F("\nPersistent CP Button Hit. Rebooting") ); 
   
    // This will keep CP forever, until you successfully enter CP, and Save data to clear the flag.
    Blynk.resetAndEnterConfigPortalPersistent();
  }
}
#endif

void heartBeatPrint()
{
  static int num = 1;

  if (Blynk_WF.connected())
  {
    Serial.print(F("B"));
  }
  else
  {
    Serial.print(F("F"));
  }

  if (Blynk_GSM.connected())
  {
    Serial.print(F("G"));
  }
  else
  {
    Serial.print(F("F"));
  }

  if (num == 40)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(F(" "));
  }
}

void check_status()
{
  static unsigned long checkstatus_timeout = 0;

#define STATUS_CHECK_INTERVAL     60000L

  // Send status report every STATUS_REPORT_INTERVAL (60) seconds: we don't need to send updates frequently if there is no status change.
  if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    // report status to Blynk
    heartBeatPrint();

    checkstatus_timeout = millis() + STATUS_CHECK_INTERVAL;
  }
}

bool valid_apn = false;
bool GSM_CONNECT_OK = false;
BlynkTimer timer; // Create a Timer object called "timer"!
static uint32_t count;
#ifdef UBIDOTS
TinyGsmClient  client(modem);
#endif
uint32_t ch[]={1739,1741,1741,1741,1739,1741,1739,1740,1741,1740,1740,1739,1739,1739,1740,1734,1741,1739,1741,1739,1740,1737,1729,1722,1706,1687,1663,1645,1651,1664,1659,1667,1671,1671,1680,1677,1680,1683,1680,1692,1693,1638,1624,1786,1991,2205,2431,2667,2915,3181,3247,3055,2703,1968,1341,1264,1333,1392,1419,1431,1456,1464,1466,1473,1498,1593,1633,1657,1682,1710,1728,1744,1755,1762,1762,1744,1733,1728,1712,1698,1667,1657,1664,1664,1671,1675,1679,1685,1687,1686,1695,1695,1699,1711,1703,1648,1637,1813,2015,2224,2448,2687,2935,3195,3235,3031,2667,1904,1323,1264,1344,1408,1424,1447,1463,1467,1474,1488,1517,1612,1645,1669,1698,1712,1728,1745,1759,1770,1761,1749,1741,1730,1717,1703,1665,1664,1666,1670,1682,1680,1682,1686,1686,1696,1702,1705,1712,1705,1700,1647,1646,1840,2029,2241,2478,2709,2961,3205,3235,3023,2635,1854,1314,1266,1347,1406,1417,1446,1455,1460,1472,1478,1526,1616,1645,1678,1699,1717,1735,1747,1766,1769,1757,1747,1729,1726,1719,1696,1668,1669,1668,1680,1680,1680,1686,1687,1689,1696,1693,1702,1701,1705,1699,1634,1652,1847,2039,2259,2491,2731,2992,3221,3243,3007,2587,1798,1289,1270,1361,1408,1422,1449,1456,1469,1476,1474,1531,1608,1645,1671,1693,1718,1730,1743,1765,1762,1755,1744,1728,1728,1712,1691,1671,1664,1671,1680,1673,1682,1686,1684,1699,1695,1700,1704,1698,1711,1694,1638,1680,1871,2069,2288,2511,2750,3010,3223,3231,2985,2535,1738,1274,1280,1360,1405,1426,1450,1459,1466,1473,1487,1535,1615,1650,1679,1699,1714,1728,1747,1766,1759,1756,1744,1724,1728,1712,1694,1667,1662,1675,1670,1680,1685,1683,1689,1696,1697,1707,1703,1705,1703,1681,1637,1679,1874,2079,2293,2519,2767,3024,3239,3217,2949,2481,1680,1266,1286,1360,1410,1427,1441,1458,1462,1472,1478,1535,1614,1648,1669,1698,1712,1728,1746,1758,1761,1747,1733,1730,1722,1705,1683,1658,1661,1664,1666,1679,1680,1680,1687,1690,1701,1702,1698,1709,1711,1685,1632,1684,1903,2097,2309,2547,2782,3047,3247,3199,2933,2432,1629,1264,1293,1366,1414,1424,1450,1461,1470,1482,1486,1554,1617,1648,1680,1709,1718,1742,1753,1771,1773,1759,1749,1739,1728,1715,1690,1669,1667,1670,1681,1684,1680,1687,1690,1693,1701,1699,1702,1707,1707,1683,1631,1708,1915,2113,2337,2559,2803,3071,3249,3187,2911,2375,1587,1262,1296,1378,1415,1429,1455,1463,1473,1476,1482,1564,1617,1648,1680,1700,1715,1735,1744,1761,1762,1744,1744,1728,1728,1712,1680,1664,1663,1669,1673,1669,1679,1681,1680,1692,1693,1692,1706,1702,1706,1678,1624,1716,1924,2122,2347,2559,2806,3071,3231,3159,2874,2309,1531,1240,1296,1372,1408,1434,1452,1457,1470,1472,1483,1570,1621,1662,1682,1706,1722,1733,1756,1765,1762,1749,1734,1727,1723,1698,1679,1664,1656,1666,1665,1673,1680,1670,1686,1694,1691,1696,1698,1701,1708,1665,1616,1727,1933,2133,2357,2590,2833,3107,3245,3153,2853,2259,1490,1248,1308,1376,1413,1433,1454,1466,1473,1477,1489,1578,1630,1664,1683,1711,1718,1740,1759,1761,1769,1759,1734,1729,1717,1701,1680,1659,1664,1677,1673,1680,1686,1685,1696,1690,1695,1703,1703,1710,1712,1671,1632,1750,1959,2160,2379,2619,2859,3127,3262,3142,2839,2210,1456,1254,1320,1386,1424,1437,1456,1472,1474,1486,1489,1584,1638,1661,1686,1712,1722,1738,1749,1761,1771,1749,1744,1735,1725,1711,1680,1662,1666,1669,1677,1678,1678,1683,1690,1689,1697,1696,1703,1707,1705,1664,1627,1759,1968,2167,2395,2633,2871,3151,3255,3118,2799,2133,1424,1260,1324,1392,1418,1434,1458,1456,1462,1473,1488,1587,1633,1661,1688,1709,1723,1737,1744,1759,1760,1749,1739,1725,1728,1705,1674,1663,1664,1666,1679,1676,1680,1687,1683,1690,1692,1694,1703,1701,1707,1659,1628,1776,1983,2185,2415,2649,2895,3167,3247,3102,2768,2077,1395,1259,1328,1393,1410,1436,1456,1458,1474,1472,1489,1593,1626,1663,1685,1711,1726,1737,1753,1762,1755,1746,1733,1726,1719,1690,1664,1662,1657,1667,1671,1673,1680,1680,1682,1687,1689,1695,1702,1698,1701,1648,1625,1787,1986,2195,2426,2659,2911,3171,3247,3071,2723,2001,1357,1256,1337,1392,1415,1442,1456,1462,1469,1472,1501,1590,1631,1665,1682,1710,1726,1736,1757,1760,1753,1745,1731,1727,1718,1693,1666,1657,1662,1674,1667,1670,1678,1677,1683,1684,1682,1694,1695,1705,1699,1648,1638,1806,2007,2219,2439,2679,2931,3179,3248,3056,2691,1948,1337,1264,1344,1392,1420,1437,1451,1461,1470,1479,1515,1603,1646,1673,1695,1722,1728,1745,1762,1764,1769,1747,1731,1730,1712,1703,1675,1664,1670,1669,1666,1677,1680,1687,1691,1687,1696,1693,1700,1707,1697,1648,1646,1825,2022,2230,2462,2703,2943,3197,3239,3023,2651,1879,1312,1266,1339,1398,1422,1440,1456,1464,1472,1478,1517,1605,1637,1665,1691,1708,1722,1741,1750,1761,1758,1739,1730,1727,1717,1702,1663,1663,1669,1670,1680,1681,1680,1691,1685,1694,1700,1698,1708,1711,1694,1648,1654,1845,2043,2255,2485,2719,2975,3219,3247,3024,2622,1827,1310,1277,1355,1409,1424,1454,1457,1460,1472,1483,1522,1615,1645,1678,1701,1712,1729,1744,1759,1770,1763,1749,1744,1731,1723,1700,1668,1666,1666,1671,1680,1679,1687,1686,1691,1699,1699,1706,1711,1707,1697,1639,1664,1861,2055,2269,2501,2736,2996,3231,3239,2995,2558,1775,1293,1280,1364,1415,1430,1456,1465,1474,1480,1488,1535,1613,1646,1680,1697,1714,1737,1744,1759,1758,1751,1746,1728,1725,1712,1685,1661,1661,1665,1674,1667,1679,1682,1680,1690,1690,1694,1701,1697,1703,1690,1632,1670,1872,2066,2287,2507,2751,3013,3223,3219,2951,2509,1715,1274,1286,1362,1406,1426,1452,1456,1470,1472,1481,1535,1609,1647,1669,1686,1713,1728,1745,1763,1759,1753,1742,1727,1725,1705,1680,1662,1654,1666,1667,1670,1680,1679,1680,1689,1690,1696,1696,1699,1708,1680,1625,1680,1889,2090,2299,2527,2769,3030,3229,3211,2935,2462,1664,1264,1290,1362,1414,1433,1446,1462,1472,1474,1488,1554,1619,1654,1678,1699,1715,1730,1754,1761,1765,1761,1744,1734,1728,1712,1691,1666,1664,1669,1670,1685,1686,1683,1687,1685,1696,1697,1697,1707,1707,1680,1629,1690,1899,2096,2313,2543,2789,3053,3247,3199,2919,2407,1603,1258,1296,1366,1410,1424,1447,1462,1464,1472,1484,1554,1617,1650,1677,1705,1713,1733,1754,1762,1771,1750,1738,1729,1719,1710,1685,1657,1662,1664,1670,1676,1674,1680,1688,1689,1696,1696,1697,1708,1709,1680,1631,1712,1921,2115,2335,2559,2807,3071,3251,3183,2901,2353,1568,1261,1296,1373,1415,1427,1456,1466,1473,1486,1486,1568,1622,1650,1680,1697,1714,1733,1744,1760,1762,1748,1741,1728,1718,1709,1680,1664,1668,1667,1678,1680,1677,1686,1683,1694,1696,1697,1698,1701,1703,1674,1625,1727,1936,2128,2352,2559,2826,3103,3243,3159,2867,2289,1518,1252,1296,1372,1402,1424,1449,1457,1469,1472,1482,1577,1620,1659,1680,1704,1721,1735,1755,1770,1767,1755,1744,1726,1721,1707,1676,1664,1658,1665,1678,1678,1687,1687,1691,1696,1693,1701,1703,1701,1710,1671,1623,1741,1938,2146,2371,2602,2845,3119,3247,3151,2835,2231,1474,1246,1312,1377,1409,1433,1446,1458,1472,1469,1485,1579,1625,1658,1680,1703,1723,1733,1750,1762,1763,1754,1733,1725,1717,1700,1679,1658,1656,1665,1664,1670,1680,1679,1685,1683,1684,1699,1696,1699,1709,1662,1626,1746,1957,2162,2375,2615,2861,3123,3239,3119,2802,2166,1429,1246,1313,1376,1415,1433,1449,1460,1461,1472,1488,1584,1632,1659,1676,1707,1721,1730,1754,1763,1769,1749,1735,1729,1719,1707,1675,1662,1666,1673,1674,1680,1678,1679,1683,1683,1692,1693,1696,1708,1712,1664,1627,1761,1971,2178,2403,2643,2879,3153,3263,3115,2789,2111,1408,1261,1328,1388,1424,1438,1457,1466,1469,1484,1488,1588,1633,1660,1690,1712,1724,1744,1753,1761,1763,1745,1735,1728,1718,1710,1673,1662,1669,1664,1676,1674,1679,1688,1683,1686,1697,1691,1702,1709,1709,1664,1629,1783,1990,2192,2423,2655,2902,3167,3255,3095,2751,2039,1380,1262,1328,1392,1414,1433,1456,1463,1483,1482,1500,1601,1632,1664,1693,1712,1729,1744,1760,1769,1759,1749,1744,1733,1729,1712,1673,1665,1665,1671,1680,1678,1685,1686,1692,1697,1696,1697,1711,1706,1711,1658,1643,1808,2002,2213,2435,2671,2925,3184,3249,3071,2717,1983,1356,1262,1344,1397,1416,1441,1456,1461,1475,1475,1506,1600,1633,1665,1691,1716,1735,1743,1765,1766,1760,1751,1734,1729,1722,1697,1675,1664,1665,1677,1678,1680,1686,1681,1689,1694,1695,1699,1699,1706,1707,1648,1645,1814,2015,2239,2459,2691,2943,3199,3255,3056,2685,1927,1332,1264,1348,1399,1425,1445,1460,1472,1475,1482,1520,1600,1634,1664,1687,1712,1723,1737,1758,1757,1757,1744,1731,1729,1712,1691,1665,1651,1660,1665,1667,1674,1678,1677,1681,1680,1691,1691,1692,1703,1695,1641,1648,1827,2030,2241,2465,2707};
void sendUptime()
{
  //Blynk_GSM.virtualWrite(V0,ch[count]);
  Blynk_GSM.virtualWrite(V0,ch[count]);
  count = count+1;
  count%=1500;
}

void setup()
{
  // Set console baud rate
  SerialMon.begin(115200);
  while (!SerialMon);
  timer.setInterval(800L,sendUptime); 
  delay(200);
  
  SerialMon.print(F("\nStart ESP32_GSM (Simultaneous WiFi+GSM) using "));
  SerialMon.print(CurrentFileFS);
  SerialMon.print(F(" on ")); SerialMon.println(ARDUINO_BOARD);
  SerialMon.println(BLYNK_GSM_MANAGER_VERSION);

#if USE_BLYNK_WM
  Serial.println(ESP_DOUBLE_RESET_DETECTOR_VERSION);
#endif  

  // Set-up modem reset, enable, power pins
  //pinMode(MODEM_PWKEY, OUTPUT);
  //pinMode(MODEM_RST, OUTPUT);
  //pinMode(MODEM_POWER_ON, OUTPUT);

  //digitalWrite(MODEM_PWKEY, LOW);
  //digitalWrite(MODEM_RST, HIGH);
  //digitalWrite(MODEM_POWER_ON, HIGH);

  SerialMon.println(F("Set GSM module baud rate"));

  // Set GSM module baud rate
  //SerialAT.begin(115200);
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  Serial.println(F("Use WiFi to connect Blynk"));

#if USE_BLYNK_WM

  // Set config portal SSID and Password
  Blynk_WF.setConfigPortal("TestPortal-ESP32", "TestPortalPass");
    
  // Use configurable AP IP, instead of default IP 192.168.4.1
  Blynk_WF.setConfigPortalIP(IPAddress(192, 168, 232, 1));
  // Set config portal channel, default = 1. Use 0 => random channel from 1-12 to avoid conflict
  Blynk_WF.setConfigPortalChannel(0);

  // Select either one of these to set static IP + DNS
  Blynk_WF.setSTAStaticIPConfig(IPAddress(192, 168, 2, 232), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0));
  //Blynk_WF.setSTAStaticIPConfig(IPAddress(192, 168, 2, 232), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0),
  //                           IPAddress(192, 168, 2, 1), IPAddress(8, 8, 8, 8));
  //Blynk_WF.setSTAStaticIPConfig(IPAddress(192, 168, 2, 232), IPAddress(192, 168, 2, 1), IPAddress(255, 255, 255, 0),
  //                           IPAddress(4, 4, 4, 4), IPAddress(8, 8, 8, 8));
  
  // Use this to default DHCP hostname to ESP8266-XXXXXX or ESP32-XXXXXX
  //Blynk_WF.begin();
  // Use this to personalize DHCP hostname (RFC952 conformed)
  // 24 chars max,- only a..z A..Z 0..9 '-' and no '-' as last char
  Blynk_WF.begin("ESP32-WiFi-GSM");
  
#else
  //Blynk_WF.begin(wifi_blynk_tok, ssid, pass, blynk_server, BLYNK_HARDWARE_PORT);
#ifndef UBIDOTS
  Blynk_GSM.config(modem, gsm_blynk_tok, blynk_server, BLYNK_HARDWARE_PORT);
  GSM_CONNECT_OK = Blynk_GSM.connectNetwork(apn, gprsUser, gprsPass);

  if (GSM_CONNECT_OK)
    Blynk_GSM.connect();
#else
  modem.init();//modem.restart();
  
#endif


  
#endif

#if USE_BLYNK_WM
  Blynk_WF_Configuration localBlynkGSM_ESP32_config;

  Blynk_WF.getFullConfigData(&localBlynkGSM_ESP32_config);

  Serial.print(F("gprs apn = "));
  Serial.println(localBlynkGSM_ESP32_config.apn);

  if ( Blynk.inConfigPortal() || (String(localBlynkGSM_ESP32_config.apn) == NO_CONFIG) )
  {
    Serial.println(F("DRD/MRD, Forced Config Portal or No valid stored apn. Must run only WiFi to Open config portal"));
    valid_apn = false;
  }
  else
  {
    valid_apn = true;

    for (uint16_t index = 0; index < NUM_BLYNK_CREDENTIALS; index++)
    {
      Blynk_GSM.config(modem, localBlynkGSM_ESP32_config.Blynk_Creds[index].gsm_blynk_token,
                       localBlynkGSM_ESP32_config.Blynk_Creds[index].blynk_server, localBlynkGSM_ESP32_config.blynk_port);

      GSM_CONNECT_OK = Blynk_GSM.connectNetwork(localBlynkGSM_ESP32_config.apn, localBlynkGSM_ESP32_config.gprsUser,
                       localBlynkGSM_ESP32_config.gprsPass);

      if (GSM_CONNECT_OK)
      {
        if ( Blynk_GSM.connect() == true )
          break;
      }
    }
  }
#endif
}

#if (USE_BLYNK_WM && USE_DYNAMIC_PARAMETERS)
void displayCredentials()
{
  Serial.println(F("\nYour stored Credentials :"));

  for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
  {
    Serial.print(myMenuItems[i].displayName);
    Serial.print(F(" = "));
    Serial.println(myMenuItems[i].pdata);
  }
}
#endif

void loop()
{
  //Blynk_WF.run();

#if USE_BLYNK_WM
  if (valid_apn)
#endif
#ifndef UBIDOTS
  {
    if (GSM_CONNECT_OK)
      Blynk_GSM.run();
    timer.run();
  }
#else
static uint8_t clientconnected=false;
static uint32_t count =0;
char sensordata[]="ESP32/1.0|POST|BBFF-QxviaYNuZBDTExAYOk0QA5PTVNwDE9|ecg_monitor:ECG_Monitor=>ecg_monitor:";
char sensortoPost[200] ={0};
char sensorRxPost[100] ={0};
if( clientconnected == false)
{
  modem.waitForNetwork(600000L);
  modem.gprsConnect("airtelgprs.com", "", "");  
  client.connect("169.55.61.243",9012);
  delay(2000);
}
  //uint32_t timeout = millis();
  if (!client.connected())  {
    // Print available data
    clientconnected = true;
  }
  else
  {    
    clientconnected = true;
    sprintf(sensortoPost,"%s%d%s",sensordata,ch[count],"|end");
    count=count%1500;
    count++;
    Serial.print(sensortoPost);
    client.write(sensortoPost);
    //client.read((uint8_t*)sensorRxPost,sizeof(sensorRxPost));    
    //Serial.print(sensorRxPost);                                                                                                                                                                                                                                                                                                                                          
    delay(500);
  }
#endif
  check_status();
  
#if (USE_BLYNK_WM && USE_DYNAMIC_PARAMETERS)
  static bool displayedCredentials = false;

  if (!displayedCredentials)
  {
    for (uint16_t i = 0; i < NUM_MENU_ITEMS; i++)
    {
      if (!strlen(myMenuItems[i].pdata))
      {
        break;
      }

      if ( i == (NUM_MENU_ITEMS - 1) )
      {
        displayedCredentials = true;
        displayCredentials();
      }
    }
  }
#endif
}