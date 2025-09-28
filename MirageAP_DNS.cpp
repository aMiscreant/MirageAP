// MirageAP_DNS.cpp
#include "MirageAP_DNS.h"
#include <DNSServer.h>

static DNSServer dnsServer;

void MirageAP_DNS::start(IPAddress apIP) {
  dnsServer.start(53, "*", apIP);
}

void MirageAP_DNS::process() {
  dnsServer.processNextRequest();
}
