#include <sstream>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string.h>
#include <time.h>
#include <ctime>
#include <map>
#include <vector>
#include <set>
#include <assert.h>

#include "parse_log.h"

#include "model.h"

int main(int argc, char** argv)
{
  Model_t M;
  std::map<uint32_t, Model_t> Models;
  std::map<uint32_t, Learn_t> Learns;

  std::ifstream rbd_log( argv[1] );
  if (!rbd_log.is_open()) {
    std::cerr << "Cannot open " << argv[1] << std::endl;
  }
  std::string line;
  std::set<std::string> objects;
  uint64_t count = 0;
  uint64_t countx = 0;
  while (getline(rbd_log, line))
  {
    uint32_t client_no;
    std::string object_name;
    struct op_io_t specifics;

    if ( !parse_line(line, client_no, object_name, specifics) )
	continue;
    //std::cout << line.substr(0,26) << std::endl;
    std::cout << std::fixed << std::setprecision(6) << specifics.tv <<
	" " << specifics.client_no << " " << specifics.object_name << " " << specifics.opcode << " " << specifics.offset << " " << specifics.len << "." << std::endl;

    Learns[client_no].learn_object_op(Models[client_no], specifics);
    //Models[client_no].r_w_print();
    count++;
    if (count > 90000) {
    if (objects.count(specifics.object_name) == 0) {
      countx++;
    }
    objects.insert(specifics.object_name);
    if (count%1000 == 0) {
      //std::cout << "# "<< double(objects.size()) / count << std::endl;
      std::cout << "# "<< countx << std::endl;
      countx = 0;
    }
    }
  }
  for (auto &l: Learns) {
    l.second.learn_end(Models[l.first]);
  }
  for (auto &x: Models)
  {
    std::cout << std::endl << x.first << std::endl;//": ";
    size_t len = x.second.get_length();
    if (len < 10)
      continue;
    x.second.r_w_print();
    continue;
    Generator_t g(x.second);
    for (int i=0; i < len; i++) {
      op_io_t specifics;
      g.get_op(specifics);
      std::cout << std::fixed << std::setprecision(6) << specifics.tv <<
  	" " << specifics.client_no << " " << specifics.object_name << " " <<
	specifics.opcode << " " << specifics.offset << " " << specifics.len << "." << std::endl;
      assert (specifics.offset <= Model_t::max_object_size);
      assert (specifics.len <= Model_t::max_object_size);

    }

  }
  /*
  while (xxx-- > 0)
  {
    if (M.r_w_model() == op_read)
      std::cout << "R" << std::endl;
    else
      std::cout << "W" << std::endl;
  }
  */
}
