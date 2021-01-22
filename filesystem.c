#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
//---------------IMPORTANT GLOBAL VARIABLES-------------------------
#define FILENAME_MAXLEN 8 // including the NULL char
#define BLOCK_SIZE 1024
#define TOTAL_INODES 16
#define MAX_FILE 8 * 1024
#define BLOCK_COUNT 128
#define ROOTNODE 0
int myfs;
char *path_curr[10];
int path_len = 0;

// MYFS

//-----------STRUCTS------------------

typedef struct inode
{
	int dir; // boolean value. 1 if it's a directory.
	char name[FILENAME_MAXLEN];
	int size;		  // actual file/directory size in bytes.
	int blockptrs[8]; // direct pointers to blks containing file's content.
	int used;		  // boolean value. 1 if the entry is in use.
	int rsvd;		  // reserved for future use
} inode;

typedef struct dirent
{
	char name[FILENAME_MAXLEN];
	int namelen; // length of entry name
	int inode;	 // this entry inode index
	int wy;
} dirent;

typedef struct superblock
{
	int freeblocklist[127];
	inode inodelist[16];
} superblock;

typedef struct datablock
{
	char files[1024];
	struct dirent directory[64];
} datablock;

struct superblock SuperBlock;
struct datablock Datablks[127];

//------------------HELPER FUNCTIONS----------------

int filesystem()
{
	// creation of rootnode
	SuperBlock.freeblocklist[ROOTNODE] = 1;
	SuperBlock.inodelist[ROOTNODE].dir = 1; // 1 bcz it's a directory.
	SuperBlock.inodelist[ROOTNODE].size = sizeof(dirent);
	SuperBlock.inodelist[ROOTNODE].blockptrs[0] = 0; //only one block pointer is used
	SuperBlock.inodelist[ROOTNODE].used = 1;		  //this inode is now in use
	SuperBlock.inodelist[ROOTNODE].rsvd = 0;		  //reserved for future use
	strncpy(SuperBlock.inodelist[ROOTNODE].name, "/", 8);

	return 0;
}

int return_inode(int path_len){
	//calculates the inode position based on path length
	char *new[10];
	if (path_len > 1)
	{
		new[0] = path_curr[path_len - 2];
		for (int i = 0; i < 16; i++)
		{
			if (strcmp(SuperBlock.inodelist[i].name, new[0]) == 0)
			{
				return i; 
			}
		}
	}
	else if (path_len == 1)
	{
		return 0;
	}
}

void file_del(int index){
	// deassigns values from an inode such that the file is deleted
	SuperBlock.inodelist[index].dir = -1;
	SuperBlock.inodelist[index].size = -1;
	SuperBlock.inodelist[index].used = 0;
	SuperBlock.inodelist[index].rsvd= 0;
	strcpy(SuperBlock.inodelist[index].name, "");
}

int SearchInode(char *filename){
	// looks for the inode and returns the index
	// from inodelist
	for(int i = 0; i<16; i++){
		if(strcmp(SuperBlock.inodelist[i].name,filename) == 0){
			return i;
		}
	}
	printf("error: file/directory not found\n");
	return -1;
}
void direc_del(int Oblock, int ind){
	// changes values of datablock such that the directory is deleted
	Datablks[Oblock].directory[ind].inode = -1;
	strncpy(Datablks[Oblock].directory[ind].name,"", 8);
	Datablks[Oblock].directory[ind].namelen = -1;
	Datablks[Oblock].directory[ind].wy = -1;
}

void move(int f_inode, int index){
	// copies contents from one place to another
	SuperBlock.inodelist[f_inode].dir = 0;
	SuperBlock.inodelist[f_inode].size = SuperBlock.inodelist[index].size;
	SuperBlock.inodelist[f_inode].used = SuperBlock.inodelist[index].used;
	SuperBlock.inodelist[f_inode].rsvd=SuperBlock.inodelist[index].rsvd;
}

