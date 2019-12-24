#include <stdlib.h>
#include <stdio.h>
#include <defrag.h>
#include <string.h>

int defrag(char *image_name)
{
	superblock sb;
	FILE *input, *output;
	int eof;

	if(!strcmp(image_name,"disk_defrag"))
	{
		printf("Input image name can't be disk_defrag.\nTry renaming the input image.\n");
		return SYSERR;
	}

	input = fopen(image_name,"r");
	if(input==NULL)
	{
		printf("Give image doesn't exist.\n");
		return SYSERR;
	}

	output = fopen("disk_defrag", "w");
	if(output==NULL)
	{
		printf("Can't create output image.\n");
		return SYSERR;
	}

	unsigned char *bns = malloc(1024);
	fread(bns, 1024, 1, input);
	fwrite(bns, 1024, 1, output);
	free(bns);

	fseek(input, 0, SEEK_END);
	eof = ftell(input);

	fseek(input, 512, SEEK_SET);
	fread(&sb, sizeof(superblock), 1, input);

	int inode_count = ((sb.data_offset - sb.inode_offset)*sb.blocksize)/sizeof(inode);
	int inode_start = DISK_START + (sb.inode_offset*sb.blocksize);
	int data_start = DISK_START + (sb.data_offset*sb.blocksize);
	int inode_pb = sb.blocksize/sizeof(int);

	inode ind[inode_count];
	fseek(input, 1024, SEEK_SET);
	fread(&ind, sizeof(ind), 1, input);

	unsigned int *i1block, *i2block, *i3block;
	unsigned char *dblock;
	int i,j,k,l;
	int block_per_file;
	int next_free = 0;
	int my_block_level_1, my_block_level_2, my_block_level_3;
	for(i=0; i<inode_count; i++)
	{
		if(ind[i].size>0)
		{		   
			block_per_file = (ind[i].size/sb.blocksize) + (ind[i].size%sb.blocksize != 0);
			//direct
			dblock = malloc(sb.blocksize);
			for(j=0; j<N_DBLOCKS ; j++)
			{
				if(block_per_file > 0)
				{
					fseek(input, data_start+(ind[i].dblocks[j]*sb.blocksize), SEEK_SET);
					fseek(output, data_start+(next_free*sb.blocksize), SEEK_SET);
					fread(dblock, sb.blocksize, 1, input);
					fwrite(dblock, sb.blocksize, 1, output);
					ind[i].dblocks[j] = next_free++;
					block_per_file--;
				}
				else
					ind[i].dblocks[j] = -1; 
			}
			free(dblock);

			//indirect1
			dblock = malloc(sb.blocksize);
			i1block = malloc(sb.blocksize);
			for(j=0; j<N_IBLOCKS; j++)
			{
				if(block_per_file > 0)
				{
					my_block_level_1 = next_free++;
					fseek(input, data_start+(ind[i].iblocks[j]*sb.blocksize), SEEK_SET);
					fread(i1block, sb.blocksize, 1, input);
					
					for(k=0; k<inode_pb; k++)
					{
						if(block_per_file > 0)
						{
							fseek(input, data_start+(i1block[k]*sb.blocksize), SEEK_SET);
							fseek(output, data_start+(next_free*sb.blocksize), SEEK_SET);
							fread(dblock, sb.blocksize, 1, input);
							fwrite(dblock, sb.blocksize, 1, output);
							i1block[k]=next_free++;
							block_per_file--;
						}
						else
							i1block[k] = -1;
					}
					ind[i].iblocks[j]=my_block_level_1;
					fseek(output, data_start+(my_block_level_1*sb.blocksize), SEEK_SET);
					fwrite(i1block, sb.blocksize, 1, output);
				}
				else
					ind[i].iblocks[j] = -1;
			}
			free(dblock);
			free(i1block);

			//indirect2
			i2block = malloc(sb.blocksize);
			i1block = malloc(sb.blocksize);
			dblock = malloc(sb.blocksize);

			if(block_per_file > 0)
			{
				my_block_level_2 = next_free++;
				fseek(input, data_start+(ind[i].i2block*sb.blocksize), SEEK_SET);
				fread(i2block, sb.blocksize, 1, input);
				
				for(j=0; j<inode_pb; j++)
				{
					if(block_per_file > 0)
					{
						my_block_level_1 = next_free++;
						fseek(input, data_start+(i2block[j]*sb.blocksize), SEEK_SET);
						fread(i1block, sb.blocksize, 1, input);

						for(k=0; k<inode_pb; k++)
						{
							if(block_per_file > 0)
							{
								fseek(input, data_start+(i1block[k]*sb.blocksize), SEEK_SET);
								fseek(output, data_start+(next_free*sb.blocksize), SEEK_SET);
								fread(dblock, sb.blocksize, 1, input);
								fwrite(dblock, sb.blocksize, 1, output);
								i1block[k]=next_free++;
								block_per_file--;
							}
							else
								i1block[k] = -1;
						}
						i2block[j]=my_block_level_1;
						fseek(output, data_start+(my_block_level_1*sb.blocksize), SEEK_SET);
						fwrite(i1block, sb.blocksize, 1, output);
					}
					else
						i2block[j] = -1;
				}
				ind[i].i2block=my_block_level_2;
				fseek(output, data_start+(my_block_level_2*sb.blocksize), SEEK_SET);
				fwrite(i2block, sb.blocksize, 1, output);
			}
			else
				ind[i].i2block = -1;

			free(dblock);
			free(i1block);
			free(i2block);

			//indirect3
			i3block = malloc(sb.blocksize);
			i2block = malloc(sb.blocksize);
			i1block = malloc(sb.blocksize);
			dblock = malloc(sb.blocksize);

			if(block_per_file > 0)
			{
				my_block_level_3 = next_free++;
				fseek(input, data_start+(ind[i].i3block*sb.blocksize), SEEK_SET);
				fread(i3block, sb.blocksize, 1, input);
				
				for(j=0; j<inode_pb; j++)
				{
					if(block_per_file > 0)
					{
						my_block_level_2 = next_free++;
						fseek(input, data_start+(i3block[j]*sb.blocksize), SEEK_SET);
						fread(i2block, sb.blocksize, 1, input);

						for(k=0; k<inode_pb; k++)
						{
							if(block_per_file > 0)
							{
								my_block_level_1 = next_free++;
								fseek(input, data_start+(i2block[k]*sb.blocksize), SEEK_SET);
								fread(i1block, sb.blocksize, 1, input);

								for(l=0; l<inode_pb; l++)
								{
									if(block_per_file > 0)
									{
										fseek(input, data_start+(i1block[l]*sb.blocksize), SEEK_SET);
										fseek(output, data_start+(next_free*sb.blocksize), SEEK_SET);
										fread(dblock, sb.blocksize, 1, input);
										fwrite(dblock, sb.blocksize, 1, output);
										i1block[l]=next_free++;
										block_per_file--;
									}
									else
										i1block[l] = -1;
								}
								i2block[k]=my_block_level_1;
								fseek(output, data_start+(my_block_level_1*sb.blocksize), SEEK_SET);
								fwrite(i1block, sb.blocksize, 1, output);
							}
							else
								i2block[k] = -1;
						}
						i3block[j]=my_block_level_2;
						fseek(output, data_start+(my_block_level_2*sb.blocksize), SEEK_SET);
						fwrite(i2block, sb.blocksize, 1, output);
					}
					else
						i3block[j] = -1;
				}
				ind[i].i3block=my_block_level_3;
				fseek(output, data_start+(my_block_level_3*sb.blocksize), SEEK_SET);
				fwrite(i3block, sb.blocksize, 1, output);
			}
			else
				ind[i].i3block = -1;

			free(dblock);
			free(i1block);
			free(i2block);
			free(i3block);
		}
	}

	/*Write inodes back*/
	fseek(output, inode_start, SEEK_SET);
	for(i=0; i<inode_count; i++)
		fwrite(&(ind[i]), sizeof(inode), 1, output);

	/*write super*/
	sb.free_block = next_free;

	fseek(output, 512, SEEK_SET);
	fwrite(&sb, sizeof(superblock), 1, output);

	/*write 0 for free*/
	unsigned int *free_blk;
	free_blk = calloc(1, sb.blocksize);
	fseek(output, data_start+(next_free*sb.blocksize), SEEK_SET);
	int next_free_seq = next_free + 1;
	while( ftell(output) < DISK_START+(sb.swap_offset*sb.blocksize) )
	{
		free_blk[0]=next_free_seq;
		fwrite(free_blk, sb.blocksize, 1, output);
		
		next_free_seq++;
		if(next_free_seq+sb.data_offset==sb.swap_offset)
			next_free_seq=-1;
	}
	free(free_blk);

	/*Write swap*/
	dblock = malloc(sb.blocksize);
	fseek(input, DISK_START+(sb.swap_offset*sb.blocksize), SEEK_SET);
	while(ftell(input)!=eof)
	{
		fread(dblock, sb.blocksize, 1, input);
		fwrite(dblock, sb.blocksize, 1, output);
	}
	free(dblock);

	fclose(input);
	fclose(output);

	return OK;
}

int main(int argc, char** argv)
{
	if(argc<2)
	{
		printf("Image name not defined.\nExiting...\n");
		return SYSERR;
	}

	if(argc>2)
	{
		printf("More than one image name defined.\nExiting...\n");
		return SYSERR;
	}

	printf("Starting defragmantation for %s\n", argv[1]);
	
	if( defrag(argv[1]) == OK )
	{
		printf("Defragmantation done for %s\nExiting...\n", argv[1]);
		return OK;
	}
	else
	{
		printf("Defragmantation FAILED for %s\nExiting...\n", argv[1]);
		return SYSERR;
	}
}
