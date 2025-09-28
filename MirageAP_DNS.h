// MirageAP_DNS.h
#ifndef MIRAGEAP_DNS_H
#define MIRAGEAP_DNS_H

#include <ESP8266WiFi.h>

namespace MirageAP_DNS {
  void start(IPAddress apIP);
  void process();
}

#endif
