# TODO

## Software Stuff (Tasks for AI)
- [x] Get the RTC Part to work: Put the ESP32 Into Deep Sleep and keep measuring the time.
- [x] The next big task is to get the service layer running. That is the data service which collects actual sensor readings, stores current readings, calculates stats, and makes data validation as well as then uses the display service (which is a library on top of the pure hardware display manager) to display the information. I started to implement this with a MOC, to be able to test the service layer logic without the need to interact with hardware. its currently untested and i stopped developing at this point

- [x] Update the Markdown Note in Obsidian to reflect the current system design: /Users/davidwitulla/Obsidian/my-obsidian-vault/1 Projects/ESP32 BME280 Tracker/ESP32 BME280 Software.md Especially the Architectural Overview Mermaid Diagram should be on point. 
- [x] Also I want to implement and test the wifi manager. It should conntect to my wifi. 
- [x] Then The Connetivtiy Service should be implemented and tested. The connectivity Service ensures that the RTC has the correct time stamp  and does:
- ConnectivityService
• ensureTimeSync
• syncTimeIfNeeded
• > Connection management``
- [x] The Datetime is currently not initialized in my projects. This needs to be addressed
- [x] Add an additional "test hardware" enviroment for the pio test -e hardware command. I want to be able to execute this test independently of the main. Also i would like to have an executubale "pio run -t upload -e hardware" which uses all of the basic hardware on the lowest possible level together. With this step I also want to have the "test_embedded" directory renamed into "test_hardware"
- [x] ReWrite the Display Example. The current Exmple is completly outdated and dont look good on the screen. 
- [x] Write an Example which shows the current time (clock) on the display, utilizing the connetivity service for getting the time sync and the display for plotting the data. It should also write the data to the terminal, so we can see if the display is not working .
- [x] Tell the AI that it should regularly updated PORJECT_STATUS.md. Just in case it needs to compact its memory and may forogot important things. And for me to know here we are
- [x] Fix test runner — missing colon in output format prevented PlatformIO from parsing PASS/FAIL/SKIP results
- [x] I want to have a docusarus (or other framework, pls research alternatives for docusaurus, I like modern ones) wiki for this project. It should be also regulary updated and part of AGENTS.md
- [] Update the Docu/Wiki
- [] Update the Readme
- [] Update Project Status
- [x] For Debuggiong Reasons I had the idea that the device is writing the log data on the sd card and or even to a file on my computer or both!?. I have no idea how to implement this, but this seems like a good idea for me.
- [x] In the TIME SYNC Menu. Button A should always just navigate between the points and button B should toggle between them or active sync when the SYNC option now  is selected. This scheme should be followed throughout the UI 
- [x] Add abort possibility in the OTA Menu (Button B exit, 120s timeout)
- [x] Add a Dashboard which shows current Temperature, Humidity, Altitude, Time, with sub-menus "Menu" and "Log Comfort"
- [x] Remove auto-sync from button wake (device responds instantly, sync on-demand from menu only)
- [x] Both buttons (A+B) = abort any operation, always returns to Dashboard
- [x] Add "Sleep" item to Dashboard
- [x] Comfort log protection: one log per day max (shows "Already logged today!")
- [x] Abbort possibilty in the "OTA" Menuswxe
- [x] Add a Dashboard which shows current Temperature, Hummidity, Altidude, Time, and with the Sub Menus "Menu, and LOG Comfort". help me with the User interface. I would like to work on it in a graphical way or at least some form of markup language or graphical user description. Put this descrption into the
- [x] ADd to the Documentation and the Code: BRESS BOTH BUTTONS AT ONCE to ABBORT ANY OPERATION (like sync or so. PRESSING BOTH BUTTONS WILL always return to the dashboard. )
- [x] Also add the Menu Item "SLeep" to the Dashboard. 
- [ ] It shouldnt be possible to log Multiple Sleep Entries during the Day or at least I want to discuss it how we could possible implement this. or protect the sleep comfort log from getting crowded with wrong and accidental logs. 
- [x] Remove the "log comfort" item from the menu. Pls build the menu according to the readme   
- [x] I want to add some On the devise sleep configurations. At least the time interval, but i am also open for more on the devise configs, like auto ntp sync time intervals
- [ ] For Some reason the device is not able to connect to the Wifi anymore even tho i am in the correct network
- [x] During the Wifi Connection in the terminal. the output should alternate between "." and ".." so I can see that its still trying to connect. WHen its just "." i dont know if its stuck or still working
- [x] The Abbort Operating during the "Connecting WiFi" does not work. Is this more complicated to implement? Does this need architectural re work?
- [x] I want to see an Icon on the Dashboard which indicated wether the Devise is connected to BLE or Wifi 
- [x] There is currently a lot of distance between the battery bar and the footer, move the battery bar closer to the footer.
- [ ] The Bun run dev command fails. PLs AI run the command yourself and check the error log
----

