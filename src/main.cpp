#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 90 // neopixelの数
#define DATA_PIN 6 // arduinoのd6ピン
#define BRIGHTNESS 200 // LEDの最大輝度(0-255)
#define NUM_STATUS_LEDS 30 // ステータス表示に使用するLEDの数
#define NUM_MAIN_LEDS (NUM_LEDS - NUM_STATUS_LEDS) // メイン表示に使用するLEDの数
#define NUM_BOOLS 12 // 受信するbool値の数 (statusBoolsのサイズ)
#define STATUS_BRIGHTNESS 50 // ステータスLEDの個別の明るさ(0-255)

CRGB leds[NUM_LEDS];

enum LedState {
    OFF,            // 0.LEDオフ : 消灯
    UART_LOST,      // 1.uart遮断 : 黄点滅
    CAN_LOST,       // 2.can遮断 : 赤点滅
    NORMAL,         // 3.通常(通信OK) : 青紫ドクンドクン
    CLEAR,          // 4.クリア : 緑点滅ウェーブ
};

LedState currentState = NORMAL;
bool statusBools[NUM_BOOLS]; // 受信したbool値 false: 緑点灯, true: 赤点滅 リミットスイッチ false(sw押されている): 黄点灯, true: 緑点灯

unsigned long previousMillis = 0;
bool blinkState = false;

// UART通信監視用
unsigned long lastUartReceivedMillis = 0; // 最後に受信した時刻

// ステータスLED用の点滅管理
unsigned long statusBlinkMillis = 0;
bool statusBlinkState = false;

/**
 * @brief 消灯
*/
void handleOff() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
}

/**
 * @brief 黄点滅(500ms間隔)
 */
void handleUartLost() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 500) {
        previousMillis = currentMillis;
        blinkState = !blinkState;
    }
    CRGB color = blinkState ? CRGB::Yellow : CRGB::Black;
    fill_solid(leds, NUM_MAIN_LEDS, color);
}

/**
 * @brief 赤点滅(500ms間隔)
 */
void handleCanLost() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 500) {
        previousMillis = currentMillis;
        blinkState = !blinkState;
    }
    CRGB color = blinkState ? CRGB::Red : CRGB::Black;
    fill_solid(leds, NUM_MAIN_LEDS, color);
}

/**
 * @brief 青紫ドクンドクン
 */
void handleNormal() {
    // --- 鼓動の見た目を調整する定数 ---
    const unsigned long BEAT_CYCLE_MS = 1200; // 1回の鼓動サイクル全体の時間 (ミリ秒)
    // --- 1回目の鼓動 (大きく「どくん」) ---
    const unsigned long FIRST_BEAT_START = 0;
    const unsigned long FIRST_BEAT_END = 400; // ★余韻のために少し時間を長く
    const unsigned long FIRST_BEAT_FADE_IN_MS = 60; // ★急速に明るくなる時間
    const uint8_t FIRST_BEAT_BRIGHTNESS = 200;
    // --- 2回目の鼓動 (小さく「どくん」) ---
    const unsigned long SECOND_BEAT_START = 300;
    const unsigned long SECOND_BEAT_END = 900; // ★余韻のために少し時間を長く
    const unsigned long SECOND_BEAT_FADE_IN_MS = 50; // ★急速に明るくなる時間
    const uint8_t SECOND_BEAT_BRIGHTNESS = 100;
    // 1. 現在の時刻をサイクル時間で割った余りを求める
    unsigned long timeInCycle = millis() % BEAT_CYCLE_MS;
    uint8_t brightness = 0; // 基本は消灯
    // 2. 現在の時刻がどの区間にあるかを判断する
    if (timeInCycle >= FIRST_BEAT_START && timeInCycle < FIRST_BEAT_END) {
        // --- 1回目の鼓動の処理 ---
        unsigned long peakTime = FIRST_BEAT_START + FIRST_BEAT_FADE_IN_MS;
        if (timeInCycle < peakTime) {
            // ★急速に明るくなっていく区間
            brightness = map(timeInCycle, FIRST_BEAT_START, peakTime, 0, FIRST_BEAT_BRIGHTNESS);
        } else {
            // ★ゆっくりと暗くなっていく区間（余韻）
            brightness = map(timeInCycle, peakTime, FIRST_BEAT_END, FIRST_BEAT_BRIGHTNESS, 0);
        }
    } else if (timeInCycle >= SECOND_BEAT_START && timeInCycle < SECOND_BEAT_END) {
        // --- 2回目の鼓動の処理 ---
        unsigned long peakTime = SECOND_BEAT_START + SECOND_BEAT_FADE_IN_MS;
        if (timeInCycle < peakTime) {
            // ★急速に明るくなっていく区間
            brightness = map(timeInCycle, SECOND_BEAT_START, peakTime, 0, SECOND_BEAT_BRIGHTNESS);
        } else {
            // ★ゆっくりと暗くなっていく区間（余韻）
            brightness = map(timeInCycle, peakTime, SECOND_BEAT_END, SECOND_BEAT_BRIGHTNESS, 0);
        }
    }
    // 3. 計算した明るさを全てのLEDに適用
    fill_solid(leds, NUM_MAIN_LEDS, CRGB(brightness*0.45, 0, brightness*1.27));

    // // 白のウェーブ
    // const float WAVE_SPEED = 240.0;     // 波の移動速度（数値を大きくすると遅くなる）
    // const float WAVE_LENGTH = 7.0;    // 波長（数値を大きくすると山の間隔が広がる）
    // const float WAVE_SHARPNESS = 6.0;  // 波の鋭さ（数値を大きくすると山が鋭くなり、谷が広がる）
    // for (int i = 0; i < NUM_LEDS; i++) {
    //     // 1. 時間とLEDの位置から、sin関数の角度を計算
    //     float angle = (millis() / WAVE_SPEED) + (i / WAVE_LENGTH);
    //     // 2. sin関数の結果 (-1.0 ～ 1.0) を、0.0 ～ 1.0 の範囲に変換
    //     float sin_0_to_1 = (sin(angle) + 1.0) / 2.0;
    //     // 3.【重要】値を累乗して、波の山の部分を鋭く、谷の部分を広くする
    //     float wave_factor = pow(sin_0_to_1, WAVE_SHARPNESS);
    //     // 4. 計算した係数を、実際の明るさ (0 ～ 255) に変換
    //     uint8_t brightness = wave_factor * 255;
    //     // 5. i番目のLEDに色を設定
    //     leds[i] = CRGB(brightness, brightness, brightness);
    // }
    // // 全てのLEDの色情報を一度に更新
    // FastLED.show();
    // 以下は、ただsin関数を使った白のグラデーション
    // uint8_t breath = (sin(millis() / 2000.0 * PI) + 1.0) / 2.0 * 255;
    // fill_solid(leds, NUM_LEDS, CRGB(breath, breath, breath));
    // FastLED.show();
}

