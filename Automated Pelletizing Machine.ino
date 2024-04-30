#include "EasyNextionLibrary.h"
#include "Arduino.h"
#include "control.h"
#include "PCF8575.h"
#include <Preferences.h>
#include <ezButton.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float humidity;
float temperature;

Preferences Settings;

char *secondsToHHMMSS(int total_seconds)
{
    int hours, minutes, seconds;

    hours = total_seconds / 3600;         // Divide by number of seconds in an hour
    total_seconds = total_seconds % 3600; // Get the remaining seconds
    minutes = total_seconds / 60;         // Divide by number of seconds in a minute
    seconds = total_seconds % 60;         // Get the remaining seconds

    // Format the output string
    static char hhmmss_str[7]; // 6 characters for HHMMSS + 1 for null terminator
    sprintf(hhmmss_str, "%02d%02d%02d", hours, minutes, seconds);
    return hhmmss_str;
}

EasyNex myNex(Serial2);
PCF8575 pcf8575(0x25);

// Declaration of LCD Variables
const int NUM_SETTING_ITEMS = 10;
const int NUM_TEST_SCREEN = 4;

int currentSettingScreen = 0;
int currentTestMenuScreen = 0;

bool settingFlag = false;
bool refreshScreenFlag = false;
bool RunAutoFlag, RunTestFlag = false;

String setting_items[NUM_SETTING_ITEMS][2] = { // array with item names
    {"Grinder", "SEC"},
    {"Bin Open", "SEC"},
    {"Bin Close", "SEC"},
    {"Scale Open", "SEC"},
    {"Scale Close", "SEC"},
    {"Mixer", "MIN"},
    {"Mixer Open", "SEC"},
    {"Mixer Close", "SEC"},
    {"Pelletizer", "MIN"},
    {"Drier", "MIN"}};

double parametersTimer[NUM_SETTING_ITEMS] = {1, 3, 1, 1, 8, 1, 1, 9, 1, 0};
double parametersTimerMaxValue[NUM_SETTING_ITEMS] = {1200, 1200, 1200, 1200, 1200, 1200, 1200, 1200, 1200, 1200};

const int controlPin[16] = {P0, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11, P12, P13, P14, P15};
bool triggerType = LOW;

int rGrinder = P8;
int rBinOpen = P15;
int rBinClose = P7;
int rMolasses = P2;
int rCocoOil = P10;
int rSprayer = P4;
int rScaleOpen = P14;
int rScaleClose = P6;
int rMixer = P1;
int rMixerOpen = P5;
int rMixerClose = P13;
int rPelletizer = P0;
int rSpinner = P9;
int rHeater = P3;
int rFan = P11;
int rFinishLinear = P12;

int rStart = 13;
int rStop = 14;
int rDischarge = 18;

int feedbackSilo1 = 25;
int feedbackSilo2 = 26;
int feedbackSilo3 = 27;
int feedbackSilo4 = 32;
int feedbackDischarge = 33;

bool cornBinStatus, scaleDischargeStatus, mixerDischargeStatus, finishLinearStatus = false;

Control BinOpen(secondsToHHMMSS(3));
Control BinClose(secondsToHHMMSS(6));
Control ScaleOpen(secondsToHHMMSS(4));
Control ScaleClose(secondsToHHMMSS(8));
Control Mixer(secondsToHHMMSS(5));
Control MixerLinearOpen(secondsToHHMMSS(4));
Control MixerLinearClose(secondsToHHMMSS(8));
Control Pelletizer(secondsToHHMMSS(5));
Control Drier(secondsToHHMMSS(5));

Control batchingStart(secondsToHHMMSS(2));
Control batchingStop(secondsToHHMMSS(2));
Control dischargeWaitTimer(secondsToHHMMSS(4));
Control batchingDischarge(secondsToHHMMSS(2));

ezButton FeedbackSilo1(feedbackSilo1);
ezButton FeedbackSilo2(feedbackSilo2);
ezButton FeedbackSilo3(feedbackSilo3);
ezButton FeedbackSilo4(feedbackSilo4);
ezButton FeedbackDischarge(feedbackDischarge);

bool feedbackSilo1Status, feedbackSilo2Status, feedbackSilo3Status, feedbackSilo4Status, feedbackDischargeStatus = false;

bool binOpenStatus = false;
bool scaleOpenStatus = false;

int testBatchingStatus = 0;
int runAutoBatchingStatus = 0;

void saveSettings()
{
    Settings.putDouble("grinder", parametersTimer[0]);
    Settings.putDouble("binOpen", parametersTimer[1]);
    Settings.putDouble("binClose", parametersTimer[2]);
    Settings.putDouble("scaleOpen", parametersTimer[3]);
    Settings.putDouble("scaleClose", parametersTimer[4]);
    Settings.putDouble("mixer", parametersTimer[5]);
    Settings.putDouble("mixerOpen", parametersTimer[6]);
    Settings.putDouble("mixerClose", parametersTimer[7]);
    Settings.putDouble("pelletizer", parametersTimer[8]);
    Settings.putDouble("drier", parametersTimer[9]);
}
void loadSettings()
{
    Serial.println("---- Start Reading Settings ----");
    parametersTimer[0] = Settings.getDouble("grinder", 0);
    parametersTimer[1] = Settings.getDouble("binOpen", 0);
    parametersTimer[2] = Settings.getDouble("binClose", 0);
    parametersTimer[3] = Settings.getDouble("scaleOpen", 0);
    parametersTimer[4] = Settings.getDouble("scaleClose", 0);
    parametersTimer[5] = Settings.getDouble("mixer", 0);
    parametersTimer[6] = Settings.getDouble("mixerOpen", 0);
    parametersTimer[7] = Settings.getDouble("mixerClose", 0);
    parametersTimer[8] = Settings.getDouble("pelletizer", 0);
    parametersTimer[9] = Settings.getDouble("drier", 0);

    Serial.println("Grinder : " + String(parametersTimer[0]));
    Serial.println("BinOpen : " + String(parametersTimer[1]));
    Serial.println("BinClose : " + String(parametersTimer[2]));
    Serial.println("ScaleOpen : " + String(parametersTimer[3]));
    Serial.println("ScaleClose : " + String(parametersTimer[4]));
    Serial.println("Mixer : " + String(parametersTimer[5]));
    Serial.println("MixerOpen : " + String(parametersTimer[6]));
    Serial.println("MixerClose : " + String(parametersTimer[7]));
    Serial.println("Pelletizer : " + String(parametersTimer[8]));
    Serial.println("Drier : " + String(parametersTimer[9]));
    Serial.println("---- End Reading Settings ----");

    BinOpen.setTimer(secondsToHHMMSS(parametersTimer[1]));
    BinClose.setTimer(secondsToHHMMSS(parametersTimer[2]));
    ScaleOpen.setTimer(secondsToHHMMSS(parametersTimer[3]));
    ScaleClose.setTimer(secondsToHHMMSS(parametersTimer[4]));
    Mixer.setTimer(secondsToHHMMSS(parametersTimer[5]*60));
    MixerLinearOpen.setTimer(secondsToHHMMSS(parametersTimer[6]));
    MixerLinearClose.setTimer(secondsToHHMMSS(parametersTimer[7]));
    Pelletizer.setTimer(secondsToHHMMSS(parametersTimer[8]*60));
    Drier.setTimer(secondsToHHMMSS(parametersTimer[9]*60));
}

