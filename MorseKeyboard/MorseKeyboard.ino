/**
 * Клавиатура на азбуке Морзе v0.1
 * 
 * Подходит для Arduino Leonardo, Digispark и прочих ардуино-совместимых контроллеров,
 * которые умеют изображать USB-клавиатуру
 * 
 * Настройка перед использованием:
 * Посмотреть на свою клавиатуру, если у вас точка и запятая на клавишах 6 и 7,
 * то нужно раскомментировать макрос MAC_LAYOUT
 * 
 * Затем нужно указать свой способ переключения раскладок - LAYOUT_SWITCH
 * Подобрать под себя тайминги, и можно вводить текст одной кнопкой!
 * Перед подключением ключа убедитесь что установлена русская раскладка клавиатуры,
 * иначе ключ будет вводить не те буквы.
 * Если вводит не те, переключите раскладку в системе без использования ключа.
 * 
 * Использование:
 * Нажимайте на кнопку формируя точки и тире.
 * Длинное нажатие - существенно длиннее обычного тире - переключает раскладку системы и ключа
 * Знак раздела (-...-) заменяет Enter. Чтобы удобнее сидеть в чате
 */

// Библиотека для эмуляции USB-клавиатуры
#include "Keyboard.h"

// На каком пине висит ключ (кнопка)
#define KEY_PIN 7

// Задержка отпускания кнопки в мс - чтобы избежать дребезга
#define DEBOUNCE_INTERVAL 2

// Если пользователь не вводит новых точек и тире в течение этого времени в мс,
// то декодировать и отправить символ
#define SEND_INTERVAL 500

// Длина точкив мс, всё что длиннее будет считаться тире
#define DOT_LENGTH 200

// Если была тишина в течение интервала, то в начале ставим пробел перед следующей буквой
#define SPACE_INTERVAL 2000

// Способ переключения раскладок
// 1 Ctrl + Shift
// 2 Alt + Shift
// 3 Cmd + Space
#define LAYOUT_SWITCH 3

// Раскладка клавиатуры как у мака
// Закомментировать если обычная
//#define MAC_LAYOUT

// Переключение раскладок длинным нажатием
#define LAYOUT_SWITCH_LENGTH 2000

// Первая буква после точки - большая
#define CAPITAL_AFTER_DOT 1

char *morseCode[] = {
  ".-",    // А
  "-...",  // Б
  ".--",   // В
  "--.",   // Г
  "-..",   // Д
  ".",     // Е
  "...-",  // Ж
  "--..",  // З
  "..",    // И
  ".---",  // Й
  "-.-",   // К
  ".-..",  // Л
  "--",    // М
  "-.",    // Н
  "---",   // О
  ".--.",  // П
  ".-.",   // Р
  "...",   // С
  "-",     // Т
  "..-",   // У
  "..-.",  // Ф
  "....",  // Х
  "-.-.",  // Ц
  "---.",  // Ч
  "----",  // Ш
  "--.-",  // Щ
  "--.--", // Ъ
  "-.--",  // Ы
  "-..-",  // Ь
  "..-..", // Э
  "..--",  // Ю
  ".-.-",  // Я
  ".----", // 1
  "..---", // 2
  "...--", // 3
  "....-", // 4
  ".....", // 5
  "-....", // 6
  "--...", // 7
  "---..", // 8
  "----.", // 9
  "-----", // 0
};

// Все буквы русского алфавита кроме ё в английской раскладке
char morse_to_char_rus[] = "f,dult;pbqrkvyjghcnea[wxio]sm'.z1234567890";

// Все буквы английского алфавита в порядке русской раскладки
char morse_to_char_eng[] = "abwgdevzijklmnoprstufhc\0\0q\0yx\0\0\01234567890";

void setup() {
  // put your setup code here, to run once:
  pinMode(KEY_PIN, INPUT_PULLUP);
  Keyboard.begin();
  Serial.begin(9600);
}

