/***************************************************************************/
/*                                                                         */
/* QR_decomp.c - LU Decomposition class for mruby                          */
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
#include "QR_decomp.h"

#ifndef MIN
#define MAX(a, b)                                                              \
  ({                                                                           \
    __typeof__(a) _a = (a);                                                    \
    __typeof__(b) _b = (b);                                                    \
    _a < _b ? _a : _b;                                                         \
  })
#endif

#pragma mark -
#pragma mark • Utilities

// Garbage collector handler, for play_data struct
// if play_data contains other dynamic data, free it too!
// Check it with GC.start
void qr_decomp_destructor(mrb_state *mrb, void *p_) {
  qr_decomp_data_s *lu = (qr_decomp_data_s *)p_;
  gsl_matrix_free(lu->mat);
  gsl_vector_free(lu->tau);
  free(lu);
};

// Creating data type and reference for GC, in a const struct
const struct mrb_data_type qr_decomp_data_type = {"qr_decomp_data",
                                                  qr_decomp_destructor};

// Utility function for getting the struct out of the wrapping IV @data
void mrb_qr_decomp_get_data(mrb_state *mrb, mrb_value self,
                            qr_decomp_data_s **data) {
  mrb_value data_value;
  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // Loading data from data_value into p_data:
  Data_Get_Struct(mrb, data_value, &qr_decomp_data_type, *data);
  if (!*data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not access @data");
}

#pragma mark -
#pragma mark • Initializations

// Data Initializer C function (not exposed!)
static mrb_value mrb_qr_initialize(mrb_state *mrb, mrb_value self) {
  mrb_value data_value;            // this IV holds the data
  qr_decomp_data_s *p_data = NULL; // pointer to the C struct
  mrb_value matrix;
  gsl_matrix *p_mat = NULL;
  mrb_int size1, size2;

  mrb_get_args(mrb, "o", &matrix);
  if (!mrb_obj_is_kind_of(mrb, matrix, mrb_class_get(mrb, "Matrix"))) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Argument must be a Matrix");
  }

  mrb_matrix_get_data(mrb, matrix, &p_mat);
  // if (p_mat->size1 < p_mat->size2) {
  //   mrb_raise(mrb, E_ARGUMENT_ERROR, "Argument must be square or horizontal
  //   Matrix");
  // }
  size1 = p_mat->size1;
  size2 = p_mat->size2;

  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));
  // if @data already exists, free its content:
  if (!mrb_nil_p(data_value)) {
    Data_Get_Struct(mrb, data_value, &qr_decomp_data_type, p_data);
    free(p_data);
  }
  // Allocate and zero-out the data struct:
  p_data = (qr_decomp_data_s *)malloc(sizeof(qr_decomp_data_s));
  if (!p_data) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not allocate @data");
  }
  p_data->mat = gsl_matrix_calloc(size1, size2);
  p_data->tau = gsl_vector_calloc(MIN(size1, size2));
  p_data->size1 = size1;
  p_data->size1 = size2;
  // copy argument matrix into local object data
  gsl_matrix_memcpy(p_data->mat, p_mat);
  // invert in-place
  gsl_linalg_QR_decomp(p_data->mat, p_data->tau);

  // Wrap struct into @data:
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@data"), // set @data
             mrb_obj_value( // with value hold in struct
                 Data_Wrap_Struct(mrb, mrb->object_class, &qr_decomp_data_type,
                                  p_data)));
  printf("%d x %d\n", size1, size2);
  return mrb_nil_value();
}

#pragma mark -
#pragma mark • Operations

// int gsl_linalg_QR_solve (const gsl_matrix * LU, const gsl_permutation * p,
// const gsl_vector * b, gsl_vector * x)
static mrb_value mrb_qr_solve(mrb_state *mrb, mrb_value self) {
  mrb_value result, x_vec;
  qr_decomp_data_s *p_data = NULL;
  gsl_vector *p_res = NULL, *p_x = NULL;
  mrb_value args[1];

  mrb_get_args(mrb, "o", &x_vec);
  if (!mrb_obj_is_kind_of(mrb, x_vec, mrb_class_get(mrb, "Vector"))) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Argument must be a Vector");
  }

  // call utility for unwrapping @data into p_data:
  mrb_qr_decomp_get_data(mrb, self, &p_data);

  args[0] = mrb_fixnum_value(p_data->tau->size);
  result = mrb_obj_new(mrb, mrb_class_get(mrb, "Vector"), 1, args);
  mrb_vector_get_data(mrb, result, &p_res);
  mrb_vector_get_data(mrb, x_vec, &p_x);

  if (gsl_linalg_QR_solve(p_data->mat, p_data->tau, p_x, p_res)) {
    mrb_raise(mrb, E_QR_DECOMP_ERROR, "Singular matrix");
  }
  return result;
}

#pragma mark -
#pragma mark • Gem setup

void mrb_gsl_qr_decomp_init(mrb_state *mrb) {
  struct RClass *lu;

  mrb_load_string(mrb, "class QRDecompError < Exception; end");

  lu = mrb_define_class(mrb, "QRDecomp", mrb->object_class);
  mrb_define_method(mrb, lu, "initialize", mrb_qr_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, lu, "solve", mrb_qr_solve, MRB_ARGS_REQ(1));
}
