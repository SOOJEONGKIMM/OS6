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
	tmpbuf[BLOCKSIZE]='\0';
	
	
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	
	inodenum=file_handle_array[file_handle].inode_number;

	ssufs_readInode(inodenum, tmp);
	if(tmp->file_size==0 ){
		//free(tmp);
		return -1;
	}
	int blkcnt=0;
	for(int i=0;i<NUM_DATA_BLOCKS;i++){
		if(tmp->direct_blocks[i]==-1){
			blkcnt=i;
			break;
		}
	}
	int start_offset=file_handle_array[file_handle].offset;
	int last_offset=file_handle_array[file_handle].offset+nbytes;
	//if(blkcnt==0 && totalblk==3)
	//	blkcnt=4;
	if(last_offset%BLOCKSIZE == nbytes){
		blkcnt++;
	}
	int firstblknum;
	if(blkcnt==0)
		firstblknum=0;
	else if(blkcnt>0 && last_offset%BLOCKSIZE>0)
		firstblknum=blkcnt-1;

	
	printf("debug start_offset:%d\n",start_offset);
	int blknum = start_offset / BLOCKSIZE;
	if(blknum!=0){
		if(start_offset % BLOCKSIZE == 0)
			blknum--;
	}
	printf("firstblknum:%d\n",firstblknum);
	printf("blknum:%d\n",blknum);
	int backup = start_offset % BLOCKSIZE;
	
	for(int i=firstblknum;i<=blknum+firstblknum;i++){//0 is blk1
		ssufs_readDataBlock(tmp->direct_blocks[i],tmpbuf);//+i*BLOCKSIZE);
		if(i==firstblknum)
			strncpy(buf, tmpbuf + start_offset, BLOCKSIZE-backup);
		else if(i==firstblknum+1)
			strncat(buf, tmpbuf+(BLOCKSIZE-backup), nbytes-(BLOCKSIZE-backup));
	}
	
	
	//strncpy(buf, tmpbuf, nbytes);
	ssufs_lseek(file_handle, nbytes);
	
	printf("buf:%s--------------------------------------------------\n",tmpbuf);

	//file_handle_array[file_handle].offset = last_offset;//if succeed
	//free(tmp);
	return 0;

}

