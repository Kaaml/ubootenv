//author: Kamil Mazurkiewicz
#define DEBUG
#include <linux/init.h>
#include <linux/module.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/mpage.h>
#include <linux/vfs.h>
#include <linux/blkdev.h>
#include <linux/uio.h>
#include <linux/string.h>

#include "file.h"

#define UBOOTFS_MAGIC 0x00B007F5
int root_ino = 0; 
struct inode * mfile_inode = NULL;

/*static const struct inode_operations uboot_file_inode_operations= {
	.getattr	= simple_getattr,
	.setattr	= simple_setattr
};*/

const struct file_operations s_dir_operations = {
	.open			= uboot_dir_open,
	.release        = dcache_dir_close,
	.llseek         = dcache_dir_lseek,
	.read           = generic_read_dir,
	.iterate_shared = dcache_readdir,
	.fsync          = noop_fsync,
};


static struct inode* mk_dir(struct super_block *sb )
{
	struct inode * inode;
	struct dentry *dentry;
	struct dentry *root = sb->s_root;

	inode_lock(d_inode(root));

	dentry = d_alloc_name(root, "dupa");
	if(!dentry){
		pr_err("nie mozna stowrzyc katalogu!");
	}

	inode = new_inode(sb);
	if(!inode){
		pr_err("nie mozna stworzyc inode");
	}
	inode->i_ino = 2;
	inode->i_mtime = inode->i_atime = inode->i_ctime = current_time(inode);
	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;
	//inode_init_owner(inode, root, 0777|S_IFDIR );
	inode->i_mode = 077|S_IFDIR;
	inode->i_uid = current_fsuid();
	inode->i_gid = current_fsgid();
	d_add(dentry, inode );

	inode_unlock(d_inode(root));

	return inode;
}

static int mk_file(struct inode *parent, const char* file_name, const char* file_content )
{
	struct inode * inode;
	struct dentry *dentry;

	struct super_block *sb = parent->i_sb;
	struct dentry *root = sb->s_root;

	inode_lock(d_inode(root));
	//pr_debug("tutaj! [%s]\n", file_name );
	dentry = d_alloc_name(root, file_name);
	if(!dentry){
		pr_err("nie mozna stowrzyc katalogu!");
	}

	inode = new_inode(sb);
	if(!inode){
		pr_err("nie mozna stworzyc inode");
	}
	inode->i_private = (void*)file_content;
	inode->i_ino = get_next_ino();
	inode->i_mtime = inode->i_atime = inode->i_ctime = current_time(inode);
	inode->i_op = &uboot_file_inode_operations;
	inode->i_fop = &uboot_file_operations;
	//pr_debug("towrzenie pliku OK\n");
	drop_nlink(inode);
	
	inode_init_owner(inode, parent, 0777|S_IFREG );
	
	//inode->i_mode = 077|S_IREG;
	//inode->i_uid = current_fsuid();
	//inode->i_gid = current_fsgid();
	d_add(dentry, inode );

	inode_unlock(d_inode(root));
	return 0;
}

static int init_files(char *data, unsigned size, struct inode * parent )
{
	char * data_cpy = NULL;

	data_cpy = data+ 5; //omit CRC sum 
	pr_debug("[UBOOT DBG]: init file data= %p", data );

	while( data_cpy < (data + size ) && (*data_cpy != '\0' && *(data_cpy+2) != '\0' ) )
	{
		int len = strlen(data_cpy );
		char* l = strpbrk( data_cpy, "=\0" );	
		*l++ = '\0';
		//pr_debug("l= %s\t %s", l, data_cpy );
		//char *file_n = data_cpy;
		
		mk_file(parent, data_cpy, l );

		data_cpy += len +1;
	}
	
	pr_debug("[UBOOT_DBG]: Init files succesfull");
	return 0;
}

//for test
#define uboot_size 0x2800

static struct dentry *uboot_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	pr_debug("[UBOOT DBG]: Uboot_lookup()");
	return simple_lookup(dir,dentry, flags );
}


static const struct inode_operations uboot_dir_inode_operations = {
	.lookup = uboot_lookup,
	.link = simple_link,
	.unlink = simple_unlink
};

static void uboot_put_super(struct super_block *sb)
{
      pr_debug("[UBOOT]: Uboot super block destroyed\n");
	  kzfree( sb->s_fs_info );
}


static struct super_operations const uboot_super_ops = {
      .put_super = uboot_put_super,
};


static int parse_options(struct super_block *sb, void *data )
{
	unsigned int offset;
	pr_debug("[UBOOT]: Uboot options: %s\nOffset: %x", (char*) data, offset );
	if( data == NULL ){
		return -1;
	}
	sscanf(data, "offset=%x", &offset );
	

	return 0;
}

