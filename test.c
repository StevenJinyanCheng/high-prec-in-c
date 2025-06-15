#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

// Include the prec.c file directly for simplicity
// In a real project, you would separate declaration and implementation
#include "prec.c"

void test_basic_operations() {
    printf("Testing basic operations...\n");
    
    // Create some numbers
    precn_t a = precn_new(1);
    precn_t b = precn_new(1);
    precn_t result = precn_new(2);
    
    // Test setting values
    precn_set_u32(a, 42);
    precn_set_u32(b, 17);
    
    // Test printing
    printf("a = "); precn_print_hex(a);
    printf("b = "); precn_print_hex(b);
    
    // Test addition
    precn_add(result, a, b);
    printf("a + b = "); precn_print_hex(result);
    assert(result->siz == 1 && result->a[0] == 59);
    
    // Test subtraction
    precn_sub(result, a, b);
    printf("|a - b| = "); precn_print_hex(result);
    assert(result->siz == 1 && result->a[0] == 25);
    
    // Test multiplication
    precn_mul(result, a, b);
    printf("a * b = "); precn_print_hex(result);
    assert(result->siz == 1 && result->a[0] == 714);
    
    // Free memory
    precn_free(a);
    precn_free(b);
    precn_free(result);
    
    printf("Basic tests passed!\n\n");
}

void test_larger_numbers() {
    printf("Testing operations with larger numbers...\n");
    
    // Create numbers
    precn_t a = precn_new(3);
    precn_t b = precn_new(2);
    precn_t result = precn_new(5);
    
    // Set a to 0xFFFFFFFF (max uint32)
    precn_set_u32(a, 0xFFFFFFFF);
    
    // Set b to 0x1
    precn_set_u32(b, 0x1);
    
    printf("a = "); precn_print_hex(a);
    printf("b = "); precn_print_hex(b);
    
    // Test addition (should be 0x100000000)
    precn_add(result, a, b);
    printf("a + b = "); precn_print_hex(result);
    assert(result->siz == 2 && result->a[0] == 0 && result->a[1] == 1);
    
    // Test subtraction
    precn_sub(result, a, b);
    printf("|a - b| = "); precn_print_hex(result);
    assert(result->siz == 1 && result->a[0] == 0xFFFFFFFE);
    
    // Set a to a large 2-word value
    precn_zero(a);
    a->a[0] = 0xFFFFFFFF;
    a->a[1] = 0xAAAAAAAA;
    a->siz = 2;
    
    // Set b to another value
    precn_zero(b);
    b->a[0] = 0x55555555;
    b->a[1] = 0x11111111;
    b->siz = 2;
    
    printf("a = "); precn_print_hex(a);
    printf("b = "); precn_print_hex(b);
    
    // Test addition
    precn_add(result, a, b);
    printf("a + b = "); precn_print_hex(result);
    assert(result->siz == 2 && result->a[0] == 0x55555554 && result->a[1] == 0xBBBBBBBC);
    
    // Test subtraction with large numbers
    precn_sub(result, a, b);
    printf("|a - b| = "); precn_print_hex(result);
    assert(result->siz == 2 && result->a[0] == 0xAAAAAAAA && result->a[1] == 0x99999999);
    
    // Test multiplication
    precn_mul(result, a, b);
    printf("a * b = "); precn_print_hex(result);
    
    // Free memory
    precn_free(a);
    precn_free(b);
    precn_free(result);
    
    printf("Large number tests passed!\n\n");
}

void test_subtraction_order() {
    printf("Testing subtraction order (|a-b| = |b-a|)...\n");
    
    precn_t a = precn_new(2);
    precn_t b = precn_new(2);
    precn_t result1 = precn_new(2);
    precn_t result2 = precn_new(2);
    
    // Set a = 1000, b = 1
    precn_set_u32(a, 1000);
    precn_set_u32(b, 1);
    
    printf("a = "); precn_print_hex(a);
    printf("b = "); precn_print_hex(b);
    
    // Compute |a-b| and |b-a|
    precn_sub(result1, a, b);
    precn_sub(result2, b, a);
    
    printf("|a - b| = "); precn_print_hex(result1);
    printf("|b - a| = "); precn_print_hex(result2);
    
    // Results should be identical
    assert(precn_cmp(result1, result2) == 0);
    
    precn_free(a);
    precn_free(b);
    precn_free(result1);
    precn_free(result2);
    
    printf("Subtraction order test passed!\n\n");
}

