#include "wled.h"

#define WLED_DEBOUNCE_THRESHOLD      50 // only consider button input of at least 50ms as valid (debouncing)
#define WLED_MAX_PRESETS            250 // max. no. of presets (iteration ceiling for preset discovery)

class UsermodBinarySwitches : public Usermod {

  private:

    // Private class members. You can declare variables and functions only accessible to your usermod here
    bool enabled = false;
    uint binary_state = 0;

    // config variables — defaults set inside readFromConfig()

    std::vector<uint> preset_map;

    // string that are used multiple time (this will save some flash memory)
    static const char _name[];
    static const char _enabled[];

  public:

    // non WLED related methods, may be used for data exchange between usermods (non-inline methods should be defined out of class)

    /**
     * Enable/Disable the usermod
     */
    inline void enable(bool enable) { enabled = enable; }

    /**
     * Get usermod enabled/disabled state
     */
    inline bool isEnabled() { return enabled; }

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * readFromConfig() is called prior to setup()
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() override {}

    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     */
    void loop() override {}

    static uint num_switches() {
      uint switches = 0;
      for (const Button &button : buttons) {
        if (button.type == BTN_TYPE_SWITCH || button.type == BTN_TYPE_TOUCH_SWITCH)
          switches++;
      }
      DEBUG_PRINT(FPSTR(_name));
      DEBUG_PRINTF(PSTR(": %i switches found\n"), switches);
      return switches;
    }

    /*
     * addToConfig() saves settings to cfg.json under the "um" object. WLED calls this whenever settings are saved.
     * The Usermod Settings page in the UI is generated automatically from the keys you write here.
     *
     * Usermod Settings Overview:
     * - Numeric values are treated as floats in the browser.
     *   - If the numeric value entered into the browser contains a decimal point, it will be parsed as a C float
     *     before being returned to the Usermod.  The float data type has only 6-7 decimal digits of precision, and
     *     doubles are not supported, numbers will be rounded to the nearest float value when being parsed.
     *     The range accepted by the input field is +/- 1.175494351e-38 to +/- 3.402823466e+38.
     *   - If the numeric value entered into the browser doesn't contain a decimal point, it will be parsed as a
     *     C int32_t (range: -2147483648 to 2147483647) before being returned to the usermod.
     *     Overflows or underflows are truncated to the max/min value for an int32_t, and again truncated to the type
     *     used in the Usermod when reading the value from ArduinoJson.
     * - Pin values can be treated differently from an integer value by using the key name "pin"
     *   - "pin" can contain a single or array of integer values
     *   - On the Usermod Settings page there is simple checking for pin conflicts and warnings for special pins
     *     - Red color indicates a conflict.  Yellow color indicates a pin with a warning (e.g. an input-only pin)
     *   - Tip: use int8_t to store the pin value in the Usermod, so a -1 value (pin not set) can be used
     *
     * To force a config write from loop(), call serializeConfig() — but use it sparingly (flash wear,
     * possible LED stutter). Never call it from a network callback.
     */
    void addToConfig(JsonObject& root) override
    {
      JsonObject top = root.createNestedObject(FPSTR(_name));
      top[FPSTR(_enabled)] = enabled;
      // top["testInt"] = testInt;
      // cannot be called "mapping" - since that contains the word "pin" and therefore gets treated as pin datatzpe
      JsonArray presetMap = top.createNestedArray("preset map");
      uint n_combinations = pow(2, num_switches());
      for (int i = 0; i < n_combinations && n_combinations > 1; i++)
          // upon save if new switches are added we get an out of range problem
          // so we zero out the new combinations' mappings
          presetMap.add(i < preset_map.size() ? preset_map.at(i) : 0 );
    }


    /*
     * readFromConfig() is called before setup() and again after settings are saved.
     * Return false if any expected keys were missing — WLED will then call addToConfig() to write the defaults.
     * getJsonValue(src, dest) copies the value if present and returns true; leaves dest unchanged if missing.
     * getJsonValue(src, dest, default) also assigns a default when the key is absent.
     */
    bool readFromConfig(JsonObject& root) override
    {
      JsonObject top = root[FPSTR(_name)];
      if (top.isNull()) {
        DEBUG_PRINT(FPSTR(_name));
        DEBUG_PRINTLN(F(": No config found. (Using defaults.)"));
      }

      bool configComplete = !top.isNull();

      configComplete &= getJsonValue(top[FPSTR(_enabled)], enabled, enabled);

      // A 3-argument getJsonValue() assigns the 3rd argument as a default value
      // if the Json value is missing
      // configComplete &= getJsonValue(top["testInt"], testInt, 42);

      // "pin" fields have special handling in settings page (or some_pin as well)

       // cache output (so it only gets called once)
      uint n_combinations = pow(2, num_switches());
      preset_map.clear();
      preset_map.insert(preset_map.end(), n_combinations, 0);
      DEBUG_PRINT(FPSTR(_name));
      DEBUG_PRINTF(PSTR(": loading config using %i combinations\n"), n_combinations);
      for (int i = 0; i < n_combinations && n_combinations > 1; i++)
        configComplete &= getJsonValue(top["preset map"][i], preset_map.at(i) , 0);

      return configComplete;
    }

