/*----------------------------------------------------------------------------*
 * @file WooNotfierPoC.ino
 * 
 * @author Gavin Smalley (gavin.smalley@googlemail.com)
 * @brief MQTT WooCommerce Sales Notifier
 * @link https://github.com/yknivag/WooNofity-Arduino.git
 * @version 1.0
 * @date 2020-06-22
 * 
 * @copyright Copyright (c) 2020
 * 
 * Description
 * ===========
 * 
 * The sketch takes an MQTT feed from WooCommerce and outputs to Serial.
 * 
 * The sketch is dependent on a WordPress/WooCommerce plugin being installed 
 * on the shop server (see README.md) for more details.
 * 
 * It uses shiftr.io as the MQTT broker, but it should work with any MQTT
 * broker which is compatible with the WooMQTT plugin.
 * 
 * Wiring (for a Wemos D1 Mini, other boards may vary)
 * ===================================================
 *  
 * This "Proof Of Concept" sketch requires no wiring, as it simply outputs the 
 *   data received to the Serial Monitor.
 *  
 * Comments
 * ========
 *  
 *  The sketch is subscribed to all of the end points, but you only need to
 *    to use those you are interested in.  Code to perform action or updates 
 *    etc should be added to "notify_xxx()" functions for order pings or to
 *    the "process_xxx()" functions for the stats topics.
 *  
 *  For example if you wanted a light to flash every time an order was received
 *    and paid for, you would put the code to flash the light in the function
 *    named "notify_orders_processing()".
 *    
 *  There is no personal data transmitted over MQTT, just order numbers, 
 *    product IDs and summary statistics.  If you wish to retrieve full 
 *    details of a new order to display on a screen in response to the MQTT
 *    ping then you will need to use the WP API.  A further example will
 *    be provided for this in due course.
 *  
  ----------------------------------------------------------------------------*/

// --- --- INCLUDES --- INCLUDES --- INCLUDES --- --- //

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <MQTTClient.h>           // Arduino MQTT Library by Joel Gaehwiler

// --- --- DEFINES --- DEFINES --- DEFINES --- --- //

// --- --- DEFAULTS --- DEFAULTS --- DEFAULTS --- --- //

// WiFi Credentials
char* wifi_ssid = "***";
char* wifi_password = "***";

// MQTT Broker Details
char mqtt_server[128] = "broker.shiftr.io";
char mqtt_port[6] = "1883";
char mqtt_username[128] = "***";
char mqtt_password[128] = "***";
char mqtt_topic_prefix[64] = ""; // Use this if you specified an MQTT topic prefix in WooMQTT - it should be the same.
char device_id[16]; // We later use "itoa (chipID, this_candle_id, 10);" to populate this with the value of ESP.getChipId(); - it is used as a unique identifier for the MQTT connection

// Define MQTT topic suffixes used by WooNotify MQTT.
char mqtt_topic_suffix_orders_pending[32] = "orders/pending";
char mqtt_topic_suffix_orders_onhold[32] = "orders/on-hold";
char mqtt_topic_suffix_orders_processing[32] = "orders/processing";
char mqtt_topic_suffix_orders_completed[32] = "orders/completed";
char mqtt_topic_suffix_orders_cancelled[32] = "orders/cancelled";
char mqtt_topic_suffix_orders_failed[32] = "orders/failed";
char mqtt_topic_suffix_orders_refunded[32] = "orders/refunded";
char mqtt_topic_suffix_stock_low[32] = "stock/low";
char mqtt_topic_suffix_stock_out[32] = "stock/out";
char mqtt_topic_suffix_stats_order[32] = "stats/orders";
char mqtt_topic_suffix_stats_stock[32] = "stats/stock";

// Set up global variables to hold the MQTT topic names (these are prepended with mqtt_topic_prefix/ later).  These match the topics used by WooMQTT
char mqtt_topic_orders_pending[128];
char mqtt_topic_orders_onhold[128];
char mqtt_topic_orders_processing[128];
char mqtt_topic_orders_completed[128];
char mqtt_topic_orders_cancelled[128];
char mqtt_topic_orders_failed[128];
char mqtt_topic_orders_refunded[128];
char mqtt_topic_stock_low[128];
char mqtt_topic_stock_out[128];
char mqtt_topic_stats_order[128];
char mqtt_topic_stats_stock[128];

// --- --- CONTROLS --- CONTROLS --- CONTROLS --- --- //

