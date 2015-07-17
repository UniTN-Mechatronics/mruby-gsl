#*************************************************************************#
#                                                                         #
# vector.rb - Vector class for mruby                                      #
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

class Vector
  include Enumerable
  include Comparable
  attr_reader :length
  attr_accessor :format
  
  def self.[](*ary)
    raise ArgumentError unless ary.kind_of? Array
    v = Vector.new(ary.size)
    ary.each_with_index {|e,i| v[i] = e.to_f}
    return v
  end
  
  alias :size :length
  
  def +(o); return self.dup.add! o; end
  def -(o); return self.dup.sub! o; end
  def *(o); return self.dup.mul! o; end
  def /(o); return self.dup.div! o; end
  
  def t; return self.to_mat.t; end
  
  def to_mat
    m = Matrix.new(@length, 1)
    self.each_with_index {|e,i| m[i,0] = e}
    return m
  end
    
  def <=>(other)
    self.length <=> other.length
  end
  
  def each
    raise ArgumentError, "Need a block" unless block_given?
    self.length.times do |i|
      yield self[i]
    end
  end
  
  def median
    self.quantile
  end
  
  def inspect
    "V#{self.to_a}"
  end
  
  def to_s
    lines = []
    mask = "⎜ #{@format} ⎟"
    self.each do |e|
      lines << (mask % e)
    end
    return "⎡#{' ' * (lines[0].length-6)}⎤\n" + lines.join("\n") + "\n⎣#{' ' * (lines[0].length-6)}⎦\n"
  end
  
end

