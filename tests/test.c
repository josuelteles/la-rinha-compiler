/**
 * @file test.c
 *
 * @brief Rinha Language Interpreter - Compiler Development Challenge
 *
 * For more information, please visit the documentation at:
 * [rinha-de-compiler](https://github.com/aripiprazole/rinha-de-compiler)
 *
 * @author Josuel Teles
 * @date September 14, 2023
 */

#include "test.h"
#include "rinha.h"


TEST(rinha_hello_world) {
  char *code =
      " print(\"Hello, World!\");";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_hello_world", code, &response, true);

  EXPECT_EQ(response.type, STRING);
  EXPECT_STREQ(response.string, "Hello, World!");
}

TEST(rinha_fibonacci) {

  char *code =
     "let fib = fn (n) => {\n"
     "    if (n < 2) {    \n"
     "        n           \n"
     "    } else {        \n"
     "        fib(n - 1) + fib(n - 2)\n"
     "    }  \n"
     "};\n"
     "print(fib(20)); \n";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_fibonacci", code, &response, true);

  EXPECT_EQ(response.type, INTEGER);
  EXPECT_EQ(response.number, 6765);
}

//---------------------------------------------------------------------------

TEST(rinha_sum0) {

  char *code =
     "let sum = fn (a, b) => { a + b };\n"
     "print(sum(3, 2));\n";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_sum0", code, &response, true);

  EXPECT_EQ(response.type, INTEGER);
  EXPECT_EQ(response.number, 5);
}


TEST(rinha_sum1) {

  char *code =
     "let sum = fn (a, b) => { a + b }\n"
     "print(sum(3, 2) + sum( 1, 2 ));\n";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_sum1", code, &response, true);

  EXPECT_EQ(response.type, INTEGER);
  EXPECT_EQ(response.number, 8);
}

TEST(rinha_sum2) {

  char *code =
     "let sum0 = fn ( arg1, arg2) => \n"
            "{ arg1 + arg2 };\n "
     "let sum1 = fn (var1, var2) => "
            "{ sum0( var1, var2 ) + sum0( var1, var2 ) }; "
     "print(sum1(3, 2) + sum1(6, 8));\n";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_sum2", code, &response, true);

  EXPECT_EQ(response.type, INTEGER);
  EXPECT_EQ(response.number, 38);
}

TEST(rinha_sum3) {

  char *code =
      "let sum = fn (n) => {\n"
      "    n + 1;\n"
      "};\n"
      "\n"
      "let a = 2;\n"
      "let b = 5;\n"
      "let c = fn (v1, v2) => { v1-v2 };\n"
      "print(c(8, 9)); \n"
     // "print (sum(fn (a, b) => { a+b } )); \n"
      "print (sum(58)+c(a,b)); \n";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_sum3", code, &response, true);

  EXPECT_EQ(response.type, INTEGER);
  EXPECT_EQ(response.number, 56);
}

// ------------------------------------------------------------------------

TEST(rinha_calc0) {

  char *code =
     "let a = 9 \n"
     "let b = (a + 2) * 3 / 2\n"
     "print(b * 6);";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_calc0", code, &response, true);

  EXPECT_EQ(response.type, INTEGER);
  EXPECT_EQ(response.number, 96);
}

TEST(rinha_calc1) {

  char *code =
      "let a = \"'/{} string test\" \n"
      "let b = 3 + a\n"
      "print(b)";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_calc1", code, &response, true);

  EXPECT_EQ(response.type, STRING);
  EXPECT_STREQ(response.string, "3'/{} string test");
}

TEST(rinha_cond0) {

  char *code =
    "let teste = fn (arg1, arg2) => { \n"
    "    if ( arg1 > arg2 || 6 > 5 || 7 > 8 || 2 > 1  ) { \n"
    "        print(\"COND1\"); \n"
    "    } else {"
    "        print(\"COND2\"); \n"
    "    } \n"
    "}; \n"
    " teste(0, 3); ";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_cond0", code, &response, true);

  EXPECT_EQ(response.type, STRING);
  EXPECT_STREQ(response.string, "COND1");
}

TEST(rinha_tuples) {

  char *code =
      "let t = ((3*5),\"test\");\n"
      "let a = 88;\n"
      "let b = 99;\n"
      "let t2 = first((second((96, a)), b)); \n"
      "print(second((first((55, 60)), first((second((100, 200)), 90))))\n";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_tuples", code, &response, true);

  EXPECT_EQ(response.type, INTEGER);
  EXPECT_EQ(response.number, 200);
}

TEST(rinha_concat) {

  char *code =
      "let a = 5;\n"
      "let b = 33;\n"
      "let c = a = b = 567;\n"
      "print(\"c = [\"+c+\"]\");\n";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_concat", code, &response, true);

  EXPECT_EQ(response.type, STRING);
  EXPECT_STREQ(response.string, "c = [567]");
}

TEST(rinha_closure0) {

  char *code =
     " let z = fn () => { \n"
     "   let x = 2; \n"
     "   let f = fn (y) => x + y; \n"
     "   f \n"
     "}; \n"
     " let f = z(); "
     " print(f(1)) ";

  rinha_value_t response = {0};
  rinha_clear_stack();

  rinha_script_exec("rinha_closure0", code, &response, true);

  EXPECT_EQ(response.type, INTEGER);
  EXPECT_EQ(response.number, 3);
}

int main() {
  _test_t tests[] = {
     rinha_hello_world_test,

     rinha_fibonacci_test,

     rinha_sum0_test,
     rinha_sum1_test,
     rinha_sum2_test,
     rinha_sum3_test,

     rinha_calc0_test,
     rinha_calc1_test,

     rinha_cond0_test,
     rinha_tuples_test,
     rinha_concat_test,

     rinha_closure0_test,
  };

  run_tests(tests, sizeof(tests) / sizeof(tests[0]));

  return 0;
}

