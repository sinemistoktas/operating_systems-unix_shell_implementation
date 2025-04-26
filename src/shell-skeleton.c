#include <errno.h>
#include <stdbool.h>
#include <stdio.h>  // printf(), fgets()
#include <stdlib.h> // free()
#include <string.h> // strtok(), strcmp(), strdup()
#include <sys/wait.h> // waitpid()
#include <termios.h> // termios, TCSANOW, ECHO, ICANON
#include <unistd.h> // POSIX API: fork()
#include <dirent.h> // For directory operations
#define MAX_MATCHES 128 // For the auto-complete functionality.
#include <fcntl.h> // for open()
#define FILE_MODE (S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH) // Create file permissions 

const char *sysname = "ˢˡᵃsh"; 
char autocomplete_buf[512] = {0};

// TODO : Fill below
const int groupSize = 2; // TODO: change to 2 if you are two people
const char *student1Name = "Yamaç Ömür";
const char *student1Id = "0079458";
const char *student2Name = "Sinemis Toktaş";
const char *student2Id = "0076644";



typedef struct cmd_t {
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;  //  how many arguments are there
	char **args;  // pointer to char pointers for each argument
	char *redirects[3]; // stdin/stdout to/from file
	struct cmd_t *next; // for piping
} cmd_t;


void process_command(cmd_t *cmd);

// Release allocated memory for a command structure
void free_command(cmd_t *cmd) {
    // if arg count is greater than 0 free command args
	if (cmd->arg_count) {
		for (int i = 0; i < cmd->arg_count; ++i)
			free(cmd->args[i]);
		free(cmd->args);
	}

    // free if redirects are not null
	for (int i = 0; i < 3; ++i) {
		if (cmd->redirects[i])
			free(cmd->redirects[i]);
	}

    // free if next command in pipe chain is not null
	if (cmd->next) {
		free_command(cmd->next);
		cmd->next = NULL;
	}

    // free the char array for name
	free(cmd->name);
	free(cmd);
}



// Show the command prompt
void show_prompt() {
	char cwd[512], hostname[512];
	gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("👤%s 💻%s 📂%s %s ╰┈➤ ", getenv("USER"), hostname, cwd, sysname);
}


