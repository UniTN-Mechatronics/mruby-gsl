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
  p_data->size1 = size1;
  p_data->size2 = size2;
  p_data->minsize = MIN(size1, size2);
  p_data->mat = gsl_matrix_calloc(size1, size2);
  p_data->tau = gsl_vector_calloc(p_data->minsize);

  // copy argument matrix into local object data
  gsl_matrix_memcpy(p_data->mat, p_mat);
  // invert in-place
  gsl_linalg_QR_decomp(p_data->mat, p_data->tau);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@size1"), mrb_fixnum_value(size1));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@size2"), mrb_fixnum_value(size2));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@minsize"),
             mrb_fixnum_value(p_data->minsize));
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@residuals"), mrb_nil_value());
  // Wrap struct into @data:
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@data"), // set @data
             mrb_obj_value( // with value hold in struct
                 Data_Wrap_Struct(mrb, mrb->object_class, &qr_decomp_data_type,
                                  p_data)));
  return mrb_nil_value();
}

#pragma mark -
#pragma mark • Accessors

static mrb_value mrb_qr_residuals(mrb_state *mrb, mrb_value self) {
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@residuals"));
}

static mrb_value mrb_qr_minsize(mrb_state *mrb, mrb_value self) {
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@minsize"));
}

static mrb_value mrb_qr_size1(mrb_state *mrb, mrb_value self) {
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@size1"));
}

static mrb_value mrb_qr_size2(mrb_state *mrb, mrb_value self) {
  return mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@size2"));
}

static mrb_value mrb_qr_matrix(mrb_state *mrb, mrb_value self) {
  mrb_value result;
  qr_decomp_data_s *p_data = NULL;
  gsl_matrix *p_res = NULL;
  mrb_value args[2];

  // call utility for unwrapping @data into p_data:
  mrb_qr_decomp_get_data(mrb, self, &p_data);
  args[0] = mrb_fixnum_value(p_data->size1);
  args[1] = mrb_fixnum_value(p_data->size2);
  result = mrb_obj_new(mrb, mrb_class_get(mrb, "Matrix"), 2, args);
  mrb_matrix_get_data(mrb, result, &p_res);
  gsl_matrix_memcpy(p_res, p_data->mat);
  return result;
}

static mrb_value mrb_qr_tau(mrb_state *mrb, mrb_value self) {
  mrb_value result;
  qr_decomp_data_s *p_data = NULL;
  gsl_vector *p_res = NULL;
  mrb_value args[1];

  // call utility for unwrapping @data into p_data:
  mrb_qr_decomp_get_data(mrb, self, &p_data);
  args[0] = mrb_fixnum_value(p_data->minsize);
  result = mrb_obj_new(mrb, mrb_class_get(mrb, "Vector"), 1, args);
  mrb_vector_get_data(mrb, result, &p_res);
  gsl_vector_memcpy(p_res, p_data->tau);
  return result;
}

#pragma mark -
#pragma mark • Operations

// int gsl_linalg_QR_solve (const gsl_matrix * QR, const gsl_vector * tau, const
// gsl_vector * b, gsl_vector * x)
static mrb_value mrb_qr_solve(mrb_state *mrb, mrb_value self) {
  mrb_value result, b_vec;
  qr_decomp_data_s *p_data = NULL;
  gsl_vector *p_result = NULL, *p_b = NULL;
  mrb_value args[1];

  mrb_get_args(mrb, "o", &b_vec);
  if (!mrb_obj_is_kind_of(mrb, b_vec, mrb_class_get(mrb, "Vector"))) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Argument must be a Vector");
  }

  // call utility for unwrapping @data into p_data:
  mrb_qr_decomp_get_data(mrb, self, &p_data);
  if (p_data->size1 != p_data->size2) {
    mrb_raise(mrb, E_QR_DECOMP_ERROR, "Matrix must be square");
  }

  args[0] = mrb_fixnum_value(p_data->tau->size);
  result = mrb_obj_new(mrb, mrb_class_get(mrb, "Vector"), 1, args);
  mrb_vector_get_data(mrb, result, &p_result);
  mrb_vector_get_data(mrb, b_vec, &p_b);

  if (gsl_linalg_QR_solve(p_data->mat, p_data->tau, p_b, p_result)) {
    mrb_raise(mrb, E_QR_DECOMP_ERROR, "Singular matrix");
  }
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@residuals"), mrb_nil_value());
  return result;
}

// int gsl_linalg_QR_lssolve (const gsl_matrix * QR, const gsl_vector * tau,
// const gsl_vector * b, gsl_vector * x, gsl_vector * residual)
static mrb_value mrb_qr_lssolve(mrb_state *mrb, mrb_value self) {
  mrb_value result, b_vec, residuals;
  qr_decomp_data_s *p_data = NULL;
  gsl_vector *p_result = NULL, *p_b = NULL, *p_residuals;
  mrb_value args[1];

  mrb_get_args(mrb, "o", &b_vec);
  if (!mrb_obj_is_kind_of(mrb, b_vec, mrb_class_get(mrb, "Vector"))) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "Argument must be a Vector");
  }

  // call utility for unwrapping @data into p_data:
  mrb_qr_decomp_get_data(mrb, self, &p_data);
  if (p_data->size1 <= p_data->size2) {
    mrb_raise(mrb, E_QR_DECOMP_ERROR,
              "Matrix must have more rows than columns");
  }

  args[0] = mrb_fixnum_value(p_data->size2);
  result = mrb_obj_new(mrb, mrb_class_get(mrb, "Vector"), 1, args);
  mrb_vector_get_data(mrb, result, &p_result);
  mrb_vector_get_data(mrb, b_vec, &p_b);

  args[0] = mrb_fixnum_value(p_b->size);
  residuals = mrb_obj_new(mrb, mrb_class_get(mrb, "Vector"), 1, args);
  mrb_vector_get_data(mrb, residuals, &p_residuals);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@residuals"), residuals);

  if (gsl_linalg_QR_lssolve(p_data->mat, p_data->tau, p_b, p_result,
                            p_residuals)) {
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
  mrb_define_method(mrb, lu, "residuals", mrb_qr_residuals, MRB_ARGS_NONE());
  mrb_define_method(mrb, lu, "matrix", mrb_qr_matrix, MRB_ARGS_NONE());
  mrb_define_method(mrb, lu, "tau", mrb_qr_tau, MRB_ARGS_NONE());
  mrb_define_method(mrb, lu, "size1", mrb_qr_size1, MRB_ARGS_NONE());
  mrb_define_method(mrb, lu, "size2", mrb_qr_size2, MRB_ARGS_NONE());
  mrb_define_method(mrb, lu, "minsize", mrb_qr_minsize, MRB_ARGS_NONE());

  mrb_define_method(mrb, lu, "initialize", mrb_qr_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, lu, "solve", mrb_qr_solve, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, lu, "lssolve", mrb_qr_lssolve, MRB_ARGS_REQ(1));
}
