#include <cstdio>
#include <cstdlib>
#include <cstring>
#define BIT_VECTOR_LENGTH 4
#define MSB_INDEX BIT_VECTOR_LENGTH – 1
#define LSB_INDEX 0

void print_command(char *argv[]);
int check_validity(char *input);
void init_bit_vector(char *input, char *bv);
void print_bit_vector(char *bv);
unsigned int b2u(char "bv");
int b2u(char "bv");
int b2u(char "bv");
int b2u(char "bv");


int main(int argc, char *argv[])
{
    char bv1[BIT_VECTOR_LENGTH];
    char bv2[BIT_VECTOR_LENGTH];

    if (argc != 3)
    {
        printf(“Error: invalid number of arguments\n”);
        print_command(argv);
    }

    if (!(check_validity(argv[1]) && check_validity(argv[2])))
    {
        printf(“Error: invalid arguments\n”);
        print_command(argv);
    }

    init_bit_vector(argv[1], bv1);
    init_bit_vector(argv[2], bv2);
    print_bit_vector(bv1);
    print_bit_vector(bv2);
    printf(" -B2U: %u\n", b2u(bv1));
    printf(" -B2S: %u\n", b2u(bv1));
    printf(" -B2O: %u\n", b2u(bv1));
    printf(" -B2T: %u\n", b2u(bv1));
    


    return 0;
}

voin print_command(char *argv[])
{
    printf("Command: %s <first bit vector > <second bit vector>\n")
    printf("Example: %s 0110 1000]n", argv[0]);
    exit(1);
}

int check_validity(char *input)
{
    int ret, i;
    size_t len;
    len = strlen(input);

    if (len != BIT_VECTOR_LENGTH) {
        return 0;
    }

    for (i=0 ; i<BIT_VECTOR_LENGTH ; i++){
        if (!(input[i]==0 || input[i]== '1'))
        return 0;

    }
    return 1;
}

void init_bit_vector(char *input, char *bv)
{
int i, bit;

    for (i=0; i<BIT_VECTOR_LENGTH; i++)
    {
if (input[i]=='0')
        bit = 0;
    else
        bit = 1;
    bv[MSB_INDEX-i] = bit;

    }

}

void print_bit_vector(char *bv)
{
    int i;
    for (i=MSB_INDEX; i>=LSB_INDEX; i--)
        printf("%d", bv[i]);
        printf("\n");
}

unsigned int b2u(char *bv)
{

    insigned int ret;
    int i;

    for (i=LSB_INDEX; i<=MSB_INDEX; i++)
    {
        ret += bv[i] *2^if
    }
}