int ssufs_write(int file_handle, char *buf, int nbytes){
	/* 5 */
	int inodenum=0;
	char tmpbuf[BLOCKSIZE];
	tmpbuf[BLOCKSIZE]='\0';
	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	
	inodenum=file_handle_array[file_handle].inode_number;
	ssufs_readInode(inodenum, tmp);
	int blknum=0;
		
	int start_offset=file_handle_array[file_handle].offset;
	printf("start_offset:%d\n",start_offset);
	int last_offset = file_handle_array[file_handle].offset + nbytes;// + nbytes;
	printf("last_offset:%d\n",last_offset);
	int totalblk = last_offset / BLOCKSIZE;
	printf("totalblk:%d\n",totalblk);
	if(totalblk!=0){
	if((last_offset % BLOCKSIZE) == 0)
		totalblk--;
	}
	if(tmp->file_size==0)
		totalblk=0;
		//printf("totalblk:%d\n",totalblk);
	/*else if(blknum==0 && tmp->file_size%BLOCKSIZE != 0){
		blknum=1;
	}*/
	//printf("debug start_offset@@@@@@@@@@@@@@@@@@@@@@@@@:%d\n",start_offset);
	//printf("debug lastoffset@@@@@@@@@@@@@@@@@@@@@@@@@:%d\n",last_offset);
	
	int blkcnt=0;
	int freeblk=0;
	for(int i=0;i<NUM_DATA_BLOCKS;i++){
		if(tmp->direct_blocks[i]==-1){
			freeblk=1;
			blkcnt=i;
			break;
		}
	}
	if(blkcnt==0 && totalblk==3)
		blkcnt=4;
	if(last_offset%BLOCKSIZE == nbytes){
		blkcnt++;
	}
	else if(start_offset%BLOCKSIZE==0){
		blkcnt++;
	}
	printf("blkcnt:%d\\\\\\\\\\\n",blkcnt);


	int update_blknum = nbytes / BLOCKSIZE;
	
	if(update_blknum!=0){
		if(nbytes % BLOCKSIZE == 0){
			update_blknum--;
		} 
	}

	if(!freeblk){
			//free(tmp);
			if(last_offset % BLOCKSIZE <= nbytes){
				printf("no free blk , returned.......\n");
				return -1;
			}
	}
	//printf("update_blknum:%d===============\n",update_blknum);
	if(tmp->file_size==0)
		update_blknum=0;
	
	int tmp_direct_blocks[MAX_FILE_SIZE];
	memset(tmp_direct_blocks,-1,MAX_FILE_SIZE);

	if(tmp->file_size != 0){ //ex) 0 1 -1 -1
		
		//printf("update_blknum:%d===============\n",update_blknum);
		for(int i=0; i<= totalblk;){//MAX_FILE_SIZE){//ex)16 32 48 64
		
			if(tmp->direct_blocks[i]!=-1)
				i++;
			else if(tmp->direct_blocks[i]==-1){
				
				blknum=ssufs_allocDataBlock();
				printf("0 1 -1 -1 blknum:%d\n",blknum);
				if(blknum==-1){//no free block
					//free(tmp);
				
					return -1;
				}
				tmp->direct_blocks[i] = blknum;
				i++;
				
			}
		}
		
	}
	else{// -1 -1 -1 -1
		for(int i=0; i<= totalblk;i++){
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

	tmpbuf[BLOCKSIZE]='\0';
	char origin[BLOCKSIZE];
	char updatebuf[BLOCKSIZE];
	char updatebuf2[BLOCKSIZE];
	char updatebuf3[BLOCKSIZE];
	char updatebuf4[BLOCKSIZE];
	updatebuf[BLOCKSIZE]='\0';//has origin+new data
		updatebuf2[BLOCKSIZE]='\0';//only new data-->uses two blk
		updatebuf3[BLOCKSIZE]='\0';//only new data-->uses three blk
		updatebuf4[BLOCKSIZE]='\0';//only new data-->uses four blk
	//memset(tmpbuf2, -1 , BLOCKSIZE);
	strncpy(tmpbuf, buf, nbytes);
	//printf("tmpbuf yet:%s\n",tmpbuf);
	tmpbuf[nbytes]='\0';
	//printf("tmpbuf :%s\n",tmpbuf);
	int overwritedone=0;
	int blkuse=0;
	int firstblknum;
	if(tmp->file_size!=0 ){//&& totalblk>0){//totalblk = using blk count
		tmpbuf[BLOCKSIZE]='\0';
		
		origin[BLOCKSIZE]='\0';
	//printf("totalblk:%d\n",totalblk);
	printf("blkcnt:%d\n",blkcnt);
	
	
	if(blkcnt==0)
		firstblknum=0;
	else if(blkcnt>0 && last_offset%BLOCKSIZE>=0)
		firstblknum=blkcnt-1;
	
	

	int backup = start_offset % BLOCKSIZE;
	//printf("updateblk_num:%d\n",update_blknum);
	printf("firstblknum:%d\n",firstblknum);
	int i;
	if(totalblk>=0 && ((tmp->file_size % BLOCKSIZE) != 0)){//0 is blk1
		blkuse=1;
		
		ssufs_readDataBlock(tmp->direct_blocks[firstblknum],updatebuf);//get original buf
		//printf(" origin:%s\n",updatebuf);
		
		strncat(updatebuf, tmpbuf, BLOCKSIZE-backup);//update with new data 
		//printf("origin new:%s\n",updatebuf);
		updatebuf[strlen(updatebuf)]='\0';
	
		if(totalblk>=1 && (last_offset-BLOCKSIZE*totalblk) < nbytes){//uses two blk
			blkuse=2;
			
			strncpy(updatebuf2, tmpbuf+(BLOCKSIZE-backup), nbytes-(BLOCKSIZE-backup));
			updatebuf2[nbytes-(BLOCKSIZE-backup)]='\0';
			//printf("nbytes-(BLOCKSIZE-backup):%d\n",nbytes-(BLOCKSIZE-backup));
			
		strcpy(updatebuf3,updatebuf2);
			if(totalblk>=2 && update_blknum>=1 && blkuse==2){//uses three blk
				blkuse=3;
				//updatebuf3[0]='\0';
				strncpy(updatebuf3, tmpbuf+(BLOCKSIZE-backup)+BLOCKSIZE, nbytes-(BLOCKSIZE-backup)-BLOCKSIZE);
				{
					if(totalblk>=3 && update_blknum>=2 && blkuse==3){//uses four blk
						blkuse=4;
						//updatebuf4[0]='\0';
						strncpy(updatebuf4, tmpbuf+(BLOCKSIZE-backup)+2*BLOCKSIZE, nbytes-(BLOCKSIZE-backup)-2*BLOCKSIZE);
					}
				}
			}
		}
	
	}
	else{//ex)tmp->file_size%BLOCKSIZE==0
		blkuse=1;
		strcpy(updatebuf, tmpbuf);
		//printf("@@@@@@@@@@@@@@debug update:%s\n",updatebuf);
	}
	}//end of backup
	else{
		if(tmp->file_size==0){
			firstblknum=0;
		}
		
		blkuse=1;
		strcpy(updatebuf, tmpbuf);
	//	printf("@@@@@@@@@@@@@@debug update:%s\n",updatebuf);
	}
	//if(blkcnt>0 && last_offset%BLOCKSIZE==0)
	//	blkuse--;
	if(strlen(updatebuf)>BLOCKSIZE){
		updatebuf[BLOCKSIZE]='\0';
	}
	if(strlen(updatebuf2)>BLOCKSIZE){
		updatebuf2[BLOCKSIZE]='\0';
	}
	if(strlen(updatebuf2)==0 && strlen(updatebuf3)>0){
		strcpy(updatebuf2,updatebuf3);
	}
	/*printf("debug update:%s\n",updatebuf);
	printf("debug update2:%s\n",updatebuf2);
	printf("debug update3:%s\n",updatebuf3);
	printf("debug update4:%s\n",updatebuf4);*/
	

	if(start_offset + nbytes > 256){
		//free(tmp);
		printf("over 256...\n");
		return -1;
	}
	printf("firstblknum:%d blkue:%d\n",firstblknum,blkuse);
	for(int i=firstblknum;i<firstblknum+blkuse;i++){
	//	printf("i:%d===============\n",i);
	if(last_offset%BLOCKSIZE>=0 && tmp->file_size!=0 && overwritedone==0 && nbytes!=BLOCKSIZE){//continue write(same block)
		printf("overwriting...\n");
		overwritedone=1;
		int i=firstblknum;
			if(strlen(updatebuf)>BLOCKSIZE){
		updatebuf[BLOCKSIZE]='\0';
	}
	//	printf("debug update:%s\n",updatebuf);
	//	printf("firstblknum:%d\n",firstblknum);
		ssufs_writeDataBlock(tmp->direct_blocks[i],updatebuf);//+i*BLOCKSIZE);
				
		
		tmp->file_size = start_offset+nbytes;
		
		ssufs_writeInode(inodenum, tmp);
		ssufs_lseek(file_handle, nbytes);
		
	}
	if(tmp->file_size==0 || (totalblk>0 && blkuse>=2 && i!=firstblknum || nbytes==BLOCKSIZE)){
		printf("new writing...\n");
		printf("blkuse:%d\n",blkuse);
		//for(int i=0;i<= totalblk;i++){
			printf("debug update:%s\n",updatebuf);
			if(strlen(updatebuf2)==0 && strlen(updatebuf3)>0){
		strcpy(updatebuf2,updatebuf3);
	}
	printf("firstblknum:%d blkue:%d\n",firstblknum,blkuse);
			if(tmp->direct_blocks[i]!=-1){				
				if(blkuse==2 && i==1)
					ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuf2+i*BLOCKSIZE);
				if(blkuse==3 && i==2)
					ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuf3+i*BLOCKSIZE);
				if(blkuse==4 && i==3)
					ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuf4+i*BLOCKSIZE);
				else{
					printf("here..? i:%d\n",i);	
					printf("debug update:%s\n",updatebuf);
					ssufs_writeDataBlock(tmp->direct_blocks[i], updatebuf);//+i*BLOCKSIZE);
				}
			
			}
		//}
	//	printf("start_offset:%d\n",file_handle_array[file_handle].offset);
		//file_handle_array[file_handle].offset = last_offset;//if succeed
		
		tmp->file_size = start_offset+nbytes;
		ssufs_writeInode(inodenum, tmp);
		ssufs_lseek(file_handle, nbytes);
	///	printf("offset[%d]:%d\n",file_handle,file_handle_array[file_handle].offset);
		/*for(int i=0;i<= update_blknum;i++){//debug
		char check[BLOCKSIZE];
		ssufs_readDataBlock(tmp->direct_blocks[i],check);
		printf("debug check**************:%s\n",check);
		printf("debug check**************:%d\n",strlen(check));
			}*/
	}
	}
	//printf("off---:%d\n",file_handle_array[file_handle].offset);
	//free(tmp);
	return 0;
}

int ssufs_lseek(int file_handle, int nseek){
	int offset = file_handle_array[file_handle].offset;

	struct inode_t *tmp = (struct inode_t *) malloc(sizeof(struct inode_t));
	ssufs_readInode(file_handle_array[file_handle].inode_number, tmp);
	
	int fsize = tmp->file_size;
	
	offset += nseek;
//	printf("offset:%d fsize:%d\n",offset, fsize);
	if ((fsize == -1) || (offset < 0) || (offset > fsize)) {
		free(tmp);
		return -1;
	}

	file_handle_array[file_handle].offset = offset;
	free(tmp);

	return 0;
}
