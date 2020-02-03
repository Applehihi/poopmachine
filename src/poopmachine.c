#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char* strlow(char* string);
void operandhandler(char* inputstring, int currentline, struct instruction* program);
void labelhandler(int currentline, int labelno, struct instruction* program, struct label_table label_table);
void addi(int* registers, struct instruction* program);
void subi(int* registers, struct instruction* program);
void movi(int* registers, struct instruction* program);
struct label_table
{
	char** label_list;
	int* jmpline;
};

union vardata
{
	int value;
	enum reg { ACC, DAT, PC, CMS, IN, OUT } reg; //Accumulator, data, program counter, comparison status, input, output
};
enum addmode { VAL, REG, NUL };
struct instruction
{
	enum opcode{NOP, MOV, ADD, SUB, SWP, JMP, NEG, END} opcode;
	union vardata operand[2];
	enum addmode addmode[2];
};
//TODO: Replace scanf with fgets

int main(void)
{
	char** program; //Dynamic array of strings
	program = calloc(0, sizeof(char*));
	/*
	Two program reading modes:
	DYN: Reads program dynamically from STDIN
	FIL: Reads and loads program from file
	*/
	printf("Enter program input mode: DYN/FIL? "); //Prompt user
	char program_mode[5];
	fgets(program_mode, 4, stdin); //Read input
	//printf("%c",program_mode[3]);
	if (program_mode[3] == '\n'||' ')
	{
		program_mode[3] = '\0';
		//printf("%s", program_mode);
	}
	else
	{
		while (fgetc(stdin) != '\n');
	}
	for (int i = 0; i < 3; i++) //Turn all chars into lowercase
	{
		program_mode[i] = tolower(program_mode[i]);
	}

	FILE* program_stream;
	if (strcmp(program_mode, "dyn") == 0) //If input is "DYN"
	{
		program_stream = stdin;
	}
	else if (strcmp(program_mode, "fil") == 0) //If input is "FIL"
	{
		char filenamebuffer[255];
		printf("Filename? "); //User prompt
		int temp = 0;
		char tchar;
		while ((tchar = fgetc(stdin)) != '\n')
		{
			filenamebuffer[temp] = tchar;
			temp++;
			filenamebuffer[temp] = '\0';
		}
		if ((program_stream = fopen(filenamebuffer, "r")) == NULL) //Test if file opened successfully
		{
			//Terminate program if file cannot open
			perror("Error opening file");
			return 1;
		}
		else
		{
			//Continue program if file opens
			printf("Successfully opened file %s", filenamebuffer);
		}
	}
	else
	{
		//Terminate program if bad input
		fprintf(stderr, "Bad input");
		return 1;
	}
	while (fgetc(stdin) != '\n'); //Flush newline
	char* fileinputbuffer;

	int linecounter = 0;
	while (1)
	{
		fileinputbuffer = calloc(0, sizeof(char));
		printf("In program input loop\n");
		if (feof(program_stream)) //If program input reaches end of file
		{
			fprintf(stderr, "End of file reached, load program? Y/N: ");
			char userinput;
			userinput = fgetc(stdin);
			userinput = tolower(userinput);
			if (userinput == 'y')
			{
				break;
			}
			else
			{
				fprintf(stderr, "Program aborted, terminating");
				return 2;
			}
		}
		int charcounter = 0;
		char tempchar;
		while ((tempchar = fgetc(program_stream)) != '\n')
		{
			if (tempchar != '\n') {

				fileinputbuffer = realloc(fileinputbuffer, (charcounter + 1) * sizeof(char));
				fileinputbuffer[charcounter] = tempchar;
				//printf("%c, %s, %d\n", tempchar, fileinputbuffer, charcounter);

			}
			charcounter++;
		}
		fileinputbuffer = realloc(fileinputbuffer, (charcounter + 1) * sizeof(char));
		fileinputbuffer[charcounter] = '\0';
		if (strcmp(fileinputbuffer, "end prog") == 0) //Terminate program input
		{
			printf("Reached end of program");
			break;
		}
		printf("fileinpbuffer: %s, %d\n", fileinputbuffer, linecounter);
		if ((program = realloc(program, (linecounter + 1) * sizeof(char*))) == NULL)
		{
			perror("Failed to allocate memory");
		}
		if ((program[linecounter] = calloc(strlen(fileinputbuffer) + 1, sizeof(char))) == NULL)
		{
			perror("Failed to allocate memory");
		}//Setup dynamic string
		strcpy(program[linecounter], fileinputbuffer); //Copy input into program memory
		printf("%s, %s", program[linecounter], fileinputbuffer);
		linecounter++; //Move on to next line of program
		free(fileinputbuffer);
	}
	//END OF INPUT HANDLING

	//LABEL PREPROCESSING
	struct label_table label_table; //Initialise label table
	label_table.label_list = calloc(0, sizeof(char*)); //Initialise dynamic memory for list of labels
	label_table.jmpline = calloc(0, sizeof(int)); //Initialise dynamic memory for list of label jump locations
	int labelno = 0;
	for (int i = 0; i < linecounter; i++)
		//run through all lines in program
		//linecounter has number of lines of input
	{
		int charcounter = 0;
		char* templabel;
		templabel = calloc(0, sizeof(char)); //Initialise dynamic memory for temporary label
		while (1) //run through all chars
		{
			if (charcounter > strlen(program[i])+2)
			{
				//If end of line is reached
				//printf("\n c>slen %s \n", templabel);
				free(templabel);
				break;
			}
			//TODO: rewrite
			if (program[i][charcounter] != ' ' || program[i][charcounter] != ':')
			{
				//Allocate more memory to dynamic temp string then take in another char
				templabel = realloc(templabel, (charcounter + 1) * sizeof(char));
				templabel[charcounter] = tolower(program[i][charcounter]); //remove tolower if program breaks, have not tested
				charcounter++;
			}
			if (program[i][charcounter] == ' ')
			{
				//Broken label
				break;
			}
			if (program[i][charcounter] == ':') //Terminating char
			{
				labelno++;
				label_table.label_list = realloc(label_table.label_list, labelno * sizeof(char*));
				label_table.jmpline = realloc(label_table.jmpline, labelno * sizeof(int));
				label_table.label_list[labelno - 1] = calloc(strlen(templabel), sizeof(char));
				//printf("%d, %s", labelno - 1, templabel);
				strcpy(label_table.label_list[labelno - 1], templabel);
				label_table.jmpline[labelno - 1] = i;
				free(templabel);
				//change label to nop for processing
				free(program[i]);
				program[i] = calloc(4, sizeof(char));
				program[i] = "NOP";
				break;
			}
		}
	}
	//PROCESSING
	printf("\nin processing");
	printf("linecounter: %d\n", linecounter);
	struct instruction* cprogram; //"Compiled" program
	cprogram = calloc(linecounter+1, sizeof(struct instruction)); //Allocate enough memory for entire program
	for (int i = 0; i < linecounter; i++)
	{ 
		printf("poop");
		//TODO:SORT OUT LABEL PROCESSING
		//IMPT: WRITE YOUR OWN STRTOK! DESTRUCTIVE FUNCTION
		//Walk through all lines of uncompiled program
		char* instruction;
		//todo: replace with reentrant strtok
		instruction = strtok(program[i], " "); //Get first token (opcode)
		instruction = strlow(instruction);
		printf("\ninstruction: %s", instruction);
		//begin if else ladder
		if (strcmp(instruction, "mov") == 0) { //MOV
			cprogram[i].opcode = MOV;
			operandhandler(instruction, i, cprogram);
		}
		else if (strcmp(instruction, "sub") == 0) { //SUB
			cprogram[i].opcode = SUB;
			operandhandler(instruction, i, cprogram);
		}
		else if (strcmp(instruction, "add") == 0) { //ADD
			cprogram[i].opcode = ADD;
			printf("\n before operand handler");
			operandhandler(instruction, i, cprogram);
			printf("\n add");
		}
		else if (strcmp(instruction, "swp") == 0) { //SWP
			cprogram[i].opcode = SWP;
			operandhandler(instruction, i, cprogram);
		}
		else if (strcmp(instruction, "neg") == 0) { //NEG
			cprogram[i].opcode = NEG;
			operandhandler(instruction, i, cprogram);
		}
		else if (strcmp(instruction, "jmp") == 0) { //JMP
			cprogram[i].opcode = JMP;
			//TODO: jmptable lookup handler
			//operandparse(instruction, i, &cprogram);
			labelhandler(i, labelno, cprogram, label_table);
		}
		else if (strcmp(instruction, "end") == 0) { //END
			cprogram[i].opcode = END;
			operandhandler(instruction, i, cprogram);
		}
		else if (strcmp(instruction, "nop") == 0) { //NOP
			cprogram[i].opcode = NOP;
			//operandhandler(instruction, i, cprogram);
		}
		printf("after parsing if else tree");
		//free(instruction);
	}
	//EVALUATION
	//Initialisation
	/*int pc = 0; //Program counter
	int acc = 0; //Accumulator
	int dat = 0; //Data
	int cms = 0; //Comparison status
	*/
	int reg[4];
	for (int i = 0; i < 4; i++)
	{
		reg[i] = 0; //init
	}
	printf("\n in evaluation");
	//Run program
	while (1)
	{
		//printf("PC: %d\n", reg[PC]);
		//printf("ACC: %d\n", reg[ACC]);
		//Bounds checking for program counter
		if (reg[PC] >= linecounter)
		{
			//Wrap around
			//printf("PC Exceeded\n");
			reg[PC] = reg[PC] % linecounter;
			//printf("Wrapped around, pc = %d\n", reg[PC]);
		}
		//Sign checking for program counter
		if (reg[PC] < 0)
		{
			//Reset program counter to 0
			reg[PC] = 0;
		}
		if (cprogram[reg[PC]].opcode == END)
		{
			break;
		}
		else if (cprogram[reg[PC]].opcode == NOP) { //No operate
			//pc++; //Increase program counter by 1
		}
		else if (cprogram[reg[PC]].opcode == SWP) { //Swap
			//XOR swap
			reg[ACC] = reg[ACC] ^ reg[DAT];
			reg[DAT] = reg[DAT] ^ reg[ACC];
			reg[ACC] = reg[ACC] ^ reg[DAT];
			//pc++;
		}
		else if (cprogram[reg[PC]].opcode == ADD) { //Add
			addi(reg, cprogram);
		}
		else if (cprogram[reg[PC]].opcode == SUB) { //Subtract
			subi(reg, cprogram);
		}
		else if (cprogram[reg[PC]].opcode == NEG) { //Subtract
			reg[cprogram[reg[PC]].operand[0].reg] = -reg[cprogram[reg[PC]].operand[0].reg];
		}
		else if (cprogram[reg[PC]].opcode == JMP) { //Jump
			reg[PC] = cprogram[reg[PC]].operand[0].value;
		}
		else if (cprogram[reg[PC]].opcode == MOV) { //Move
			//oh boy
			//printf("mov");
			movi(reg, cprogram);
		}
		//printf("Finished one cycle\n");
		reg[PC]++;
		getchar();
	}
}


