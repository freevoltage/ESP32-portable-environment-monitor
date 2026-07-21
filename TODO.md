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
- [x] The Datetime is currently not initialized in my projects. This needs to be addressed
- [x] Add an additional "test hardware" enviroment for the pio test -e hardware command. I want to be able to execute this test independently of the main. Also i would like to have an executubale "pio run -t upload -e hardware" which uses all of the basic hardware on the lowest possible level together. With this step I also want to have the "test_embedded" directory renamed into "test_hardware"
- [] ReWrite the Display Example. The current Exmple is completly outdated and dont look good on the screen. 
- [] Write an Example which shows the current time (clock) on the display, utilizing the connetivity service for getting the time sync and the display for plotting the data. It should also write the data to the terminal, so we can see if the display is not working .,
- [] Tell the AI that it should regularly updated PORJECT_STATUS.md. Just in case it needs to compact its memory and may forogot important things. And for me to know here we are
- [] I want to have a docusarus (or other framework, pls research alternatives for docusaurus, I like modern ones) wiki for this project. It should be also regulary updated and part of AGENTS.md

----

## Hardware Stuff (Tasks for Me)

- [] Add a Button to the Breadboard and then program the button to wake up from deep Sleep

---

## My Tasks

The hiking station redesign is complete. The last remaining items are either your responsibility or low-priority cleanup:

### Your tasks:
- [] Test on real hardware (pio test -e main)
- [] Deploy firmware (pio run -t upload -e main)
- [] Verify button behavior, comfort logging, and graph display

### Low-priority cleanup (if you want me to work on something):
- [] Update AGENTS.md with current architecture (it still references old structure)
- [] storage_manager::cleanup() — marked TODO, not implemented
- [] Remove placeholder tests in test/test_native/ and test/test_lib/
- [] Fix display_manager::showConnectivityStatus() stub

### Future features (require hardware decisions):
- [] Battery/power management + solar
- [] Multi-sensor support
- [] Web dashboard + OTA updates