int directories_iterator(int Oblock){
	// iterates over all the directories in datablock list
	int v = 0;
	while(Datablks[Oblock].directory[v].wy == 1)
	{
		v++;
	}
	return v;
}

int parent(char *filename)
{
	// looks for the parent
	path_len = 0;
	for (int i = 0; i < 10; i++)
	{
		path_curr[i] = NULL;
	}

	char sepe[] = " /";
	char *ptr = strtok(filename, sepe);
	
	while (ptr != NULL)
	{
		path_curr[path_len] = ptr; 
		path_len = path_len + 1;	  
		ptr = strtok(NULL, sepe);
	}
	//once the path len is calculated, look for inode
	int x = return_inode(path_len);
	return x;
	
}


int findFreeblks(int *blks, int blksrequired){
int Block_c=1;

	for (int i=0; i<blksrequired; i++) {
	while(Block_c <= BLOCK_COUNT) {
		if (SuperBlock.freeblocklist[Block_c] != 1) goto FOUND_BLOCK;
		Block_c++;
	}
	printf("error: no free blks\n");
	return -1;
FOUND_BLOCK:
	blks[i] = Block_c;
	SuperBlock.freeblocklist[Block_c] = 1;
	Block_c++;
	}
	return 0;
}


// listing

void listing()
{
	myfs = open("./myfs", O_RDWR | O_CREAT);
	for(int i = 0; i<16;i++){
	if(SuperBlock.inodelist[i].used == 1){
		if(SuperBlock.inodelist[i].dir == 0)
			printf("File: ");
		else{
		
		int ind = 0;
		int p =0;
		char *str[16];
		while(Datablks[SuperBlock.inodelist[i].blockptrs[0]].directory[ind].wy == 1){
			strcpy(str[0], Datablks[SuperBlock.inodelist[i].blockptrs[0]].directory[ind].name);
			
			
			p=p++;
			write(myfs, str[0], sizeof(str[0]));
			ind++;
			
		}
		printf("Directory: ");
		printf("%s ", Datablks[SuperBlock.inodelist[i].blockptrs[0]].directory[ind].name);

		}
		
		printf("%s %d\n",SuperBlock.inodelist[i].name, SuperBlock.inodelist[i].size);
	}
	}
}
//Create directory

int create_directory(char *Dname)
{
	int node_parent = parent(Dname);
	int f_inode;

	int i = 0;
	while(SuperBlock.inodelist[i].used!=0){
		i++;
	}
	if (i==0){
		printf("error: All inodes are full\n");
		return -1;
	}
	else{
		f_inode=i;
	}
	if (f_inode == -1)
		return -1;
	int free_block = findFreeblks(SuperBlock.inodelist[f_inode].blockptrs, 1); // free block
	if (free_block == -1)
		return -1;
	// assign value to the inode from inodelist and index f_inode
	SuperBlock.inodelist[f_inode].dir = 1;
	strncpy(SuperBlock.inodelist[f_inode].name, path_curr[path_len - 1], 8);
	SuperBlock.inodelist[f_inode].used = 1;
	SuperBlock.inodelist[f_inode].rsvd = 0;
	SuperBlock.inodelist[f_inode].size = sizeof(dirent);
	int Oblock = SuperBlock.inodelist[node_parent].blockptrs[0]; //gives us the parent datablock
	int ind = directories_iterator(Oblock);
	Datablks[Oblock].directory[ind].inode = f_inode;
	Datablks[Oblock].directory[ind].namelen = sizeof(Datablks[Oblock].directory[ind].name)/sizeof(Datablks[Oblock].directory[ind].name[0]);
	Datablks[Oblock].directory[ind].wy = 1;
	strncpy(Datablks[Oblock].directory[ind].name, path_curr[path_len-1], 8);
	
}
//--------------------------"CREATE FILE" ----------------------

