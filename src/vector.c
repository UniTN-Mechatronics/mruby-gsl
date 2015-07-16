/***************************************************************************/
/*                                                                         */
/* vector.c - Vector class for mruby                                       */
/* Copyright (C) 2015 Paolo Bosetti                                        */
/* paolo[dot]bosetti[at]unitn.it                                           */
/* Department of Industrial Engineering, University of Trento              */
/*                                                                         */
/* This library is free software.  You can redistribute it and/or          */
/* modify it under the terms of the GNU GENERAL PUBLIC LICENSE 2.0.        */
/*                                                                         */
/* This library is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* Artistic License 2.0 for more details.                                  */
/*                                                                         */
/* See the file LICENSE                                                    */
/*                                                                         */
/***************************************************************************/

#include <gsl/gsl_blas.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_sort_vector.h>
#include <gsl/gsl_rng.h>
#include "vector.h"

#pragma mark -
#pragma mark • Utilities

// Garbage collector handler, for play_data struct
// if play_data contains other dynamic data, free it too!
// Check it with GC.start
void vector_destructor(mrb_state *mrb, void *p_) {
  gsl_vector *v = (gsl_vector *)p_;
  gsl_vector_free(v);
};

// Creating data type and reference for GC, in a const struct
const struct mrb_data_type vector_data_type = {"vector_data",
                                               vector_destructor};

// Utility function for getting the struct out of the wrapping IV @data
void mrb_vector_get_data(mrb_state *mrb, mrb_value self, gsl_vector **data) {
  mrb_value data_value;
  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // Loading data from data_value into p_data:
  Data_Get_Struct(mrb, data_value, &vector_data_type, *data);
  if (!*data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not access @data");
}

#pragma mark -
#pragma mark • Init and accessing

// Data Initializer C function (not exposed!)
static void mrb_vector_init(mrb_state *mrb, mrb_value self, mrb_int n) {
  mrb_value data_value; // this IV holds the data
  gsl_vector *p_data;   // pointer to the C struct

  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // if @data already exists, free its content:
  if (!mrb_nil_p(data_value)) {
    Data_Get_Struct(mrb, data_value, &vector_data_type, p_data);
    free(p_data);
  }
  // Allocate and zero-out the data struct:
  p_data = gsl_vector_calloc(n);
  if (!p_data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not allocate @data");

  // Wrap struct into @data:
  mrb_iv_set(
      mrb, self, mrb_intern_lit(mrb, "@data"), // set @data
      mrb_obj_value(                           // with value hold in struct
          Data_Wrap_Struct(mrb, mrb->object_class, &vector_data_type, p_data)));
}

static mrb_value mrb_vector_initialize(mrb_state *mrb, mrb_value self) {
  mrb_int n;
  mrb_get_args(mrb, "i", &n);

  // Call strcut initializer:
  mrb_vector_init(mrb, self, n);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@length"), mrb_fixnum_value(n));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@format"),
             mrb_str_new_cstr(mrb, "%10.3f"));
  return mrb_nil_value();
}

static mrb_value mrb_vector_rnd_fill(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec = NULL;
  const gsl_rng_type *T;
  gsl_rng *r;
  mrb_int h;

  mrb_vector_get_data(mrb, self, &p_vec);

  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc(T);

  for (h = 0; h < p_vec->size; h++) {
    gsl_vector_set(p_vec, h, gsl_rng_uniform(r));
  }
  return self;
}


static mrb_value mrb_vector_dup(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_vector *p_vec = NULL, *p_vec_other = NULL;
  mrb_value args[1];

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  args[0] = mrb_fixnum_value(p_vec->size);
  other = mrb_obj_new(mrb, mrb_class_get(mrb, "Vector"), 1, args);
  mrb_vector_get_data(mrb, other, &p_vec_other);
  gsl_vector_memcpy(p_vec_other, p_vec);
  return other;
}

static mrb_value mrb_vector_all(mrb_state *mrb, mrb_value self) {
  mrb_float v;
  gsl_vector *p_vec = NULL;

  mrb_get_args(mrb, "f", &v);
  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  gsl_vector_set_all(p_vec, v);
  return self;
}

static mrb_value mrb_vector_zero(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec = NULL;

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  gsl_vector_set_zero(p_vec);
  return self;
}

static mrb_value mrb_vector_basis(mrb_state *mrb, mrb_value self) {
  mrb_int i;
  gsl_vector *p_vec = NULL;

  mrb_get_args(mrb, "i", &i);
  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  gsl_vector_set_basis(p_vec, i);
  return self;
}


#pragma mark -
#pragma mark • Tests

static mrb_value mrb_vector_equal(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_vector *p_vec, *p_vec_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  mrb_vector_get_data(mrb, other, &p_vec_other);
  if (1 == gsl_vector_equal(p_vec, p_vec_other))
    return mrb_true_value();
  else
    return mrb_false_value();
}


#pragma mark -
#pragma mark • Accessors

