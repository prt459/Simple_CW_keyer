// Basic CW memory keyer, for Arduino Nano, by VK3HN, 7 Apr 2017.  
// 
// Supports:
// - hard coded character weights
// - hard coded or variable speed using a potentiometer
// - two or more hard coded messages, with ability to interrupt playing messages
// - straight key
// - sidetone
// - semi-break-in, PTT line to control transmit/receive switching, with configurable drop-out delay  
// - CW ident (sends hard-coded message (eg. callsign) every n seconds   
//
// Description and wiring diagram at https://vk3hn.wordpress.com/Arduino-CW-keyer-for-a-BiTx-or-other-homebrew-rig
//  
// Arduino connections: --------------------------------------------------------------------------------------         
// Keyed line is D2 
// PTT line (to activate a transmitter) is D3 (works for both keyer and straight key)
// Sidetone on D8 (use R-C filter before injecting into audio amp; can use piezo or 8 ohm speaker with 220 ohm series resistor to GND)
// Pushbuttons for 2 (hard-coded) messages are on D9 and D10 (for additional memories, see notes below)
//   (tapping paddle or key interrupts the message) 
// Paddle left and right are D11 and D12 (center GND) 
// Straight key may be connected to D4
// Wiper of potentiometer across 5v to A3 (keyer speed)
// All other parameters are #define'd at the top and should be self-explanatory.
//
// Adding an extra keyer memory: ------------------------------------------------------------------------------------
// To add an additional keyer memory:
//
// 1. Decide which Arduino digital pin you will use for the additional memory pushbutton (D5 to D7 are avalable) 
//
// 2. Around line #62 add additional #define for the new memory pushbutton (a third memory pushbutton shown here)
//       #define PIN_KEYER_MEM3     5  // digital pin to read pushbutton Keyer Memory 3 
//
// 3. At line #152 Add additional pre-coded keyer message to the msg array 
//       String morse_msg[] = 
//       {
//          "CQ CQ DE VK3HN VK3HN K"
//          , "CQ CQ SOTA DE VK3HN/P VK3HN/P K" 
//          , "Message 3"  
//       };
//
// 4. Around line #422 add  pinMode(PIN_KEYER_MEM3,INPUT_PULLUP);
//
// 5. At line #460 add a test for the additional keyer memory, and increment the morse_msg[] index
//      if(get_button(PIN_KEYER_MEM3)) play_message(morse_msg[2], 0);                                      
//--------------------------------------------------------------------------------------------------------------

// CW Ident (sends callsign every n seconds),   
// to activate this feature, #define CW_IDENT and set CW_IDENT_SECS:

// #define CW_IDENT   // uncomment to activate CW Ident 
#define CW_IDENT_SECS     180  // seconds between CW ident 
#define CW_IDENT_WPM      70  // CW ident speed (dot length mS)


// keyer connections and parameters:
#define PIN_KEY_LINE       2  // digital pin for the key line (mirrors PIN_TONE_OUT)
#define PIN_PTT_LINE       3  // digital pin for the PTT line (to activate a transmitter)
#define PIN_STRAIGHT_KEY   4  // straight key (to GND)

#define PIN_TONE_OUT       8  // digital pin with keyed audio side-tone on it

#define PIN_KEYER_MEM1     9  // digital pin to read pushbutton Keyer Memory 1 
#define PIN_KEYER_MEM2    10  // digital pin to read pushbutton Keyer Memory 2 
// for additional keyer memories, #define other digital pins here (suggest D5, D6, or D7)
// #define PIN_KEYER_MEM3  5 // digital pin to read pushbutton Keyer Memory 3 

#define PIN_PADDLE_R      11  // digital pin for paddle left (dot)
#define PIN_PADDLE_L      12  // digital pin for paddle right (dash)

#define PIN_KEYER_SPEED    3  // analogue input for speed potentiometer (wire from +5v to GND)  
#define CW_TONE_HZ       700  // CW tone frequency (Hz)  

#define CW_DASH_LEN        4  // length of dash (in dots)  (typically 3 to 5, change to suit your preference) 
#define BREAK_IN_DELAY   300  // break-in hang time (mS)
#define SERIAL_LINE_WIDTH 80  // number of chars on Serial console, after which we write a newline 


enum trx_state_e {
  E_STATE_RX, 
  E_STATE_TX
};  // state of the txcvr 

trx_state_e curr_state;

enum key_state_e {
  E_KEY_UP, 
  E_KEY_DOWN
};  // state of the key line 
key_state_e key_state;

int char_sent_ms, curr_ms;
int dot_length_ms = 60;   // keyer base speed (60 equates to 10 w.p.m.)
int last_ident_ms;        // CW ident timer
int ident_secs_count = 0; // seconds counter
                         
