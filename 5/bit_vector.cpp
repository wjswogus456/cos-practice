#include <cstdio>
#include <cstdlib>
#include <cstring>
#define BIT_VECTOR_LENGTH 4
#define MSB_INDEX BIT_VECTOR_LENGTH - 1
#define LSB_INDEX 0

void print_command(char *argv[]);
int check_validity(char *input);
void init_bit_vector(char *input, char *bv);
void print_bit_vector(char *bv);

unsigned int b2u(char *bv);
int b2s(char *bv);
int b2o(char *bv);
int b2t(char *bv);

void bit_addition(char *bv1, char *bv2, char *result);
unsigned int unsigned_addition(char *bv1, char *bv2);
int twos_complement_addition(char *bv1, char *bv2);
int ones_complement_addition(char *bv1, char *bv2);
int sign_magnitude_addition(char *bv1, char *bv2);


int main(int argc, char *argv[])
{
  char bv1[BIT_VECTOR_LENGTH];
  char bv2[BIT_VECTOR_LENGTH];

  if (argc != 3)
  {
    printf("Error: invalid number of arguments\n");
    print_command(argv);
  }

  if (!(check_validity(argv[1]) && check_validity(argv[2])))
  {
    printf("Error: invalid arguments\n");
    print_command(argv);
  }

  init_bit_vector(argv[1], bv1);
  init_bit_vector(argv[2], bv2);
  print_bit_vector(bv1);
  printf(" - b2u: %u\n", b2u(bv1));
  printf(" - b2s: %d\n", b2s(bv1));
  printf(" - b2o: %d\n", b2o(bv1));
  printf(" - b2t: %d\n", b2t(bv1));

  print_bit_vector(bv2);
  printf(" - b2u: %u\n", b2u(bv2));
  printf(" - b2s: %d\n", b2s(bv2));
  printf(" - b2o: %d\n", b2o(bv2));
  printf(" - b2t: %d\n", b2t(bv2));

  printf("Addition >>\n");
  printf(" - Unsigned addition: %u\n", unsigned_addition(bv1, bv2));
  printf(" - Sign magnitude addition: %d\n", sign_magnitude_addition(bv1, bv2));
  printf(" - One's complement addition: %d\n", ones_complement_addition(bv1, bv2));
  printf(" - Two's complement addition: %d\n", twos_complement_addition(bv1, bv2));

  return 0;
}

void print_command(char *argv[])
{
  printf("Command: %s <first bit vector> <second bit vector>\n", argv[0]);
  printf("Example: %s 0110 1000\n", argv[0]);
  exit(1);
}

int check_validity(char *input)
{
  int i, len;
  len = strlen(input);

  if (len < BIT_VECTOR_LENGTH)
  {
    return 0;
  }

  for (i=0; i<len; i++)
  {
    if (!(input[i] == '0' || input[i] == '1'))
      return 0;
  }

  return 1;
}

void init_bit_vector(char *input, char *bv)
{
  int i, len;
  len = strlen(input);

  for (i=0; i<len; i++)
  {
    if (input[i] == '0')
      bv[BIT_VECTOR_LENGTH-i-1] = 0;
    else
      bv[BIT_VECTOR_LENGTH-i-1] = 1;
  }
}

void print_bit_vector(char *bv)
{
  int i;
  printf("Bit Vector >>\n");
  for (i=MSB_INDEX; i>=LSB_INDEX; i--)
  {
    printf("%d", bv[i]);
  }
  printf("\n");
}

unsigned int b2u(char *bv)
{
  unsigned int ret;
  int i, power;

  ret = 0;
  power = 1;
  for (i = LSB_INDEX; i<= MSB_INDEX; i++)
    ret += bv[i] * (power << i);
  return ret;
}

int b2s(char *bv)
{
  int i, ret, power;
  ret = 0;
  power = 1;

  for (i = LSB_INDEX; i<MSB_INDEX; i++)
    ret += bv[i] * (power << i);

  if (bv[MSB_INDEX])
    ret *= -1;

  return ret;
}

int b2o(char *bv)
{
  int i, ret, power;
  ret = 0;
  power = 1;

  ret = -1 * bv[MSB_INDEX] * ((power << MSB_INDEX) - 1);
  for (i=LSB_INDEX; i<MSB_INDEX; i++)
    ret += bv[i] * (power << i);

  return ret;
}

int b2t(char *bv)
{
  int i, ret, power;
  ret = 0;
  power = 1;

  ret = -1 * bv[MSB_INDEX] * (power << MSB_INDEX);
  for (i=LSB_INDEX; i<MSB_INDEX; i++)
    ret += bv[i] * (power << i);

  return ret;
}

void bit_addition(char *bv1, char *bv2, char *result)
{
  int i, tmp;
  char carry[BIT_VECTOR_LENGTH+1];

  for (i=LSB_INDEX; i<=MSB_INDEX; i++)
  {
    tmp = bv1[i] + bv2[i] + carry[i];

    switch (tmp)
    {
      case 0:
        result[i] = 0;
        carry[i+1] = 0;
        break;

      case 1:
        result[i] = 1;
        carry[i+1] = 0;
        break;

      case 2:
        result[i] = 0;
        carry[i+1] = 1;
        break;

      case 3:
        result[i] = 1;
        carry[i+1] = 1;
        break;
    }
  }
}


unsigned int unsigned_addition(char *bv1, char *bv2)
{
  char result[BIT_VECTOR_LENGTH];
  bit_addition(bv1, bv2, result);
  return b2u(result);
}

int sign_magnitude_addition(char *bv1, char *bv2)
{
  char result[BIT_VECTOR_LENGTH];
  bit_addition(bv1, bv2, result);
  return b2s(result);
}

int ones_complement_addition(char *bv1, char *bv2)
{
  char result[BIT_VECTOR_LENGTH];
  bit_addition(bv1, bv2, result);
  return b2o(result);
}

int twos_complement_addition(char *bv1, char *bv2)
{
  char result[BIT_VECTOR_LENGTH];
  bit_addition(bv1, bv2, result);
  return b2t(result);
}












