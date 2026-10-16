# USB install interrupted — excluded

The authorized I2304 phone initially appeared over USB as 10BE1Y2EL40003J.
Preflight and stopped-save backup completed. Streamed replace-install then
returned exit 1, and ADB reported the device offline. No uninstall or data-clear
fallback was attempted. No launch override or wake-setting write was performed.

Post-install APK and save verification could not run. The original result.json
uses false defaults for unperformed checks; these do NOT prove that saves,
commandline or wake changed. Treat the installed version and preservation as
unverified until reconnection. Retain before-stopped.json and checkpoint backups.
The installer now records unperformed verification as null / not-run, with all
50 device-tool tests passing. Historical receipt remains unmodified.

ADB reconnect-offline was attempted once. Phone subsequently remained offline;
no wireless service was advertised. Next: verify installed APK, all saved hashes,
original override and wake against this run BEFORE any new install or launch.
