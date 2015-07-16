# mruby-gsl
[![Build Status](https://travis-ci.org/UniTN-Mechatronics/mruby-gsl.svg)](https://travis-ci.org/UniTN-Mechatronics/mruby-gsl)

This is a (very) partial wrapper to GSL functions. Its main target is to provide the basic functionalities for working with Matrices and Vectors.

## Installing
To include in your custom mruby, add the following to `build_config.rb`:

```ruby
conf.gem :github => 'UniTN-Mechatronics/mruby-gsl', :branch => 'master'
```

To simply test it into a minimal mruby:

```sh
$ make
$ tmp/mruby/bin/mirb
```

## Error messages
By default, GSL error messages are printed to stdout. This happens in addition to standard Ruby errors. If you want to disable GSL error messages, use the Kernel method `gsl_info_off`, and use `gsl_info_on` to re-enable.

Default behavior can be reversed (i.e. no printout) by disabling the compiler switch `GSL_ERROR_MSG_PRINTOUT` in `mrbgem.rake`.

## Vectors and Matrix Arithmetic Operators
There are two types of operators for Vectors and Matrices: destructive and non destructuve. For example, `Vector#add!` sums a scalar or another vector (element-wise) to the original vector itself (so it is desctructive), and so do `Vector#sub!`, `Vector#mul!`, and `Vector#div!`. 

Conversely, the corresponding methods `Vector#+`, `Vector#-`, `Vector#*`, and `Vector#/` return a new vector (non-destructive). The operator `Vector#^` returns the scalar product (a Float).

Similarly, for Matrices, `Matrix#add!`, `Matrix#sub!`, `Matrix#mul!`, and `Matrix#div!` operates on the matrix performing element-wise operations with a scalar or another equally-sized Matrix. The corresponding, non-destrctive operations are `Matrix#+`, `Matrix#-`, `Matrix#*`, and `Matrix#/`. 

The Matrix multiplication is obtained by `Matrix#^`, which expects another Matrix with compatible sizes, or a Vector (implicitly converted to a column-matrix).

## Vector class

The `Vector` class implements a fixed-length numeric vector (using `double` values for internal storage).

```ruby
v1 = Vector[1,2,3] #=> V[1,2,3]
v2 = Vector[6,5,4] #=> V[6,5,4]
puts v1[1]         #=> 2
v1.to_a            #=> [1, 2, 3]
v1.add! v2         #=> v1 = V[7, 7, 7], changes v1! also Vector#sub, Vector#mul, Vector#div
v1.sum             #=> 21, summation over all elements, also Vector#norm
v2.max_index       #=> 0, also Vector#max, Vector#min, Vector#min_index
v1 = Vector[1,2,3]
v1^v2              #=> 28
v2.reverse!        #=> V[4,5,6]
v2.swap!(0,2)      #=> V[6,5,4]
```

The `Vector` class includes the Enumerable module and supports iteration via #each.

Also available methods:

* `Vector#size`
* `Vector#rnd_fill`
* `Vector#all`
* `Vector#zero`
* `Vector#basis`
* `Vector#===`, same vectors
* `Vector#==`, same sizes
* `Vector#[]`
* `Vector#[]=`
* `Vector#mean`, optional Float argument for passing a given value of mean
* `Vector#variance`, optional Float argument for passing a given value of mean
* `Vector#sd`, optional Float argument for passing a given value of mean
* `Vector#absdev`, optional Float argument for passing a given value of mean
* `Vector#median`
* `Vector#quantile`

The `Vector` class includes the Enumerable module and supports iteration via `#each`.

## Matrix class

The `Matrix` class implements a fixed-size numeric matrix (using `double` values for internal storage).

```ruby
m1 = Matrix[[1,2],[-3,1]]   #=> M[[1, 2], [-3, 1]]
m2 = Matrix[[7,-5],[-2,37]] #=> M[[7, -5], [-2, 37]]
puts m1                     #=> fancy output
m1 ^ m2                     #=> M[[3, 69], [-23, 52]]
m1.mul! 2                   #=> M[[2, 4], [-6, 2]], element-wise operators
m1.t                        #=> M[[2, -6], [4, 2]], transpose, also Matrix#t!
v1.to_mat                   #=> M[[1], [2], [3]]
m1*Vector[3,4]              #=> V[22, -10]
```

Element getters have two alternative syntaxes:

1. `m[i,j]` gives the *i,j*-th element (also for writing)
2. `m[i]` returns the *i*-th row, as a Vector
3. `m[]` returns an Array of Vectors representing rows of `m`
4. `m[i][j]` as for 1., but **not for writing!**

Also available methods:

* `Matrix#nrows`
* `Matrix#ncols`
* `Matrix#size`, returns an array of `[nrows, ncols]`
* `Matrix#rnd_fill`
* `Matrix#===`, same matrices
* `Matrix#==`, same sizes
* `Matrix#[]`
* `Matrix#[]=`
* `Matrix#row`
* `Matrix#col`
* `Matrix#set_row `
* `Matrix#set_col `
* `Matrix#all`
* `Matrix#zero`
* `Matrix#identity`
* `Matrix#max`
* `Matrix#max_index`
* `Matrix#min`
* `Matrix#min_index`
* `Matrix#swap_rows`
* `Matrix#swap_cols`
* `Matrix#each_col`
* `Matrix#each_row`
* `Matrix#lu`
* `Matrix#det`
* `Matrix#inv`

The `Matrix` class includes the Enumerable module and supports iteration via `#each`. Notably, there is the `#each_with_indexes` method (whose block takes three arguments), and the `#map!` method.


## LUDecomp

LU Decomposition, for inverting matrices and solving linear systems.

```ruby
m1 = Matrix[[1,2],[-3,1]] 
lu = LUDecomp.new(m1)  #=> also: lu = m1.lu
lu.inv                 #=> M[[0.14285714285714, -0.28571428571429], [0.42857142857143, 0.14285714285714]]
lu.solve Vector[3,-7]  #=> V[2.4285714285714, 0.28571428571429]
lu.det                 #=> 7
```

## To Do list

The following features are expected to be implemented, in order of precedence:

* QR decomposition
* SV decomposition
* Eigensystems
* Interpolation
* FFT
