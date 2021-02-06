#include "fs.h"

#define MAX_FILES 64
#define MAX_FILDES 32 //maximum number of references a file can have.
#define MAX_F_NAME 15
#define MAX_BLOCKS 8192
#define DATA_START 2000 //first block that is available for data.
#define DIR_START 2
#define FAT_START 4

struct super_block fs;
struct file_descriptor fildes[MAX_FILDES];
int* FAT;
struct dir_entry* DIR;
struct data_node sdata[MAX_BLOCKS - DATA_START]; //this is an in memory array of all our data in our current fs.

int make_fs(char* disk_name)
{
	if (make_disk(disk_name) == -1)
	{printf("Creation of disk failed.\n");return -1;}

	if (open_disk(disk_name) == -1)
	{printf("failed to open disk.\n"); return -1;}

	//initialize super block
	fs.data_idx = (int) DATA_START;
	fs.dir_idx = (int)DIR_START;
	fs.dir_len = 1;
	fs.fat_idx = FAT_START;
	fs.fat_len = 8; //this allows 32k block indexing. More than enough.
	
	//initialize fat
	FAT = calloc(MAX_BLOCKS, sizeof(int));
	//FAT[12 * 4] = 126; //testing only!
	//initialize DIR
	DIR = calloc(MAX_FILES, sizeof(struct dir_entry)); 

	char *writefs = calloc(BLOCK_SIZE, sizeof(char));
	char *writedir = calloc(BLOCK_SIZE, sizeof(char));
	//char* writenode = calloc(MAX_BLOCKS - DATA_START, sizeof(struct data_node));

	memcpy(writefs, &fs, sizeof(fs));
	memcpy(writedir, DIR, MAX_FILES*sizeof(struct dir_entry)); 
	

	if (block_write(0, writefs) == -1 || block_write(DIR_START, writedir))
	{
		printf("failed initialization write. aborting.\n");
		return -1;
	}                      

	for (int fatindex = 0; fatindex < fs.fat_len; fatindex++)
	{
		//char writeFAT[BLOCK_SIZE];
		char* writeFAT = calloc(BLOCK_SIZE, sizeof(char));
		// 0 1 2 3 4 5 6 7
		memcpy(writeFAT, &FAT[fatindex * 1024], sizeof(char)*4096); //write each block at a time

		if (block_write(fs.fat_idx + fatindex, writeFAT) == -1)
		{ 
			printf("failed initialization write. aborting.\n");
			return -1;
		}

		free(writeFAT);
	}

	if (close_disk(disk_name) == -1)
	{printf("failed to close disk.\n"); return -1;}
	free(writefs);
	free(writedir);
	free(DIR);
	free(FAT);
	return 0;
}


int mount_fs(char* disk_name) {
	
	if (open_disk(disk_name) == -1)
	{printf("failed to open disk.\n"); return -1;}

	char *readfs = calloc(BLOCK_SIZE, sizeof(char));
	char *readdir = calloc(BLOCK_SIZE, sizeof(char));
	//prepping global variables
	FAT = calloc(MAX_BLOCKS, sizeof(int));
	DIR = calloc(MAX_FILES, sizeof(struct dir_entry));
	if (block_read(0, readfs) == -1)
	{
		printf("failed super block write\n");
		return -1;
	}
	memcpy(&fs, readfs, sizeof(fs)); //open superblock
	if (block_read(DIR_START, readdir) == -1) {
		printf("failed directory read\n");
		return -1;
	}


	memcpy(DIR, readdir, MAX_FILES * sizeof(struct dir_entry)); //open directory
	for (int fatindex = 0; fatindex < fs.fat_len; fatindex++)
	{		
		//char readFAT[BLOCK_SIZE];
		char* readFAT = calloc(BLOCK_SIZE, sizeof(char));
		//printf("fat index: %d\n", fatindex);
		if (block_read((fs.fat_idx + fatindex), readFAT) != 0)
		{
			printf("failed initialization write. aborting.\n");
			return -1;
		}
		//memcpy((FAT + fatindex * 4096), readFAT, sizeof(char)*4096); //write each block at a time
		memcpy(&FAT[fatindex*1024], readFAT, sizeof(char) * 4096); //write each block at a time
		//printf("value of fat: %d\n", FAT[12 * 4]);
		free(readFAT);
	}

	//get all block data
	for (int block = 0; block < MAX_BLOCKS - DATA_START; block++)
	{
		char* readBlock = calloc(BLOCK_SIZE, sizeof(char));
		if (block_read(block + DATA_START, readBlock) != 0) { printf("failed to grab node %d\n", block); return -1; }
		memcpy(&(sdata[block]), readBlock, sizeof(struct data_node));//copy node data into a node index.
		free(readBlock);

	}

	free(readfs);
	free(readdir);
	return 0;
}

