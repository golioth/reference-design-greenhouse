# Golioth Greenhouse Controller Reference Design

The Greenhouse controller demonstrates how to monitor environmental
factors like light intensity, temperature, humidity, and pressure using
the Golioth IoT platform. Specifically, the following parameters can be
monitored:

  - 💦 Relative humidity (%RH)
  - 🌡️ Temperature (°C)
  - 💨 Pressure (kPa)
  - ☀️ Light Intensity (LUX)
  - 🔴 Red Light Value
  - 🟢 Green Light Value
  - 🔵 Blue Light Value

Relays are used to remotely actuate grow lights and ventilation
automatically or on demand. From the Golioth web console you may deploy
OTA firmware updates, adjust device Settings, and issue Remote Procedure
Calls (RPCs).

Business use cases and hardware build details are available on [the
Greenhouse Controller Project
Page](https://projects.golioth.io/reference-designs/greenhouse-controller/).

This repository contains the firmware source code and [pre-built release
firmware
images](https://github.com/golioth/reference-design-greenhouse/releases).

## Supported Hardware

- Nordic nRF9160-DK
- Golioth Aludel Elixir
- Golioth Aludel Mini

### Additional Sensors/Components

- Broadcom ADPS-9960 digital ambient light sensor
- Bosch BME280 digital humidity, pressure, and temperature Sensors
- OMRON G6D1AASIDC5s minature relay (x2)

## Golioth Features

This app implements:

  - [Device Settings
    Service](https://docs.golioth.io/firmware/golioth-firmware-sdk/device-settings-service)
  - [Remote Procedure Call
    (RPC)](https://docs.golioth.io/firmware/golioth-firmware-sdk/remote-procedure-call)
  - [Stream
    Client](https://docs.golioth.io/firmware/golioth-firmware-sdk/stream-client)
  - [LightDB State
    Client](https://docs.golioth.io/firmware/golioth-firmware-sdk/light-db-state/)
  - [Over-the-Air (OTA) Firmware
    Upgrade](https://docs.golioth.io/firmware/golioth-firmware-sdk/firmware-upgrade/firmware-upgrade)
  - [Backend
    Logging](https://docs.golioth.io/device-management/logging/)

### Settings Service

The following settings should be set in the Device Settings menu of the
[Golioth Console](https://console.golioth.io).

  - `LOOP_DELAY_S`
    Adjusts the delay between sensor readings. Set to an integer value
    (seconds).

    Default value is `60` seconds.

  - `LIGHT_AUTO`
    Enables or disables the automatic grow light control. Set to a
    boolean value.

    Default value is `true`.

  - `TEMP_AUTO`
    Enables or disables the automatic ventilation control. Set to a
    boolean value.

    Default value is `true`.

  - `LIGHT_THRESH`
    Clear Light Intensity threshold for automatic grow light control.

    Default value is `900` LUX.

  - `TEMP_THRESH`
    Temperature threshold for automatic ventilation control.

    Default value is `21.5` °C.

### Remote Procedure Call (RPC) Service

The following RPCs can be initiated in the Remote Procedure Call menu of
the [Golioth Console](https://console.golioth.io).

  - `get_network_info`
    Query and return network information.

  - `reboot`
    Reboot the system.

  - `set_log_level`
    Set the log level.

    The method takes a single parameter which can be one of the
    following integer values:

      - `0`: `LOG_LEVEL_NONE`
      - `1`: `LOG_LEVEL_ERR`
      - `2`: `LOG_LEVEL_WRN`
      - `3`: `LOG_LEVEL_INF`
      - `4`: `LOG_LEVEL_DBG`

### Time-Series Stream data

Sensor data is periodically sent to the following `sensor/*` paths of
the LightDB Stream service:

  - `sensor/light/int`: Clear Light Intensity (LUX)
  - `sensor/light/r`: Red Light Value
  - `sensor/light/g`: Green Light Value
  - `sensor/light/b`: Blue Light Value
  - `sensor/weather/humidity`:Humidity (%RH)
  - `sensor/weather/pressure`: Pressure (kPa)
  - `sensor/weather/temp`: Temperature (°C)

``` json
{
  "sensor": {
    "light": {
      "b": 21,
      "g": 26,
      "int": 79,
      "r": 43
    },
    "weather": {
      "hum": 51.5957,
      "pre": 97.726843,
      "tem": 26.47
    }
  }
}
```

If your board includes a battery, voltage and level readings
will be sent to the `battery` path.

> [!NOTE]
> Your Golioth project must have a Pipeline enabled to receive this
> data. See the [Add Pipeline to Golioth](#add-pipeline-to-golioth)
> section below.

### Stateful Data (LightDB State)

The concept of Digital Twin is demonstrated with the LightDB State
`light` and `vent` variables that are subpaths of the `desired` and
`state` paths.

  - `desired` values may be changed from the cloud side. The device will
    recognize these, validate them for \[0..1\] bounding, and then
    switch the appropriate relay on or off based on this value. The
    device will then reset these values to `-1`. Note that these values
    will be overridden by the application if the Settings system has the
    auto feature enabled.
  - `state` values will be updated by the device to reflect the device's
    actual state for the two relays. The cloud may read the `state`
    paths to determine device status. In this arrangement, only the
    device should ever write to the `state` paths.

``` json
{
  "desired": {
    "light": -1,
    "vent": -1
  },
  "state": {
    "light": 0,
    "vent": 0
  }
}
```

But default the state values will be `0` and `1`. Try updating the
`desired` values and observe how the device updates its state.

### OTA Firmware Update

This application includes the ability to perform Over-the-Air (OTA)
firmware updates. To do so, you need a binary compiled with a different
version number than what is currently running on the device.

> [!NOTE]
> If a newer release is available than what your device is currently
> running, you may download the pre-compiled binary that ends in
> `_update.bin` and use it in step 2 below.

1. Update the version number in the `VERSION` file and perform a
   pristine (important) build to incorporate the version change.
2. Upload the `build/app/zephyr/zephyr.signed.bin` file as a Package for
   your Golioth project.

   - Use `main` as the package name.
   - Use the same version number from step 1.

3. Create a Cohort and add your device to it.
4. Create a Deployment for your Cohort using the package name and
   version number from step 2.
5. Devices in your Cohort will automatically upgrade to the most
   recently deployed firmware.

Visit [the Golioth Docs OTA Firmware Upgrade
page](https://docs.golioth.io/firmware/golioth-firmware-sdk/firmware-upgrade/firmware-upgrade)
for more info.

### Further Information in Header Files

Please refer to the comments in each header file for a
service-by-service explanation of this template.

## Add Pipeline to Golioth

Golioth uses [Pipelines](https://docs.golioth.io/data-routing) to route
stream data. This gives you flexibility to change your data routing
without requiring updated device firmware.

Whenever sending stream data, you must enable a pipeline in your Golioth
project to configure how that data is handled. Add the contents of
`pipelines/json-to-lightdb.yml` as a new pipeline as follows (note that
this is the default pipeline for new projects and may already be
present):

1.  Navigate to your project on the Golioth web console.
2.  Select `Pipelines` from the left sidebar and click the `Create`
    button.
3.  Give your new pipeline a name and paste the pipeline configuration
    into the editor.
4.  Click the toggle in the bottom right to enable the pipeline and
    then click `Create`.

All data streamed to Golioth in JSON format will now be routed to
LightDB Stream and may be viewed using the web console. You may change
this behavior at any time without updating firmware simply by editing
this pipeline entry.

## Local set up

> [!IMPORTANT]
> Do not clone this repo using git. Zephyr's `west` meta tool should be
> used to set up your local workspace.

### Install the Python virtual environment (recommended)

``` shell
cd ~
mkdir golioth-reference-design-greenhouse
python -m venv golioth-reference-greenhouse/.venv
source golioth-reference-design-greenhouse/.venv/bin/activate
pip install wheel west ecdsa
```

### Use `west` to initialize and install

``` shell
cd ~/golioth-reference-design-greenhouse
west init -m git@github.com:golioth/reference-design-greenhouse.git .
west update
west zephyr-export
pip install -r deps/zephyr/scripts/requirements.txt
```

## Building the application

Build the Zephyr sample application for the [Nordic nRF9160
DK](https://www.nordicsemi.com/Products/Development-hardware/nrf9160-dk)
(`nrf9160dk_nrf9160_ns`) from the top level of your project. After a
successful build you will see a new `build` directory. Note that any
changes (and git commits) to the project itself will be inside the `app`
folder. The `build` and `deps` directories being one level higher
prevents the repo from cataloging all of the changes to the dependencies
and the build (so no `.gitignore` is needed).

Prior to building, update `VERSION` file to reflect the firmware version
number you want to assign to this build. Then run the following commands
to build and program the firmware onto the device.

> [!WARNING]
> You must perform a pristine build (use `-p` or remove the `build`
> directory) after changing the firmware version number in the `VERSION`
> file for the change to take effect.

``` text
$ (.venv) west build -p -b nrf9160dk/nrf9160/ns --sysbuild app
$ (.venv) west flash
```

Configure PSK-ID and PSK using the device shell based on your Golioth
credentials and reboot:

``` text
uart:~$ settings set golioth/psk-id <my-psk-id@my-project>
uart:~$ settings set golioth/psk <my-psk>
uart:~$ kernel reboot cold
```

## External Libraries

The following code libraries are installed by default. If you are not
using the custom hardware to which they apply, you can safely remove
these repositories from `west.yml` and remove the includes/function
calls from the C code.

  - [golioth-zephyr-boards](https://github.com/golioth/golioth-zephyr-boards)
    includes the board definitions for the Golioth Aludel-Mini
  - [zephyr-network-info](https://github.com/golioth/zephyr-network-info)
    is a helper library for querying, formatting, and returning network
    connection information via Zephyr log or Golioth RPC

## Have Questions?

Please get in touch with Golioth engineers by starting a new thread on
the [Golioth Forum](https://forum.golioth.io/).
