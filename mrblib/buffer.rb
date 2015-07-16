#*************************************************************************#
#                                                                         #
# buffer.rb - Circular buffer class for mruby                             #
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

class Buffer < Vector
  attr_reader :head
  
  def initialize(args)
    super args
    @head = 0
  end
  
  def <<(v)
    raise ArgumentError unless v.respond_to? :to_f
    self[@head] = v.to_f
    @head += 1
    @head = 0 if @head >= self.size
    return v
  end
  
  def [](i)
    i += @head
    i -= self.size if i >= self.size
    i += self.size if i < 0
    super i
  end
  
  def each
    0.upto(self.size-1) {|i| yield self[i]}
  end
  
end