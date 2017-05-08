#define DEBUG
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include "file.h"

extern int root_ino;
extern struct inode *mfile_inode;

int uboot_dir_open(struct inode *dir, struct file* dfile )
{
	const char* file_name = dfile->f_path.dentry->d_name.name;
	pr_debug("[UBOOT DBG]: uboot dir open [%s]", file_name);
	return dcache_dir_open(dir, dfile);	
}

int uboot_readdir(struct file *file, struct dir_context *ctx)
{
	pr_debug("uboot readdir" );
	dir_emit_dots(file, ctx);
	{
	//	dir_emit(ctx, "nazwa", 6, mfile_inode->i_ino, DT_DIR );
	}
	return 0;
}

static int uboot_fill_file(struct seq_file *m, void *v )
{
	const char* file_name = m->file->f_path.dentry->d_name.name;	
	pr_debug("[UBOOT DBG]: fill file dataptr: %p file_name: [%s]", m->private, file_name);
	seq_printf(m, "%s\n", (char*)m->private);
	return 0;
}

static int uboot_file_open(struct inode *inode, struct file *file )
{
	pr_debug("uboo_file_open %p", inode->i_private );
	return single_open(file, uboot_fill_file, inode->i_private );
}

static ssize_t uboot_file_write(struct file *file, const char __user* u, size_t len, loff_t *ua )
{
	const char* file_name = file->f_path.dentry->d_name.name;
//	pr_debug( "[UBOOT DBG]: File write file: [%s], file_private: %p", file_name, file->f_inode->i_private );
	pr_debug( "[UBOOT DBG]: File content: [%s], len: %lu", (char*) u, len );
	return len;
}

const struct file_operations uboot_dir_operations = {
	.open 		= dcache_dir_open,
	.release 	= dcache_dir_close,
	.read		= generic_read_dir,
	.iterate_shared = uboot_readdir,
	.fsync = noop_fsync
};

const struct file_operations uboot_file_operations = {
	.open		= uboot_file_open,
	.release	= single_release,
	.read		= seq_read,
	.write		= uboot_file_write,
//	.read_iter 	= generic_file_read_iter,
//	.write_iter = generic_file_write_iter,
	.fsync		= noop_fsync,
	.splice_read = generic_file_splice_read,
	.splice_write = iter_file_splice_write,
	.llseek		= generic_file_llseek,
	.write_iter = generic_file_write_iter,
	.mmap		= generic_file_mmap

//	.release = moja_funkcja_do_release
};

const struct inode_operations uboot_file_inode_operations = {
	.getattr = simple_getattr, 
	.setattr = simple_setattr
};

struct inode * uboot_create_inode(struct super_block * sb)
{
	struct inode *inode = new_inode(sb);
	if(!inode)
	{
		pr_debug("nie mozna stworzyc inode\n");
		return NULL;
	}
	inode->i_ino = get_next_ino();
//	inode->i_mapping->a_ops = //default operations?
	return inode;
}

struct inode * uboot_create_reg_inode(struct inode *dir, mode_t mode )
{
	struct inode *inode = new_inode(dir->i_sb);
	if( !inode )
		return NULL;

	inode->i_ino = get_next_ino();
	//inode->init_time(inode, dir->i_mtime = dir->i_ctime = CURRENT_TIME );
	inode_init_owner(inode, dir, S_IFREG|mode );

//	inode->i_op = 
	inode->i_fop = &uboot_file_operations;
	return inode;
}
