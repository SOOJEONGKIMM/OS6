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
	if(getnode >= MAX_FILES) 
		return -1;
	if(getnode>=0){//file doesnt exist
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
			
			return inodenum;
		}
		else{	
			return -1;
		}
	}
	
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
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	ssufs_readInode(inodenum, tmp);
	//memset(tmp->direct_blocks,-1,NUM_DATA_BLOCKS);
	ssufs_writeInode(inodenum, tmp);
	int start_offset=file_handle_array[inodenum].offset;
	printf("++start_offset:%d\n",start_offset);
	
	for(int i=0;i<MAX_OPEN_FILES;i++){
		if(file_handle_array[i].inode_number==-1){//not in use
			file_handle_array[i].inode_number=inodenum;
			file_handle_array[i].offset=0;
			return i;
		}
		
	}
	//no file index to return 
		return -1;
}

void ssufs_close(int file_handle){
	file_handle_array[file_handle].inode_number = -1;
	file_handle_array[file_handle].offset = 0;
}

int ssufs_read(int file_handle, char *buf, int nbytes){
	/* 4 */
	int inodenum;
	char tmpbuf[BLOCKSIZE];
	tmpbuf[BLOCKSIZE]='\0';
	
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	
	inodenum=file_handle_array[file_handle].inode_number;

	ssufs_readInode(inodenum, tmp);
	if(tmp->file_size==0 ){
		//free(tmp);
		return -1;
	}
	memset(buf,0,strlen(buf));
	int start_offset=file_handle_array[file_handle].offset;
	int last_offset=file_handle_array[file_handle].offset+nbytes;
	if(last_offset > 256){
		return -1;
	}
	
	int firstblknum = start_offset / BLOCKSIZE;
	int totalblk = last_offset / BLOCKSIZE;//'0 totalblk' means using 1 block
	if(totalblk!=0){
	if((last_offset % BLOCKSIZE) == 0)
		totalblk--;
	}
	if(tmp->file_size==0)
		totalblk=0;

	int update_blknum = nbytes / BLOCKSIZE; //blk that is updating
	if(update_blknum!=0){
		if(nbytes % BLOCKSIZE == 0){
			update_blknum--;
		} 
	}
	int blkuse=0; //using block
	if(totalblk>=0 && ((last_offset % BLOCKSIZE) != 0)){//0 is blk1
		blkuse=1;
		if(totalblk>=1 && (last_offset-BLOCKSIZE*totalblk) < nbytes){//uses two blk
			blkuse=2;
			if(totalblk>=2 && update_blknum>=1 && blkuse==2){//uses three blk
				blkuse=3;
				if(totalblk>=3 && update_blknum>=2 && blkuse==3){//uses four blk
						blkuse=4;
				}
			}
		}
	}

	int backup;
	if(start_offset % BLOCKSIZE == 0)
		backup = 0;
	else
		backup = BLOCKSIZE - (start_offset % BLOCKSIZE) ;
	int nextblkdata = last_offset % BLOCKSIZE;
	
	memset(buf,'\0',BLOCKSIZE);
	for(int i=firstblknum;i<blkuse+firstblknum;i++){//0 is blk1
		ssufs_readDataBlock(tmp->direct_blocks[i],tmpbuf);//+i*BLOCKSIZE);
		if(i==firstblknum){
			if(blkuse>1){
				strncpy(buf, tmpbuf + start_offset%BLOCKSIZE, backup);//ex)!----32 bytes 
			}
			else if(blkuse==1){
				if(start_offset%BLOCKSIZE==0)
					strncpy(buf, tmpbuf, nbytes);//ex)!---32 bytes of data----|
				else
					strncpy(buf, tmpbuf + start_offset%BLOCKSIZE, nbytes);//ex)!---32 bytes of data----|
			}
		}
		else if(i==firstblknum+1)
			strncat(buf, tmpbuf+start_offset, nextblkdata);//ex)of data------|
			
	}
	ssufs_lseek(file_handle, nbytes);
	free(tmp);
	return 0;

}

