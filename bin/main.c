#include <stdint.h>
#include <limits.h>
#if CHAR_BIT != 8 || INT_MAX != 2147483647 || INT_MIN != (-2147483647 - 1)
#error Flower C backend requires 8-bit bytes and a signed 32-bit C int type
#endif
typedef char flower_int_storage_must_be_4_bytes[(sizeof(int) == 4) ? 1 : -1];
#include <stdio.h>
#define FLOWER_INTEGER_i8(value) ((int8_t)(((uint8_t)(value) <= INT8_MAX) ? (int64_t)(uint8_t)(value) : (-INT64_C(1) - (int64_t)(UINT8_MAX - (uint8_t)(value)))))
#define FLOWER_INTEGER_u8(value) ((uint8_t)(value))
static int8_t flower_integer_i8(uint64_t value) {
  return FLOWER_INTEGER_i8(value);
}
static uint8_t flower_integer_u8(uint64_t value) {
  return FLOWER_INTEGER_u8(value);
}
#define FLOWER_INTEGER_i16(value) ((int16_t)(((uint16_t)(value) <= INT16_MAX) ? (int64_t)(uint16_t)(value) : (-INT64_C(1) - (int64_t)(UINT16_MAX - (uint16_t)(value)))))
#define FLOWER_INTEGER_u16(value) ((uint16_t)(value))
static int16_t flower_integer_i16(uint64_t value) {
  return FLOWER_INTEGER_i16(value);
}
static uint16_t flower_integer_u16(uint64_t value) {
  return FLOWER_INTEGER_u16(value);
}
#define FLOWER_INTEGER_i32(value) ((int32_t)(((uint32_t)(value) <= INT32_MAX) ? (int64_t)(uint32_t)(value) : (-INT64_C(1) - (int64_t)(UINT32_MAX - (uint32_t)(value)))))
#define FLOWER_INTEGER_u32(value) ((uint32_t)(value))
static int32_t flower_integer_i32(uint64_t value) {
  return FLOWER_INTEGER_i32(value);
}
static uint32_t flower_integer_u32(uint64_t value) {
  return FLOWER_INTEGER_u32(value);
}
#define FLOWER_INTEGER_i64(value) ((int64_t)(((uint64_t)(value) <= INT64_MAX) ? (int64_t)(uint64_t)(value) : (-INT64_C(1) - (int64_t)(UINT64_MAX - (uint64_t)(value)))))
#define FLOWER_INTEGER_u64(value) ((uint64_t)(value))
static int64_t flower_integer_i64(uint64_t value) {
  return FLOWER_INTEGER_i64(value);
}
static uint64_t flower_integer_u64(uint64_t value) {
  return FLOWER_INTEGER_u64(value);
}
typedef struct { char* data; int length; } flower_string;
static int flower_cstr_len(char* s) {
  int i = 0;
  if (s == NULL) return 0;
  while (s[i] != '\0') i++;
  return i;
}
static flower_string flower_string_from_cstr(char* s) {
  flower_string out;
  out.data = s;
  out.length = (int)flower_cstr_len(s);
  return out;
}
static int flower_string_eq(flower_string a, flower_string b) {
  int i = 0;
  if (a.length != b.length) return 0;
  while (i < a.length) {
      if (a.data[i] != b.data[i]) return 0;
      i++;
  }
  return 1;
}
static void flower_print_string(flower_string s) {
  int i = 0;
  while (i < s.length) {
      putchar(s.data[i]);
      i++;
  }
}
#include <stdlib.h>
static void flower_integer_trap(const char* message) {
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
#define FLOWER_add_i8(a, b) FLOWER_INTEGER_i8((uint64_t)(a) + (uint64_t)(b))
static int8_t flower_add_i8(int8_t a, int8_t b) {
  return FLOWER_add_i8(a, b);
}
#define FLOWER_sub_i8(a, b) FLOWER_INTEGER_i8((uint64_t)(a) - (uint64_t)(b))
static int8_t flower_sub_i8(int8_t a, int8_t b) {
  return FLOWER_sub_i8(a, b);
}
#define FLOWER_mul_i8(a, b) FLOWER_INTEGER_i8((uint64_t)(a) * (uint64_t)(b))
static int8_t flower_mul_i8(int8_t a, int8_t b) {
  return FLOWER_mul_i8(a, b);
}
#define FLOWER_neg_i8(a) FLOWER_INTEGER_i8(UINT64_C(0) - (uint64_t)(a))
static int8_t flower_neg_i8(int8_t a) {
  return FLOWER_neg_i8(a);
}
#define FLOWER_div_i8(a, b) ((b) == 0 ? (flower_integer_trap("integer division by zero"), (int8_t)0) : ((a) == INT8_MIN && (b) == -1 ? (flower_integer_trap("signed integer division overflow"), (int8_t)0) : (int8_t)((a) / (b))))
static int8_t flower_div_i8(int8_t a, int8_t b) {
  return FLOWER_div_i8(a, b);
}
#define FLOWER_add_u8(a, b) FLOWER_INTEGER_u8((uint64_t)(a) + (uint64_t)(b))
static uint8_t flower_add_u8(uint8_t a, uint8_t b) {
  return FLOWER_add_u8(a, b);
}
#define FLOWER_sub_u8(a, b) FLOWER_INTEGER_u8((uint64_t)(a) - (uint64_t)(b))
static uint8_t flower_sub_u8(uint8_t a, uint8_t b) {
  return FLOWER_sub_u8(a, b);
}
#define FLOWER_mul_u8(a, b) FLOWER_INTEGER_u8((uint64_t)(a) * (uint64_t)(b))
static uint8_t flower_mul_u8(uint8_t a, uint8_t b) {
  return FLOWER_mul_u8(a, b);
}
#define FLOWER_neg_u8(a) FLOWER_INTEGER_u8(UINT64_C(0) - (uint64_t)(a))
static uint8_t flower_neg_u8(uint8_t a) {
  return FLOWER_neg_u8(a);
}
#define FLOWER_div_u8(a, b) ((b) == 0 ? (flower_integer_trap("integer division by zero"), (uint8_t)0) : (uint8_t)((a) / (b)))
static uint8_t flower_div_u8(uint8_t a, uint8_t b) {
  return FLOWER_div_u8(a, b);
}
#define FLOWER_add_i16(a, b) FLOWER_INTEGER_i16((uint64_t)(a) + (uint64_t)(b))
static int16_t flower_add_i16(int16_t a, int16_t b) {
  return FLOWER_add_i16(a, b);
}
#define FLOWER_sub_i16(a, b) FLOWER_INTEGER_i16((uint64_t)(a) - (uint64_t)(b))
static int16_t flower_sub_i16(int16_t a, int16_t b) {
  return FLOWER_sub_i16(a, b);
}
#define FLOWER_mul_i16(a, b) FLOWER_INTEGER_i16((uint64_t)(a) * (uint64_t)(b))
static int16_t flower_mul_i16(int16_t a, int16_t b) {
  return FLOWER_mul_i16(a, b);
}
#define FLOWER_neg_i16(a) FLOWER_INTEGER_i16(UINT64_C(0) - (uint64_t)(a))
static int16_t flower_neg_i16(int16_t a) {
  return FLOWER_neg_i16(a);
}
#define FLOWER_div_i16(a, b) ((b) == 0 ? (flower_integer_trap("integer division by zero"), (int16_t)0) : ((a) == INT16_MIN && (b) == -1 ? (flower_integer_trap("signed integer division overflow"), (int16_t)0) : (int16_t)((a) / (b))))
static int16_t flower_div_i16(int16_t a, int16_t b) {
  return FLOWER_div_i16(a, b);
}
#define FLOWER_add_u16(a, b) FLOWER_INTEGER_u16((uint64_t)(a) + (uint64_t)(b))
static uint16_t flower_add_u16(uint16_t a, uint16_t b) {
  return FLOWER_add_u16(a, b);
}
#define FLOWER_sub_u16(a, b) FLOWER_INTEGER_u16((uint64_t)(a) - (uint64_t)(b))
static uint16_t flower_sub_u16(uint16_t a, uint16_t b) {
  return FLOWER_sub_u16(a, b);
}
#define FLOWER_mul_u16(a, b) FLOWER_INTEGER_u16((uint64_t)(a) * (uint64_t)(b))
static uint16_t flower_mul_u16(uint16_t a, uint16_t b) {
  return FLOWER_mul_u16(a, b);
}
#define FLOWER_neg_u16(a) FLOWER_INTEGER_u16(UINT64_C(0) - (uint64_t)(a))
static uint16_t flower_neg_u16(uint16_t a) {
  return FLOWER_neg_u16(a);
}
#define FLOWER_div_u16(a, b) ((b) == 0 ? (flower_integer_trap("integer division by zero"), (uint16_t)0) : (uint16_t)((a) / (b)))
static uint16_t flower_div_u16(uint16_t a, uint16_t b) {
  return FLOWER_div_u16(a, b);
}
#define FLOWER_add_i32(a, b) FLOWER_INTEGER_i32((uint64_t)(a) + (uint64_t)(b))
static int32_t flower_add_i32(int32_t a, int32_t b) {
  return FLOWER_add_i32(a, b);
}
#define FLOWER_sub_i32(a, b) FLOWER_INTEGER_i32((uint64_t)(a) - (uint64_t)(b))
static int32_t flower_sub_i32(int32_t a, int32_t b) {
  return FLOWER_sub_i32(a, b);
}
#define FLOWER_mul_i32(a, b) FLOWER_INTEGER_i32((uint64_t)(a) * (uint64_t)(b))
static int32_t flower_mul_i32(int32_t a, int32_t b) {
  return FLOWER_mul_i32(a, b);
}
#define FLOWER_neg_i32(a) FLOWER_INTEGER_i32(UINT64_C(0) - (uint64_t)(a))
static int32_t flower_neg_i32(int32_t a) {
  return FLOWER_neg_i32(a);
}
#define FLOWER_div_i32(a, b) ((b) == 0 ? (flower_integer_trap("integer division by zero"), (int32_t)0) : ((a) == INT32_MIN && (b) == -1 ? (flower_integer_trap("signed integer division overflow"), (int32_t)0) : (int32_t)((a) / (b))))
static int32_t flower_div_i32(int32_t a, int32_t b) {
  return FLOWER_div_i32(a, b);
}
#define FLOWER_add_u32(a, b) FLOWER_INTEGER_u32((uint64_t)(a) + (uint64_t)(b))
static uint32_t flower_add_u32(uint32_t a, uint32_t b) {
  return FLOWER_add_u32(a, b);
}
#define FLOWER_sub_u32(a, b) FLOWER_INTEGER_u32((uint64_t)(a) - (uint64_t)(b))
static uint32_t flower_sub_u32(uint32_t a, uint32_t b) {
  return FLOWER_sub_u32(a, b);
}
#define FLOWER_mul_u32(a, b) FLOWER_INTEGER_u32((uint64_t)(a) * (uint64_t)(b))
static uint32_t flower_mul_u32(uint32_t a, uint32_t b) {
  return FLOWER_mul_u32(a, b);
}
#define FLOWER_neg_u32(a) FLOWER_INTEGER_u32(UINT64_C(0) - (uint64_t)(a))
static uint32_t flower_neg_u32(uint32_t a) {
  return FLOWER_neg_u32(a);
}
#define FLOWER_div_u32(a, b) ((b) == 0 ? (flower_integer_trap("integer division by zero"), (uint32_t)0) : (uint32_t)((a) / (b)))
static uint32_t flower_div_u32(uint32_t a, uint32_t b) {
  return FLOWER_div_u32(a, b);
}
#define FLOWER_add_i64(a, b) FLOWER_INTEGER_i64((uint64_t)(a) + (uint64_t)(b))
static int64_t flower_add_i64(int64_t a, int64_t b) {
  return FLOWER_add_i64(a, b);
}
#define FLOWER_sub_i64(a, b) FLOWER_INTEGER_i64((uint64_t)(a) - (uint64_t)(b))
static int64_t flower_sub_i64(int64_t a, int64_t b) {
  return FLOWER_sub_i64(a, b);
}
#define FLOWER_mul_i64(a, b) FLOWER_INTEGER_i64((uint64_t)(a) * (uint64_t)(b))
static int64_t flower_mul_i64(int64_t a, int64_t b) {
  return FLOWER_mul_i64(a, b);
}
#define FLOWER_neg_i64(a) FLOWER_INTEGER_i64(UINT64_C(0) - (uint64_t)(a))
static int64_t flower_neg_i64(int64_t a) {
  return FLOWER_neg_i64(a);
}
#define FLOWER_div_i64(a, b) ((b) == 0 ? (flower_integer_trap("integer division by zero"), (int64_t)0) : ((a) == INT64_MIN && (b) == -1 ? (flower_integer_trap("signed integer division overflow"), (int64_t)0) : (int64_t)((a) / (b))))
static int64_t flower_div_i64(int64_t a, int64_t b) {
  return FLOWER_div_i64(a, b);
}
#define FLOWER_add_u64(a, b) FLOWER_INTEGER_u64((uint64_t)(a) + (uint64_t)(b))
static uint64_t flower_add_u64(uint64_t a, uint64_t b) {
  return FLOWER_add_u64(a, b);
}
#define FLOWER_sub_u64(a, b) FLOWER_INTEGER_u64((uint64_t)(a) - (uint64_t)(b))
static uint64_t flower_sub_u64(uint64_t a, uint64_t b) {
  return FLOWER_sub_u64(a, b);
}
#define FLOWER_mul_u64(a, b) FLOWER_INTEGER_u64((uint64_t)(a) * (uint64_t)(b))
static uint64_t flower_mul_u64(uint64_t a, uint64_t b) {
  return FLOWER_mul_u64(a, b);
}
#define FLOWER_neg_u64(a) FLOWER_INTEGER_u64(UINT64_C(0) - (uint64_t)(a))
static uint64_t flower_neg_u64(uint64_t a) {
  return FLOWER_neg_u64(a);
}
#define FLOWER_div_u64(a, b) ((b) == 0 ? (flower_integer_trap("integer division by zero"), (uint64_t)0) : (uint64_t)((a) / (b)))
static uint64_t flower_div_u64(uint64_t a, uint64_t b) {
  return FLOWER_div_u64(a, b);
}
flower_string VERSION = ((flower_string){ "0.1.0", (int)(sizeof("0.1.0") - 1) });


void mod_main_flo_help() {
flower_print_string(((flower_string){ "Usage:\n", (int)(sizeof("Usage:\n") - 1) }));
flower_print_string(((flower_string){ "\tbioflo\t<command>\t[arguments]\n", (int)(sizeof("\tbioflo\t<command>\t[arguments]\n") - 1) }));
flower_print_string(((flower_string){ "\nCommands:\n", (int)(sizeof("\nCommands:\n") - 1) }));
flower_print_string(((flower_string){ "\th, \thelp\t\tShow this help message\n", (int)(sizeof("\th, \thelp\t\tShow this help message\n") - 1) }));
flower_print_string(((flower_string){ "\tv, \tversion\tShow the current version of BioFlo\n", (int)(sizeof("\tv, \tversion\tShow the current version of BioFlo\n") - 1) }));
flower_print_string(((flower_string){ "\tp, process\tProcess a biological sequence file\n", (int)(sizeof("\tp, process\tProcess a biological sequence file\n") - 1) }));
flower_print_string(((flower_string){ "\nExample:\n", (int)(sizeof("\nExample:\n") - 1) }));
flower_print_string(((flower_string){ "\tbioflo --help\n", (int)(sizeof("\tbioflo --help\n") - 1) }));
flower_print_string(((flower_string){ "\tbioflo -v\n", (int)(sizeof("\tbioflo -v\n") - 1) }));
flower_print_string(((flower_string){ "\tbioflo process sequence.fasta\n", (int)(sizeof("\tbioflo process sequence.fasta\n") - 1) }));
flower_print_string(((flower_string){ "\nOptions may use one or two leading dashes\n", (int)(sizeof("\nOptions may use one or two leading dashes\n") - 1) }));
flower_print_string(((flower_string){ "\nTips:\n", (int)(sizeof("\nTips:\n") - 1) }));
flower_print_string(((flower_string){ "\tFlags are currently case sensitive: -HELP will not work\n", (int)(sizeof("\tFlags are currently case sensitive: -HELP will not work\n") - 1) }));
flower_print_string(((flower_string){ "\nPlease reach out to contact.ivy353@gmail.com for any questions, comments, or concerns <3\n", (int)(sizeof("\nPlease reach out to contact.ivy353@gmail.com for any questions, comments, or concerns <3\n") - 1) }));
}


int32_t main(int32_t argc, char* argv[]) {
if ((argc < ((int32_t)(INT64_C(2))))) {
flower_print_string(((flower_string){ "Missing command. Use `--help` for usage information.\n", (int)(sizeof("Missing command. Use `--help` for usage information.\n") - 1) }));
return ((int32_t)(INT64_C(1)));
}
char* argument = argv[((int32_t)(INT64_C(1)))];
while ((argument[((int32_t)(INT64_C(0)))] == '-')) {
if ((argument[((int32_t)(INT64_C(1)))] == '\0')) {
flower_print_string(((flower_string){ "Missing command. Use `--help` for usage information.\n", (int)(sizeof("Missing command. Use `--help` for usage information.\n") - 1) }));
return ((int32_t)(INT64_C(1)));
}
argument = (argument + ((int32_t)(INT64_C(1))));
}
flower_string command = flower_string_from_cstr(argument);
if ((flower_string_eq(command, ((flower_string){ "h", (int)(sizeof("h") - 1) }))  ||  flower_string_eq(command, ((flower_string){ "help", (int)(sizeof("help") - 1) })))) {
mod_main_flo_help();
return ((int32_t)(INT64_C(0)));
}
if ((flower_string_eq(command, ((flower_string){ "v", (int)(sizeof("v") - 1) }))  ||  flower_string_eq(command, ((flower_string){ "version", (int)(sizeof("version") - 1) })))) {
flower_print_string(((flower_string){ "Version:\t", (int)(sizeof("Version:\t") - 1) }));
flower_print_string(VERSION);
printf("%c", '\n');
return ((int32_t)(INT64_C(0)));
}
if ((flower_string_eq(command, ((flower_string){ "p", (int)(sizeof("p") - 1) }))  ||  flower_string_eq(command, ((flower_string){ "process", (int)(sizeof("process") - 1) })))) {
if ((argc < ((int32_t)(INT64_C(3))))) {
flower_print_string(((flower_string){ "Missing filepath. Usage: `bioflo process <filepath>`\n", (int)(sizeof("Missing filepath. Usage: `bioflo process <filepath>`\n") - 1) }));
return ((int32_t)(INT64_C(1)));
}
flower_print_string(((flower_string){ "Processing file: '", (int)(sizeof("Processing file: '") - 1) }));
printf("%s", argv[((int32_t)(INT64_C(2)))]);
flower_print_string(((flower_string){ "'\n", (int)(sizeof("'\n") - 1) }));
return ((int32_t)(INT64_C(0)));
}
flower_print_string(((flower_string){ "Unknown command: '", (int)(sizeof("Unknown command: '") - 1) }));
flower_print_string(command);
flower_print_string(((flower_string){ "'. Use `--help` for usage information.\n", (int)(sizeof("'. Use `--help` for usage information.\n") - 1) }));
return ((int32_t)(INT64_C(1)));
}
