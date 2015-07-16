assert('Vector#to_a') do
  ary = [1,2,3]
  vec = Vector[*ary]
  assert_equal(ary) { vec.to_a }
end

assert('Vector#[]') do
  vec = Vector[1,2,3]
  assert_equal(2) { vec[1] }
end

assert('Vector#sum') do
  vec = Vector[1,2,3]
  assert_equal(6) { vec.sum }
end

assert('Vector#add!') do
  v1 = Vector[1,2,3]
  v2 = Vector[3,2,1]
  v1.add! v2
  result = (v1 === Vector[4,4,4])
  assert_true(result)
end

assert('Vector#^') do
  v1 = Vector[1,2,3]
  v2 = Vector[3,2,1]
  assert_equal(10) {v1 ^ v2}
end

assert('Vector#*') do
  v1 = Vector[1,2,3]
  v2 = Vector[3,2,1]
  assert_equal(Vector[3,4,3]) {v1 * v2}
end

assert('Vector#each_with_index') do
  ary = [1,2,3]
  vec = Vector[*ary]
  vec.each_with_index do |e,i|
    assert_equal(ary[i]) { e }
  end
end


assert('Matrix#^') do
  m1 = Matrix[[1,2,3],[4,5,6]]
  m2 = Matrix[[1,2],[3,4],[5,6]]
  v = Vector[1,2,3]
  assert_equal(m1 ^ v) { Vector[14, 32]}
  assert_true((m1 ^ m2) === Matrix[[22, 28], [49, 64]])
end

assert('LUDecomp#inv') do
  m1 = Matrix[[1,2],[4,5]]
  a = [-5/3,2/3,4/3,-1/3]
  inv = m1.lu.inv
  inv.each_with_index do |e,i|
    assert_true((a[i] - e) < 1E-9)
  end
end
