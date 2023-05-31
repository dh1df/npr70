extern SPI spi_0;
extern SPI spi_1;
extern DigitalOut CS2;
extern InterruptIn Int_SI4463;

extern AnalogIn Random_pin;
extern DigitalOut LED_RX_loc;
extern DigitalOut LED_connected;
extern DigitalOut SI4463_SDN;
extern DigitalInOut FDD_trig_pin;
extern InterruptIn FDD_trig_IRQ;
extern DigitalOut PTT_PA_pin;

extern void loop(void);
extern void init1(void);
