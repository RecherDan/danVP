#include <stdio.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

int main(void) {
	register int a __asm__ ("r14");
	register int b __asm__ ("r15");
	srand (time(NULL));
	printf("TEST PROGRAM: start\n");
	a=7;
	int val=15;
	asm("mov $0x0,%rax");
	asm("mov $0x5,%rax");
	asm("mov $0x4,%rax");
	asm("mov $0x8,%rax");
	asm("mov $0x0,%rax");
	asm("mov $0x4,%rax");
	asm("mov $0x0,%rax");
	asm("mov $0x4,%rax");
	asm("mov $0x0,%rax");
	asm("mov $0x0,%rax");
	for ( int i=0 ; i<10000 ; i++ ) {
		if ( i%2000 != 0 ) {
			a=0x1212; // 123
		}
		else {
			a=0x1213; // 122
		}
		b=a;
	  	//if ( rand()%100000000 == 0 ) 
		//	asm("mov $0x121,%rax");
		//else
			//int x;
			//if ( i%1 == 0  ) {
			//	asm("mov $0x123,%r15");
			//}
			//	if ( rand()%1000 == 0 ) asm("mov $0x122,%r15");
			//if ( i>0 && i%1000 == 0 )
			//	asm("mov $0x122,%rax");
			//if ( (i > 2000) && (rand()%100 != 0) ) {
			//if ( i == 2000) {
			//	asm("mov $0x122,%rax");
			//}
			//}
			//asm("mov %r15,%r14");	
			//asm("mov %%rax,%0" : "=r"(x));
			//printf("val: %3x\n", b);
			
	}
printf("TEST PROGRAM: finish\n");

}
