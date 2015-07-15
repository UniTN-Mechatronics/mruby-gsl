# mruby-gsl
[![Build Status](https://travis-ci.org/UniTN-Mechatronics/mruby-gsl.svg)](https://travis-ci.org/UniTN-Mechatronics/mruby-gsl)

This is a (very) partial wrapper to GSL functions. Its main target is to provide the basic functionalities for working with Matrices and Vectors.

## Error messages
By default, GSL error messages are printed to stdout. This happens in addition to standard Ruby errors. If you want to disable GSL error messages, use the Kernel method `gsl_info_off`, and use `gsl_info_on` to re-enable.

Default behavior can be reversed (i.e. no printout) by disabling the compiler switch `GSL_ERROR_MSG_PRINTOUT` in `mrbgem.rake`.

## Vector class

The `Vector` class implements a fixed-length numeric vector (using `double` values for internal storage).

```ruby
v1 = Vector[1,2,3] #=> V[1,2,3]
v2 = Vector[6,5,4] #=> V[6,5,4]
puts v1[1]         #=> 2
v1.to_a            #=> [1, 2, 3]
v1.add v2          #=> V[7, 7, 7], changes v1! also Vector#sub, Vector#mul, Vector#div
v1.sum             #=> 21, summation over all elements, also Vector#norm
v2.max_index       #=> 0, also Vector#max, Vector#min, Vector#min_index
v1 = Vector[1,2,3]
v1*v2              #=> 28
v2.reverse!        #=> V[4,5,6]
v2.swap!(0,2)      #=> V[6,5,4]
```

The `Vector` class includes the Enumerable module and supports iteration via #each.

Also available methods:

* `Vector#all`
* `Vector#zero`
* `Vector#basis`
* `Vector#===`, same matrices
* `Vector#==`, same sizes
* `Vector#[]`
* `Vector#[]=`
* `Vector#mean`, optional Float argument for passing a given value of mean
* `Vector#variance`, optional Float argument for passing a given value of mean
* `Vector#sd`, optional Float argument for passing a given value of mean
* `Vector#absdev`, optional Float argument for passing a given value of mean
* `Vector#median`
* `Vector#quantile`

The `Matrix` class includes the Enumerable module and supports iteration via `#each`.

## Matrix class

The `Matrix` class implements a fixed-size numeric matrix (using `double` values for internal storage).

```ruby
m1 = Matrix[[1,2],[-3,1]]   #=> M[[1, 2], [-3, 1]]
m2 = Matrix[[7,-5],[-2,37]] #=> M[[7, -5], [-2, 37]]
puts m1                     #=> fancy output
m1 * m2                     #=> M[[3, 69], [-23, 52]]
m1.mul 2                    #=> M[[2, 4], [-6, 2]], element-wise operators
m1.t                        #=> M[[2, -6], [4, 2]], transpose, also Matrix#t!
v1.to_mat                   #=> M[[1], [2], [3]]
m1*Vector[3,4]              #=> V[22, -10]
```

Also available methods:

* `Matrix#===, same matrices`
* `Matrix#==, same sizes`
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

The `Matrix` class includes the Enumerable module and supports iteration via `#each`.

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