static mrb_value mrb_vector_get_i(mrb_state *mrb, mrb_value self) {
  mrb_int i = 0;
  gsl_vector *p_vec = NULL;

  mrb_get_args(mrb, "i", &i);
  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  if (i >= p_vec->size) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Vector index out of range!");
  }
  return mrb_float_value(mrb, gsl_vector_get(p_vec, (size_t)i));
}

static mrb_value mrb_vector_set_i(mrb_state *mrb, mrb_value self) {
  mrb_int i = 0;
  mrb_float f;
  gsl_vector *p_vec = NULL;

  mrb_get_args(mrb, "if", &i, &f);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  if (i >= p_vec->size) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Vector index out of range!");
  }
  gsl_vector_set(p_vec, (size_t)i, (double)f);
  return mrb_float_value(mrb, f);
}

static mrb_value mrb_vector_to_a(mrb_state *mrb, mrb_value self) {
  int i;
  mrb_value ary = mrb_nil_value();
  gsl_vector *p_vec = NULL;
  mrb_float e;
  mrb_vector_get_data(mrb, self, &p_vec);
  ary = mrb_ary_new_capa(mrb, p_vec->size);
  for (i = 0; i < p_vec->size; i++) {
    e = *(p_vec->data + i * p_vec->stride);
    mrb_ary_set(mrb, ary, i, mrb_float_value(mrb, e));
  }
  return ary;
}

#pragma mark -
#pragma mark • Properties

static mrb_value mrb_vector_max(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec = NULL;
  mrb_vector_get_data(mrb, self, &p_vec);
  return mrb_float_value(mrb, gsl_vector_max(p_vec));
}

static mrb_value mrb_vector_min(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec = NULL;
  mrb_vector_get_data(mrb, self, &p_vec);
  return mrb_float_value(mrb, gsl_vector_min(p_vec));
}

static mrb_value mrb_vector_max_index(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec = NULL;
  mrb_vector_get_data(mrb, self, &p_vec);
  return mrb_fixnum_value(gsl_vector_max_index(p_vec));
}

static mrb_value mrb_vector_min_index(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec = NULL;
  mrb_vector_get_data(mrb, self, &p_vec);
  return mrb_fixnum_value(gsl_vector_min_index(p_vec));
}

#pragma mark -
#pragma mark • Operations

static mrb_value mrb_vector_add(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_vector *p_vec, *p_vec_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);

  if (mrb_obj_is_kind_of(mrb, other, mrb_class_get(mrb, "Vector"))) {
    mrb_vector_get_data(mrb, other, &p_vec_other);
    if (p_vec->size != p_vec_other->size) {
      mrb_raise(mrb, E_VECTOR_ERROR, "Vector indexes don't match!");
    }
    gsl_vector_add(p_vec, p_vec_other);
  } else if (mrb_obj_is_kind_of(mrb, other, mrb_class_get(mrb, "Numeric"))) {
    gsl_vector_add_constant(p_vec, mrb_to_flo(mrb, other));
  }
  return self;
}

static mrb_value mrb_vector_sub(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_vector *p_vec, *p_vec_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  mrb_vector_get_data(mrb, other, &p_vec_other);
  if (p_vec->size != p_vec_other->size) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Vector dimensions don't match!");
  }
  gsl_vector_sub(p_vec, p_vec_other);
  return self;
}

static mrb_value mrb_vector_mul(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_vector *p_vec, *p_vec_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  if (mrb_obj_is_kind_of(mrb, other, mrb_class_get(mrb, "Vector"))) {
    mrb_vector_get_data(mrb, other, &p_vec_other);
    if (p_vec->size != p_vec_other->size) {
      mrb_raise(mrb, E_VECTOR_ERROR, "Vector indexes don't match!");
    }
    gsl_vector_mul(p_vec, p_vec_other);
  } else if (mrb_obj_is_kind_of(mrb, other, mrb_class_get(mrb, "Numeric"))) {
    gsl_vector_scale(p_vec, mrb_to_flo(mrb, other));
  }
  return self;
}

static mrb_value mrb_vector_div(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_vector *p_vec, *p_vec_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  mrb_vector_get_data(mrb, other, &p_vec_other);
  if (p_vec->size != p_vec_other->size) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Vector indexes don't match!");
  }
  gsl_vector_div(p_vec, p_vec_other);
  return self;
}

static mrb_value mrb_vector_prod(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_vector *p_vec, *p_vec_other;
  mrb_float res;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  mrb_vector_get_data(mrb, other, &p_vec_other);
  if (!mrb_obj_is_kind_of(mrb, other, mrb_class_get(mrb, "Vector"))) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Need a Vector!");
  }
  if (p_vec->size != p_vec_other->size) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Vector indexes don't match!");
  }
  if (gsl_blas_ddot(p_vec, p_vec_other, &res)) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Cannot multiply");
  }
  return mrb_float_value(mrb, res);
}

static mrb_value mrb_vector_norm(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  return mrb_float_value(mrb, gsl_blas_dnrm2(p_vec));
}

