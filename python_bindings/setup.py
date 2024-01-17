from distutils.core import setup, Extension

hep_evd =  Extension(
    'hep_evd',
    sources = ['hep_evd_bindings.cpp'],
    include_dirs=['../'],
    language='c++',
    extra_compile_args=['-std=c++17'],
)
setup (ext_modules=[hep_evd])