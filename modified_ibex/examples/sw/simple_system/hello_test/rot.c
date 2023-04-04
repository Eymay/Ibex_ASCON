#include<stdint.h>

int main()
{
int a = 15;
int b = 2;

int *c;

*c = ((a>>b) | (a<<(32-b)));
return 0;
}