bool homeAllLinearFlag = false;

void homeAllLinear()
{
    if (BinClose.isStopped() == false)
    {
        BinClose.run();
        if (BinClose.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rBinClose, HIGH);
        }
        else
        {
            pcf8575.digitalWrite(rBinClose, LOW);
        }
    }

    if (ScaleClose.isStopped() == false)
    {
        ScaleClose.run();
        if (ScaleClose.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rScaleClose, HIGH);
        }
        else
        {
            pcf8575.digitalWrite(rScaleClose, LOW);
        }
    }

    if (MixerLinearClose.isStopped() == false)
    {
        MixerLinearClose.run();
        if (MixerLinearClose.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rMixerClose, HIGH);
        }
        else
        {
            pcf8575.digitalWrite(rMixerClose, LOW);
        }
    }

    if (MixerLinearClose.isStopped() == true && ScaleClose.isStopped() == true && BinClose.isStopped() == true)
    {
        homeAllLinearFlag = false;
        myNex.writeStr("page home");
    }
}

void setFeedbackPins()
{
    FeedbackSilo1.setDebounceTime(50);
    FeedbackSilo2.setDebounceTime(50);
    FeedbackSilo3.setDebounceTime(50);
    FeedbackSilo4.setDebounceTime(50);
    FeedbackDischarge.setDebounceTime(50);
}
void readFeedbackStatus()
{
    FeedbackSilo1.loop();
    FeedbackSilo2.loop();
    FeedbackSilo3.loop();
    FeedbackSilo4.loop();
    FeedbackDischarge.loop();

    if (FeedbackSilo1.isPressed())
    {
        feedbackSilo1Status = true;
    }

    if (FeedbackSilo1.isReleased())
    {
        feedbackSilo1Status = false;
    }

    if (FeedbackSilo2.isPressed())
    {
        feedbackSilo2Status = true;
    }

    if (FeedbackSilo2.isReleased())
    {
        feedbackSilo2Status = false;
    }

    if (FeedbackSilo3.isPressed())
    {
        feedbackSilo3Status = true;
    }

    if (FeedbackSilo3.isReleased())
    {
        feedbackSilo3Status = false;
    }

    if (FeedbackSilo4.isPressed())
    {
        feedbackSilo4Status = true;
    }

    if (FeedbackSilo4.isReleased())
    {
        feedbackSilo4Status = false;
    }

    if (FeedbackDischarge.isPressed())
    {
        feedbackDischargeStatus = true;
    }

    if (FeedbackDischarge.isReleased())
    {
        feedbackDischargeStatus = false;
    }
}

void setPinsForRelay()
{
    for (int i = 0; i < 16; i++)
    {
        pcf8575.pinMode(controlPin[i], OUTPUT); // set pin as output
    }

    Serial.begin(115200);

    pcf8575.begin();

    for (int i = 0; i < 16; i++)
    {
        if (triggerType == LOW)
        {
            pcf8575.digitalWrite(controlPin[i], HIGH);
        }
        else
        {
            pcf8575.digitalWrite(controlPin[i], LOW);
        }
    }
}

void StopAllTimer()
{
    BinOpen.stop();
    BinClose.stop();
    ScaleOpen.stop();
    ScaleClose.stop();
    Mixer.stop();
    MixerLinearOpen.stop();
    MixerLinearClose.stop();
    Pelletizer.stop();
    Drier.stop();

    batchingStart.stop();
    batchingStop.stop();
    dischargeWaitTimer.stop();
    batchingDischarge.stop();
}

void StopAllMotor()
{
    pcf8575.digitalWrite(rGrinder, HIGH);
    pcf8575.digitalWrite(rBinOpen, HIGH);
    pcf8575.digitalWrite(rBinClose, HIGH);
    pcf8575.digitalWrite(rMolasses, HIGH);
    pcf8575.digitalWrite(rCocoOil, HIGH);
    pcf8575.digitalWrite(rSprayer, HIGH);
    pcf8575.digitalWrite(rScaleOpen, HIGH);
    pcf8575.digitalWrite(rScaleClose, HIGH);
    pcf8575.digitalWrite(rMixer, HIGH);
    pcf8575.digitalWrite(rMixerOpen, HIGH);
    pcf8575.digitalWrite(rMixerClose, HIGH);
    pcf8575.digitalWrite(rPelletizer, HIGH);
    pcf8575.digitalWrite(rSpinner, HIGH);
    pcf8575.digitalWrite(rHeater, HIGH);
    pcf8575.digitalWrite(rFan, HIGH);
    pcf8575.digitalWrite(rFinishLinear, HIGH);

    digitalWrite(rStart, HIGH);
    digitalWrite(rStop, HIGH);
    digitalWrite(rDischarge, HIGH);
}

