
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>


#define BAUD     9600
#define UBRR_VAL ((F_CPU/16/BAUD)-1)


#define ROW_MASK   ((1<<PD2)|(1<<PD3)|(1<<PD4)|(1<<PD5))
#define COL_MASKD  ((1<<PD6)|(1<<PD7))
#define COL_MASKB  ((1<<PB0)|(1<<PB1))


#define SERVO_DDR  DDRB
#define SERVO_PIN  PB2


#define LED_DDR    DDRB
#define LED_PORT   PORTB
#define LED_PIN    PB5


#define RST_DDR    DDRC
#define RST_PORT   PORTC
#define RST_PIN    PC6


#define AREF_DDR   DDRC
#define AREF_PORT  PORTC

// passwords
const char *doorCode  = "1111";       
const char *serialPwd = "Password";


volatile uint32_t ms_ticks       = 0;
volatile uint32_t unlockDeadline = 0;
volatile uint8_t  unlocked       = 0;


void     uart_init(void);
void     uart_putchar(char c);
char     uart_getchar(void);
void     uart_putstr(const char *s);
uint8_t  scan_keypad(char *out);
uint8_t  getKeyStable(char *out);
void     pwm_servo_us(uint16_t us);
void     setup_timers(void);
uint8_t  check_serial_pwd(void);
void     start_unlock(void);
void     lock_down(void);

ISR(TIMER2_COMPA_vect) {
    static uint16_t hb = 0;
    ms_ticks++;
    if (++hb >= 500) {
        hb = 0;
        LED_PORT ^= (1<<LED_PIN);
    }
}

int main(void) {
    DDRD  |= ROW_MASK;
    PORTD |= ROW_MASK;
    DDRD  &= ~COL_MASKD; PORTD |= COL_MASKD;
    DDRB  &= ~COL_MASKB; PORTB |= COL_MASKB;
    SERVO_DDR |= (1<<SERVO_PIN);
    LED_DDR   |= (1<<LED_PIN);
    LED_PORT  &= ~(1<<LED_PIN);
    RST_DDR   &= ~(1<<RST_PIN);
    RST_PORT  |=  (1<<RST_PIN);
    AREF_DDR  &= ~(1<<PC5);
    AREF_PORT &= ~(1<<PC5);

    uart_init();

   
    ICR1   = 39999;
    TCCR1A = (1<<COM1B1)|(1<<WGM11);
    TCCR1B = (1<<WGM13)|(1<<WGM12)|(1<<CS11);
    pwm_servo_us(1000);  // locking


    setup_timers();
    sei();

    uart_putstr("Enter 4-digit code:\r\n");

    char entry[5] = {0}, lastKey = 0, key;
    uint8_t idx = 0;

    while (1) {
        if (getKeyStable(&key)) {
            if (key != lastKey) {
                lastKey = key;
                uart_putchar(key);
                entry[idx++] = key;
                if (idx == strlen(doorCode)) {
                    uart_putstr("\r\n");
                    if (!strcmp(entry, doorCode)) {
                        uart_putstr("Code OK. Enter password:\r\n");
                        if (check_serial_pwd()) start_unlock();
                        else uart_putstr("Wrong password. Try code again:\r\n");
                    } else {
                        uart_putstr("Wrong code. Try again:\r\n");
                    }
                    idx = 0;
                    memset(entry, 0, sizeof(entry));
                }
            }
        } else {
            lastKey = 0;
        }
        // Autolock
        if (unlocked && ms_ticks >= unlockDeadline) {
            lock_down();
        }
    }
}

void uart_init(void) {
    UBRR0 = UBRR_VAL;
    UCSR0B = (1<<TXEN0)|(1<<RXEN0);
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

void uart_putchar(char c) {
    while (!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}

char uart_getchar(void) {
    while (!(UCSR0A & (1<<RXC0)));
    return UDR0;
}

void uart_putstr(const char *s) {
    while (*s) uart_putchar(*s++);
}


uint8_t scan_keypad(char *out) {
    static const char map[4][4] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };
    for (uint8_t r = 0; r < 4; r++) {
        PORTD = (PORTD & ~ROW_MASK) | (1 << (PD2 + r));
        _delay_us(40);
        uint8_t cd = (~PIND & COL_MASKD) >> PD6;
        uint8_t cb = (~PINB & COL_MASKB) >> PB0;
        uint8_t col = 0xFF;
        if (cd & 0x01) col = 0;
        if (cd & 0x02) col = 1;
        if (cb & 0x01) col = 2;
        if (cb & 0x02) col = 3;
        if (col < 4) {
            *out = map[r][col];
            PORTD |= ROW_MASK;
            return 1;
        }
    }
    PORTD |= ROW_MASK;
    return 0;
}

// Debounce
uint8_t getKeyStable(char *out) {
    char a, b;
    if (!scan_keypad(&a))  return 0;
    _delay_ms(30);
    if (!scan_keypad(&b))  return 0;
    if (a == b) {
        *out = a;
        return 1;
    }
    return 0;
}


void pwm_servo_us(uint16_t us) {
    OCR1B = us * 2;
}

void setup_timers(void) {
    
    TCCR2A = (1<<WGM21);
    OCR2A  = (F_CPU/64/1000) - 1;
    TCCR2B = (1<<CS22);
    TIMSK2 = (1<<OCIE2A);
}


uint8_t check_serial_pwd(void) {
    char buf[16] = {0}, c;
    uint8_t i = 0;
    while (1) {
        c = uart_getchar();
        if (c=='\r' || c=='\n' || i>=15) break;
        buf[i++] = c;
        uart_putchar(c);
    }
    uart_putstr("\r\n");
    buf[i] = 0;
    return (strcmp(buf, serialPwd) == 0);
}

void start_unlock(void) {
    uart_putstr("Access granted. Unlocking...\r\n");
    pwm_servo_us(2000);             
    unlocked = 1;
    unlockDeadline = ms_ticks + 5000;
}

void lock_down(void) {
    pwm_servo_us(1000);             
    unlocked = 0;
    uart_putstr("Locked.\r\nEnter 4-digit code:\r\n");
}
