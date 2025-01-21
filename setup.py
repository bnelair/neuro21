from setuptools import setup, find_packages
from pybind11.setup_helpers import Pybind11Extension, build_ext

import pybind11

def get_requirements():
    with open("requirements.txt", "r") as f:
        return f.read().splitlines()

# Define the C++ extension
ext_modules = [
    Pybind11Extension(
        "pyneuro21.cpp.ccompression",
        sources=[
            "neuro21_cpp/src/gorilla_compression.cpp",  # Corrected file extension
            "pyneuro21/cpp/ccompression.cpp"    # Corrected file extension
        ],
        include_dirs=[pybind11.get_include(), "neuro21_cpp/compression"],  # Path to header files

        extra_compile_args=["-std=c++17"],  # C++17 standard
    ),
]

# Setup configuration
setup(
    name="pyneuro21",
    version="0.0.1",
    author="Your Name",
    author_email="your.email@example.com",
    description="A package for EEG data processing",
    long_description="README.md",
    long_description_content_type="text/markdown",
    packages=find_packages(include=["pyneuro21", "pyneuro21.*"]),  # Only include the Python package
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.7",
    install_requires=["numpy", "pybind11"] + get_requirements(),
    extras_require={"test": ["pytest"]},
)