Control StartBatching();
Control StopBatching();
Control DischhargeBatching();

unsigned long currentMillis;
unsigned long previousMillis = 0;
const long interval = 1000;

int runAutoStatus = 0;

bool testBatchingStatusFlag = false;
bool testDispensingStatusFlag = false;

void runTest()
{
    // Serial.println("Test Function - Top");
    if (testBatchingStatusFlag == true)
    {
        Serial.println("Test Batching Flag");
        switch (testBatchingStatus)
        {
        case 1:
            if (feedbackSilo1Status == true)
            {
                pcf8575.digitalWrite(rGrinder, LOW);
                Serial.println("Grinder Run");
            }
            else
            {
                pcf8575.digitalWrite(rGrinder, HIGH);
            }
            break;
        case 2:
            // Check for Bin Feedback and Open
            if (binOpenStatus == false && feedbackSilo2Status == true && BinOpen.isStopped() == true)
            {
                Serial.println("Corn bin Open");
                BinOpen.start();
                binOpenStatus = true;
            }

            // Check for Bin Feedback and Close
            if (binOpenStatus == true && feedbackSilo2Status == false && BinClose.isStopped() == true)
            {
                Serial.println("Corn bin Close");
                BinClose.start();
            }
            break;
        case 3:

            if (feedbackSilo3Status == true)
            {
                pcf8575.digitalWrite(rMolasses, LOW);
                Serial.println("Molasses is Running");
            }
            else
            {
                pcf8575.digitalWrite(rMolasses, HIGH);
            }
            break;
        case 4:

            if (feedbackSilo4Status == true)
            {
                pcf8575.digitalWrite(rCocoOil, LOW);
                Serial.println("Coco Oil is Running");
            }
            else
            {
                pcf8575.digitalWrite(rCocoOil, HIGH);
            }
            break;
        default:
            break;
        }

        switch (testBatchingStatus)
        {
        case 0:
            if (feedbackSilo1Status == true)
            {
                testBatchingStatus = 1;
            }
            break;
        case 1:
            if (feedbackSilo2Status == true)
            {
                testBatchingStatus = 2;
            }
            break;
        case 2:
            if (feedbackSilo3Status == true)
            {
                testBatchingStatus = 3;
            }
            break;
        case 3:
            if (feedbackSilo4Status == true)
            {
                testBatchingStatus = 4;
            }
            break;
        case 4:
            if (feedbackSilo4Status == false)
            {
                enableButtonBatching();
                testBatchingStatus = 0;
                binOpenStatus = false;
                testBatchingStatusFlag = false;
            }
            break;
        default:
            break;
        }

        if (binOpenStatus == true && feedbackSilo2Status == true && BinOpen.isStopped() == false)
        {
            BinOpen.run();
            if (BinOpen.isTimerCompleted() == true)
            {
                pcf8575.digitalWrite(rBinOpen, HIGH);
            }
            else
            {
                Serial.println("Corn bin Openning");
                pcf8575.digitalWrite(rBinOpen, LOW);
            }
        }

        if (binOpenStatus == true && BinClose.isStopped() == false)
        {
            BinClose.run();
            if (BinClose.isTimerCompleted() == true)
            {
                pcf8575.digitalWrite(rBinClose, HIGH);
                binOpenStatus = false;
            }
            else
            {
                Serial.println("Corn bin Closing");
                pcf8575.digitalWrite(rBinClose, LOW);
            }
        }
    }
    else if (testDispensingStatusFlag == true)
    {
        if (scaleOpenStatus == false && feedbackDischargeStatus == true && ScaleOpen.isStopped() == false)
        {
            ScaleOpen.run();
            if (ScaleOpen.isTimerCompleted() == true)
            {
                pcf8575.digitalWrite(rScaleOpen, HIGH);
                scaleOpenStatus = true;
                ScaleClose.start();
            }
            else
            {
                pcf8575.digitalWrite(rScaleOpen, LOW);
            }
        }

        if (scaleOpenStatus == true && ScaleClose.isStopped() == false)
        {
            ScaleClose.run();
            if (ScaleClose.isTimerCompleted() == true)
            {
                pcf8575.digitalWrite(rScaleClose, HIGH);
                scaleOpenStatus = false;
                testDispensingStatusFlag = false;
                enableButtonBatching();
            }
            else
            {
                pcf8575.digitalWrite(rScaleClose, LOW);
            }
        }

        if (feedbackDischargeStatus == true)
        {
            pcf8575.digitalWrite(rSprayer, LOW);
            // Serial.println("Sprayer is Running");
        }
        else
        {
            pcf8575.digitalWrite(rSprayer, HIGH);
        }
    }

    // Serial.println("Test Main Flag");
    if (BinOpen.isStopped() == false && testBatchingStatusFlag == false)
    {
        BinOpen.run();
        if (BinOpen.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rBinOpen, HIGH);
            cornBinStatus = true;
            myNex.writeStr("tsw btnItem2,1");
        }
        else
        {
            pcf8575.digitalWrite(rBinOpen, LOW);
        }
    }

    if (BinClose.isStopped() == false && testBatchingStatusFlag == false)
    {
        BinClose.run();
        if (BinClose.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rBinClose, HIGH);
            cornBinStatus = false;
            myNex.writeStr("tsw btnItem2,1");
        }
        else
        {
            pcf8575.digitalWrite(rBinClose, LOW);
        }
    }

    if (ScaleOpen.isStopped() == false && testBatchingStatusFlag == false)
    {
        ScaleOpen.run();
        if (ScaleOpen.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rScaleOpen, HIGH);
            scaleDischargeStatus = true;
            myNex.writeStr("tsw btnItem2,1");
        }
        else
        {
            pcf8575.digitalWrite(rScaleOpen, LOW);
        }
    }

    if (ScaleClose.isStopped() == false && testBatchingStatusFlag == false)
    {
        ScaleClose.run();
        if (ScaleClose.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rScaleClose, HIGH);
            scaleDischargeStatus = false;
            myNex.writeStr("tsw btnItem2,1");
        }
        else
        {
            pcf8575.digitalWrite(rScaleClose, LOW);
        }
    }

    if (MixerLinearOpen.isStopped() == false)
    {
        MixerLinearOpen.run();
        if (MixerLinearOpen.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rMixerOpen, HIGH);
            mixerDischargeStatus = true;
            myNex.writeStr("tsw btnItem4,1");
        }
        else
        {
            pcf8575.digitalWrite(rMixerOpen, LOW);
        }
    }

    if (MixerLinearClose.isStopped() == false)
    {
        MixerLinearClose.run();
        if (MixerLinearClose.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rMixerClose, HIGH);
            mixerDischargeStatus = false;
            myNex.writeStr("tsw btnItem4,1");
        }
        else
        {
            pcf8575.digitalWrite(rMixerClose, LOW);
        }
    }

    if (batchingStart.isStopped() == false)
    {

        batchingStart.run();
        if (batchingStart.isTimerCompleted() == true)
        {
            digitalWrite(rStart, HIGH);
            testBatchingStatusFlag = true;
        }
        else
        {
            digitalWrite(rStart, LOW);
        }
    }

    if (batchingStop.isStopped() == false)
    {
        batchingStop.run();
        if (batchingStop.isTimerCompleted() == true)
        {
            digitalWrite(rStop, HIGH);
            testBatchingStatusFlag = false;
            testDispensingStatusFlag = false;
        }
        else
        {
            digitalWrite(rStop, LOW);
        }
    }

    if (batchingDischarge.isStopped() == false)
    {
        batchingDischarge.run();
        if (batchingDischarge.isTimerCompleted() == true)
        {
            digitalWrite(rDischarge, HIGH);
            testDispensingStatusFlag = true;
            ScaleOpen.start();
        }
        else
        {
            digitalWrite(rDischarge, LOW);
        }
    }
}

