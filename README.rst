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

.. list-table:: **Custom Golioth Hardware**
   :header-rows: 1

   * - Hardware
     - Zephyr Board
     - Project Page
   * - .. image:: images/Greenhouse_controller.png
          :width: 240
     - ``aludel_mini_v1_sparkfun9160_ns``
     - `Greenhouse Controller Project Page`_

############################

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

Supported Golioth Zephyr SDK Features
=====================================

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

Create a Python virtual environment (recommended)
=================================================

.. code-block:: shell

   cd ~
   mkdir golioth-reference-design-greenhouse
   python -m venv golioth-reference-design-greenhouse/.venv
   source golioth-reference-design-greenhouse/.venv/bin/activate

Install ``west`` meta-tool
==========================

.. code-block:: shell

   pip install wheel west

Use ``west`` to initialize the workspace and install dependencies
=================================================================

.. code-block:: console

   cd ~/golioth-reference-design-greenhouse
   west init -m git@github.com:golioth/reference-design-greenhouse.git .
   west update
   west zephyr-export
   pip install -r deps/zephyr/scripts/requirements.txt

Build the firmware
==================

Build the Zephyr firmware from the top-level workspace of your project. After a
successful build you will see a new ``build/`` directory.

Note that this git repository was cloned into the ``app`` folder, so any changes
you make to the application itself should be committed inside this repository.
The ``build`` and ``deps`` directories in the root of the workspace are managed
outside of this git repository by the ``west`` meta-tool.

Prior to building, update ``CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION`` in the ``prj.conf`` file to
reflect the firmware version number you want to assign to this build. Then run the following
commands to build and program the firmware onto the device.

.. pull-quote::
   [!IMPORTANT]

   When running the commands below, make sure to replace the placeholder
   ``<your_zephyr_board_id>`` with the actual Zephyr board from the table above
   that matches your follow-along hardware.

.. code-block:: text

   $ (.venv) west build -p -b <your_zephyr_board_id> app

For example, to build firmware version ``1.2.3`` for Golioth's ``Aludel Mini v1``:

.. code-block:: text

   $ (.venv) west build -p -b aludel_mini_v1_sparkfun9160_ns app

Flash the firmware
==================

.. code-block:: text

   $ (.venv) west flash

Provision the device
====================

In order for the device to securely authenticate with the Golioth Cloud, we need
to provision the device with a pre-shared key (PSK). This key will persist
across reboots and only needs to be set once after the device firmware has been
programmed. In addition, flashing new firmware images with ``west flash`` should
not erase these stored settings unless the entire device flash is erased.

Configure the PSK-ID and PSK using the device UART shell and reboot the device:

.. code-block:: text

   uart:~$ settings set golioth/psk-id <my-psk-id@my-project>
   uart:~$ settings set golioth/psk <my-psk>
   uart:~$ kernel reboot cold


External Libraries
******************

The following code libraries are installed by default. If you are not using the
custom hardware to which they apply, you can safely remove these repositories
from ``west.yml`` and remove the includes/function calls from the C code.

* `golioth-zephyr-boards`_ includes the board definitions for the Golioth
  Aludel-Mini
* `zephyr-network-info`_ is a helper library for querying, formatting, and
  returning network connection information via Zephyr log or Golioth RPC

Pulling in updates from the Reference Design Template
*****************************************************

This reference design was forked from the `Reference Design Template`_ repo. We
recommend the following workflow to pull in future changes:

* Setup

  * Create a ``template`` remote based on the Reference Design Template
    repository

* Merge in template changes

  * Fetch template changes and tags
  * Merge template release tag into your ``main`` (or other branch)
  * Resolve merge conflicts (if any) and commit to your repository

.. code-block:: shell

   # Setup
   git remote add template https://github.com/golioth/reference-design-template.git
   git fetch template --tags

   # Merge in template changes
   git fetch template --tags
   git checkout your_local_branch
   git merge template_v1.0.0

   # Resolve merge conflicts if necessary
   git add resolved_files
   git commit

.. _Golioth Console: https://console.golioth.io
.. _golioth-zephyr-boards: https://github.com/golioth/golioth-zephyr-boards
.. _MikroE Arduino UNO click shield: https://www.mikroe.com/arduino-uno-click-shield
.. _MikroE Weather Click: https://www.mikroe.com/weather-click
.. _Greenhouse Controller Project Page: https://projects.golioth.io/reference-designs/greenhouse-controller
.. _releases: https://github.com/golioth/
.. _Reference Design Template: https://github.com/golioth/reference-design-template
.. _Zephyr Getting Started Guide: https://docs.zephyrproject.org/latest/develop/getting_started/
.. _Developer Training: https://training.golioth.io
.. _SemVer: https://semver.org
.. _zephyr-network-info: https://github.com/golioth/zephyr-network-info
