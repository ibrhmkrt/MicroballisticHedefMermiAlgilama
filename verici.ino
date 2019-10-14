#include <avr/io.h>
#include <avr/interrupt.h>

unsigned  int sayac = 0;//Hedefin vurulduğu Toplam sayı

int ilk_siddet = 0;
int ortam_siddeti = 100;//1,5 saniye ortam gürültü ortalaması, daha sonra 1 önceki analog kanal değeri
bool ortam_dinleme = true;//false değeri için hedef atışa hazırdır
bool yeni_atis = false;//yeni bir dogru atış tespitinde true değerini alır bir sonraki atış bekleme için false yapılır
bool hazir = false;//atış tespitinde atış algılanmasında sonra gerekli olan sürenin geçmesi ve analog kalaın değerlendirmeye hazır olma durumunu tutar(true)
byte gelen_istek;
//senkron bekleme
bool bekleme = true;
unsigned long atis_sonu = 0;
int fark;
int esik = 200;
void setup() {
  Serial.begin(115200);//com port a
  pinMode(13, OUTPUT);//6. pin atış tespitinde kullanılacak buzzer pini
  delay(1000);

  ADCSRA = 0x8F;      // Enable the ADC and its interrupt feature
  //and set the ACD clock pre-scalar to clk/128
  //ADMUX = 0xE0;     // Select internal 2.56V as Vref, left justify

  //ADC MODUL AYARLARI
  ADMUX |= (1 << REFS0); //AREF seçimi,REFS1 "0" kalır ve besleme gerilimi olan 5V referans alınır
  ADMUX &= 0xF0;//ADC kanalı seçimi(A0)*/
  ADCSRA |= (1 << ADSC); //ADC dönüşüm başlatma bayragını aktif etme, dönüşüm bittiğinde "0" olur.

  TCNT1 = 61600; // 1 saniye 15625 için >> 65535-15625, 61600 ~2,5 saniye
  TCCR1A = 0x00;//Normal mode
  TCCR1B &= (0 << CS11); // 1024 prescler
  TCCR1B |= (1 << CS10); // 1024 prescler
  TCCR1B |= (1 << CS12); // 1024 prescler
  TIMSK1 |= (1 << TOIE1); // Timer1 taşma kesmesi aktif

  sei(); //Global kesmeler devrede

}

void loop() {
  if (millis() < 1500) { // açılışta 1,5 saniye süresince ortamın gürültü ortalmasını hesaplanır

    ortam_siddeti = (ilk_siddet + ortam_siddeti) / 2;
  }
  else
  {
    ortam_dinleme = false;

  }

  ADCSRA |= (1 << ADSC); //ADC dönüşüm başlatma bayragını aktif etme, dönüşüm bittiğinde "0" olur.
  if (yeni_atis == true) {
    yeni_atis = false;
    atis_sonu = millis();
    bekleme = true;
  }

  if (Serial.available()) {
    //Serial.println("oke ");
    gelen_istek = Serial.read();
    if (gelen_istek == 251) //reset
    {
      sayac = 0;
    }
    if (gelen_istek == 252) { //set
      esik = esik + 100;
      if (esik > 1023) {
        esik = 100;
      }

    }
  }




}



int analog_degerr = 0;//analog kanaldan gelen değerin saklandığı değişken 0-1024

ISR(ADC_vect) {
  analog_degerr = ADC;

  /*Serial.print("0, 350, 700, ");
    Serial.println(analog_degerr);*/

  if (ortam_dinleme == true) {
    ilk_siddet = analog_degerr;
  }

  else {
    fark = abs(analog_degerr - ortam_siddeti);
    if (fark >= esik)
    {
      //Serial.println("PUAN : ");


      if (hazir == true) {
        //Serial.print("TOPLAM PUAN : ");
        sayac++;
        sayac = sayac % 100;
        /*Serial.print("sesSiddeti:");
          Serial.print(fark);
          Serial.print(",");
          Serial.print("isabetSayisi:");*/
        Serial.println(sayac);
        Serial.println(esik);


        digitalWrite(13, HIGH);

        TCNT1 = 61600;// 65535-61600(1024 prescaler)=~250 ms --- 1 sn 15625
        TIFR1 |= (1 << TOV1) ;//timer1 taşma bayragı sıfırlanır.
        TIMSK1 |= (1 << TOIE1) ;// Timer1 taşma kesmesi aktif

        yeni_atis = true;
        hazir = false;

        //senkron
        bekleme = false;

      }
    }
    ortam_siddeti = analog_degerr;
  }
}

ISR (TIMER1_OVF_vect)    // Timer1 ISR
{
  TIMSK1 &= (0 << TOIE1) ;   // Timer1 taşma kesmesi devredışı (TOIE1)
  digitalWrite(13, LOW);
  hazir = true;  // for 1 sec at 16 MHz
}