static mrb_value mrb_vector_sum(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  return mrb_float_value(mrb, gsl_blas_dasum(p_vec));
}

static mrb_value mrb_vector_swap(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;
  mrb_int i, j;
  mrb_get_args(mrb, "ii", &i, &j);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  if (gsl_vector_swap_elements(p_vec, i, j)) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Cannot swap");
  }
  return self;
}

static mrb_value mrb_vector_reverse(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  if (gsl_vector_reverse(p_vec)) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Cannot reverse");
  }
  return self;
}


#pragma mark -
#pragma mark • Statistics

static mrb_value mrb_vector_mean(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;
  mrb_float result;
  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  result = gsl_stats_mean(p_vec->data, p_vec->stride, p_vec->size);
  return mrb_float_value(mrb, result);
}

static mrb_value mrb_vector_variance(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;
  mrb_float result;
  mrb_float m;
  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  if (mrb_get_args(mrb, "|f", &m) == 1) {
    result = gsl_stats_variance_m(p_vec->data, p_vec->stride, p_vec->size, m);
  } else {
    result = gsl_stats_variance(p_vec->data, p_vec->stride, p_vec->size);
  }
  return mrb_float_value(mrb, result);
}

static mrb_value mrb_vector_sd(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;
  mrb_float result;
  mrb_float m;
  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  if (mrb_get_args(mrb, "|f", &m) == 1) {
    result = gsl_stats_sd_m(p_vec->data, p_vec->stride, p_vec->size, m);
  } else {
    result = gsl_stats_sd(p_vec->data, p_vec->stride, p_vec->size);
  }
  return mrb_float_value(mrb, result);
}

static mrb_value mrb_vector_absdev(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;
  mrb_float result;
  mrb_float m;
  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  if (mrb_get_args(mrb, "|f", &m) == 1) {
    result = gsl_stats_absdev_m(p_vec->data, p_vec->stride, p_vec->size, m);
  } else {
    result = gsl_stats_absdev(p_vec->data, p_vec->stride, p_vec->size);
  }
  return mrb_float_value(mrb, result);
}

static mrb_value mrb_vector_quantile(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec = NULL, *p_sort_vec = NULL;
  mrb_float result;
  mrb_float f;
  mrb_int n;
  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  n = mrb_get_args(mrb, "|f", &f);
  if (f < 0 || f > 1) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Quantile must be in [0,1]");
  }
  p_sort_vec = gsl_vector_calloc(p_vec->size);
  if (gsl_vector_memcpy(p_sort_vec, p_vec)) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Cannot copy vector");
  }
  gsl_sort_vector(p_sort_vec);
  if (n == 1) {
    result = gsl_stats_quantile_from_sorted_data(
        p_sort_vec->data, p_sort_vec->stride, p_sort_vec->size, f);
  } else {
    result = gsl_stats_quantile_from_sorted_data(
        p_sort_vec->data, p_sort_vec->stride, p_sort_vec->size, 0.5);
  }
  return mrb_float_value(mrb, result);
}


#pragma mark -
#pragma mark • Gem setup

void mrb_gsl_vector_init(mrb_state *mrb) {
  struct RClass *gsl;

  mrb_load_string(mrb, "class VectorError < Exception; end");

  gsl = mrb_define_class(mrb, "Vector", mrb->object_class);
  mrb_define_method(mrb, gsl, "all", mrb_vector_all, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "zero", mrb_vector_zero, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "basis", mrb_vector_basis, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, gsl, "initialize", mrb_vector_initialize,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "rnd_fill", mrb_vector_rnd_fill,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "dup", mrb_vector_dup, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "===", mrb_vector_equal, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "[]", mrb_vector_get_i, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "[]=", mrb_vector_set_i, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, gsl, "to_a", mrb_vector_to_a, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "max", mrb_vector_max, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "min", mrb_vector_min, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "max_index", mrb_vector_max_index,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "min_index", mrb_vector_min_index,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "add!", mrb_vector_add, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "sub!", mrb_vector_sub, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "mul!", mrb_vector_mul, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "div!", mrb_vector_div, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "^", mrb_vector_prod, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "norm", mrb_vector_norm, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "sum", mrb_vector_sum, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "swap!", mrb_vector_swap, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, gsl, "reverse!", mrb_vector_reverse, MRB_ARGS_NONE());

  mrb_define_method(mrb, gsl, "mean", mrb_vector_mean, MRB_ARGS_OPT(1));
  mrb_define_method(mrb, gsl, "variance", mrb_vector_variance, MRB_ARGS_OPT(1));
  mrb_define_method(mrb, gsl, "sd", mrb_vector_sd, MRB_ARGS_OPT(1));
  mrb_define_method(mrb, gsl, "absdev", mrb_vector_absdev, MRB_ARGS_OPT(1));
  mrb_define_method(mrb, gsl, "quantile", mrb_vector_quantile, MRB_ARGS_OPT(1));
}
