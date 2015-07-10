/***************************************************************************/
/*                                                                         */
/* play.c - mruby testing                                                  */
/* Copyright (C) 2015 Paolo Bosetti and Matteo Ragni,                      */
/* paolo[dot]bosetti[at]unitn.it and matteo[dot]ragni[at]unitn.it          */
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

#include "matrix.h"
#include "vector.h"

// Garbage collector handler, for play_data struct
// if play_data contains other dynamic data, free it too!
// Check it with GC.start
void matrix_destructor(mrb_state *mrb, void *p_) {
  gsl_matrix *v = (gsl_matrix *)p_;
  gsl_matrix_free(v);
  // or simply:
  // mrb_free(mrb, pd);
};

// Creating data type and reference for GC, in a const struct
const struct mrb_data_type matrix_data_type = {"matrix_data",
                                               matrix_destructor};

// Utility function for getting the struct out of the wrapping IV @data
void mrb_matrix_get_data(mrb_state *mrb, mrb_value self, gsl_matrix **data) {
  mrb_value data_value;
  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // Loading data from data_value into p_data:
  Data_Get_Struct(mrb, data_value, &matrix_data_type, *data);
  if (!*data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not access @data");
}

// Data Initializer C function (not exposed!)
static void mrb_matrix_init(mrb_state *mrb, mrb_value self, mrb_int n,
                            mrb_int m) {
  mrb_value data_value; // this IV holds the data
  gsl_matrix *p_data;   // pointer to the C struct

  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // if @data already exists, free its content:
  if (!mrb_nil_p(data_value)) {
    Data_Get_Struct(mrb, data_value, &matrix_data_type, p_data);
    free(p_data);
  }
  // Allocate and zero-out the data struct:
  p_data = gsl_matrix_calloc(n, m);
  if (!p_data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not allocate @data");

  // Wrap struct into @data:
  mrb_iv_set(
      mrb, self, mrb_intern_lit(mrb, "@data"), // set @data
      mrb_obj_value(                           // with value hold in struct
          Data_Wrap_Struct(mrb, mrb->object_class, &matrix_data_type, p_data)));
}

static mrb_value mrb_matrix_initialize(mrb_state *mrb, mrb_value self) {
  mrb_int n, m;
  mrb_get_args(mrb, "ii", &n, &m);
  
  // Call strcut initializer:
  mrb_matrix_init(mrb, self, n, m);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@rows"), mrb_fixnum_value(n));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@cols"), mrb_fixnum_value(m));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@format"), mrb_str_new_cstr(mrb, "%10.3f"));
  return mrb_nil_value();
}

static mrb_value mrb_matrix_all(mrb_state *mrb, mrb_value self) {
  mrb_float v;
  gsl_matrix *p_mat = NULL;

  mrb_get_args(mrb, "f", &v);
  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  gsl_matrix_set_all(p_mat, v);
  return self;
}

static mrb_value mrb_matrix_zero(mrb_state *mrb, mrb_value self) {
  gsl_matrix *p_mat = NULL;

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  gsl_matrix_set_zero(p_mat);
  return self;
}

static mrb_value mrb_matrix_identity(mrb_state *mrb, mrb_value self) {
  gsl_matrix *p_mat = NULL;

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  gsl_matrix_set_identity(p_mat);
  return self;
}

static mrb_value mrb_matrix_equal(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_matrix *p_mat, *p_mat_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  mrb_matrix_get_data(mrb, other, &p_mat_other);
  if (1 == gsl_matrix_equal(p_mat, p_mat_other))
    return mrb_true_value();
  else
    return mrb_false_value();
}

static mrb_value mrb_matrix_get_ij(mrb_state *mrb, mrb_value self) {
  mrb_int i, j;
  gsl_matrix *p_mat = NULL;

  mrb_get_args(mrb, "ii", &i, &j);
  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  if (i >= p_mat->size1 || j >= p_mat->size2) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix index out of range!");
  }
  return mrb_float_value(mrb, gsl_matrix_get(p_mat, (size_t)i, (size_t)j));
}