void addi(int* registers, struct instruction* program)
{
	//printf("PC in addi: %d", registers[PC]);
	//printf("program[%d].addmode[%d]: \n", registers[PC], 0 /*program[registers[PC]]->addmode[0]*/);
	//printf("a %d", program[registers[PC]]->addmode[0]);
	if (program[registers[PC]].addmode[0] == REG)
	{
		//printf("got here");
		if (program[registers[PC]].operand[0].reg == IN)
		{
			int temp;
			temp = getintinput();
			registers[ACC] += temp;//add user input to accumulator
		}
		else if (program[registers[PC]].operand[0].reg == OUT)
		{
			fprintf(stderr, "Error on line %d", registers[PC]);
		}
		else
		{
			//add value from corresponding register
			registers[ACC] += registers[program[registers[PC]].operand[0].reg]; 
		}
	}
	else if (program[registers[PC]].addmode[0] == VAL)
	{
		//add value in instruction
		//printf("got here");
		registers[ACC] += program[registers[PC]].operand[0].value;
	}
}
void subi(int* registers, struct instruction* program)
{
	if (program[registers[PC]].addmode[0] == REG)
	{
		if (program[registers[PC]].operand[0].reg == IN)
		{
			int temp;
			temp = getintinput();
			registers[ACC] -= temp;//subtract user input from accumulator
		}
		else if (program[registers[PC]].operand[0].reg == OUT)
		{
			fprintf(stderr, "Error on line %d", registers[PC]);
		}
		else
		{
			//substract value from corresponding register
			registers[ACC] -= registers[program[registers[PC]].operand[0].reg];
		}
	}
	else if (program[registers[PC]].addmode[0] == VAL)
	{
		//add value in instruction
		registers[ACC] -= program[registers[PC]].operand[0].value;
	}
}
void movi(int* registers, struct instruction* program)
{
	//printf("in mov");
	int source = 0;
	//Source
	//printf("addmode: %d", program[registers[PC]].addmode[1]);
	if (program[registers[PC]].addmode[1] == REG)
	{
		//printf("source isreg\n");
		//Register
		if (program[registers[PC]].operand[1].reg == OUT)
		{
			//Cannot move data from out
			fprintf(stderr, "Error on line %d", registers[PC]);
		}
		else if (program[registers[PC]].operand[1].reg == IN)
		{
			//Get input from user
			source = getintinput();
		}
		else
		{
			//Get value from register
			source = registers[program[registers[PC]].operand[1].reg];
		}
	}
	else
	{
		//Value
		source = program[registers[PC]].operand[1].value;
	}
	//Destination
	//printf("add mode: %d\n", program[registers[PC]]->addmode[0]);
	if (program[registers[PC]].addmode[0] == REG)
	{
		//printf("isreg\n");
		if (program[registers[PC]].operand[0].reg == IN)
		{
			//Cannot move data to in
			fprintf(stderr, "Error on line %d", registers[PC]);
		}
		else if (program[registers[PC]].operand[0].reg == OUT)
		{
			//Print output
			fprintf(stdout, "OUT: %d\n", source);
		}
		else
		{
			//Set value of register to source
			registers[program[registers[PC]].operand[0].reg] = source;
		}
	}
	/*
	else
	{
		printf("Function not currently supported");
	}*/
}
int getintinput(void)
{
	int charcounter = 0;
	char tempchar;
	char* inputbuffer = calloc(0, sizeof(char));
	while ((tempchar = fgetc(stdin)) != '\n')
	{
		if (tempchar != '\n') {

			inputbuffer = realloc(inputbuffer, (charcounter + 1) * sizeof(char));
			inputbuffer[charcounter] = tempchar;
			//printf("%c, %s, %d\n", tempchar, fileinputbuffer, charcounter);

		}
		charcounter++;
	}
	inputbuffer = realloc(inputbuffer, (charcounter + 1) * sizeof(char));
	inputbuffer[charcounter] = '\0';
	char* endptr;
	int number;
	number = strtol(inputbuffer, &endptr, 10); //convert string to int
	//non integral things after reading in all numbers will be dumped into endptr
	if (*endptr == '\0')
	{
		free(inputbuffer);
		//endptr will be string null terminator is all things read are digits
		return number;
	}
	else if (endptr == inputbuffer)
	{
		free(inputbuffer);
		perror("Error taking user input");
		return -1;
	}
	else if ((number == LONG_MIN || number == LONG_MAX) && errno == ERANGE)
	{
		free(inputbuffer);
		perror("Error taking user input");
		return -1;
	}
}