/**
 * @brief CLEAR: 中心から外側へ白色のウェーブ
 */
void handleClear() {
    // セクション設定
    const int NUM_SECTIONS = 6;
    const int SECTION_SIZE = 15;
    const int CENTERS[] = {7, 22, 37, 52, 67, 82}; // 各セクションの中心LED番号

    // ウェーブのアニメーションパラメータ
    const float WAVE_SPEED = 0.02; // 波の速さ (大きいほど速い)
    const float WAVE_WIDTH = 1.0;   // 波の光っている部分の幅
    
    // 時間に基づいて、中心からの「光のピーク位置」を計算 (0.0 ～ SECTION_SIZE/2.0 の範囲を繰り返す)
    // fmodを使って、中心から端(約7.5)まで動いたらまた中心に戻るのこぎり波を作る
    float currentRadius = fmod(millis() * WAVE_SPEED, (float)(SECTION_SIZE / 2.0 + 2.0)); 

    // 背景を黒でクリア（残像を残したい場合は nscale8 を使用）
    // fill_solid(leds, NUM_MAIN_LEDS, CRGB::Black); 
    // 残像効果を入れて滑らかにする場合:
    for(int i = 0; i < NUM_LEDS; i++) leds[i].nscale8(200);

    for (int s = 0; s < NUM_SECTIONS; s++) {
        int center = CENTERS[s];
        int start = s * SECTION_SIZE;
        int end = start + SECTION_SIZE;

        for (int i = start; i < end; i++) {
            // 中心からの距離
            float dist = abs(i - center);

            // 現在の波の半径(currentRadius)と、このLEDの距離(dist)の差が小さいほど明るくする
            // ガウス関数のような計算で明るさを決定
            float diff = dist - currentRadius;
            float brightness = 200.0 * exp(-(diff * diff) / (2 * (WAVE_WIDTH / 2.0) * (WAVE_WIDTH / 2.0)));
            
            // LEDに色を加算（白色） CRGB::White は (255, 255, 255)
            // 既存の色に足し合わせることで、ウェーブが重なっても綺麗に見える
            if (brightness > 10) { // 閾値
                leds[i] += CRGB((uint8_t)brightness, (uint8_t)brightness, (uint8_t)brightness);
                 // もしくは単に設定する場合
                // leds[i] = CRGB((uint8_t)brightness, (uint8_t)brightness, (uint8_t)brightness);
            }
        }
    }
}

/**
 * @brief ステータスLEDの更新
 */
