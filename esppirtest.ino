
int pirPin = D5;
int val;
 
void setup()
{
Serial.begin(115200);
}
 
void loop()
{
val = digitalRead(pirPin);
//low = no motion, high = motion
if (val == LOW)
{
  Serial.println("No motion");
}
else
{
  Serial.println("Motion detected  ALARM");
}
 
delay(1000);
}


