#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/mesh/msg.h>

#include "../../mesh_model_defines/robot_movement_srv.h"
#include "model_handler.h"

/* Application handler functions */
movement_received_handler_t app_movement_handler;
start_movement_handler_t app_start_movement_handler;

/* SIG models */

static struct bt_mesh_cfg_cli cfg_cli = {};

static void attention_on(struct bt_mesh_model *model)
{
    printk("attention_on()\n");
}

static void attention_off(struct bt_mesh_model *model)
{
    printk("attention_off()\n");
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
    .attn_on = attention_on,
    .attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
    .cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_model sig_models[] = {
    BT_MESH_MODEL_CFG_SRV,
    BT_MESH_MODEL_CFG_CLI(&cfg_cli),
    BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
};

/* Vendor models */

static int movement_config_recieved(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    struct robot_movement_config *mov_conf = (struct robot_movement_config *)buf->data;
    int err = 0;
    if (app_movement_handler != NULL)
    {
        app_movement_handler(mov_conf);
    }
    return err;
}

static int start_movement_recieved(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    if (app_start_movement_handler != NULL){
        app_start_movement_handler();
    }
    return 0;
}

static const struct bt_mesh_model_op movement_server_ops[] = {
    {OP_VND_ROBOT_MOVEMENT_SET, BT_MESH_LEN_EXACT(sizeof(struct robot_movement_set_msg)), movement_config_recieved},
    {OP_VND_ROBOT_CLEAR_TO_MOVE, BT_MESH_LEN_EXACT(0), start_movement_recieved},
    BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_model vendor_models[] = {
    BT_MESH_MODEL_VND(CONFIG_BT_COMPANY_ID, ROBOT_MOVEMENT_SRV_MODEL_ID, movement_server_ops, NULL, NULL),
};

/* Composition */
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, sig_models, vendor_models),
};

static struct bt_mesh_comp comp = {
    .cid = CONFIG_BT_COMPANY_ID,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(movement_received_handler_t movement_handler,
    start_movement_handler_t start_movement_handler)
{
    app_movement_handler = movement_handler;
    app_start_movement_handler = start_movement_handler;
    return &comp;
}