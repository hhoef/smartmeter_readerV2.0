/**
 * Define the data we're interested in, as well as the datastructure to
 * hold the parsed data. This list shows all supported fields, remove
 * any fields you are not using from the below list to make the parsing
 * and printing code smaller.
 * Each template argument below results in a field of the same name.
 */
using MyData = ParsedData<
  /* String */     identification,
  /* String */     p1_version,
  /* String */     timestamp,
  /* String */     equipment_id,
  /* FixedValue */ energy_delivered_tariff1,
  /* FixedValue */ energy_delivered_tariff2,
  /* FixedValue */ energy_returned_tariff1,
  /* FixedValue */ energy_returned_tariff2,
  /* String */     electricity_tariff,
  /* FixedValue */ power_delivered,
  /* FixedValue */ power_returned,
  /* FixedValue */ electricity_threshold,
  /* uint8_t */    electricity_switch_position,
  /* uint32_t */   electricity_failures,
  /* uint32_t */   electricity_long_failures,
  /* String */     electricity_failure_log,
  /* uint32_t */   electricity_sags_l1,
  /* uint32_t */   electricity_sags_l2,
  /* uint32_t */   electricity_sags_l3,
  /* uint32_t */   electricity_swells_l1,
  /* uint32_t */   electricity_swells_l2,
  /* uint32_t */   electricity_swells_l3,
  /* String */     message_short,
  /* String */     message_long,
  /* FixedValue */ voltage_l1,
  /* FixedValue */ voltage_l2,
  /* FixedValue */ voltage_l3,
  /* FixedValue */ current_l1,
  /* FixedValue */ current_l2,
  /* FixedValue */ current_l3,
  /* FixedValue */ power_delivered_l1,
  /* FixedValue */ power_delivered_l2,
  /* FixedValue */ power_delivered_l3,
  /* FixedValue */ power_returned_l1,
  /* FixedValue */ power_returned_l2,
  /* FixedValue */ power_returned_l3,
  /* uint16_t */   gas_device_type,
  /* String */     gas_equipment_id,
  /* uint8_t */    gas_valve_position,
  /* TimestampedFixedValue */ gas_delivered,
  /* uint16_t */   thermal_device_type,
  /* String */     thermal_equipment_id,
  /* uint8_t */    thermal_valve_position,
  /* TimestampedFixedValue */ thermal_delivered,
  /* uint16_t */   water_device_type,
  /* String */     water_equipment_id,
  /* uint8_t */    water_valve_position,
  /* TimestampedFixedValue */ water_delivered,
  /* uint16_t */   slave_device_type,
  /* String */     slave_equipment_id,
  /* uint8_t */    slave_valve_position,
  /* TimestampedFixedValue */ slave_delivered
>;

/**
 * This illustrates looping over all parsed fields using the
 * ParsedData::applyEach method.
 *
 * When passed an instance of this Printer object, applyEach will loop
 * over each field and call Printer::apply, passing a reference to each
 * field in turn. This passes the actual field object, not the field
 * value, so each call to Printer::apply will have a differently typed
 * parameter.
 *
 * For this reason, Printer::apply is a template, resulting in one
 * distinct apply method for each field used. This allows looking up
 * things like Item::name, which is different for every field type,
 * without having to resort to virtual method calls (which result in
 * extra storage usage). The tradeoff is here that there is more code
 * generated (but due to compiler inlining, it's pretty much the same as
 * if you just manually printed all field names and values (with no
 * cost at all if you don't use the Printer).
 */
struct Printer {
  template<typename Item>
  void apply(Item &i) {
    if (i.present()) {
      Serial.print(Item::name);
      Serial.print(F(": "));
      Serial.print(i.val());
      Serial.print(Item::unit());
      Serial.println();
    }
  }
};

struct UDPprinter {
  template<typename Item>
  void apply(Item &i) {
    if (i.present()) {
      Udp.print(Item::name);
      Udp.print(F(": "));
      Udp.print(i.val());
      Udp.print(Item::unit());
      Udp.print("\n");
    }
  }
};

void OLED_prepare(void) {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

// show connection info
void OLED_connect() {
  u8g2.clearBuffer();
  OLED_prepare();
  u8g2.drawStr(0, 0, "WiFi connected");   
  u8g2.drawStr(0, 8, "SSID:");  
  WiFi.SSID().toCharArray(oledTxt, sizeof(oledTxt));
  u8g2.drawStr(5*7+2, 8, oledTxt);  
  u8g2.drawStr(0, 16, "IP address:");
  sprintf(oledTxt, "%03d.%03d.%03d.%03d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  u8g2.drawStr(0*7+2, 24, oledTxt);
  u8g2.sendBuffer();
}

// display draw
void OLED_dsmr(MyData data) {
  u8g2.clearBuffer();
  OLED_prepare();
  u8g2.drawStr(0, 0, "DSMR reader");
  u8g2.setCursor(0, 8); 
  u8g2.print("Watt :"); u8g2.print(data.power_delivered); u8g2.print(" Kw");
  u8g2.setCursor(0, 16);
  u8g2.print("Power:"); u8g2.print(data.energy_delivered_tariff2 + data.energy_delivered_tariff2); u8g2.print(" Kwh");
  u8g2.setCursor(0, 24);
  u8g2.print("Gas  :"); u8g2.print(data.gas_delivered); u8g2.print(" m2");
  u8g2.sendBuffer();
}

// send udp data
void UDP_print(char* UDPdata) {
  int bite_send;
  Udp.beginPacket(remote_ip, remote_port);
  bite_send = Udp.write(UDPdata);
  Udp.endPacket();
}