int umount_fs(char* disk_name) {

	char* writefs = calloc(BLOCK_SIZE, sizeof(char));
	char* writedir = calloc(BLOCK_SIZE, sizeof(char));

	memcpy(writefs, &fs, sizeof(fs));
	memcpy(writedir, DIR, MAX_FILES * sizeof(struct dir_entry));
	
	if (block_write(0, writefs) == -1)
	{
		printf("failed initialization write. aborting.\n");
		return -1;
	}

	if (block_write(DIR_START, writedir) == -1)
	{
		printf("failed initialization write. aborting.\n");
		return -1;
	}

	
	for (int fatindex = 0; fatindex < fs.fat_len; fatindex++)
	{
		char* writeFAT = calloc(BLOCK_SIZE, sizeof(char));
		memcpy(writeFAT, &FAT[fatindex * 1024], 4096*sizeof(char)); //write each block at a time
		if (block_write(fs.fat_idx + fatindex, writeFAT) == -1)
		{
			printf("failed initialization write. aborting.\n");
			return -1;
		}
		free(writeFAT);
	}

	//save all block data
	for (int block = 0; block < MAX_BLOCKS - DATA_START; block++)
	{
		if (block_write(block + DATA_START, (char*)&sdata[block]) != 0) { printf("failed to grab node %d\n", block); return -1; }
	}


	if (close_disk() == -1)
	{
		printf("failed to close disk.\n"); return -1;
	}
	free(FAT);
	free(DIR);
	free(writefs);
	free(writedir);
	return 0;
}


int fs_create(char* name)
{
	if ((strlen(name) > MAX_F_NAME))
	{
		printf("File name not supported.\n"); return -1;
	}
	int empty = -1;
	for (int i = 0; i < MAX_FILES; i++)
	{
		if (DIR[i].used == 1 && (strcmp(DIR[i].name, name)==0))
		{
			printf("File name already in use.\n"); return -1;
		}
		if (DIR[i].used == 0)
		{
			empty = i; //indicator that theres space for another file.
			//can't break since we have to make sure the file doesn't exist already.
		}
	}
	int readyBlock = -1;
	//find block on disk that can fit our file. we assume the file system is mounted.
	for (int block = 0; block < (MAX_BLOCKS-DATA_START); block++)
	{
		if (FAT[DATA_START + block] == 0) {
			readyBlock = DATA_START + block;
			break;
		} 
	}

	//if there is no empty block in data blocks or open directory entry, abort.
	if (readyBlock == -1 || empty == -1)
	{
		printf("Cannot create another file. storage full. aborting.\n");
		return -1;
	}
	//all set to initiate file!

	//update fat and dir
	FAT[readyBlock] = 1; //this block is now used.

	struct dir_entry newEntry = { .used = 1, .size = 0, .head = readyBlock, .ref_cnt=0, .blocks = 1 };
	memcpy(&newEntry.name, name, strlen(name));
	memcpy(&(DIR[empty]), &newEntry, sizeof(newEntry));
	sdata[readyBlock - DATA_START].next = -1; //set next to be -1.
	memset(sdata[readyBlock - DATA_START].data, 0, sizeof(BLOCK_ESIZE)); //set node data to be 0.

	return 0;
}

//delete a file. remember to clean corresponding FAT entries.
int fs_delete(char* name) {
	int found = -1;
	for (int i = 0; i < MAX_FILES; i++)
	{
		if (DIR[i].used == 1 && (strcmp(DIR[i].name, name)==0))
		{
			found = 1;
			if (DIR[i].ref_cnt == 0) { //delete the file
				DIR[i].used = 0; //lazy delete.
				//now clean FAT
				int curBlock = DIR[i].head;
				FAT[curBlock] = 0;
				for (int block = 0; block < DIR[i].blocks - 1; block++)
				{
					int nextBlock = find_file_block(-1, curBlock, 1);
					FAT[nextBlock] = 0; //make block empty.
					sdata[curBlock - DATA_START].next = -1; //sever connection
					curBlock = nextBlock;
				}
				break;
			}
			else {
				printf("There are still %d open file descriptors!\n", DIR[i].ref_cnt);
				return -1;
			}
		}
	}

	if (found == -1) {
		return -1;
	}
	else {
		return 0;
	}
}


