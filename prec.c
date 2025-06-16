#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct __precn_struct {
    int siz, alloc_size;
    uint32_t *a; // little endian
};
typedef struct __precn_struct *precn_t;

// Allocate a new high-precision integer with given size (number of uint32_t digits)
precn_t precn_new(int size) {
    precn_t n = (precn_t)malloc(sizeof(struct __precn_struct));
    n->siz = 0;
    n->alloc_size = size > 0 ? size : 1;
    n->a = (uint32_t*)calloc(n->alloc_size, sizeof(uint32_t));
    return n;
}

// Free a high-precision integer
void precn_free(precn_t n) {
    if (n) {
        free(n->a);
        free(n);
    }
}

// Set n to zero
void precn_zero(precn_t n) {
    memset(n->a, 0, n->alloc_size * sizeof(uint32_t));
    n->siz = 0;
}

// Copy src to dst
void precn_copy(precn_t dst, const precn_t src) {
    if (dst->alloc_size < src->siz) {
        dst->a = (uint32_t*)realloc(dst->a, src->siz * sizeof(uint32_t));
        dst->alloc_size = src->siz;
    }
    memcpy(dst->a, src->a, src->siz * sizeof(uint32_t));
    dst->siz = src->siz;
}

// Set n to uint32_t value
void precn_set_u32(precn_t n, uint32_t val) {
    precn_zero(n);
    if (val) {
        n->a[0] = val;
        n->siz = 1;
    }
}

// Remove leading zeros
void precn_normalize(precn_t n) {
    while (n->siz > 0 && n->a[n->siz - 1] == 0)
        n->siz--;
}

// Compare a and b: returns -1 if a < b, 0 if a == b, 1 if a > b
int precn_cmp(const precn_t a, const precn_t b) {
    precn_normalize((precn_t)a);
    precn_normalize((precn_t)b);
    if (a->siz < b->siz) return -1;
    if (a->siz > b->siz) return 1;
    for (int i = a->siz - 1; i >= 0; --i) {
        if (a->a[i] < b->a[i]) return -1;
        if (a->a[i] > b->a[i]) return 1;
    }
    return 0;
}

// Addition: res = a + b
void precn_add(precn_t res, const precn_t a, const precn_t b) {
    int max = a->siz > b->siz ? a->siz : b->siz;
    if (res->alloc_size < max + 1) {
        res->a = (uint32_t*)realloc(res->a, (max + 1) * sizeof(uint32_t));
        res->alloc_size = max + 1;
    }
    uint64_t carry = 0;
    int i;
    for (i = 0; i < max; ++i) {
        uint64_t av = i < a->siz ? a->a[i] : 0;
        uint64_t bv = i < b->siz ? b->a[i] : 0;
        uint64_t sum = av + bv + carry;
        res->a[i] = (uint32_t)sum;
        carry = sum >> 32;
    }
    if (carry) {
        res->a[i++] = (uint32_t)carry;
    }
    res->siz = i;
    precn_normalize(res);
}

// Subtraction: res = |a - b|
void precn_sub(precn_t res, const precn_t a, const precn_t b) {
    precn_t big;
    precn_t small;
    int cmp = precn_cmp(a, b);
    if (cmp >= 0) {
        big = a;
        small = b;
    } else {
        big = b;
        small = a;
    }
    int max = big->siz;
    if (res->alloc_size < max) {
        res->a = (uint32_t*)realloc(res->a, max * sizeof(uint32_t));
        res->alloc_size = max;
    }
    int64_t borrow = 0;
    int i;
    for (i = 0; i < max; ++i) {
        int64_t av = i < big->siz ? big->a[i] : 0;
        int64_t bv = i < small->siz ? small->a[i] : 0;
        int64_t diff = av - bv - borrow;
        if (diff < 0) {
            diff += ((int64_t)1 << 32);
            borrow = 1;
        } else {
            borrow = 0;
        }
        res->a[i] = (uint32_t)diff;
    }
    res->siz = max;
    precn_normalize(res);
}

// Multiplication: res = a * b
void precn_mul(precn_t res, const precn_t a, const precn_t b) {
    int n = a->siz, m = b->siz;
    int sz = n + m;
    if (res->alloc_size < sz) {
        res->a = (uint32_t*)realloc(res->a, sz * sizeof(uint32_t));
        res->alloc_size = sz;
    }
    memset(res->a, 0, sz * sizeof(uint32_t));
    for (int i = 0; i < n; ++i) {
        uint64_t carry = 0;
        for (int j = 0; j < m; ++j) {
            uint64_t prod = (uint64_t)a->a[i] * b->a[j] + res->a[i + j] + carry;
            res->a[i + j] = (uint32_t)prod;
            carry = prod >> 32;
        }
        res->a[i + m] = (uint32_t)carry;
    }
    res->siz = sz;
    precn_normalize(res);
}

