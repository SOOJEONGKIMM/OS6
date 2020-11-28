#include "ssufs-ops.h"

extern struct filehandle_t file_handle_array[MAX_OPEN_FILES];

int ssufs_allocFileHandle() {
	for(int i = 0; i < MAX_OPEN_FILES; i++) {
		if (file_handle_array[i].inode_number == -1) {
			return i;
		}
	}
	return -1;
}

int ssufs_create(char *filename){
	/* 1 */
	int inodenum;
	int getnode;
	getnode=open_namei(filename);
	//if(getnode!=-1){//file exists
	if(getnode>=0){
		return -1;
	}
	else{
		//inode_freelist에서 비어있는 첫 노드의 인덱스를 반환한다.
		inodenum=ssufs_allocInode();
		if(inodenum>=0){//has empty
			struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
			ssufs_readInode(inodenum, tmp);
			tmp->status=INODE_IN_USE;
			strcpy(tmp->name, filename);
			ssufs_writeInode(inodenum, tmp);
			free(tmp);
			return inodenum;
		}
		else{
			
			return -1;
		}
	}
	
//	return inodenum;
	
}

void ssufs_delete(char *filename){
	/* 2 */
	int inodenum, inodeptr;
	inodenum=open_namei(filename);
	if(inodenum==-1){
		return;
	}
	ssufs_freeInode(inodenum);

}

int ssufs_open(char *filename){
	/* 3 */
	int inodenum;
	inodenum=open_namei(filename);
	if(inodenum==-1){//file doesnt exists
		return -1;
	}
	for(int i=0;i<MAX_OPEN_FILES;i++){
		if(file_handle_array[i].inode_number==-1){//not in use
			file_handle_array[i].inode_number=inodenum;
			file_handle_array[i].offset=0;
			return i;
		}
		else{
			return -1;
		}
	}
}

void ssufs_close(int file_handle){
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = 0;
}

int ssufs_read(int file_handle, char *buf, int nbytes){
	/* 4 */
	int inodenum;
	char tmpbuf[BLOCKSIZE];
	memset(tmpbuf, -1, BLOCKSIZE);
	
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	inodenum=file_handle_array[file_handle].inode_number;
	ssufs_readInode(inodenum, tmp);
	if(tmp->file_size==0 ){//|| tmp->file_size < nbytes)
		//free(tmp);
		return -1;
	}
	int start_offset=file_handle_array[file_handle].offset;
	int last_offset = file_handle_array[file_handle].offset + nbytes;
	int blksize = tmp->file_size / BLOCKSIZE;
	if(tmp->file_size % BLOCKSIZE == 0)
		blksize=0;
		//blksize--;
	
	for(int i=0;i<=blksize;i++){//0 is blk1
		ssufs_readDataBlock(tmp->direct_blocks[i],tmpbuf+i*BLOCKSIZE);
	}
	strncpy(buf, tmpbuf + start_offset, nbytes);
	file_handle_array[file_handle].offset = last_offset;//if succeed
	//free(tmp);
	return 0;

}