//open the file "name"

int fs_open(char* name)
{
	
	int found = -1;
	for (int i = 0; i < MAX_FILES; i++)
	{
		if (DIR[i].used == 1 && (strcmp(DIR[i].name, name)==0))
		{
			found = i;
			break;
		}
	}

	if (found == -1)
	{
		printf("couldn't find file.\n");
		return -1;
	}
	
	
	//find open fildes
	for (int i = 0; i < MAX_FILDES; i++)
	{
		//if not used, fill it in.
		if (fildes[i].used == 0)
		{
			DIR[found].ref_cnt += 1; //now that we found the file, we have to set fildes and directory references
			fildes[i].used = 1;
			fildes[i].file = DIR[found].head;
			fildes[i].offset = 0;
			fildes[i].dir_index = found;
			
			return i; //return index of our open file descriptor
		}
	}
	
	//if we reach here, that means all file descriptors are used.
	return -1;
}

//write to file with file descriptor fildes.
int fs_write(int fd, void* buf, size_t nbyte)
{
	if (fd > 31 || fd < 0 || fildes[fd].used == 0)
	{
		printf("no file with matching file descriptor.\n");
		return -1;
	}

	char* cbuf = calloc(nbyte, sizeof(char));
	memcpy(cbuf, buf, nbyte); //alter format to be a char pointer.
	int written = 0; //tracks how many bytes we've written.
	int blockIDX = fildes[fd].offset/BLOCK_ESIZE; //block we're currently writing to. 
	//printf("File location: %d and offset block:%d\n", fildes[fd].file, blockIDX);
	int blockNum = find_file_block(fildes[fd].dir_index, DIR[fildes[fd].dir_index].head, blockIDX); //still gets block index but much faster.
	if (blockNum == -1) { printf("Disk Full from start!\n"); return written; }
	int blockSpace = BLOCK_ESIZE * (blockIDX + 1) - fildes[fd].offset;// how many bytes are free in current block.
	int totalSpace = BLOCK_SIZE * BLOCK_SIZE - fildes[fd].offset;
	if (totalSpace <= 0) { printf("File is out of space!\n"); free(cbuf);  return written; }
	int writeSpace;
	(totalSpace > blockSpace) ? (writeSpace = blockSpace) : (writeSpace = totalSpace);
	//printf("we can write %d bytes to this block.\n", writeSpace);
	int sizeWrite = (int)nbyte;
	int write_not_done = 1;
	while (write_not_done)
	{
		//printf("write size left: %d\n", sizeWrite);
		char* data = malloc(BLOCK_ESIZE * sizeof(char));
		memcpy(data, &(sdata[blockNum - DATA_START].data), BLOCK_ESIZE);
		int writeStart = fildes[fd].offset % BLOCK_ESIZE;

		if (sizeWrite > writeSpace)//we need to write more than space left in our current block.
		{
			memcpy(&data[writeStart], &(cbuf[written]), writeSpace);//simply write to all of writeSpace
			memcpy(&(sdata[blockNum - DATA_START].data), data, BLOCK_ESIZE);
			fildes[fd].offset += writeSpace;
			written += writeSpace;
			sizeWrite -= writeSpace;
			blockIDX = fildes[fd].offset / BLOCK_ESIZE;
			blockNum = find_file_block(fildes[fd].dir_index, blockNum, 1); //should just be able to get the next block. 
			if (blockNum == -1) { 
				printf("Disk Full!\n");
				if (fildes[fd].offset > DIR[fildes[fd].dir_index].size) {
					DIR[fildes[fd].dir_index].size = fildes[fd].offset;
				}

				free(cbuf);
				return written; 
			}
			blockSpace = BLOCK_ESIZE * (blockIDX + 1) - fildes[fd].offset;// how many bytes are free in current block.
			totalSpace = BLOCK_SIZE * BLOCK_SIZE - fildes[fd].offset;
			if (totalSpace <= 0) { printf("File is our of space!\n"); DIR[fildes[fd].dir_index].size = fildes[fd].offset; free(cbuf); return written; }
			(totalSpace > blockSpace) ? (writeSpace = blockSpace) : (writeSpace = totalSpace);
			
			
		}
		else {

			//now perform necessary write operations
			memcpy(&(data[writeStart]), &(cbuf[written]), sizeWrite);
			memcpy(sdata[blockNum - DATA_START].data, data, BLOCK_ESIZE);
			fildes[fd].offset += sizeWrite;
			written += sizeWrite;
			sizeWrite -= sizeWrite;
			write_not_done = 0;
			
		}

		if (fildes[fd].offset > DIR[fildes[fd].dir_index].size) {
			DIR[fildes[fd].dir_index].size = fildes[fd].offset;
		}
		free(data);
	}
	//printf("write done! we wrote %d bytes.\n", written);
	free(cbuf);
	return written;
}