void test_subtraction_with_borrow() {
    printf("Testing subtraction with borrow propagation...\n");
    
    precn_t a = precn_new(3);
    precn_t b = precn_new(3);
    precn_t result = precn_new(3);
    
    // Set a = 0x0000000100000000 (first word is 0, second word is 1)
    precn_zero(a);
    a->a[0] = 0x00000000;
    a->a[1] = 0x00000001;
    a->siz = 2;
    
    // Set b = 0x0000000000000001 (just 1)
    precn_set_u32(b, 0x00000001);
    
    printf("a = "); precn_print_hex(a);
    printf("b = "); precn_print_hex(b);
    
    // This should require a borrow from the second word to subtract 1 from 0
    precn_sub(result, a, b);
    printf("|a - b| = "); precn_print_hex(result);
    
    // Result should be 0xFFFFFFFF
    assert(result->siz == 1);
    assert(result->a[0] == 0xFFFFFFFF);
    
    // Set up a more complex case with multiple borrows
    precn_zero(a);
    a->a[0] = 0x00000000;
    a->a[1] = 0x00000000;
    a->a[2] = 0x00000001;
    a->siz = 3;
    
    precn_zero(b);
    b->a[0] = 0x00000001;
    b->siz = 1;
    
    printf("a = "); precn_print_hex(a);
    printf("b = "); precn_print_hex(b);
    
    // This should require borrows to cascade through all words
    precn_sub(result, a, b);
    printf("|a - b| = "); precn_print_hex(result);
    
    // Result should be 0x0000000100000000FFFFFFFF
    assert(result->siz == 2);
    assert(result->a[0] == 0xFFFFFFFF);
    assert(result->a[1] == 0xFFFFFFFF);
    
    precn_free(a);
    precn_free(b);
    precn_free(result);
    
    printf("Subtraction with borrow tests passed!\n\n");
}

void test_division() {
    printf("Testing division operations...\n");
    
    precn_t dividend = precn_new(5);
    precn_t divisor = precn_new(3);
    precn_t quotient = precn_new(5);
    precn_t remainder = precn_new(5);
    precn_t temp = precn_new(5);
    
    // Test simple division: 100 / 7
    precn_set_u32(dividend, 100);
    precn_set_u32(divisor, 7);
    
    printf("dividend = "); precn_print_hex(dividend);
    printf("divisor = "); precn_print_hex(divisor);
    
    int result = precn_divmod(quotient, remainder, dividend, divisor);
    assert(result == 0);
    
    printf("quotient = "); precn_print_hex(quotient);
    printf("remainder = "); precn_print_hex(remainder);
    
    // 100 / 7 = 14 remainder 2
    assert(quotient->siz == 1 && quotient->a[0] == 14);
    assert(remainder->siz == 1 && remainder->a[0] == 2);
    
    // Test larger division: 0xFFFFFFFF / 0x10000
    precn_set_u32(dividend, 0xFFFFFFFF);
    precn_set_u32(divisor, 0x10000);
    
    printf("\nLarge division test 1:\n");
    printf("dividend = "); precn_print_hex(dividend);
    printf("divisor = "); precn_print_hex(divisor);
    
    result = precn_divmod(quotient, remainder, dividend, divisor);
    assert(result == 0);
    
    printf("quotient = "); precn_print_hex(quotient);
    printf("remainder = "); precn_print_hex(remainder);
    
    // Verify: quotient * divisor + remainder = dividend
    precn_mul(temp, quotient, divisor);
    precn_add(temp, temp, remainder);
    printf("verification (q*d+r) = "); precn_print_hex(temp);
    assert(precn_cmp(temp, dividend) == 0);
    
    // Test very large division: 0x123456789ABCDEF0 / 0x1000
    precn_zero(dividend);
    dividend->a[0] = 0x9ABCDEF0;
    dividend->a[1] = 0x12345678;
    dividend->siz = 2;
    
    precn_set_u32(divisor, 0x1000);
    
    printf("\nLarge division test 2:\n");
    printf("dividend = "); precn_print_hex(dividend);
    printf("divisor = "); precn_print_hex(divisor);
    
    result = precn_divmod(quotient, remainder, dividend, divisor);
    assert(result == 0);
    
    printf("quotient = "); precn_print_hex(quotient);
    printf("remainder = "); precn_print_hex(remainder);
    
    // Verify: quotient * divisor + remainder = dividend
    precn_mul(temp, quotient, divisor);
    precn_add(temp, temp, remainder);
    printf("verification (q*d+r) = "); precn_print_hex(temp);
    assert(precn_cmp(temp, dividend) == 0);
    
    // Test division by larger divisor
    precn_zero(dividend);
    dividend->a[0] = 0xFFFFFFFF;
    dividend->a[1] = 0xFFFFFFFF;
    dividend->a[2] = 0x12345678;
    dividend->siz = 3;
    
    precn_zero(divisor);
    divisor->a[0] = 0x87654321;
    divisor->a[1] = 0x11111111;
    divisor->siz = 2;
    
    printf("\nLarge division test 3:\n");
    printf("dividend = "); precn_print_hex(dividend);
    printf("divisor = "); precn_print_hex(divisor);
    
    result = precn_divmod(quotient, remainder, dividend, divisor);
    assert(result == 0);
    
    printf("quotient = "); precn_print_hex(quotient);
    printf("remainder = "); precn_print_hex(remainder);
    
    // Verify: quotient * divisor + remainder = dividend
    precn_mul(temp, quotient, divisor);
    precn_add(temp, temp, remainder);
    printf("verification (q*d+r) = "); precn_print_hex(temp);
    assert(precn_cmp(temp, dividend) == 0);
    
    // Test division by zero
    precn_set_u32(divisor, 0);
    result = precn_divmod(quotient, remainder, dividend, divisor);
    assert(result == -1);
    
    // Test when dividend < divisor
    precn_set_u32(dividend, 5);
    precn_set_u32(divisor, 10);
    
    result = precn_divmod(quotient, remainder, dividend, divisor);
    assert(result == 0);
    assert(quotient->siz == 0); // quotient = 0
    assert(remainder->siz == 1 && remainder->a[0] == 5);
    
    precn_free(dividend);
    precn_free(divisor);
    precn_free(quotient);
    precn_free(remainder);
    precn_free(temp);
    
    printf("Division tests passed!\n\n");
}

