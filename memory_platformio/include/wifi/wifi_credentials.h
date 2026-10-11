// !! GITIGNORED — never commit this file !!

#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

struct WifiCredential {
  const char* ssid;
  const char* password;
};

static const WifiCredential WIFI_NETWORKS[] = {
    // {"KUni Pflasterspektakel 2026", "KU_Pflaster-2026_nJ"}, // ARS ELECTRONICA LINZ
    // {"HolesovickaSachta", "12345678"}, // FINALFINAL
    // {"simonuviphone", "heslointernet"}, // iphone, didnt get it to work
    {"vsup", "Eiffel.115"},
    {"Zavadilka", "nazavadilce1434"},
    {"vsup-vr", "UMPRUMVirtualRealityNetwork"}};

static const int WIFI_NETWORK_COUNT =
    sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

#endif
