#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

// Pinu definīcijas
#define PWM_PINS 2         // PWM izvades pins
#define RELEJA_PINS 4      // Releja kontroles pins

#define POGA_SAMAZINA 5    // Poga 1 - Samazina temperatūru
#define POGA_IESL_IZSL 6   // Poga 2 - Ieslēgt/izslēgt
#define POGA_PALIELINA 7   // Poga 3 - Palielina temperatūru

#define TEMP_SENSORS A1    // Temperatūras sensora pins
#define SPRIEG_PINS A0     // Sprieguma mērīšanas pins

// Displeja konfigurācija
#define EKRANA_PLATUMS 128
#define EKRANA_AUGSTUMS 64
Adafruit_SSD1306 ekrans(EKRANA_PLATUMS, EKRANA_AUGSTUMS, &Wire, -1);

// Globālie mainīgie
int temperatura;           // Pašreizējā temperatūra
int merkTemperatura = 0;   // Mērķa temperatūra
int sasniedzTemperatura = 999;  // Temperatūra, kas jāsasniedz
int pogasStavoklis = 3;    // Pogas stāvoklis/temperatūras izvēle
int pogasLaiks;            // Pogas atatsītes taimeris
float spriegums = 0;       // Ieejas spriegums
bool iesledzSistemu = 0;   // Sistēmas ieslēgšanas/izslēgšanas stāvoklis
long autoIzslLaiks = 0;    // Automātiskās izslēgšanas taimeris

void setup() {
    Serial.begin(9600);
    Serial.print("Seriālais ports inicializēts");
    delay(100);

    // Displeja inicializācija
    ekrans.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    ekrans.setTextSize(1);
    ekrans.setTextColor(WHITE);

    // Pinu konfigurācija
    pinMode(PWM_PINS, OUTPUT);
    pinMode(RELEJA_PINS, OUTPUT);
    digitalWrite(RELEJA_PINS, LOW);

    pinMode(POGA_SAMAZINA, INPUT_PULLUP);
    pinMode(POGA_IESL_IZSL, INPUT_PULLUP);
    pinMode(POGA_PALIELINA, INPUT_PULLUP);

    pinMode(TEMP_SENSORS, INPUT);

    // Displeja sākuma animācija
    for(int16_t i=0; i<ekrans.height()/3; i+=2) {
        ekrans.drawRect(i/3, i, ekrans.width()-2*i/3, ekrans.height()-2*i, SSD1306_WHITE);
        ekrans.display();
        delay(5);
    }

    // Parādīt autora vārdu
    ekrans.setCursor(10, 30);
    ekrans.setTextSize(1);
    ekrans.println("name");
    ekrans.display();
    delay(2000);
    ekrans.clearDisplay();
}

void loop() {
    autoIzslegt();      // Automātiski izslēgt pēc neaktivitātes
    parlSpriegumu();    // Pārbaudīt ieejas spriegumu
    nolasitTemp();      // Iegūt temperatūru
    apstradatPogas();   // Apstrādāt pogu ievades

    // Temperatūras kontrole
    if (iesledzSistemu == 1 && temperatura < 100) {
        sasniedzTemperatura = merkTemperatura;
        silditVaiDzeset();  // Sildīšanas/dzesēšanas kontrole
    } else {
        digitalWrite(PWM_PINS, LOW);  // Izslēgt PWM, ja sistēma ir izslēgta
    }

    atjaunotEkranu();     // Atjaunināt displeju
}

// Automātiski izslēgt pēc neaktivitātes (5 minūtes)
void autoIzslegt() {
    Serial.println(autoIzslLaiks);
    if (millis() - autoIzslLaiks > 300000) {  // 300000 = 5 minūtes
        autoIzslLaiks = millis();
        iesledzSistemu = 0;
        Serial.println("Automātiska izslēgšana");
    }
}

// Pogu apstrāde
void apstradatPogas() {
    // Temperatūras samazināšanas poga
    if (digitalRead(POGA_SAMAZINA) == LOW && pogasLaiks + 200 < millis()) {
        if (pogasStavoklis > 0) {
            pogasStavoklis = pogasStavoklis - 1;
            pogasLaiks = millis();
            autoIzslLaiks = millis();    // Atiestatīt automātiskās izslēgšanas taimeri
        }
    }

    // Temperatūras palielināšanas poga
    if (digitalRead(POGA_PALIELINA) == LOW && pogasLaiks + 200 < millis()) {
        if (pogasStavoklis < 5) {
            pogasStavoklis = pogasStavoklis + 1;
            pogasLaiks = millis();
            autoIzslLaiks = millis();    // Atiestatīt automātiskās izslēgšanas taimeri
        }
    }

    // Iestatīt temperatūras mērķi atkarībā no pogas stāvokļa
    const int tempIzveles[] = {-10, 0, 15, 30, 45, 60};
    if (pogasStavoklis >= 0 && pogasStavoklis <= 5) {
        merkTemperatura = tempIzveles[pogasStavoklis];
    }

    // Ieslēgšanas/izslēgšanas poga
    if (digitalRead(POGA_IESL_IZSL) == LOW && pogasLaiks + 200 < millis()) {
        autoIzslLaiks = millis();    // Atiestatīt automātiskās izslēgšanas taimeri
        pogasLaiks = millis();
        iesledzSistemu = !iesledzSistemu;  // Pārslēgt barošanas stāvokli
    }
}

