#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <sstream>
int debug = 100;

int read_memory(int write_fd, int read_fd, int location) {
   std::string write_packet;
   write_packet = "0 " + std::to_string(location) + "\n";
   write(write_fd, &write_packet, write_packet.size());
   char read_char;
   std::string read_packet;
   while (true) {
      read(read_fd, &read_char, 1);
      if (read_char == '\n') {
         break;
      }
      read_packet += read_char;
   }
   std::istringstream read_stream(read_packet);
   std::string temp;
   read_stream >> temp;
   if (std::stoi(temp) == 0) {
      printf("failed read");
   }
   else {
      read_stream >> temp;
   }
   return std::stoi(temp);
}

void write_memory(int write_fd, int read_fd, int location, int data) {
   std::string write_packet;
   write_packet = "1 " + std::to_string(location) + " " + std::to_string(data) + "\n";
   write(write_fd, &write_packet, write_packet.size());
   char read_char;
   std::string read_packet;
   while (true) {
      read(read_fd, &read_char, 1);
      if (read_char == '\n') {
         break;
      }
      read_packet += read_char;
   }
   std::istringstream read_stream(read_packet);
   std::string temp;
   read_stream >> temp;
   if (std::stoi(temp) == 0) {
      printf("failed write");
   }
}

void exit_memory(int write_fd) {
   std::string write_packet = "2";
   write(write_fd, &write_packet, write_packet.size());
}


void spawn_memory(int &write_pipe, int &read_pipe, const std::string& file_name) {
   int pipe_fd_CPU[2], pipe_fd_memory[2];
   int pipe_status_CPU, pipe_status_memory;
   pipe_status_CPU = pipe(pipe_fd_CPU);
   if (pipe_status_CPU != 0) {
      printf("Failed CPU pipe");
      exit(1);
   }
   pipe_status_memory = pipe(pipe_fd_memory);
   if (pipe_status_memory != 0) {
      printf("Failed memory pipe");
      exit(1);
   }

   int pid = fork();
   if (pid == 0) {
      //Memory
      // making cin and cout the read and write pipes
      close(0);
      dup2(pipe_fd_memory[0], 0);
      close(1);
      dup2(pipe_fd_CPU[1], 1);
      close(pipe_fd_CPU[0]);
      close(pipe_fd_memory[1]);
      int exec_return = execl("memory", "memory", file_name.c_str(), NULL);
      // should never reach
      std::cerr << "failed to start memory subsystem. exec return = " << exec_return << ", errno = "
                << errno << std::endl;
      exit(1);
   }
   close(pipe_fd_CPU[1]);
   close(pipe_fd_memory[0]);
   write_pipe = pipe_fd_memory[1];
   read_pipe = pipe_fd_CPU[0];
}

void cpu_instructions(int write_fd, int read_fd) {
   bool interrupt_disable = false;
   int PC = 0;
   int IR = 0;
   int AC = 0;
   int X = 0;
   int Y = 0;
   int SP = 999;
   std::string memory_request;
   while (true) {
      IR = read_memory(write_fd, read_fd, PC);
      if(debug > 10){
         std::cerr <<"PC = " <<  PC << ", IR = " << IR << ", AC = " << AC << ", X = " << X << ", Y = " << Y << ", SP = " << SP << std::endl;
      }
      if(X > 25){

      }
      switch (IR) {
         case 1:
            PC++;
            AC = read_memory(write_fd, read_fd, PC);
            break;

         case 2: {

            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            AC = read_memory(write_fd, read_fd, addr);
            break;

         }
         case 3: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            int indirect_addr = read_memory(write_fd, read_fd, addr);
            AC = read_memory(write_fd, read_fd, indirect_addr);
            break;
         }

         case 4: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            AC = read_memory(write_fd, read_fd, addr + X);
            break;
         }

         case 5: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            int indirect_addr = read_memory(write_fd, read_fd, addr + Y);
            AC = read_memory(write_fd, read_fd, indirect_addr);
            break;
         }

         case 6: {
            int SP_addr = read_memory(write_fd, read_fd, SP);
            AC = read_memory(write_fd, read_fd, SP_addr + X);
            break;
         }

         case 7: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            write_memory(write_fd, read_fd, addr, AC);
            break;
         }

         case 8: {
            AC = (rand() % 100) + 1;
            break;
         }

         case 9: {
            PC++;
            int port = read_memory(write_fd, read_fd, PC);
            if (port == 1) {
               std::cout << AC;
            }
            else {
               std::cout << char(AC);
            }
            std::cout.flush();
            break;
         }

         case 10: {
            AC += X;
            break;
         }

         case 11: {
            AC += Y;
            break;
         }

         case 12: {
            AC -= X;
            break;
         }

         case 13: {
            AC -= Y;
            break;
         }

         case 14: {
            X = AC;
            break;
         }

         case 15: {
            AC = X;
            break;
         }

         case 16: {
            Y = AC;
            break;
         }

         case 17: {
            AC = Y;
            break;
         }

         case 18: {
            SP = AC;
            break;
         }

         case 19: {
            AC = SP;
            break;
         }

         case 20: {
            PC++;
            PC = read_memory(write_fd, read_fd, PC);
            continue;
         }

         case 21: {
            PC++;
            if (AC == 0) {
               PC = read_memory(write_fd, read_fd, PC);
               continue;
            }
            break;
         }

         case 22: {
            PC++;
            if (AC != 0) {
               PC = read_memory(write_fd, read_fd, PC);
               continue;
            }
            break;
         }

         case 23: {
            PC++;
            int addr = read_memory(write_fd, read_fd, PC);
            PC++;
            write_memory(write_fd, read_fd, SP, PC);
            PC = addr;
            SP--;
            continue;
         }

         case 24: {
            SP++;
            PC = read_memory(write_fd, read_fd, SP);
            break;
         }

         case 25: {
            X++;
            break;
         }

         case 26: {
            X--;
            break;
         }

         case 27: {
            write_memory(write_fd, read_fd, SP, AC);
            SP--;
            break;
         }

         case 28: {
            SP++;
            AC = read_memory(write_fd, read_fd, SP);
            break;
         }

         case 29: {
            int SP_user = SP;
            SP = 2000;
            write_memory(write_fd, read_fd, SP, SP_user);
            SP--;
            write_memory(write_fd, read_fd, SP, PC);
            PC = 1500;
            interrupt_disable = true;
            break;
         }

         case 30: {

            PC = read_memory(write_fd, read_fd, SP);
            SP++;
            SP = read_memory(write_fd, read_fd, SP);
            interrupt_disable = false;
            break;
         }

         case 50: {
            exit_memory(write_fd);
            exit(1);
         }
      }
      PC++;

   }
}

int main(int argc, char *argv[]) {

   // get arguments
   std::string file_name = argv[1];
   //int timer = std::stoi(argv[1]);

   int write_fd;
   int read_fd;
   spawn_memory(write_fd, read_fd, file_name);
   cpu_instructions(write_fd, read_fd);


   //process
   //CPU

/*
// testing readfile
int memory[2000];
std::string file_name = "sample1.txt";
readFile(file_name, memory);
*/

   return 0;

}