int ssufs_write(int file_handle, char *buf, int nbytes){
	/* 5 */
	int inodenum=0;
	char tmpbuf[BLOCKSIZE];
	memset(tmpbuf, -1, BLOCKSIZE);
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	
	inodenum=file_handle_array[file_handle].inode_number;
	ssufs_readInode(inodenum, tmp);
	//if(tmp->file_size==0 )
	//	return -1;
printf("debug  first filesize @@@@@@@@@@@@@@@@@@@@@@@@@:%d\n",tmp->file_size);
	int blknum=0;
		
	int start_offset=file_handle_array[file_handle].offset;
	int last_offset = file_handle_array[file_handle].offset + nbytes;
	int blksize = tmp->file_size / BLOCKSIZE;
	
	if(blksize!=0){
	if((tmp->file_size % BLOCKSIZE) == 0)
		blksize--;
		//blksize=1;
	}
	if(tmp->file_size==0)
		blksize=0;
	else if(blksize==0 && tmp->file_size%BLOCKSIZE != 0){
		blksize=1;
	}
	printf("debug  first blksize @@@@@@@@@@@@@@@@@@@@@@@@@:%d\n",blksize);
	printf("debug lastoffset@@@@@@@@@@@@@@@@@@@@@@@@@:%d\n",last_offset);
	printf("debug  rest @@@@@@@@@@@@@@@@@@@@@@@@@:%d\n",last_offset%BLOCKSIZE);
	int update_blknum = last_offset / BLOCKSIZE;
	if(update_blknum!=0){
	if(update_blknum % BLOCKSIZE == 0){
		//update_blknum=1;
		update_blknum--;
	} 
	}
	printf("update_blknum:%d===============\n",update_blknum);
	if((last_offset % BLOCKSIZE != 0) && update_blknum==0){
		printf("not here??\n");
		update_blknum=1;
	}
	if(tmp->file_size==0)
		update_blknum=0;
	
	int tmp_direct_blocks[MAX_FILE_SIZE];
	memset(tmp_direct_blocks,-1,MAX_FILE_SIZE);
	int index=-1;

	if(tmp->file_size != 0){ //ex) 0 1 -1 -1
		int freeblk=0;
		printf("update_blknum:%d===============\n",update_blknum);
		for(int i=0; i<= update_blknum;){//MAX_FILE_SIZE){//4
		//for(int i=0; i< NUM_DATA_BLOCKS;){
			
			if(tmp->direct_blocks[i]!=-1)
				i++;
			else if(tmp->direct_blocks[i]==-1){
				//if(last_offset > tmp->file_size)
				//	return -1;
				freeblk=1;
				blknum=ssufs_allocDataBlock();
				printf("0 1 -1 -1 blknum:%d\n",blknum);
				if(blknum==-1){//no free block
					//free(tmp);
				
					return -1;
				}
				tmp->direct_blocks[i] = blknum;
				index=i;
				printf("direct_blocks[%d]:%d\n",i,tmp->direct_blocks[i]);
				i++;
			}
		}
		if(!freeblk){
			//free(tmp);
				printf("no free blk returned.......\n");
			return -1;
		}
	}
	else{// -1 -1 -1 -1
		for(int i=0; i<= update_blknum;i++){
		//for(int i=0; i< NUM_DATA_BLOCKS;i++){
			blknum=ssufs_allocDataBlock();
			//if(last_offset > tmp->file_size)
			//		return -1;
			printf("-1 -1 -1 -1 blknum:%d\n",blknum);
			if(blknum==-1){//no free block
				//free(tmp);
				return -1;
			}
			tmp->direct_blocks[i] = blknum;
			ssufs_writeInode(inodenum, tmp);
		}
	}
printf("debug  blksize@@@@@@@@@@@@@@@@@@@@@@@@@:%d\n",blksize);
tmpbuf[BLOCKSIZE]='\0';
	char tmpbuf2[BLOCKSIZE];
	//memset(tmpbuf2, -1 , BLOCKSIZE);
	for(int i=0;i<blksize;i++){//0 is blk1
	//for(int i=0;i<NUM_INODES;i++){
	//for(int i=0;i<MAX_FILE_SIZE;i++){
		tmpbuf[BLOCKSIZE]='\0';
		tmpbuf2[BLOCKSIZE]='\0';
		printf("debug tmpbuf2@@@@@@@@@@@@@@@@@@@@@@@@@:%s\n",tmpbuf2);
		printf("direct_blocks[%d]:%d\n",i,tmp->direct_blocks[i]);
		ssufs_readDataBlock(tmp->direct_blocks[i],tmpbuf2+1);//get original buf
		printf("debug tmpbuf2@@@@@@@@@@@@@@@@@@@@@@@@@:%s\n",tmpbuf2);
		strncat(tmpbuf, tmpbuf2, strlen(tmpbuf2));//update with new data 
		printf("debug tmpbuf@@@@@@@@@@@@@@@@@@@@@@@@@:%s\n",tmpbuf);
	//}
	}
	strncpy(tmpbuf+start_offset, buf, nbytes);
	int buflen=strlen(tmpbuf);
	//if(buflen < start_offset+nbytes){
	//	return -1;
	//}
	//strncpy(buf, tmpbuf + start_offset, nbytes);
	//strncpy(tmpbuf + start_offset, buf, nbytes);

	//strncpy(tmpbuf , buf, nbytes);
	if(start_offset + nbytes > 256){
		//free(tmp);
		printf("over 256...\n");
		return -1;
	}
	else if((start_offset + nbytes ) <= tmp->file_size){//overwrite

		for(int i=0;i<MAX_FILE_SIZE;i++){
			if(tmp->direct_blocks[i]!=-1){
				ssufs_writeDataBlock(tmp->direct_blocks[i],tmpbuf+i*BLOCKSIZE);
				printf("debug-----------------------------------:%d\n",tmp->direct_blocks[i]);
				
			}
		}
		
		file_handle_array[file_handle].offset = last_offset;//if succeed
		ssufs_writeInode(inodenum, tmp);

		
	}
	else{
		/*int allocnum;
		for(int i=0; i<= update_blknum;i++){
			allocnum=ssufs_allocDataBlock();
			//if(allocnum==-1)//no free block
			//	return -1;
			//tmp_direct_blocks[i] = blknum;
		}*/
		for(int i=0;i<= update_blknum;i++){
			if(tmp->direct_blocks[i]!=-1){
				ssufs_writeDataBlock(tmp->direct_blocks[i], tmpbuf);//+i*BLOCKSIZE);
				printf("debug++++++++++++++++++++++++++++++++++:%d\n",tmp->direct_blocks[i]);
			}
		}
		
		file_handle_array[file_handle].offset = last_offset;//if succeed
		tmp->file_size = start_offset+nbytes;
		ssufs_writeInode(inodenum, tmp);
		/*for(int i=0;i<= update_blknum;i++){//debug
		char check[BLOCKSIZE];
		ssufs_readDataBlock(tmp->direct_blocks[i],check);
		printf("debug check**************:%s\n",check);
		printf("debug check**************:%d\n",strlen(check));
			}*/
	}
	
	//free(tmp);
	return 0;
}

int ssufs_lseek(int file_handle, int nseek){
	int offset = file_handle_array[file_handle].offset;

	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);
	
	int fsize = tmp->file_size;
	
	offset += nseek;

	if ((fsize == -1) || (offset < 0) || (offset > fsize)) {
		free(tmp);
		return -1;
	}

	file_handle_array[file_handle].offset = offset;
	free(tmp);

	return 0;
}