void test_modular_operations() {
    printf("Testing modular operations...\n");
    
    precn_t a = precn_new(3);
    precn_t m = precn_new(2);
    precn_t result = precn_new(3);
    precn_t quotient = precn_new(3);
    precn_t temp = precn_new(5);
    
    // Test simple modulo: 100 % 7 = 2
    precn_set_u32(a, 100);
    precn_set_u32(m, 7);
    
    printf("a = "); precn_print_hex(a);
    printf("m = "); precn_print_hex(m);
    
    int res = precn_mod(result, a, m);
    assert(res == 0);
    
    printf("a %% m = "); precn_print_hex(result);
    
    // Verify: a - (a / m) * m = a % m
    precn_div(quotient, a, m);
    precn_mul(temp, quotient, m);
    precn_sub(temp, a, temp);
    assert(precn_cmp(temp, result) == 0);
    
    // Test larger modulo: 0xFFFFFFFF % 0x10000 = 0xFFFF
    precn_set_u32(a, 0xFFFFFFFF);
    precn_set_u32(m, 0x10000);
    
    res = precn_mod(result, a, m);
    assert(res == 0);
    
    printf("Large mod test: 0xFFFFFFFF %% 0x10000 = "); precn_print_hex(result);
    
    // Verify: a - (a / m) * m = a % m
    precn_div(quotient, a, m);
    precn_mul(temp, quotient, m);
    precn_sub(temp, a, temp);
    assert(precn_cmp(temp, result) == 0);
    
    // Test modulo by zero (should fail)
    precn_set_u32(m, 0);
    res = precn_mod(result, a, m);
    assert(res == -1);
    
    // Test when dividend < divisor
    precn_set_u32(a, 5);
    precn_set_u32(m, 10);
    
    res = precn_mod(result, a, m);
    assert(res == 0);
    assert(result->siz == 1 && result->a[0] == 5);
    
    precn_free(a);
    precn_free(m);
    precn_free(result);
    precn_free(quotient);
    precn_free(temp);
    
    printf("Modular operations tests passed!\n\n");
}