void disableButtonBatching()
{
    myNex.writeStr("tsw btnPrev,0");
    myNex.writeStr("tsw btnNext,0");
    myNex.writeStr("tsw btnExit,0");
    myNex.writeStr("tsw btnDischarge,0");
    myNex.writeStr("tsw btnStart,0");
}
void enableButtonBatching()
{
    myNex.writeStr("tsw btnPrev,1");
    myNex.writeStr("tsw btnNext,1");
    myNex.writeStr("tsw btnExit,1");
    myNex.writeStr("tsw btnDischarge,1");
    myNex.writeStr("tsw btnStart,1");
}

bool RunAutoCornBinOpen, RunAutoScaleOpen, RunAutoMixerOpen = false;
int RunAutoBatchingStatus = 0;
String valueToPrint = "";
void runAuto()
{
    /*
    1 - Click Batch Start
    2 - Run Bins and Read Feedback
    3 - Dischage and Read Feedback On Mixer
    3 - Mixer Timer
    4 - Open Mixer and Run Pelletizer
    5 - Pelletizer Timer and Run Drier Motor / Heater / Read Heat and Humidity
    6 - Drier Timer
    7 - Discharge Until Stop
    */
    switch (runAutoStatus)
    {
    case 1:
        batchingStart.run();
        if (batchingStart.isTimerCompleted() == true)
        {
            digitalWrite(rStart, HIGH);
            runAutoStatus = 2;
            pcf8575.digitalWrite(rHeater, LOW);
            pcf8575.digitalWrite(rFan, LOW);
        }
        else
        {
            digitalWrite(rStart, LOW);
        }
        break;
    case 2:
        switch (RunAutoBatchingStatus)
        {
        case 1:
            if (feedbackSilo1Status == true)
            {
                pcf8575.digitalWrite(rGrinder, LOW);
            }
            else
            {
                pcf8575.digitalWrite(rGrinder, HIGH);
            }
            break;
        case 2:
            // Check for Bin Feedback and Open
            if (RunAutoCornBinOpen == false && feedbackSilo2Status == true && BinOpen.isStopped() == true)
            {
                BinOpen.start();
                RunAutoCornBinOpen = true;
            }

            // Check for Bin Feedback and Close
            if (RunAutoCornBinOpen == true && feedbackSilo2Status == false && BinClose.isStopped() == true)
            {
                BinClose.start();
            }
            break;
            
        case 3:
            if (feedbackSilo3Status == true)
            {
                pcf8575.digitalWrite(rMolasses, LOW);
            }
            else
            {
                pcf8575.digitalWrite(rMolasses, HIGH);
            }
            break;
        case 4:

            if (feedbackSilo4Status == true)
            {
                pcf8575.digitalWrite(rCocoOil, LOW);
            }
            else
            {
                pcf8575.digitalWrite(rCocoOil, HIGH);
            }
            break;
        default:
            break;
        }

        switch (RunAutoBatchingStatus)
        {
        case 0:
            if (feedbackSilo1Status == true)
            {
                RunAutoBatchingStatus = 1;
            }
            break;
        case 1:
            if (feedbackSilo2Status == true)
            {
                RunAutoBatchingStatus = 2;
            }
            break;
        case 2:
            if (feedbackSilo3Status == true)
            {
                RunAutoBatchingStatus = 3;
            }
            break;
        case 3:
            if (feedbackSilo4Status == true)
            {
                RunAutoBatchingStatus = 4;
            }
            break;
        case 4:
            if (feedbackSilo4Status == false)
            {
                enableButtonBatching();
                RunAutoBatchingStatus = 0;
                RunAutoCornBinOpen = false;
                runAutoStatus = 3;
                dischargeWaitTimer.start();
            }
            break;
        default:
            break;
        }

        if (feedbackSilo2Status == true && BinOpen.isStopped() == false)
        {
            BinOpen.run();
            if (BinOpen.isTimerCompleted() == true)
            {
                pcf8575.digitalWrite(rBinOpen, HIGH);
            }
            else
            {
                pcf8575.digitalWrite(rBinOpen, LOW);
            }
        }

        if (RunAutoCornBinOpen == true &&feedbackSilo2Status == false && BinClose.isStopped() == false)
        {
            BinClose.run();
            if (BinClose.isTimerCompleted() == true)
            {
                pcf8575.digitalWrite(rBinClose, HIGH);
                RunAutoCornBinOpen = false;
            }
            else
            {
                Serial.println("Corn bin Closing");
                pcf8575.digitalWrite(rBinClose, LOW);
            }
        }
        break;

    case 3:
        if (dischargeWaitTimer.isStopped() == false)
        {
            dischargeWaitTimer.run();
            if (dischargeWaitTimer.isTimerCompleted() == true)
            {
                batchingDischarge.start();
                runAutoStatus = 4;
            }
        }
        break;
    case 4:
        if (batchingDischarge.isStopped() == false)
        {
            Serial.println("Running Discharge Button");
            batchingDischarge.run();
            if (batchingDischarge.isTimerCompleted() == true)
            {
                Serial.println("Running Discharge Relay Off");
                digitalWrite(rDischarge, HIGH);
                pcf8575.digitalWrite(rMixer, LOW);
                runAutoStatus = 5;
            }
            else
            {
                Serial.println("Running Discharge Relay On");
                digitalWrite(rDischarge, LOW);
            }
        }
        break;
    case 5:
        if (RunAutoScaleOpen == false && feedbackDischargeStatus == true && ScaleOpen.isStopped() == false)
        {
            ScaleOpen.run();
            if (ScaleOpen.isTimerCompleted() == true)
            {
                pcf8575.digitalWrite(rScaleOpen, HIGH);
                RunAutoScaleOpen = true;
            }
            else
            {
                pcf8575.digitalWrite(rScaleOpen, LOW);
            }
        }

        if (RunAutoScaleOpen == true && ScaleClose.isStopped() == false)
        {
            ScaleClose.run();
            if (ScaleClose.isTimerCompleted() == true)
            {
                pcf8575.digitalWrite(rScaleClose, HIGH);
                RunAutoScaleOpen = false;
                runAutoStatus = 6;
                Mixer.start();
            }
            else
            {
                pcf8575.digitalWrite(rScaleClose, LOW);
            }
        }

        if (feedbackDischargeStatus == true && ScaleOpen.isStopped() == true)
        {
            ScaleOpen.start();
        }
        else if (RunAutoScaleOpen == true && feedbackDischargeStatus == false)
        {
            ScaleClose.start();
        }

        if (feedbackDischargeStatus == true)
        {
            pcf8575.digitalWrite(rSprayer, LOW);
        }
        else
        {
            pcf8575.digitalWrite(rSprayer, HIGH);
        }
        break;
    case 6:
        Mixer.run();
        if (Mixer.isTimerCompleted() == true)
        {
            MixerLinearOpen.start();
            pcf8575.digitalWrite(rPelletizer, LOW);
            pcf8575.digitalWrite(rSpinner, LOW);
            runAutoStatus = 7;
        }
        else
        {
            pcf8575.digitalWrite(rMixer, LOW);
        }
        break;
    case 7:
        if (MixerLinearOpen.isStopped() == false)
        {
            MixerLinearOpen.run();
            if (MixerLinearOpen.isTimerCompleted() == true)
            {
                pcf8575.digitalWrite(rMixerOpen, HIGH);
                RunAutoMixerOpen = true;
                Pelletizer.start();
                runAutoStatus = 8;
            }
            else
            {
                pcf8575.digitalWrite(rMixerOpen, LOW);
            }
        }
        break;
    case 8:
        Pelletizer.run();
        if (Pelletizer.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rPelletizer, HIGH);
            pcf8575.digitalWrite(rSpinner, HIGH);
            pcf8575.digitalWrite(rMixer, HIGH);
            runAutoStatus = 9;
            MixerLinearClose.start();
            Drier.start();
        }
        else
        {
            pcf8575.digitalWrite(rPelletizer, LOW);
        }
        break;
    case 9:
        if (MixerLinearClose.isStopped() == false)
        {
            MixerLinearClose.run();
            if (MixerLinearClose.isTimerCompleted() == true)
            {
                pcf8575.digitalWrite(rMixerClose, HIGH);
                RunAutoMixerOpen = false;
            }
            else
            {
                pcf8575.digitalWrite(rMixerClose, LOW);
            }
        }

        Drier.run();
        if (Drier.isTimerCompleted() == true)
        {
            // pcf8575.digitalWrite(rSpinner, HIGH);
            pcf8575.digitalWrite(rHeater, HIGH);
            pcf8575.digitalWrite(rFan, HIGH);
            runAutoStatus = 10;
        }
        else
        {
            pcf8575.digitalWrite(rHeater, LOW);
            pcf8575.digitalWrite(rFan, LOW);
        }

        break;
    case 10:
        pcf8575.digitalWrite(rSpinner, LOW);
        pcf8575.digitalWrite(rFinishLinear, LOW);
        break;
    default:
        Serial.println("Default Running");
        break;
    }
}

