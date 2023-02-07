const int phi_pin = 13; // PORTB.5
const int nwr_pin = 12;
const int nrd_pin = 11;
const int ncs_pin = 10;

const int addr_data_pin = 9; // PORTB.1
const int addr_clk_pin = 8; // PORTB.0

const int data_pins[8] = {
  2, 3, 15, 14, 4, 5, 6, 7 // PIN 2 = PORTD.2
};

const int sensor_read_pin = 16; // PINC.2
const int sensor_vout_pin = 17;


void writeCartByte(unsigned int address, unsigned int value)
{
  PORTB &= ~BIT(0); // addr_clk_pin
  
  int i;
  for(i = 0; i < 16; i++)
  {
    if( (address&(1<<(15-i))) ? HIGH : LOW ) PORTB |= BIT(1); // addr_data_pin
    else PORTB &= ~BIT(1);
    PORTB |= BIT(0); // addr_clk_pin
    PORTB &= ~BIT(0); // addr_clk_pin
  }
  
  
  
  setWriteMode(address < 0x8000);  // is_rom?
 
  setData(value);
  
  performWrite();

  if(address >= 0x8000) // is_sram?
  {
    PORTB &= ~BIT(5); // PHI clock signal
    asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
    PORTB |= BIT(5);
    asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
    PORTB &= ~BIT(5);
  }
  
  setWaitMode();
}


void writeCartByte(unsigned int address, unsigned int value)
{
  setAddress(address);
  
  setWriteMode(address < 0x8000);  // is_rom?
 
  setData(value);
  
  performWrite();

  if(address >= 0x8000) // is_sram?
  {
    PORTB &= ~BIT(5); // PHI clock signal
    asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
    PORTB |= BIT(5);
    asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop");
    PORTB &= ~BIT(5);
  }
  
  setWaitMode();
}


static inline void setAddress(unsigned int addr)
{
  PORTB &= ~BIT(0); // addr_clk_pin
  
  int i;
  for(i = 0; i < 16; i++)
  {
    if( (addr&(1<<(15-i))) ? HIGH : LOW ) PORTB |= BIT(1); // addr_data_pin
    else PORTB &= ~BIT(1);
    PORTB |= BIT(0); // addr_clk_pin
    PORTB &= ~BIT(0); // addr_clk_pin
  }
}

static inline void setData(unsigned int value)
{
  int i;
  for(i = 0; i < 8; i++)
   digitalWrite(data_pins[i], value & (1<<i) ? HIGH : LOW);
}
static inline void setWriteMode(int is_rom)
{
  if(is_rom)
    digitalWrite(ncs_pin, HIGH);
  else
    digitalWrite(ncs_pin, LOW);
  
  int i;
  for(i = 0; i < 8; i++)
    pinMode(data_pins[i], OUTPUT);
}

static inline void performWrite(void)
{
  digitalWrite(nwr_pin, LOW);
  digitalWrite(nrd_pin, HIGH);
}

static inline void setWaitMode(void)
{
  digitalWrite(ncs_pin, HIGH);
  digitalWrite(nwr_pin, HIGH);
  digitalWrite(nrd_pin, HIGH);
  
  int i;
  for(i = 0; i < 8; i++)
    pinMode(data_pins[i], INPUT);
}
