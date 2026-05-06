// !! GITIGNORED — never commit this file !!
// Copy from wifi_credentials.example.h and fill in your details.

#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

struct WifiCredential {
  const char *ssid;
  const char *password;
};

static const WifiCredential WIFI_NETWORKS[] = {
    {"vsup-vr", "UMPRUMVirtualRealityNetwork"},
    {"Zavadilka", "nazavadilce1434"},
    {"vsup", "Eiffel.115"}
    // {"school_network", "school_password"},
};

static const int WIFI_NETWORK_COUNT =
    sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

#endif