// Parse a command string into a command struct
void parse_command(char *buf, cmd_t *cmd) {
	const char *splitters = " \t"; // split at space and tab -> define space and tab as seperators
	int index, len;
	len = strlen(buf); // get length of input string

	// trim left whitespace
	while (len > 0 && strchr(splitters, buf[0]) != NULL) {
		buf++;
		len--;
	}

    // trim right whitespace
	while (len > 0 && strchr(splitters, buf[len - 1]) != NULL)
        buf[--len] = 0;

	// marked for auto-complete
	if (len > 0 && buf[len - 1] == '?') cmd->auto_complete = true;	// if command ends with ? mark it for auto-complete

	// background execution
	if (len > 0 && buf[len - 1] == '&')	cmd->background = true;	// if command ends with & mark it for background-execution

	// parse command name
	char *pch = strtok(buf, splitters); // get the first token
	if (pch == NULL) { // if token is empty create an empty name
		cmd->name = (char *)malloc(1);
		cmd->name[0] = 0;
	} else { // alloc memory and copy the command name
		cmd->name = (char *)malloc(strlen(pch) + 1);
		strcpy(cmd->name, pch);
	}

	cmd->args = (char **)malloc(sizeof(char *)); // init arguments array

	int redirect_index; // for tracking redirections
	int arg_index = 0; // for tracking argument indices
	char temp_buf[1024], *arg; // for temporary buffers-> temp_buf: temp space to hold a word, arg: pointer that will point inside temp_buf

	while (1) { // iteratively continue tokenizing 
		// tokenize input on splitters
		pch = strtok(NULL, splitters); // NULL tells strtok to continue from where you left off
		if (!pch) break; // breaks when there are no more token

		arg = temp_buf; 
		strcpy(arg, pch); // copy token to temporary buffer
		len = strlen(arg);

		// trim whitespaces from both ends of the token
		// trim left whitespace
		while (len > 0 && strchr(splitters, arg[0]) != NULL) { 
			arg++;
			len--;
		}

		// trim right whitespace
		while (len > 0 && strchr(splitters, arg[len - 1]) != NULL) 
			arg[--len] = 0;
		

		// empty arg, go for next
		if (len == 0) continue; // skips empty argument
		

		// pipe handling -> if the argument is a pipe symbol "|", recursively parse the remaining command 
		// mark for piping to another command
		if (strcmp(arg, "|") == 0) { 
            cmd_t *c = malloc(sizeof( cmd_t)); // create a new command structure for the command after pipe
			int l = strlen(pch);
			pch[l] = splitters[0]; // restore strtok termination
			index = 1;
			while (pch[index] == ' ' || pch[index] == '\t')
				index++; // skip whitespaces

			parse_command(pch + index, c);
			pch[l] = 0; // put back strtok termination
			cmd->next = c; // link c to current command
			continue;
		}

		// background process handling
		// background process; already marked in cmd
		if (strcmp(arg, "&") == 0) continue;
		// if the argument is "&", it is already marked earlier, skip it
		

		// I/O redirection handling
		// mark IO redirection
		redirect_index = -1;
		if (arg[0] == '<') {
			redirect_index = 0; // 0 -> take input from file <
		}

		if (arg[0] == '>') {
			printf("arg: %s\n", arg+1); // debug
			if (len > 1 && arg[1] == '>') {
				redirect_index = 2; // 2 -> append output to file >>
				arg++;
				len--;
			} else {
				redirect_index = 1; // 1 -> write output to file >
			}
		}

		if (redirect_index != -1) {
			// Check if filename is included in arg -> example: >out.txt
			if (len>1) {
				cmd->redirects[redirect_index] = malloc(len);
				strcpy(cmd->redirects[redirect_index], arg + 1); // store the filename that follows the redirection symbol
			} 
			else { // for cases when there is space after redirection symbol -> example: > out.txt
				pch = strtok(NULL, splitters); // parse again to get the filename
				
				if (pch == NULL) {
					printf("ERROR! : No filename provided after redirection symbol\n");
					break;
				}

				cmd->redirects[redirect_index] = malloc(strlen(pch) + 1);
				strcpy(cmd->redirects[redirect_index], pch); // store the filename that follows the redirection symbol

			}
			
			continue;
		}

		// normal arguments
		if (len > 2 &&
			((arg[0] == '"' && arg[len - 1] == '"') ||
			 (arg[0] == '\'' && arg[len - 1] == '\''))) // quote wrapped arg
		{
			// gets rid of quotes from start and end
			arg[--len] = 0; 
			arg++;
		}

		// store normal arguments
		cmd->args = (char **)realloc(cmd->args, sizeof(char *) * (arg_index + 1)); // reallocate arguments array to make room for the new argument

		cmd->args[arg_index] = (char *)malloc(len + 1); // allocate memory for the new argument
		strcpy(cmd->args[arg_index++], arg); // copy the new argument and increment the argument index
	}

	// finalize the arguments array
	cmd->arg_count = arg_index; // store number of arguments in the argument counter

    // first argument should be the name of the executable
    // last argument should be NULL to delimit the end

	// increase args size by 2
	cmd->args = (char **)realloc(cmd->args, sizeof(char *) * (cmd->arg_count += 2));

	// shift everything forward by 1
	for (int i = cmd->arg_count - 2; i > 0; --i)
		cmd->args[i] = cmd->args[i - 1];	

	// set args[0] as a copy of name
	cmd->args[0] = strdup(cmd->name);

	// set args[arg_count-1] (last) to NULL
	cmd->args[cmd->arg_count - 1] = NULL;
}