// read from file 
int fs_read(int fd, void* buf, size_t nbyte) {

	if (fd > 31 || fd < 0 || fildes[fd].used == 0)
	{
		printf("no file with matching file descriptor.\n");
		return -1;
	}
	int read = 0;
	char* cbuf = calloc(nbyte, sizeof(char));
	int blockIDX = fildes[fd].offset / BLOCK_ESIZE; //block we're currently reading from. 
	if (fildes[fd].offset >= DIR[fildes[fd].dir_index].size) { printf("cant read, no space\n");  free(cbuf); return read; }
	int blockNum = find_file_block(-1, DIR[fildes[fd].dir_index].head, blockIDX);
	int fileSpace = DIR[fildes[fd].dir_index].size - fildes[fd].offset; //this is the actual space we have left in our file.
	int blockSpace = BLOCK_ESIZE * (blockIDX + 1) - fildes[fd].offset;// how many bytes are left in this block (past offset)
	int readSpace;
	(fileSpace > blockSpace) ? (readSpace = blockSpace) : (readSpace = fileSpace); //readSpace is whatever is less.
	int read_not_done = 1;
	int sizeRead = (int)nbyte;
	//printf("starting read at offset: %d\n", fildes[fd].offset);
	while (read_not_done)
	{
		char* data = malloc(BLOCK_SIZE * sizeof(char));
		memcpy(data, &(sdata[blockNum - DATA_START].data), BLOCK_ESIZE);
		int readStart = fildes[fd].offset % BLOCK_ESIZE;

		if (sizeRead > readSpace)//we need to write more than space left in our current block.
		{
			memcpy(&(cbuf[read]), &(data[readStart]), readSpace); //we only can read what space we have into buf
			read += readSpace;
			fildes[fd].offset += readSpace;
			sizeRead -= readSpace;
			if (fildes[fd].offset >= DIR[fildes[fd].dir_index].size) { printf("cant read, no space. wrote %d\n", read); memcpy(buf, cbuf, read);  free(cbuf); return read; } //if not, we can assume we've written to entire block.
			blockIDX = fildes[fd].offset / BLOCK_ESIZE; //block we're currently reading from. 
			blockNum = find_file_block( -1, blockNum, 1);
			//printf("blocknum im reading from:%d\n", blockNum);
			int fileSpace = DIR[fildes[fd].dir_index].size - fildes[fd].offset; //this is the actual space we have left in our file.
			int blockSpace = BLOCK_ESIZE * (blockIDX + 1) - fildes[fd].offset;// how many bytes are left in this block (past offset)
			(fileSpace > blockSpace) ? (readSpace = blockSpace) : (readSpace = fileSpace); //readSpace is whatever is less.
			
		}
		else {
			memcpy(&(cbuf[read]), &(data[readStart]), sizeRead);
			read += sizeRead;
			fildes[fd].offset += sizeRead;
			sizeRead -= sizeRead;
			read_not_done = 0;
		}
		free(data);
	}
	memcpy(buf, cbuf, read); //put everything we read into buf.
	free(cbuf);
	return read;
}

