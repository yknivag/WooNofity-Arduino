# WooNofity Arduino Examples
This repository contains example Arduino sketches to use with the [WooNotify](https://github.com/yknivag/WooNotify) whilst is available in the WordPress plugin directory as [Alerts via MQQT for WooCommerce](https://wordpress.org/plugins/wc-mqtt-alerts/).

## Background
If you wish to do more than display numbers on a screen, for example if you want to flash a light or ring a bell when a new order is ready to be fulfilled, then you can add the code to do that to the appropriate functions in the examples.  However to know which function to use, it is necessary to have an understanding of WooCommerce order states.

Full details of these order states may be found in [the official documentation](https://docs.woocommerce.com/document/managing-orders/), and the following chart lists the functions based on the WooCommerce State and the data available.  The chart also includes the data for stock dates.

| WooCommerce State | function name | Data available (type) |
| ----------------- | ------------- | --------------------- |
| Pending Payment | notify_orders_pending() | Order No (String) |
| On-Hold | notify_orders_onhold() | Order No (String) |
| Processing | notify_orders_processing() | Order No (String) |
| Completed | notify_orders_completed() | Order No (String) |
| Cancelled | notify_orders_cancelled() | Order No (String) |
| Failed |notify_orders_failed() | Order No (String) |
| Refunded |notify_orders_refunded() | Order No (String) |
| Low Stock | notify_stock_low() | Product ID (String) |
| Out Of Stock | notify_stock_out() | Product ID (String) |

In addition to notifications of individual order and stock events, the plugin sends a summary of orders in each state after every change.  This comes as JSON array and the examples show how to get the appropriate data from it.  The naming conventions are the same as those detailed above.

## Security & Portability
There is no personal data transmitted over MQTT, just order numbers, product IDs and summary statistics.  As a result there is no requirement to secure this datastream.

The first sketch uses only the MQTT feed and so whilst written for the ESP8266 but should run on any board that provides a compatible WiFi `client`.  Just replace the `#include <ESP8266WiFi.h>` with the appropriate library that provides `client` for your board.

## Examples
Currently there is one example here.  Another using the WooCommerce API will follow shortly.

### WooNotifierPoC (Proof Of Concept Sketch)
This sketch provides a bare minimum implemetation using all the feeds available.  It simply prints the data received to the Serial Monitor, but it is fully extendable to provide whatever functionality you may wish to implement.

The sketch is subscribed to all of the end points, but you only need use those you are interested in.  Code to perform action or updates etc should be added to `notify_xxx()` functions for order pings or to the `process_xxx()` functions for the stats topics.

For example if you wanted a light to flash every time an order was received and paid for, you would put the code to flash the light in the function named `notify_orders_processing()`.