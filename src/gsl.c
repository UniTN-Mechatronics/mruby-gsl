#include "mruby.h"
#include "vector.h"
#include "matrix.h"

void mrb_mruby_gsl_gem_init(mrb_state *mrb) {
  mrb_gsl_vector_init(mrb);
  mrb_gsl_matrix_init(mrb);
}

void mrb_mruby_gsl_gem_final(mrb_state *mrb) {}
