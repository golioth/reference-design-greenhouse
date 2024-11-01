..
   Copyright (c) 2024 Golioth, Inc.
   SPDX-License-Identifier: Apache-2.0

Golioth Greenhouse Controller Reference Design
##############################################

This repository contains the firmware source code and `pre-built release
firmware images <releases_>`_ for the Golioth Greenhouse Controller reference
design.

The full project details are available on the `Greenhouse Controller Project Page`_.

Supported Hardware
******************

This firmware can be built for a variety of supported hardware platforms.

.. pull-quote::
   [!IMPORTANT]

   In Zephyr, each of these different hardware variants is given a unique
   "board" identifier, which is used by the build system to generate firmware
   for that variant.

   When building firmware using the instructions below, make sure to use the
   correct Zephyr board identifier that corresponds to your follow-along
   hardware platform.

.. list-table:: **Follow-Along Hardware**
   :header-rows: 1

   * - Hardware
     - Zephyr Board
     - Follow-Along Guide

   * - .. image:: images/golioth-greenhouse-controller-fah-nrf9160dk.jpg
          :width: 240
     - ``nrf9160dk/nrf9160/ns``
     - `nRF9160 DK Follow-Along Guide`_

.. list-table:: **Custom Golioth Hardware**
   :header-rows: 1

   * - Hardware
     - Zephyr Board
     - Project Page
   * - .. image:: images/Greenhouse_controller.png
          :width: 240
     - ``aludel_mini/nrf9160/ns``
     - `Greenhouse Controller Project Page`_

Firmware Overview
*****************

This is a Reference Design for a Greenhouse controller that monitors
environmental factors like light intensity, temperature, humidity, and pressure
and uses relays to actuate grow lights and ventilation.

Specifically, the following parameters can be monitored:

* 💦 Relative humidity (%RH)
* 🌡️ Temperature (°C)
* 💨 Pressure (kPa)
* ☀️  Light Intensity (LUX)
* 🔴 Red Light Value
* 🟢 Green Light Value
* 🔵 Blue Light Value

The sensor values are uploaded to the LightDB Stream database in the Golioth
Cloud. The sensor sampling frequency and other sensor parameters are remotely
configurable via the Golioth Settings service.

Add Pipeline to Golioth
***********************

Golioth uses `Pipelines`_ to route stream data. This gives you flexibility to change your data
routing without requiring updated device firmware.

Whenever sending stream data, you must enable a pipeline in your Golioth project to configure how
that data is handled. Add the contents of ``pipelines/cbor-to-lightdb.yml`` as a new pipeline as
follows (note that this is the default pipeline for new projects and may already be present):

   1. Navigate to your project on the Golioth web console.
   2. Select ``Pipelines`` from the left sidebar and click the ``Create`` button.
   3. Give your new pipeline a name and paste the pipeline configuration into the editor.
   4. Click the toggle in the bottom right to enable the pipeline and then click ``Create``.

All data streamed to Golioth in CBOR format will now be routed to LightDB Stream and may be viewed
using the web console. You may change this behavior at any time without updating firmware simply by
editing this pipeline entry.

Golioth Features
****************

This firmware implements the following features from the Golioth Zephyr SDK:

- `Device Settings Service <https://docs.golioth.io/firmware/zephyr-device-sdk/device-settings-service>`_
- `LightDB State Client <https://docs.golioth.io/firmware/zephyr-device-sdk/light-db/>`_
- `LightDB Stream Client <https://docs.golioth.io/firmware/zephyr-device-sdk/light-db-stream/>`_
- `Logging Client <https://docs.golioth.io/firmware/zephyr-device-sdk/logging/>`_
- `Over-the-Air (OTA) Firmware Upgrade <https://docs.golioth.io/firmware/device-sdk/firmware-upgrade>`_
- `Remote Procedure Call (RPC) <https://docs.golioth.io/firmware/zephyr-device-sdk/remote-procedure-call>`_

Device Settings Service
-----------------------

The following settings should be set in the Device Settings menu of the
`Golioth Console`_.

``LOOP_DELAY_S``
   Adjusts the delay between sensor readings. Set to an integer value (seconds).

   Default value is ``60`` seconds.

``LIGHT_AUTO``
   Enables or disables the automatic grow light control.
   Set to a boolean value.

   Default value is ``true``.

``TEMP_AUTO``
   Enables or disables the automatic ventilation control.
   Set to a boolean value.

   Default value is ``true``.

``LIGHT_THRESH``
   Clear Light Intensity threshold for automatic grow light control.

   Default value is ``900`` LUX.


``TEMP_THRESH``
   Temperature threshold for automatic ventilation control.

   Default value is ``21.5`` °C.

LightDB Stream Service
----------------------

