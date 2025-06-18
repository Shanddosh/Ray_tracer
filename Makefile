CXX = g++

main:
  $(CXX) main.cpp -o out

render: 
  ./out
  convert output.ppm output.png

clean: 
  rm out
  rm output.ppm