bool space_inserted;
bool paddle_squeezed;
unsigned int ch_counter;

// morse reference table
struct morse_char_t {
  char ch[7]; 
};

morse_char_t MorseCode[] = {
  {'A', '.', '-',  0,   0,   0,   0},
  {'B', '-', '.', '.', '.',  0,   0},
  {'C', '-', '.', '-', '.',  0,   0},
  {'D', '-', '.', '.',  0,   0,   0},
  {'E', '.',  0,   0,   0,   0,   0},
  {'F', '.', '.', '-', '.',  0,   0},
  {'G', '-', '-', '.',  0,   0,   0},
  {'H', '.', '.', '.', '.',  0,   0},
  {'I', '.', '.',  0,   0,   0,   0},
  {'J', '.', '-', '-', '-',  0,   0},
  {'K', '-', '.', '-',  0,   0,   0},
  {'L', '.', '-', '.', '.',  0,   0},
  {'M', '-', '-',  0,   0,   0,   0},
  {'N', '-', '.',  0,   0,   0,   0},
  {'O', '-', '-', '-',  0,   0,   0},
  {'P', '.', '-', '-', '.',  0,   0},
  {'Q', '-', '-', '.', '-',  0,   0},
  {'R', '.', '-', '.',  0,   0,   0},
  {'S', '.', '.', '.',  0,   0,   0},
  {'T', '-',  0,   0,   0,   0,   0},
  {'U', '.', '.', '-',  0,   0,   0},
  {'V', '.', '.', '.', '-',  0,   0},
  {'W', '.', '-', '-',  0,   0,   0},
  {'X', '-', '.', '.', '-',  0,   0},
  {'Y', '-', '.', '-', '-',  0,   0},
  {'Z', '-', '-', '.', '.',  0,   0},
  {'0', '-', '-', '-', '-', '-',  0},
  {'1', '.', '-', '-', '-', '-',  0},
  {'2', '.', '.', '-', '-', '-',  0},
  {'3', '.', '.', '.', '-', '-',  0},
  {'4', '.', '.', '.', '.', '-',  0},
  {'5', '.', '.', '.', '.', '.',  0},
  {'6', '-', '.', '.', '.', '.',  0},
  {'7', '-', '-', '.', '.', '.',  0},
  {'8', '-', '-', '-', '.', '.',  0},
  {'9', '-', '-', '-', '-', '.',  0},
  {'/', '-', '.', '.', '-', '.',  0},
  {'?', '.', '.', '-', '-', '.', '.'},
  {'.', '.', '-', '.', '-', '.', '-'},
  {',', '-', '-', '.', '.', '-', '-'}
};

// This array of Strings holds the predefined (compiled) keyer memory messages, edit to suit your needs.
// To add additional memory messages, simply add additional stings in double quotes, preceded by a comma.  
String morse_msg[] = 
{
    "CQ CQ DE VK3HN K" 
  , "CQ SOTA DE VK3HN/P K" 
// , "message 3"  
// , "message 4"  
// , "message  5 etc"  
};

int morse_lookup(char c)
// returns the index of parameter 'c' in MorseCode array, or -1 if not found
{
  for(unsigned int i=0; i<sizeof(MorseCode); i++)
  {
    if(c == MorseCode[i].ch[0])
      return i;
  }
  return -1; 
}

bool get_button(byte btn)
{
// Read the digital pin 'btn', with debouncing; pins are pulled up (ie. default is HIGH)
// returns true (1) if pin is high, false (0) otherwise 
  if (!digitalRead(btn)) 
  {
    delay(5);  // was 20mS, needs to be long enough to ensure debouncing
    if (!digitalRead(btn)) return 1;   // return true
  }
  return 0;  // return false
}

int read_analogue_pin(byte p)
{
// Take an averaged reading of analogue pin 'p'  
  int i, val=0, nbr_reads=2; 
  for (i=0; i<nbr_reads; i++)
  {
    val += analogRead(p);
    delay(5); 
  }
  return val/nbr_reads; 
}

int read_keyer_speed()
{ 
  // Connect up a 1k to 10k linear potentiometer that sweeps between 5V and ground, take the wiper to 
  // analog pin PIN_KEYER_SPEED.  You can adjust the range by reducing the voltage range
  // with resistors either side of the potentiometer; or, scale here in the code.
  int n = read_analogue_pin((byte)PIN_KEYER_SPEED);
  //Serial.print("Speed returned="); Serial.println(n);
  
  dot_length_ms = 60 + (n-183)/5;   // scale to wpm (10 wpm == 60mS dot length)
                                     // '511' should be mid point of returned range
                                     // change '5' to widen/narrow speed range...
                                     // smaller number -> greater range  
  return n;
}

