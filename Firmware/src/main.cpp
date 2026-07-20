#include <Arduino.h>

int counter = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);   // give USB serial time to enumerate
    Serial.println("Kandi board 3 boot");
}

void loop() {
    Serial.print("alive ");
    Serial.println(counter++);
    delay(1000);
}