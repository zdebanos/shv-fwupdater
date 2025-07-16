#include <shv/tree/shv_tree.h>
#include <shv/tree/shv_file_com.h>
#include <shv/tree/shv_connection.h>
#include <shv/tree/shv_methods.h>

#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <sys/ioctl.h>

#ifdef __NuttX__
#include <nxboot.h>
#include <nuttx/mtd/mtd.h>
#include <sys/boardctl.h>
#endif

static atomic_bool running;

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

// ------------------------- fwUpdate METHODS ------------------------------ //


int shv_file_crc(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  int ret;
  shv_file_node_t *file_node = (shv_file_node_t*) item;
  ret = shv_file_process_crc(shv_ctx, rid, file_node);
  if (ret < 0)
    {
      shv_send_response_error(shv_ctx, rid, SHV_RE_PLATFORM_ERROR);
    }
  else
    {
      shv_file_confirm_crc(shv_ctx, rid, file_node);
    }
  return ret;
}

int shv_file_write(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  int ret = 0;
  shv_file_node_t *file_node = (shv_file_node_t*) item;
  ret = shv_file_process_write(shv_ctx, rid, file_node);
  if (ret < 0)
    {
      shv_send_response_error(shv_ctx, rid, SHV_RE_PLATFORM_ERROR);
    }
  else
    {
      shv_file_confirm_write(shv_ctx, rid, file_node);
    }
  return 0;
}

int shv_file_stat(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_file_node_t *file_node = (shv_file_node_t*) item;
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_file_send_stat(shv_ctx, rid, file_node);
  return 0;
}

int shv_file_size(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_file_node_t *file_node = (shv_file_node_t*) item;
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_file_send_size(shv_ctx, rid, file_node);
  return 0;
}

const shv_method_des_t shv_dev_fwUpdate_dmap_item_crc =
{
  .name = "crc",
  .method = shv_file_crc
};

const shv_method_des_t shv_dev_fwUpdate_dmap_item_write =
{
  .name = "write",
  .method = shv_file_write
};

const shv_method_des_t shv_dev_fwUpdate_dmap_item_stat =
{
  .name = "stat",
  .method = shv_file_stat
};

const shv_method_des_t shv_dev_fwUpdate_dmap_item_size =
{
  .name = "size",
  .method = shv_file_size
};

const shv_method_des_t * const shv_dev_fwUpdate_dmap_items[] =
{
  &shv_dev_fwUpdate_dmap_item_crc,
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
  &shv_dev_fwUpdate_dmap_item_size,
  &shv_dev_fwUpdate_dmap_item_stat,
  &shv_dev_fwUpdate_dmap_item_write
};

const shv_dmap_t shv_dev_fwUpdate_dmap =
  SHV_CREATE_NODE_DMAP(fwUpdate, shv_dev_fwUpdate_dmap_items);

// ------------------------- .dotdevice METHODS ---------------------------- //

int shv_dotdevice_name(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_str(shv_ctx, rid, "nx-shv-fwupdater");
  return 0;
}

int shv_dotdevice_version(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_str(shv_ctx, rid, "0.1.0");
  return 0;
}

int shv_dotdevice_uptime(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);

  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_int(shv_ctx, rid, t.tv_sec);
  return 0;
}

int shv_dotdevice_reset(shv_con_ctx_t *shv_ctx, shv_node_t *item, int rid)
{
  shv_unpack_data(&shv_ctx->unpack_ctx, 0, 0);
  shv_send_int(shv_ctx, rid, 0);
  printf("Performing reset in 2 seconds!\n");
  fflush(stdout);
  usleep(2000 * 1000);
#ifdef CONFIG_SHV_LIBS4C_PLATFORM_NUTTX
  boardctl(BOARDIOC_RESET, BOARDIOC_RESETCAUSE_CPU_SOFT);
#endif
  /* On linux, this should do nothing. */
  return 0;
}

const shv_method_des_t shv_dev_dotdevice_dmap_item_name =
{
  .name = "name",
  .method = shv_dotdevice_name
};

