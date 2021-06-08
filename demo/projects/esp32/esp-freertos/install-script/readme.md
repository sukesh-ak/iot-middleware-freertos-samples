# Setup script for ESP devices

This folder contains two scripts to run in WSL (Windows Subsystem for Linux).
The esp-setup.sh script will install the toolchain and tools needed to compile project for ESP board (to run once at the initial setup).
The esp-set-devenv.sh will set the path and environment variables (to run each time you start the WSL).

## Using the scripts

### Initial Setup

To setup your environment for ESP32 the first time on WSL, we have provided a setup script. To run this script, go to where you have cloned this repo. Go into the ***install-script*** folder by typing: `cd install-script`
From this folder run: `source ./esp-setup.sh`.

Once run, you're ready to build any of these code samples.

### Restore or Changing Dev Environment

When you close and restart WSL, the path information is not saved in your environment.  If you wish to save your path information you can write the path information into your bash profile. However, if you switch versions often you may not wish to store this information the profile. 
To make it easier to restore the correct path information and environment variables, a script is provided for convenience. 

The script has 2 options to restore or change your path and update your environment variables: 

* **Legacy**: for ESP32 stable version 3.3
* **Stable**: for ESP32 latest version 4.2

To run the script to restore or switch to the Legacy version type `source ./esp-set-devenv.sh` or `source ./esp-set-devenv.sh stable`

To run the script for Stable version, type `source ./esp-set-devenv.sh latest`.


> You can run the aforementioned script at any time to switch between versions, once your environment has been setup via the initial setup script.