static mrb_value mrb_matrix_get_row(mrb_state *mrb, mrb_value self) {
  mrb_int i;
  mrb_value res, args[1];
  gsl_matrix *p_mat = NULL;
  gsl_vector *p_vec = NULL;

  mrb_get_args(mrb, "i", &i);
  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  if (i >= p_mat->size1) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix index out of range!");
  }
  args[0] = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@cols"));
  res = mrb_obj_new(mrb, mrb_class_get(mrb, "Vector"), 1, args);
  mrb_vector_get_data(mrb, res, &p_vec);
  gsl_matrix_get_row (p_vec, p_mat, i);
  return res;
}

static mrb_value mrb_matrix_get_col(mrb_state *mrb, mrb_value self) {
  mrb_int i;
  mrb_value res, args[1];
  gsl_matrix *p_mat = NULL;
  gsl_vector *p_vec = NULL;

  mrb_get_args(mrb, "i", &i);
  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  if (i >= p_mat->size2) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix index out of range!");
  }
  args[0] = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@rows"));
  res = mrb_obj_new(mrb, mrb_class_get(mrb, "Vector"), 1, args);
  mrb_vector_get_data(mrb, res, &p_vec);
  gsl_matrix_get_col (p_vec, p_mat, i);
  return res;
}

static mrb_value mrb_matrix_set_ij(mrb_state *mrb, mrb_value self) {
  mrb_int i, j;
  mrb_float f;
  gsl_matrix *p_mat = NULL;

  mrb_get_args(mrb, "iif", &i, &j, &f);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  if (i >= p_mat->size1 || j >= p_mat->size2) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix index out of range!");
  }
  gsl_matrix_set(p_mat, (size_t)i, (size_t)j, (double)f);
  return mrb_float_value(mrb, f);
}

static mrb_value mrb_matrix_set_row(mrb_state *mrb, mrb_value self) {
  mrb_int i;
  mrb_value other;
  gsl_matrix *p_mat = NULL;
  gsl_vector *p_vec = NULL;

  mrb_get_args(mrb, "io", &i, &other);
  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  if (i >= p_mat->size1) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix row index out of range!");
  }
  mrb_vector_get_data(mrb, other, &p_vec);
  if (p_mat->size2 != p_vec->size) {
    mrb_raise(mrb, E_MATRIX_ERROR, "Size mismatch!");
  }
  gsl_matrix_set_row(p_mat, i, p_vec);
  return self;
}

static mrb_value mrb_matrix_set_col(mrb_state *mrb, mrb_value self) {
  mrb_int i;
  mrb_value other;
  gsl_matrix *p_mat = NULL;
  gsl_vector *p_vec = NULL;

  mrb_get_args(mrb, "io", &i, &other);
  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  if (i >= p_mat->size2) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix col index out of range!");
  }
  mrb_vector_get_data(mrb, other, &p_vec);
  if (p_mat->size1 != p_vec->size) {
    mrb_raise(mrb, E_MATRIX_ERROR, "Size mismatch!");
  }
  gsl_matrix_set_col(p_mat, i, p_vec);
  return self;
}


static mrb_value mrb_matrix_max(mrb_state *mrb, mrb_value self) {
  gsl_matrix *p_mat = NULL;
  mrb_matrix_get_data(mrb, self, &p_mat);
  return mrb_float_value(mrb, gsl_matrix_max(p_mat));
}

static mrb_value mrb_matrix_min(mrb_state *mrb, mrb_value self) {
  gsl_matrix *p_mat = NULL;
  mrb_matrix_get_data(mrb, self, &p_mat);
  return mrb_float_value(mrb, gsl_matrix_min(p_mat));
}

static mrb_value mrb_matrix_max_index(mrb_state *mrb, mrb_value self) {
  unsigned long i, j;
  mrb_value res = mrb_ary_new_capa(mrb, 2);
  gsl_matrix *p_mat = NULL;
  mrb_matrix_get_data(mrb, self, &p_mat);
  gsl_matrix_max_index(p_mat, &i, &j);
  mrb_ary_push(mrb, res, mrb_fixnum_value(i));
  mrb_ary_push(mrb, res, mrb_fixnum_value(j));
  return res;
}