void labelhandler(int currentline, int labelno, struct instruction* program, struct label_table label_table)
{
	//Handling of label table to replace instances of labels in jmp instructions
	//Searches through entire table for instance of match, then replaces with jmpline
	//For easier parsing when running program
	char* intermediate;
	intermediate = strtok(NULL, " ");
	intermediate = strlow(intermediate);
	for (int i = 0; i < labelno; i++) //run through label table
	{
		if (strcmp(intermediate, label_table.label_list[i]) == 0) //if label matches argument
		{
			program[currentline].addmode[0] = VAL; //set address mode to value
			program[currentline].operand[0].value = label_table.jmpline[i]; //set operand value to corresponding jmpline
		}
	}
}

void operandhandler(char* inputstring, int currentline, struct instruction* program)
{
	//parse raw input and "compile"
	//turn all chars into lowercase for easier handling
	//inputstring = strlow(inputstring);
	
	for (int i = 0; i < 2; ++i)
	{
		//get operand token
		char* intermediate;
		printf("i:%d\n", i);
		intermediate = strtok(NULL, " "); //maybe replace with reentrant version of strtok
		if (intermediate == NULL)
		{
			printf("null");
			program[currentline].addmode[i] = NUL;
			program[currentline].operand[i].reg = 0;
			break;
		}
		printf("currentline: %d", currentline);
		intermediate = strlow(intermediate);
		printf("\ni: %d, intermediate: %s", i, intermediate);
		if (strcmp(intermediate, "acc") == 0) //ACC input(accumulator)
		{
			//printf("acc operand\n");
			program[currentline].addmode[i] = REG; //Set address mode to register
			program[currentline].operand[i].reg = ACC;
		}
		else if (strcmp(intermediate, "dat") == 0) //DAT input(data)
		{
			program[currentline].addmode[i] = REG; //Set address mode to register
			program[currentline].operand[i].reg = DAT;
		}
		else if (strcmp(intermediate, "pc") == 0) //PC input(program counter)
		{
			program[currentline].addmode[i] = REG; //Set address mode to value
			program[currentline].operand[i].reg = PC;
		}
		else if (strcmp(intermediate, "in") == 0) //IN User input
		{
			program[currentline].addmode[i] = REG; //Set address mode to value
			program[currentline].operand[i].reg = IN;
		}
		else if (strcmp(intermediate, "out") == 0) //OUT Console output
		{
			printf("gamer");
			program[currentline].addmode[i] = REG; //Set address mode to value
			program[currentline].operand[i].reg = OUT;
		}
		else if (strcmp(intermediate, "0") == 0) //Value 0 input(Since information about 0 is lost using strcmp)
		{
			program[currentline].addmode[i] = VAL;
			program[currentline].operand[i].value = 0;
		}
		else if (atoi(intermediate)) //Value input (atoi returns 0 if no character can be converted)
		{
			printf("value");
			program[currentline].addmode[i] = VAL; //Set address mode to value 
			program[currentline].operand[i].value = atoi(intermediate);
			printf("\nafter operand assignment");
		}
		else if (intermediate == NULL)
		{
			printf("null");
			program[currentline].addmode[i] = NUL;
			program[currentline].operand[i].reg = 0;
		}
		else
		{
			fprintf(stderr, "Bad input on line %d", currentline + 1);
			exit(1);
		}
		printf("\n after operand handler if else tree\n");
		//free(intermediate);
		printf("program[%d].addmode[%d]: %d\n", currentline, i, program[currentline].addmode[i]);
	}
	printf("cucumber");
	
}
char* strlow(char* string)
{
	//convert entire string to lowercase, ignoring non alphabetical characters
	for (int i = 0; i < strlen(string); i++)
	{
		string[i] = tolower(string[i]);
	}
	return string;
}