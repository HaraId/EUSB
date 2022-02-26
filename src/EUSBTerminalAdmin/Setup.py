import os
from distutils.core import setup, Extension

def main():
    setup(name="eusb",
          version="1.0.0",
          description="Python interface for the fputs C library function",
          author="Harald",
          author_email="...@gmail.com",
          ext_modules=[Extension("eusb", ["PyEUSB.cpp"])])

if __name__ == "__main__":
    main()