void RunAutoBatching()
{
    switch (runAutoBatchingStatus)
    {
    case 1:
        if (feedbackSilo1Status == true)
        {
            pcf8575.digitalWrite(rGrinder, LOW);
        }
        else
        {
            pcf8575.digitalWrite(rGrinder, HIGH);
        }
        break;
    case 2:
        // Check for Bin Feedback and Open
        if (binOpenStatus = false && feedbackSilo2 == true && BinOpen.isStopped() == true)
        {
            BinOpen.start();
            binOpenStatus = true;
        }
        // Check for Bin Feedback and Close
        if (binOpenStatus == true && feedbackSilo2 == false && BinClose.isStopped() == true)
        {
            BinClose.start();
        }
        break;
    case 3:
        if (feedbackSilo3Status == true)
        {
            pcf8575.digitalWrite(rMolasses, LOW);
        }
        else
        {
            pcf8575.digitalWrite(rMolasses, HIGH);
        }
        break;
    case 4:
        if (feedbackSilo4Status == true)
        {
            pcf8575.digitalWrite(rCocoOil, LOW);
        }
        else
        {
            pcf8575.digitalWrite(rCocoOil, HIGH);
        }
        break;
    default:
        break;
    }

    switch (runAutoBatchingStatus)
    {
    case 0:
        if (feedbackSilo1Status == true)
        {
            runAutoBatchingStatus = 1;
        }
        break;
    case 1:
        if (feedbackSilo2Status == true)
        {
            runAutoBatchingStatus = 2;
        }
        break;
    case 2:
        if (feedbackSilo3Status == true)
        {
            runAutoBatchingStatus = 3;
        }
        break;
    case 3:
        if (feedbackSilo4Status == true)
        {
            runAutoBatchingStatus = 4;
        }
        break;
    case 4:
        if (feedbackSilo4Status == false)
        {
            enableButtonBatching();
        }
        break;
    default:
        break;
    }

    if (binOpenStatus == true && feedbackSilo2 == true && BinOpen.isStopped() == false)
    {
        BinOpen.run();
        if (BinOpen.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rBinOpen, HIGH);
        }
        else
        {
            pcf8575.digitalWrite(rBinOpen, LOW);
        }
    }

    if (binOpenStatus == true && BinClose.isStopped() == false)
    {
        BinClose.run();
        if (BinClose.isTimerCompleted() == true)
        {
            pcf8575.digitalWrite(rBinClose, HIGH);
            binOpenStatus = false;
        }
        else
        {
            pcf8575.digitalWrite(rBinClose, LOW);
        }
    }
}

