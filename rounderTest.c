#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

size_t rounder(size_t n) {

  size_t leading = __builtin_clzl(n);
  size_t following = __builtin_ctzl(n);

  if ((leading + following + 1) == 8*sizeof(size_t)) {
    return following;
  } else if ((leading + following + 1) < 8*sizeof(size_t)) {
    return 8*sizeof(size_t) - leading;
  }
}
    

int main(){

  size_t n = 172;
  
  size_t result = rounder(n);
  printf("%d\n", result);

  
}
