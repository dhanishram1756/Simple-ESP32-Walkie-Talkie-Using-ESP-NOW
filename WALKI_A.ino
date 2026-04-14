
#include <esp_now.h>
#include <WiFi.h>

// === CONFIGURATION ===
#define DEVICE_ID 'A'  // Change to 'B' for second device

// Pin definitions
#define MIC_PIN 34
#define SPEAKER_PIN 25
#define PTT_PIN 32

// Optimized audio settings for ESP-NOW
#define SAMPLE_RATE 16000     
#define PACKET_SIZE 240       
#define BITS_PER_SAMPLE 8     

// === MAC ADDRESS OF PEER ===
uint8_t peerAddress[] = {0x00,0x00,0x00,0x00,0x00,0x00}; // UPDATE THIS! (only add each 2 hexadecimal digit after 'x')

// === GLOBAL VARIABLES ===
uint8_t txBuffer[PACKET_SIZE];
uint8_t rxBuffer[PACKET_SIZE];
volatile bool hasNewAudio = false;
volatile int rxLength = 0;
volatile bool pttPressed = false;
esp_now_peer_info_t peerInfo;

// DAC output task
TaskHandle_t playbackTaskHandle = NULL;

// === PLAYBACK TASK ===
void playbackTask(void *parameter) {
  while (true) {
    if (hasNewAudio && !pttPressed) {
      // Play received audio
      for (int i = 0; i < rxLength; i++) {
        dacWrite(SPEAKER_PIN, rxBuffer[i]);
        delayMicroseconds(93.5); 
      }
      hasNewAudio = false;
    }
    vTaskDelay(1);
  }
}

// === ESP-NOW RECEIVE CALLBACK ===
void onDataReceive(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  if (!pttPressed && len <= PACKET_SIZE) {
    memcpy(rxBuffer, data, len);
    rxLength = len;
    hasNewAudio = true;
  }
}

// === ESP-NOW SEND CALLBACK ===
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  // Optional: monitor transmission status
  if (status != ESP_NOW_SEND_SUCCESS) {
    // Packet lost - normal for wireless
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║  ESP32 Walkie-Talkie (ESP-NOW)    ║");
  Serial.println("║  Optimized for Voice Clarity      ║");
  Serial.println("╚════════════════════════════════════╝\n");
  
  Serial.print("Device ID: ");
  Serial.println((char)DEVICE_ID);
  
  // === PIN SETUP ===
  pinMode(PTT_PIN, INPUT_PULLUP);
  pinMode(MIC_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  
  // Set initial DAC to mid-level
  dacWrite(SPEAKER_PIN, 128);
  
  // === WIFI SETUP ===
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("\n⚠️  IMPORTANT: Copy this MAC and update peerAddress[] in the other device!\n");
  
  // === ESP-NOW SETUP ===
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while(1) delay(1000);
  }
  Serial.println("✓ ESP-NOW initialized");
  
  // Register callbacks
  esp_now_register_recv_cb(onDataReceive);
  esp_now_register_send_cb(onDataSent);
  
  // Add peer
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer - CHECK MAC ADDRESS!");
    Serial.println("   Update peerAddress[] with other device's MAC\n");
  } else {
    Serial.println("✓ Peer device registered\n");
  }
  
  // === CREATE PLAYBACK TASK ===
  xTaskCreatePinnedToCore(
    playbackTask,
    "AudioPlayback",
    4096,
    NULL,
    2,  // High priority
    &playbackTaskHandle,
    0   // Core 0
  );
  
  Serial.println("═══════════════════════════════════");
  Serial.println("          READY TO USE  ");
  Serial.println("═══════════════════════════════════");
  Serial.println("Press PTT button to TALK");
  Serial.println("Release PTT button to LISTEN\n");
}

void loop() {
  bool currentPTT = (digitalRead(PTT_PIN) == LOW);
  
  // ═══ TRANSMIT MODE ═══
  if (currentPTT) {
    if (!pttPressed) {
      Serial.println("🔴 TRANSMITTING...");
      pttPressed = true;
      dacWrite(SPEAKER_PIN, 128); 
    }
    
    // Capture audio samples
    unsigned long sampleTime = micros();
    
    for (int i = 0; i < PACKET_SIZE; i++) {
      // Read microphone
      int rawADC = analogRead(MIC_PIN);
      
      // Map to 8-bit with centering
      int centered = rawADC - 2048;  
      txBuffer[i] = map(centered, -2048, 2047, 0, 255);
      
      // Precise timing for 8kHz sampling
      while (micros() - sampleTime < (i + 1) * 125); // 125us = 8kHz
    }
    
    // Send packet immediately
    esp_err_t result = esp_now_send(peerAddress, txBuffer, PACKET_SIZE);
    
    // Very short delay to prevent overwhelming ESP-NOW
    delayMicroseconds(500);
    
  }
  // ═══ RECEIVE MODE ═══
  else {
    if (pttPressed) {
      Serial.println("🟢 LISTENING...");
      pttPressed = false;
    }
    
    // Let playback task handle audio output
    delay(1);
  }
}