bool isDebug = true; // Set this to true to see debug messages on the Serial Monitor.
uint32_t lastConnectMillis = 0;

// --- --- INITIALISATIONS --- INITIALISATIONS --- INITIALISATIONS --- --- //

WiFiClient net;
MQTTClient client;

// --- --- FUNCTIONS --- FUNCTIONS --- FUNCTIONS --- --- //

// Function to deal with debug prints. (c/o Ralph Bacon - note in Arduino IDE the function declaration MUST be on the same line as the template definition.)
template<typename T> void debugPrint(T printMe) {
  if (isDebug) {
    Serial.print(printMe);
    Serial.flush();
  }
}
template<typename T> void debugPrintln(T printMe) {
  if (isDebug) {
    Serial.println(printMe);
    Serial.flush();
  }
}

// Function to handle WiFi and MQTT Connection
void connect() {

  // Connect to WiFi
  debugPrint("Connecting to WiFi... ");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  if (isDebug) {
    WiFi.printDiag(Serial);  // Remove this line if you do not want to see WiFi password printed
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay (250);
    debugPrint("... ");
  }
  debugPrintln("");
  debugPrintln("WiFi connected");
  debugPrint("IP address: ");
  debugPrintln(WiFi.localIP());
  debugPrintln("");

  // Connect to MQTT
  debugPrint("\nMQTT Connecting... ");
  client.begin(mqtt_server, atoi(mqtt_port), net);
  client.onMessage(messageReceived);
  while (!client.connect(device_id, mqtt_username, mqtt_password)) {
    debugPrint("... ");
    delay(250);
  }
  debugPrintln("");
  debugPrintln("MQTT connected");
  debugPrintln("");

  // It is not necessary to subscribe to all of the topics here depending on your requirements.  Check the README.md file for more details.
  if (client.subscribe(mqtt_topic_orders_pending)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_orders_pending);
  if (client.subscribe(mqtt_topic_orders_onhold)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_orders_onhold);
  if (client.subscribe(mqtt_topic_orders_processing)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_orders_processing);
  if (client.subscribe(mqtt_topic_orders_completed)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_orders_completed);
  if (client.subscribe(mqtt_topic_orders_cancelled)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_orders_cancelled);
  if (client.subscribe(mqtt_topic_orders_failed)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_orders_failed);
  if (client.subscribe(mqtt_topic_orders_refunded)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_orders_refunded);
  if (client.subscribe(mqtt_topic_stock_low)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_stock_low);
  if (client.subscribe(mqtt_topic_stock_out)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_stock_out);
  if (client.subscribe(mqtt_topic_stats_order)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_stats_order);
  if (client.subscribe(mqtt_topic_stats_stock)) {
    debugPrint("Subscribed To: ");
  } else {
    debugPrint("Failed to Subscribe To: ");
  }
  debugPrintln(mqtt_topic_stats_stock);
}

// Function to handle reception of MQTT messages
void messageReceived(String &topic, String &payload) {
  debugPrint("Incoming MQTT Message: ");
  debugPrint(topic);
  debugPrint(" - ");
  debugPrint(payload);
  debugPrintln("");

  if (topic == mqtt_topic_orders_pending) {
     notify_orders_pending(payload);
  }
  if (topic == mqtt_topic_orders_onhold) {
     notify_orders_onhold(payload);
  }
  if (topic == mqtt_topic_orders_processing) {
     notify_orders_processing(payload);
  }
  if (topic == mqtt_topic_orders_completed) {
     notify_orders_completed(payload);
  }
  if (topic == mqtt_topic_orders_cancelled) {
     notify_orders_cancelled(payload);
  }
  if (topic == mqtt_topic_orders_failed) {
     notify_orders_failed(payload);
  }
  if (topic == mqtt_topic_orders_refunded) {
     notify_orders_refunded(payload);
  }
  if (topic == mqtt_topic_stock_low) {
     notify_stock_low(payload);
  }
  if (topic == mqtt_topic_stock_out) {
     notify_stock_out(payload);
  }
  if (topic == mqtt_topic_stats_order) {
     process_stats_order(payload);
  }
  if (topic == mqtt_topic_stats_stock) {
     process_stats_stock(payload);
  }
}

