#include <stdio.h>
#include <math.h>
int main (void)
{
    {
        double a = sin (1e22), b = log (17.1);
        double c = exp (0.42);
        double d = 173746*a + 94228*b - 78487*c;
        printf ("a = %.16e\n", a);
        printf ("173746*a = %.16e\n", 173746*a);
        printf ("b = %.16e\n", b);
        printf ("94228*b = %.16e\n", 94228*b);
        printf ("c = %.16e\n", c);
        printf ("-78487*c = %.16e\n", -78487*c);
        printf ("d = %.16e\n", d);
    }
 
    {
        //_Decimal64 a = sin (1e22);
        //_Decimal64 b = log (17.1);
        //_Decimal64 c = exp (0.42);
        //_Decimal64 d = 173746*a+94228*b-78487*c;
        //printf ("d = %.16e\n", (double) d);
    }
    
    return 0;
}
