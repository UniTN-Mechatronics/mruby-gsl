#include "mruby.h"
#include "vector.h"
#include "matrix.h"
#include "LU_decomp.h"

void error_handler(const char *reason, const char *file, int line,
                   int gsl_errno) {
  printf("GSL: %s in %s[%d]: %s\n", gsl_strerror(gsl_errno), file, line,
         reason);
}

static mrb_value mrb_gsl_info_on(mrb_state *mrb, mrb_value self) {
  gsl_set_error_handler(&error_handler);
  return mrb_true_value();
}

static mrb_value mrb_gsl_info_off(mrb_state *mrb, mrb_value self) {
  gsl_set_error_handler_off();
  return mrb_false_value();
}

void mrb_mruby_gsl_gem_init(mrb_state *mrb) {
// disable GSL error handler
#ifdef GSL_ERROR_MSG_PRINTOUT
  gsl_set_error_handler(&error_handler);
#else
  gsl_set_error_handler_off();
#endif
  mrb_define_method(mrb, mrb->kernel_module, "gsl_info_on", mrb_gsl_info_on,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, mrb->kernel_module, "gsl_info_off", mrb_gsl_info_off,
                    MRB_ARGS_NONE());

  mrb_gsl_vector_init(mrb);
  mrb_gsl_matrix_init(mrb);
  mrb_gsl_lu_decomp_init(mrb);
}

void mrb_mruby_gsl_gem_final(mrb_state *mrb) {}
