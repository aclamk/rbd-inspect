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
#include <memory>
#include "parse_log.h"

#include "model.h"

void print_help() {
  static const char help[] =
"Usage: gen-iolog [OPTION]...\n"
"Generate sequence of operations to simulate RBD.\n"
"Output is a iolog file that can be read by FIO's --read_iolog option.\n"
"\n"
" -m|--model FILE         Read rbd operation models from FILE.\n"
" -p|--operations CNT     Generate CNT number of I/O operations\n"
" -o FILE                 Write output to FILE\n"
" -b|--objects CNT        Amount of objects that are operated on.\n"
" -h, --help              Help\n";
  std::cout << help << std::endl;
}

std::vector<Player_t> all_players;

bool load_models(const std::string& models)
{
	bool result = true;
	FILE* f;
	f = fopen("test-record", "r");
	while (true)
	{
		int ch = fgetc(f);
		if (feof(f)) break;
		ungetc(ch, f);
		Player_t p;
		if (!p.load(f)) {
			std::cerr << "Failed to load models from '" << models << "'" << std::endl;
			result = false;
		}
		all_players.push_back(p);
	}
	fclose(f);
	return result;
}

bool generate_distribution(std::map<double, Player_t& >& distribution)
{
  //create probability matrix to select player
  uint32_t max_length = 0;
  uint64_t total_length = 0;
  for (auto& x: all_players)
  {
    uint32_t length = x.get_length();
    if (max_length < length)
      max_length = length;
    total_length += length;
  }
  double sump = 0;
  for (auto& x: all_players)
  {
    double w = 0;
    uint32_t length = x.get_length();
    w = (double)max_length / length;
    sump += w;
  }

  double c = 0;
  for (auto& x: all_players)
  {
    double w = 0;
    uint32_t length = x.get_length();
    w = (double)max_length / length / sump;
    distribution.emplace(c, x);
    c += w;
  }
  return true;
}

bool generate_iolog(const std::string& output_name,
					size_t operation_count,
					size_t max_object_count)
{
  std::map<double, Player_t& > distribution;
	std::multimap<int64_t, std::shared_ptr<Playback_t> > active_players;

	std::ofstream foutput;
	std::ostream* output;
	if (output_name == "") {
		output = &std::cout;
	} else {
		foutput.open(output_name.c_str(), std::ofstream::binary);
		if (!foutput.good()) {
			std::cerr << "Cannot open '" << output_name << "' for writing" << std::endl;
			return false;
		}
		output = &foutput;
	}

  generate_distribution(distribution);

	*output << "fio version 2 iolog" << std::endl;

	Playback_objects_t object_pool("rbd_data.____.");

	std::string commands;
	Player_t *next_player = nullptr;
  uint64_t next_time = 0;
	bool infinite = operation_count == 0;
	do
	{
    if (!next_player) {
      double v = (double)rand() / RAND_MAX;
      next_player = &(distribution.lower_bound(v)->second);
    }

    if (object_pool.names_count() + next_player->get_object_count() <= max_object_count) {
      next_time = active_players.size() ? active_players.begin()->first : 0;
      std::shared_ptr<Playback_t> pb(new Playback_t(*next_player, object_pool, next_time));
      next_player = nullptr;
      uint64_t next_op;
      pb->blktrace_get_next_time(next_op);
      active_players.emplace(next_op, std::shared_ptr<Playback_t>(pb));

      pb->blktrace_open(commands);
      *output << commands;
    }

	  auto first = active_players.begin();
	  if (first != active_players.end()) {
	    assert(first->second->blktrace_get_commands(commands));
	    *output << commands;
	    if (first->second->blktrace_get_next_time(next_time)) {
	      active_players.emplace(next_time, first->second);
	    }
	    active_players.erase(first);
	  }
	} while (infinite || --operation_count > 0);

	//close on finish
	for (auto &x:active_players) {
	  std::string commands;
	  x.second->blktrace_close(commands);
	  *output << commands;
	}

	foutput.close();
	return true;
}



int main(int argc, char** argv)
{
  Model_t M;
  std::map<uint32_t, Model_t> Models;
  std::map<uint32_t, Learn_t> Learns;
  std::map<uint32_t, Recorder_t> Recorders;

  std::multimap<int64_t, std::shared_ptr<Playback_t> > active_players;
  char* endp;
  std::string models = "";
  size_t object_count = 0;
  size_t operation_count = 0;
  std::string output_name;

  argc--; argv++;
  std::set<std::string> oneparam_args={"-m", "--model", "-b", "--objects", "-p", "--operations", "-o"};
  while (argc >= 1)
  {
	  std::string arg = *argv;
	  argc--; argv++;

	  if (oneparam_args.count(arg) > 0) {
		  //one param argument
		  if (argc < 1) {
			  std::cerr << "Option " << arg << " requires parameter" << std::endl;
			  exit(-1);
		  }
		  std::string value = *argv;
		  if (arg == "-m" || arg == "--model") {
			  models = value;
		  }
		  if (arg == "-b" || arg == "--objects") {
			  object_count = strtol(*argv, &endp, 0);
			  if (*endp != '\0') {
				  std::cerr << "Option " << arg << " cannot accept `" << value << "'" << std::endl;
				  exit(-1);
			  }
		  }
		  if (arg == "-p" || arg == "--operations") {
			  operation_count = strtol(*argv, &endp, 0);
			  if (*endp != '\0') {
				  std::cerr << "Option " << arg << " cannot accept `" << value << "'" << std::endl;
				  exit(-1);
			  }
		  }
		  if (arg == "-o") {
			  output_name = value;
		  }
	      argc--; argv++;
		  continue;
	  }
	  if (arg == "-h" || arg == "--help") {
		  print_help();
		  return 0;
	  }
	  std::cerr << "Unknown option '" << arg << "'" << std::endl;
	  print_help();
	  return 1;
  }

  if (models == "") {
	  std::cerr << "No model supplied." << std::endl;
	  print_help();
	  return 1;
  }

  if (!load_models(models))
	  return 1;

  if (!generate_iolog(output_name, operation_count, object_count))
    return 1;

}
