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

#include "vector.h"

// Garbage collector handler, for play_data struct
// if play_data contains other dynamic data, free it too!
// Check it with GC.start
void vector_destructor(mrb_state *mrb, void *p_) {
  gsl_vector *v = (gsl_vector *)p_;
  gsl_vector_free(v);
  // or simply:
  // mrb_free(mrb, pd);
};

// Creating data type and reference for GC, in a const struct
const struct mrb_data_type vector_data_type = {"vector_data",
                                               vector_destructor};

// Utility function for getting the struct out of the wrapping IV @data
void mrb_vector_get_data(mrb_state *mrb, mrb_value self,
                                gsl_vector **data) {
  mrb_value data_value;
  data_value = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@data"));

  // Loading data from data_value into p_data:
  Data_Get_Struct(mrb, data_value, &vector_data_type, *data);
  if (!*data)
    mrb_raise(mrb, E_RUNTIME_ERROR, "Could not access @data");
}


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
  return mrb_nil_value();
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

static mrb_value mrb_vector_equal(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_vector *p_vec, *p_vec_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  mrb_vector_get_data(mrb, other, &p_vec_other);
  if (p_vec->size != p_vec_other->size) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Vector indexes don't match!");
  }
  if (1 == gsl_vector_equal(p_vec, p_vec_other))
    return mrb_true_value();
  else
    return mrb_false_value();
}

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

static mrb_value mrb_vector_length(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec = NULL;
  mrb_vector_get_data(mrb, self, &p_vec);
  return mrb_fixnum_value(p_vec->size);
}

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

static mrb_value mrb_vector_add(mrb_state *mrb, mrb_value self) {
  mrb_value other;
  gsl_vector *p_vec, *p_vec_other;
  mrb_get_args(mrb, "o", &other);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  mrb_vector_get_data(mrb, other, &p_vec_other);
  if (p_vec->size != p_vec_other->size) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Vector indexes don't match!");
  }
  gsl_vector_add(p_vec, p_vec_other);
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
    mrb_raise(mrb, E_VECTOR_ERROR, "Vector indexes don't match!");
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
  mrb_vector_get_data(mrb, other, &p_vec_other);
  if (p_vec->size != p_vec_other->size) {
    mrb_raise(mrb, E_VECTOR_ERROR, "Vector indexes don't match!");
  }
  gsl_vector_mul(p_vec, p_vec_other);
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

static mrb_value mrb_vector_scale(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;
  mrb_float factor;
  mrb_get_args(mrb, "f", &factor);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  gsl_vector_scale(p_vec, factor);
  return self;
}

static mrb_value mrb_vector_add_scalar(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;
  mrb_float offset;
  mrb_get_args(mrb, "f", &offset);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  gsl_vector_add_constant(p_vec, offset);
  return self;
}

static mrb_value mrb_vector_swap(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;
  mrb_int i, j;
  mrb_get_args(mrb, "ii", &i, &j);

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  gsl_vector_swap_elements(p_vec, i, j);
  return self;
}

static mrb_value mrb_vector_reverse(mrb_state *mrb, mrb_value self) {
  gsl_vector *p_vec;

  // call utility for unwrapping @data into p_data:
  mrb_vector_get_data(mrb, self, &p_vec);
  gsl_vector_reverse(p_vec);
  return self;
}

#if 0
static mrb_value mrb_play_d(mrb_state *mrb, mrb_value self) {
  play_data_s *p_data = NULL;

  // call utility for unwrapping @data into p_data:
  mrb_play_get_data(mrb, self, &p_data);

  // Play with p_data content:
  return mrb_float_value(mrb, p_data->d);
}


static mrb_value mrb_play_ary(mrb_state *mrb, mrb_value self) {
  play_data_s *p_data = NULL;
  mrb_value ary;
  mrb_int i;
  // call utility for unwrapping @data into p_data:
  mrb_play_get_data(mrb, self, &p_data);

  // Play with p_data content:
  ary = mrb_ary_new_capa(mrb, p_data->i);
  for (i = 0; i < p_data->i; i++) {
    mrb_ary_set(mrb, ary, i, mrb_float_value(mrb, p_data->ary[i]));
  }
  return ary;
}

static mrb_value mrb_play_set_ary(mrb_state *mrb, mrb_value self) {
  mrb_value ary_in = mrb_nil_value();
  play_data_s *p_data = NULL;
  mrb_int i;
  mrb_value elem;
  mrb_get_args(mrb, "A", &ary_in);

  // call utility for unwrapping @data into p_data:
  mrb_play_get_data(mrb, self, &p_data);
  if (p_data->ary)
    free(p_data->ary);

  p_data->i = RARRAY_LEN(ary_in);
  p_data->ary = (double *)malloc(sizeof(double) * p_data->i);
  for (i = 0; i < p_data->i; i++) {
    elem = mrb_ary_entry(ary_in, i);
    if (mrb_fixnum_p(elem) || mrb_float_p(elem))
      p_data->ary[i] = mrb_to_flo(mrb, elem);
    else {
      p_data->i = 0;
      p_data->ary = (double *)malloc(0);
      mrb_raisef(mrb, E_RUNTIME_ERROR, "Non-numeric entry at position %S",
                 mrb_fixnum_value(i));
    }
  }
  return mrb_fixnum_value(i);
}

#endif

void mrb_gsl_vector_init(mrb_state *mrb) {
  struct RClass *gsl;

  mrb_load_string(mrb, "class VectorError < Exception; end");

  gsl = mrb_define_class(mrb, "Vector", mrb->object_class);
  mrb_define_method(mrb, gsl, "all", mrb_vector_all, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "zero", mrb_vector_zero, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "basis", mrb_vector_basis, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, gsl, "initialize", mrb_vector_initialize,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "===", mrb_vector_equal, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "[]", mrb_vector_get_i, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "[]=", mrb_vector_set_i, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, gsl, "to_a", mrb_vector_to_a, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "length", mrb_vector_length, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "max", mrb_vector_max, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "min", mrb_vector_min, MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "max_index", mrb_vector_max_index,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "min_index", mrb_vector_min_index,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, gsl, "add", mrb_vector_add, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "sub", mrb_vector_sub, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "mul", mrb_vector_mul, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "div", mrb_vector_div, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "scale", mrb_vector_scale, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "add_scalar", mrb_vector_add_scalar,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, gsl, "swap!", mrb_vector_swap, MRB_ARGS_REQ(2));
  mrb_define_method(mrb, gsl, "reverse!", mrb_vector_reverse, MRB_ARGS_NONE());
}
