# TODO

- [x] Get the RTC Part to work: Put the ESP32 Into Deep Sleep and keep measuring the time.

- [x] The next big task is to get the service layer running. That is the data service which collects actual sensor readings, stores current readings, calculates stats, and makes data validation as well as then uses the display service (which is a library on top of the pure hardware display manager) to display the information. I started to implement this with a MOC, to be able to test the service layer logic without the need to interact with hardware. its currently untested and i stopped developing at this point

- [] Update the Markdown Note in Obsidian to reflect the current system design: /Users/davidwitulla/Obsidian/my-obsidian-vault/1 Projects/ESP32 BME280 Tracker/ESP32 BME280 Software.md
- Especially the Architectural Overview Mermaid Diagram should be on point. 


- [x] Also I want to implement and test the wifi manager. It should conntect to my wifi. 
- [x] Then The Connetivtiy Service should be implemented and tested. The connectivity Service ensures that the RTC has the correct time stamp  and does:
- ConnectivityService
• ensureTimeSync
• syncTimeIfNeeded
• > Connection management``
- [] The Datetime is currently not initialized in my projects. This needs to be addressed

- [] Add an additional "test hardware" enviroment for the pio test -e hardware command. I want to be able to execute this test independently of the main. Also i would like to have an executubale "pio run -t upload -e hardware" which uses all of the basic hardware on the lowest possible level together. With this step I also want to have the "test_embedded" directory renamed into "test_hardware"


- [] Write an Example which shows the current time (clock) on the display, utilizing the connetivity service for getting the time sync and the display for plotting the data. It should also write the data to the terminal, so we can see if the display is not working .,
----


- [] Add a Button to the Breadboard and then program the button to wake up from deep Sleep