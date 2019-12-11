extern "C" void reset6502();
extern "C" void nmi6502();
extern "C" void irq6502();
extern "C" void exec6502(uint32_t tickcount);
extern "C" void step6502();
extern "C" void hookexternal(void* funcptr);
extern "C" uint8_t read6502(uint16_t address);
extern "C" void write6502(uint16_t address, uint8_t value);