// Sildīšanas/dzesēšanas kontrole atkarībā no mērķa temperatūras
void silditVaiDzeset() {
    // Iestatīt releja stāvokli atkarībā no sildīšanas vai dzesēšanas režīma
    // Temperatūrai < 30 mēs esam dzesēšanas režīmā (relejs LOW)
    // Temperatūrai >= 30 mēs esam sildīšanas režīmā (relejs HIGH)
    if (sasniedzTemperatura < 30) {
        digitalWrite(RELEJA_PINS, LOW);  // Dzesēšanas režīms
    } else {
        digitalWrite(RELEJA_PINS, HIGH); // Sildīšanas režīms
    }

    // Īpašs gadījums maksimālai dzesēšanai
    if (sasniedzTemperatura == -10) {
        digitalWrite(PWM_PINS, HIGH);    // Vienmēr ieslēgts maksimālai dzesēšanai
        return;
    }

    // Visām pārējām temperatūrām, ieslēgt/izslēgt PWM atkarībā no pašreizējās un mērķa temperatūras
    if ((sasniedzTemperatura < 30 && temperatura > sasniedzTemperatura) ||    // Dzesēšana: pašreizējā > mērķa
        (sasniedzTemperatura >= 30 && temperatura < sasniedzTemperatura)) {   // Sildīšana: pašreizējā < mērķa
            digitalWrite(PWM_PINS, HIGH);  // Ieslēgt
        } else {
            digitalWrite(PWM_PINS, LOW);   // Izslēgt
        }
}

// Rādīt pašreizējo statusu uz OLED displeja
void atjaunotEkranu() {
    ekrans.clearDisplay();
    ekrans.setCursor(1, 5);

    if (temperatura > 100) {
        // Rādīt kļūdu, ja temperatūras sensors nav pievienots
        ekrans.setTextSize(2);
        ekrans.print("Nav pievienots elements!");
    } else {
        // Rādīt pašreizējo temperatūru
        ekrans.setTextSize(1);
        ekrans.println("Paslaik temp.: ");
        // Temperatūras kompensācija zemām vērtībām
        if (temperatura < 10) {
            ekrans.print(temperatura - 2);
        } else {
            ekrans.print(temperatura);
        }
        ekrans.print(" ");
        ekrans.print((char)247);  // Grādu simbols
        ekrans.println("C");
        ekrans.println("");

        // Rādīt mērķa temperatūru
        ekrans.setTextSize(1);
        ekrans.println("Izvele:");

        // Izcelt mērķa temperatūru, ja sistēma ir ieslēgta
        if (iesledzSistemu == 1) {
            ekrans.setTextSize(2);
        } else {
            ekrans.setTextSize(1);
        }

        if (merkTemperatura == -10) {
            ekrans.print("Max -");  // Maksimālā dzesēšana
            ekrans.print((char)247);
            ekrans.print("C");
        } else {
            ekrans.print(merkTemperatura);
        }

        ekrans.setTextSize(1);
        ekrans.print("/");

        // Izcelt OFF, ja sistēma ir izslēgta
        if (iesledzSistemu == 1) {
            ekrans.setTextSize(1);
        } else {
            ekrans.setTextSize(2);
        }
        ekrans.print("OFF");
    }

    ekrans.display();
}

// Nolasīt un aprēķināt temperatūru no sensora
void nolasitTemp() {
    int rawVoltage = analogRead(TEMP_SENSORS);
    float millivolti = (rawVoltage / 1024.0) * 5000;
    float kelvini = (millivolti / 10);
    float celsiji = kelvini - 273.15;
    temperatura = celsiji;
}

// Sprieguma pārbaude un validācija
void parlSpriegumu() {
    // Veikt 30 mērījumu vidējo vērtību stabilai nolasīšanai
    spriegums = 0;
    for (int i = 0; i < 30; i++) {
        spriegums = spriegums + analogRead(SPRIEG_PINS);
        delay(2);
    }

    spriegums = spriegums / 30;
    spriegums = spriegums * 5 * 3 / 1024.0;  // Pārvērst spriegumā
    Serial.print(spriegums);
    Serial.print(" ");

    // Pārbaudīt spriegumu tikai tad, ja sistēma ir izslēgta un nav darbināta ar USB
    if (iesledzSistemu == 0 && spriegums > 1) {
        ekrans.clearDisplay();
        ekrans.setTextSize(1);

        // Pārbaudīt, vai spriegums ir pārāk zems (< 8.5V)
        while (spriegums < 8.5) {
            digitalWrite(PWM_PINS, LOW);

            ekrans.clearDisplay();
            ekrans.setCursor(5, 10);
            ekrans.println("Palielini iejas spriegumu uz 9V!");
            ekrans.println("");
            ekrans.println("Spriegums tagad:");
            ekrans.println(spriegums);
            ekrans.display();

            // Atjaunot sprieguma nolasīšanu
            spriegums = 0;
            for (int i = 0; i < 30; i++) {
                spriegums = spriegums + analogRead(SPRIEG_PINS);
                delay(2);
            }
            spriegums = spriegums / 30;
            spriegums = spriegums * 5 * 3 / 1024.0;
        }

        // Pārbaudīt, vai spriegums ir pārāk augsts (> 9.5V)
        while (spriegums > 9.5) {
            digitalWrite(PWM_PINS, LOW);

            ekrans.clearDisplay();
            ekrans.setCursor(5, 10);
            ekrans.println("Samazini iejas spriegumu uz 9V!");
            ekrans.println("");
            ekrans.println("Spriegums tagad:");
            ekrans.println(spriegums);
            ekrans.display();

            // Atjaunot sprieguma nolasīšanu
            spriegums = 0;
            for (int i = 0; i < 30; i++) {
                spriegums = spriegums + analogRead(SPRIEG_PINS);
                delay(2);
            }
            spriegums = spriegums / 30;
            spriegums = spriegums * 5 * 3 / 1024.0;
            Serial.print(spriegums);
            Serial.print(" ");
        }
    }
}