void setPinout()
{
    pinMode(rStart, OUTPUT);
    digitalWrite(rStart, HIGH);
    pinMode(rStop, OUTPUT);
    digitalWrite(rStop, HIGH);
    pinMode(rDischarge, OUTPUT);
    digitalWrite(rDischarge, HIGH);
}

void setup()
{
    myNex.begin(9600);
    dht.begin();

    setPinsForRelay();

    StopAllTimer();

    Settings.begin("timerSetting", false);

    setFeedbackPins();
    setPinout();
    // saveSettings();
    loadSettings();

    homeAllLinearFlag = true;
    BinClose.start();
    ScaleClose.start();
    MixerLinearClose.start();
    myNex.writeStr("page pHomingAll");
}

void loop()
{

    if (homeAllLinearFlag == true)
    {
        homeAllLinear();
    }
    else
    {
        myNex.NextionListen();
        readFeedbackStatus();
        if (refreshScreenFlag == true)
        {
            refreshScreen();
        }

        if (RunAutoFlag == true)
        {
            humidity = dht.readHumidity();
            temperature = dht.readTemperature();
            runAuto();
            unsigned long currentMillis = millis();
            if (currentMillis - previousMillis >= interval)
            {
                previousMillis = currentMillis;
                refreshScreenFlag = true;
            }
        }

        if (RunTestFlag == true)
        {
            // Serial.println("Main Loop - Test");
            runTest();

            unsigned long currentMillis = millis();
            if (currentMillis - previousMillis >= interval)
            {
                previousMillis = currentMillis;
                refreshScreenFlag = true;
            }
        }
    }
}

// Select Settings - 00
void trigger0()
{
    myNex.writeStr("page pSettings");
    settingFlag = true;
    refreshScreenFlag = true;
}

// Save Settings - 01
void trigger1()
{
    myNex.writeStr("page home");
    settingFlag = false;
    refreshScreenFlag = true;
    saveSettings();
    loadSettings();
}

// Prev Setting - 02
void trigger2()
{
    if (currentSettingScreen == 0)
    {
        currentSettingScreen = NUM_SETTING_ITEMS - 1;
    }
    else
    {
        currentSettingScreen--;
    }
    refreshScreenFlag = true;
}

// Next Settings - 03
void trigger3()
{
    if (currentSettingScreen == NUM_SETTING_ITEMS - 1)
    {
        currentSettingScreen = 0;
    }
    else
    {
        currentSettingScreen++;
    }
    refreshScreenFlag = true;
}

// + Small Settings - 04
void trigger4()
{
    if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
    {
        parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
    }
    else
    {
        parametersTimer[currentSettingScreen] += 1;
        if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
        {
            parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
        }
    }
    refreshScreenFlag = true;
}

// - Small Settings - 05
void trigger5()
{
    if (parametersTimer[currentSettingScreen] <= 0)
    {
        parametersTimer[currentSettingScreen] = 0;
    }
    else
    {
        parametersTimer[currentSettingScreen] -= 1;

        if (parametersTimer[currentSettingScreen] <= 0)
        {
            parametersTimer[currentSettingScreen] = 0;
        }
    }
    refreshScreenFlag = true;
}

// + 10 Settings - 06
void trigger6()
{
    if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
    {
        parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
    }
    else
    {
        parametersTimer[currentSettingScreen] += 10;

        if (parametersTimer[currentSettingScreen] >= parametersTimerMaxValue[currentSettingScreen] - 1)
        {
            parametersTimer[currentSettingScreen] = parametersTimerMaxValue[currentSettingScreen];
        }
    }
    refreshScreenFlag = true;
}

// -10 Settings - 07
void trigger7()
{
    if (parametersTimer[currentSettingScreen] <= 0)
    {
        parametersTimer[currentSettingScreen] = 0;
    }
    else
    {
        parametersTimer[currentSettingScreen] -= 10;

        if (parametersTimer[currentSettingScreen] <= 0)
        {
            parametersTimer[currentSettingScreen] = 0;
        }
    }
    refreshScreenFlag = true;
}

