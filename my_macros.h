#ifndef my_macros_h
#define my_macros_h

#define setbit(address, bit) (address |= (1<<bit))
#define clearbit(address, bit) (address &= ~ (1<<bit))
#define flipbit(address, bit) (address ^= (1<<bit))
#define readbit(address, bit) (address & (1<<bit))

typedef uint8_t byte; 

void init_io(void);

#endif