// Includes
#include <Arduino.h>
#include <Ethernet.h> //Load Ethernet Library
#include <SPI.h>      //Load SPI Library
#include <Wire.h>     //imports the wire library
#include <SD.h>       //imports the SD library
#include <math.h>

// Declare variables
byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x96, 0x7E}; // Assign mac address
byte ip[] = {192, 168, 200, 69};
EthernetClient tcp_client;       // Create a tcp client object
EthernetServer tcp_server(4455); // Create a tcp server object

// Variables
int spi_clock = 14000000;
int spi_mode = SPI_MODE0;
SPISettings spi_setting(spi_clock, MSBFIRST, spi_mode);
bool DEBUG_FLAG = false;

// Structs
struct Message
{
  byte command;
  int address;
  byte *data;
};

// Constants
const byte CONNECTING = 0x00;
const byte I2C_CONF = 0x01;
const byte I2C_WRITE = 0x02;
const byte I2C_READ = 0x03;
const byte SPI_CONF = 0x04;
const byte SPI_READ_WRITE = 0x05;
const byte GPIO_WRITE = 0x06;
const byte GPIO_READ = 0x07;
const byte ADC_READ = 0x08;
const byte DEBUG = 0x0F;
const byte PACKET_SIZE_DELIMITER = 0x3A;
const byte DEBUG_ADDRESS = 0x01;
const int DEFAULT_I2C_CLOCK = 100000; // Default to 100kHz clock

int i2c_clock = DEFAULT_I2C_CLOCK;

// Methods
int get_packet_size()
{
  // Get the first char of the packet size,
  byte c = tcp_client.read();

  byte packet_size_bytes[4] = {0, 0, 0, 0};
  int current_index = 0;

  // While character not ":"
  // Everything before the : is the packet size
  // while (c != ':')
  while (c != PACKET_SIZE_DELIMITER)
  {
    packet_size_bytes[current_index] = c;
    c = tcp_client.read();
    current_index += 1;
  }
  int packet_size = 0;
  packet_size += packet_size_bytes[0] << 24;
  packet_size += packet_size_bytes[1] << 16;
  packet_size += packet_size_bytes[2] << 8;
  packet_size += packet_size_bytes[3];

  return packet_size;
}

Message decode_packet(int packet_size)
{
  struct Message packet;
  packet.command = tcp_client.read();
  packet.address = tcp_client.read();

  packet.data = (byte *)malloc(sizeof(byte) * packet_size);
  if (packet.data == NULL)
  {
    // Handle memory allocation failure
    // Serial.println("Error: Byte array malloc failed");
    while (1)
      ;
  }
  else
  {
    for (int i = 0; i < packet_size - 2; i++)
    {
      byte c = tcp_client.read();
      // Serial.println(c);
      packet.data[i] = c;
    }
  }

  // Ensure proper cleanup in case of error
  if (packet.data == NULL)
  {
    free(packet.data);
    packet.data = NULL;
  }
  return packet;
}

int convert_bytes_to_int(int packet_size, byte *data)
{
  int i2c_reads = 0;
  int count = 0;
  for (int i = packet_size - 1; i >= 0; i--)
  {
    i2c_reads += data[i] << count * 8;
    count += 1;
  }
  return i2c_reads;
}

byte *convert_int_to_bytes(int number_to_convert)
{
  byte *converted_int = (byte *)malloc(sizeof(byte) * 2);
  converted_int[0] = highByte(number_to_convert);
  converted_int[1] = lowByte(number_to_convert);

  // if (DEBUG_FLAG)
  // {
  //   Serial.println("number_to_convert: " + String(number_to_convert));
  //   Serial.println("converted_int[1]: " + String(converted_int[1]));
  //   Serial.println("converted_int[0]: " + String(converted_int[0]));
  // }

  return converted_int;
}

void setup()
{
  // Disable the SD card
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  Serial.begin(9600);
  Serial1.begin(115200);
  SPI.begin();

  Ethernet.begin(mac, ip); // Inialize the Ethernet

  while (Ethernet.linkStatus() == LinkOFF)
  {
    delay(1000);
  }

  tcp_server.begin();
}

