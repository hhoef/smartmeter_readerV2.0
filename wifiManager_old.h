    //WiFiManager
    WiFiManagerParameter custom_dsmr_server("server", "dsmr server", dsmr_server, 16);
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
//    wifiManager.autoConnect("AutoConnectAP", "bladiebla");
    wifiManager.setTimeout(120);                            //sets timeout until configuration portal gets turned off

    // id/name, placeholder/prompt, default, length
    wifiManager.addParameter(&custom_dsmr_server);

    if (!wifiManager.startConfigPortal("OnDemandAP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
    strcpy(dsmr_server, custom_dsmr_server.getValue());
    IPAddress remote_ip(192, 168, 178, 4);
    Serial.print("DSMR IP Address: ");
    Serial.println(dsmr_server);
    Serial.println("connected...yeey)");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    OLED_connect();
    ArduinoOTA.handle();
    delay(5000);
  }
