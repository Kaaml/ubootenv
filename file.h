#define DEBUG
#include <linux/slab.h>
//#include <linux/pagemap.h>
//#include <linux/version.h>




struct inode * uboot_create_inode(struct super_block * sb ) ;
//struct inode * uboot_create_inode(struct super_block *sb);
int uboot_readdir(struct file *file, struct dir_context *ctx );
int uboot_dir_open(struct inode *inode, struct file* file );
struct inode * uboot_create_reg_inode(struct inode *dir, mode_t mode );

extern const struct file_operations uboot_dir_operations;
extern const struct file_operations uboot_file_operations;

extern const struct inode_operations uboot_file_inode_operations;