  // bool getPresetName(byte index, String& name)
  //   {
  //     if (!requestJSONBufferLock(JSON_LOCK_PRESET_NAME)) return false;
  //     bool presetExists = false;
  //     if (readObjectFromFileUsingId(getPresetsFileName(), index, pDoc)) {
  //       JsonObject fdo = pDoc->as<JsonObject>();
  //       if (fdo["n"]) {
  //         name = (const char*)(fdo["n"]);
  //         presetExists = true;
  //       }
  //     }
  //     releaseJSONBufferLock();
  //     return presetExists;
  //   }

    /*
     * appendConfigData() is called when the Usermod Settings page renders.
     * Write JavaScript snippets to settingsScript to add helper text or dropdowns for your config fields.
     * addInfo('<ModName>:<key>', 1, '<html>') adds a tooltip/label next to the field.
     * addDropdown / addOption replace a plain text input with a <select>.
     */
    void appendConfigData(Print& settingsScript) override
    {
      auto printBits = [](size_t const size, uint const n) -> std::string {
        std::string ret;
        for (int i = size-1; i >= 0; i--)
            ret += std::to_string( (n >> i) & 1);

        return ret;
      };

      auto get_preset_options = []() -> std::string {
        std::string ret = "addOption(dd,'0: Default Nothingness',0);";
        for (int i = 1; i <= WLED_MAX_PRESETS; i++) {
          String name;
          if (getPresetName(i, name)) {
            ret += "addOption(dd,'";
            ret += std::to_string(i) + ": " + name.c_str();
            ret += "'," + std::to_string(i) + ");";
          }
        }
        return ret;
      };
      // cache output
      auto preset_options = get_preset_options();

      uint n_combinations = pow(2, num_switches());
      for (int i = 0; i < n_combinations && n_combinations > 1; i++) {
        settingsScript.print(F("addInfo('")); settingsScript.print(FPSTR(_name));
        settingsScript.print(F(":preset map[]',")); settingsScript.print(i);
        settingsScript.print(F(",'<i>(state: ")); settingsScript.print(printBits(num_switches() ,i).c_str());
        settingsScript.print(F(")</i>');"));

        settingsScript.print(F("dd=addDropdown('")); settingsScript.print(FPSTR(_name)); settingsScript.print(F(":preset map[]',")); settingsScript.print(i); settingsScript.print(F(");"));
        // settingsScript.print(F("addOption(dd,'TEXT HERE',0);"));
        settingsScript.print(preset_options.c_str());
      }
    }

    /**
     * handleButton() can be used to override default button behaviour. Returning true
     * will prevent button working in a default way.
     * Replicating button.cpp
     */
    bool handleButton(uint8_t b) override {
      yield();
      // ignore certain button types as they may have other consequences
      if (!enabled)
        return false;

      // do your button handling here
      if (buttons[b].type == BTN_TYPE_SWITCH || buttons[b].type == BTN_TYPE_TOUCH_SWITCH || buttons[b].type == BTN_TYPE_PIR_SENSOR) {
        // isButtonPressed() handles inverted/noninverted logic
        if (buttons[b].pressedBefore != isButtonPressed(b)) {
          DEBUG_PRINTF_P(PSTR("Switch: State changed %u\n"), b);
          buttons[b].pressedTime = millis();
          buttons[b].pressedBefore = !buttons[b].pressedBefore; // toggle pressed state
        }

        if (buttons[b].longPressed == buttons[b].pressedBefore) return true;

        if (millis() - buttons[b].pressedTime > WLED_DEBOUNCE_THRESHOLD) {
          //fire edge event only after 50ms without change (debounce)
          DEBUG_PRINTF_P(PSTR("Switch: Activating  %u\n"), b);

          binary_state ^= 1 << b;

          DEBUG_PRINTF_P(PSTR("Button %u, pin %i -> state: %i\n"), b, buttons[b].pin, buttons[b].pressedBefore);
          DEBUG_PRINT(FPSTR(_name));
          DEBUG_PRINTF_P(PSTR(": Switch pattern: %i\n"), binary_state);

          applyPreset(preset_map.at(binary_state), CALL_MODE_BUTTON_PRESET);

          buttons[b].longPressed = buttons[b].pressedBefore; //save the last "long term" switch state
        }
          return true;
        }
      return false;
    }

};


// add more strings here to reduce flash memory usage
const char UsermodBinarySwitches::_name[]    PROGMEM = "BinarySwitches";
const char UsermodBinarySwitches::_enabled[] PROGMEM = "enabled";


static UsermodBinarySwitches binary_switches_usermod;
REGISTER_USERMOD(binary_switches_usermod);
