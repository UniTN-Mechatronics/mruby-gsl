#include "mruby.h"
#include "vector.h"
#include "matrix.h"
#include "LU_decomp.h"

void error_handler(const char *reason, const char *file, int line, int gsl_errno) {
  printf("GSL: %s in %s[%d]: %s\n", gsl_strerror(gsl_errno), file, line, reason);
}

void mrb_mruby_gsl_gem_init(mrb_state *mrb) {
  // disable GSL error handler
  gsl_set_error_handler(&error_handler);
  
  mrb_gsl_vector_init(mrb);
  mrb_gsl_matrix_init(mrb);
  mrb_gsl_lu_decomp_init(mrb);
}

void mrb_mruby_gsl_gem_final(mrb_state *mrb) {}
