#*************************************************************************#
#                                                                         #
# matrix.rb - Matrix class for mruby                                      #
# Copyright (C) 2015 Paolo Bosetti                                        #
# paolo[dot]bosetti[at]unitn.it                                           #
# Department of Industrial Engineering, University of Trento              #
#                                                                         #
# This library is free software.  You can redistribute it and/or          #
# modify it under the terms of the GNU GENERAL PUBLIC LICENSE 2.0.        #
#                                                                         #
# This library is distributed in the hope that it will be useful,         #
# but WITHOUT ANY WARRANTY; without even the implied warranty of          #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           #
# Artistic License 2.0 for more details.                                  #
#                                                                         #
# See the file LICENSE                                                    #
#                                                                         #
#*************************************************************************#

class Matrix
  include Enumerable
  include Comparable
  attr_reader :ncols, :nrows
  attr_accessor :format
  
  def self.[](*ary)
    raise ArgumentError unless ary.kind_of? Array
    m = Matrix.new(ary.size, ary[0].size)
    ary.each_with_index do |row,i|
      row.each_with_index {|e,j| m[i,j] = e.to_f}
    end
    return m
  end
  
  def to_a
    rows = []
    cols = []
    @nrows.times do |i|
      cols = []
      @ncols.times do |j|
        cols << self[i,j]
      end
      rows << cols
    end
    return rows
  end
  
  def <=>(other)
    (@nrows * @ncols) <=> (other.rows * other.cols)
  end
  
  def size
    [@nrows, @ncols]
  end
  
  def each
    raise ArgumentError, "Need a block" unless block_given?
    @nrows.times do |i|
      @ncols.times do |j|
        yield self[i,j]
      end
    end
  end
  
  def each_with_indexes
    raise ArgumentError, "Need a block" unless block_given?
    @nrows.times do |i|
      @ncols.times do |j|
        yield self[i,j], i, j
      end
    end
  end
  
  def map!
    raise ArgumentError, "Need a block" unless block_given?
    @nrows.times do |i|
      @ncols.times do |j|
        self[i,j] = yield self[i,j]
      end
    end
    return self
  end
  
  def each_col
    raise ArgumentError, "Need a block" unless block_given?
    @ncols.times do |j|
      yield self.col(j), j
    end
  end
  
  def each_row
    raise ArgumentError, "Need a block" unless block_given?
    @nrows.times do |i|
      yield self.row(i), i
    end
  end
  
  def lu
    return LUDecomp.new self
  end
  
  def det
    return LUDecomp.new(self).det
  end
  
  def inv
    return LUDecomp.new(self).inv
  end
  
  def inspect
    "M#{self.to_a}"
  end
  
  def to_s
    lines = []
    mask = "⎜ #{(@format + ' ') * @ncols}⎟"
    self.each_row do |r|
      lines << (mask % r.to_a)
    end
    return "⎡#{' ' * (lines[0].length-6)}⎤\n" + lines.join("\n") + "\n⎣#{' ' * (lines[0].length-6)}⎦\n"
  end
end

