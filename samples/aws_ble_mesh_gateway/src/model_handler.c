
#include <zephyr/bluetooth/mesh.h>
#include "model_handler.h"

#include <bluetooth/mesh/models.h>

/* SIG models */

static struct bt_mesh_cfg_cli cfg_cli = {};

static void attention_on(struct bt_mesh_model *model)
{
    // TODO: implement
    printk("attention_on()\n");
}

static void attention_off(struct bt_mesh_model *model)
{
    // TODO: implement
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

/**
 * @brief Handle status message returned by a movement server after sending
 * a ```movement set``` command.
 *
 * @param model Model instance receiving the message
 * @param ctx Message context
 * @param buf Message buffer containing the payload, not including the opcode
 * @return 0 on success, negative ERRNO code otherwise
 */
static int movement_set_status_msg_handler(struct bt_mesh_model *model,
                                       struct bt_mesh_msg_ctx *ctx,
                                       struct net_buf_simple *buf)
{
    // TODO: implement. Should call a callback function that is registered by the user.
    printk("Movement set status message received\n");
    return 0;
}

/**
 * @brief Handle status message returned by a movement server after completing a movement.
 *
 * @param model Model instance receiving the message
 * @param ctx Message context
 * @param buf Message buffer containing the payload, not including the opcode
 * @return 0 on success, negative ERRNO code otherwise
 */
static int movement_done_status_msg_handler(struct bt_mesh_model *model,
                                       struct bt_mesh_msg_ctx *ctx,
                                       struct net_buf_simple *buf)
{
    // TODO: implement. Should call a callback function that is registered by the user.
    printk("Movement done status message received\n");
    return 0;
}

static const struct bt_mesh_model_op movement_client_ops[] = {
    {
        OP_VND_ROBOT_MOVEMENT_SET_STATUS,
        BT_MESH_LEN_EXACT(sizeof(struct robot_movement_set_status_msg)),
        movement_set_status_msg_handler,
    },
    {
        OP_VND_ROBOT_MOVEMENT_DONE_STATUS,
        BT_MESH_LEN_EXACT(sizeof(struct robot_movement_done_status_msg)),
        movement_done_status_msg_handler,
    },
    BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_model vendor_models[] = {
    BT_MESH_MODEL_VND(CONFIG_BT_COMPANY_ID, ROBOT_MOVEMENT_CLI_MODEL_ID, movement_client_ops, NULL, NULL),
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

const struct bt_mesh_comp *model_handler_init(void)
{

    return &comp;
}