// Test Machine Item 1 - 08
void trigger8()
{
    if (currentTestMenuScreen == 0)
    {
        if (pcf8575.digitalRead(rGrinder) == HIGH)
        {
            pcf8575.digitalWrite(rGrinder, LOW);
            myNex.writeNum("btnItem1.val", 1);
        }
        else
        {
            pcf8575.digitalWrite(rGrinder, HIGH);
            myNex.writeNum("btnItem1.val", 0);
        }
    }
    else if (currentTestMenuScreen == 1)
    {
        if (pcf8575.digitalRead(rSprayer) == HIGH)
        {
            pcf8575.digitalWrite(rSprayer, LOW);
            myNex.writeNum("btnItem1.val", 1);
        }
        else
        {
            pcf8575.digitalWrite(rSprayer, HIGH);
            myNex.writeNum("btnItem1.val", 0);
        }
    }
    else if (currentTestMenuScreen == 2)
    {
        if (pcf8575.digitalRead(rPelletizer) == HIGH)
        {
            pcf8575.digitalWrite(rPelletizer, LOW);
            myNex.writeNum("btnItem1.val", 1);
        }
        else
        {
            pcf8575.digitalWrite(rPelletizer, HIGH);
            myNex.writeNum("btnItem1.val", 0);
        }
    }
    else
    {
    }
}

// Test Machine Item 2 - 09
void trigger9()
{
    if (currentTestMenuScreen == 0)
    {
        if (cornBinStatus == false)
        {
            BinOpen.start();
            myNex.writeStr("tsw btnItem2,0");
            myNex.writeNum("btnItem2.val", 1);
        }
        else
        {
            BinClose.start();
            myNex.writeStr("tsw btnItem2,0");
            myNex.writeNum("btnItem2.val", 0);
        }
    }
    else if (currentTestMenuScreen == 1)
    {
        if (scaleDischargeStatus == false)
        {
            ScaleOpen.start();
            myNex.writeStr("tsw btnItem2,0");
            myNex.writeNum("btnItem2.val", 1);
        }
        else
        {
            ScaleClose.start();
            myNex.writeStr("tsw btnItem2,0");
            myNex.writeNum("btnItem2.val", 0);
        }
    }
    else if (currentTestMenuScreen == 2)
    {
        if (pcf8575.digitalRead(rSpinner) == HIGH)
        {
            pcf8575.digitalWrite(rSpinner, LOW);
            myNex.writeNum("btnItem2.val", 1);
        }
        else
        {
            pcf8575.digitalWrite(rSpinner, HIGH);
            myNex.writeNum("btnItem2.val", 0);
        }
    }
    else
    {
    }
}

// Test Machine Item 3 - 0A
void trigger10()
{
    if (currentTestMenuScreen == 0)
    {
        if (pcf8575.digitalRead(rMolasses) == HIGH)
        {
            pcf8575.digitalWrite(rMolasses, LOW);
            myNex.writeNum("btnItem3.val", 1);
        }
        else
        {
            pcf8575.digitalWrite(rMolasses, HIGH);
            myNex.writeNum("btnItem3.val", 0);
        }
    }
    else if (currentTestMenuScreen == 1)
    {
        if (pcf8575.digitalRead(rMixer) == HIGH)
        {
            pcf8575.digitalWrite(rMixer, LOW);
            myNex.writeNum("btnItem3.val", 1);
        }
        else
        {
            pcf8575.digitalWrite(rMixer, HIGH);
            myNex.writeNum("btnItem3.val", 0);
        }
    }
    else if (currentTestMenuScreen == 2)
    {
        if (pcf8575.digitalRead(rHeater) == HIGH)
        {
            pcf8575.digitalWrite(rHeater, LOW);
            pcf8575.digitalWrite(rFan, LOW);
            myNex.writeNum("btnItem3.val", 1);
        }
        else
        {
            pcf8575.digitalWrite(rHeater, HIGH);
            pcf8575.digitalWrite(rFan, HIGH);
            myNex.writeNum("btnItem3.val", 0);
        }
    }
    else
    {
    }
}

// Test Machine Item 4 - 0B
void trigger11()
{
    if (currentTestMenuScreen == 0)
    {
        if (pcf8575.digitalRead(rCocoOil) == HIGH)
        {
            pcf8575.digitalWrite(rCocoOil, LOW);
            myNex.writeNum("btnItem4.val", 1);
        }
        else
        {
            pcf8575.digitalWrite(rCocoOil, HIGH);
            myNex.writeNum("btnItem4.val", 0);
        }
    }
    else if (currentTestMenuScreen == 1)
    {
        if (mixerDischargeStatus == false)
        {
            MixerLinearOpen.start();
            myNex.writeStr("tsw btnItem4,0");
            myNex.writeNum("btnItem4.val", 1);
        }
        else
        {
            MixerLinearClose.start();
            myNex.writeStr("tsw btnItem4,0");
            myNex.writeNum("btnItem4.val", 0);
        }
    }
    else if (currentTestMenuScreen == 2)
    {
        if (pcf8575.digitalRead(rFinishLinear) == HIGH)
        {
            pcf8575.digitalWrite(rFinishLinear, LOW);
            myNex.writeNum("btnItem4.val", 1);
        }
        else
        {
            pcf8575.digitalWrite(rFinishLinear, HIGH);
            myNex.writeNum("btnItem4.val", 0);
        }
    }
    else
    {
    }
}

// Test Machine Prev - 0C
void trigger12()
{
    if (currentTestMenuScreen == 0)
    {
        currentTestMenuScreen = NUM_TEST_SCREEN - 1;
    }
    else
    {
        currentTestMenuScreen--;
    }
    selectTestScreen();
}

// Test Machine Next - 0D
void trigger13()
{
    if (currentTestMenuScreen == NUM_TEST_SCREEN - 1)
    {
        currentTestMenuScreen = 0;
    }
    else
    {
        currentTestMenuScreen++;
    }
    selectTestScreen();
}

// Test Machine Exit - 0E
void trigger14()
{
    currentTestMenuScreen = 0;
    myNex.writeStr("page home");
    RunTestFlag = false;
}

// RunAuto - 0F
void trigger15()
{
    myNex.writeStr("page pRun");
    RunAutoFlag = true;
    runAutoStatus = 1;
    batchingStart.start();
}

// RunAuto Stop - 10
void trigger16()
{
    myNex.writeStr("page home");
    RunAutoFlag = false;
    StopAllMotor();
    StopAllTimer();
    runAutoStatus = 0;
    RunAutoBatchingStatus = 0;
    RunAutoCornBinOpen = false;
    RunAutoScaleOpen = false;
    RunAutoMixerOpen = false;
}