static mrb_value mrb_matrix_min_index(mrb_state *mrb, mrb_value self) {
  unsigned long i, j;
  mrb_value res = mrb_ary_new_capa(mrb, 2);
  gsl_matrix *p_mat = NULL;
  mrb_matrix_get_data(mrb, self, &p_mat);
  gsl_matrix_min_index(p_mat, &i, &j);
  mrb_ary_push(mrb, res, mrb_fixnum_value(i));
  mrb_ary_push(mrb, res, mrb_fixnum_value(j));
  return res;
}

static mrb_value mrb_matrix_add(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_matrix *p_mat, *p_mat_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  mrb_matrix_get_data(mrb, other, &p_mat_other);
  if (p_mat->size1 != p_mat_other->size1 ||
      p_mat->size2 != p_mat_other->size2) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix dimensions don't match!");
  }
  gsl_matrix_add(p_mat, p_mat_other);
  return self;
}

static mrb_value mrb_matrix_sub(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_matrix *p_mat, *p_mat_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  mrb_matrix_get_data(mrb, other, &p_mat_other);
  if (p_mat->size1 != p_mat_other->size1 ||
      p_mat->size2 != p_mat_other->size2) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix dimensions don't match!");
  }
  gsl_matrix_sub(p_mat, p_mat_other);
  return self;
}

static mrb_value mrb_matrix_mul(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_matrix *p_mat, *p_mat_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  mrb_matrix_get_data(mrb, other, &p_mat_other);
  if (p_mat->size1 != p_mat_other->size1 ||
      p_mat->size2 != p_mat_other->size2) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix dimensions don't match!");
  }
  gsl_matrix_mul_elements(p_mat, p_mat_other);
  return self;
}

static mrb_value mrb_matrix_div(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_matrix *p_mat, *p_mat_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  mrb_matrix_get_data(mrb, other, &p_mat_other);
  if (p_mat->size1 != p_mat_other->size1 ||
      p_mat->size2 != p_mat_other->size2) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix dimensions don't match!");
  }
  gsl_matrix_div_elements(p_mat, p_mat_other);
  return self;
}

static mrb_value mrb_matrix_scale(mrb_state *mrb, mrb_value self) {
  gsl_matrix *p_mat;
  mrb_float factor;
  mrb_get_args(mrb, "f", &factor);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  gsl_matrix_scale(p_mat, factor);
  return self;
}

static mrb_value mrb_matrix_add_scalar(mrb_state *mrb, mrb_value self) {
  gsl_matrix *p_mat;
  mrb_float offset;
  mrb_get_args(mrb, "f", &offset);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  gsl_matrix_add_constant(p_mat, offset);
  return self;
}

static mrb_value mrb_matrix_transpose_self(mrb_state *mrb, mrb_value self) {
  gsl_matrix *p_mat;
  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  if (p_mat->size1 != p_mat->size2) {
    mrb_raise(mrb, E_MATRIX_ERROR, "matrix must be square!");
  }
  gsl_matrix_transpose(p_mat);
  return self;
}

static mrb_value mrb_matrix_transpose(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_matrix *p_mat, *p_mat_other;
  mrb_value args[2];
  // swap dimensions!
  args[1] = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@rows"));
  args[0] = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@cols"));
  other = mrb_obj_new(mrb, mrb_class_get(mrb, "Matrix"), 2, args);
  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  mrb_matrix_get_data(mrb, other, &p_mat_other);
  gsl_matrix_transpose_memcpy(p_mat_other, p_mat);
  return other;
}

static mrb_value mrb_matrix_swap_rows(mrb_state *mrb, mrb_value self) {
  gsl_matrix *p_mat;
  mrb_int i, j;
  mrb_get_args(mrb, "ii", &i, &j);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  gsl_matrix_swap_rows(p_mat, i, j);
  return self;
}