// Prompt a command from the user
void prompt( cmd_t *cmd) {
	size_t index = 0;
	char c;
	char buf[512];
	static char oldbuf[512]; // static variable persist through method calls, think like global variable 

    int escape_code_state = 0; // ANSI escape code sequence 1 -> ESC , 2 -> ESC + [

	// tcgetattr gets the parameters of the current terminal
	// STDIN_FILENO will tell tcgetattr that it should write the settings
	// of stdin to oldt
	static struct termios backup_termios, new_termios;
	tcgetattr(STDIN_FILENO, &backup_termios);
	new_termios = backup_termios;

	// ICANON : canonical mode: normally takes care that one line at a time will be processed
	// that means it will return if it sees a "\n" or an EOF or an EOL
	new_termios.c_lflag &= ~(ICANON | ECHO); // disable canonical mode and disable automatic echo

	// Those new settings will be set to STDIN
	// TCSANOW tells tcsetattr to change attributes immediately.

    // if you exit from the process or return from this function
    // without restoring the old settings, the whole terminal will
    // be broken for everybody (including the bash outside)
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	show_prompt();
	buf[0] = 0;

	while (1) {
		c = getchar();

		// tab
		if (c == 9) {
			buf[index++] = '?'; // autocomplete, we signal autocomplete by adding a question mark to the end
			break;
		}

		// backspace
		if (c == 127) {
			if (index > 0) {
				putchar(8); // go back 1
	            putchar(' '); // write empty over
            	putchar(8); // go back 1 again
				index--;
			}
			continue;
		}

        // Escap code handler : 27 91 is the escape code: ESC + [
        // see https://en.wikipedia.org/wiki/ANSI_escape_code
        if (c == 27) {
            escape_code_state = 1;
			continue;
		}

        if (escape_code_state == 1 && c == 91) {
            escape_code_state = 2;
			continue;
		}

        // handle ANSI terminal escape codes
        if (escape_code_state == 2) {

            // UP ARROW - currently it has a limited history function, limited to only 1 prior command
		    if (c == 65) {
                buf[index] = 0;
			    while (index > 0) { // delete everything
				    putchar(8); // go back 1
	                putchar(' '); // write empty over
                	putchar(8); // go back 1 again
				    index--;
			    }

			    char tmpbuf[512];	
			    strcpy(tmpbuf, buf); // swap, we can't just exchange pointers because oldbuf is static therefore not on the heap
			    strcpy(buf, oldbuf);
			    strcpy(oldbuf, tmpbuf);

                printf("%s", buf);           
			    index = strlen(buf);			    
		    }


            // DOWN ARROW - you might need this for the history feature
		    if (c == 66) {
                
            }

            escape_code_state = 0;
            continue;
        }

		putchar(c); // echo the typed character to screen

		buf[index++] = c; // add the character to buffer
		if (index >= sizeof(buf) - 1) break; // too long
		if (autocomplete_buf[0]) {
			
			while (index > 0) {
				putchar(8);
				putchar(' ');
				putchar(8);
				index--;
			}

			strcpy(buf, autocomplete_buf);
			printf("%s", buf);
			index = strlen(buf);

			autocomplete_buf[0] = '\0';
			continue;
		}


		if (c == '\n') // enter key
			break;

	}

	// trim newline from the end
	if (index > 0 && buf[index - 1] == '\n') index--;	

	buf[index++] = '\0'; // null terminate string

	strcpy(oldbuf, buf);

	parse_command(buf, cmd);

	// MUST restore the old settings
	tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
}


// Helper function to run a .sh file. It only takes the name
// of the .sh file as an argument, ASSUMING that the programmer has
// already checked that it was a shell file beforehand.

void run_shell_script(char* file_name) {
	char buffer[512];
	FILE *script_file = fopen(file_name, "r");
	
	if (script_file == NULL) {
		return;
	}
	
	// Code to iterate over the file line by line.
	while (fgets(buffer, sizeof(buffer), script_file)) {
		buffer[strcspn(buffer, "\n")] = 0; // Removes the first occurrence of a newline.
		// Here, I used the same logic as the main function. Each
		// line is treated as a command, so a command struct is created.
		// However, we don't want the terminal to ask for a user prompt,
		// so I simply parsed the line and turned it into a command,
		// processed the command, and freed it.
		cmd_t *cmd = malloc(sizeof( cmd_t));
		memset(cmd, 0, sizeof( cmd_t));
		parse_command(buffer, cmd);
		process_command(cmd);
		free_command(cmd);
	}
}

int main(int argc, char*argv[]) {

    // TODO: see the top of the source code
    // If the main function is provided with 2 arguments and the last
    // argument contains ".sh", then we should execute the provided shell
    // file. After execution, the terminal exits as instructed.
    if (argc == 2 && strstr(argv[1], ".sh") != NULL) {
	    run_shell_script(argv[1]);
	    return 0;
    }

    printf("\n%s Shell implemented by %s (%s)",sysname,student1Name,student1Id);
    if(groupSize>1) printf(" and %s (%s)",student2Name,student2Id);
    printf("\n");

	while (1) {
		cmd_t *cmd = malloc(sizeof( cmd_t));		
		memset(cmd, 0, sizeof( cmd_t));  // clear memory

		prompt(cmd);
        
		process_command(cmd);        

		free_command(cmd);
	}

	printf("\n");
	return 0;
}


