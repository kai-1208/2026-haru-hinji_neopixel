#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 30 // neopixelの数
#define DATA_PIN 6 // arduinoのd6ピン
#define BRIGHTNESS 200 // LEDの最大輝度(0-255)

CRGB leds[NUM_LEDS];

enum LedState {
    OFF,            // 0.LEDオフ : 消灯
    COMM_LOST,      // 1.通信遮断 : 赤点滅
    NORMAL,         // 2.通常(通信OK) : 白グラデーション
    // AUTO,           // 3.自動 : 黄色
    // SEMI_AUTO,      // 4.半自動(センサ・システム動作中) : 黄色点滅
    // HIGH_SPEED,     // 5.高速モード : 赤色
    // LOW_SPEED,      // 6.低速モード : 青色
    // STANDBY,        // 7.待機モード : 白色ランダム
};

LedState currentState = NORMAL;

unsigned long previousMillis = 0;
bool blinkState = false;

/**
 * @brief 消灯
*/
void handleOff() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
}

/**
 * @brief 赤点滅(500ms間隔)
 */
void handleCommLost() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 500) {
        previousMillis = currentMillis;
        blinkState = !blinkState;
    }
    
    CRGB color = blinkState ? CRGB::Red : CRGB::Black;
    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();
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
    fill_solid(leds, NUM_LEDS, CRGB(brightness*0.45, 0, brightness*1.27));
    FastLED.show();

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
 * @brief 黄色点灯
 */
void handleAuto() {
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
    fill_solid(leds, NUM_LEDS, CRGB(brightness, brightness, 0));
    FastLED.show();


    // fill_solid(leds, NUM_LEDS, CRGB::Yellow);
    // FastLED.show();
}

/**
 * @brief 黄色点滅(500ms間隔)
 */
void handleSemiAuto() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 500) {
        previousMillis = currentMillis;
        blinkState = !blinkState;
    }

    CRGB color = blinkState ? CRGB::Yellow : CRGB::Black;
    fill_solid(leds, NUM_LEDS, color);
    FastLED.show();
}

/**
 * @brief 赤色の光が片道に流れる（スキャナー）
 */
void handleHighSpeed() {
    // --- 見た目を調整する定数 ---
    const uint8_t FADE_RATE = 5;   // 残像の消える速さ (0-255, 大きいほど速く消える)
    const uint8_t BPM = 140;         // 光が流れる速さ (Beats Per Minute)

    // 1. 全てのLEDを少しずつ暗くしていく（フェードアウト効果で残像を作る）
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i].nscale8(255 - FADE_RATE);
    }
    
    // 2. 時間に応じて0から255まで単調に増加する値を取得する
    uint8_t beat = beat8(BPM);

    // 3. その値(0-255)を、LEDの位置(0 ～ NUM_LEDS-1)の範囲に変換(マッピング)する
    int currentPos = map(beat, 0, 255, 0, NUM_LEDS - 1);


    // 4. 計算した位置のLEDを赤色で上書き点灯する
    leds[currentPos] = CRGB::Red;

    FastLED.show();
}

/**
 * @brief N個の青い光が等間隔で流れる（時間制御による超低速フェード）
 */
void handleLowSpeed() {
    const int NUM_POINTS = 1; // 流れる数を設定

    // --- 見た目を調整する定数 ---
    const uint8_t BPM = 30;        // 光が流れる速さ

    // 消灯処理を行う「間隔」（ミリ秒）。この数値を大きくするほど、消灯は遅くなる
    const int FADE_INTERVAL_MS = 12; 
    
    // 一回の消灯処理でどれだけ暗くするか。数値を大きくすると、カクカクと消えるようになる
    const uint8_t FADE_AMOUNT = 5;

    // 1.「FADE_INTERVAL_MS」で指定した時間ごとに、中の処理を1回だけ実行する
    EVERY_N_MILLISECONDS(FADE_INTERVAL_MS) {
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i].nscale8(255 - FADE_AMOUNT);
        }
    }
    
    // 2. 先頭になる光の基準位置を計算する (ここは変更なし)
    uint8_t beat = beat8(BPM);
    int leader_pos = map(beat, 0, 255, 0, NUM_LEDS);

    // 3. 光と光の間隔を計算する
    int spacing = NUM_LEDS / NUM_POINTS;

    // 4. ループを使って、N個の光をそれぞれ計算して点灯させる
    for (int i = 0; i < NUM_POINTS; i++) {
        int current_pos = (leader_pos + i * spacing) % NUM_LEDS;
        leds[current_pos] = CRGB::Blue;
    }

    FastLED.show();
}

/**
 * @brief 7. 待機モード：白色のランダムなきらめき
 */
// void handleStandby() {
//     // --- 見た目を調整する定数 ---
//     const int FADE_SPEED = 254; // 全体を暗くする速さ (255に近いほどゆっくり)
//     const int SPARKLE_INTERVAL_MS = 150; // 新しい光が発生する間隔 (ミリ秒)

//     // 1. 全てのLEDをゆっくりと暗くしていく（フェードアウト効果）
//     for (int i = 0; i < NUM_LEDS; i++) {
//         leds[i].nscale8(FADE_SPEED);
//     }

//     // 2. 一定時間ごとに新しい光を追加する
//     unsigned long currentMillis = millis();
//     if (currentMillis - previousMillis >= SPARKLE_INTERVAL_MS) {
//         previousMillis = currentMillis;

//         // ランダムな位置に、ランダムな明るさの白を追加する
//         int pos = random(NUM_LEDS);
//         leds[pos] = CRGB(255, 255, 255);
//     }
    
//     FastLED.show();
// }

void checkSerialInput() {
    if (Serial.available() > 0) {
        char command = Serial.read();
        LedState previousState = currentState;

        switch (command) {
            case '0': currentState = OFF;         break;
            case '1': currentState = COMM_LOST;   break;
            case '2': currentState = NORMAL;      break;
            // case '3': currentState = AUTO;        break;
            // case '4': currentState = SEMI_AUTO;   break;
            // case '5': currentState = HIGH_SPEED;  break;
            // case '6': currentState = LOW_SPEED;   break;
            // case '7': currentState = STANDBY;     break;
            default: return;
        }

        // 状態が変わったらメッセージを表示
        if (previousState != currentState) {
            Serial.print("State changed to: ");
            Serial.println(command);
            previousMillis = millis();
            blinkState = true;
        }
    }
}

void setup() {
    Serial.begin(9600);
    delay(2000); // 起動待機

    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);

    Serial.println("Hello, world");
    currentState = NORMAL;
}

void loop() {
    checkSerialInput();

    switch (currentState) {
        case OFF:           handleOff();          break;
        case COMM_LOST:     handleCommLost();     break;
        case NORMAL:        handleNormal();       break;
        // case AUTO:          handleAuto();         break;
        // case SEMI_AUTO:     handleSemiAuto();     break;
        // case HIGH_SPEED:    handleHighSpeed();    break;
        // case LOW_SPEED:     handleLowSpeed();     break;
        // case STANDBY:       handleStandby();      break;
    }
}