void test_random_modular() {
    printf("Testing random modular operations...\n");
    
    srand(54321); // Different seed from division tests
    
    for (int test = 0; test < 10; test++) {
        printf("Random mod test %d:\n", test + 1);
        
        // Generate random sizes
        int dividend_size = 500 + rand() % 4501; // 500-5000 words
        int divisor_size = 1 + rand() % (dividend_size / 2 + 1); // smaller than dividend
        
        precn_t dividend = precn_new(dividend_size + 1);
        precn_t divisor = precn_new(divisor_size + 1);
        precn_t mod_result = precn_new(divisor_size + 1);
        precn_t quotient = precn_new(dividend_size + 1);
        precn_t verification = precn_new(dividend_size + 2);
        
        // Fill with random data
        for (int i = 0; i < dividend_size; i++) {
            dividend->a[i] = ((uint32_t)rand() << 16) | rand();
        }
        dividend->siz = dividend_size;
        precn_normalize(dividend);
        
        for (int i = 0; i < divisor_size; i++) {
            divisor->a[i] = ((uint32_t)rand() << 16) | rand();
        }
        divisor->siz = divisor_size;
        precn_normalize(divisor);
        
        // Ensure divisor is not zero
        if (divisor->siz == 0) {
            precn_set_u32(divisor, 1 + rand() % 1000);
        }
        
        printf("Dividend size: %d words, Divisor size: %d words\n", dividend->siz, divisor->siz);
        
        // Perform modulo
        int result = precn_mod(mod_result, dividend, divisor);
        assert(result == 0);
        
        // Verify: dividend - (dividend / divisor) * divisor = dividend % divisor
        precn_div(quotient, dividend, divisor);
        precn_mul(verification, quotient, divisor);
        precn_sub(verification, dividend, verification);
        
        if (precn_cmp(verification, mod_result) != 0) {
            printf("MODULO VERIFICATION FAILED!\n");
            assert(0);
        }
        
        // Verify remainder < divisor
        assert(precn_cmp(mod_result, divisor) < 0);
        
        printf("✓ Random mod test %d passed\n", test + 1);
        
        precn_free(dividend);
        precn_free(divisor);
        precn_free(mod_result);
        precn_free(quotient);
        precn_free(verification);
    }
    
    printf("All random modular tests passed!\n\n");
}

void test_random_division() {
    printf("Testing random division with large numbers...\n");
    
    srand(12345); // Fixed seed for reproducible tests
    
    for (int test = 0; test < 10; test++) {
        printf("Random test %d:\n", test + 1);
        
        // Generate random dividend size (50-5000 words for better performance)
        int dividend_size = 50 + rand() % 4501;
        // Generate random divisor size (1-5000 words, but smaller than dividend)
        int divisor_size = 1 + rand() % (dividend_size / 2 + 1);
        
        precn_t dividend = precn_new(dividend_size + 1);
        precn_t divisor = precn_new(divisor_size + 1);
        precn_t quotient = precn_new(dividend_size + 1);
        precn_t remainder = precn_new(dividend_size + 1);
        precn_t verification = precn_new(dividend_size + divisor_size + 2);
        
        // Fill dividend with random data
        for (int i = 0; i < dividend_size; i++) {
            dividend->a[i] = ((uint32_t)rand() << 16) | rand();
        }
        dividend->siz = dividend_size;
        precn_normalize(dividend);
        
        // Fill divisor with random data (ensure it's not zero)
        for (int i = 0; i < divisor_size; i++) {
            divisor->a[i] = ((uint32_t)rand() << 16) | rand();
        }
        divisor->siz = divisor_size;
        precn_normalize(divisor);
        
        // Ensure divisor is not zero
        if (divisor->siz == 0) {
            precn_set_u32(divisor, 1 + rand() % 1000);
        }
        
        // Ensure dividend >= divisor for meaningful test
        if (precn_cmp(dividend, divisor) < 0) {
            precn_t temp = precn_new(dividend_size + 1);
            precn_copy(temp, dividend);
            precn_copy(dividend, divisor);
            precn_copy(divisor, temp);
            precn_free(temp);
        }
        
        printf("Dividend size: %d words, Divisor size: %d words\n", dividend->siz, divisor->siz);
        
        // Perform division
        int result = precn_divmod(quotient, remainder, dividend, divisor);
        assert(result == 0);
        
        // Verify: quotient * divisor + remainder = dividend
        precn_mul(verification, quotient, divisor);
        precn_add(verification, verification, remainder);
        
        if (precn_cmp(verification, dividend) != 0) {
            printf("VERIFICATION FAILED!\n");
            printf("Original dividend first 4 words: %08x %08x %08x %08x\n", 
                   dividend->a[0], dividend->a[1], dividend->a[2], dividend->a[3]);
            printf("Verification first 4 words: %08x %08x %08x %08x\n",
                   verification->a[0], verification->a[1], verification->a[2], verification->a[3]);
            assert(0);
        }
        
        // Verify remainder < divisor
        assert(precn_cmp(remainder, divisor) < 0);
        
        printf("✓ Random test %d passed\n", test + 1);
        
        precn_free(dividend);
        precn_free(divisor);
        precn_free(quotient);
        precn_free(remainder);
        precn_free(verification);
    }
    
    printf("All random division tests passed!\n\n");
}

int main() {
    printf("Testing precn high-precision library\n");
    printf("====================================\n\n");
    
    test_basic_operations();
    test_larger_numbers();
    test_subtraction_order();
    test_subtraction_with_borrow();
    test_division();
    test_modular_operations();
    test_random_modular();
    test_random_division();
    
    printf("All tests passed successfully!\n");
    return 0;
}