int create_file(char *filename, int size) {

	int node_parent = parent(filename);
	int blks_required = ceil((size/1024)); //number of blocks required by the file
	if (blks_required > 8){
		printf("error: file can't be saved because filesize exceeding limit\n");
			return -1;
	}
	int f_inode;
	int i = 0;
	while(SuperBlock.inodelist[i].used!=0){
		i++;
	}
	if (i==0){
		printf("error: All inodes are full\n");
		return -1;
	}
	else{
		f_inode=i;
	}
	if (f_inode == -1) return -1;
	int ret = findFreeblks(SuperBlock.inodelist[f_inode].blockptrs,blks_required);
		if (ret == -1) return -1;
	SuperBlock.inodelist[f_inode].used =1;
	SuperBlock.inodelist[f_inode].rsvd=0;
	SuperBlock.inodelist[f_inode].dir = 0;
	SuperBlock.inodelist[f_inode].size = size;
	strcpy(SuperBlock.inodelist[f_inode].name, path_curr[path_len-1]);
	int Oblock = SuperBlock.inodelist[node_parent].blockptrs[0];
	int ind = directories_iterator(Oblock);
	// initilizing a directory
	Datablks[Oblock].directory[ind].inode = f_inode;
	Datablks[Oblock].directory[ind].namelen = sizeof(Datablks[Oblock].directory[ind].name)/sizeof(Datablks[Oblock].directory[ind].name[0]);
	Datablks[Oblock].directory[ind].wy = 1;
	strncpy(Datablks[Oblock].directory[ind].name, path_curr[path_len-1], 8);
}

//DELETE FILE
int delete_file(char *filename) 
{
	int node_parent = parent(filename); //get the parent
	char file_name[8];
	strncpy(file_name, path_curr[path_len-1],8);
	int index = SearchInode(file_name);
	// remove file
	file_del(index);
	int Oblock = SuperBlock.inodelist[node_parent].blockptrs[0]; //gives us the parent datablock can only be at the first index
	int Fblock = SuperBlock.inodelist[index].blockptrs[0];
	SuperBlock.freeblocklist[Fblock] = 0;
	int ind = 0;
	while(strcmp(Datablks[Oblock].directory[ind].name, file_name)!=0) // looks for the empty directory in our global datablock
	{
		ind++;
	}
	//remove from directory
	direc_del(Oblock, ind);
	return 0;

}


//DELETE DIRECTORY
int delete_directory( char *dirname)
{
	char dir[8];
	sscanf(dirname, "/%[^/]%s", dir, dirname);
	strcpy(dir, strtok(dirname,"/"));

	int index = SearchInode(dir);
	if (index == -1) return -1;
	SuperBlock.inodelist[index].dir = -1;
	SuperBlock.inodelist[index].size = -1;
	SuperBlock.inodelist[index].used = 0;
	SuperBlock.inodelist[index].rsvd= 0;
	strcpy(SuperBlock.inodelist[index].name, "");
	int block_index = SuperBlock.inodelist[index].blockptrs[0];
	int ind = 0;
	while(Datablks[block_index].directory[ind].wy == 1){
		int inode = Datablks[block_index].directory[ind].inode;

	}
	SuperBlock.freeblocklist[block_index] = 0; 
}


//MOVE FILE
int move_file(char *source, char *destination)
{
	int node_parent = parent(source);
	char dir[8];
	strcpy(dir, path_curr[path_len-1]);
	int index = SearchInode(dir);

	int Oblock = SuperBlock.inodelist[node_parent].blockptrs[0];
	int ind = 0;
	while(strcmp(Datablks[Oblock].directory[ind].name, dir)!=0 && ind < 64)
	{
		ind++;
	}
	direc_del(Oblock, ind);
	node_parent = parent(destination);
	int block = SuperBlock.inodelist[node_parent].blockptrs[0];
	int j = 0;

	while(Datablks[block].directory[j].wy == 1)
	{
		ind++;
	}
	Datablks[Oblock].directory[ind].inode = index;
	strncpy(Datablks[Oblock].directory[ind].name, path_curr[path_len-1], 8);
	int siz = sizeof(Datablks[Oblock].directory[ind].name)/sizeof(Datablks[Oblock].directory[ind].name[0]);
	Datablks[Oblock].directory[ind].namelen = siz;
	Datablks[Oblock].directory[ind].wy = 1;
}