// Division with remainder: quotient = dividend / divisor, remainder = dividend % divisor
// Returns 0 on success, -1 if divisor is zero
int precn_divmod(precn_t quotient, precn_t remainder, const precn_t dividend, const precn_t divisor) {
    // Check for division by zero
    if (divisor->siz == 0) {
        return -1;
    }
    
    // If dividend < divisor, quotient = 0, remainder = dividend
    if (precn_cmp(dividend, divisor) < 0) {
        precn_zero(quotient);
        precn_copy(remainder, dividend);
        return 0;
    }
    
    // Initialize quotient and remainder
    precn_zero(quotient);
    precn_zero(remainder);
    
    // Ensure quotient has enough space
    if (quotient->alloc_size < dividend->siz) {
        quotient->a = (uint32_t*)realloc(quotient->a, dividend->siz * sizeof(uint32_t));
        quotient->alloc_size = dividend->siz;
    }
    
    // Long division algorithm - process bit by bit
    for (int i = dividend->siz * 32 - 1; i >= 0; i--) {
        // Shift remainder left by 1
        uint32_t carry = 0;
        for (int j = 0; j < remainder->siz; j++) {
            uint64_t temp = ((uint64_t)remainder->a[j] << 1) | carry;
            remainder->a[j] = (uint32_t)temp;
            carry = temp >> 32;
        }
        if (carry) {
            if (remainder->siz < remainder->alloc_size) {
                remainder->a[remainder->siz++] = carry;
            }
        }
        
        // Get bit i from dividend
        int word_idx = i / 32;
        int bit_idx = i % 32;
        if (word_idx < dividend->siz) {
            uint32_t bit = (dividend->a[word_idx] >> bit_idx) & 1;
            if (bit) {
                // Add bit to remainder
                if (remainder->siz == 0) {
                    remainder->a[0] = 1;
                    remainder->siz = 1;
                } else {
                    remainder->a[0] |= 1;
                }
            }
        }
        
        // If remainder >= divisor, subtract and set quotient bit
        if (precn_cmp(remainder, divisor) >= 0) {
            precn_sub(remainder, remainder, divisor);
            
            // Set bit i in quotient
            int q_word = i / 32;
            int q_bit = i % 32;
            if (q_word < quotient->alloc_size) {
                quotient->a[q_word] |= (1U << q_bit);
                if (q_word >= quotient->siz) {
                    quotient->siz = q_word + 1;
                }
            }
        }
    }
    
    precn_normalize(quotient);
    precn_normalize(remainder);
    return 0;
}

// Simple division: quotient = dividend / divisor
int precn_div(precn_t quotient, const precn_t dividend, const precn_t divisor) {
    precn_t temp_remainder = precn_new(dividend->siz);
    int result = precn_divmod(quotient, temp_remainder, dividend, divisor);
    precn_free(temp_remainder);
    return result;
}

// Modulo: remainder = dividend % divisor
int precn_mod(precn_t remainder, const precn_t dividend, const precn_t divisor) {
    precn_t temp_quotient = precn_new(dividend->siz);
    int result = precn_divmod(temp_quotient, remainder, dividend, divisor);
    precn_free(temp_quotient);
    return result;
}

// Left shift by n bits: res = a << n
void precn_shl(precn_t res, const precn_t a, int n) {
    if (n == 0) {
        precn_copy(res, a);
        return;
    }
    
    int word_shift = n / 32;
    int bit_shift = n % 32;
    int new_size = a->siz + word_shift + (bit_shift > 0 ? 1 : 0);
    
    if (res->alloc_size < new_size) {
        res->a = (uint32_t*)realloc(res->a, new_size * sizeof(uint32_t));
        res->alloc_size = new_size;
    }
    
    // Clear result
    memset(res->a, 0, new_size * sizeof(uint32_t));
    
    if (bit_shift == 0) {
        // Simple word shift
        for (int i = 0; i < a->siz; i++) {
            res->a[i + word_shift] = a->a[i];
        }
    } else {
        // Bit shift with carry
        uint32_t carry = 0;
        for (int i = 0; i < a->siz; i++) {
            uint64_t temp = ((uint64_t)a->a[i] << bit_shift) | carry;
            res->a[i + word_shift] = (uint32_t)temp;
            carry = temp >> 32;
        }
        if (carry) {
            res->a[a->siz + word_shift] = carry;
        }
    }
    
    res->siz = new_size;
    precn_normalize(res);
}
/*
// Right shift by n bits: res = a >> n
void precn_shr(precn_t res, const precn_t a, int n) {
    if (n == 0) {
        precn_copy(res, a);
        return;
    }
    
    int word_shift = n / 32;
    int bit_shift = n % 32;
    
    if (word_shift >= a->siz) {
        precn_zero(res);
        return;
    }
    
    int new_size = a->siz - word_shift;
    if (res->alloc_size < new_size) {
        res->a = (uint32_t*)realloc(res->a, new_size * sizeof(uint32_t));
        res->alloc_size = new_size;
    }
    
    if (bit_shift == 0) {
        // Simple word shift
        for (int i = 0; i < new_size; i++) {
            res->a[i] = a->a[i + word_shift];
        }
    } else {
        // Bit shift with borrow
        uint32_t borrow = 0;
        for (int i = new_size - 1; i >= 0; i--) {
            uint64_t temp = ((uint64_t)borrow << 32) | a->a[i + word_shift];
            res->a[i] = (uint32_t)(temp >> bit_shift);
            borrow = a->a[i + word_shift];
        }
    }
    
    res->siz = new_size;
    precn_normalize(res);
}
*/
// Print as hex (for debugging)
void precn_print_hex(const precn_t n) {
    if (n->siz == 0) {
        printf("0x0\n");
        return;
    }
    printf("0x");
    for (int i = n->siz - 1; i >= 0; --i) {
        printf("%08x", n->a[i]);
    }
    printf("\n");
}

// ...add more functions as needed...
