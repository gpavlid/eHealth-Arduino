/*
  iHealth implementation of web server based on eHealth platform to serve biometric data
  based on the e-Health platform by Cooking Hacks
  http://www.cooking-hacks.com/documentation/tutorials/ehealth-biometric-sensor-platform-arduino-raspberry-pi-medical
  
  ↄ⃝ 2014 George Pavlidis, http://georgepavlides.info
  
  The code uses the following libraries 
     SPI, Ethernet, PinChangeInt, eHealth
 */

#include <SPI.h>
#include <Ethernet.h>
#include <PinChangeInt.h>
#include <eHealth.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network: 
//
// !!!! REMEMBER TO CHANGE HEXxxx with the appropriate HEX numbers !!!!
//
byte mac[] = { HEX1, HEX2, HEX3, HEX4, HEX5, HEX6 };
//IPAddress ip(192,168,0,1); // this is not needed

// Initialize the Ethernet server library
// (port 80 is default for HTTP):
EthernetServer server(80);

// global variable to be used with pulse oximeter
int cont = 0;

// initialize the debug variables
const boolean debug = 1;
boolean reportServer = 1;

// initialization
void setup() {
  // initialize the glycometer
  eHealth.readGlucometer();
  // initialize blood pressure sensor
  eHealth.readBloodPressureSensor();
  // initialize the SPO2 sensor
  eHealth.initPulsioximeter();
  //Attach the interrupt for using the pulsioximeter.   
  PCintPort::attachInterrupt(6, readPulsioximeter, RISING);
  // intialize position sensor
  eHealth.initPositionSensor();
  // start the Ethernet connection and the server:
  Ethernet.begin(mac);
  server.begin();
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
}

// main program
void loop() {

	// initialize body posture strings
	char *body_positions[] = { "Supine position", "Left lateral decubitus", "Rigth lateral decubitus", "Prone position", "Stand or sit position" };
	// initialize constant server strings
	const char *sensorTitle[] = { "Airflow", "Body temperature", "ECG", "Oxygen saturation", "Heart rate", 
																	"Systolic pressure", "Diastolic pressure", "Pulse", "EMG", "Skin conductance", 
																	"Skin resistance", "Skin conductance voltage", "Glycose level", "Body posture" };
	const char *sensorMetric[] = { "%", "deg.C", "%", "%", "bpm", "mmHg", "mmHg", "bpm", "%",
																		"μs", "KOhms", "V", "mg/dL", "" };
	// variables declaration
	uint8_t i, air, SPO2, BPM, numMeasurements, systolicPressure, diastolicPressure, pulsePressure, glycose, body_position;
	float temperature, ECG, EMG, skinConductance, skinResistance, skinConductanceVoltage;
	char *body_posture;
	boolean currentLineIsBlank;
	float sensorValue[13];
	char *sensorValuePosture;

  // debugging...show the server IP in the serial interface
  if (reportServer) {
    Serial.println("*");
    Serial.print("*** server is at ");
    Serial.print(Ethernet.localIP());
    Serial.println(" ***");
    delay(10);
    reportServer = 0;
  }
      
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    if (debug) Serial.println("************ client connected ************");

    // read airflow sensor
    // returns a value from 0 to 1024
    // air = eHealth.getAirFlow()*100/1025;
    sensorValue[0] = eHealth.getAirFlow()*100/1025;
    
    // read body temperature sensor
    // return a float with the last value of temperature measured  in degrees Celcius
    // temperature = eHealth.getTemperature();
    sensorValue[1] = eHealth.getTemperature();
    
    // read ECG sensor
    // returns an analogic value in volts (0 – 5) to represent the ECG wave form
    // ECG = eHealth.getECG()*100/5;
    sensorValue[2] = eHealth.getECG()*100/5;

    // read SPO2 sensor
    // returns the oxygen saturation in % and the heart rate in BPM
    // SPO2 = eHealth.getOxygenSaturation();
    sensorValue[3] = eHealth.getOxygenSaturation();
    // BPM = eHealth.getBPM();
    sensorValue[4] = eHealth.getBPM();
    
    // read blood pressure sensor
    // returns an array of three values that correspond to the latest measurments of 
    // the systolic and diastolic pressure in mmHG and the pulse in BPM
    numMeasurements = eHealth.getBloodPressureLength();
		i = numMeasurements-1;  // just get the last measurement stored in the device
		sensorValue[5] = 30+eHealth.bloodPressureDataVector[i].systolic;
		sensorValue[6] = eHealth.bloodPressureDataVector[i].diastolic;
		sensorValue[7] = eHealth.bloodPressureDataVector[i].pulse;

    // read the EMG sensor
    // returns an analogic value in volts (read 0 – 1023 by the ADC) to represent the EMG wave form
    // EMG = eHealth.getEMG()*100/1024;
    sensorValue[8] = eHealth.getEMG()*100/1024;
   
    // read galvanic skin response sensor
    // returns the value of the skin resistance in Ohms and the skin conductance in μs 
    // along with the conductance voltage in V
    // skinConductance = eHealth.getSkinConductance();
    sensorValue[9] = eHealth.getSkinConductance();
    // skinResistance = eHealth.getSkinResistance()/1000;  // converted to KOhms
    sensorValue[10] = eHealth.getSkinResistance()/1000;  // converted to KOhms
    // skinConductanceVoltage = eHealth.getSkinConductanceVoltage();
    sensorValue[11] = eHealth.getSkinConductanceVoltage();
    
    // read glycose sensor
    // returns the glycose level in mg/dL
    numMeasurements = eHealth.getGlucometerLength();
		i = numMeasurements-1;  // just get the last measurement stored in the device
		sensorValue[12] = eHealth.glucoseDataVector[i].glucose;
    
    // read position sensor
    // returns the code number of the position that was identified
    /* position code number definition
        1 == Supine position.
        2 == Left lateral decubitus.
        3 == Rigth lateral decubitus.
        4 == Prone position.
        5 == Stand or sit position.
    */ 
    // body_position = eHealth.getBodyPosition();
    // body_posture = body_positions[ body_position];
    sensorValuePosture = body_positions[ eHealth.getBodyPosition()];
   
    // an http request ends with a blank line
    currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: application/json"));
          client.println(F("Connection: close"));  // the connection will be closed after completion of the response
          client.println();
          // output the value of each analog input pin as a json-p object
          client.println("{\"healthData\":[");
          for (int i = 0; i < 13; i++) {
            client.print("{\"sensor\":\"");
            client.print(sensorTitle[i]);
            client.print("\", ");
            client.print("\"value\":\"");
            client.print(sensorValue[i]);
            client.print("\", ");
            client.print("\"metric\":\"");
            client.print(sensorMetric[i]);
            client.println("\"},");
          }
          i=13;
          client.print("{\"sensor\":\"");
          client.print(sensorTitle[i]);
          client.print("\", ");
          client.print("\"value\":\"");
          client.print(sensorValuePosture);
          client.print("\", ");
          client.print("\"metric\":\"");
          client.print(sensorMetric[i]);
          client.println("\"}");
          client.println("]}");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    if (debug) Serial.println("************ client disonnected ************");
  }
}

void readPulsioximeter(){    
  cont ++; 
  if (cont == 50) { //Get only one of the 50 possible measures to reduce the latency
    eHealth.readPulsioximeter();  
    cont = 0;
  }
}