void process_command( cmd_t *cmd) {

    // built-ins
	if (strcmp(cmd->name, "") == 0) return;
	if (strcmp(cmd->name, "exit") == 0)  exit(0);
	if (strcmp(cmd->name, "cd") == 0) {
		if (cmd->arg_count > 0) 
			if (chdir(cmd->args[1]) == -1)
                printf("- %s: %s  ---  %s\n", cmd->name, strerror(errno),cmd->args[1]);			            		
        return;
	}

    // TODO: implement other builtin commands here
    // do not forget to return from this method if cmd was a built-in and already processed
    // otherwise you will continue and fork!


    if (cmd->auto_complete){
        // TODO: think about commands marked as auto-complete, they will not be executed just completed
	if (cmd->name && strlen(cmd->name) > 0 && cmd->name[strlen(cmd->name) - 1] == '?') {
		cmd->name[strlen(cmd->name) - 1] = 0;
	}

        char *tab_path_env = getenv("PATH");
	if (tab_path_env == NULL) {
		printf("ERROR! : Your PATH environment variable was not set. Please do not try to use the autocomplete functionality.");
		return;
	}

	char *tab_path_copy = strdup(tab_path_env);
	char *tab_curr_directory = strtok(tab_path_copy, ":");
	char **matching_exes = malloc(sizeof(char *) * MAX_MATCHES);
	int match_count = 0;

	while (tab_curr_directory != NULL) {
		DIR *directory = opendir(tab_curr_directory);
		if (directory) {
			struct dirent *entry;
			while ((entry = readdir(directory)) != NULL) {
				char tab_full_path[512];
				if (strncmp(entry->d_name, cmd->name, strlen(cmd->name)) == 0) {
					bool already_in_array = false;
					for (int i = 0; i < match_count; i++) {
						if (strcmp(matching_exes[i], entry->d_name) == 0) {
							already_in_array = true;
							break;
						}
					}		
					snprintf(tab_full_path, sizeof(tab_full_path), "%s/%s", tab_curr_directory, entry->d_name);
					if (access(tab_full_path, X_OK) == 0 && !already_in_array) {
					//TODO: what do we do when we find an executable file with the same
					// name as the user's input command's name
						char command_full_path[512];
						snprintf(command_full_path, sizeof(command_full_path), "%s/%s", tab_curr_directory, cmd->name);
						if (strcmp(tab_full_path, command_full_path) == 0) {
							DIR *user_directory = opendir(".");
							if (user_directory) {
								printf("\n");
								struct dirent *dir_entry;
								while ((dir_entry = readdir(user_directory)) != NULL) {
									printf("%s\n", dir_entry->d_name);
									}
								closedir(user_directory);
								free(tab_path_copy);
								return;
							}
						}
						matching_exes[match_count] = malloc(strlen(entry->d_name) + 1);
						strcpy(matching_exes[match_count], entry->d_name);
						match_count++;

					}
				}
			}
		}
		closedir(directory);
		tab_curr_directory = strtok(NULL, ":");
	}

	free(tab_path_copy);

	printf("\n");
	
	if (match_count == 0) {
		printf("No matches found!\n");
		free(matching_exes);
		return;
	}

	if (match_count == 1) {
		snprintf(autocomplete_buf, sizeof(autocomplete_buf), "%s", matching_exes[0]);
		free(matching_exes[0]);
		free(matching_exes);
		return;
	}

	for (int i=0; i < match_count; i++) {
		printf("%s\n", matching_exes[i]);
	}

	fflush(stdout);

	for (int i=0; i < match_count; i++) {
		free(matching_exes[i]);
	}

	free(matching_exes);

        return;
    }

    if (cmd->next != NULL){
        // TODO: consider pipe chains
        printf("\nPipe chains not implemented yet!\n");
        return;
    }


    if (cmd->redirects[0] != NULL || cmd->redirects[1] != NULL || cmd->redirects[2] != NULL){
        // TODO: consider stdin/stdout redirects
        //printf("\nStdIn/StdOut redirection not implemented yet!\n");
        //return;

		// redirection is done after forking to protect parent(shell) code
		// after fork, we redirect stdout/stdin to a file in the child process 
    }


    // TODO: implement path resolution here, if you can't locate the
    // command print error message and return before forking!
    
    	// I call the "getenv" function here to find the "PATH" environment
	// variable. This will basically return the possible directories
	// for executable files, which I will manually check for the absolute path.
    	char *path_env = getenv("PATH");
	if (path_env == NULL) {
		printf("ERROR! : The PATH environment variable was not set.");
		return;
	}
	
	// Allocated space for a copy of the path environment, since I will be manipulating it.
	char* path_copy = strdup(path_env);
	// The path variable contains a list of possible execution paths delimited by ":". So, I tokenize
	// the string to access each path.
	char* curr_directory = strtok(path_copy, ":");
	bool path_found = false;
	char* path_to_execute;

	while (curr_directory != NULL) {
		char full_path[512];
		// I append the name of the command to the curr_directory string, resulting in a possible 
		// path of execution.
		snprintf(full_path, sizeof(full_path), "%s/%s", curr_directory, cmd->name);
		// The following function checks whether there is really a path which is executable
		// on that specified address. "X_OK" indicates "executable".
		if (access(full_path, X_OK) == 0) {
			path_found = true;
			path_to_execute = full_path;
			break; // I can terminate the loop now as the current "full_path" variable is correct.
		}

		curr_directory = strtok(NULL, ":"); // Keep tokenizing until we find a path.

	}
	
	if (!path_found) {
		printf("ERROR! : Couldn't locate the command you have requested.");
		free(path_copy);
		return;
	}

	free(path_copy); // Free the memory allocated copy.

    // Command is not a builtin then
	pid_t pid = fork();
	
	if (pid == 0) {
        // CHILD

        // This shows how to do exec with auto-path resolve
		// add a NULL argument to the end of args, and the name to the beginning
		// as required by exec

		if (cmd->redirects[0]){ // for the < case
			int fd = open(cmd->redirects[0], O_RDONLY); // Open the file for reading 
		 
			if (fd == -1){ // open() failed
				printf("ERROR! : Problem about input redirection (<)");
				exit(1);
			}

			dup2(fd, STDIN_FILENO); // redirect stdin to the file
			close(fd); // close file descriptor 
		}

		else if (cmd->redirects[1]){ // for the > case

			int fd = open(cmd->redirects[1], O_WRONLY | O_CREAT | O_TRUNC, // Open the file for writing, if it doesn't exist create a new file, if it exists delete it -> overwrite it
				FILE_MODE); // Create file permissions: user read and write, group read, other read
		 
			if (fd == -1){ // open() failed
				printf("ERROR! : Problem about output redirection (>)");
				printf("ERROR! : Problem about output redirection (>): %s\n", strerror(errno));
				printf("redirect to: %s\n", cmd->redirects[1]);
				exit(1);
			}

			dup2(fd, STDOUT_FILENO); // redirect stdout  to the file
			close(fd); // close file descriptor 
		}

		else if (cmd->redirects[2]){ // for the >> case
			int fd = open(cmd->redirects[2], O_WRONLY | O_CREAT | O_APPEND, // Open the file for writing, if it doesn't exist create a new file, if it exists append to existing file			
			FILE_MODE); // Create file permissions: user read and write, group read, other read
		 
			if (fd == -1){ // open() failed
				printf("ERROR! : Problem about append redirection (>>)");
				exit(1);
			}

			dup2(fd, STDOUT_FILENO); // redirect stdout  to the file
			close(fd); // close file descriptor 
		}


		// TODO: implement exec for the resolved path using execv()
		// execvp(cmd->name, cmd->args); // <- DO NOT USE THIS, replace it with execv()
		
		execv(path_to_execute, cmd->args); // Loads the located file into the child process for execution.

        // if exec fails print error message
		printf("ERROR! : Failed to execute comment: %s\n", cmd->name);

        // normally you should never reach here
        exit(-1);

	} else {
        // PARENT

        wait(0); // wait for child process to finish		
	}

}
