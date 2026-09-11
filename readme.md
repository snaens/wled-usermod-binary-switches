# WLED binary switches usermod

This repository is a [WLED](https://github.com/wled/WLED) usermod for switching presets (with physical switches) in a binary integer pattern.
```
Example with 3 switches (either on (1) or off (0))
 --Switches--   --WLED--
        0 0 1 → Preset 1
        0 1 1 → Preset 3
        1 0 0 → Preset 4
```

## Getting started

### 2. Wire it into your WLED build



### 3. Share it

Tag your working version and add your usermod to the [Community Usermods page](https://kno.wled.ge/advanced/community-usermods/) by sending a PR to [WLED-Docs](https://github.com/wled/WLED-Docs).  Other developers can add your usermod to their builds by adding your repository to their build's `custom_usermods`!

```ini
custom_usermods =
  ${env:esp32dev.custom_usermods}
  https://github.com/you/wled-usermod-my_sensor.git#v1.0.0
```


## What's in this repo

**`library.json`** — PlatformIO library manifest. The `"libArchive": false` setting is required; without it the build will fail. Add any library dependencies here.

**`usermod_example.cpp`** — A fully annotated example covering all available lifecycle hooks:

| Method | When called |
|---|---|
| `setup()` | Once at boot, after config is loaded, before WiFi |
| `connected()` | Each time WiFi (re)connects |
| `loop()` | Every main loop iteration |
| `addToJsonInfo()` | When `/json/info` is requested |
| `addToJsonState()` / `readFromJsonState()` | On `/json/state` get/post |
| `addToConfig()` / `readFromConfig()` | Persistent settings in `cfg.json` |
| `appendConfigData()` | When the Usermod Settings page renders |
| `handleOverlayDraw()` | Just before each LED strip update |
| `handleButton()` | On button events |
| `onMqttMessage()` / `onMqttConnect()` | MQTT events |
| `onStateChange()` | When WLED state changes |

`REGISTER_USERMOD(instance)` at the bottom of the file handles self-registration — there is no `usermods_list.cpp` to edit.

For full documentation see the [WLED Custom Features](https://kno.wled.ge/advanced/custom-features/) page.
