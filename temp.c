#include <stdio.h>

int read[10], i = 0;

int main()
{
    int num;
    scanf("%x", &num);

    num = num >> 8;
    int get = num & 0x0F;

    printf("%x", get);

    while(delay == 10)
    {
        // triggered every ten seconds
        for(int j = 0; j < i; j++)
        {
            printf("%d", read[j]);
        }
        i = 0;
    }
    return 0;
}

void __interrupt _isr()
{
    // triggered every one second
    read[i++] = PORTB;
}