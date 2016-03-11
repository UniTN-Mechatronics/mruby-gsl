MRuby::Gem::Specification.new('mruby-gsl') do |spec|
  # Note that it needs WiringPI libraries (http://wiringpi.com)
  spec.license = 'MIT'
  spec.author  = 'Paolo Bosetti, University of Trento'
  spec.summary = 'Linear Algebra for mruby, based on GSL'
  spec.version = 0.1
  spec.description = spec.summary
  spec.homepage = "Not yet defined"
  
  if not build.kind_of? MRuby::CrossBuild then
    spec.cc.command = 'gcc' # clang does not work!
    spec.cc.flags << %w|-DGSL_ERROR_MSG_PRINTOUT|
    spec.cc.include_paths << "/usr/local/include"
    spec.linker.library_paths << "/usr/local/lib"
    spec.linker.libraries << %w|gsl gslcblas|
  else
    # complete for your case scenario
    spec.cc.flags << %w|-DGSL_ERROR_MSG_PRINTOUT|
    spec.linker.libraries << %w|gsl gslcblas|
  end
end