const shv_method_des_t shv_dev_dotdevice_dmap_item_version =
{
  .name = "version",
  .method = shv_dotdevice_version
};

const shv_method_des_t shv_dev_dotdevice_dmap_item_uptime =
{
  .name = "uptime",
  .method = shv_dotdevice_uptime
};

const shv_method_des_t shv_dev_dotdevice_dmap_item_reset =
{
  .name = "reset",
  .method = shv_dotdevice_reset
};

const shv_method_des_t * const shv_dev_dotdevice_dmap_items[] =
{
  &shv_dmap_item_dir,
  &shv_dmap_item_ls,
  &shv_dev_dotdevice_dmap_item_name,
  &shv_dev_dotdevice_dmap_item_reset,
  &shv_dev_dotdevice_dmap_item_uptime,
  &shv_dev_dotdevice_dmap_item_version
};

const shv_dmap_t shv_dev_dotdevice_dmap =
  SHV_CREATE_NODE_DMAP(dotdevice, shv_dev_dotdevice_dmap_items);

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
  shv_node_t *tree_root, *dotdevice_node, *fwStable_node;
  shv_file_node_t *fwUpdate_node;
  struct mtd_geometry_s geometry;

#ifdef __NuttX__
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
      close(flash_fd);
      return NULL;
    }

  fwUpdate_node = shv_tree_file_node_new("fwUpdate",
                                         &shv_dev_fwUpdate_dmap, 0);
  if (fwUpdate_node == NULL)
    {
      close(flash_fd);
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

  fwUpdate_node->file_type = REGULAR;
#ifdef __NuttX__
  fwUpdate_node->name = "*NXBOOT_MTD*";
  fwUpdate_node->file_maxsize = geometry.erasesize * geometry.neraseblocks;
  fwUpdate_node->file_pagesize = geometry.blocksize;
#else
  fwUpdate_node->name = "./test.bin";
  fwUpdate_node->file_maxsize = 1 << 25; /* 32 MiB */
  fwUpdate_node->file_pagesize = 1024;
#endif
  shv_tree_add_child(tree_root, &fwUpdate_node->shv_node);
#ifdef __NuttX__
  close(flash_fd);
#endif

  dotdevice_node = shv_tree_node_new(".device", &shv_dev_dotdevice_dmap, 0);
  if (dotdevice_node == NULL)
    {
      free(tree_root);
      free(fwUpdate_node);
      return NULL;
    }
  shv_tree_add_child(tree_root, dotdevice_node);

  fwStable_node = shv_tree_node_new("fwStable", &shv_dev_fwStable_dmap, 0);
  if (fwStable_node == NULL)
    {
      free(tree_root);
      free(fwUpdate_node);
      return NULL;
    }
  shv_tree_add_child(tree_root, fwStable_node);

  return tree_root;
}

int get_priority_for_com(void)
{
  return 99;
}

static void quit_handler(int signum)
{
  puts("Stopping SHV FW Updater!");
  atomic_store(&running, false);
}

int main(int argc, char *argv[])
{
  printf("Faulty firmware!\n");
  /* Define the SHV Communication parameters */

  struct shv_connection connection;
  shv_node_t *tree_root;
  shv_con_ctx_t *ctx;

  /* Initalize the communication */

  shv_connection_init(&connection, SHV_TLAYER_TCPIP);
  connection.broker_user =     "mzapoknobs";
  connection.broker_password = "d4268ee1bdb5605b4c";
  connection.broker_mount =    "test/shvlibs4c";
  if (shv_connection_tcpip_init(&connection, "147.32.87.165", 3755) < 0)
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
  ctx = shv_com_init(tree_root, &connection);
  if (ctx == NULL)
    {
      fprintf(stderr, "Can't establish the comm with the broker.\n");
      return 1;
    }

  puts("Alloc Root Ok! The connection should start");

  atomic_store(&running, true);
  signal(SIGTERM, quit_handler);

  while (atomic_load(&running))
    {
      /* Wake up every 2 seconds to check if the service should still run */

      usleep(2e6);
    }

  puts("Close the communication");
  shv_com_close(ctx);

  return 0;
}