void handleStatusLeds() {
    uint8_t brightness = STATUS_BRIGHTNESS; // ステータスLEDの明るさ
    // LED 60-64: statusBools[9]
    CRGB color1 = statusBools[9] ? CRGB::White : CRGB::Black;
    if (statusBools[9]) color1.nscale8(brightness);
    for(int i = 60; i < 65; i++) leds[i] = color1;

    // LED 65-69: statusBools[10]
    CRGB color2 = statusBools[10] ? CRGB::White : CRGB::Black;
    if (statusBools[10]) color2.nscale8(brightness);
    for(int i = 65; i < 70; i++) leds[i] = color2;

    // LED 70-74: statusBools[11]
    CRGB color3 = statusBools[11] ? CRGB::White : CRGB::Black;
    if (statusBools[11]) color3.nscale8(brightness);
    for(int i = 70; i < 75; i++) leds[i] = color3;

    // LED 75-79: statusBools[8]
    CRGB color4 = statusBools[8] ? CRGB::White : CRGB::Black;
    if (statusBools[8]) color4.nscale8(brightness);
    for(int i = 75; i < 80; i++) leds[i] = color4;

    // LED 80-84: statusBools[7]
    CRGB color5 = statusBools[7] ? CRGB::White : CRGB::Black;
    if (statusBools[7]) color5.nscale8(brightness);
    for(int i = 80; i < 85; i++) leds[i] = color5;

    // LED 85-89: statusBools[6]
    CRGB color6 = statusBools[6] ? CRGB::White : CRGB::Black;
    if (statusBools[6]) color6.nscale8(brightness);
    for(int i = 85; i < 90; i++) leds[i] = color6;
}

/**
 * @brief 受信データのパース処理
 * 形式: "State,bool1,bool2...bool15\n" 例: "3,0,1,0...\n"
 */
void checkSerialInput() {
    if (Serial.available() > 0) {
        lastUartReceivedMillis = millis(); // 最後に受信した時刻を更新

        String input = Serial.readStringUntil('\n');
        input.trim(); // 改行コード除去

        if (input.length() > 0) {
            // 最初の1文字目をStateとして取得
            char stateChar = input.charAt(0);
            
            // State更新
            switch (stateChar) {
                case '0': currentState = OFF; break;
                case '1': currentState = UART_LOST; break;
                case '2': currentState = CAN_LOST; break;
                case '3': currentState = NORMAL; break;
                case '4': currentState = CLEAR; break;
                default: 
                    break;
            }

            // if (!digitalRead(2)) {
            //     currentState = OFF;
            // }

            // 2文字目以降（カンマ区切り）をパース
            // フォーマット例: "3,0,1,0,0..."
            // 最初のカンマを探す
            // int searchIndex = 1; 
            int firstCommaIndex = input.indexOf(',');
            int searchIndex = (firstCommaIndex != -1) ? firstCommaIndex + 1 : input.length();

            for (int i = 0; i < NUM_BOOLS; i++) {
                int commaIndex = input.indexOf(',', searchIndex);
                if (commaIndex == -1) {
                    // 最後の要素の場合、またはフォーマット不正で途中終了した場合
                    if (searchIndex < input.length()) {
                        String valStr = input.substring(searchIndex); // 末尾まで
                        statusBools[i] = (valStr.toInt() == 1);
                    } else {
                        statusBools[i] = false;
                    }
                    break; 
                }
                String valStr = input.substring(searchIndex, commaIndex);
                statusBools[i] = (valStr.toInt() == 1);
                searchIndex = commaIndex + 1;
            }
            Serial.println("curr_state: " + String(currentState));
            Serial.print("status_bools: ");
            for (int i = 0; i < NUM_BOOLS; i++) {
                Serial.print(statusBools[i]);
                Serial.print(" ");
            }
            Serial.println("\n");
        } 
        // Serial.println("Received input: " + input);
    } else {
        if (millis() - lastUartReceivedMillis >= 2000) { // uart、し... 死んでる
            currentState = UART_LOST;
            // Serial.println("UART timeout\n");
        }
    }
}

void setup() {
    pinMode(2, INPUT_PULLUP);
    Serial.begin(9600);
    Serial.setTimeout(50);
    delay(2000); // 起動待機

    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);

    Serial.println("Hello, world");
    currentState = CLEAR;

    lastUartReceivedMillis = millis(); // 初期化

    // 初期化：すべてのステータスをfalse(緑)にしておく
    for(int i=0; i<NUM_BOOLS; i++) {
        statusBools[i] = false;
    }
}

void loop() {
    checkSerialInput();

    // currentState = CLEAR;

    if (currentState != CLEAR) {
        handleStatusLeds();
    }

    switch (currentState) {
        case OFF:           handleOff();          break;
        case UART_LOST:     handleUartLost();     break;
        case CAN_LOST:      handleCanLost();      break;
        case NORMAL:        handleNormal();       break;
        case CLEAR:         handleClear();        break;
    }

    EVERY_N_MILLISECONDS(30) {
        FastLED.show();
    }
}