void loop()
{
  tcp_client = tcp_server.available();

  if (tcp_client)
  { // if packetSize is >0, that means someone has sent a request
    while (tcp_client.available())
    {
      int packet_size = get_packet_size();
      int data_packet_size = packet_size - 2;

      struct Message packet = decode_packet(packet_size);

      if (DEBUG_FLAG)
      {
        Serial.println("\tNEW PACKET");
        Serial.print("Packet_size: ");
        Serial.println(packet_size);

        Serial.print("Command: ");
        Serial.println(packet.command);

        Serial.print("Address: ");
        Serial.println(packet.address);
        Serial.println();
      }

      if (packet.command == CONNECTING)
      {
        tcp_server.write("acknowledged");
      }
      else if (packet.command == I2C_CONF)
      {
        switch (packet.address)
        {
        // Case 0 should never happen
        case 0:
          break;
        // Case 1, set clock
        case 1:
          // Data should have 4 bytes, so I am hardcoding it
          int clock = convert_bytes_to_int(4, packet.data);
          i2c_clock = clock;
          break;
        }
      }
      else if (packet.command == I2C_WRITE)
      {
        Wire.begin();
        Wire.setClock(i2c_clock);
        Wire.beginTransmission(packet.address);
        for (int i = 0; i < data_packet_size; i++)
        {
          Wire.write(packet.data[i]);
        }
        Wire.endTransmission();
        Wire.end();
      }
      else if (packet.command == I2C_READ)
      {
        int i2c_read_num = convert_bytes_to_int(4, packet.data);
        // Serial.print("i2c_read_num: ");
        // Serial.println(i2c_read_num);
        Wire.begin();
        Wire.requestFrom(packet.address, i2c_read_num, true);
        byte data_read[i2c_read_num];
        if (Wire.available() >= i2c_read_num)
        {
          for (int i = 0; i < i2c_read_num; i++)
          {
            data_read[i] = Wire.read();
            // //Serial.print("Wire.read: ");
            // //Serial.println(data_read[i]);
          }
          tcp_server.write(data_read, i2c_read_num);
        }
        else
        {
          // Serial.println("No data available");
        }
        Wire.end();
      }
      else if (packet.command == SPI_CONF)
      {
        switch (packet.address)
        {
        case 0:
          spi_mode = SPI_MODE0;
          break;
        case 1:
          spi_mode = SPI_MODE1;
          break;
        case 2:
          spi_mode = SPI_MODE2;
          break;
        case 3:
          spi_mode = SPI_MODE3;
          break;
        }
        spi_clock = convert_bytes_to_int(4, packet.data);

        if (DEBUG_FLAG)
        {
          Serial.print("spi_clock: ");
          Serial.println(spi_clock);
        }
      }
      else if (packet.command == SPI_READ_WRITE)
      {
        // Disable ethernet spi
        digitalWrite(10, HIGH);

        pinMode(packet.address, OUTPUT);
        digitalWrite(packet.address, LOW);

        SPI.beginTransaction(spi_setting);
        SPI.transfer(packet.data, data_packet_size);
        SPI.endTransaction();

        // Re-enable ethernet spi
        digitalWrite(10, LOW);
        digitalWrite(packet.address, HIGH);

        tcp_server.write(packet.data, data_packet_size);
      }
      else if (packet.command == GPIO_WRITE)
      {
        int high_low = int(packet.data[0]);
        pinMode(packet.address, OUTPUT);
        digitalWrite(packet.address, high_low);
      }
      else if (packet.command == GPIO_READ)
      {
        pinMode(packet.address, INPUT);
        int val_read = digitalRead(packet.address);
        if (DEBUG_FLAG)
        {
          Serial.println("Pin: " + String(packet.address) + " is: " + val_read);
        }
        tcp_server.write(byte(val_read));
      }
      else if (packet.command == ADC_READ)
      {
        analogReadResolution(12);
        int val_read = analogRead(packet.address);
        if (DEBUG_FLAG)
        {
          Serial.println("Pin: " + String(packet.address) + " is: " + val_read);
        }
        byte *converted_int = convert_int_to_bytes(val_read);
        tcp_server.write(converted_int, 2);
        free(converted_int);
        converted_int = NULL;
      }
      else if (packet.command == DEBUG)
      {
        int debug_data = convert_bytes_to_int(4, packet.data);
        // Print Debug flag
        if (packet.address == DEBUG_ADDRESS)
        {
          if (debug_data == 1)
          {
            DEBUG_FLAG = true;
            Serial.print("DEBUG_FLAG: ");
            Serial.println(DEBUG_FLAG);
          }
          else if (debug_data == 0)
          {
            DEBUG_FLAG = false;
            Serial.print("DEBUG_FLAG: ");
            Serial.println(DEBUG_FLAG);
          }
        }
      }
      else
      {
        if (DEBUG_FLAG)
        {
          Serial.println("Bad command");
          tcp_server.write("Bad command");
        }
      }
      free(packet.data); // gotta make sure that there are no memory leaks
      packet.data = NULL;
    }
  }
  Ethernet.maintain();
}
