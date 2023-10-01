#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>

// debug tool
int debug = 1;

// debug tool
void print_memory(int memory[]){
   for(int i = 0; i < 2000; i++){
      if(i % 10 == 0){
         std::cerr << i << ": ";
      }
      std::cerr << memory[i] << ", ";
      if((i + 1) % 10 == 0){
         std::cerr << std::endl;
      }
   }
}

// parses file and puts it into memory
void readFile(const std::string &file_name, int memory[]) {
   // debug tool
   if (debug > 10) {
      std::cerr << "Reading file '" << file_name << "'" << std::endl;
   }
   // makes the file into a stream
   std::ifstream file(file_name);
   if (!file.is_open()) {
      std::cerr << "Failed to open File" << std::endl;
      return;
   }
   // reads file line by line
   int index = 0;
   std::string line;
   while (getline(file, line)) {
      std::string opcode;
      // check for memory position change
      if (line[0] == '.') {
         // check for comments after
         int x = line.find(' ');
         if (line.find(' ') > -1) {
            index = std::stoi(line.substr(1, line.find(' ')));
         }
         else {
            index = std::stoi(line.substr(1, line.size()));
         }
         continue;
      }
         // check for empty line or if there is no content
      else if (line.empty() || line[0] == ' ') {
         continue;
      }
         // should be data or opcode at this point and will save it off into memory and ignore
      else if (line.find(' ')) {
         opcode = line.substr(0, line.find(' '));
         //memory location
         memory[index] = std::stoi(opcode);
      }
      else {
         memory[index] = std::stoi(line);
      }
      index++;
   }
   file.close();
}

int main(int argc, char *argv[]) {
   if(argc < 2){
      printf("no argument");
      exit(1);
   }
   std::string file_name = argv[1];
   int memory[2000];
   // set memory to 0s
   std::memset(memory, 0, sizeof(memory));
   readFile(file_name, memory);
   // debug tool
   if(debug > 200){
      print_memory(memory);
   }
   std::string input_string;
   // debug tool
   if(debug > 100)
   {
      std::cerr << "starting getline loop" << std::endl;
   }
   // packets come as one line, and it gets a whole line at a time
   while (getline(std::cin, input_string)) {
      // debug tool
      if(debug > 100){
         std::cerr << "getting new line = " << input_string << std::endl;
      }

      // getting type of operation and memory location
      std::string output;
      std::istringstream input_stream(input_string);
      std::string temp;
      input_stream >> temp;
      int operation = std::stoi(temp);
      input_stream >> temp;
      int location = std::stoi(temp);

      // exit case
      if(operation == 2){
         if(debug > 10){
            std::cerr << "exit processed" << std::endl;
         }
         exit(0);
      }
      // checking for useless line
      else if (location < 0 || location > 1999) {
         output = "0";
      }

      // 0 is a read and 1 is a write operation
      // read
      else if (operation == 0) {
         // formatting line
         output = "1 " + std::to_string(memory[location]);
         if(debug > 10){
            std::cerr << "reading location - "<< location << " data - " << memory[location] << std::endl;
         }
      }
      // write
      else if (operation == 1) {
         // getting data
         input_stream >> temp;
         memory[location] = std::stoi(temp);
         output = "1 " + temp;
         if(debug > 10){
            std::cerr << "writing location - " << location << " data - " << std::stoi(temp) << std::endl;
         }
      }
      // failure
      else {
         output = "0";
      }
      // debug tool
      if(debug > 100){
         std::cerr << "sending response = " << output << std::endl;
      }
      std::cout << output << std::endl;
   }
   return 0;
}