//function to create superblock of ubootfs
static int uboot_fill_sb(struct super_block *sb, void *data, int silent)
{
	int j = 0;
	int i = 0;
	unsigned size = 0;
	struct buffer_head **bh  = NULL;
	int blocks = 0;
	struct inode *root = NULL;
	char *uboot_cache = NULL;
	char *tmp = NULL;
	sb->s_magic = UBOOTFS_MAGIC;
	sb->s_op = &uboot_super_ops;
	sb->s_fs_info = data;
	sb->s_time_gran = 1;
	sb->s_maxbytes = 512;
	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	
	root = new_inode(sb);
	if (!root)
	{
		pr_err("[UBOOT]: error inode allocation failed\n");
		return -ENOMEM;
	}

	root_ino = root->i_ino = get_next_ino();
	root->i_atime = root->i_mtime = root->i_ctime = CURRENT_TIME;
	inode_init_owner(root, NULL, S_IFDIR | 0777 );

	root->i_fop = &s_dir_operations; //&uboot_dir_operations;
	root->i_op = &uboot_dir_inode_operations;
	sb->s_root = d_make_root( root );
	if (!sb->s_root)
	{
		pr_err("[UBOOT]: error root creation failed\n");
		return -ENOMEM;
	}

	sb_min_blocksize(sb, uboot_size);
	blocks = uboot_size / sb->s_blocksize;
	pr_debug("[UBOOT DBG]: Uboot_size: %d, block_size: %lu, blocks_num: %d", uboot_size, sb->s_blocksize, blocks);

	//parse arugments
	parse_options(sb, data );

	bh = kzalloc(sizeof(struct buffer_head)* blocks, GFP_KERNEL );
	if( bh == NULL ){
		pr_err("[UBOOT] eror: **bg = NULL");
		return -ENOMEM; 
	}

	for( i = 0; i < blocks ; ++i )
	{
		bh[i] = sb_bread(sb, i );
		if( bh[i] == NULL )
		{
			pr_debug("bh[%d] =  %p", i, bh[i] );
		}else
		{
			if( bh[i]->b_data[0] == 0 && bh[i]->b_data[3] == 0 )
			{
				//emtpy block
				pr_debug("[UBOOT DBG]: emtpy blok [%d]", i );
				break;
			}
			pr_debug("[UBOOT DBG]: bh[%d] = %p\tsize: %lu\n", i, bh[i], bh[i]->b_size );
		}
	}
	size = bh[0]->b_size *i;
	pr_debug("[UBOOT DBG]: Liczba blokow: %d, rozmiar: %u", i, size );
	uboot_cache = kzalloc( size, GFP_KERNEL );
	sb->s_fs_info = uboot_cache;
	if( uboot_cache == NULL )
	{
		pr_debug("[UBOOT DBG]: uboot_cache == NULL" );
		return -ENOMEM;
	}
	pr_debug("[UBOOT DBG]: buffor uboot cache %p size: %d", uboot_cache, size );
	tmp = uboot_cache;
	for( j = 0; j < i; ++j )
	{
		pr_debug( "[UBOOT DBG]: tmp\t%p\tsize: %lu", tmp, bh[j]->b_size );
		memcpy( tmp, bh[j]->b_data, bh[j]->b_size );
		tmp += bh[j]->b_size;
		brelse( bh[j] );
	}
	kzfree( bh );

	init_files(uboot_cache, size, root);
	return 0;
}


//function called on mount command
static struct dentry *uboot_mount(struct file_system_type *type, int flags,
                                      char const *dev, void *data)
{
	struct dentry *const entry = mount_bdev(type, flags, dev, data, uboot_fill_sb );
	pr_debug( "flags: %d, data: %s", flags, (char*)data );

	if (IS_ERR(entry))
		pr_err("failed to mount ubootfs\n");
	return entry;

}

void uboot_kill_sb(struct super_block *sb)
{
	if(sb->s_root)
		d_genocide(sb->s_root);
	kill_block_super(sb);

}

static struct file_system_type uboot_type = {
	.owner = THIS_MODULE,
	.name = "ubootenv",
	.mount = uboot_mount,
	.kill_sb = uboot_kill_sb,
	.fs_flags = FS_REQUIRES_DEV,
};


static int __init uboot_init(void)
{
	pr_debug("[UBOOT DBG]: uboot module loaded\n");
	return register_filesystem( &uboot_type );
}

static void __exit uboot_exit(void)
{
	pr_debug("[UBOOT DBG]:uboot module exit\n");
	unregister_filesystem( &uboot_type );

}

module_init(uboot_init);
module_exit(uboot_exit);


MODULE_AUTHOR("Kaaml");
MODULE_LICENSE("OWN");