static mrb_value mrb_matrix_swap_cols(mrb_state *mrb, mrb_value self) {
  gsl_matrix *p_mat;
  mrb_int i, j;
  mrb_get_args(mrb, "ii", &i, &j);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);
  gsl_matrix_swap_columns(p_mat, i, j);
  return self;
}

static mrb_value mrb_matrix_prod(mrb_state *mrb, mrb_value self) {
  mrb_value other, res;
  gsl_matrix *p_mat, *p_mat_other, *p_mat_res;
  gsl_vector *p_vec_other, *p_vec_res;
  mrb_value args[2];
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_matrix_get_data(mrb, self, &p_mat);

  if (mrb_obj_is_kind_of(mrb, other, mrb_class_get(mrb, "Matrix"))) {
    args[1] = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@rows"));
    args[0] = mrb_iv_get(mrb, other, mrb_intern_lit(mrb, "@cols"));
    res = mrb_obj_new(mrb, mrb_class_get(mrb, "Matrix"), 2, args);
    mrb_matrix_get_data(mrb, other, &p_mat_other);
    mrb_matrix_get_data(mrb, res, &p_mat_res);

    if (p_mat->size2 != p_mat_other->size1) {
      mrb_raise(mrb, E_MATRIX_ERROR, "matrix dimensions don't match!");
    }
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, p_mat, p_mat_other, 0.0,
                   p_mat_res);
  }
  else if (mrb_obj_is_kind_of(mrb, other, mrb_class_get(mrb, "Vector"))) {
    args[0] = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@rows"));
    res = mrb_obj_new(mrb, mrb_class_get(mrb, "Vector"), 1, args);
    mrb_vector_get_data(mrb, other, &p_vec_other);
    mrb_vector_get_data(mrb, res, &p_vec_res);

    if (p_mat->size2 != p_vec_other->size) {
      mrb_raise(mrb, E_MATRIX_ERROR, "matrix dimensions don't match!");
    }
    gsl_blas_dgemv(CblasNoTrans, 1.0, p_mat, p_vec_other, 0.0, p_vec_res);
  }
  return res;
}

void mrb_gsl_matrix_init(mrb_state *mrb) {
  struct RClass *gsl;

  mrb_load_string(mrb, "class MatrixError < Exception; end");

  gsl = mrb_define_class(mrb, "Matrix", mrb->object_class);
  mrb_define_method(mrb, gsl, "initialize", mrb_matrix_initialize,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "all", mrb_matrix_all, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "zero", mrb_matrix_zero, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "identity", mrb_matrix_identity, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "===", mrb_matrix_equal, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "[]", mrb_matrix_get_ij, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, gsl, "row", mrb_matrix_get_row, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "col", mrb_matrix_get_col, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "[]=", mrb_matrix_set_ij, MRB_ARGS_REQ(3));
  mrb_define_method(mrb, gsl, "set_row", mrb_matrix_set_row, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, gsl, "set_col", mrb_matrix_set_col, MRB_ARGS_REQ(2));

  mrb_define_method(mrb, gsl, "max", mrb_matrix_max, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "max_index", mrb_matrix_max_index,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "min", mrb_matrix_min, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "min_index", mrb_matrix_min_index,
                    MRB_ARGS_NONE());

  mrb_define_method(mrb, gsl, "add", mrb_matrix_add, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "sub", mrb_matrix_sub, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "mul", mrb_matrix_mul, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "div", mrb_matrix_div, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "scale", mrb_matrix_scale, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "add_scalar", mrb_matrix_add_scalar,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "t!", mrb_matrix_transpose_self, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "t", mrb_matrix_transpose, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "swap_rows", mrb_matrix_swap_rows,
                    MRB_ARGS_REQ(2));
  mrb_define_method(mrb, gsl, "swap_cols", mrb_matrix_swap_cols,
                    MRB_ARGS_REQ(2));
  mrb_define_method(mrb, gsl, "*", mrb_matrix_prod, MRB_ARGS_REQ(1));
}