// COPY FILE
int CopyFile(char *source, char *destination)
{
	char dir[8];
	char *ch;
	ch = strtok(source, "/");
	while (ch != NULL) {
		strcpy(dir, ch);
		ch = strtok(NULL, "/");
		
	}
	int index = SearchInode(dir);
	if(index == -1) return -1;
	int f_inode;

	int i = 0;
	while(SuperBlock.inodelist[i].used!=0){
		i++;
	}
	if (i==0){
		printf("error: All inodes are full\n");
		return -1;
	}
	else{
		f_inode=i;
	}
	if (f_inode == -1) return -1;
	int ret = findFreeblks(SuperBlock.inodelist[f_inode].blockptrs, 1);
	if (ret == -1) return -1;
	int dst_Datablks = SuperBlock.inodelist[f_inode].blockptrs[0];
	int src_Datablks = SuperBlock.inodelist[index].blockptrs[0];
	strcpy(Datablks[dst_Datablks].files, Datablks[src_Datablks].files);
	move(f_inode, index);
	int node_parent = parent(destination);
	strcpy(dir, path_curr[path_len-1]);
	strcpy(SuperBlock.inodelist[f_inode].name,dir);
	int Oblock = SuperBlock.inodelist[node_parent].blockptrs[0]; 
	int ind = directories_iterator(Oblock);
	Datablks[Oblock].directory[ind].inode = f_inode;
	strncpy(Datablks[Oblock].directory[ind].name, path_curr[path_len-1], 8);
	int siz = sizeof(Datablks[Oblock].directory[ind].name)/sizeof(Datablks[Oblock].directory[ind].name[0]);
	Datablks[Oblock].directory[ind].namelen = siz;
	Datablks[Oblock].directory[ind].wy = 1;
	return 0;
}

int main (int argc, char* argv[]) {

	FILE *stream;
	char *filename = argv[1];
	stream = fopen(filename, "r");

	if (stream == NULL)
{
	printf("Could not open file %s", filename);
	return 1;
}

	int f = filesystem();
	char input_line[100];
	
	char command[3], arg_1[100], arg_2[100];
	int size;

	while ( fgets(input_line, 100, stream) ){
		if (input_line[0] != '#' && input_line[0] != '\n'){
			sscanf(input_line,"%[^ \n] %[^\n]", command, input_line);

			if (strcmp(command, "FS") == 0) { // creates new filesystem with only root directory
				filesystem();
			}
			else if (strcmp(command, "LL") == 0) {
				listing();
			}
			else if (strcmp(command, "CR") == 0) {
				sscanf(input_line, "%[^ ] %d\n", arg_1, &size);
				
				create_file(arg_1, size);
			}
			else if (strcmp(command, "CP") == 0) {
				sscanf(input_line, "%[^ ] %[^\n]", arg_1, arg_2);
				CopyFile(arg_1, arg_2);
			}
			else if (strcmp(command, "DL") == 0) {
				sscanf(input_line, "%[^\n]", arg_1);
				delete_file(arg_1);
			}
			
			else if (strcmp(command, "MV") == 0) {
				sscanf(input_line, "%[^ ] %[^\n]", arg_1, arg_2);
				move_file(arg_1, arg_2);
			}
			
			else if (strcmp(command, "DD") == 0) {
				sscanf(input_line, "%[^\n]", arg_1);
				// delete_directory(arg_1);
			}
			else if (strcmp(command, "CD") == 0) {
				sscanf(input_line, "%[^\n]", arg_1);
				create_directory(arg_1);
			}
		}
	}

	listing();
	return 0;
}