void set_key_state(key_state_e k)
{
// push the paddle/key down, or let it spring back up
// changes global 'key_state' {E_KEY_DOWN, E_KEY_UP}

  switch (k)
  {
      case E_KEY_DOWN:
      {
        // do whatever you need to key the transmitter
        digitalWrite(PIN_KEY_LINE, 1);
        digitalWrite(13, 1);  // turn the Nano's LED on
        key_state = E_KEY_DOWN;
        // Serial.println("key down");
      }
      break;

      case E_KEY_UP:
      {
        // do whatever you need to un-key the transmitter 
        digitalWrite(PIN_KEY_LINE, LOW);
        digitalWrite(13, 0);  // turn the Nano's LED off
        key_state = E_KEY_UP;
        char_sent_ms = millis();
        space_inserted = false;
        // Serial.println("key down");
      }
      break;
  }    
}

void activate_state(trx_state_e s)
{
// performs PTT control -- if necessary, activate the receiver or the transmitter 
// changes global 'curr_state' {E_STATE_RX, E_STATE_TX}
  switch (s)
  {
      case E_STATE_RX:
      {
        if(curr_state == E_STATE_RX)
        {
          // already in receive state, nothing to do!
        }
        else
        { 
          // turn transmitter off (drop PTT line)
          digitalWrite(PIN_PTT_LINE, 0); 
          // un-mute receiver
          curr_state = E_STATE_RX;
          // Serial.println();
          Serial.println("\n>Rx");
        }
      }
      break;

      case E_STATE_TX:
      {
        if(curr_state == E_STATE_TX)
        {
          // already in transmit state, nothing to do!
        }
        else
        {
          // turn transmitter on (raise PTT line) 
          digitalWrite(PIN_PTT_LINE, 1); 
          curr_state = E_STATE_TX;
          Serial.println("\n>Tx");
        }
      }
      break;
  }    
}

void send_dot()
{
  delay(dot_length_ms);  // wait for one dot period (space)

  // send a dot 
  tone(PIN_TONE_OUT, CW_TONE_HZ);
  set_key_state(E_KEY_DOWN);
  if(ch_counter % SERIAL_LINE_WIDTH == 0) Serial.println(); 
  Serial.print(".");
  delay(dot_length_ms);  // key down for one dot period
  noTone(PIN_TONE_OUT);
  set_key_state(E_KEY_UP); 
  ch_counter++;
}

void send_dash()
{
  delay(dot_length_ms);  // wait for one dot period (space)
  // send a dash 
  tone(PIN_TONE_OUT, CW_TONE_HZ);
  set_key_state(E_KEY_DOWN); 
  if(ch_counter % SERIAL_LINE_WIDTH == 0) Serial.println(); 
  Serial.print("-");
  delay(dot_length_ms * CW_DASH_LEN);  // key down for CW_DASH_LEN dot periods
  noTone(PIN_TONE_OUT);
  set_key_state(E_KEY_UP); 
  ch_counter++;
}

void send_letter_space()
{
  delay(dot_length_ms * 4);  // wait for 3 or 4 dot periods (choose which spacing you prefer)
  Serial.print(" ");
}

void send_word_space()
{
  delay(dot_length_ms * 7);  // wait for 6 or 7 dot periods (choose your preference)
  Serial.print("  ");
}

void send_morse_char(char c)
{
  // 'c' is a '.' or '-' char, send it 
  if(c == '.') send_dot();
  else if (c == '-') send_dash();
  // ignore anything else
}

void send_straight_key()
{
  // straight key is closed, so send until it goes open
  tone(PIN_TONE_OUT, CW_TONE_HZ);
  set_key_state(E_KEY_DOWN); 

  // monitor PIN_STRAIGHT_KEY until it goes open
  while (get_button(PIN_STRAIGHT_KEY)) delay(10); 
  
  noTone(PIN_TONE_OUT);
  set_key_state(E_KEY_UP); 
}


