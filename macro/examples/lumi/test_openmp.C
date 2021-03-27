#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string>
#include <vector>
#include <iostream>
#define MAX 10
//g++ -o test_openmp testopenmp.C -fopenmp `root-config --cflags --libs`
int main() {
   int count = 0;
   std::vector<std::string> vec{"\\\"/home/deliner/work/data/lumi/pass1_tmp2/2016\\\"","\\\"/home/deliner/work/data/lumi/pass1_tmp2/2016_2\\\""};
/*   #pragma omp parallel num_threads(MAX)
   {
      #pragma omp atomic
      count++;
   }
*/
  #pragma omp parallel for num_threads(3)
  for(int i=0;i<vec.size();i++) {
    std::cout<<std::endl<<i<<std::endl;
    
    std::string com= "aliroot -b -q .x taskGrid.C\\("+vec[i]+"\\)";
    std::cout<<com<<std::endl;
    system(com.c_str());
  }
  return 0;
}