Sensor data is periodically sent to the following ``sensor/*`` endpoints of the
LightDB Stream service:

* ``sensor/ligth/int``: Clear Light Intensity (LUX)
* ``sensor/ligth/r``: Red Light Value
* ``sensor/ligth/g``: Green Light Value
* ``sensor/ligth/b``: Blue Light Value
* ``sensor/weather/humidity``:Humidity (%RH)
* ``sensor/weather/pressure``: Pressure (kPa)
* ``sensor/weather/temp``: Temperature (°C)

Battery voltage and level readings are periodically sent to the following
``battery/*`` endpoints:

* ``battery/batt_v``: Battery Voltage (V)
* ``battery/batt_lvl``: Battery Level (%)

LightDB State Service
---------------------

The concept of Digital Twin is demonstrated with the LightDB State ``light``
and ``vent`` variables that are members of the ``desired`` and ``state``
endpoints. Variables ``light`` and ``vent`` correspond to relay state,
where ``0`` means the relay is open, and ``1`` that the relay is closed.
Changing the values of ``desired`` endpoints will have effect only if
``LIGHT_AUTO`` or ``TEMP_AUTO`` are set to ``false`` in the Settings Service.

* ``desired`` values may be changed from the cloud side. The device will recognize
  these, validate them for ``1`` and ``0``, and then reset these endpoints
  to ``-1``

* ``state`` values will be updated by the device whenever a valid value is
  received from the ``desired`` endpoints. The cloud may read the ``state``
  endpoints to determine device status, but only the device should ever write to
  the ``state`` endpoints.

Remote Procedure Call (RPC) Service
-----------------------------------

The following RPCs can be initiated in the Remote Procedure Call menu of the
`Golioth Console`_.

``get_network_info``
   Query and return network information.

``reboot``
   Reboot the system.

``set_log_level``
   Set the log level.

   The method takes a single parameter which can be one of the following integer
   values:

   * ``0``: ``LOG_LEVEL_NONE``
   * ``1``: ``LOG_LEVEL_ERR``
   * ``2``: ``LOG_LEVEL_WRN``
   * ``3``: ``LOG_LEVEL_INF``
   * ``4``: ``LOG_LEVEL_DBG``

Building the firmware
*********************

The firmware build instructions below assume you have already set up a Zephyr
development environment and have some basic familiarity with building firmware
using the Zephyr Real Time Operating System (RTOS).

If you're brand new to building firmware with Zephyr, you will need to follow
the `Zephyr Getting Started Guide`_ to install the Zephyr SDK and related
dependencies.

We also provide free online `Developer Training`_ for Zephyr at:

https://training.golioth.io/docs/zephyr-training

.. pull-quote::
   [!IMPORTANT]

   Do not clone this repo using git. Zephyr's ``west`` meta-tool should be used
   to set up your local workspace.

Install the Python virtual environment (recommended)
====================================================

.. code-block:: shell

   cd ~
   mkdir golioth-reference-design-greenhouse
   python -m venv golioth-reference-greenhouse/.venv
   source golioth-reference-design-greenhouse/.venv/bin/activate
   pip install wheel west

Use ``west`` to initialize and install
======================================

.. code-block:: shell

   cd ~/golioth-reference-design-greenhouse
   west init -m git@github.com:golioth/reference-design-greenhouse.git .
   west update
   west zephyr-export
   pip install -r deps/zephyr/scripts/requirements.txt

Building the application
************************

Build the Zephyr sample application for the `Nordic nRF9160 DK`_
(``nrf9160dk_nrf9160_ns``) from the top level of your project. After a
successful build you will see a new ``build`` directory. Note that any changes
(and git commits) to the project itself will be inside the ``app`` folder. The
``build`` and ``deps`` directories being one level higher prevents the repo from
cataloging all of the changes to the dependencies and the build (so no
``.gitignore`` is needed).

Prior to building, update ``VERSION`` file to reflect the firmware version number you want to assign
to this build. Then run the following commands to build and program the firmware onto the device.


.. pull-quote::
   [!IMPORTANT]

   You must perform a pristine build (use ``-p`` or remove the ``build`` directory)
   after changing the firmware version number in the ``VERSION`` file for the change to take effect.

.. code-block:: text

   $ (.venv) west build -p -b nrf9160dk/nrf9160/ns --sysbuild app
   $ (.venv) west flash

Configure PSK-ID and PSK using the device shell based on your Golioth
credentials and reboot:

.. code-block:: text

   uart:~$ settings set golioth/psk-id <my-psk-id@my-project>
   uart:~$ settings set golioth/psk <my-psk>
   uart:~$ kernel reboot cold

OTA Firmware Update
*******************

This application includes the ability to perform Over-the-Air (OTA) firmware updates:

1. Update the version number in the `VERSION` file and perform a pristine (important) build to
   incorporate the version change.
2. Upload the `build/app/zephyr/zephyr.signed.bin` file as an artifact for your Golioth project
   using `main` as the package name.
3. Create and roll out a release based on this artifact.

Visit `the Golioth Docs OTA Firmware Upgrade page`_ for more info.

External Libraries
******************

The following code libraries are installed by default. If you are not using the
custom hardware to which they apply, you can safely remove these repositories
from ``west.yml`` and remove the includes/function calls from the C code.

* `golioth-zephyr-boards`_ includes the board definitions for the Golioth
  Aludel-Mini
* `zephyr-network-info`_ is a helper library for querying, formatting, and
  returning network connection information via Zephyr log or Golioth RPC

.. _Golioth Console: https://console.golioth.io
.. _Nordic nRF9160 DK: https://www.nordicsemi.com/Products/Development-hardware/nrf9160-dk
.. _Pipelines: https://docs.golioth.io/data-routing
.. _the Golioth Docs OTA Firmware Upgrade page: https://docs.golioth.io/firmware/golioth-firmware-sdk/firmware-upgrade/firmware-upgrade
.. _golioth-zephyr-boards: https://github.com/golioth/golioth-zephyr-boards
.. _MikroE Arduino UNO click shield: https://www.mikroe.com/arduino-uno-click-shield
.. _MikroE Weather Click: https://www.mikroe.com/weather-click
.. _Greenhouse Controller Project Page: https://projects.golioth.io/reference-designs/greenhouse-controller
.. _nRF9160 DK Follow-Along Guide: https://projects.golioth.io/reference-designs/greenhouse-controller/guide-nrf9160-dk
.. _releases: https://github.com/golioth/
.. _Reference Design Template: https://github.com/golioth/reference-design-template
.. _Zephyr Getting Started Guide: https://docs.zephyrproject.org/latest/develop/getting_started/
.. _Developer Training: https://training.golioth.io
.. _SemVer: https://semver.org
.. _zephyr-network-info: https://github.com/golioth/zephyr-network-info
