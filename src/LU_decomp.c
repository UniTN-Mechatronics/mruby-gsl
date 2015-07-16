/***************************************************************************/
/*                                                                         */
/* LU_decomp.c - LU Decomposition class for mruby                          */
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

#include <gsl/gsl_linalg.h>
#include <stdio.h>
#include "matrix.h"
#include "vector.h"
#include "LU_decomp.h"


#pragma mark -
#pragma mark • Utilities

// Garbage collector handler, for play_data struct
// if play_data contains other dynamic data, free it too!
// Check it with GC.start
void lu_decomp_destructor(mrb_state *mrb, void *p_) {
  lu_decomp_data_s *lu = (lu_decomp_data_s *)p_;
  gsl_matrix_free(lu->mat);
  gsl_permutation_free(lu->p);
  free(lu);
};

// Creating data type and reference for GC, in a const struct
const struct mrb_data_type lu_decomp_data_type = {"lu_decomp_data",
                                                  lu_decomp_destructor};

// Utility function for getting the struct out of the wrapping IV @data
void mrb_lu_decomp_get_data(mrb_state *mrb, mrb_value self,
                            lu_decomp_data_s **data) {
  mrb_value data_value;
  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // Loading data from data_value into p_data:
  Data_Get_Struct(mrb, data_value, &lu_decomp_data_type, *data);
  if (!*data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not access @data");
}


#pragma mark -
#pragma mark • Initializations

// Data Initializer C function (not exposed!)
static mrb_value mrb_lu_initialize(mrb_state *mrb, mrb_value self) {
  mrb_value data_value;            // this IV holds the data
  lu_decomp_data_s *p_data = NULL; // pointer to the C struct
  mrb_value matrix;
  gsl_matrix *p_mat = NULL;
  mrb_int n;

  mrb_get_args(mrb, "o", &matrix);
  if (!mrb_obj_is_kind_of(mrb, matrix, mrb_class_get(mrb, "Matrix"))) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Argument must be a Matrix");
  }

  mrb_matrix_get_data(mrb, matrix, &p_mat);
  if (p_mat->size1 != p_mat->size2) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Argument must be a square Matrix");
  }
  n = p_mat->size1;

  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));
  // if @data already exists, free its content:
  if (!mrb_nil_p(data_value)) {
    Data_Get_Struct(mrb, data_value, &lu_decomp_data_type, p_data);
    free(p_data);
  }
  // Allocate and zero-out the data struct:
  p_data = (lu_decomp_data_s *)malloc(sizeof(lu_decomp_data_s));
  if (!p_data) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not allocate @data");
  }
  p_data->mat = gsl_matrix_calloc(n, n);
  p_data->p = gsl_permutation_calloc(n);
  p_data->size = n;
  // copy argument matrix into local object data
  gsl_matrix_memcpy(p_data->mat, p_mat);
  // invert in-place
  gsl_linalg_LU_decomp(p_data->mat, p_data->p, &p_data->sgn);

  // Wrap struct into @data:
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@data"), // set @data
             mrb_obj_value( // with value hold in struct
                 Data_Wrap_Struct(mrb, mrb->object_class, &lu_decomp_data_type,
                                  p_data)));
  return mrb_nil_value();
}


#pragma mark -
#pragma mark • Operations

// int gsl_linalg_LU_invert (const gsl_matrix * LU, const gsl_permutation * p,
// gsl_matrix * inverse)
static mrb_value mrb_lu_invert(mrb_state *mrb, mrb_value self) {
  mrb_value result;
  lu_decomp_data_s *p_data = NULL;
  gsl_matrix *p_res = NULL;
  mrb_value args[2];

  // call utility for unwrapping @data into p_data:
  mrb_lu_decomp_get_data(mrb, self, &p_data);
  args[0] = args[1] = mrb_fixnum_value(p_data->size);
  result = mrb_obj_new(mrb, mrb_class_get(mrb, "Matrix"), 2, args);
  mrb_matrix_get_data(mrb, result, &p_res);
  if (gsl_linalg_LU_invert(p_data->mat, p_data->p, p_res)) {
    mrb_raise(mrb, E_LU_DECOMP_ERROR, "Singular matrix");
  }
  return result;
}

// double gsl_linalg_LU_det (gsl_matrix * LU, int signum)
static mrb_value mrb_lu_det(mrb_state *mrb, mrb_value self) {
  double result = 0;
  lu_decomp_data_s *p_data = NULL;

  // call utility for unwrapping @data into p_data:
  mrb_lu_decomp_get_data(mrb, self, &p_data);
  result = gsl_linalg_LU_det(p_data->mat, p_data->sgn);
  return mrb_float_value(mrb, result);
}

// int gsl_linalg_LU_solve (const gsl_matrix * LU, const gsl_permutation * p,
// const gsl_vector * b, gsl_vector * x)
static mrb_value mrb_lu_solve(mrb_state *mrb, mrb_value self) {
  mrb_value result, x_vec;
  lu_decomp_data_s *p_data = NULL;
  gsl_vector *p_res = NULL, *p_x = NULL;
  mrb_value args[1];

  mrb_get_args(mrb, "o", &x_vec);
  if (!mrb_obj_is_kind_of(mrb, x_vec, mrb_class_get(mrb, "Vector"))) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Argument must be a Vector");
  }

  // call utility for unwrapping @data into p_data:
  mrb_lu_decomp_get_data(mrb, self, &p_data);
  args[0] = mrb_fixnum_value(p_data->size);

  mrb_vector_get_data(mrb, x_vec, &p_x);
  if (p_x->size != p_data->size) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Argument must be a square Matrix");
  }

  result = mrb_obj_new(mrb, mrb_class_get(mrb, "Vector"), 1, args);
  mrb_vector_get_data(mrb, result, &p_res);
  if (gsl_linalg_LU_solve(p_data->mat, p_data->p, p_x, p_res)) {
    mrb_raise(mrb, E_LU_DECOMP_ERROR, "Singular matrix");
  }
  return result;
}



#pragma mark -
#pragma mark • Gem setup

void mrb_gsl_lu_decomp_init(mrb_state *mrb) {
  struct RClass *lu;

  mrb_load_string(mrb, "class LUDecompError < Exception; end");

  lu = mrb_define_class(mrb, "LUDecomp", mrb->object_class);
  mrb_define_method(mrb, lu, "initialize", mrb_lu_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, lu, "inv", mrb_lu_invert, MRB_ARGS_NONE());
  mrb_define_method(mrb, lu, "det", mrb_lu_det, MRB_ARGS_NONE());
  mrb_define_method(mrb, lu, "solve", mrb_lu_solve, MRB_ARGS_REQ(1));
}