void loop() {
  // Введённый код Морзе
  static char code[10] = "";
  
  // Время начала нажатия на кнопку
  static unsigned long key_press_begin = 0;

  // Время с момента отпускания кнопки
  static unsigned long key_release_begin = 0;
  
  // Признак того, что кнопка считается нажатой
  static char is_button_pressed = 0;

  // Признак того, что пока отправить нажатие
  static char is_symbol_sent = 0;

  // Признак того что следующая буква - большая
  static char next_is_capital = 1;
  
  // Признак того что нужен пробел
  static char space_required = 0;

  // Признак того, что пробел не нужен
  static char no_space_required = 1;

  // Текущая раскладка
  static char english_layout = 0;
  
  int button_state = digitalRead(KEY_PIN);
  // Если кнопка нажата
  if(button_state == LOW) {
    if(is_button_pressed == 0) {
      key_press_begin = millis();
      is_button_pressed = 1;
      if(is_symbol_sent) {
        is_symbol_sent = 0;
      }
    }
    key_release_begin = millis();
  }
  else if(button_state == HIGH) {
    if(is_button_pressed && (millis() - key_release_begin) > DEBOUNCE_INTERVAL) {
      if((key_release_begin - key_press_begin) > LAYOUT_SWITCH_LENGTH) {
        layoutSwitch();
        is_symbol_sent = 1;
        if(english_layout) {
          english_layout = 0;
        }
        else {
          english_layout = 1;
        }
      }
      else if((key_release_begin - key_press_begin) > DOT_LENGTH) {
        if(strlen(code) < 10) {
          strcat(code, "-");
        }
      }
      else {
        if(strlen(code) < 10) {
          strcat(code, ".");
        }
      }
      is_button_pressed = 0;
    }
    if(!is_button_pressed && !is_symbol_sent && (millis() - key_release_begin) > SEND_INTERVAL) {
      Serial.println(code);

      if(space_required && !no_space_required) {
        Keyboard.press(' ');
        Keyboard.releaseAll();
      }
      char code_found = 0;
      for(int i = 0; i < strlen(morse_to_char_rus); i++) {
        if(!strcmp(morseCode[i], code)) {
          if(next_is_capital && CAPITAL_AFTER_DOT) {
            Keyboard.press(KEY_LEFT_SHIFT);
            next_is_capital = 0;
          }
          if(english_layout) {
            if(morse_to_char_eng[i]) {
              Keyboard.press(morse_to_char_eng[i]);
            }
          }
          else {
            if(morse_to_char_rus[i]) {
              Keyboard.press(morse_to_char_rus[i]);
            }
          }
          if(morse_to_char_rus[i] == '.') {
            next_is_capital = 1;
          }
          Keyboard.releaseAll();
          code_found = 1;
          no_space_required = 0;
          break;
        }
      }
      // Некоторые коды нужно обрабатывать вручную
      if(!code_found) {
        if(!strcmp(code, "........")) {
          Keyboard.press(KEY_BACKSPACE);
          Keyboard.releaseAll();
          code_found = 1;
          no_space_required = 0;
        }
        if(!strcmp(code, "-...-")) {
          Keyboard.press(KEY_RETURN);
          Keyboard.releaseAll();
          code_found = 1;
          no_space_required = 1;
        }
        // Английский совпадает для мака и ПК
        if(english_layout) {
          if(!strcmp(code, ".-.-.-")) { // .
            Keyboard.press('.');
          }
          if(!strcmp(code, "--..--")) { // ,
            Keyboard.press(',');
          }
          if(!strcmp(code, "---...")) { // :
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press(';');
          }
          if(!strcmp(code, "-.-.-.")) { // ;
            Keyboard.press(';');
          }
          if(!strcmp(code, ".----.")) { // '
            Keyboard.press('\'');
          }
          if(!strcmp(code, ".-..-.")) { // "
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('"');
          }
          if(!strcmp(code, "-....-")) { // -
            Keyboard.press('-');
          }
          if(!strcmp(code, "-..-.")) { // /
            Keyboard.press('/');
          }
          if(!strcmp(code, "..--.-")) { // _
            Keyboard.press('_');
          }
          if(!strcmp(code, "..--..")) { // ?
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('/');
          }
          if(!strcmp(code, "-.-.--")) { // !
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('1');
          }
          if(!strcmp(code, ".-.-.")) { // +
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('=');
          }
          if(!strcmp(code, ".--.-.")) { // @
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('2');
          }
          Keyboard.releaseAll();
        }
#ifdef MAC_LAYOUT
        else {
          if(!strcmp(code, "......")) { // .
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('7');
          }
          if(!strcmp(code, ".-.-.-")) { // ,
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('6');
          }
          if(!strcmp(code, "---...")) { // :
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('5');
          }
          if(!strcmp(code, "-.-.-.")) { // ;
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('8');
          }
          if(!strcmp(code, ".-..-.")) { // "
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('2');
          }
          if(!strcmp(code, "-....-")) { // -
            Keyboard.press('-');
          }
          if(!strcmp(code, "-..-.")) { // /
            Keyboard.press('/');
          }
          if(!strcmp(code, "..--.-")) { // _
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('-');
          }
          if(!strcmp(code, "..--..")) { // ?
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('/');
          }
          if(!strcmp(code, "--..--")) { // !
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('1');
          }
          if(!strcmp(code, ".-.-.")) { // +
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('=');
          }
          Keyboard.releaseAll();
        }
#else
        else {
          if(!strcmp(code, "......")) {
            Keyboard.press('/');
          }
          if(!strcmp(code, ".-.-.-")) {
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('?');
          }
          if(!strcmp(code, "---...")) {
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('6');
          }
          if(!strcmp(code, "-.-.-.")) {
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('4');
          }
          if(!strcmp(code, ".-..-.")) {
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('2');
          }
          if(!strcmp(code, "-....-")) {
            Keyboard.press('-');
          }
          if(!strcmp(code, "-..-.")) {
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('\\');
          }
          if(!strcmp(code, "..--.-")) {
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('-');
          }
          if(!strcmp(code, "..--..")) {
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('7');
          }
          if(!strcmp(code, "--..--")) {
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('1');
          }
          if(!strcmp(code, ".-.-.")) {
            Keyboard.press(KEY_LEFT_SHIFT);
            Keyboard.press('=');
          }
        Keyboard.releaseAll();
      }
#endif
      }
      is_symbol_sent = 1;
      strcpy(code, "");
      space_required = 0;
    }
    if((millis() - key_release_begin) > SPACE_INTERVAL) {
      space_required = 1;
    }
  }
}

void layoutSwitch() {
  switch(LAYOUT_SWITCH) {
    case 1:
      Keyboard.press(KEY_LEFT_CTRL);
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.releaseAll();
      break;
    case 2:
      Keyboard.press(KEY_LEFT_ALT);
      Keyboard.press(KEY_LEFT_SHIFT);
      Keyboard.releaseAll();
      break;
    case 3:
      Keyboard.press(KEY_LEFT_GUI);
      Keyboard.press(' ');
      Keyboard.releaseAll();
      break;
  }
}
