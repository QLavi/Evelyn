@echo off

glslangValidator -V -o test.vert.spv test.vert
glslangValidator -V -o test.frag.spv test.frag
