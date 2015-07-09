#*************************************************************************#
#                                                                         #
# raspberry.rb - mruby gem provoding access to Raspberry Pi IOs           #
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
  
  def self.[](*ary)
    raise ArgumentError unless ary.kind_of? Array
    v = Vector.new(ary.size)
    ary.each_with_index {|e,i| v[i] = e.to_f}
    return v
  end
  
  alias :- :sub
  alias :/ :div
  alias :size :length
  
  def *(other)
    case other
    when self.class
      self.mul(other)
    when Numeric
      self.scale(other)
    else
      raise ArgmentError, "Only Vectors or Numerics"
    end
  end
  
  def +(other)
    case other
    when self.class
      self.add(other)
    when Numeric
      self.add_scalar(other)
    else
      raise ArgmentError, "Only Vectors or Numerics"
    end
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
  
  def inspect
    "V#{self.to_a}"
  end
end