//simply close the file descriptor.
int fs_close(int fd) {
	if (fd > 31 || fd < 0 || fildes[fd].used == 0)
	{
		printf("no file with matching file descriptor.\n");
		return -1;
	}

	fildes[fd].used = 0; //lazy delete.
	DIR[fildes[fd].dir_index].ref_cnt -= 1;
	return 0;

}

//move the offset, but not past the file size
int fs_lseek(int fd, off_t offset) {
	if (fd > 31 || fd < 0 || fildes[fd].used == 0)
	{
		printf("no file with matching file descriptor.\n");
		return -1;
	}
	if (offset > DIR[fildes[fd].dir_index].size || offset < 0) {
		printf("invalid seek.\n");
		return -1;
	}
	fildes[fd].offset = (int)offset;
	return 0;
}

int fs_get_filesize(int fd) {
	if (fd > 31 || fd < 0 || fildes[fd].used == 0)
	{
		printf("no file with matching file descriptor.\n");
		return -1;
	}
	return DIR[fildes[fd].dir_index].size;
}

int fs_listfiles(char*** files) {

	char** ourFiles = malloc(sizeof(char*) * MAX_FILES);
	int fc = 0;
	for (int i = 0; i < MAX_FILES; i++)
	{
		if (DIR[i].used == 1)
		{
			ourFiles[fc] = malloc((MAX_F_NAME + 1) * sizeof(char));
			memcpy(ourFiles[fc], &(DIR[i].name), sizeof(DIR[i].name));
			fc++;
		}
	}

	ourFiles[fc] = (char*)NULL;
	*files = ourFiles;
	return 0;
}

int fs_truncate(int fd, off_t length) {
	if (fd > 31 || fd < 0 || fildes[fd].used == 0)
	{
		printf("no file with matching file descriptor.\n");
		return -1;
	}
	if (length > DIR[fildes[fd].dir_index].size || length < 0) {
		printf("Invalid Truncation\n");
		return -1;
	}

	//cut off unnecessary blocks
	int numBlocks = DIR[fildes[fd].dir_index].blocks; //total blocks
	int truncatedStart = (length / BLOCK_ESIZE); //last block we don't truncate
	int head = fildes[fd].file;
	int blockNum = find_file_block(-1, head, truncatedStart); //last block index we don't truncate
	truncatedStart++;//now it is the next block. we truncate this.
	for (int block = truncatedStart; block < numBlocks; block++)
	{
		
		int blockNumNext = find_file_block(-1, blockNum, 1);
		sdata[blockNum - DATA_START].next = -1;
		FAT[blockNumNext] = 0;
		blockNum = blockNumNext;
	}
	//shorten length
	DIR[fildes[fd].dir_index].size = length;
	DIR[fildes[fd].dir_index].blocks = truncatedStart;
	if (fildes[fd].offset > DIR[fildes[fd].dir_index].size)
	{
		fildes[fd].offset = DIR[fildes[fd].dir_index].size;
	}
	return 0;
}
//my helper methods

//this function returns the file system block number of the nth block of a file with head number head.
//if there is no nth block, the function will generate blocks until then. (given that space is available).
int find_file_block(int dir_idx, int cur_block, int n)
{
	int node = cur_block; //start at known block.
	
	for (int i = 0; i < n; i++)
	{
		struct data_node curNode = sdata[node - DATA_START]; //have to offset to match sdata indices.
		int nextNode = curNode.next;
		if (nextNode == -1) //meaning we don't have a next data block
		{
			int appendBlock = -1;
			//find block on disk that can fit our file. we assume the file system is mounted.
			for (int block = 0; block < (MAX_BLOCKS - DATA_START); block++)
			{
				if (FAT[DATA_START + block] == 0) {
					appendBlock = DATA_START + block;
					curNode.next = appendBlock;
					//initialize next node
					struct data_node newNode;
					memset(newNode.data, 0, sizeof(newNode.data));
					newNode.next = -1;
					memcpy(&(sdata[node - DATA_START]), &curNode, sizeof(curNode));
					memcpy(&(sdata[curNode.next - DATA_START]), &newNode, sizeof(newNode));
					FAT[appendBlock] = 1;
					//update directory
					DIR[dir_idx].blocks += 1;
					nextNode = curNode.next;
					break;
				}
			}
			if (appendBlock == -1) {
				printf("No more blocks available!\n");
				return -1;
			}
		}
		node = nextNode;
	}

	return node;
}