// Functions to handle what happens when a new MQTT message is received.
// Put your actions in these functions.
void notify_orders_pending(String payload) {
  //Payload will be the order number which transitioned state.
  Serial.print("New order transitioned to \"Payment Pending\". Order No: ");
  Serial.println(payload);
  Serial.println();
  yield();
}
void notify_orders_onhold(String payload) {
  //Payload will be the order number which transitioned state.
  Serial.print("An order transitioned to \"On-Hold\". Order No: ");
  Serial.println(payload);
  Serial.println();
  yield();
}
void notify_orders_processing(String payload) {
  //Payload will be the order number which transitioned state.
  Serial.print("An order transitioned to \"Processing\". Order No: ");
  Serial.println(payload);
  Serial.println();
  yield();
}
void notify_orders_completed(String payload) {
  //Payload will be the order number which transitioned state.
  Serial.print("An order transitioned to \"Completed\". Order No: ");
  Serial.println(payload);
  Serial.println();
  yield();
}
void notify_orders_cancelled(String payload) {
  //Payload will be the order number which transitioned state.
  Serial.print("An order transitioned to \"Cancelled\". Order No: ");
  Serial.println(payload);
  Serial.println();
  yield();
}
void notify_orders_failed(String payload) {
  //Payload will be the order number which transitioned state.
  Serial.print("An order transitioned to \"Failed\". Order No: ");
  Serial.println(payload);
  Serial.println();
  yield();
}
void notify_orders_refunded(String payload) {
  //Payload will be the order number which transitioned state.
  Serial.print("An order transitioned to \"Refunded\". Order No: ");
  Serial.println(payload);
  Serial.println();
  yield();
}
void notify_stock_low(String payload) {
  //Payload will be the product ID which went low in stock.
  Serial.print("A product transitioned to \"Low In Stock\". Product ID: ");
  Serial.println(payload);
  Serial.println();
  yield();
}
void notify_stock_out(String payload) {
  //Payload will be the product ID which went out of stock.
  Serial.print("A product transitioned to \"Out Of Stock\". Product ID: ");
  Serial.println(payload);
  Serial.println();
  yield();
}
void process_stats_order(String payload) {
  //Payload will be a serialised JSON array containing the number of orders in each state.
  Serial.print("Order Counts: ");
  Serial.println(payload);
  Serial.println();
  yield();
  Serial.println("Decoding JSON...");
  const size_t capacity = JSON_OBJECT_SIZE(7) + 80;
  DynamicJsonDocument doc(capacity);

  const char* json = payload.c_str();

  deserializeJson(doc, json);

  long orders_pending = doc["payment-pending"]; // 10241024
  long orders_onhold = doc["on-hold"]; // 10241024
  long orders_processing = doc["processing"]; // 10241024
  long orders_completed = doc["completed"]; // 10241024
  long orders_cancelled = doc["cancelled"]; // 10241024
  long orders_failed = doc["failed"]; // 10241024
  long orders_refunded = doc["refunded"]; // 10241024

  Serial.println("De-Serialised Output: ");

  Serial.print("Number of orders in Payment-Pending: ");
  Serial.println(orders_pending);
  Serial.print("Number of orders in On-Hold: ");
  Serial.println(orders_onhold);
  Serial.print("Number of orders in Processing: ");
  Serial.println(orders_processing);
  Serial.print("Number of orders in Completed: ");
  Serial.println(orders_completed);
  Serial.print("Number of orders in Cancelled: ");
  Serial.println(orders_cancelled);
  Serial.print("Number of orders in Failed: ");
  Serial.println(orders_failed);
  Serial.print("Number of orders in Refunded: ");
  Serial.println(orders_refunded);
  Serial.println();
}
void process_stats_stock(String payload) {
  // Payload will be a serialised JSON array containing the number of items low in stock/out of stock.
  Serial.print("Stock Counds: ");
  Serial.println(payload);
  yield();
  Serial.println("Decoding JSON...");
  const size_t capacity = JSON_OBJECT_SIZE(2) + 80;
  DynamicJsonDocument doc(capacity);

  const char* json = payload.c_str();

  deserializeJson(doc, json);

  long stock_low = doc["low-stock"];
  long stock_out = doc["out-of-stock"];

  Serial.println("De-Serialised Output: ");

  Serial.print("Number of products Low-In-Stock: ");
  Serial.println(stock_low);
  Serial.print("Number of produts Out-Of-Stock: ");
  Serial.println(stock_out);
  Serial.println();
}

// --- --- SETUP --- SETUP --- SETUP --- --- //

