// Template for wifi_credentials.h (which is gitignored).
// Copy this file to wifi_credentials.h and fill in your details.

#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

struct WifiCredential {
  const char *ssid;
  const char *password;
};

static const WifiCredential WIFI_NETWORKS[] = {
    {"home_network", "home_password"},
    // {"school_network", "school_password"},
};

static const int WIFI_NETWORK_COUNT =
    sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

#endif
