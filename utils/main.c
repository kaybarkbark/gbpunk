#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// function pointer example, since I never used them

void printx(uint8_t a, uint8_t b, uint8_t c){
	printf("X says %i %i %i\n", a, b, c);
}

void printy(uint8_t a, uint8_t b, uint8_t c){
	printf("Y says %i %i %i\n", a, b, c);
}

int main(){
	uint8_t a = 1;
	uint8_t b = 2;
	uint8_t c = 3;
	void (*func_ptr)(uint8_t, uint8_t, uint8_t);
	func_ptr = &printx;
	(*func_ptr)(a, b, c);
	func_ptr = &printy;
	(*func_ptr)(a, b, c);
}
