#include <shv/tree/shv_tree.h>
#include <shv/tree/shv_file_node.h>
#include <shv/tree/shv_connection.h>
#include <shv/tree/shv_methods.h>
#include <shv/tree/shv_clayer_posix.h>
#include <shv/tree/shv_dotdevice_node.h>

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <semaphore.h>

#ifdef __NuttX__
#include <nxboot.h>
#include <nuttx/mtd/mtd.h>
#include <sys/boardctl.h>
#endif

static sem_t running;

/* Custom definitions for the file node (NXBoot) */

#ifdef __NuttX__
static int shv_nxboot_opener(shv_file_node_t *item)
{
  struct shv_file_node_fctx *fctx = (struct shv_file_node_fctx*) item->fctx;
  if (!(fctx->flags & SHV_FILE_POSIX_BITFLAG_OPENED))
    {
      fctx->fd = nxboot_open_update_partition();
      if (fctx->fd < 0)
        {
          return -1;
        }
      fctx->flags |= SHV_FILE_POSIX_BITFLAG_OPENED;
    }
  return 0;
}
#endif

// ------------------------- ROOT METHODS ---------------------------------- //

int shv_root_device_type(shv_con_ctx_t * shv_ctx, shv_node_t *item, int rid)
{
  const char *str = "SHV4LIBS Testing";
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_str(shv_ctx, rid, str);
  return 0;
}

const shv_method_des_t shv_dev_root_dmap_item_device_type =
{
  .name = "deviceType",
  .method = shv_root_device_type
};

const shv_method_des_t * const shv_dev_root_dmap_items[] =
{
  &shv_dev_root_dmap_item_device_type,
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
};

const shv_dmap_t shv_dev_root_dmap =
  SHV_CREATE_NODE_DMAP(root, shv_dev_root_dmap_items);

// ------------------------- .app METHODS ---------------------------- //

int shv_dotapp_vmajor(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_int(shv_ctx, rid, 1);
  return 0;
}

int shv_dotapp_vminor(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_int(shv_ctx, rid, 0);
  return 0;
}

int shv_dotapp_name(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_str(shv_ctx, rid, "SHV Firmware Updater");
  return 0;
}

int shv_dotapp_ping(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_empty_response(shv_ctx, rid);
  return 0;
}

const shv_method_des_t shv_dev_dotapp_dmap_item_vmajor =
{
  .name = "shvVersionMajor",
  .method = shv_dotapp_vmajor
};

const shv_method_des_t shv_dev_dotapp_dmap_item_vminor =
{
  .name = "shvVersionMinor",
  .method = shv_dotapp_vminor
};

const shv_method_des_t shv_dev_dotapp_dmap_item_name =
{
  .name = "name",
  .method = shv_dotapp_name
};

const shv_method_des_t shv_dev_dotapp_dmap_item_ping =
{
  .name = "ping",
  .method = shv_dotapp_ping
};

const shv_method_des_t * const shv_dev_dotapp_dmap_items[] =
{
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
  &shv_dev_dotapp_dmap_item_name,
  &shv_dev_dotapp_dmap_item_ping,
  &shv_dev_dotapp_dmap_item_vmajor,
  &shv_dev_dotapp_dmap_item_vminor,
};

const shv_dmap_t shv_dev_dotapp_dmap =
  SHV_CREATE_NODE_DMAP(dotapp, shv_dev_dotapp_dmap_items);


// ------------------------- fwStable METHODS ---------------------------- //

int shv_fwStable_confirm(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  /* On linux, this should  do nothing. */
#ifdef __NuttX__
  nxboot_confirm();
#endif
  printf("New firmware confirmed!\n");
  fflush(stdout);
  shv_send_int(shv_ctx, rid, 0);
  return 0;
}

const shv_method_des_t shv_dev_fwStable_dmap_item_confirm =
{
  .name = "confirm",
  .method = shv_fwStable_confirm
};

const shv_method_des_t * const shv_dev_fwStable_dmap_items[] =
{
  &shv_dev_fwStable_dmap_item_confirm,
  &shv_dmap_item_dir,
  &shv_dmap_item_ls
};

const shv_dmap_t shv_dev_fwStable_dmap =
  SHV_CREATE_NODE_DMAP(dotdevice, shv_dev_fwStable_dmap_items);