void setup() {
  // put your setup code here, to run once:

  // Start Serial for debugging
  Serial.begin(115200);
  Serial.println();
  Serial.println("-------------------------------------------------------------------------------");
  Serial.println("|                     WooNotifyPoC - MQTT for WooCommerce                     |");
  Serial.println("|                     ===================================                     |");
  Serial.println("|     Author:  Gavin Smalley (yknivag)                                        |");
  Serial.println("|     GitHub:  https://github.com/yknivag/WooNofity-Arduino.git               |");
  Serial.println("|     Version: 1.0                                                            |");
  Serial.println("|     Date:    2020-06-26                                                     |");
  Serial.println("|                                                                             |");
  Serial.println("|  This Proof of Concept sketch takes an MQTT feed from WooCommerce and       |");
  Serial.println("|    outputs to Serial.                                                       |");
  Serial.println("-------------------------------------------------------------------------------");
  Serial.println();
  debugPrintln("\n Starting...");

  // Fill in defaults that can't be built above...
  int chipID = ESP.getChipId();
  itoa (chipID, device_id, 10);

  // Create MQTT Topic Names
  strcpy (mqtt_topic_orders_pending, mqtt_topic_prefix);
  strcat (mqtt_topic_orders_pending, mqtt_topic_suffix_orders_pending);

  strcpy (mqtt_topic_orders_onhold, mqtt_topic_prefix);
  strcat (mqtt_topic_orders_onhold, mqtt_topic_suffix_orders_onhold);

  strcpy (mqtt_topic_orders_processing, mqtt_topic_prefix);
  strcat (mqtt_topic_orders_processing, mqtt_topic_suffix_orders_processing);

  strcpy (mqtt_topic_orders_completed, mqtt_topic_prefix);
  strcat (mqtt_topic_orders_completed, mqtt_topic_suffix_orders_completed);

  strcpy (mqtt_topic_orders_cancelled, mqtt_topic_prefix);
  strcat (mqtt_topic_orders_cancelled, mqtt_topic_suffix_orders_cancelled);

  strcpy (mqtt_topic_orders_failed, mqtt_topic_prefix);
  strcat (mqtt_topic_orders_failed, mqtt_topic_suffix_orders_failed);

  strcpy (mqtt_topic_orders_refunded, mqtt_topic_prefix);
  strcat (mqtt_topic_orders_refunded, mqtt_topic_suffix_orders_refunded);

  strcpy (mqtt_topic_stock_low, mqtt_topic_prefix);
  strcat (mqtt_topic_stock_low, mqtt_topic_suffix_stock_low);

  strcpy (mqtt_topic_stock_out, mqtt_topic_prefix);
  strcat (mqtt_topic_stock_out, mqtt_topic_suffix_stock_out);

  strcpy (mqtt_topic_stats_order, mqtt_topic_prefix);
  strcat (mqtt_topic_stats_order, mqtt_topic_suffix_stats_order);

  strcpy (mqtt_topic_stats_stock, mqtt_topic_prefix);
  strcat (mqtt_topic_stats_stock, mqtt_topic_suffix_stats_stock);

  debugPrintln("MQTT Topics...");
  debugPrintln("");
  debugPrint("New Payment-Pending Order Topic  : ");
  debugPrintln(mqtt_topic_orders_pending);
  debugPrint("New On-Hold Order Topic  : ");
  debugPrintln(mqtt_topic_orders_onhold);
  debugPrint("New Processing Order Topic  : ");
  debugPrintln(mqtt_topic_orders_processing);
  debugPrint("New Completed Order Topic  : ");
  debugPrintln(mqtt_topic_orders_completed);
  debugPrint("New Cancelled Order Topic  : ");
  debugPrintln(mqtt_topic_orders_cancelled);
  debugPrint("New Failed Order Topic  : ");
  debugPrintln(mqtt_topic_orders_failed);
  debugPrint("New Refunded Order Topic  : ");
  debugPrintln(mqtt_topic_orders_refunded);
  debugPrint("New Low In Stock Topic  : ");
  debugPrintln(mqtt_topic_stock_low);
  debugPrint("New Out Of Stock Topic  : ");
  debugPrintln(mqtt_topic_stock_out);
  debugPrint("Order Stats Topic  : ");
  debugPrintln(mqtt_topic_stats_order);
  debugPrint("Stats Stock Topic  : ");
  debugPrintln(mqtt_topic_stats_stock);

  // Connect to WiFi and MQTT
  connect();
}

// --- --- LOOP --- LOOP --- LOOP --- --- //

void loop() {
  // check for MQTT messages
  if (!client.connected()) {
    connect();
  }
  client.loop();
  delay(10);  // fixes some issues with WiFi stability
}