## Hardware Stuff (Tasks for Me)

- [x] Add a Button to the Breadboard and then program the button to wake up from deep Sleep

---

## My Tasks

The hiking station redesign is complete. The last remaining items are either your responsibility or low-priority cleanup:

### Your tasks:
- [x] Test on real hardware (pio test -e main)
- [x] Deploy firmware (pio run -t upload -e main)
- [] Verify button behavior, comfort logging, and graph display

### Low-priority cleanup (if you want me to work on something):
- [x] Update AGENTS.md with current architecture (it still references old structure)
- [x] storage_manager::cleanup() — marked TODO, not implemented
- [x] Remove placeholder tests in test/test_native/ and test/test_lib/
- [x] Fix display_manager::showConnectivityStatus() stub

### Completed features (committed):
- [x] Battery management (MAX17048 fuel gauge, I2C power control, TFT battery display)
- [x] OTA updates (ElegantOTA + AsyncWebServer, partition table, TFT progress, auth)
- [x] BLE time sync (NimBLE-Arduino, 5 configurable sync modes, LittleFS persistence)

### Future features (require hardware decisions):
- [] Multi-sensor support
- [] Web dashboard (WiFi status page, live data, config)
- [] BLE companion phone app (see BLE_INTERFACE.md)



## Verification Results
- After uploading the main. The devcise does not stay in deep sleep. It automatically boots up after a few seconds and goes back to the dashboard. Even when i select the "sleep" option from the dashboard, the device goes to sleep but immediatly wakes up again and to the dashboard. The expected behaviour would be, that I need to wake it up from the Dashboard using the BUTTON B. 
- Menu: In the Menu there is not "return path". ( ah well there is, by pressing both buttons. but would be nice to have a dedicated "back" option)
- Another problem is that the measured values for temperature and hummidty are very incorrect. The measurement results are always higher than the reality, by a coupld of degrees. I dont know if it is possible to correct it using the BME280. If this is an offset thing, or calibration thing. Also the altidude is way to high. I am in berlin, which should be arround 34-38m above ocean, but the display shows 115

- [x] I had the idea for an additional "calendar" view. Or Logging View. Where I get the logging data vissible for the past days. (Maybe since the beginning of my hike) Then I would need some way to let the device know when my hike started. But well no I think this is too compliicated. I think it just just use whatever is saved on the SD Card. But it needs to use the date of the savings, Because maybe I forogot to log on a day and then it should not be confused. And this view could also allow me to re-open a day and change its logging value? That would be cool right?

- [x] The Calendar view should the date buggy. it currently shows it like "JULAUGSEPOCTNOV: Comfort" It should show the actual date in DD-MM-YY format.
Also the "global go back key combbo of BUTTON A + BUTTON B does not work here at all. so i am stuck in the calendar mode.

I just currently synced the Time via WIFI. then pressed "RESET" and the time sync was gone and the time displayed was 1970 again. THIS IA CRITICAL BUG 


