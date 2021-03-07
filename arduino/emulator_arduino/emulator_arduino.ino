//Arduino pins
#define GBClock 2
#define GBIn 4 //on the emulated device, out for the physical device
#define GBOut 5 //on the emulated device, in for the physical device

//states of incoming packets from Game Boy
#define TIMEOUT 10
#define IDLING 0
#define PREAMBLE_PARTIAL 1
#define HEADER 2
#define DATASUM 3
#define DATASUM_DONE 4
#define RESPONSE_PARTIAL 5

#define GB_TIMEOUT 150

//clock status
#define CLOCK_LOW 0
#define CLOCK_LOW_READ 1
#define CLOCK_HIGH 2

byte temp_byte;
byte status;
byte clock_status;
byte rx_byte;
byte tx_byte;
byte packet_state;
byte header_remain;
int data_remain;
unsigned long tick;

unsigned long gb_tick;
byte timed_out;

byte GBSerialIO(byte tx_byte) {
  byte rx_byte=0;
  for (byte c=0;  c<8;  c++) {

    //read cycle - begins when clock is low
    gb_tick = millis();
    while(digitalRead(GBClock) == 1){
      if (millis() - gb_tick > GB_TIMEOUT){
        packet_state = TIMEOUT;
        timed_out = 1;
        return 0x00;
      }
    }
    rx_byte <<= 1;
    if(digitalRead(GBIn))
    {
      rx_byte |= 1;                  
    }

    //write cycle - begins when clock is high
    
    if((tx_byte << c) & 0x80){
      digitalWrite(GBOut, 1);
    }
    else{ 
      digitalWrite(GBOut, 0);
    }   

    gb_tick = millis();
    while(digitalRead(GBClock) == 0){
      if (millis() - gb_tick > GB_TIMEOUT){
        timed_out = 1;
        return 0x00;
      }
    }
  }
  return rx_byte;
}

void setup() {
  pinMode(GBIn, INPUT_PULLUP);
  pinMode(GBOut, OUTPUT);
  pinMode(GBClock, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  for(int i=0; i<2; i++){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
  packet_state = IDLING;
  tx_byte = 0x00;
  status = 0x00;
  Serial.begin(115200);
  tick = millis();
  timed_out = 0x00;
}

void loop() {
  if (Serial.available() > 0) {
    temp_byte = Serial.read();
    if (temp_byte == 105) {
      Serial.write("nice");
    } else {
      status = temp_byte;
    }
  }
    
  if (digitalRead(GBClock) == 0 && clock_status==CLOCK_HIGH) {
    clock_status = CLOCK_LOW;
  }
  if (digitalRead(GBClock) == 1) {
    clock_status = CLOCK_HIGH;
  }

  if (clock_status == CLOCK_LOW){
    clock_status = CLOCK_LOW_READ;
    digitalWrite(LED_BUILTIN, LOW);
    rx_byte = GBSerialIO(tx_byte);
    tick = millis();

    switch (packet_state)
    {
      case TIMEOUT:
        tx_byte = 0x00;
        packet_state = IDLING;
        break;
        
      case IDLING:
        tx_byte = 0x00;
        if (rx_byte==0x88) {
          packet_state = PREAMBLE_PARTIAL;
        }
        break;
        
      case PREAMBLE_PARTIAL:
        if (rx_byte==0x33) {
          packet_state = HEADER;
          header_remain = 4;
        } else {
          packet_state = IDLING;
        }
        break;
        
      case HEADER:
        header_remain -= 1;
        if (header_remain == 1){ //LSB of data size
          data_remain = rx_byte + 2; //checksum is 2 bytes
        }
        if (header_remain == 0){ //MSB of data size
          data_remain += rx_byte*256;
          packet_state = DATASUM;
        }
        break;
        
      case DATASUM:
        data_remain -= 1;
        if (data_remain == 0){ 
          packet_state = DATASUM_DONE;
          tx_byte = 0x81; //always first byte of response
        }
        break;

      case DATASUM_DONE:
        packet_state = RESPONSE_PARTIAL;
        tx_byte = status;
        break;

      case RESPONSE_PARTIAL:
        packet_state = IDLING;
        tx_byte = 0x00;
        break;
    }
    
    Serial.write(rx_byte);
    Serial.write(tx_byte);
    Serial.write(packet_state);
    Serial.write(timed_out);

    timed_out = 0x00;
  }

  if (millis() - tick > 1500) {
    packet_state = IDLING;
    tick = millis();
    for(int i=0; i<2; i++){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(25);
      digitalWrite(LED_BUILTIN, LOW);
      delay(25);
    }
  }
  if (packet_state == IDLING){
    digitalWrite(LED_BUILTIN, HIGH);
  }

  //Serial.write(clock_status+69);
  //Serial.write("\n");
  //delay(100);
}
