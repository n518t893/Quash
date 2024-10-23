/* @file quash.c 
 * Authors: Liv Sutton and Chloe Tran
 * 
 * This file contains the heart of the Quash shell, handling the main 
 * execution logic. It sets up the shell state, manages user input, 
 * and runs commands in a loop until the shell is told to quit.
 */


// Included Files
#include "quash.h" // Main shell header for structure definitions

#include <limits.h> // For PATH_MAX and other limits
#include <stdbool.h> // For boolean types
#include <string.h> // For string manipulation functions
#include <unistd.h> // For POSIX API like isatty
#include <stdio.h> // For standard I/O operations

#include "command.h" // Header for command structures
#include "execute.h" // Header for execution functions
#include "parsing_interface.h" // Header for parsing commands
#include "memory_pool.h" // Header for memory management


// Private Variables 
static QuashState state; // This keeps track of the shell's current status

// Private Functions

// Initialize the shell state with defaults
static QuashState initial_state() {
  return (QuashState) {
    true
    isatty(STDIN_FILENO),  // Check if we're interacting with a terminal
    NULL   // Placeholder for the command string
  };
}

// Display a prompt for the user to enter a command
static void print_prompt() {
  bool should_free = true;
  char* cwd = get_current_directory(&should_free); // Grab current working directory
  assert(cwd != NULL); // Ensure we successfully got the directory
  char* last_dir = cwd; // Start with the full cwd

  // Find the last directory in the path
  for (int i = 0; cwd[i] != '\0'; ++i) {
    if (cwd[i] == '/' && cwd[i + 1] != '\0') {
      last_dir = cwd + i + 1; // Update to point just after the last slash
    }
  }

  // Print the prompt showing the last directory
  printf("[QUASH - @%s]$ ", last_dir);
  fflush(stdout); // Make sure it shows up right away
  if (should_free)
    free(cwd); // Clean up the allocated cwd memory
}

//Public Functions

bool is_running() {
  return state.running; 
}

// Create a copy of the current command string
char* get_command_string() {
  return strdup(state.parsed_str); // Duplicate the string for safety
}

// Check if we're reading input from a terminal
bool is_tty() {
  return state.is_a_tty; // Just return the status
}

// Stop the main loop of Quash
void end_main_loop() {
  state.running = false; 
}

/**
 * @brief Quash entry point
 *
 * This is where the magic happens! It initializes the Quash shell, 
 * greets the user, and enters a loop to handle commands until 
 * the shell is closed.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 *
 * @return Program exit status
 */
int main(int argc, char** argv) {
  state = initial_state(); // Get our shell state ready

  // If we're in a terminal, print a welcome message
  if (is_tty()) {
    puts("Welcome to Quash!"); // Friendly greeting
    puts("Type \"exit\" or \"quit\" to quit"); // Instructions for quitting
    puts("---------------------------------"); // Divider
    fflush(stdout); // Show everything now
  }

  // Set up cleanup actions for when we exit
  atexit(destroy_parser); // Free the parser resources
  atexit(destroy_memory_pool); // Free the memory pool

  // Main loop for running commands
  while (is_running()) {
    if (is_tty())
      print_prompt(); // Show the command prompt if in terminal

    initialize_memory_pool(1024); // Set up a fresh memory pool for command parsing
    CommandHolder* script = parse(&state); // Parse the input commands

    if (script != NULL)
      run_script(script); // If we got valid commands, execute them

    destroy_memory_pool(); // Clean up the memory pool after execution
  }

  return EXIT_SUCCESS; // Everything went fine, exit successfully
}