int ssufs_write(int file_handle, char *buf, int nbytes){
	/* 5 */
	int inodenum=0;
	char tmpbuf[300];
	tmpbuf[300]='\0';

	if(file_handle >= MAX_FILES)
		return -1;
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	
	inodenum=file_handle_array[file_handle].inode_number;
	ssufs_readInode(inodenum, tmp);
	
	int blknum=0;
		
	int start_offset=file_handle_array[file_handle].offset;
	int last_offset = file_handle_array[file_handle].offset + nbytes;
	int totalblk = last_offset / BLOCKSIZE;
	if(totalblk!=0){
	if((last_offset % BLOCKSIZE) == 0)
		totalblk--;
	}
	
	int blkcnt=0;
	int freeblk=0;
	for(int i=0;i<MAX_FILE_SIZE;i++){
		if(tmp->direct_blocks[i]==-1){
			freeblk=1;
			blkcnt=i;
			break;
		}
	}
	if(blkcnt==0 && totalblk==3)//time to overwrite last blk
		blkcnt=3;
	if(last_offset%BLOCKSIZE == nbytes){
		blkcnt++;
	}
	else if(start_offset%BLOCKSIZE==0){
		blkcnt++;
	}
	int update_blknum = nbytes / BLOCKSIZE;
	
	if(update_blknum!=0){
		if(nbytes % BLOCKSIZE == 0){
			update_blknum--;
		} 
	}

	if(!freeblk){
			
			if(last_offset % BLOCKSIZE <= nbytes){
				free(tmp);
				return -1;
			}
	}
	if(tmp->file_size==0)
		update_blknum=0;
	int tmp_direct_blocks[MAX_FILE_SIZE];
	memset(tmp_direct_blocks,-1,MAX_FILE_SIZE);

	if(tmp->file_size != 0){ //ex) 0 1 -1 -1

		for(int i=0; i<= totalblk;){//MAX_FILE_SIZE){//ex)16 32 48 64
		
			if(tmp->direct_blocks[i]!=-1)
				i++;
			else if(tmp->direct_blocks[i]==-1){
				
				blknum=ssufs_allocDataBlock();
				if(blknum==-1){//no free block
					free(tmp);
					return -1;
				}
				tmp->direct_blocks[i] = blknum;
				i++;
				
			}
		}
		
	}
	else{// -1 -1 -1 -1
		for(int i=0; i<= totalblk;i++){
			blknum=ssufs_allocDataBlock();
			if(blknum==-1){//no free block
				free(tmp);
				return -1;
			}
			tmp->direct_blocks[i] = blknum;
			ssufs_writeInode(inodenum, tmp);
		}
	}

	memset(tmpbuf,'\0',300);
	char origin[BLOCKSIZE];
	char copybuf2[BLOCKSIZE];
	memset(copybuf2,0,BLOCKSIZE);
	char copybuf3[BLOCKSIZE];
	memset(copybuf3,0,BLOCKSIZE);
	char copybuf4[BLOCKSIZE];
	memset(copybuf4,0,BLOCKSIZE);
	char updatebuf[BLOCKSIZE];
	char updatebuf2[BLOCKSIZE];
	char updatebuf3[BLOCKSIZE];
	char updatebuff3[BLOCKSIZE];
	char updatebuf4[BLOCKSIZE];
	memset(updatebuf,0,BLOCKSIZE);//has origin+new data
	memset(updatebuf2,0,BLOCKSIZE);;//only new data-->uses two blk
	memset(updatebuf3,0,BLOCKSIZE);;//only new data-->uses three blk
	memset(updatebuff3,0,BLOCKSIZE);;//only new data-->uses three blk
	memset(updatebuf4,0,BLOCKSIZE);;//only new data-->uses four blk
	
	strncpy(tmpbuf, buf, nbytes);
	tmpbuf[nbytes]='\0';
	
	int overwritedone=0;
	int blkuse=0;
	int firstblknum;
	if(tmp->file_size!=0 ){//&& totalblk>0){//totalblk = using blk count
		origin[BLOCKSIZE]='\0';

	if(blkcnt==0)
		firstblknum=0;
	else if(blkcnt>0 && last_offset%BLOCKSIZE>=0)
		firstblknum=blkcnt-1;
	
	

	int backup = start_offset % BLOCKSIZE;
	int i;
	if(totalblk>=0 && ((tmp->file_size % BLOCKSIZE) != 0)){//0 is blk1
		blkuse=1;
		
		ssufs_readDataBlock(tmp->direct_blocks[firstblknum],updatebuf);//get original buf
		strncat(updatebuf, tmpbuf, BLOCKSIZE-backup);//update with new data
		updatebuf[strlen(updatebuf)]='\0';
	
		if(totalblk>=1 && (last_offset-BLOCKSIZE*totalblk) < nbytes){//uses two blk
			blkuse=2;
			
			strncpy(updatebuf2, tmpbuf+(BLOCKSIZE-backup), nbytes-(BLOCKSIZE-backup));
			updatebuf2[nbytes-(BLOCKSIZE-backup)]='\0';
			
		strcpy(updatebuf3,updatebuf2);
	
			if(totalblk-firstblknum>=2 && ((last_offset-BLOCKSIZE*totalblk) < nbytes) && blkuse==2){//uses three blk
				blkuse=3;
				strncpy(updatebuff3, tmpbuf+(BLOCKSIZE-backup), nbytes-(BLOCKSIZE-backup));
			
					if(totalblk-firstblknum>=3 && ((last_offset-BLOCKSIZE*totalblk) < nbytes) && blkuse==3){//uses four blk
						blkuse=4;
						strncpy(updatebuf4, tmpbuf+(BLOCKSIZE-backup), nbytes-(BLOCKSIZE-backup));
						printf("debug update4:%s\n",updatebuf4);
					}
				
			}
		}
	
	}
	else{//ex)tmp->file_size%BLOCKSIZE==0  ex2)first write, using two blks
			blkuse=1;
			strcpy(updatebuf, tmpbuf);
	}
	}//end of backup
	else{
		if(tmp->file_size==0){
			firstblknum=0;

			if(totalblk==0){
			blkuse=1;
			strcpy(updatebuf, tmpbuf);
			}
			else if(totalblk==1){
				blkuse=2;
				strncpy(updatebuf, tmpbuf, BLOCKSIZE);
				strncpy(copybuf2, tmpbuf+BLOCKSIZE, last_offset%BLOCKSIZE);
			}
			else if(totalblk==2){
				blkuse=3;
				strncpy(updatebuf, tmpbuf, BLOCKSIZE);
				strncpy(copybuf2, tmpbuf+BLOCKSIZE, BLOCKSIZE);
				strncpy(copybuf3, tmpbuf+2*BLOCKSIZE, last_offset%BLOCKSIZE);
			}
			else{
				blkuse=4;
				strncpy(updatebuf, tmpbuf, BLOCKSIZE);
				strncpy(copybuf2, tmpbuf+BLOCKSIZE, BLOCKSIZE);
				strncpy(copybuf3, tmpbuf+2*BLOCKSIZE, BLOCKSIZE);
				strncpy(copybuf4, tmpbuf+3*BLOCKSIZE, last_offset%BLOCKSIZE);
			}
		}
		else{
			blkuse=1;
			strcpy(updatebuf, tmpbuf);
		}
	}
	
	if(strlen(updatebuf)>BLOCKSIZE){
		updatebuf[BLOCKSIZE]='\0';
	}
	if(strlen(updatebuf2)>BLOCKSIZE){
		updatebuf2[BLOCKSIZE]='\0';
	}
	if(strlen(updatebuf2)==0 && strlen(updatebuf3)>0){
		strcpy(updatebuf2,updatebuf3);
	}

	if(start_offset + nbytes > 256){
		free(tmp);
		printf("over 256...\n");
		return -1;
	}

	for(int i=firstblknum;i<firstblknum+blkuse;i++){

		if(tmp->file_size==0 || (totalblk>0 && blkuse>=2 && i!=firstblknum || nbytes==BLOCKSIZE)){
			if(strlen(updatebuf2)==0 && strlen(updatebuf3)>0){
				strcpy(updatebuf2,updatebuf3);
			}
	
		if(start_offset!=0){
			if(tmp->direct_blocks[i]!=-1){				
				if(blkuse>=2 && i==firstblknum+1){
					ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuf2);//+i*BLOCKSIZE);
					ssufs_dump();
				}
				if(blkuse>=3 && i==firstblknum+2)
					ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuff3);
				if(blkuse>=4 && i==firstblknum+3)
					ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuf4);
				else{
					if(nbytes==BLOCKSIZE)
						ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuf);
					else
						ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuf);
				}
			
			}
		}
		else if(start_offset==0){
			if(tmp->direct_blocks[i]!=-1){				
				if(blkuse>=2 && i==1){
					ssufs_writeDataBlock(tmp->direct_blocks[i], tmpbuf+i*BLOCKSIZE);
					if(blkuse>=3 && i==2)
						ssufs_writeDataBlock(tmp->direct_blocks[i], copybuf3);
					if(blkuse>=4 && i==3)
						ssufs_writeDataBlock(tmp->direct_blocks[i], copybuf4);
				}
				else{
					if(nbytes==BLOCKSIZE)
						ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuf);
					else{
						ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuf);
					}
				}
			
			}
		}
		tmp->file_size = start_offset+nbytes;
		ssufs_writeInode(inodenum, tmp);
		ssufs_lseek(file_handle, nbytes);
	}
	else if(last_offset%BLOCKSIZE>=0 &&  overwritedone==0 && nbytes!=BLOCKSIZE){//continue write(same block)
		overwritedone=1;
		int i=firstblknum;
		if(strlen(updatebuf)>BLOCKSIZE){
			updatebuf[BLOCKSIZE]='\0';
		}
		ssufs_writeDataBlock(tmp->direct_blocks[i],updatebuf);//+i*BLOCKSIZE);
				
		
		tmp->file_size = start_offset+nbytes;
		
		ssufs_writeInode(inodenum, tmp);
		ssufs_lseek(file_handle, nbytes);
		
	}
	}
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