void play_message(String m, int s)
{
// sends the message in string 'm' as CW, with inter letter and word spacing
// s is the speed to play at; if s == 0, use the current speed.
  
  int i, j, n, old_s; 
  char buff[100];

  Serial.println(m);

  // alternative: use ch = m.charAt(index);
  m.toCharArray(buff, m.length()+1);

  if(s > 0)  // caller has passed in a speed to send message at 
  {
    old_s = dot_length_ms; // preserve the current keyer speed
    dot_length_ms = s;
  }

  digitalWrite(PIN_PTT_LINE, 1); // turn transmitter on 

  bool abort_flg = false;
  i=0; 
  
  while((unsigned int)!abort_flg and (i < m.length()))
  {
    // iterate thru the message string 
    if(buff[i] == ' ') send_word_space(); 
    else
    {
      if( (n = morse_lookup(buff[i])) == -1 )
      {
        // char not found, ignore it (but report it on Serial)
        Serial.print("Char in message not found in MorseTable <");
        Serial.print(buff[i]);
        Serial.println(">");
      }
      else
      {
        // char found, so send it as dots and dashes
        // Serial.println(n);
        for(j=1; j<7; j++) send_morse_char(MorseCode[n].ch[j]);
        send_letter_space();  // send an inter-letter space

        // check the keyer speed potentiometer (it may have changed!)
        if(s==0) read_keyer_speed();  // if speed has changed mid message 
                
        // you can interrupt messages with a left tap or right tap on the paddle
        if(get_button(PIN_PADDLE_L)) abort_flg = true; 
        if(get_button(PIN_PADDLE_R)) abort_flg = true; 
        if(get_button(PIN_STRAIGHT_KEY)) abort_flg = true;  // ...or a tap on the straight key
      } // else
    } // else
    i++;
  } // while  
  
  Serial.println();
  if(s > 0)  // reset speed back to what it was  
    dot_length_ms = old_s;

  digitalWrite(PIN_PTT_LINE, 0); // turn transmitter off

} // play_message


void setup()
{
  Serial.begin(9600);  
  Serial.println("Basic keyer VK3HN v1.0 4-Apr-2017. Please reuse, hack and pass on.");
  Serial.println("If you enjoy this script, leave a comment at http://vk3hn.wordpress.com");
  Serial.println("");

  pinMode(PIN_PADDLE_L,  INPUT_PULLUP);
  pinMode(PIN_PADDLE_R,  INPUT_PULLUP);
  pinMode(PIN_STRAIGHT_KEY, INPUT_PULLUP);

  pinMode(PIN_KEYER_MEM1,INPUT_PULLUP);
  pinMode(PIN_KEYER_MEM2,INPUT_PULLUP);
  pinMode(PIN_PTT_LINE,  INPUT_PULLUP);

  digitalWrite(PIN_PTT_LINE, 0);  // send the PTT line low

  curr_state = E_STATE_RX;
  key_state = E_KEY_UP;
  char_sent_ms = millis();
  space_inserted = false;
  paddle_squeezed = false;
  ch_counter = 0;

  last_ident_ms = millis(); 

  // dump out the MorseCode table for diagnostic
/*  for(int i=0; i<40; i++)
  {
    Serial.print(MorseCode[i].ch[0]);
    Serial.print(' ');
    for(int j=1; j<7; j++)
      Serial.print(MorseCode[i].ch[j]);
    Serial.println();
  }
*/
  // play the two messages as a diagnostic
//  play_message(morse_msg[0], 0);
//  delay(2000);
//  play_message(morse_msg[1], 0);
//  delay(2000);
}


void loop()
{  
//-- keyer code begins --------------------------------------------------
  // read the speed control 
  read_keyer_speed();

  // see if a memory button has been pushed
  if(get_button(PIN_KEYER_MEM1)) play_message(morse_msg[0], 0);  
  if(get_button(PIN_KEYER_MEM2)) play_message(morse_msg[1], 0); 
//   if(get_button(PIN_KEYER_MEM3)) play_message(morse_msg[2], 0); 

  // see if the paddle or key has been pressed
  bool l_paddle = get_button(PIN_PADDLE_L);
  bool r_paddle = get_button(PIN_PADDLE_R);
  bool straight_key = get_button(PIN_STRAIGHT_KEY); 

  // if something was pressed, activate PTT  
  if(l_paddle or r_paddle or straight_key) activate_state(E_STATE_TX);

  if(straight_key) send_straight_key();
  if(l_paddle) send_dot();
  if(r_paddle) send_dash();
  if(l_paddle and r_paddle) // paddle is being squeezed
  {
     send_dot();
     send_dash();
  }  

  curr_ms = millis();
  // if paddle has been idle for BREAK_IN_DELAY drop out of transmit 
  if((curr_state == E_STATE_TX) and (curr_ms - char_sent_ms) > BREAK_IN_DELAY)
  {
    // drop back to receive to implement break-in
    activate_state(E_STATE_RX); 
  }
  //-- keyer code ends --------------------------------------------------
  
  
  //-- CW ident code begins ---------------------------------------------
  
#ifdef CW_IDENT
  if(abs(curr_ms - last_ident_ms) > 1000)
  {
    // at a second boundary
    ident_secs_count++;
    last_ident_ms = curr_ms;
    if(ident_secs_count%10 == 0)
    {
      Serial.print(ident_secs_count);
      Serial.print(" ");
    }
  }

  if(ident_secs_count == CW_IDENT_SECS)
  {
    if(curr_state == E_STATE_RX) 
      play_message("VK3HN", CW_IDENT_WPM);  // only Id if not transmitting
    ident_secs_count = 0;
  }
#endif 
  //-- CW ident code end ------------------------------------------------
}