// Test Machine Open - 11
void trigger17()
{
    currentTestMenuScreen = 0;
    selectTestScreen();
    RunTestFlag = true;
    Serial.println("Open Test Menu");
}

// Start Batching - 12
void trigger18()
{
    Serial.println("Click Start Batching");
    batchingStart.start();
    disableButtonBatching();
}

// Start Batching - 13
void trigger19()
{
    Serial.println("Click Stop Batching");
    batchingStop.start();
    testBatchingStatus = 0;
    testBatchingStatusFlag = false;
    enableButtonBatching();
}

// Start Batching - 14
void trigger20()
{
    Serial.println("Click Dispense Batching");
    batchingDischarge.start();
    disableButtonBatching();
}

void selectTestScreen()
{
    if (currentTestMenuScreen == 0)
    {
        myNex.writeStr("page pTest1");
    }
    else if (currentTestMenuScreen == 1)
    {
        myNex.writeStr("page pTest2");
    }
    else if (currentTestMenuScreen == 2)
    {
        myNex.writeStr("page pTest3");
    }
    else
    {
        myNex.writeStr("page pTest4");
    }
}

void refreshScreen()
{
    if (refreshScreenFlag == true)
    {
        refreshScreenFlag = false;
        if (settingFlag == true)
        {
            myNex.writeStr("item.txt", setting_items[currentSettingScreen][0]);
            String valueToPrint = String(parametersTimer[currentSettingScreen]) + " " + setting_items[currentSettingScreen][1];
            myNex.writeStr("value.txt", valueToPrint);
        }
        else if (RunAutoFlag == true)
        {
            switch (runAutoStatus)
            {
            case 1:
                myNex.writeStr("txtbProcess.txt", "   Start Batching");
                myNex.writeStr("txtbRemainTime.txt", "   " + String(batchingStart.getTimeRemaining()));
                break;
            case 2:
                switch (RunAutoBatchingStatus)
                {
                case 1:
                    myNex.writeStr("txtbProcess.txt", "   Grinding");
                    myNex.writeStr("txtbRemainTime.txt", "   N/A");
                    break;
                case 2:
                    myNex.writeStr("txtbProcess.txt", "   Dispense Corn");
                    myNex.writeStr("txtbRemainTime.txt", "   N/A");
                    break;
                case 3:
                    myNex.writeStr("txtbProcess.txt", "   Dispense Molasses");
                    myNex.writeStr("txtbRemainTime.txt", "   N/A");
                    break;
                case 4:
                    myNex.writeStr("txtbProcess.txt", "   Dispense Coco Oil");
                    myNex.writeStr("txtbRemainTime.txt", "   N/A");
                    break;
                default:
                    break;
                }
                break;
            case 3:
                myNex.writeStr("txtbProcess.txt", "   Pause Timer");
                myNex.writeStr("txtbRemainTime.txt", "   " + String(dischargeWaitTimer.getTimeRemaining()));
                break;
            case 4:
                myNex.writeStr("txtbProcess.txt", "   Start Discharge Scale");
                myNex.writeStr("txtbRemainTime.txt", "   " + String(batchingDischarge.getTimeRemaining()));
                break;
            case 5:
                myNex.writeStr("txtbProcess.txt", "   Discharge Scale");
                myNex.writeStr("txtbRemainTime.txt", "   N/A");
                break;
            case 6:
                myNex.writeStr("txtbProcess.txt", "   Mixing");
                myNex.writeStr("txtbRemainTime.txt", "   " + String(Mixer.getTimeRemaining()));
                break;
            case 7:
                myNex.writeStr("txtbProcess.txt", "   Open Mixer");
                myNex.writeStr("txtbRemainTime.txt", "   " + String(MixerLinearOpen.getTimeRemaining()));
                break;
            case 8:
                myNex.writeStr("txtbProcess.txt", "   Pelletizing");
                myNex.writeStr("txtbRemainTime.txt", "   " + String(Pelletizer.getTimeRemaining()));
                break;
            case 9:
                myNex.writeStr("txtbProcess.txt", "   Drying");
                myNex.writeStr("txtbRemainTime.txt", "  N/A");
                Serial.println(Drier.getTimeRemaining());
                break;
            case 10:
                myNex.writeStr("txtbProcess.txt", "   Discharging");
                myNex.writeStr("txtbRemainTime.txt", "  N/A");
                
                break;
            default:
                break;
            }

            myNex.writeStr("txtbTemp.txt", "  " + String(temperature, 0) + "C");
            myNex.writeStr("txtbHumidity.txt", "  " + String(humidity, 0) + "%");
        }
        else if (RunTestFlag == true)
        {
            if (currentTestMenuScreen == 3)
            {
                if (testBatchingStatusFlag == true)
                {
                    switch (testBatchingStatus)
                    {
                    case 1:
                        Serial.println("Update Grinding");
                        myNex.writeStr("txtbBatching.txt", "Grinding");
                        break;
                    case 2:
                        Serial.println("Update Dispense Corn");
                        myNex.writeStr("txtbBatching.txt", "Dispense Corn");
                        break;
                    case 3:
                        Serial.println("Update Dispense Molasses");
                        myNex.writeStr("txtbBatching.txt", "Dispense Molasses");
                        break;
                    case 4:
                        Serial.println("Update Dispense Coco Oil");
                        myNex.writeStr("txtbBatching.txt", "Dispense Coco Oil");
                        break;
                    default:
                        Serial.println("Set to N/A");
                        myNex.writeStr("txtbBatching.txt", "N/A");
                        break;
                    }
                }
                else if (testDispensingStatusFlag == true)
                {
                    if (feedbackDischargeStatus == true)
                    {
                        myNex.writeStr("txtbBatching.txt", "Dispense and Spray");
                    }
                    else
                    {
                        myNex.writeStr("txtbBatching.txt", "N/A");
                    }
                }
                else
                {
                    myNex.writeStr("txtbBatching.txt", "N/A");
                }
            }
        }
    }
}