shv_node_t *shv_tree_create(void)
{
  shv_node_t *tree_root, *fwStable_node, *dotapp_node;
  shv_dotdevice_node_t *dotdevice_node;
  shv_file_node_t *fwUpdate_node;

#ifdef __NuttX__
  struct mtd_geometry_s geometry;
  int flash_fd;
  flash_fd = nxboot_open_update_partition();
  if (flash_fd < 0)
    {
      return NULL;
    }
#endif

  puts("Creating the SHV Tree root");
  tree_root = shv_tree_node_new("", &shv_dev_root_dmap, 0);
  if (tree_root == NULL)
    {
#ifdef __NuttX__
      close(flash_fd);
#endif
      return NULL;
    }

  fwUpdate_node = shv_tree_file_node_new("fwUpdate",
                                         &shv_file_node_dmap, 0);
  if (fwUpdate_node == NULL)
    {
#ifdef __NuttX__
      close(flash_fd);
#endif
      free(tree_root);
      return NULL;
    }

#ifdef __NuttX__
  if (ioctl(flash_fd, MTDIOC_GEOMETRY,
            (unsigned long)((uintptr_t)&geometry)) < 0)
    {
      close(flash_fd);
      free(tree_root);
      free(fwUpdate_node);
      return NULL;
    }
#endif

  fwUpdate_node->file_type = SHV_FILE_MTD;
#ifdef __NuttX__
  fwUpdate_node->file_maxsize = geometry.erasesize * geometry.neraseblocks;
  fwUpdate_node->file_pagesize = geometry.blocksize;
  fwUpdate_node->file_erasesize = geometry.erasesize;
  /* Update the fops table in the file node */

  fwUpdate_node->fops.opener = shv_nxboot_opener;
#else
  fwUpdate_node->name = "./test.bin";
  fwUpdate_node->file_maxsize = 1 << 25; /* 32 MiB */
  fwUpdate_node->file_pagesize = 1024;
  fwUpdate_node->file_erasesize = 4096;
#endif
  shv_tree_add_child(tree_root, &fwUpdate_node->shv_node);
#ifdef __NuttX__
  close(flash_fd);
#endif

  dotapp_node = shv_tree_node_new(".app", &shv_dev_dotapp_dmap, 0);
  if (dotapp_node == NULL)
    {
      free(tree_root);
      free(fwUpdate_node);
      return NULL;
    }
  shv_tree_add_child(tree_root, dotapp_node);

  dotdevice_node = shv_tree_dotdevice_node_new(&shv_dotdevice_dmap, 0);
  if (dotdevice_node == NULL)
    {
      free(tree_root);
      free(fwUpdate_node);
      free(dotapp_node);
      return NULL;
    }
  dotdevice_node->name = "SHV Compatible Device";
  dotdevice_node->serial_number = "0xDEADBEEF";
  dotdevice_node->version = "0.1.0";
  shv_tree_add_child(tree_root, &dotdevice_node->shv_node);

  fwStable_node = shv_tree_node_new("fwStable", &shv_dev_fwStable_dmap, 0);
  if (fwStable_node == NULL)
    {
      free(tree_root);
      free(fwUpdate_node);
      free(dotapp_node);
      return NULL;
    }
  shv_tree_add_child(tree_root, fwStable_node);

  return tree_root;
}

static void quit_handler(int signum)
{
  puts("Stopping SHV FW Updater!");
  sem_post(&running);
}

static void print_help(char *name)
{
  printf("%s: <user> <passwd> <mnt-point> <ip-addr> <tcp/ip-port>\n", name);
}

static void attention_cb(shv_con_ctx_t *shv_ctx, enum shv_attention_reason r)
{
  if (r == SHV_ATTENTION_ERROR)
    {
      printf("Error occured in SHV, the reason is: %s\n",
             shv_errno_str(shv_ctx));
      sem_post(&running);
    }
}

int main(int argc, char *argv[])
{
  /* Define the SHV Communication parameters */

  int ret;
  struct shv_connection connection;
  shv_node_t *tree_root;
  shv_con_ctx_t *ctx;

  /* Initalize the communication. But only if parameters are passed. */

  if (argc != 6)
    {
      print_help(argv[0]);
      return 1;
    }

  const char *user = argv[1];
  const char *passwd = argv[2];
  const char *mount = argv[3];
  const char *ip = argv[4];
  const char *port_s = argv[5];
  int port = atoi(port_s);

  shv_connection_init(&connection, SHV_TLAYER_TCPIP);
  connection.broker_user =     user;
  connection.broker_password = passwd;
  connection.broker_mount =    mount;
  connection.reconnect_period = 10;
  connection.reconnect_retries = 0;
  if (shv_connection_tcpip_init(&connection, ip, port) < 0)
    {
      fprintf(stderr, "Have you supplied valid params to shv_connection?\n");
      return 1;
    }
  puts("SHV Connection Init OK");

  tree_root = shv_tree_create();
  if (tree_root == NULL)
    {
      fprintf(stderr, "Can't create the SHV tree.");
      return 1;
    }

  puts("SHV Tree created!");
  ctx = shv_com_init(tree_root, &connection, attention_cb);
  if (ctx == NULL)
    {
      fprintf(stderr, "Can't establish the comm with the broker.\n");
      return 1;
    }

  ret = shv_create_process_thread(99, ctx);
  if (ret < 0)
    {
      fprintf(stderr, "%s\n", shv_errno_str(ctx));
      free(ctx);
      return 1;
    }

  sem_init(&running, 0, 0);
  signal(SIGTERM, quit_handler);

  sem_wait(&running);

  puts("Close the communication");
  shv_com_destroy(ctx);

  return 0;
}
